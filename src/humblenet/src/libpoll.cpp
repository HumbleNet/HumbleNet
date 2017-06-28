#ifndef EMSCRIPTEN

#include "libpoll.h"

#include <cassert>
#include <errno.h>

#ifndef WIN32
#include <pthread.h>
#endif

extern "C" {
#include <ILibAsyncSocket.h>
#include <ILibParsers.h>
#include <ILibWebRTC.h>
#include <ILibWrapperWebRTC.h>
}  // extern "C"

#define LOG printf

extern "C" {
	extern int ILibChainLock_RefCounter;
	extern sem_t ILibChainLock;
}

#ifdef WIN32

#define mutex_t CRITICAL_SECTION
void mutex_init(mutex_t* m) {
	InitializeCriticalSectionAndSpinCount( m, 2000 );
}

void mutex_destroy(mutex_t* m) {
	DeleteCriticalSection( m );
}
void mutex_wait(mutex_t* m) {
	EnterCriticalSection( m );
}

int mutex_trywait(mutex_t* m) {
	return TryEnterCriticalSection( m ) != 0 ? 0 : 1;
}

int mutex_timedlock(mutex_t* m, long ms) {
	int ret = TryEnterCriticalSection( m );
	while (ret ==0 && ms > 0) {
		ms -= 10;
		Sleep( 10 );
		ret = TryEnterCriticalSection( m );
	}
	return ret != 0 ? 0 : 1;
}

void mutex_post(mutex_t* m) {
	LeaveCriticalSection( m );
}

#elif _POSIX_TIMEOUTS > 0

#include <pthread.h>

#define mutex_t pthread_mutex_t
void mutex_init(mutex_t* m) {
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE );

	pthread_mutex_init(m, &attr);
}

int mutex_timedlock(mutex_t* m, long ms ) {
	struct timespec ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = 1000 * (ms % 1000);

	return pthread_mutex_timedlock( m, &ts );
}

#define mutex_destroy(x) pthread_mutex_destroy(x)
#define mutex_wait(x) pthread_mutex_lock(x)
#define mutex_trywait(x) pthread_mutex_trylock(x)
#define mutex_post(x) pthread_mutex_unlock(x)

#else

#include <pthread.h>

struct mutex_t {
	mutex_t()
	:thread(0)
	,depth(0)
	{
		pthread_mutexattr_t attr;

		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE );

		pthread_mutex_init(&m, &attr);
	}

	int wait() {
		return was_locked( pthread_mutex_lock(&m) );
	}

	int trywait() {
		return was_locked( pthread_mutex_trylock(&m) );
	}

	int timedwait(long ms) {
		if( ms == 0 )
			return trywait();

		int ret = 0;
		do
		{
			ret = pthread_mutex_trylock(&m);
			if( !ret )
				break;
			usleep(1000); // 1ms
		}while( --ms > 0 );

		return was_locked( ret );
	}

	void post() {
		assert( thread == pthread_self() );
		assert( depth > 0 );

		if( ! depth-- ) {
			thread = 0;
		}

		int ret = pthread_mutex_unlock(&m);

		assert( ret == 0 );
	}

	~mutex_t() {
		pthread_mutex_destroy(&m);
	}
protected:
	int was_locked( int ret ) {
		if( ret == 0 ) {
			depth ++;
			thread = pthread_self();
		} else {
			char self_name[64] = {0};
			char other_name[64] = {0};

			get_thread_name( pthread_self(), self_name, sizeof(self_name) );
			get_thread_name( thread, other_name, sizeof(other_name) );

			LOG("Lock failed on thread %s, error %s(%d), currently owner by %s", self_name, strerror(ret), ret, other_name );
		}
		return ret;
	}

	static void get_thread_name( pthread_t t, char* buf, size_t size ) {
		if( t == 0 ) {
			strncpy(buf, "<none>", size);
		} else {
			pthread_getname_np( t, buf, size );
			if( ! buf[0] )
				strncpy(buf, "Thread",size);

			int len = strlen(buf);
			// add the id..

			uint64_t id;
			pthread_threadid_np(t, &id);
			sprintf(buf+len, "(%lld)", id);
		}
	}
private:
	pthread_mutex_t m;
	pthread_t thread;
	volatile int depth;
};

