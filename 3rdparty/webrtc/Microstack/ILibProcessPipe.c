/*
Copyright 2006 - 2017 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifdef MEMORY_CHECK
#include <assert.h>
#define MEMCHECK(x) x
#else
#define MEMCHECK(x)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


#include "ILibParsers.h"
#include "ILibRemoteLogging.h"
#include "ILibProcessPipe.h"
#include <assert.h>
#ifndef WIN32
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef __APPLE__
#include "util.h"
#else
#include <pty.h>
#endif
#endif

#define CONSOLE_SCREEN_WIDTH 80
#define CONSOLE_SCREEN_HEIGHT 25

typedef struct ILibProcessPipe_Manager_Object
{
	ILibChain_Link ChainLink;
	ILibLinkedList ActivePipes;

#ifdef WIN32
	int abort;
	HANDLE updateEvent;
	HANDLE workerThread;
	DWORD workerThreadID;
	void *activeWaitHandle;
#endif
}ILibProcessPipe_Manager_Object;
struct ILibProcessPipe_PipeObject;

typedef void(*ILibProcessPipe_GenericReadHandler)(char *buffer, int bufferLen, int* bytesConsumed, void* user1, void* user2);
typedef void(*ILibProcessPipe_GenericSendOKHandler)(void* user1, void* user2);
typedef void(*ILibProcessPipe_GenericBrokenPipeHandler)(struct ILibProcessPipe_PipeObject* sender);
struct ILibProcessPipe_Process_Object; // Forward Prototype

typedef struct ILibProcessPipe_PipeObject
{
	char* buffer;
	int bufferSize;

	int readOffset;
	int totalRead;
	int processingLoop;

	ILibProcessPipe_Manager_Object *manager;
	struct ILibProcessPipe_Process_Object* mProcess;
	ILibQueue WriteBuffer;
	void *handler;
	ILibProcessPipe_GenericBrokenPipeHandler brokenPipeHandler;
	void *user1, *user2;
#ifdef WIN32
	HANDLE mPipe_Reader_ResumeEvent;
	HANDLE mPipe_ReadEnd;
	HANDLE mPipe_WriteEnd;
	struct _OVERLAPPED *mOverlapped;
	void *mOverlapped_opaqueData;
#else
	int mPipe_ReadEnd, mPipe_WriteEnd;
#endif
	int PAUSED;
}ILibProcessPipe_PipeObject;



typedef struct ILibProcessPipe_Process_Object
{
	int exiting;
	unsigned int flags1, flags2;
	ILibProcessPipe_Manager_Object *parent;
	unsigned long PID;
	void *userObject;
	
	ILibProcessPipe_PipeObject *stdIn;
	ILibProcessPipe_PipeObject *stdOut;
	ILibProcessPipe_PipeObject *stdErr;
	ILibProcessPipe_Process_ExitHandler exitHandler;
#ifdef WIN32
	HANDLE hProcess;
#endif

}ILibProcessPipe_Process_Object;

typedef struct ILibProcessPipe_WriteData
{
	char *buffer;
	int bufferLen;
	ILibTransport_MemoryOwnership ownership;
}ILibProcessPipe_WriteData;

const int ILibMemory_ILibProcessPipe_Pipe_CONTAINERSIZE = sizeof(ILibProcessPipe_PipeObject);

ILibProcessPipe_WriteData* ILibProcessPipe_WriteData_Create(char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership)
{
	ILibProcessPipe_WriteData* retVal;

	if ((retVal = (ILibProcessPipe_WriteData*)malloc(sizeof(ILibProcessPipe_WriteData))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(ILibProcessPipe_WriteData));
	retVal->bufferLen = bufferLen;
	if (ownership == ILibTransport_MemoryOwnership_USER)
	{
		if ((retVal->buffer = (char*)malloc(bufferLen)) == NULL) { ILIBCRITICALEXIT(254); }
		memcpy_s(retVal->buffer, bufferLen, buffer, bufferLen);
		retVal->ownership = ILibTransport_MemoryOwnership_CHAIN;
	}
	else
	{
		retVal->buffer = buffer;
		retVal->ownership = ownership;
	}
	return(retVal);
}
#define ILibProcessPipe_WriteData_Destroy(writeData) if (writeData->ownership == ILibTransport_MemoryOwnership_CHAIN) { free(writeData->buffer); } free(writeData);
ILibProcessPipe_Pipe ILibProcessPipe_Process_GetStdErr(ILibProcessPipe_Process p)
{
	return(((ILibProcessPipe_Process_Object*)p)->stdErr);
}
ILibProcessPipe_Pipe ILibProcessPipe_Process_GetStdOut(ILibProcessPipe_Process p)
{
	return(((ILibProcessPipe_Process_Object*)p)->stdOut);
}

#ifdef WIN32
typedef struct ILibProcessPipe_WaitHandle
{
	ILibProcessPipe_Manager_Object *parent;
	HANDLE event;
	void *user;
	ILibProcessPipe_WaitHandle_Handler callback;
}ILibProcessPipe_WaitHandle;

int ILibProcessPipe_Manager_WindowsWaitHandles_Remove_Comparer(void *source, void *matchWith)
{
	if (source == NULL) { return(1); }
	return(((ILibProcessPipe_WaitHandle*)source)->event == matchWith ? 0 : 1);
}

void __stdcall ILibProcessPipe_WaitHandle_Remove_APC(ULONG_PTR obj)
{
	ILibProcessPipe_Manager_Object *manager = (ILibProcessPipe_Manager_Object*)((void**)obj)[0];
	HANDLE event = (HANDLE)((void**)obj)[1];
	void *node = ILibLinkedList_GetNode_Search(manager->ActivePipes, ILibProcessPipe_Manager_WindowsWaitHandles_Remove_Comparer, event);
	ILibProcessPipe_WaitHandle *waiter;

	if (node != NULL) 
	{ 
		waiter = (ILibProcessPipe_WaitHandle*)ILibLinkedList_GetDataFromNode(node);
		free(waiter);
		ILibLinkedList_Remove(node); 
	}
	free((void*)obj);
}
void ILibProcessPipe_WaitHandle_Remove(ILibProcessPipe_Manager mgr, HANDLE event)
{
	ILibProcessPipe_Manager_Object *manager = (ILibProcessPipe_Manager_Object*)mgr;
	void **data = (void**)ILibMemory_Allocate(2 * sizeof(void*), 0, NULL, NULL);
	data[0] = manager;
	data[1] = event;
	
	QueueUserAPC((PAPCFUNC)ILibProcessPipe_WaitHandle_Remove_APC, manager->workerThread, (ULONG_PTR)data);
}
void __stdcall ILibProcessPipe_WaitHandle_Add_APC(ULONG_PTR obj)
{
	ILibProcessPipe_WaitHandle *waitHandle = (ILibProcessPipe_WaitHandle*)obj;
	ILibLinkedList_AddTail(waitHandle->parent->ActivePipes, waitHandle);
}
void ILibProcessPipe_WaitHandle_AddEx(ILibProcessPipe_Manager mgr, ILibProcessPipe_WaitHandle *waitHandle)
{
	ILibProcessPipe_Manager_Object *manager = (ILibProcessPipe_Manager_Object*)mgr;
	if (manager->workerThreadID == GetCurrentThreadId())
	{
		// We're on the same thread, so we can just add it in
		ILibLinkedList_AddTail(manager->ActivePipes, waitHandle);
		SetEvent(manager->updateEvent);
	}
	else
	{
		// We're on a different thread, so we need to dispatch to the worker thread
		QueueUserAPC((PAPCFUNC)ILibProcessPipe_WaitHandle_Add_APC, manager->workerThread, (ULONG_PTR)waitHandle);
	}
}
void ILibProcessPipe_WaitHandle_Add(ILibProcessPipe_Manager mgr, HANDLE event, void *user, ILibProcessPipe_WaitHandle_Handler callback)
{
	ILibProcessPipe_Manager_Object *manager = (ILibProcessPipe_Manager_Object*)mgr;
	ILibProcessPipe_WaitHandle *waitHandle;
	waitHandle = (ILibProcessPipe_WaitHandle*)ILibMemory_Allocate(sizeof(ILibProcessPipe_WaitHandle), 0, NULL, NULL);

	waitHandle->parent = manager;
	waitHandle->event = event;
	waitHandle->user = user;
	waitHandle->callback = callback;


	ILibProcessPipe_WaitHandle_AddEx(mgr, waitHandle);
}

void ILibProcessPipe_Manager_WindowsRunLoopEx(void *arg)
{
	ILibProcessPipe_Manager_Object *manager = (ILibProcessPipe_Manager_Object*)arg;
	HANDLE hList[FD_SETSIZE * (2*sizeof(HANDLE))];
	ILibLinkedList active = manager->ActivePipes;

	void* node;
	ILibProcessPipe_WaitHandle* data;

	int i, x;

	memset(hList, 0, sizeof(HANDLE)*FD_SETSIZE);
	manager->workerThreadID = GetCurrentThreadId();

	while (manager->abort == 0)
	{
		hList[0] = manager->updateEvent;
		i = 1;

		
		//Prepare the rest of the WaitHandle Array, for the WaitForMultipleObject call
		node = ILibLinkedList_GetNode_Head(active);
		while (node != NULL)
		{
			if ((data = (ILibProcessPipe_WaitHandle*)ILibLinkedList_GetDataFromNode(node)) != NULL)
			{
				ILibRemoteLogging_printf(ILibChainGetLogger(manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "WindowsRunLoopEx():   [%p]", data);

				hList[i] = data->event;
				hList[i + FD_SETSIZE] = (HANDLE)data;
				++i;
			}
			node = ILibLinkedList_GetNextNode(node);
		}

		ILibRemoteLogging_printf(ILibChainGetLogger(manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "WindowsRunLoopEx(): Number of Handles: %d", i);

		while ((x = WaitForMultipleObjectsEx(i, hList, FALSE, INFINITE, TRUE)) != WAIT_IO_COMPLETION)
		{
			if (x == WAIT_FAILED || (x-WAIT_OBJECT_0) == 0) { break; }
			data = (ILibProcessPipe_WaitHandle*)hList[x + FD_SETSIZE];
			manager->activeWaitHandle = data;
			if (data != NULL && data->callback != NULL) { data->callback(data->event, data->user); }
		}
		ResetEvent(manager->updateEvent);
	}
}
ILibProcessPipe_Manager_WindowsRunLoop(void *arg)
{
	CONTEXT winException;
	__try
	{
		ILibProcessPipe_Manager_WindowsRunLoopEx(arg);
	}
	__except (ILib_WindowsExceptionFilter(GetExceptionCode(), GetExceptionInformation(), &winException))
	{
		ILib_WindowsExceptionDebug(&winException);
	}
}
void ILibProcessPipe_Manager_Start(void* chain, void* user)
{ 
	ILibProcessPipe_Manager_Object* man = (ILibProcessPipe_Manager_Object*)user;

	UNREFERENCED_PARAMETER(chain);
	man->workerThread = ILibSpawnNormalThread((voidfp1)&ILibProcessPipe_Manager_WindowsRunLoop, user);
}
#else
void ILibProcessPipe_Process_ReadHandler(void* user);
void ILibProcessPipe_Manager_OnPreSelect(void* object, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	ILibProcessPipe_Manager_Object *man = (ILibProcessPipe_Manager_Object*)object;
	void *node;
	ILibProcessPipe_PipeObject *j;

	node = ILibLinkedList_GetNode_Head(man->ActivePipes);
	while(node != NULL && (j = (ILibProcessPipe_PipeObject*)ILibLinkedList_GetDataFromNode(node)) != NULL)
	{
		FD_SET(j->mPipe_ReadEnd, readset);
		node = ILibLinkedList_GetNextNode(node);
	}

}
void ILibProcessPipe_Manager_OnPostSelect(void* object, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset)
{
	ILibProcessPipe_Manager_Object *man = (ILibProcessPipe_Manager_Object*)object;
	void *node, *nextNode;
	ILibProcessPipe_PipeObject *j;

	node = ILibLinkedList_GetNode_Head(man->ActivePipes);
	while(node != NULL && (j = (ILibProcessPipe_PipeObject*)ILibLinkedList_GetDataFromNode(node)) != NULL)
	{
		nextNode = ILibLinkedList_GetNextNode(node);
		if(FD_ISSET(j->mPipe_ReadEnd, readset) != 0)
		{
			ILibProcessPipe_Process_ReadHandler(j);
		}

		node = nextNode;
	}
}
#endif
void ILibProcessPipe_Manager_OnDestroy(void *object)
{
	ILibProcessPipe_Manager_Object *man = (ILibProcessPipe_Manager_Object*)object;

#ifdef WIN32
	man->abort = 1;
	SetEvent(man->updateEvent);
	WaitForSingleObject(man->workerThread, INFINITE);
#endif

	ILibLinkedList_Destroy(man->ActivePipes);
}
ILibProcessPipe_Manager ILibProcessPipe_Manager_Create(void *chain)
{
	ILibProcessPipe_Manager_Object *retVal;

	if ((retVal = (ILibProcessPipe_Manager_Object*)malloc(sizeof(ILibProcessPipe_Manager_Object))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(ILibProcessPipe_Manager_Object));

	retVal->ChainLink.ParentChain = chain;
	retVal->ActivePipes = ILibLinkedList_Create();

#ifdef WIN32
	retVal->updateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ILibChain_OnStartEvent_AddHandler(chain, (ILibChain_StartEvent)&ILibProcessPipe_Manager_Start, retVal);
#else
	retVal->ChainLink.PreSelectHandler = &ILibProcessPipe_Manager_OnPreSelect;
	retVal->ChainLink.PostSelectHandler = &ILibProcessPipe_Manager_OnPostSelect;
#endif
	retVal->ChainLink.DestroyHandler = &ILibProcessPipe_Manager_OnDestroy;

	if (ILibIsChainRunning(chain) == 0)
	{
		ILibAddToChain(chain, retVal);
	}
	else
	{
		ILibChain_SafeAdd(chain, retVal);
	}
	return(retVal);
}

void ILibProcessPipe_FreePipe(ILibProcessPipe_PipeObject *pipeObject)
{
#ifdef WIN32
	if (pipeObject->mPipe_ReadEnd != NULL) { CloseHandle(pipeObject->mPipe_ReadEnd); }
	if (pipeObject->mPipe_WriteEnd != NULL) { CloseHandle(pipeObject->mPipe_WriteEnd); }
	if (pipeObject->mOverlapped != NULL) { CloseHandle(pipeObject->mOverlapped->hEvent); free(pipeObject->mOverlapped); }
	if (pipeObject->mPipe_Reader_ResumeEvent != NULL) { CloseHandle(pipeObject->mPipe_Reader_ResumeEvent); }
#endif

	if (pipeObject->buffer != NULL) { free(pipeObject->buffer); }
	if (pipeObject->WriteBuffer != NULL)
	{
		ILibProcessPipe_WriteData* data;
		while ((data = (ILibProcessPipe_WriteData*)ILibQueue_DeQueue(pipeObject->WriteBuffer)) != NULL)
		{
			ILibProcessPipe_WriteData_Destroy(data);
		}
		ILibQueue_Destroy(pipeObject->WriteBuffer);
	}
	if (pipeObject->mProcess != NULL)
	{
		if (pipeObject->mProcess->stdIn == pipeObject) { pipeObject->mProcess->stdIn = NULL; }
		if (pipeObject->mProcess->stdOut == pipeObject) { pipeObject->mProcess->stdOut = NULL; }
		if (pipeObject->mProcess->stdErr == pipeObject) { pipeObject->mProcess->stdErr = NULL; }
	}
	free(pipeObject);
}

#ifdef WIN32
void ILibProcessPipe_PipeObject_DisableInherit(HANDLE* h)
{
	HANDLE tmpRead = *h;
	DuplicateHandle(GetCurrentProcess(), tmpRead, GetCurrentProcess(), h,  0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(tmpRead);
}
#endif

#ifdef WIN32
ILibProcessPipe_Pipe ILibProcessPipe_Pipe_CreateFromExistingWithExtraMemory(ILibProcessPipe_Manager manager, HANDLE existingPipe, ILibProcessPipe_Pipe_ReaderHandleType handleType, int extraMemorySize)
#else
ILibProcessPipe_Pipe ILibProcessPipe_Pipe_CreateFromExistingWithExtraMemory(ILibProcessPipe_Manager manager, int existingPipe, int extraMemorySize)
#endif
{
	ILibProcessPipe_PipeObject* retVal = NULL;

	retVal = (ILibProcessPipe_PipeObject*)ILibMemory_Allocate(ILibMemory_ILibProcessPipe_Pipe_CONTAINERSIZE, extraMemorySize, NULL, NULL);
	retVal->manager = (ILibProcessPipe_Manager_Object*)manager;

#ifdef WIN32
	if (handleType == ILibProcessPipe_Pipe_ReaderHandleType_Overlapped)
	{
		if ((retVal->mOverlapped = (struct _OVERLAPPED*)malloc(sizeof(struct _OVERLAPPED))) == NULL) { ILIBCRITICALEXIT(254); }
		memset(retVal->mOverlapped, 0, sizeof(struct _OVERLAPPED));
		if ((retVal->mOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) { ILIBCRITICALEXIT(254); }
	}
#else
	fcntl(existingPipe, F_SETFL, O_NONBLOCK);
#endif

	retVal->mPipe_ReadEnd = existingPipe;
	retVal->mPipe_WriteEnd = existingPipe;
	return(retVal);
}

void ILibProcessPipe_Pipe_SetBrokenPipeHandler(ILibProcessPipe_Pipe targetPipe, ILibProcessPipe_Pipe_BrokenPipeHandler handler)
{
	((ILibProcessPipe_PipeObject*)targetPipe)->brokenPipeHandler = (ILibProcessPipe_GenericBrokenPipeHandler)handler;
}

ILibProcessPipe_PipeObject* ILibProcessPipe_CreatePipe(ILibProcessPipe_Manager manager, int pipeBufferSize, ILibProcessPipe_GenericBrokenPipeHandler brokenPipeHandler)
{
	ILibProcessPipe_PipeObject* retVal = NULL;
#ifdef WIN32
	char pipeName[255];
	SECURITY_ATTRIBUTES saAttr;
#else
	int fd[2];
#endif

	if ((retVal = (ILibProcessPipe_PipeObject*)malloc(sizeof(ILibProcessPipe_PipeObject))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(ILibProcessPipe_PipeObject));
	retVal->brokenPipeHandler = brokenPipeHandler;
	retVal->manager = (ILibProcessPipe_Manager_Object*)manager;

#ifdef WIN32
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	sprintf_s(pipeName, sizeof(pipeName), "\\\\.\\pipe\\%p", (void*)retVal);


	if ((retVal->mOverlapped = (struct _OVERLAPPED*)malloc(sizeof(struct _OVERLAPPED))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal->mOverlapped, 0, sizeof(struct _OVERLAPPED));
	
	if ((retVal->mOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) { ILIBCRITICALEXIT(254); }

	retVal->mPipe_ReadEnd = CreateNamedPipeA(pipeName, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, pipeBufferSize, pipeBufferSize, 0, &saAttr);
	if (retVal->mPipe_ReadEnd == INVALID_HANDLE_VALUE) { ILIBCRITICALEXIT(254); }

	retVal->mPipe_WriteEnd = CreateFileA(pipeName, GENERIC_WRITE, 0, &saAttr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (retVal->mPipe_WriteEnd == INVALID_HANDLE_VALUE) { ILIBCRITICALEXIT(254); }
#else
	if(pipe(fd)==0) 
	{
		fcntl(fd[0], F_SETFL, O_NONBLOCK); 
		fcntl(fd[1], F_SETFL, O_NONBLOCK);
		retVal->mPipe_ReadEnd = fd[0];
		retVal->mPipe_WriteEnd = fd[1];
	}
#endif
	
	return(retVal);
}

void ILibProcessPipe_Process_Destroy(ILibProcessPipe_Process_Object *p)
{
	if (p->exiting != 0) { return; }
	if (p->stdIn != NULL) { ILibProcessPipe_FreePipe(p->stdIn); }
	if (p->stdOut != NULL) { ILibProcessPipe_FreePipe(p->stdOut); }
	if (p->stdErr != NULL) { ILibProcessPipe_FreePipe(p->stdErr); }
	free(p);
}
#ifndef WIN32
void ILibProcessPipe_Process_BrokenPipeSink(ILibProcessPipe_PipeObject* sender)
{
	ILibProcessPipe_Process_Object *p = sender->mProcess;
	int status;

	if (ILibIsRunningOnChainThread(sender->manager->ChainLink.ParentChain) != 0)
	{
		// This was called from the Reader
		if (p->exitHandler != NULL)
		{
			waitpid((pid_t)p->PID, &status, 0);
			p->exitHandler(p, WEXITSTATUS(status), p->userObject);
		}
	}
}
#endif
void* ILibProcessPipe_Process_KillEx(ILibProcessPipe_Process p)
{
	void *retVal = ILibProcessPipe_Process_Kill(p);
	ILibProcessPipe_Process_Destroy((ILibProcessPipe_Process_Object*)p);
	return(retVal);
}
void* ILibProcessPipe_Process_Kill(ILibProcessPipe_Process p)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)p;

#ifdef WIN32
	// First things first, unhook all the pipes from the Windows Run Loop
	if (j->stdIn != NULL && j->stdIn->mOverlapped != NULL) { ILibProcessPipe_WaitHandle_Remove(j->parent, j->stdIn->mOverlapped->hEvent); }
	if (j->stdOut != NULL && j->stdOut->mOverlapped != NULL) { ILibProcessPipe_WaitHandle_Remove(j->parent, j->stdOut->mOverlapped->hEvent); }
	if (j->stdErr != NULL && j->stdErr->mOverlapped != NULL) { ILibProcessPipe_WaitHandle_Remove(j->parent, j->stdErr->mOverlapped->hEvent); }
	if (j->hProcess != NULL) { ILibProcessPipe_WaitHandle_Remove(j->parent, j->hProcess); TerminateProcess(j->hProcess, 1067); } 
#else
	int code;
	if (j->stdIn != NULL) { j->stdIn->brokenPipeHandler = NULL; }
	if (j->stdOut != NULL) { j->stdOut->brokenPipeHandler = NULL; }
	if (j->stdErr != NULL) { j->stdErr->brokenPipeHandler = NULL; }

	kill((pid_t)j->PID, SIGKILL);
	waitpid((pid_t)j->PID, &code, 1);
#endif
	return(j->userObject);
}
int ILibProcessPipe_Process_GetPID(ILibProcessPipe_Process p) //ToDo: Change return type to pid_t
{
	return(p != NULL ? (int)((ILibProcessPipe_Process_Object*)p)->PID : 0);
}
ILibProcessPipe_Process ILibProcessPipe_Manager_SpawnProcessEx(ILibProcessPipe_Manager pipeManager, char* target, char* const* parameters, ILibProcessPipe_SpawnTypes spawnType)
{
	ILibProcessPipe_Process_Object* retVal = NULL;

#ifdef WIN32
	STARTUPINFOA info = { 0 };
	PROCESS_INFORMATION processInfo = { 0 };
	SECURITY_ATTRIBUTES saAttr;
	char* parms = NULL;
	DWORD sessionId;
	HANDLE token=NULL, userToken=NULL, procHandle=NULL;
	int allocParms = 0;

	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&info, sizeof(STARTUPINFOA));

	if (spawnType != ILibProcessPipe_SpawnTypes_DEFAULT && (sessionId = WTSGetActiveConsoleSessionId()) == 0xFFFFFFFF) { return(NULL); } // No session attached to console, but requested to execute as logged in user
	if (spawnType != ILibProcessPipe_SpawnTypes_DEFAULT)
	{
		procHandle = GetCurrentProcess(); 
		if (OpenProcessToken(procHandle, TOKEN_DUPLICATE, &token) == 0) { ILIBMARKPOSITION(2); return(NULL); }
		if (DuplicateTokenEx(token, MAXIMUM_ALLOWED, 0, SecurityImpersonation, TokenPrimary, &userToken) == 0) { CloseHandle(token); ILIBMARKPOSITION(2); return(NULL); }
		if (SetTokenInformation(userToken, (TOKEN_INFORMATION_CLASS)TokenSessionId, &sessionId, sizeof(sessionId)) == 0) { CloseHandle(token); CloseHandle(userToken); ILIBMARKPOSITION(2); return(NULL); }
		if (spawnType == ILibProcessPipe_SpawnTypes_WINLOGON) { info.lpDesktop = "Winsta0\\Winlogon"; }
	}
	if (parameters != NULL && parameters[0] != NULL && parameters[1] == NULL)
	{
		parms = parameters[0];
	}
	else if (parameters != NULL && parameters[0] != NULL && parameters[1] != NULL)
	{
		int len = 0;
		int i = 0;
		int sz = 0;

		while (parameters[i] != NULL)
		{
			sz += ((int)strnlen_s(parameters[i++], _MAX_PATH) + 1);
		}
		sz += (i - 1); // Need to make room for delimiter characters
		parms = (char*)malloc(sz); 
		i = 0; len = 0;
		allocParms = 1;

		while (parameters[i] != NULL)
		{
			len += sprintf_s(parms + len, sz - len, "%s%s", (i==0)?"":" ", parameters[i]);
			++i;
		}
	}

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
#else
	pid_t pid;
#endif

	if ((retVal = (ILibProcessPipe_Process_Object*)malloc(sizeof(ILibProcessPipe_Process_Object))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(ILibProcessPipe_Process_Object));
	
	retVal->stdErr = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
	retVal->stdErr->mProcess = retVal;
	retVal->parent = (ILibProcessPipe_Manager_Object*)pipeManager;
	
#ifdef WIN32
	retVal->stdIn = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
	retVal->stdIn->mProcess = retVal;
	retVal->stdOut = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
	retVal->stdOut->mProcess = retVal;

	ILibProcessPipe_PipeObject_DisableInherit(&(retVal->stdIn->mPipe_WriteEnd));
	ILibProcessPipe_PipeObject_DisableInherit(&(retVal->stdOut->mPipe_ReadEnd));
	ILibProcessPipe_PipeObject_DisableInherit(&(retVal->stdErr->mPipe_ReadEnd));

	info.cb = sizeof(STARTUPINFOA);
	info.hStdError = retVal->stdErr->mPipe_WriteEnd;
	info.hStdInput = retVal->stdIn->mPipe_ReadEnd;
	info.hStdOutput = retVal->stdOut->mPipe_WriteEnd;
	info.dwFlags |= STARTF_USESTDHANDLES;

	if((spawnType == ILibProcessPipe_SpawnTypes_DEFAULT && !CreateProcessA(target, parms, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &info, &processInfo)) ||
		(spawnType != ILibProcessPipe_SpawnTypes_DEFAULT && !CreateProcessAsUserA(userToken, target, parms, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &info, &processInfo)))
	{
		ILibProcessPipe_FreePipe(retVal->stdErr);
		ILibProcessPipe_FreePipe(retVal->stdOut);
		ILibProcessPipe_FreePipe(retVal->stdIn);
		if (allocParms != 0) { free(parms); }
		free(retVal);
		if (token != NULL) { CloseHandle(token); }
		if (userToken != NULL) { CloseHandle(userToken); }
		return(NULL);
	}


	if (allocParms != 0) { free(parms); }
	CloseHandle(retVal->stdOut->mPipe_WriteEnd);	retVal->stdOut->mPipe_WriteEnd = NULL;
	CloseHandle(retVal->stdErr->mPipe_WriteEnd);	retVal->stdErr->mPipe_WriteEnd = NULL;
	CloseHandle(retVal->stdIn->mPipe_ReadEnd);		retVal->stdIn->mPipe_ReadEnd = NULL;

	retVal->hProcess = processInfo.hProcess;
	if (processInfo.hThread != NULL) CloseHandle(processInfo.hThread);
	retVal->PID = processInfo.dwProcessId;
#else
	if (spawnType == ILibProcessPipe_SpawnTypes_TERM)
	{
		int pipe;
		struct winsize w;
		w.ws_row = CONSOLE_SCREEN_HEIGHT;
		w.ws_col = CONSOLE_SCREEN_WIDTH;
		w.ws_xpixel = 0;
		w.ws_ypixel = 0;
		pid = forkpty(&pipe, NULL, NULL, &w);	
		
		retVal->stdIn = ILibProcessPipe_Pipe_CreateFromExisting(pipeManager, pipe);
		retVal->stdIn->mProcess = retVal;
		retVal->stdOut = ILibProcessPipe_Pipe_CreateFromExisting(pipeManager, pipe);
		retVal->stdOut->mProcess = retVal;
	}
	else
	{
		retVal->stdIn = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
		retVal->stdIn->mProcess = retVal;
		retVal->stdOut = ILibProcessPipe_CreatePipe(pipeManager, 4096, &ILibProcessPipe_Process_BrokenPipeSink);
		retVal->stdOut->mProcess = retVal;
		pid = vfork();
	}
	if(pid < 0)
	{
		ILibProcessPipe_FreePipe(retVal->stdErr);
		ILibProcessPipe_FreePipe(retVal->stdOut);
		ILibProcessPipe_FreePipe(retVal->stdIn);
		free(retVal);
		return(NULL);
	}
	if(pid==0)
	{
		close(retVal->stdErr->mPipe_ReadEnd); //close read end of stderr pipe
		dup2(retVal->stdErr->mPipe_WriteEnd, STDERR_FILENO);
		close(retVal->stdErr->mPipe_WriteEnd);

		if (spawnType == ILibProcessPipe_SpawnTypes_TERM)
		{
			putenv("TERM=xterm");
		}
		else
		{
			close(retVal->stdIn->mPipe_WriteEnd); //close write end of stdin pipe
			close(retVal->stdOut->mPipe_ReadEnd); //close read end of stdout pipe

			dup2(retVal->stdIn->mPipe_ReadEnd, STDIN_FILENO);
			dup2(retVal->stdOut->mPipe_WriteEnd, STDOUT_FILENO);

			close(retVal->stdIn->mPipe_ReadEnd);
			close(retVal->stdOut->mPipe_WriteEnd);
		}
		execv(target, parameters);
		exit(1);
	}
	if (spawnType != ILibProcessPipe_SpawnTypes_TERM)
	{
		close(retVal->stdIn->mPipe_ReadEnd);
		close(retVal->stdOut->mPipe_WriteEnd);
	}
	close(retVal->stdErr->mPipe_WriteEnd);
	retVal->PID = pid;
#endif
	return(retVal);
}
void ILibProcessPipe_Pipe_SwapBuffers(ILibProcessPipe_Pipe obj, char* newBuffer, int newBufferLen, int newBufferReadOffset, int newBufferTotalBytesRead, char **oldBuffer, int *oldBufferLen, int *oldBufferReadOffset, int *oldBufferTotalBytesRead)
{
	ILibProcessPipe_PipeObject *pipeObject = (ILibProcessPipe_PipeObject*)obj;

	*oldBuffer = pipeObject->buffer;
	*oldBufferLen = pipeObject->bufferSize;
	*oldBufferReadOffset = pipeObject->readOffset;
	*oldBufferTotalBytesRead = pipeObject->totalRead;

	pipeObject->buffer = newBuffer;
	pipeObject->bufferSize = newBufferLen;
	pipeObject->readOffset = newBufferReadOffset;
	pipeObject->totalRead = newBufferTotalBytesRead;
}
#ifdef WIN32
BOOL ILibProcessPipe_Process_ReadHandler(HANDLE event, void* user)
#else
void ILibProcessPipe_Process_ReadHandler(void* user)
#endif
{
	ILibProcessPipe_PipeObject *pipeObject = (ILibProcessPipe_PipeObject*)user;
	int consumed;
	int err;

#ifdef WIN32
	BOOL result;
	DWORD bytesRead;
	UNREFERENCED_PARAMETER(event);
#else
	int bytesRead;
#endif
	pipeObject->processingLoop = 1;
	do
	{
#ifdef WIN32
		result = GetOverlappedResult(pipeObject->mPipe_ReadEnd, pipeObject->mOverlapped, &bytesRead, FALSE);
		if (result == FALSE) { break; }
#else
		bytesRead = (int)read(pipeObject->mPipe_ReadEnd, pipeObject->buffer + pipeObject->totalRead, pipeObject->bufferSize - pipeObject->totalRead);
		if(bytesRead <= 0)
		{
			break;
		}

#endif
		
		pipeObject->totalRead += bytesRead;
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibProcessPipe[ReadHandler]: %u bytes read on Pipe: %p", bytesRead, (void*)pipeObject);

		if (pipeObject->handler == NULL)
		{
			//
			// Since the user doesn't care about the data, we'll just empty the buffer
			//
			pipeObject->readOffset = 0;
			pipeObject->totalRead = 0;
			continue;
		}
		
		while (pipeObject->PAUSED == 0)
		{
			consumed = 0;
			ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_5, "ProcessPipe: buffer/%p offset/%d totalRead/%d", (void*)pipeObject->buffer, pipeObject->readOffset, pipeObject->totalRead);

			((ILibProcessPipe_GenericReadHandler)pipeObject->handler)(pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset, &consumed, pipeObject->user1, pipeObject->user2);
			if (consumed == 0)
			{
				//
				// None of the buffer was consumed
				//
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibProcessPipe[ReadHandler]: No bytes consumed on Pipe: %p", (void*)pipeObject);

				//
				// We need to move the memory to the start of the buffer, or else we risk running past the end, if we keep reading like this
				//
				memmove_s(pipeObject->buffer, pipeObject->bufferSize, pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset);
				pipeObject->totalRead -= pipeObject->readOffset;
				pipeObject->readOffset = 0;

				break; // Break out of inner while loop
			}
			else if (consumed == (pipeObject->totalRead - pipeObject->readOffset))
			{
				//
				// Entire Buffer was consumed
				//
				pipeObject->readOffset = 0;
				pipeObject->totalRead = 0;

				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibProcessPipe[ReadHandler]: ReadBuffer drained on Pipe: %p", (void*)pipeObject);
				break; // Break out of inner while loop
			}
			else
			{
				//
				// Only part of the buffer was consumed
				//
				pipeObject->readOffset += consumed;
			}
		}
		if (pipeObject->PAUSED != 0) { break; }
	}
#ifdef WIN32
	while ((result = ReadFile(pipeObject->mPipe_ReadEnd, pipeObject->buffer, pipeObject->bufferSize, NULL, pipeObject->mOverlapped)) == TRUE); // Note: This is actually the end of a do-while loop
	if (pipeObject->PAUSED == 0 && (err = GetLastError()) != ERROR_IO_PENDING)
#else
	while(pipeObject->PAUSED == 0); // Note: This is actually the end of a do-while loop
	err = 0;
	if(bytesRead == 0 || ((err = errno) != EAGAIN && errno != EWOULDBLOCK && pipeObject->PAUSED == 0))
#endif
	{
		//
		// Broken Pipe
		//
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibProcessPipe[ReadHandler]: BrokenPipe(%d) on Pipe: %p", err, (void*)pipeObject);
#ifdef WIN32
		ILibProcessPipe_WaitHandle_Remove(pipeObject->manager, pipeObject->mOverlapped->hEvent); // Pipe Broken, so remove ourselves from the processing loop
#else
		// Sincc we are on the microstack thread, we can directly access the LinkedList

#endif
		ILibLinkedList_Remove(ILibLinkedList_GetNode_Search(pipeObject->manager->ActivePipes, NULL, pipeObject));

		if (pipeObject->brokenPipeHandler != NULL) 
		{
			((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject); 
		}
#ifdef WIN32
		return(FALSE);
#else
		return;
#endif
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibProcessPipe[ReadHandler]: Pipe: %p [EMPTY]", (void*)pipeObject);
	}
	pipeObject->processingLoop = 0;
	ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibProcessPipe[ReadHandler]: Pipe: %p [EMPTY]", (void*)pipeObject);

#ifdef WIN32
	return(TRUE);
#else
	return;
#endif
}
#ifdef WIN32
BOOL ILibProcessPipe_Process_WindowsWriteHandler(HANDLE event, void* user)
{
	ILibProcessPipe_PipeObject *pipeObject = (ILibProcessPipe_PipeObject*)user;
	BOOL result;
	DWORD bytesWritten;
	ILibProcessPipe_WriteData* data;
	
	UNREFERENCED_PARAMETER(event);
	result = GetOverlappedResult(pipeObject->mPipe_WriteEnd, pipeObject->mOverlapped, &bytesWritten, FALSE);
	if (result == FALSE)
	{ 
		// Broken Pipe
		ILibProcessPipe_WaitHandle_Remove(pipeObject->manager, pipeObject->mOverlapped->hEvent); // Pipe Broken, so remove ourselves from the processing loop
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibProcessPipe[WriteHandler]: BrokenPipe(%d) on Pipe: %p", GetLastError(), (void*)pipeObject);
		if (pipeObject->brokenPipeHandler != NULL) { ((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject); }
		ILibProcessPipe_FreePipe(pipeObject);
		return(FALSE);
	}

	ILibQueue_Lock(pipeObject->WriteBuffer);
	while ((data = (ILibProcessPipe_WriteData*)ILibQueue_DeQueue(pipeObject->WriteBuffer)) != NULL)
	{
		ILibProcessPipe_WriteData_Destroy(data);
		data = (ILibProcessPipe_WriteData*)ILibQueue_PeekQueue(pipeObject->WriteBuffer);
		if (data != NULL)
		{
			result = WriteFile(pipeObject->mPipe_WriteEnd, data->buffer, data->bufferLen, NULL, pipeObject->mOverlapped);
			if (result == TRUE) { continue; }
			if (GetLastError() != ERROR_IO_PENDING)
			{
				// Broken Pipe
				ILibQueue_UnLock(pipeObject->WriteBuffer);
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibProcessPipe[WriteHandler]: BrokenPipe(%d) on Pipe: %p", GetLastError(), (void*)pipeObject);
				if (pipeObject->brokenPipeHandler != NULL) { ((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject); }
				ILibProcessPipe_FreePipe(pipeObject);
				return(FALSE);
			}
			break;
		}
	}
	if (ILibQueue_IsEmpty(pipeObject->WriteBuffer) != 0)
	{
		ILibProcessPipe_WaitHandle_Remove(pipeObject->manager, pipeObject->mOverlapped->hEvent);
		ILibQueue_UnLock(pipeObject->WriteBuffer);
		if (pipeObject->handler != NULL) ((ILibProcessPipe_GenericSendOKHandler)pipeObject->handler)(pipeObject->user1, pipeObject->user2);
	}
	else
	{
		ILibQueue_UnLock(pipeObject->WriteBuffer);
	}
	return(TRUE);
}
#endif
void ILibProcessPipe_Process_SetWriteHandler(ILibProcessPipe_PipeObject *pipeObject, ILibProcessPipe_GenericSendOKHandler handler, void* user1, void* user2)
{
	pipeObject->handler = (void*)handler;
	pipeObject->user1 = user1;
	pipeObject->user2 = user2;
}

void ILibProcessPipe_Process_StartPipeReaderWriterEx(void *object)
{
	ILibProcessPipe_PipeObject* pipeObject = (ILibProcessPipe_PipeObject*)object;
	ILibLinkedList_AddTail(pipeObject->manager->ActivePipes, pipeObject);
}

void ILibProcessPipe_Pipe_Pause(ILibProcessPipe_Pipe pipeObject)
{
	ILibProcessPipe_PipeObject *p = (ILibProcessPipe_PipeObject*)pipeObject;
	p->PAUSED = 1;

#ifdef WIN32
	if (p->mOverlapped == NULL)
	{
		// Overlapped isn't supported, so using a separate reader thread
		ResetEvent(p->mPipe_Reader_ResumeEvent);
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.Pause(): Opaque = %p",(void*)p->mOverlapped_opaqueData);
		if (p->manager->workerThreadID == GetCurrentThreadId())
		{
			// Already on the dispatch thread, so we can directly update the list
			if (p->mOverlapped_opaqueData == NULL)
			{
				void *node = ILibLinkedList_GetNode_Search(p->manager->ActivePipes, NULL, p->manager->activeWaitHandle);
				if (node != NULL)
				{
					ILibLinkedList_Remove(node);
					p->mOverlapped_opaqueData = p->manager->activeWaitHandle;
					ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Opaque = %p", (void*)p->mOverlapped_opaqueData);
					SetEvent(p->manager->updateEvent); // Just need to set event, so the wait list will be rebuilt
				}
				else
				{
					ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.Pause(): Active pipe not in list");
				}
			}
			else
			{
				ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.Pause(): Already paused");
			}
		}
		else
		{
			// Not on the dispatch thread, so we'll have to dispatch
			p->mOverlapped_opaqueData = NULL;
			ILibProcessPipe_WaitHandle_Remove(p->manager, p->mOverlapped->hEvent);
		}

		//if (p->mOverlapped_opaqueData == NULL)
		//{
		//	void *node = ILibLinkedList_GetNode_Search(p->manager->ActivePipes, NULL, p->manager->activeWaitHandle);
		//	if (node != NULL)
		//	{
		//		ILibLinkedList_Remove(node);
		//		p->mOverlapped_opaqueData = p->manager->activeWaitHandle;
		//		ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Opaque = %p", (void*)p->mOverlapped_opaqueData);
		//		SetEvent(p->manager->updateEvent);
		//	}
		//	else
		//	{
		//		ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.Pause(): Active pipe not in list");
		//	}
		//}
	}
#else
	ILibLinkedList_Remove(ILibLinkedList_GetNode_Search(p->manager->ActivePipes, NULL, pipeObject));
#endif
}


void ILibProcessPipe_Pipe_ResumeEx_ContinueProcessing(ILibProcessPipe_PipeObject *p)
{
	int consumed;
	p->PAUSED = 0;
	p->processingLoop = 1;
	while (p->readOffset != 0 && p->PAUSED == 0)
	{
		consumed = 0;
		((ILibProcessPipe_GenericReadHandler)p->handler)(p->buffer + p->readOffset, p->totalRead - p->readOffset, &consumed, p->user1, p->user2);
		if (consumed == 0)
		{
			//
			// None of the buffer was consumed
			//
			ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibProcessPipe[ReadHandler]: No bytes consumed on Pipe: %p", (void*)p);

			//
			// We need to move the memory to the start of the buffer, or else we risk running past the end, if we keep reading like this
			//
			memmove_s(p->buffer, p->bufferSize, p->buffer + p->readOffset, p->totalRead - p->readOffset);
			p->totalRead -= p->readOffset;
			p->readOffset = 0;
		}
		else if (consumed == (p->totalRead - p->readOffset))
		{
			//
			// Entire Buffer was consumed
			//
			p->readOffset = 0;
			p->totalRead = 0;

			ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibProcessPipe[ReadHandler]: ReadBuffer drained on Pipe: %p", (void*)p);
			break; // Break out of inner while loop
		}
		else
		{
			//
			// Only part of the buffer was consumed
			//
			p->readOffset += consumed;
		}
	}
	p->processingLoop = 0;
}
#ifdef WIN32
void __stdcall ILibProcessPipe_Pipe_ResumeEx_APC(ULONG_PTR obj)
{
	ILibProcessPipe_PipeObject *p = (ILibProcessPipe_PipeObject*)obj;
	
	ILibProcessPipe_Pipe_ResumeEx_ContinueProcessing(p);

	if (p->PAUSED == 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.ResumeEx_APC(): Adding Pipe [%p]", (void*)p);
		if (p->mOverlapped_opaqueData != NULL)
		{
			ILibProcessPipe_WaitHandle_AddEx(p->manager, (ILibProcessPipe_WaitHandle*)p->mOverlapped_opaqueData);
			p->mOverlapped_opaqueData = NULL;
		}
		else
		{
			ILibProcessPipe_WaitHandle_Add(p->manager, p->mOverlapped->hEvent, p, ILibProcessPipe_Process_ReadHandler);
		}
		ReadFile(p->mPipe_ReadEnd, p->buffer, p->bufferSize, NULL, p->mOverlapped);
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.ResumeEx_APC(): Skipping Pipe [%p]", (void*)p);
	}
}
#endif
void ILibProcessPipe_Pipe_ResumeEx(ILibProcessPipe_PipeObject* p)
{
	ILibRemoteLogging_printf(ILibChainGetLogger(p->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ProcessPipe.ResumeEx(): processingLoop = %d", p->processingLoop);

#ifdef WIN32
	if (p->manager->workerThreadID == GetCurrentThreadId())
	{
		// We were called from the event dispatch (ie: we're on the same thread)
		if (p->mOverlapped_opaqueData != NULL)
		{
			ILibProcessPipe_WaitHandle_AddEx(p->manager, (ILibProcessPipe_WaitHandle*)p->mOverlapped_opaqueData);
			p->mOverlapped_opaqueData = NULL;
		}
		else
		{
			ILibProcessPipe_WaitHandle_Add(p->manager, p->mOverlapped->hEvent, p, ILibProcessPipe_Process_ReadHandler);
		}
		p->PAUSED = 0;
	}
	else
	{
		// We're on a different thread, so we need to dispatch to the worker thread
		QueueUserAPC((PAPCFUNC)ILibProcessPipe_Pipe_ResumeEx_APC, p->manager->workerThread, (ULONG_PTR)p);
	}
	return;
#endif

	ILibProcessPipe_Pipe_ResumeEx_ContinueProcessing(p);
	if (p->PAUSED == 0)
	{
		ILibLifeTime_Add(ILibGetBaseTimer(p->manager->ChainLink.ParentChain), p, 0, &ILibProcessPipe_Process_StartPipeReaderWriterEx, NULL); // Need to context switch to Chain Thread
	}
}
void ILibProcessPipe_Pipe_Resume(ILibProcessPipe_Pipe pipeObject)
{
	ILibProcessPipe_PipeObject *p = (ILibProcessPipe_PipeObject*)pipeObject;
#ifdef WIN32
	if (p->mOverlapped == NULL)
	{
		SetEvent(p->mPipe_Reader_ResumeEvent);
	}
	else
	{
		ILibProcessPipe_Pipe_ResumeEx(p);
	}
#else
	ILibProcessPipe_Pipe_ResumeEx(p);
#endif
}

#ifdef WIN32
DWORD ILibProcessPipe_Pipe_BackgroundReader(void *arg)
{
	ILibProcessPipe_PipeObject *pipeObject = (ILibProcessPipe_PipeObject*)arg;
	DWORD bytesRead = 0;
	int consumed;

	while (pipeObject->PAUSED == 0 || WaitForSingleObject(pipeObject->mPipe_Reader_ResumeEvent, INFINITE) == WAIT_OBJECT_0)
	{
		// Pipe is in ACTIVE state
		consumed = 0;
		pipeObject->PAUSED = 0;

		while (pipeObject->readOffset != 0 && pipeObject->PAUSED == 0)
		{
			((ILibProcessPipe_GenericReadHandler)pipeObject->handler)(pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset, &consumed, pipeObject->user1, pipeObject->user2);
			if (consumed == 0)
			{
				memmove_s(pipeObject->buffer, pipeObject->bufferSize, pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset);
				pipeObject->totalRead -= pipeObject->readOffset;
				pipeObject->readOffset = 0;
			}
			else if (consumed == (pipeObject->totalRead - pipeObject->readOffset))
			{
				// Entire buffer consumed
				pipeObject->readOffset = 0;
				pipeObject->totalRead = 0;
				consumed = 0;
			}
			else
			{
				// Partial Consumed
				pipeObject->readOffset += consumed;
			}
		}

		if (pipeObject->PAUSED == 1) { continue; }
		if (!ReadFile(pipeObject->mPipe_ReadEnd, pipeObject->buffer + pipeObject->readOffset, pipeObject->bufferSize - pipeObject->readOffset, &bytesRead, NULL)) { break; }

		consumed = 0;
		pipeObject->totalRead += bytesRead;
		((ILibProcessPipe_GenericReadHandler)pipeObject->handler)(pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset, &consumed, pipeObject->user1, pipeObject->user2);
		pipeObject->readOffset += consumed;
	}

	if (pipeObject->brokenPipeHandler != NULL) { pipeObject->brokenPipeHandler(pipeObject); }
	ILibProcessPipe_FreePipe(pipeObject);

	return(0);
}
#endif

void ILibProcessPipe_Process_StartPipeReader(ILibProcessPipe_PipeObject *pipeObject, int bufferSize, ILibProcessPipe_GenericReadHandler handler, void* user1, void* user2)
{
#ifdef WIN32
	BOOL result;
#endif

	if ((pipeObject->buffer = (char*)malloc(bufferSize)) == NULL) { ILIBCRITICALEXIT(254); }
	pipeObject->bufferSize = bufferSize;
	pipeObject->handler = (void*)handler;
	pipeObject->user1 = user1;
	pipeObject->user2 = user2;

#ifdef WIN32
	if (pipeObject->mOverlapped != NULL)
	{
		// This PIPE supports Overlapped I/O
		result = ReadFile(pipeObject->mPipe_ReadEnd, pipeObject->buffer, pipeObject->bufferSize, NULL, pipeObject->mOverlapped);
		ILibProcessPipe_WaitHandle_Add(pipeObject->manager, pipeObject->mOverlapped->hEvent, pipeObject, &ILibProcessPipe_Process_ReadHandler);
	}
	else
	{
		// This PIPE does NOT support overlapped I/O, so we have to fake it with a background thread
		pipeObject->mPipe_Reader_ResumeEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		ILibSpawnNormalThread(&ILibProcessPipe_Pipe_BackgroundReader, pipeObject);
	}
#else
	ILibLifeTime_Add(ILibGetBaseTimer(pipeObject->manager->ChainLink.ParentChain), pipeObject, 0, &ILibProcessPipe_Process_StartPipeReaderWriterEx, NULL); // Need to context switch to Chain Thread
#endif
}
void ILibProcessPipe_Process_PipeHandler_StdOut(char *buffer, int bufferLen, int* bytesConsumed, void* user1, void *user2)
{
	ILibProcessPipe_Process_Object *j = (ILibProcessPipe_Process_Object*)user1;
	if (user2 != NULL)
	{
		((ILibProcessPipe_Process_OutputHandler)user2)(j, buffer, bufferLen, bytesConsumed, j->userObject);
	}
}
void ILibProcessPipe_Process_PipeHandler_StdIn(void *user1, void *user2)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)user1;
	ILibProcessPipe_Process_SendOKHandler sendOk = (ILibProcessPipe_Process_SendOKHandler)user2;

	if (sendOk != NULL) sendOk(j, j->userObject);
}
#ifdef WIN32
void ILibProcessPipe_Process_OnExit_ChainSink(void *chain, void *user)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)user;
	DWORD exitCode;
	BOOL result;

	result = GetExitCodeProcess(j->hProcess, &exitCode);
	j->exiting = 1;
	j->exitHandler(j, exitCode, j->userObject);
	j->exiting = 0;

	ILibProcessPipe_Process_Destroy(j);
}
BOOL ILibProcessPipe_Process_OnExit(HANDLE event, void* user)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)user;

	UNREFERENCED_PARAMETER(event);
	ILibProcessPipe_WaitHandle_Remove(j->parent, j->hProcess);

	if (j->exitHandler != NULL)
	{
		// Everyone's lifes is made easier, by context switching to chain thread before making this call
		ILibChain_RunOnMicrostackThread(j->parent->ChainLink.ParentChain, ILibProcessPipe_Process_OnExit_ChainSink, user);
	}
	else
	{
		ILibProcessPipe_Process_Destroy(j);
	}
	return(FALSE);
}
#endif
void ILibProcessPipe_Process_UpdateUserObject(ILibProcessPipe_Process module, void *userObj)
{
	((ILibProcessPipe_Process_Object*)module)->userObject = userObj;
}
void ILibProcessPipe_Process_AddHandlers(ILibProcessPipe_Process module, int bufferSize, ILibProcessPipe_Process_ExitHandler exitHandler, ILibProcessPipe_Process_OutputHandler stdOut, ILibProcessPipe_Process_OutputHandler stdErr, ILibProcessPipe_Process_SendOKHandler sendOk, void *user)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)module;
	j->userObject = user;
	j->exitHandler = exitHandler;

	ILibProcessPipe_Process_StartPipeReader(j->stdOut, bufferSize, &ILibProcessPipe_Process_PipeHandler_StdOut, j, stdOut);
	ILibProcessPipe_Process_StartPipeReader(j->stdErr, bufferSize, &ILibProcessPipe_Process_PipeHandler_StdOut, j, stdErr);
	ILibProcessPipe_Process_SetWriteHandler(j->stdIn, &ILibProcessPipe_Process_PipeHandler_StdIn, j, sendOk);
	
#ifdef WIN32
	ILibProcessPipe_WaitHandle_Add(j->parent, j->hProcess, j, &ILibProcessPipe_Process_OnExit);
#endif
}

ILibTransport_DoneState ILibProcessPipe_Pipe_Write(ILibProcessPipe_Pipe po, char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership)
{
	ILibProcessPipe_PipeObject* pipeObject = (ILibProcessPipe_PipeObject*)po;
	ILibTransport_DoneState retVal = ILibTransport_DoneState_ERROR;

	if (pipeObject->WriteBuffer == NULL)
	{
		pipeObject->WriteBuffer = ILibQueue_Create();
	}

	ILibQueue_Lock(pipeObject->WriteBuffer);
	if (ILibQueue_IsEmpty(pipeObject->WriteBuffer) == 0)
	{
		ILibQueue_EnQueue(pipeObject->WriteBuffer, ILibProcessPipe_WriteData_Create(buffer, bufferLen, ownership));
	}
	else
	{
#ifdef WIN32
		BOOL result = WriteFile(pipeObject->mPipe_WriteEnd, buffer, bufferLen, NULL, pipeObject->mOverlapped);
		if (result == TRUE) { retVal = ILibTransport_DoneState_COMPLETE; }
#else
		int result = (int)write(pipeObject->mPipe_WriteEnd, buffer, bufferLen);
		if (result > 0) { retVal = ILibTransport_DoneState_COMPLETE; }
#endif
		else
		{
#ifdef WIN32
			if (GetLastError() == ERROR_IO_PENDING)
#else
			if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
#endif
			{
				retVal = ILibTransport_DoneState_INCOMPLETE;
				ILibQueue_EnQueue(pipeObject->WriteBuffer, ILibProcessPipe_WriteData_Create(buffer, bufferLen, ownership));
#ifdef WIN32
				ILibProcessPipe_WaitHandle_Add(pipeObject->manager, pipeObject->mOverlapped->hEvent, pipeObject, &ILibProcessPipe_Process_WindowsWriteHandler);
#else
				ILibLifeTime_Add(ILibGetBaseTimer(pipeObject->manager->ChainLink.ParentChain), pipeObject, 0, &ILibProcessPipe_Process_StartPipeReaderWriterEx, NULL); // Need to context switch to Chain Thread
#endif
			}
			else
			{
#ifdef WIN32
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibProcessPipe[Write]: BrokenPipe(%d) on Pipe: %p", GetLastError(), (void*)pipeObject);
#else
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->ChainLink.ParentChain), ILibRemoteLogging_Modules_Microstack_Pipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibProcessPipe[Write]: BrokenPipe(%d) on Pipe: %p", result < 0 ? errno : 0, (void*)pipeObject);
#endif
				ILibQueue_UnLock(pipeObject->WriteBuffer);
				if (pipeObject->brokenPipeHandler != NULL)
				{
#ifdef WIN32
					ILibProcessPipe_WaitHandle_Remove(pipeObject->manager, pipeObject->mOverlapped->hEvent); // Pipe Broken, so remove ourselves from the processing loop
#endif
					((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject);
				}
				ILibProcessPipe_FreePipe(pipeObject);
				return(ILibTransport_DoneState_ERROR);
			}
		}
	}
	ILibQueue_UnLock(pipeObject->WriteBuffer);
	
	return(retVal);
}

ILibTransport_DoneState ILibProcessPipe_Process_WriteStdIn(ILibProcessPipe_Process p, char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership)
{
	ILibProcessPipe_Process_Object *j = (ILibProcessPipe_Process_Object*)p;
	
	return(ILibProcessPipe_Pipe_Write(j->stdIn, buffer, bufferLen, ownership));
}

void ILibProcessPipe_Pipe_ReadSink(char *buffer, int bufferLen, int* bytesConsumed, void* user1, void* user2)
{
	ILibProcessPipe_Pipe target = (ILibProcessPipe_Pipe)user1;

	if (user2 != NULL) { ((ILibProcessPipe_Pipe_ReadHandler)user2)(target, buffer, bufferLen, bytesConsumed); }
}
void ILibProcessPipe_Pipe_AddPipeReadHandler(ILibProcessPipe_Pipe targetPipe, int bufferSize, ILibProcessPipe_Pipe_ReadHandler OnReadHandler)
{
	ILibProcessPipe_Process_StartPipeReader(targetPipe, bufferSize, &ILibProcessPipe_Pipe_ReadSink, targetPipe, OnReadHandler);
}

