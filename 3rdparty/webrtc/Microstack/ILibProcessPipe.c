/*
Copyright 2015 Intel Corporation

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
#if defined(_MSC_VER) && (_MSC_FULL_VER <= 180000000)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif
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
#endif

typedef struct ILibProcessPipe_Manager_Object
{
	ILibChain_PreSelect Pre;
	ILibChain_PostSelect Post;
	ILibChain_Destroy Destroy;

	void* chain;
	ILibLinkedList ActivePipes;
	ILibQueue WaitHandles_PendingAdd;
	ILibQueue WaitHandles_PendingDel;
#ifdef WIN32
	int abort;
	HANDLE updateEvent;
	HANDLE workerThread;
#endif
}ILibProcessPipe_Manager_Object;

typedef void(*ILibProcessPipe_GenericReadHandler)(char *buffer, int bufferLen, int* bytesConsumed, void* user1, void* user2);
typedef void(*ILibProcessPipe_GenericSendOKHandler)(void* user1, void* user2);
struct ILibProcessPipe_Process_Object; // Forward Prototype

typedef struct ILibProcessPipe_PipeObject
{
	char* buffer;
	int bufferSize;

	int readOffset;
	int totalRead;

	ILibProcessPipe_Manager_Object *manager;
	struct ILibProcessPipe_Process_Object* mProcess;
	ILibQueue WriteBuffer;
	void *handler;
	void* brokenPipeHandler;
	void *user1, *user2;
#ifdef WIN32
	HANDLE mPipe_ReadEnd;
	HANDLE mPipe_WriteEnd;
	struct _OVERLAPPED *mOverlapped;
#else
	int mPipe_ReadEnd, mPipe_WriteEnd;
#endif
}ILibProcessPipe_PipeObject;

typedef void(*ILibProcessPipe_GenericBrokenPipeHandler)(ILibProcessPipe_PipeObject* sender);

typedef struct ILibProcessPipe_Process_Object
{
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

ILibProcessPipe_WriteData* ILibProcessPipe_WriteData_Create(char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership)
{
	ILibProcessPipe_WriteData* retVal;

	if ((retVal = (ILibProcessPipe_WriteData*)malloc(sizeof(ILibProcessPipe_WriteData))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(ILibProcessPipe_WriteData));
	retVal->bufferLen = bufferLen;
	if (ownership == ILibTransport_MemoryOwnership_USER)
	{
		if ((retVal->buffer = (char*)malloc(bufferLen)) == NULL) { ILIBCRITICALEXIT(254); }
		memcpy(retVal->buffer, buffer, bufferLen);
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

#ifdef WIN32
typedef void(*ILibProcessPipe_WaitHandle_Handler)(HANDLE event, void* user);
typedef struct ILibProcessPipe_WaitHandle
{
	HANDLE event;
	void *user;
	ILibProcessPipe_WaitHandle_Handler callback;
}ILibProcessPipe_WaitHandle;
void ILibProcessPipe_WaitHandle_Remove(ILibProcessPipe_Manager_Object *manager, HANDLE event)
{
	ILibQueue_Lock(manager->WaitHandles_PendingDel);
	ILibQueue_EnQueue(manager->WaitHandles_PendingDel, event);
	ILibQueue_UnLock(manager->WaitHandles_PendingDel);
	SetEvent(manager->updateEvent);
}
void ILibProcessPipe_WaitHandle_Add(ILibProcessPipe_Manager_Object *manager, HANDLE event, void *user, ILibProcessPipe_WaitHandle_Handler callback)
{
	ILibProcessPipe_WaitHandle *waitHandle;
	if ((waitHandle = (ILibProcessPipe_WaitHandle*)malloc(sizeof(ILibProcessPipe_WaitHandle))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(waitHandle, 0, sizeof(ILibProcessPipe_WaitHandle));

	waitHandle->event = event;
	waitHandle->user = user;
	waitHandle->callback = callback;

	ILibQueue_Lock(manager->WaitHandles_PendingAdd);
	ILibQueue_EnQueue(manager->WaitHandles_PendingAdd, waitHandle);
	ILibQueue_UnLock(manager->WaitHandles_PendingAdd);
	SetEvent(manager->updateEvent);
}
int ILibProcessPipe_Manager_WindowsWaitHandles_Remove_Comparer(void *source, void *matchWith)
{
	return(((ILibProcessPipe_WaitHandle*)source)->event == matchWith ? 0 : 1);
}
void ILibProcessPipe_Manager_WindowsRunLoop(void *arg)
{
	ILibProcessPipe_Manager_Object *manager = (ILibProcessPipe_Manager_Object*)arg;
	HANDLE* hList = (HANDLE*)malloc(FD_SETSIZE * sizeof(HANDLE)* 2);
	ILibLinkedList active = manager->ActivePipes;

	void* node;
	ILibProcessPipe_WaitHandle* data;

	int i, x;

	memset(hList, 0, FD_SETSIZE * sizeof(HANDLE)* 2);

	while (manager->abort == 0)
	{
		hList[0] = manager->updateEvent;
		i = 1;

		//Pending Remove
		ILibQueue_Lock(manager->WaitHandles_PendingDel);
		while (ILibQueue_IsEmpty(manager->WaitHandles_PendingDel) == 0)
		{
			node = ILibQueue_DeQueue(manager->WaitHandles_PendingDel);
			node = ILibLinkedList_GetNode_Search(active, &ILibProcessPipe_Manager_WindowsWaitHandles_Remove_Comparer, node);
			if (node != NULL) { free(ILibLinkedList_GetDataFromNode(node)); ILibLinkedList_Remove(node); }
		}
		ILibQueue_UnLock(manager->WaitHandles_PendingDel);

		//Pending Add
		ILibQueue_Lock(manager->WaitHandles_PendingAdd);
		while (ILibQueue_IsEmpty(manager->WaitHandles_PendingAdd) == 0)
		{
			ILibLinkedList_AddTail(active, ILibQueue_DeQueue(manager->WaitHandles_PendingAdd));
		}
		ILibQueue_UnLock(manager->WaitHandles_PendingAdd);

		//Prepare the rest of the WaitHandle Array, for the WaitForMultipleObject call
		node = ILibLinkedList_GetNode_Head(active);
		while (node != NULL && (data = (ILibProcessPipe_WaitHandle*)ILibLinkedList_GetDataFromNode(node)) != NULL)
		{
			hList[i] = data->event;
			hList[i + FD_SETSIZE] = (HANDLE)data;
			++i;
			node = ILibLinkedList_GetNextNode(node);
		}


		while ((x = WaitForMultipleObjects(i, hList, FALSE, INFINITE) - WAIT_OBJECT_0) != 0)
		{
			data = (ILibProcessPipe_WaitHandle*)hList[x + FD_SETSIZE];
			if (data->callback != NULL) { data->callback(data->event, data->user); }
		}
		ResetEvent(manager->updateEvent);
	}

	for (x = 1; x<(1 + ILibLinkedList_GetCount(active)); ++x)
	{
		free(hList[x + FD_SETSIZE]);
	}
	free(hList);
}
void ILibProcessPipe_Manager_Start(void* chain, void* user)
{ 
	ILibProcessPipe_Manager_Object* man = (ILibProcessPipe_Manager_Object*)user;
	man->workerThread = ILibSpawnNormalThread((voidfp)&ILibProcessPipe_Manager_WindowsRunLoop, user);
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
		if(j->mPipe_ReadEnd != 0)
		{
			FD_SET(j->mPipe_ReadEnd, readset);
		}
		node = ILibLinkedList_GetNextNode(node);
	}

}
void ILibProcessPipe_Manager_OnPostSelect(void* object, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset)
{
	ILibProcessPipe_Manager_Object *man = (ILibProcessPipe_Manager_Object*)object;
	void *node;
	ILibProcessPipe_PipeObject *j;

	node = ILibLinkedList_GetNode_Head(man->ActivePipes);
	while(node != NULL && (j = (ILibProcessPipe_PipeObject*)ILibLinkedList_GetDataFromNode(node)) != NULL)
	{
		if(j->mPipe_ReadEnd != 0)
		{
			if(FD_ISSET(j->mPipe_ReadEnd, readset) != 0)
			{
				ILibProcessPipe_Process_ReadHandler(j);
			}
		}
		node = ILibLinkedList_GetNextNode(node);
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
	ILibQueue_Destroy(man->WaitHandles_PendingAdd);
	ILibQueue_Destroy(man->WaitHandles_PendingDel);
}
ILibProcessPipe_Manager ILibProcessPipe_Manager_Create(void *chain)
{
	ILibProcessPipe_Manager_Object *retVal;

	if ((retVal = (ILibProcessPipe_Manager_Object*)malloc(sizeof(ILibProcessPipe_Manager_Object))) == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(ILibProcessPipe_Manager_Object));

	retVal->chain = chain;
	retVal->ActivePipes = ILibLinkedList_Create();
	retVal->WaitHandles_PendingAdd = ILibQueue_Create();
	retVal->WaitHandles_PendingDel = ILibQueue_Create();

#ifdef WIN32
	retVal->updateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ILibChain_OnStartEvent_AddHandler(chain, (ILibChain_StartEvent)&ILibProcessPipe_Manager_Start, retVal);
#else
	retVal->Pre = &ILibProcessPipe_Manager_OnPreSelect;
	retVal->Post = &ILibProcessPipe_Manager_OnPostSelect;
#endif
	retVal->Destroy = &ILibProcessPipe_Manager_OnDestroy;
	ILibAddToChain(chain, retVal);
	return(retVal);
}

void ILibProcessPipe_FreePipe(ILibProcessPipe_PipeObject *pipeObject)
{
#ifdef WIN32
	CloseHandle(pipeObject->mPipe_ReadEnd);
	CloseHandle(pipeObject->mPipe_WriteEnd);
	CloseHandle(pipeObject->mOverlapped->hEvent);
	free(pipeObject->mOverlapped);
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

	snprintf(pipeName, sizeof(pipeName), "\\\\.\\pipe\\%p", (void*)retVal);


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
	ILibProcessPipe_FreePipe(p->stdIn);
	ILibProcessPipe_FreePipe(p->stdOut);
	ILibProcessPipe_FreePipe(p->stdErr);
	free(p);
}
#ifndef WIN32
void ILibProcessPipe_Process_BrokenPipeSink(ILibProcessPipe_PipeObject* sender)
{
	ILibProcessPipe_Process_Object *p = sender->mProcess;
	int status;

	if (ILibIsRunningOnChainThread(sender->manager->chain) != 0)
	{
		// This was called from the Reader
		if (p->exitHandler != NULL)
		{
			waitpid(p->PID, &status, 0);
			p->exitHandler(p, WEXITSTATUS(status), p->userObject);
		}
	}
}
#endif

int ILibProcessPipe_Process_GetPID(ILibProcessPipe_Process p)
{
	return(p != NULL ? ((ILibProcessPipe_Process_Object*)p)->PID : 0);
}
ILibProcessPipe_Process ILibProcessPipe_Manager_SpawnProcess(ILibProcessPipe_Manager pipeManager, char* target, char* parameters[])
{
	ILibProcessPipe_Process_Object* retVal = NULL;

#ifdef WIN32
	STARTUPINFOA info = { sizeof(info) };
	PROCESS_INFORMATION processInfo = { 0 };
	SECURITY_ATTRIBUTES saAttr;
	char* parms = NULL;

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
			sz += (strlen(parameters[i++]) + 1);
		}
		sz += (i - 1); // Need to make room for delimiter characters
		parms = (char*)malloc(sz); 
		i = 0; len = 0;

		while (parameters[i] != NULL)
		{
			len += snprintf(parms + len, sz - len, "%s%s", (i==0)?"":" ", parameters[i]);
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
	
	retVal->stdIn = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
	retVal->stdIn->mProcess = retVal;
#ifdef WIN32
	retVal->stdOut = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
#else
	retVal->stdOut = ILibProcessPipe_CreatePipe(pipeManager, 4096, &ILibProcessPipe_Process_BrokenPipeSink);
#endif
	retVal->stdOut->mProcess = retVal;
	retVal->stdErr = ILibProcessPipe_CreatePipe(pipeManager, 4096, NULL);
	retVal->stdErr->mProcess = retVal;
	retVal->parent = (ILibProcessPipe_Manager_Object*)pipeManager;
	

#ifdef WIN32
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&info, sizeof(STARTUPINFOA));

	ILibProcessPipe_PipeObject_DisableInherit(&(retVal->stdIn->mPipe_WriteEnd));
	ILibProcessPipe_PipeObject_DisableInherit(&(retVal->stdOut->mPipe_ReadEnd));
	ILibProcessPipe_PipeObject_DisableInherit(&(retVal->stdErr->mPipe_ReadEnd));

	info.cb = sizeof(STARTUPINFOA);
	info.hStdError = retVal->stdErr->mPipe_WriteEnd;
	info.hStdInput = retVal->stdIn->mPipe_ReadEnd;
	info.hStdOutput = retVal->stdOut->mPipe_WriteEnd;
	info.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcessA(target, parms, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		ILibProcessPipe_FreePipe(retVal->stdErr);
		ILibProcessPipe_FreePipe(retVal->stdOut);
		ILibProcessPipe_FreePipe(retVal->stdIn);
		free(retVal);
		return(NULL);
	}

	CloseHandle(retVal->stdOut->mPipe_WriteEnd);	retVal->stdOut->mPipe_WriteEnd = NULL;
	CloseHandle(retVal->stdErr->mPipe_WriteEnd);	retVal->stdErr->mPipe_WriteEnd = NULL;
	CloseHandle(retVal->stdIn->mPipe_ReadEnd);		retVal->stdIn->mPipe_ReadEnd = NULL;

	retVal->hProcess = processInfo.hProcess;
	if (processInfo.hThread != NULL) CloseHandle(processInfo.hThread);
	retVal->PID = processInfo.dwProcessId;
#else
	pid = vfork();
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
		close(retVal->stdIn->mPipe_WriteEnd); //close write end of stdin pipe
		close(retVal->stdOut->mPipe_ReadEnd); //close read end of stdout pipe
		
		dup2(retVal->stdIn->mPipe_ReadEnd, STDIN_FILENO);
		dup2(retVal->stdOut->mPipe_WriteEnd, STDOUT_FILENO);
		
		close(retVal->stdIn->mPipe_ReadEnd);
		close(retVal->stdOut->mPipe_WriteEnd);
		
		execv(target, parameters);
		exit(1);
	}
	close(retVal->stdIn->mPipe_ReadEnd);
	close(retVal->stdOut->mPipe_WriteEnd);
	retVal->PID = pid;
#endif


	return(retVal);
}
#ifdef WIN32
void ILibProcessPipe_Process_ReadHandler(HANDLE event, void* user)
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
#else
	int bytesRead;
#endif

	do
	{
#ifdef WIN32
		result = GetOverlappedResult(pipeObject->mPipe_ReadEnd, pipeObject->mOverlapped, &bytesRead, FALSE);
		if (result == FALSE) { break; }
#else
		bytesRead = read(pipeObject->mPipe_ReadEnd, pipeObject->buffer, pipeObject->bufferSize);
		if(bytesRead <= 0)
		{
			break;
		}

#endif
		
		pipeObject->totalRead += bytesRead;
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibNamedPipe[ReadHandler]: %u bytes read on Pipe: %p", bytesRead, (void*)pipeObject);

		if (pipeObject->handler == NULL)
		{
			//
			// Since the user doesn't care about the data, we'll just empty the buffer
			//
			pipeObject->readOffset = 0;
			pipeObject->totalRead = 0;
			continue;
		}

		while (1)
		{
			consumed = 0;
			((ILibProcessPipe_GenericReadHandler)pipeObject->handler)(pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset, &consumed, pipeObject->user1, pipeObject->user2);
			if (consumed == 0)
			{
				//
				// None of the buffer was consumed
				//
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibNamedPipe[ReadHandler]: No bytes consumed on Pipe: %p", (void*)pipeObject);

				//
				// We need to move the memory to the start of the buffer, or else we risk running past the end, if we keep reading like this
				//
				memmove(pipeObject->buffer, pipeObject->buffer + pipeObject->readOffset, pipeObject->totalRead - pipeObject->readOffset);
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

				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibNamedPipe[ReadHandler]: ReadBuffer drained on Pipe: %p", (void*)pipeObject);
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
	}
#ifdef WIN32
	while ((result = ReadFile(pipeObject->mPipe_ReadEnd, pipeObject->buffer, pipeObject->bufferSize, NULL, pipeObject->mOverlapped)) == TRUE); // Note: This is actually the end of a do-while loop
	if ((err = GetLastError()) != ERROR_IO_PENDING)
#else
	while(1); // Note: This is actually the end of a do-while loop
	err = 0;
	if(bytesRead == 0 || ((err = errno) != EAGAIN && errno != EWOULDBLOCK))
#endif
	{
		//
		// Broken Pipe
		//
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibNamedPipe[ReadHandler]: BrokenPipe(%d) on Pipe: %p", err, (void*)pipeObject);
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
		return;
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_5, "ILibNamedPipe[ReadHandler]: Pipe: %p [EMPTY]", (void*)pipeObject);
	}

}
#ifdef WIN32
void ILibProcessPipe_Process_WindowsWriteHandler(HANDLE event, void* user)
{
	ILibProcessPipe_PipeObject *pipeObject = (ILibProcessPipe_PipeObject*)user;
	BOOL result;
	DWORD bytesWritten;
	ILibProcessPipe_WriteData* data;
	
	result = GetOverlappedResult(pipeObject->mPipe_WriteEnd, pipeObject->mOverlapped, &bytesWritten, FALSE);
	if (result == FALSE)
	{ 
		// Broken Pipe
		ILibProcessPipe_WaitHandle_Remove(pipeObject->manager, pipeObject->mOverlapped->hEvent); // Pipe Broken, so remove ourselves from the processing loop
		ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibNamedPipe[WriteHandler]: BrokenPipe(%d) on Pipe: %p", GetLastError(), (void*)pipeObject);
		if (pipeObject->brokenPipeHandler != NULL) { ((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject); }
		return;
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
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibNamedPipe[WriteHandler]: BrokenPipe(%d) on Pipe: %p", GetLastError(), (void*)pipeObject);
				if (pipeObject->brokenPipeHandler != NULL) { ((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject); }
				return;
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
	return;
}
#endif
void ILibProcessPipe_Process_SetWriteHandler(ILibProcessPipe_PipeObject *pipeObject, ILibProcessPipe_GenericSendOKHandler handler, void* user1, void* user2)
{
	pipeObject->handler = handler;
	pipeObject->user1 = user1;
	pipeObject->user2 = user2;
}

void ILibProcessPipe_Process_StartPipeReaderWriterEx(void *object)
{
	ILibProcessPipe_PipeObject* pipeObject = (ILibProcessPipe_PipeObject*)object;
	ILibLinkedList_AddTail(pipeObject->manager->ActivePipes, pipeObject);
}

void ILibProcessPipe_Process_StartPipeReader(ILibProcessPipe_PipeObject *pipeObject, int bufferSize, ILibProcessPipe_GenericReadHandler handler, void* user1, void* user2)
{
#ifdef WIN32
	BOOL result;
#endif

	if ((pipeObject->buffer = (char*)malloc(bufferSize)) == NULL) { ILIBCRITICALEXIT(254); }
	pipeObject->bufferSize = bufferSize;
	pipeObject->handler = handler;
	pipeObject->user1 = user1;
	pipeObject->user2 = user2;

#ifdef WIN32
	result = ReadFile(pipeObject->mPipe_ReadEnd, pipeObject->buffer, pipeObject->bufferSize, NULL, pipeObject->mOverlapped);
	ILibProcessPipe_WaitHandle_Add(pipeObject->manager, pipeObject->mOverlapped->hEvent, pipeObject, &ILibProcessPipe_Process_ReadHandler);
#else
	ILibLifeTime_Add(ILibGetBaseTimer(pipeObject->manager->chain), pipeObject, 0, &ILibProcessPipe_Process_StartPipeReaderWriterEx, NULL); // Need to context switch to Chain Thread
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
void ILibProcessPipe_Process_OnExit(HANDLE event, void* user)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)user;
	DWORD exitCode;
	BOOL result;

	ILibProcessPipe_WaitHandle_Remove(j->parent, j->hProcess);
	result = GetExitCodeProcess(j->hProcess, &exitCode);
	if (j->exitHandler != NULL)
	{
		j->exitHandler(j, exitCode, j->userObject);
	}
	ILibProcessPipe_Process_Destroy(j);
}
#endif

void ILibProcessPipe_Process_AddHandlers(ILibProcessPipe_Process module, ILibProcessPipe_Process_ExitHandler exitHandler, ILibProcessPipe_Process_OutputHandler stdOut, ILibProcessPipe_Process_OutputHandler stdErr, ILibProcessPipe_Process_SendOKHandler sendOk, void *user)
{
	ILibProcessPipe_Process_Object* j = (ILibProcessPipe_Process_Object*)module;
	j->userObject = user;
	j->exitHandler = exitHandler;

	ILibProcessPipe_Process_StartPipeReader(j->stdOut, 4096, &ILibProcessPipe_Process_PipeHandler_StdOut, j, stdOut);
	ILibProcessPipe_Process_SetWriteHandler(j->stdIn, &ILibProcessPipe_Process_PipeHandler_StdIn, j, sendOk);
	
#ifdef WIN32
	ILibProcessPipe_WaitHandle_Add(j->parent, j->hProcess, j, &ILibProcessPipe_Process_OnExit);
#endif
}

ILibTransport_DoneState ILibProcessPipe_Pipe_Write(ILibProcessPipe_PipeObject* pipeObject, char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership)
{
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
		int result = write(pipeObject->mPipe_WriteEnd, buffer, bufferLen);
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
				ILibLifeTime_Add(ILibGetBaseTimer(pipeObject->manager->chain), pipeObject, 0, &ILibProcessPipe_Process_StartPipeReaderWriterEx, NULL); // Need to context switch to Chain Thread
#endif
			}
			else
			{
#ifdef WIN32
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibNamedPipe[Write]: BrokenPipe(%d) on Pipe: %p", GetLastError(), (void*)pipeObject);
#else
				ILibRemoteLogging_printf(ILibChainGetLogger(pipeObject->manager->chain), ILibRemoteLogging_Modules_Microstack_NamedPipe, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibNamedPipe[Write]: BrokenPipe(%d) on Pipe: %p", result < 0 ? errno : 0, (void*)pipeObject);
#endif
				if (pipeObject->brokenPipeHandler != NULL)
				{
					ILibQueue_UnLock(pipeObject->WriteBuffer);
#ifdef WIN32
					ILibProcessPipe_WaitHandle_Remove(pipeObject->manager, pipeObject->mOverlapped->hEvent); // Pipe Broken, so remove ourselves from the processing loop
#endif
					((ILibProcessPipe_GenericBrokenPipeHandler)pipeObject->brokenPipeHandler)(pipeObject);
					ILibQueue_Lock(pipeObject->WriteBuffer);
				}
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

