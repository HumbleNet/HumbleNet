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

/*! \file ILibNamedPipe.h
\brief MicroStack APIs for various functions and tasks related to named pipes
*/

#ifndef __ILibProcessPipe__
#define __ILibProcessPipe__

#include "ILibParsers.h"

typedef void* ILibProcessPipe_Manager;
typedef void* ILibProcessPipe_Process;
typedef void(*ILibProcessPipe_Process_OutputHandler)(ILibProcessPipe_Process sender, char *buffer, int bufferLen, int* bytesConsumed, void* user);
typedef void(*ILibProcessPipe_Process_SendOKHandler)(ILibProcessPipe_Process sender, void* user);
typedef void(*ILibProcessPipe_Process_ExitHandler)(ILibProcessPipe_Process sender, int exitCode, void* user);

typedef void* ILibProcessPipe_Pipe;
typedef void(*ILibProcessPipe_Pipe_ReadHandler)(ILibProcessPipe_Pipe sender, char *buffer, int bufferLen, int* bytesConsumed);
typedef void(*ILibProcessPipe_Pipe_BrokenPipeHandler)(ILibProcessPipe_Pipe sender);

typedef enum ILibProcessPipe_SpawnTypes
{
	ILibProcessPipe_SpawnTypes_DEFAULT		= 0,
	ILibProcessPipe_SpawnTypes_USER			= 1,
	ILibProcessPipe_SpawnTypes_WINLOGON		= 2,
	ILibProcessPipe_SpawnTypes_TERM			= 3,
}ILibProcessPipe_SpawnTypes;


extern const int ILibMemory_ILibProcessPipe_Pipe_CONTAINERSIZE;

#ifdef WIN32
typedef enum ILibProcessPipe_Pipe_ReaderHandleType
{
	ILibProcessPipe_Pipe_ReaderHandleType_NotOverLapped = 0,	//!< Spawn a I/O processing thread
	ILibProcessPipe_Pipe_ReaderHandleType_Overlapped = 1		//!< Use Overlapped I/O
}ILibProcessPipe_Pipe_ReaderHandleType;
#endif

ILibTransport_DoneState ILibProcessPipe_Pipe_Write(ILibProcessPipe_Pipe writePipe, char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership);
void ILibProcessPipe_Pipe_AddPipeReadHandler(ILibProcessPipe_Pipe targetPipe, int bufferSize, ILibProcessPipe_Pipe_ReadHandler OnReadHandler);
#ifdef WIN32
	ILibProcessPipe_Pipe ILibProcessPipe_Pipe_CreateFromExistingWithExtraMemory(ILibProcessPipe_Manager manager, HANDLE existingPipe, ILibProcessPipe_Pipe_ReaderHandleType handleType, int extraMemorySize);
	#define ILibProcessPipe_Pipe_CreateFromExisting(PipeManager, ExistingPipe, HandleType) ILibProcessPipe_Pipe_CreateFromExistingWithExtraMemory(PipeManager, ExistingPipe, HandleType, 0)
#else
	ILibProcessPipe_Pipe ILibProcessPipe_Pipe_CreateFromExistingWithExtraMemory(ILibProcessPipe_Manager manager, int existingPipe, int extraMemorySize);
	#define ILibProcessPipe_Pipe_CreateFromExisting(PipeManager, ExistingPipe) ILibProcessPipe_Pipe_CreateFromExistingWithExtraMemory(PipeManager, ExistingPipe, 0)
#endif

void ILibProcessPipe_Pipe_SetBrokenPipeHandler(ILibProcessPipe_Pipe targetPipe, ILibProcessPipe_Pipe_BrokenPipeHandler handler);

ILibProcessPipe_Manager ILibProcessPipe_Manager_Create(void *chain);
ILibProcessPipe_Process ILibProcessPipe_Manager_SpawnProcessEx(ILibProcessPipe_Manager pipeManager, char* target, char* const * parameters, ILibProcessPipe_SpawnTypes spawnType);
#define ILibProcessPipe_Manager_SpawnProcess(pipeManager, target, parameters) ILibProcessPipe_Manager_SpawnProcessEx(pipeManager, target, parameters, ILibProcessPipe_SpawnTypes_DEFAULT)
void* ILibProcessPipe_Process_Kill(ILibProcessPipe_Process p);
void* ILibProcessPipe_Process_KillEx(ILibProcessPipe_Process p);
int ILibProcessPipe_Process_GetPID(ILibProcessPipe_Process p);
void ILibProcessPipe_Process_AddHandlers(ILibProcessPipe_Process module, int bufferSize, ILibProcessPipe_Process_ExitHandler exitHandler, ILibProcessPipe_Process_OutputHandler stdOut, ILibProcessPipe_Process_OutputHandler stdErr, ILibProcessPipe_Process_SendOKHandler sendOk, void *user);
void ILibProcessPipe_Process_UpdateUserObject(ILibProcessPipe_Process module, void *userObj);
ILibTransport_DoneState ILibProcessPipe_Process_WriteStdIn(ILibProcessPipe_Process p, char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership);
void ILibProcessPipe_Pipe_Pause(ILibProcessPipe_Pipe pipeObject);
void ILibProcessPipe_Pipe_Resume(ILibProcessPipe_Pipe pipeObject);
void ILibProcessPipe_Pipe_SwapBuffers(ILibProcessPipe_Pipe pipeObject, char* newBuffer, int newBufferLen, int newBufferReadOffset, int newBufferTotalBytesRead, char **oldBuffer, int *oldBufferLen, int *oldBufferReadOffset, int *oldBufferTotalBytesRead);
ILibProcessPipe_Pipe ILibProcessPipe_Process_GetStdErr(ILibProcessPipe_Process p);
ILibProcessPipe_Pipe ILibProcessPipe_Process_GetStdOut(ILibProcessPipe_Process p);


#ifdef WIN32
typedef BOOL(*ILibProcessPipe_WaitHandle_Handler)(HANDLE event, void* user);
void ILibProcessPipe_WaitHandle_Add(ILibProcessPipe_Manager mgr, HANDLE event, void *user, ILibProcessPipe_WaitHandle_Handler callback);
void ILibProcessPipe_WaitHandle_Remove(ILibProcessPipe_Manager mgr, HANDLE event);
#endif
#define ILibTransports_ProcessPipe 0x60
#endif