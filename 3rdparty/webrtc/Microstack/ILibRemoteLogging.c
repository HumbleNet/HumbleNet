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

#ifdef _REMOTELOGGING

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(_MINCORE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "ILibRemoteLogging.h"
#include "ILibParsers.h"
#include "ILibWebServer.h"
#include "ILibCrypto.h"
#include <stdarg.h>

char ILibScratchPad_RemoteLogging[255];

typedef struct ILibRemoteLogging_Session
{
	unsigned int Flags;
	void* UserContext;
}ILibRemoteLogging_Session;

typedef struct ILibRemoteLogging_Module
{
	sem_t LogSyncLock;
	unsigned int LogFlags;

	ILibRemoteLogging_OnWrite OutputSink;
	ILibRemoteLogging_OnRawForward RawForwardSink;
	int RawForwardOffset;
	ILibRemoteLogging_OnCommand CommandSink[15];
	ILibRemoteLogging_Session Sessions[5];
}ILibRemoteLogging_Module;

void ILibRemoteLogging_Destroy(ILibRemoteLogging module)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)module;
	if (obj != NULL)
	{
		sem_destroy(&(obj->LogSyncLock));
		free(module);
	}
}

void ILibRemoteLogging_CompactSessions(ILibRemoteLogging_Session sessions[], int sessionsLength)
{
	int x=0,y=0;
	for(x=0;x<sessionsLength;++x)
	{
		if(sessions[x].UserContext == NULL)
		{
			for(y=x+1;y<sessionsLength;++y)
			{
				if(sessions[y].UserContext != NULL)
				{
					memcpy_s(&sessions[x], sizeof(ILibRemoteLogging_Session), &sessions[y], sizeof(ILibRemoteLogging_Session));
					memset(&sessions[y], 0, sizeof(ILibRemoteLogging_Session));
					break;
				}
			}
		}
	}
}
void ILibRemoteLogging_RemoveUserContext(ILibRemoteLogging_Session sessions[], int sessionsLength, void *userContext)
{
	int i;
	for(i=0;i<sessionsLength;++i)
	{
		if(sessions[i].UserContext == NULL){break;}
		if(sessions[i].UserContext == userContext)
		{
			memset(&sessions[i], 0, sizeof(ILibRemoteLogging_Session));
			ILibRemoteLogging_CompactSessions(sessions, sessionsLength);
			break;
		}
	}
}
ILibRemoteLogging_Session* ILibRemoteLogging_GetSession(ILibRemoteLogging_Session sessions[], int sessionsLength, void *userContext)
{
	int i;
	ILibRemoteLogging_Session *retVal = NULL;
	for(i=0;i<sessionsLength;++i)
	{
		if(sessions[i].UserContext == NULL)
		{
			sessions[i].Flags = 0x00;
			sessions[i].UserContext = userContext;
			retVal = &sessions[i];
			break;
		}
		else if(sessions[i].UserContext == userContext)
		{
			retVal = &sessions[i];
			break;
		}
	}
	return(retVal);
}