#define mutex_init(x)           (0)
#define mutex_wait(x)           (x)->wait()
#define mutex_trywait(x)        (x)->trywait()
#define mutex_timedlock(x,ms)   (x)->timedwait(ms)
#define mutex_post(x)           (x)->post()
#define mutex_destroy(x)        (0)

#endif

static mutex_t PollLock;

#define LOCK_INIT()     mutex_init(&PollLock)
#define LOCK_WAIT()     mutex_wait(&PollLock)
#define LOCK_TRY_WAIT() (mutex_trywait(&PollLock) == 0)
#define LOCK_TIMED_WAIT( ms ) (mutex_timedlock(&PollLock, ms) == 0)
#define LOCK_RELEASE()  mutex_post(&PollLock)
#define LOCK_DESTROY()  mutex_destroy(&PollLock)


#define FULL_ASYNC


//
//
// ILibChain code adapted from Microstack to allow incremental polling instead of a dedicated thread.
//
//
struct ILibChain {
	ILibChain_PreSelect PreSelect;
	ILibChain_PostSelect PostSelect;
	ILibChain_Destroy Destroy;
};

struct ILibLinkedListNode_Root
{
	sem_t LOCK;
	long count;
	void* Tag;
	struct ILibLinkedListNode *Head;
	struct ILibLinkedListNode *Tail;
};


typedef struct ILibBaseChain
{
	int TerminateFlag;
	int RunningFlag;
#ifdef _REMOTELOGGING
	ILibRemoteLogging ChainLogger;
#ifdef _REMOTELOGGINGSERVER
	void* LoggingWebServer;
#endif
#endif
	/*
	 *
	 * DO NOT MODIFY STRUCT DEFINITION ABOVE THIS COMMENT BLOCK
	 *
	 */
	
	
#if defined(WIN32)
	DWORD ChainThreadID;
#else
	pthread_t ChainThreadID;
#endif
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET Terminate;
#else
	FILE *TerminateReadPipe;
	FILE *TerminateWritePipe;
#endif
	
	void *Timer;
	void *Reserved;
	ILibLinkedList Links;
	// ILibLinkedList LinksPendingDelete;
	ILibLinkedListNode_Root* LinksPendingDelete;
	ILibHashtable ChainStash;
}ILibBaseChain;


// This sets up an interuptable select call.
int ILibInterruptibleSelect(void* Chain, int fds, fd_set& readset, fd_set& writeset,
			   fd_set& errorset, struct timeval& tv ) {
	
#if !defined(WIN32) && !defined(_WIN32_WCE)
	static int TerminatePipe[2] = {0,0};
	int flags;
#endif

	
#if !defined(WIN32) && !defined(_WIN32_WCE)
	// Since this routine is not infite anymore, only initialize once
	//    if( ((struct ILibBaseChain*)Chain)->TerminateReadPipe == NULL ) {
	if( TerminatePipe[0] == 0 ) {
		//
		// For posix, we need to use a pipe to force unblock the select loop
		//
		if (pipe(TerminatePipe) == -1) {} // TODO: Pipe error
		flags = fcntl(TerminatePipe[0],F_GETFL,0);
		//
		// We need to set the pipe to nonblock, so we can blindly empty the pipe
		//
		fcntl(TerminatePipe[0],F_SETFL,O_NONBLOCK|flags);
		((struct ILibBaseChain*)Chain)->TerminateReadPipe = fdopen(TerminatePipe[0],"r");
		((struct ILibBaseChain*)Chain)->TerminateWritePipe = fdopen(TerminatePipe[1],"w");
		//    } else {
		//        TerminatePipe[0] = fileno( ((struct ILibBaseChain*)Chain)->TerminateReadPipe );
		//        TerminatePipe[1] = fileno( ((struct ILibBaseChain*)Chain)->TerminateWritePipe );
	}
#endif
	
#if defined(WIN32) || defined(_WIN32_WCE)
	//
	// Check the fake socket, for ILibForceUnBlockChain
	//
	if (((struct ILibBaseChain*)Chain)->Terminate == ~0)
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}
	else
	{
#pragma warning( push, 3 ) // warning C4127: conditional expression is constant
		//      FD_SET(((struct ILibBaseChain*)Chain)->Terminate, &errorset);
#pragma warning( pop )
	}
#else
	//
	// Put the Read end of the Pipe in the FDSET, for ILibForceUnBlockChain
	//
	FD_SET(TerminatePipe[0], &readset);
