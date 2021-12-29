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

#ifndef __ILIBREMOTELOGGING__
#define __ILIBREMOTELOGGING__

#include "ILibParsers.h"

/*! \defgroup ILibRemoteLogging ILibRemoteLogging Module
@{
*/
//! Module Types
typedef enum ILibRemoteLogging_Modules
{
	ILibRemoteLogging_Modules_UNKNOWN			= 0x00,			//!< UNKNOWN Module
	ILibRemoteLogging_Modules_Logger			= 0x01,			//!< RESERVED: Logger
	ILibRemoteLogging_Modules_WebRTC_STUN_ICE	= 0x02,			//!< WebRTC: STUN/ICE
	ILibRemoteLogging_Modules_WebRTC_DTLS		= 0x04,			//!< WebRTC: DTLS
	ILibRemoteLogging_Modules_WebRTC_SCTP		= 0x08,			//!< WebRTC: SCTP
	ILibRemoteLogging_Modules_Agent_GuardPost	= 0x10,			//!< Mesh Agent: Guard Post
	ILibRemoteLogging_Modules_Agent_P2P			= 0x20,			//!< Mesh Agent: Peer to Peer
	ILibRemoteLogging_Modules_Agent_KVM			= 0x200,		//!< Mesh AGent: KVM
	ILibRemoteLogging_Modules_Microstack_AsyncSocket	= 0x40,	//!< Microstack: AsyncSocket, AsyncServerSocket, AsyncUDPSocket
	ILibRemoteLogging_Modules_Microstack_Web			= 0x80,	//!< Microstack: WebServer, WebSocket, WebClient
	ILibRemoteLogging_Modules_Microstack_Pipe			= 0x400,//!< Microstack: Pipe
	ILibRemoteLogging_Modules_Microstack_Generic		= 0x100,//!< Microstack: Generic
	ILibRemoteLogging_Modules_ConsolePrint				= 0x4000
}ILibRemoteLogging_Modules;
//! Logging Flags
typedef enum ILibRemoteLogging_Flags
{
	ILibRemoteLogging_Flags_NONE				= 0x00,	//!< NONE
	ILibRemoteLogging_Flags_DisableLogging		= 0x01,	//!< DISABLED
	ILibRemoteLogging_Flags_VerbosityLevel_1	= 0x02, //!< Verbosity Level 1
	ILibRemoteLogging_Flags_VerbosityLevel_2	= 0x04, //!< Verbosity Level 2
	ILibRemoteLogging_Flags_VerbosityLevel_3	= 0x08, //!< Verbosity Level 3
	ILibRemoteLogging_Flags_VerbosityLevel_4	= 0x10, //!< Verbosity Level 4
	ILibRemoteLogging_Flags_VerbosityLevel_5	= 0x20, //!< Verbosity Level 5
}ILibRemoteLogging_Flags;

typedef enum ILibRemoteLogging_Command_Logger_Flags
{
	ILibRemoteLogging_Command_Logger_Flags_ENABLE		= 0x100,	//!< Enables/Disables File Logging
	ILibRemoteLogging_Command_Logger_Flags_RESET_FILE	= 0x200,	//!< Erases the log file
	ILibRemoteLogging_Command_Logger_Flags_READ_FILE	= 0x400,	//!< Reads the log file
	ILibRemoteLogging_Command_Logger_Flags_RESET_FLAGS	= 0x800,	//!< Sets the Module/Flags to log to file
}ILibRemoteLogging_Command_Logger_Flags;

#define ILibTransports_RemoteLogging_FileTransport 0x70
typedef void* ILibRemoteLogging;
typedef void (*ILibRemoteLogging_OnWrite)(ILibRemoteLogging module, char* data, int dataLen, void *userContext);
typedef void (*ILibRemoteLogging_OnCommand)(ILibRemoteLogging sender, ILibRemoteLogging_Modules module, unsigned short flags, char* data, int dataLen, void *userContext);
typedef void(*ILibRemoteLogging_OnRawForward)(ILibRemoteLogging sender, ILibRemoteLogging_Modules module, ILibRemoteLogging_Flags flags, char *buffer, int bufferLen);

#ifdef _REMOTELOGGING
	char* ILibRemoteLogging_ConvertAddress(struct sockaddr* addr);
	char* ILibRemoteLogging_ConvertToHex(char* inVal, int inValLength);
	void ILibRemoteLogging_printf(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module, ILibRemoteLogging_Flags flags, char* format, ...);

	ILibRemoteLogging ILibRemoteLogging_Create(ILibRemoteLogging_OnWrite onOutput);
	ILibTransport* ILibRemoteLogging_CreateFileTransport(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules modules, ILibRemoteLogging_Flags flags, char* path, int pathLen);
	void ILibRemoteLogging_Destroy(ILibRemoteLogging logger);
	void ILibRemoteLogging_SetRawForward(ILibRemoteLogging logger, int bufferOffset, ILibRemoteLogging_OnRawForward onRawForward);

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
	#define ILibRemoteLogging_SetRawForward(...) ;
	#define ILibRemoteLogging_CreateFileTransport(...) NULL;
	#define ILibRemoteLogging_Destroy(...) ;
	#define ILibRemoteLogging_DeleteUserContext(...) ;
	#define ILibRemoteLogging_RegisterCommandSink(...) ;
	#define ILibRemoteLogging_Dispatch(...) ;
	#define ILibRemoteLogging_ReadModuleType(data) ILibRemoteLogging_Modules_UNKNOWN
	#define ILibRemoteLogging_ReadFlags(data) ILibRemoteLogging_Flags_NONE
	#define ILibRemoteLogging_IsModuleSet(...) 0
	#define ILibRemoteLogging_Forward(...) ;
#endif

/*! @} */

#endif