void ILibRemoteLogging_LoggerCommand_Default(ILibRemoteLogging sender, ILibRemoteLogging_Modules module, unsigned short flags, char* data, int dataLen, void *userContext)
{
	int len;
	char buf[72];

	((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
	((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);

	len = sprintf_s(buf + 4, sizeof(buf) - 4, "*** ILibRemoteLogging_FileTransport [No Logfile Set] ***");
	ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
}
void ILibRemoteLogging_SetRawForward(ILibRemoteLogging logger, int bufferOffset, ILibRemoteLogging_OnRawForward onRawForward)
{
	((ILibRemoteLogging_Module*)logger)->RawForwardSink = onRawForward;
	((ILibRemoteLogging_Module*)logger)->RawForwardOffset = bufferOffset;
}
ILibRemoteLogging ILibRemoteLogging_Create(ILibRemoteLogging_OnWrite onOutput)
{
	ILibRemoteLogging_Module *retVal;
	if((retVal = (ILibRemoteLogging_Module*)malloc(sizeof(ILibRemoteLogging_Module)))==NULL) {ILIBCRITICALEXIT(254);}
	memset(retVal, 0, sizeof(ILibRemoteLogging_Module));

	sem_init(&(retVal->LogSyncLock), 0, 1);
	retVal->OutputSink = onOutput;

	ILibRemoteLogging_RegisterCommandSink(retVal, ILibRemoteLogging_Modules_Logger, ILibRemoteLogging_LoggerCommand_Default);

	return(retVal);
}

void ILibRemoteLogging_RegisterCommandSink(ILibRemoteLogging logger, ILibRemoteLogging_Modules module, ILibRemoteLogging_OnCommand sink)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)logger;
	sem_wait(&(obj->LogSyncLock));
	obj->CommandSink[ILibWhichPowerOfTwo((int)module)] = sink;
	sem_post(&(obj->LogSyncLock));
}


void ILibRemoteLogging_DeleteUserContext(ILibRemoteLogging logger, void *userContext)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)logger;
	if (obj == NULL) { return; }

	sem_wait(&(obj->LogSyncLock));
	ILibRemoteLogging_RemoveUserContext(obj->Sessions, sizeof(obj->Sessions) / sizeof(ILibRemoteLogging_Session), userContext);
	sem_post(&(obj->LogSyncLock));
}
//! Converts a sockaddr to a friendly string, using static memory, for logging purposes
/*!
	\param addr sockaddr to convert
	\return friendly string representation (NULL Terminated)
*/
char* ILibRemoteLogging_ConvertAddress(struct sockaddr* addr)
{
	ILibInet_ntop2((struct sockaddr*)addr, ILibScratchPad_RemoteLogging, sizeof(ILibScratchPad_RemoteLogging));
	return(ILibScratchPad_RemoteLogging);
}
//! Converts binary data into a hex string, using static memory, for logging purposes
/*!
	\param inVal Binary data to convert
	\param inValLength Binary data length
	\return hex string representation of binary data (NULL Terminated)
*/
char* ILibRemoteLogging_ConvertToHex(char* inVal, int inValLength)
{
	util_tohex(inVal, inValLength<255?inValLength:254, ILibScratchPad_RemoteLogging);
	return(ILibScratchPad_RemoteLogging);
}