#endif

	int slct = select(FD_SETSIZE, &readset, &writeset, &errorset, &tv);
	
	
#if defined(WIN32) || defined(_WIN32_WCE)
	//
	// Reinitialise our fake socket if necessary
	//
	if (((struct ILibBaseChain*)Chain)->Terminate == ~0)
	{
		((struct ILibBaseChain*)Chain)->Terminate = socket(AF_INET, SOCK_DGRAM, 0);
	}
#else
	if (FD_ISSET(TerminatePipe[0], &readset))
	{
		//
		// Empty the pipe
		//
		int i = 0;
		while (fgetc(((struct ILibBaseChain*)Chain)->TerminateReadPipe)!=EOF)
		{
			i++;
		}
		LOG("select interupted (%d)\n", i);
	}
#endif

	
	return slct;
}

int ILibIterateChain(void *_Chain, fd_set& readset, fd_set& writeset,
					 fd_set& errorset, struct timeval& tv ) {
	ILibBaseChain* Chain = (ILibBaseChain*)_Chain;
	void* node;
	ILibChain *module;
	
	// Prevent execution if already in the loop.
	if( ((struct ILibBaseChain*)Chain)->RunningFlag ) {
		return -2;
	}
	
	// This prevents issues with recursion.
	if( ! LOCK_TRY_WAIT() )
		return -2;
	
	int slct;
	int v;
	
#if defined(WIN32)
	((ILibBaseChain*)Chain)->ChainThreadID = GetCurrentThreadId();
#else
	((ILibBaseChain*)Chain)->ChainThreadID = pthread_self();
#endif
	
	if (gILibChain == NULL) {gILibChain = Chain;} // Set the global instance if it's not already set
	
	((struct ILibBaseChain*)Chain)->RunningFlag = 1;
	
	// Only run through once
	//while (((struct ILibBaseChain*)Chain)->TerminateFlag == 0)
	{
		slct = 0;
		//
		// Iterate through all the PreSelect function pointers in the chain
		//
		node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
		v = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
		while(node!=NULL && (module=(ILibChain*)ILibLinkedList_GetDataFromNode(node))!=NULL)
		{
			if(module->PreSelect != NULL)
			{
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
				module->PreSelect((void*)module, &readset, &writeset, &errorset, &v);
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
			}
			node = ILibLinkedList_GetNextNode(node);
		}
		
		LOCK_RELEASE();
		
		tv.tv_sec =  v / 1000;
		tv.tv_usec = 1000 * (v % 1000);
		
		sem_wait(&ILibChainLock);
		
		while(ILibLinkedList_GetCount(((ILibBaseChain*)Chain)->LinksPendingDelete) > 0)
		{
			node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->LinksPendingDelete);
			module = (ILibChain*)ILibLinkedList_GetDataFromNode(node);
			ILibLinkedList_Remove_ByData(((ILibBaseChain*)Chain)->Links, module);
			ILibLinkedList_Remove(node);
			if(module->Destroy != NULL) {module->Destroy((void*)module);}
			free(module);
		}
		
		sem_post(&ILibChainLock);
		
		//
		// The actual Select Statement
		//
		//slct = select(FD_SETSIZE, &readset, &writeset, &errorset, &tv);
		slct = ILibInterruptibleSelect(Chain, FD_SETSIZE, readset, writeset, errorset, tv);
		if (slct == -1)
		{
			//
			// If the select simply timed out, we need to clear these sets
			//
			FD_ZERO(&readset);
			FD_ZERO(&writeset);
			FD_ZERO(&errorset);
		}
		
		LOCK_WAIT();
		
		//
		// Iterate through all of the PostSelect in the chain
		//
		node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
		while(node!=NULL && (module=(ILibChain*)ILibLinkedList_GetDataFromNode(node))!=NULL)
		{
			if (module->PostSelect != NULL)
			{
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
				module->PostSelect((void*)module, slct, &readset, &writeset, &errorset);
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
			}
			node = ILibLinkedList_GetNextNode(node);
		}
		
		((struct ILibBaseChain*)Chain)->RunningFlag = 0;
		
		LOCK_RELEASE();
	}
	
	return slct;
}

