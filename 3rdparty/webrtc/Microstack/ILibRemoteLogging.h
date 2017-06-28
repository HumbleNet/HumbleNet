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

#ifndef __ILIBREMOTELOGGING__
#define __ILIBREMOTELOGGING__

#include "ILibParsers.h"

typedef enum ILibRemoteLogging_Modules
{
	ILibRemoteLogging_Modules_UNKNOWN			= 0x00,
	ILibRemoteLogging_Modules_WebRTC_STUN_ICE	= 0x02, 
	ILibRemoteLogging_Modules_WebRTC_DTLS		= 0x04, 
	ILibRemoteLogging_Modules_WebRTC_SCTP		= 0x08,
	ILibRemoteLogging_Modules_Agent_GuardPost	= 0x10,
	ILibRemoteLogging_Modules_Agent_P2P			= 0x20,
	ILibRemoteLogging_Modules_Agent_KVM			= 0x200,
	ILibRemoteLogging_Modules_Microstack_AsyncSocket	= 0x40, /* AsyncSocket, AsyncUDPSocket, and AsyncServerSocket*/
	ILibRemoteLogging_Modules_Microstack_Web			= 0x80,	/* WebClient, WebServer, and WebSocket*/
	ILibRemoteLogging_Modules_Microstack_NamedPipe		= 0x400,
	ILibRemoteLogging_Modules_Microstack_Generic		= 0x100 /* Generic */
}ILibRemoteLogging_Modules;

typedef enum ILibRemoteLogging_Flags
{
	ILibRemoteLogging_Flags_NONE				= 0x00,
	ILibRemoteLogging_Flags_DisableLogging		= 0x01,
	ILibRemoteLogging_Flags_VerbosityLevel_1	= 0x02, 
	ILibRemoteLogging_Flags_VerbosityLevel_2	= 0x04, 
	ILibRemoteLogging_Flags_VerbosityLevel_3	= 0x08, 
	ILibRemoteLogging_Flags_VerbosityLevel_4	= 0x10, 
	ILibRemoteLogging_Flags_VerbosityLevel_5	= 0x20, 
}ILibRemoteLogging_Flags;


typedef void* ILibRemoteLogging;
typedef void (*ILibRemoteLogging_OnWrite)(ILibRemoteLogging module, char* data, int dataLen, void *userContext);
typedef void (*ILibRemoteLogging_OnCommand)(ILibRemoteLogging sender, ILibRemoteLogging_Modules module, unsigned short flags, char* data, int dataLen, void *userContext);

#ifdef _REMOTELOGGING
	char* ILibRemoteLogging_ConvertAddress(struct sockaddr* addr);
	char* ILibRemoteLogging_ConvertToHex(char* inVal, int inValLength);
	void ILibRemoteLogging_printf(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module, ILibRemoteLogging_Flags flags, char* format, ...);

	ILibRemoteLogging ILibRemoteLogging_Create(ILibRemoteLogging_OnWrite onOutput);
	void ILibRemoteLogging_Destroy(ILibRemoteLogging logger);

	void ILibRemoteLogging_DeleteUserContext(ILibRemoteLogging logger, void *userContext);
	void ILibRemoteLogging_RegisterCommandSink(ILibRemoteLogging logger, ILibRemoteLogging_Modules module, ILibRemoteLogging_OnCommand sink);
	int ILibRemoteLogging_Dispatch(ILibRemoteLogging loggingModule, char* data, int dataLen, void *userContext);
	#define ILibRemoteLogging_ReadModuleType(data) ((ILibRemoteLogging_Modules)ntohs(((unsigned short*)data)[0]))
	#define ILibRemoteLogging_ReadFlags(data) ((ILibRemoteLogging_Flags)ntohs(((unsigned short*)data)[1]))
	int ILibRemoteLogging_IsModuleSet(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module);
	void ILibRemoteLogging_Forward(ILibRemoteLogging loggingModule, char* data, int dataLen);
#else
	#define ILibRemoteLogging_ConvertAddress(...) ;
	#define ILibRemoteLogging_ConvertToHex(...) ;
	#define ILibRemoteLogging_printf(...) ;
	#define ILibRemoteLogging_Create(...) NULL;
	#define ILibRemoteLogging_Destroy(...) ;
	#define ILibRemoteLogging_DeleteUserContext(...) ;
	#define ILibRemoteLogging_RegisterCommandSink(...) ;
	#define ILibRemoteLogging_Dispatch(...) ;
	#define ILibRemoteLogging_ReadModuleType(data) ILibRemoteLogging_Modules_UNKNOWN
	#define ILibRemoteLogging_ReadFlags(data) ILibRemoteLogging_Flags_NONE
	#define ILibRemoteLogging_IsModuleSet(...) 0
	#define ILibRemoteLogging_Forward(...) ;
#endif



#endif