void ILibRemoteLogging_SendCommand(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module, ILibRemoteLogging_Flags flags, char *data, int dataLen, void *userContext)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)loggingModule;
	char dest[4096];

	if(obj->OutputSink != NULL && ((dataLen + 4) < 4096))
	{
		((unsigned short*)dest)[0] = htons(module);
		((unsigned short*)dest)[1] = htons(flags);
		if(dataLen > 0) {memcpy_s(dest+4, sizeof(dest) - 4, data, dataLen);}
		obj->OutputSink(loggingModule, dest, 4+dataLen, userContext);
	}
}
//! Logging method using printf notation
/*!
	\b NOTE: NO-OP if there is no connected viewers
	\param loggingModule ILibRemoteLogging Logging Module
	\param module ILibRemoteLogging_Modules Describing the source of the message
	\param flags ILibRemoteLogging_Flags Logging Flags
	\param format printf Format String
	\param ... Optional parameters
*/
void ILibRemoteLogging_printf(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module, ILibRemoteLogging_Flags flags, char* format, ...)
{
	int i;
	char dest[4096];
	int len;
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)loggingModule;

	va_list argptr;
	
	if (obj != NULL && obj->RawForwardSink != NULL)
	{
		// When Forwarding, TimeStamp will be added later
		len = obj->RawForwardOffset;
	}
	else
	{
		// Add TimeStamp to Log
		len = ILibGetLocalTime(dest + 4, (int)sizeof(dest) - 4);
		dest[len + 4] = ':';
		dest[len + 5] = ' ';
		len += 2;

		len += 4; // Space for header (which isn't needed when forwarding)
	}

    va_start(argptr, format);
    len += vsnprintf(dest+len, 4096 - len, format, argptr);
    va_end(argptr);
    
	if (obj != NULL && obj->RawForwardSink != NULL)
	{
		obj->RawForwardSink(obj, module, flags, dest, len);
	}
	else if(obj != NULL && obj->OutputSink != NULL)
	{
		((unsigned short*)dest)[0] = htons((unsigned short)module | (unsigned short)0x8000);
		((unsigned short*)dest)[1] = htons(flags);

		if ((module & ILibRemoteLogging_Modules_ConsolePrint) == ILibRemoteLogging_Modules_ConsolePrint) { printf("%s\n", dest + 4); }

		sem_wait(&(obj->LogSyncLock));
		for(i=0;i<(sizeof(obj->Sessions)/sizeof(ILibRemoteLogging_Session)); ++i)
		{
			if(obj->Sessions[i].UserContext == NULL) {break;}								// No more Sessions
			if((obj->Sessions[i].Flags & (unsigned int)module) == 0) {continue;}			// Logging for this module is not enabled
			if(((obj->Sessions[i].Flags >> 16) & 0x3E) < (unsigned short)flags) {continue;} // Verbosity is not high enough
			sem_post(&(obj->LogSyncLock));
			obj->OutputSink(obj, dest, len, obj->Sessions[i].UserContext);
			sem_wait(&(obj->LogSyncLock));
		}
		sem_post(&(obj->LogSyncLock));
	}	
}	
void ILibRemoteLogging_Dispatch_Update(ILibRemoteLogging_Module *obj, unsigned short module, char* updateString, void *userContext)
{
	char temp[255];
	int tempLen;

	tempLen = ILibGetLocalTime((char*)temp, (int)sizeof(temp));
	temp[tempLen] = ':';
	temp[tempLen + 1] = ' ';
	tempLen += 2;

	if((module & ILibRemoteLogging_Modules_WebRTC_STUN_ICE) == ILibRemoteLogging_Modules_WebRTC_STUN_ICE)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "WebRTC Module: STUN/ICE [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_WebRTC_STUN_ICE | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if((module & ILibRemoteLogging_Modules_WebRTC_DTLS) == ILibRemoteLogging_Modules_WebRTC_DTLS)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "WebRTC Module: DTLS [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_WebRTC_DTLS | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if((module & ILibRemoteLogging_Modules_WebRTC_SCTP) == ILibRemoteLogging_Modules_WebRTC_SCTP)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "WebRTC Module: SCTP [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_WebRTC_SCTP | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Agent_GuardPost) == ILibRemoteLogging_Modules_Agent_GuardPost)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Agent Module: GuardPost [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Agent_GuardPost | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Agent_P2P) == ILibRemoteLogging_Modules_Agent_P2P)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Agent Module: Peer2Peer [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Agent_P2P | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_AsyncSocket) == ILibRemoteLogging_Modules_Microstack_AsyncSocket)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Microstack Module: AsyncSocket [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_AsyncSocket | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_Web) == ILibRemoteLogging_Modules_Microstack_Web)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Microstack Module: WebServer/Client [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_Web | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_Generic) == ILibRemoteLogging_Modules_Microstack_Generic)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Microstack Module: Generic [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_Generic | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Agent_KVM) == ILibRemoteLogging_Modules_Agent_KVM)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Agent Module: KVM [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Agent_KVM | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_Pipe) == ILibRemoteLogging_Modules_Microstack_Pipe)
	{
		tempLen += sprintf_s(temp + tempLen, sizeof(temp) - tempLen, "Microstack Module: NamedPipe [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_Pipe | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
}
int ILibRemoteLogging_IsModuleSet(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)loggingModule;
	int i;
	int retVal = 0;
	sem_wait(&(obj->LogSyncLock));
	for (i = 0; i < sizeof(obj->Sessions) / sizeof(ILibRemoteLogging_Session); ++i)
	{
		if (obj->Sessions[i].UserContext == NULL) { break; }
		if ((obj->Sessions[i].Flags & (unsigned int)module) == (unsigned int)module) { retVal = 1; break; }
	}
	sem_post(&(obj->LogSyncLock));
	return(retVal);
}
void ILibRemoteLogging_Forward(ILibRemoteLogging loggingModule, char* data, int dataLen)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)loggingModule;
	int i;
	if (obj->OutputSink == NULL) { return; }

	sem_wait(&(obj->LogSyncLock));
	for (i = 0; i < sizeof(obj->Sessions) / sizeof(ILibRemoteLogging_Session); ++i)
	{
		if (obj->Sessions[i].UserContext == NULL) { sem_post(&(obj->LogSyncLock)); return; }
		sem_post(&(obj->LogSyncLock));
		obj->OutputSink(obj, data, dataLen, obj->Sessions[i].UserContext);
		sem_wait(&(obj->LogSyncLock));
	}
	sem_post(&(obj->LogSyncLock));
}
int ILibRemoteLogging_Dispatch(ILibRemoteLogging loggingModule, char* data, int dataLen, void *userContext)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)loggingModule;
	unsigned short module;
	unsigned short flags;

	ILibRemoteLogging_Session *session = NULL;

	if(dataLen == 0 || userContext == NULL)
	{
		return(1);
	}

	module = ILibRemoteLogging_ReadModuleType(data);
	flags = ILibRemoteLogging_ReadFlags(data);

	sem_wait(&(obj->LogSyncLock));
	session = ILibRemoteLogging_GetSession(obj->Sessions, sizeof(obj->Sessions) / sizeof(ILibRemoteLogging_Session), userContext);
	if(session == NULL) {sem_post(&(obj->LogSyncLock)); return(1);}		// Too many users

	if((flags & 0x3F) != 0x00)
	{
		// Change Verbosity of Logs
		if((flags & 0x01) == 0x01)
		{
			// Disable Modules
			session->Flags &= (0xFFFFFFFF ^ module);
			sem_post(&(obj->LogSyncLock));
			ILibRemoteLogging_Dispatch_Update(obj, module, "DISABLED", userContext);
			
		}
		else
		{
			// Enable Modules and Set Verbosity	
			session->Flags &= 0xFFC0FFFF;							// Reset Verbosity Flags
			session->Flags |= (flags << 16);						// Set Verbosity Flags
			session->Flags |= (unsigned int)module;					// Enable Modules
			sem_post(&(obj->LogSyncLock));
			ILibRemoteLogging_Dispatch_Update(obj, module, "ENABLED", userContext);
		}
	}
	else if(module != 0x00)
	{
		// Module Specific Commands
		ILibRemoteLogging_OnCommand command = obj->CommandSink[ILibWhichPowerOfTwo((int)module)];
		sem_post(&(obj->LogSyncLock));
		if(command!=NULL)
		{
			command(loggingModule, (ILibRemoteLogging_Modules)module, flags, data+4, dataLen-4, userContext);
		}
	}
	else
	{
		sem_post(&(obj->LogSyncLock));
	}
	return(0);
}