void ILibIterateChain(void *_Chain, int timeout_ms ) {
	ILibBaseChain* Chain = (ILibBaseChain*)_Chain;
	
	// Prevent execution if already in the loop.
	if( ((struct ILibBaseChain*)Chain)->RunningFlag ) {
		return;
	}
	
	struct timeval tv;
	
	fd_set readset;
	fd_set errorset;
	fd_set writeset;
	
	FD_ZERO(&readset);
	FD_ZERO(&errorset);
	FD_ZERO(&writeset);
	
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = timeout_ms % 1000;
	
	ILibIterateChain(_Chain, readset, writeset, errorset, tv );
}

void ILibDestroyChain(void *Chain) {
	void* node;
	ILibChain* module;
	//
	// This loop will start, when the Chain was signaled to quit. Clean up the chain by iterating
	// through all the Destroy methods. Not all modules in the chain will have a destroy method,
	// but call the ones that do.
	//
	// Because many modules in the chain are using the base chain timer which is the first node
	// in the chain in the base timer), these modules may start cleaning up their timers. So, we
	// are going to make an exception and Destroy the base timer last, something that would usualy
	// be destroyed first since it's at the start of the chain. This will allow modules to clean
	// up nicely.
	//
	node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
	if(node != NULL) {node = ILibLinkedList_GetNextNode(node);} // Skip the base timer which is the first element in the chain.
	
	//
	// Set the Terminate Flag to 1, so that ILibIsChainBeingDestroyed returns non-zero when we start cleanup
	//
	((struct ILibBaseChain*)Chain)->TerminateFlag = 1;
	
	while(node!=NULL && (module=(ILibChain*)ILibLinkedList_GetDataFromNode(node))!=NULL)
	{
		//
		// If this module has a destroy method, call it.
		//
		if (module->Destroy != NULL) module->Destroy((void*)module);
		//
		// After calling the Destroy, we free the module and move to the next
		//
		free(module);
		node = ILibLinkedList_Remove(node);
	}
	
	if (gILibChain ==  Chain) { gILibChain = NULL; } // Reset the global instance if it was set
	
	//
	// Go back and destroy the base timer for this chain, the first element in the chain.
	//
	node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
	if (node!=NULL && (module=(ILibChain*)ILibLinkedList_GetDataFromNode(node))!=NULL)
	{
		module->Destroy((void*)module);
		free(module);
		((ILibBaseChain*)Chain)->Timer = NULL;
	}
	
	ILibLinkedList_Destroy(((ILibBaseChain*)Chain)->Links);
	
	node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->LinksPendingDelete);
	while(node != NULL && (module = (ILibChain*)ILibLinkedList_GetDataFromNode(node)) != NULL)
	{
		if(module->Destroy != NULL) {module->Destroy((void*)module);}
		free(module);
		node = ILibLinkedList_GetNextNode(node);
	}
	ILibLinkedList_Destroy(((ILibBaseChain*)Chain)->LinksPendingDelete);
	
#ifdef _REMOTELOGGINGSERVER
	if (((ILibBaseChain*)Chain)->LoggingWebServer != NULL && ((ILibBaseChain*)Chain)->ChainLogger != NULL) { ILibRemoteLogging_Destroy(((ILibBaseChain*)Chain)->ChainLogger); }
#endif
	if(((struct ILibBaseChain*)Chain)->ChainStash != NULL) {ILibHashtable_Destroy(((struct ILibBaseChain*)Chain)->ChainStash);}
	
	//
	// Now we actually free the chain, before we just freed what the chain pointed to.
	//
#if !defined(WIN32) && !defined(_WIN32_WCE)
	//
	// Free the pipe resources
	//
	fclose(((ILibBaseChain*)Chain)->TerminateReadPipe);
	fclose(((ILibBaseChain*)Chain)->TerminateWritePipe);
	((ILibBaseChain*)Chain)->TerminateReadPipe=0;
	((ILibBaseChain*)Chain)->TerminateWritePipe=0;
#endif
#if defined(WIN32)
	if (((ILibBaseChain*)Chain)->Terminate != ~0)
	{
		closesocket(((ILibBaseChain*)Chain)->Terminate);
		((ILibBaseChain*)Chain)->Terminate = (SOCKET)~0;
	}
#endif
	
#ifdef WIN32
	WSACleanup();
#endif
	if (ILibChainLock_RefCounter == 1)
	{
		sem_destroy(&ILibChainLock);
	}
	--ILibChainLock_RefCounter;
	free(Chain);
}

