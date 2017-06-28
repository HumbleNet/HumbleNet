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

#ifdef _REMOTELOGGING

#include "ILibParsers.h"
#include "ILibWebServer.h"
#include "ILibRemoteLogging.h"
#ifndef MICROSTACK_NOTLS
#include "../core/utils.h"
#endif

char ILibScratchPad_RemoteLogging[255];

#if defined(WIN32) && !defined(snprintf) && (_MSC_PLATFORM_TOOLSET <= 120)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif

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
					memcpy(&sessions[x], &sessions[y], sizeof(ILibRemoteLogging_Session));
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

ILibRemoteLogging ILibRemoteLogging_Create(ILibRemoteLogging_OnWrite onOutput)
{
	ILibRemoteLogging_Module *retVal;
	if((retVal = (ILibRemoteLogging_Module*)malloc(sizeof(ILibRemoteLogging_Module)))==NULL) {ILIBCRITICALEXIT(254);}
	memset(retVal, 0, sizeof(ILibRemoteLogging_Module));

	sem_init(&(retVal->LogSyncLock), 0, 1);
	retVal->OutputSink = onOutput;
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
	sem_wait(&(obj->LogSyncLock));
	ILibRemoteLogging_RemoveUserContext(obj->Sessions, sizeof(obj->Sessions) / sizeof(ILibRemoteLogging_Session), userContext);
	sem_post(&(obj->LogSyncLock));
}

char* ILibRemoteLogging_ConvertAddress(struct sockaddr* addr)
{
	ILibInet_ntop2((struct sockaddr*)addr, ILibScratchPad_RemoteLogging, sizeof(ILibScratchPad_RemoteLogging));
	return(ILibScratchPad_RemoteLogging);
}
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
		if(dataLen > 0) {memcpy(dest+4, data, dataLen);}
		obj->OutputSink(loggingModule, dest, 4+dataLen, userContext);
	}
}

void ILibRemoteLogging_printf(ILibRemoteLogging loggingModule, ILibRemoteLogging_Modules module, ILibRemoteLogging_Flags flags, char* format, ...)
{
	int i;
	char dest[4096];
	int len;
	ILibRemoteLogging_Module *obj = (ILibRemoteLogging_Module*)loggingModule;

	va_list argptr;
    va_start(argptr, format);
    len = vsnprintf(dest+4, 4092, format, argptr);
    va_end(argptr);
    
	if(obj != NULL && obj->OutputSink != NULL)
	{
		((unsigned short*)dest)[0] = htons(module | 0x8000);
		((unsigned short*)dest)[1] = htons(flags);

		sem_wait(&(obj->LogSyncLock));
		for(i=0;i<(sizeof(obj->Sessions)/sizeof(ILibRemoteLogging_Session)); ++i)
		{
			if(obj->Sessions[i].UserContext == NULL) {break;}								// No more Sessions
			if((obj->Sessions[i].Flags & (unsigned int)module) == 0) {continue;}			// Logging for this module is not enabled
			if(((obj->Sessions[i].Flags >> 16) & 0x3E) < (unsigned short)flags) {continue;} // Verbosity is not high enough
			sem_post(&(obj->LogSyncLock));
			obj->OutputSink(obj, dest, len+4, obj->Sessions[i].UserContext);
			sem_wait(&(obj->LogSyncLock));
		}
		sem_post(&(obj->LogSyncLock));
	}	
}	
void ILibRemoteLogging_Dispatch_Update(ILibRemoteLogging_Module *obj, unsigned short module, char* updateString, void *userContext)
{
	char temp[255];
	int tempLen;

	if((module & ILibRemoteLogging_Modules_WebRTC_STUN_ICE) == ILibRemoteLogging_Modules_WebRTC_STUN_ICE)
	{
		tempLen = snprintf(temp, sizeof(temp), "WebRTC Module: STUN/ICE [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_WebRTC_STUN_ICE | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if((module & ILibRemoteLogging_Modules_WebRTC_DTLS) == ILibRemoteLogging_Modules_WebRTC_DTLS)
	{
		tempLen = snprintf(temp, sizeof(temp), "WebRTC Module: DTLS [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_WebRTC_DTLS | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if((module & ILibRemoteLogging_Modules_WebRTC_SCTP) == ILibRemoteLogging_Modules_WebRTC_SCTP)
	{
		tempLen = snprintf(temp, sizeof(temp), "WebRTC Module: SCTP [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_WebRTC_SCTP | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Agent_GuardPost) == ILibRemoteLogging_Modules_Agent_GuardPost)
	{
		tempLen = snprintf(temp, sizeof(temp), "Agent Module: GuardPost [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Agent_GuardPost | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Agent_P2P) == ILibRemoteLogging_Modules_Agent_P2P)
	{
		tempLen = snprintf(temp, sizeof(temp), "Agent Module: Peer2Peer [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Agent_P2P | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_AsyncSocket) == ILibRemoteLogging_Modules_Microstack_AsyncSocket)
	{
		tempLen = snprintf(temp, sizeof(temp), "Microstack Module: AsyncSocket [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_AsyncSocket | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_Web) == ILibRemoteLogging_Modules_Microstack_Web)
	{
		tempLen = snprintf(temp, sizeof(temp), "Microstack Module: WebServer/Client [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_Web | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_Generic) == ILibRemoteLogging_Modules_Microstack_Generic)
	{
		tempLen = snprintf(temp, sizeof(temp), "Microstack Module: Generic [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_Generic | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Agent_KVM) == ILibRemoteLogging_Modules_Agent_KVM)
	{
		tempLen = snprintf(temp, sizeof(temp), "Agent Module: KVM [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Agent_KVM | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
	}
	if ((module & ILibRemoteLogging_Modules_Microstack_NamedPipe) == ILibRemoteLogging_Modules_Microstack_NamedPipe)
	{
		tempLen = snprintf(temp, sizeof(temp), "Microstack Module: NamedPipe [%s]", updateString);
		ILibRemoteLogging_SendCommand(obj, (ILibRemoteLogging_Modules)(ILibRemoteLogging_Modules_Microstack_NamedPipe | 0x8000), ILibRemoteLogging_Flags_VerbosityLevel_1, temp, tempLen, userContext);
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
		if ((obj->Sessions[i].Flags & module) == module) { retVal = 1; break; }
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
	else
	{
		// Module Specific Commands
		ILibRemoteLogging_OnCommand command = obj->CommandSink[ILibWhichPowerOfTwo((int)module)];
		sem_post(&(obj->LogSyncLock));
		if(command!=NULL)
		{
			command(loggingModule, (ILibRemoteLogging_Modules)module, flags, data, dataLen, userContext);
		}
	}
	return(0);
}
#endif