typedef struct ILibRemoteLogging_FileTransport
{
	ILibTransport transport;
	ILibLinkedList_FileBacked_Root *logFile;
	char* localFilePath;
	int enabled;
	ILibRemoteLogging_Module* parent;
}ILibRemoteLogging_FileTransport;

ILibTransport_DoneState ILibFileTransport_SendSink(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done)
{
	ILibRemoteLogging_FileTransport *ft = (ILibRemoteLogging_FileTransport*)transport;

	if (ft->logFile != NULL && ft->enabled != 0)
	{
		ILibLinkedList_FileBacked_AddTail(ft->logFile, buffer, bufferLength);
	}
	return(ILibTransport_DoneState_COMPLETE);
}
unsigned int ILibFileTransport_PendingBytes(void *transport)
{
	UNREFERENCED_PARAMETER(transport);
	return(0);
}
void ILibFileTransport_CloseSink(void* transport)
{
	ILibRemoteLogging_FileTransport *ft = (ILibRemoteLogging_FileTransport*)transport;

	ILibRemoteLogging_DeleteUserContext(ft->parent, transport);
	if (ft->logFile != NULL) { ILibLinkedList_FileBacked_Close(ft->logFile); }
	if (ft->localFilePath != NULL) { free(ft->localFilePath); }
	free(transport);
}
int ILibRemoteLogging_FileTransport_ReadLogs(ILibRemoteLogging_FileTransport* ft, void *userContext)
{
	ILibLinkedList_FileBacked_Node *node = NULL;

	if (ILibLinkedList_FileBacked_IsEmpty(ft->logFile) != 0)
	{
		// No log entries
		return(1);
	}

	while ((node = ILibLinkedList_FileBacked_ReadNext(ft->logFile, node)) != NULL)
	{
		ILibTransport_Send(userContext, node->data, node->dataLen, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}

	return(0);
}

void ILibRemoteLogging_FileTransport_Report(void* userContext, ILibRemoteLogging_FileTransport *ft)
{
	int len;
	char buf[72];

	unsigned short newModules = 0;
	unsigned short flags = 0;

	((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
	((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);

	if (ft->logFile == NULL)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "*** ILibRemoteLogging_FileTransport [No Logfile Set] ***");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
		return;
	}

	ILibLinkedList_FileBacked_ReloadRoot(ft->logFile);
	newModules = (unsigned short)(ft->logFile->flags & 0xFFFF);
	flags = (unsigned short)((ft->logFile->flags >> 16) & 0x3F);

	len = sprintf_s(buf + 4, sizeof(buf) - 4, "*** Verbosity = %d ***", ILibWhichPowerOfTwo((int)flags));
	ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);

	if ((newModules & ILibRemoteLogging_Modules_WebRTC_STUN_ICE) == ILibRemoteLogging_Modules_WebRTC_STUN_ICE)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...WebRTC [STUN/ICE]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_WebRTC_DTLS) == ILibRemoteLogging_Modules_WebRTC_DTLS)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...WebRTC [DTLS]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_WebRTC_SCTP) == ILibRemoteLogging_Modules_WebRTC_SCTP)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...WebRTC [SCTP]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Agent_GuardPost) == ILibRemoteLogging_Modules_Agent_GuardPost)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...MeshAgent [GUARDPOST]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Agent_KVM) == ILibRemoteLogging_Modules_Agent_KVM)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...MeshAgent [KVM]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Agent_P2P) == ILibRemoteLogging_Modules_Agent_P2P)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...MeshAgent [P2P]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Microstack_AsyncSocket) == ILibRemoteLogging_Modules_Microstack_AsyncSocket)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...Microstack [AsyncSocket]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Microstack_Web) == ILibRemoteLogging_Modules_Microstack_Web)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...Microstack [WebServer]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Microstack_Pipe) == ILibRemoteLogging_Modules_Microstack_Pipe)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...Microstack [Pipe]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
	if ((newModules & ILibRemoteLogging_Modules_Microstack_Generic) == ILibRemoteLogging_Modules_Microstack_Generic)
	{
		len = sprintf_s(buf + 4, sizeof(buf) - 4, "...Microstack [Generic]");
		ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
	}
}