static ILibBaseChain* g_chain;

// thoughts...to allow deletion of stuffs...
// allocate a private subchain and add it to the master chain, that way i can just destroy the subchain when a module is done...leaving the master chain intact,

static void poll_async() {
	ILibStartChain(g_chain);
}

struct poll_context_t* poll_init() {
	if( ! g_chain ) {
		g_chain = (ILibBaseChain*)ILibCreateChain();
		LOCK_INIT();
#ifdef FULL_ASYNC
		ILibSpawnNormalThread(&poll_async, NULL);
#endif
	}
	return (poll_context_t*)g_chain;
}

void poll_deinit() {
	if( g_chain ) {
#ifdef FULL_ASYNC
		ILibStopChain( g_chain );
#else
		ILibDestroyChain( g_chain );
#endif
		g_chain = NULL;
	}
}

void* poll_chain() {
	return g_chain;
}

void poll_lock() {
	assert( g_chain != NULL );

	//LOCK_WAIT();
	while( ! LOCK_TIMED_WAIT(1000) ) {
		LOG("Timeout afer 1000ms waiting on lock!!!!\n");
	}
}

void poll_unlock() {
	assert( g_chain != NULL );

	LOCK_RELEASE();
}

void poll_add_module( poll_module_t* module ) {
	assert( g_chain != NULL );
	
	ILibChain_SafeAdd( g_chain, module );
}

void ILibChain_Safe_Free(void *object)
{
	free( object );
}

struct ILibBaseChain_SafeData
{
	void *Chain;
	void *Object;
};

void ILibChain_SafeDestroySink(void *object)
{
	struct ILibBaseChain_SafeData *data = (struct ILibBaseChain_SafeData*)object;
	struct ILibBaseChain *chain = (struct ILibBaseChain*)data->Chain;
	
	ILibLinkedList_Remove_ByData(chain->Links, data->Object);
	
	if( reinterpret_cast<poll_module_t*>( data->Object )->Destroy )
		reinterpret_cast<poll_module_t*>( data->Object )->Destroy(data->Object);
	
	free(data);
}

void ILibChain_Safe_Destroy(ILibBaseChain* chain, void* object) {

	struct ILibBaseChain *baseChain = (struct ILibBaseChain*)chain;
	struct ILibBaseChain_SafeData *data;
	
	if (ILibIsChainBeingDestroyed(chain) == 0)
	{
		if ((data = (struct ILibBaseChain_SafeData*)malloc(sizeof(struct ILibBaseChain_SafeData))) == NULL) ILIBCRITICALEXIT(254);
		memset(data, 0, sizeof(struct ILibBaseChain_SafeData));
		data->Chain = chain;
		data->Object = object;
		ILibLifeTime_Add(baseChain->Timer, data, 1, &ILibChain_SafeDestroySink, &ILibChain_Safe_Free);
	}
}

void poll_timeout(poll_timeout_t callback, int timeout_ms, void* user_data ) {
	struct ILibBaseChain *baseChain = (struct ILibBaseChain*)g_chain;

	ILibLifeTime_AddEx(baseChain->Timer, user_data, timeout_ms, callback, NULL);
}

void poll_destroy_module( poll_module_t* module ) {
	ILibChain_Safe_Destroy( g_chain, module );
}

int poll_select( int nfds, fd_set *readfds, fd_set *writefds,
											  fd_set *exceptfds, struct timeval *timeout) {
#ifdef FULL_ASYNC
	
#else
	struct timeval tv;
	
	fd_set readset;
	fd_set errorset;
	fd_set writeset;
	
	if( g_chain ) {
		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&errorset);
		
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		
		int ret = ILibIterateChain( g_chain, readfds	? *readfds	: readset,
								   writefds	? *writefds : writeset,
								   exceptfds ? *exceptfds: errorset,
								   timeout   ? *timeout : tv );
		
		// -2 is returned when the chain is not iterated due to locking.
		if( ret != -2 ) {
			return ret;
		} else {
			// pass through select doesnt work in threaded mode (currently)
			// so if someone asked to do so fail the assert.
			assert( nfds == 0 );
			return 0;
		}
	}
	
#endif
	return select( nfds, readfds, writefds, exceptfds, timeout );
}

void poll_interrupt() {
	if( g_chain != NULL ) {
		ILibForceUnBlockChain(g_chain);
	}
}

#endif
