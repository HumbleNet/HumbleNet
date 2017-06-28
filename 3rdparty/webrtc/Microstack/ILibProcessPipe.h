/*
Copyright 2006 - 2011 Intel Corporation

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

ILibProcessPipe_Manager ILibProcessPipe_Manager_Create(void *chain);
ILibProcessPipe_Process ILibProcessPipe_Manager_SpawnProcess(ILibProcessPipe_Manager pipeManager, char* target, char* parameters[]);
int ILibProcessPipe_Process_GetPID(ILibProcessPipe_Process p);
void ILibProcessPipe_Process_AddHandlers(ILibProcessPipe_Process module, ILibProcessPipe_Process_ExitHandler exitHandler, ILibProcessPipe_Process_OutputHandler stdOut, ILibProcessPipe_Process_OutputHandler stdErr, ILibProcessPipe_Process_SendOKHandler sendOk, void *user);
ILibTransport_DoneState ILibProcessPipe_Process_WriteStdIn(ILibProcessPipe_Process p, char* buffer, int bufferLen, ILibTransport_MemoryOwnership ownership);

#define ILibTransports_ProcessPipe 0x60
#endif