void ILibRemoteLogging_FileTransport_CommandSink(ILibRemoteLogging sender, ILibRemoteLogging_Modules module, unsigned short flags, char* data, int dataLen, void *userContext)
{
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)sender;
	int i, len;
	char buf[72];

	sem_wait(&(obj->LogSyncLock));
	for (i = 0; i < (sizeof(obj->Sessions) / sizeof(ILibRemoteLogging_Session)); ++i)
	{
		if (obj->Sessions[i].UserContext == NULL) { break; } // End of Sessions
		if (((ILibTransport*)obj->Sessions[i].UserContext)->IdentifierFlags == ILibTransports_RemoteLogging_FileTransport)
		{
			ILibRemoteLogging_FileTransport *ft = (ILibRemoteLogging_FileTransport*)obj->Sessions[i].UserContext;
			// This command applies to this type of ILibTransport
			

			if ((flags & ILibRemoteLogging_Command_Logger_Flags_ENABLE) == ILibRemoteLogging_Command_Logger_Flags_ENABLE)
			{
				switch (data[0])
				{
					case 0:
					case 1:
						// Enable/Disable
						ILibLinkedList_FileBacked_ReloadRoot(ft->logFile);
						ft->logFile->flags &= (unsigned int)0x7FFFFFFF;
						ft->enabled = data[0];
						if (ft->enabled != 0)
						{
							ft->logFile->flags |= (unsigned int)0x80000000;
						}
						ILibLinkedList_FileBacked_SaveRoot(ft->logFile);

						((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
						((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);
						len = sprintf_s(buf + 4, sizeof(buf) - 4, "ILibRemoteLogging_FileTransport [%s]", ft->enabled == 0 ? "DISABLED" : "ENABLED");
						ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
						if (ft->enabled != 0)
						{
							ILibRemoteLogging_FileTransport_Report(userContext, ft);
						}
						break;
					case 3:
						// Return the current state
						((unsigned short*)buf)[0] = htons(ILibRemoteLogging_Modules_Logger);
						((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Command_Logger_Flags_ENABLE);
						buf[4] = ft->enabled == 0 ? 0 : 1;
						ILibTransport_Send(userContext, buf, 5, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
						if (ft->enabled != 0)
						{
							ILibRemoteLogging_FileTransport_Report(userContext, ft);
						}
						break;
					default: 
						break;
				}
			}

			if ((flags & ILibRemoteLogging_Command_Logger_Flags_READ_FILE) == ILibRemoteLogging_Command_Logger_Flags_READ_FILE)
			{
				if (ILibRemoteLogging_FileTransport_ReadLogs(ft, userContext) != 0)
				{
					((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
					((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);
					len = sprintf_s(buf + 4, sizeof(buf) - 4, "ILibRemoteLogging_FileTransport: (No Log Entries)");
					ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
				}
			}
			if ((flags & ILibRemoteLogging_Command_Logger_Flags_RESET_FILE) == ILibRemoteLogging_Command_Logger_Flags_RESET_FILE)
			{
				// Clear the Log File
				if (ft->logFile != NULL)
				{
					ILibLinkedList_FileBacked_Reset(ft->logFile);

					((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
					((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);
					len = sprintf_s(buf + 4, sizeof(buf) - 4, "ILibRemoteLogging_FileTransport: (Logfile was reset)");
					ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
				}
				else
				{
					((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
					((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);
					len = sprintf_s(buf + 4, sizeof(buf) - 4, "ILibRemoteLogging_FileTransport: (No Log File)");
					ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
				}
			}
			if ((flags & ILibRemoteLogging_Command_Logger_Flags_RESET_FLAGS) == ILibRemoteLogging_Command_Logger_Flags_RESET_FLAGS)
			{
				if (dataLen == 4 && ft->logFile != NULL)
				{
					unsigned short newModules = ntohs(((unsigned short*)data)[0]);
					unsigned short newFlags = ntohs(((unsigned short*)data)[1]);
					obj->Sessions[i].Flags = (newFlags & 0x3F) << 16;
					obj->Sessions[i].Flags |= (unsigned int)newModules;

					ILibLinkedList_FileBacked_ReloadRoot(ft->logFile);
					ft->logFile->flags = (unsigned int)ft->enabled << 31;
					ft->logFile->flags |= (obj->Sessions[i].Flags & 0x7FFFFFFF);
					ILibLinkedList_FileBacked_SaveRoot(ft->logFile);

					((unsigned short*)buf)[0] = htons((unsigned short)ILibRemoteLogging_Modules_Logger | (unsigned short)0x8000);
					((unsigned short*)buf)[1] = htons(ILibRemoteLogging_Flags_VerbosityLevel_1);
					len = sprintf_s(buf + 4, sizeof(buf) - 4, "ILibRemoteLogging_FileTransport: (LogFile Flags RESET)");
					ILibTransport_Send(userContext, buf, len + 4, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);

					ILibRemoteLogging_FileTransport_Report(userContext, ft);
				}
			}
		}
	}
	sem_post(&(obj->LogSyncLock));
}
ILibTransport* ILibRemoteLogging_CreateFileTransport(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules modules, ILibRemoteLogging_Flags flags, char* path, int pathLen)
{
	ILibRemoteLogging_FileTransport *retVal = ILibMemory_Allocate(sizeof(ILibRemoteLogging_FileTransport), 0, NULL, NULL);
	char data[4];

	if (pathLen < 0) { pathLen = (int)strnlen_s(path, _MAX_PATH); }
	retVal->transport.IdentifierFlags = (int)ILibTransports_RemoteLogging_FileTransport;
	retVal->transport.SendPtr = ILibFileTransport_SendSink;
	retVal->transport.PendingBytesPtr = ILibFileTransport_PendingBytes;
	retVal->transport.ClosePtr = ILibFileTransport_CloseSink;
	retVal->parent = (ILibRemoteLogging_Module*)loggingModule;
	retVal->localFilePath = ILibString_Copy(path, pathLen);

	retVal->logFile = ILibLinkedList_FileBacked_Create(retVal->localFilePath, 2048000, 4096);
	if (retVal->logFile->flags != 0)
	{
		modules = (unsigned short)(retVal->logFile->flags & 0xFFFF);
		flags = (unsigned short)(retVal->logFile->flags >> 16);
		retVal->enabled = ((flags & 0x8000) == 0x8000) ? 1 : 0;
	}

	((unsigned short*)data)[0] = htons((unsigned short)modules);
	((unsigned short*)data)[1] = htons((unsigned short)flags & (unsigned short)0x3F);

	ILibRemoteLogging_Dispatch(loggingModule, data, (int)sizeof(data), retVal);
	ILibRemoteLogging_RegisterCommandSink(loggingModule, ILibRemoteLogging_Modules_Logger, ILibRemoteLogging_FileTransport_CommandSink);
	return((ILibTransport*)retVal);
}

#endif
