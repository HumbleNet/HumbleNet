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

#if defined(WINSOCK2)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "SimpleRendezvousServer.h"
#include "../../microstack/ILibParsers.h"
#include "../../microstack/ILibWebServer.h"
#include "../../microstack/ILibAsyncSocket.h"

#if defined(WIN32) && !defined(_WIN32_WCE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

typedef struct
{
	void (*PreSelect)(void* object,void* readset, void *writeset, void *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, void *readset, void *writeset, void *errorset);
	void (*Destroy)(void* object);

	ILibWebServer_ServerToken serverToken;
	SimpleRendezvousServer_OnGET OnGetHandler;
	SimpleRendezvousServer_OnPOST OnPostHandler;
}SimpleRendezvousServerStruct;

#ifdef MICROSTACK_TLS_DETECT
int SimpleRendezvousServer_IsTLS(SimpleRendezvousServerToken token)
{
	return(ILibWebServer_IsUsingTLS(((struct ILibWebServer_Session*)token)));
}
#endif
void OnSessionReceive(struct ILibWebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, ILibWebServer_DoneFlag done)
{
	SimpleRendezvousServerStruct *server = (SimpleRendezvousServerStruct*)sender->User;
	struct sockaddr_in6 local;
	char temp[255];
	char* receiver;
	struct parser_result *pr;

	if(done!=0)
	{
		if(header->DirectiveLength == 3 && strncasecmp(header->Directive, "GET", 3)==0 && server->OnGetHandler!=NULL)
		{
			// GET Reqeust
			ILibWebServer_GetLocalInterface(sender, (struct sockaddr*)&local);
			ILibInet_ntop2((struct sockaddr*)&local, temp, 255);

			pr = ILibParseString(temp, 0, strlen(temp), "::ffff:", 7); 
			receiver = pr->LastResult->data;

			header->DirectiveObj[header->DirectiveObjLength] = 0;
			server->OnGetHandler(server, sender, header->DirectiveObj, receiver);

			ILibDestructParserResults(pr);
		}
		else if(header->DirectiveLength == 4 && strncasecmp(header->Directive, "POST", 4)==0 && server->OnPostHandler!=NULL)
		{
			header->DirectiveObj[header->DirectiveObjLength] = 0;
			server->OnPostHandler(server, sender, header->DirectiveObj, bodyBuffer, endPointer);
		}
		else
		{
			ILibWebServer_StreamHeader_Raw(sender, 500, "Error", NULL, ILibAsyncSocket_MemoryOwnership_STATIC);
			ILibWebServer_StreamBody(sender, NULL, 0, ILibAsyncSocket_MemoryOwnership_STATIC, ILibWebServer_DoneFlag_Done);
		}
	}
}

void OnSession(struct ILibWebServer_Session *SessionToken, void *User)
{
	SessionToken->OnReceive = &OnSessionReceive;
	SessionToken->User = User;
}

SimpleRendezvousServer SimpleRendezvousServer_Create(void* chain, int localPort, SimpleRendezvousServer_OnGET GetHandler, SimpleRendezvousServer_OnPOST PostHandler)
{
	SimpleRendezvousServerStruct *retVal = (SimpleRendezvousServerStruct*)malloc(sizeof(SimpleRendezvousServerStruct));
	if(retVal==NULL){ILIBCRITICALEXIT(254);}
	memset(retVal, 0, sizeof(SimpleRendezvousServerStruct));

	retVal->serverToken = ILibWebServer_Create(chain, 3, localPort, OnSession, retVal);
	retVal->OnGetHandler = GetHandler;
	retVal->OnPostHandler = PostHandler;

	ILibAddToChain(chain, retVal);

	return(retVal);
}

#ifndef MICROSTACK_NOTLS
void SimpleRendezvousServer_SetSSL(SimpleRendezvousServer server, SSL_CTX* ctx)
{
	SimpleRendezvousServerStruct *module = (SimpleRendezvousServerStruct*)server;
#ifdef MICROSTACK_TLS_DETECT
	ILibWebServer_SetTLS(module->serverToken, ctx, 1);
#else
	ILibWebServer_SetTLS(module->serverToken, ctx);
#endif
}
#endif

void SimpleRendezvousServer_Respond(SimpleRendezvousServer server, SimpleRendezvousServerToken token, int success, char* htmlBody, int htmlBodyLength, enum ILibAsyncSocket_MemoryOwnership htmlBodyFlag)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;

	ILibWebServer_StreamHeader_Raw(session, success!=0?200:400, success!=0?"OK":"ERROR", success==1?"\r\nContent-Type: text/html":"\r\nContent-Type: text/plain", ILibAsyncSocket_MemoryOwnership_STATIC);
	ILibWebServer_StreamBody(session, htmlBody, htmlBodyLength, htmlBodyFlag, ILibWebServer_DoneFlag_Done);
}

void SimpleRendezvousServer_OnWebSocketReceive(struct ILibWebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, ILibWebServer_DoneFlag done)
{
	if(sender->User3!=NULL)
	{
		((SimpleRendezvousServer_OnWebSocket)sender->User3)(sender, InterruptFlag, header, bodyBuffer + *beginPointer, endPointer, (SimpleRendezvousServer_WebSocket_DataTypes)ILibWebServer_WebSocket_GetDataType(sender), (SimpleRendezvousServer_DoneFlag)done);
	}
}

void SimpleRendezvousServer_UpgradeWebSocket(SimpleRendezvousServerToken token, int autoReassembleMaxBufferSize, SimpleRendezvousServer_OnWebSocket Handler, SimpleRendezvousServer_OnWebSocketClose CloseHandler)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	session->User3 = (ILibWebServer_Session_OnReceive) Handler;
	session->OnReceive = &SimpleRendezvousServer_OnWebSocketReceive;
	session->OnDisconnect = (ILibWebServer_Session_OnDisconnect)CloseHandler;
	ILibWebServer_UpgradeWebSocket(session, autoReassembleMaxBufferSize);
}
enum ILibAsyncSocket_SendStatus SimpleRendezvousServer_WebSocket_Send(SimpleRendezvousServerToken token, SimpleRendezvousServer_WebSocket_DataTypes bufferType, char* buffer, int bufferLen, enum ILibAsyncSocket_MemoryOwnership userFree, SimpleRendezvousServer_FragmentFlags fragmentFlag)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	return((enum ILibAsyncSocket_SendStatus)ILibWebServer_WebSocket_Send(session, buffer, bufferLen, (ILibWebServer_WebSocket_DataTypes)bufferType, userFree, (ILibWebServer_WebSocket_FragmentFlags)fragmentFlag));
}
void SimpleRendezvousServer_WebSocket_Close(SimpleRendezvousServerToken token)
{
	ILibWebServer_WebSocket_Close((struct ILibWebServer_Session*)token);
}
int SimpleRendezvousServer_IsAuthenticated(SimpleRendezvousServerToken token, char* realm, int realmLen)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	int retVal = ILibWebServer_Digest_IsAuthenticated(session, realm, realmLen);
	if(retVal == 0)
	{
		ILibWebServer_Digest_SendUnauthorized(session, realm, realmLen, NULL, 0);
	}
	return(retVal);
}
char* SimpleRendezvousServer_GetUsername(SimpleRendezvousServerToken token)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	return(ILibWebServer_Digest_GetUsername(session));
}
int SimpleRendezvousServer_ValidatePassword(SimpleRendezvousServerToken token, char* password, int passwordLen)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	int retVal = ILibWebServer_Digest_ValidatePassword(session, password, passwordLen);
	if(retVal == 0)
	{
		ILibWebServer_Digest_SendUnauthorized(session, NULL, 0, NULL, 0);
	}
	return(retVal);
}
int SimpleRendezvousServer_WebSocket_IsRequest(SimpleRendezvousServerToken token)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	return(ILibWebServer_WebSocket_GetDataType(session) == ILibWebServer_WebSocket_DataType_REQUEST ? 1 : 0);
}
int SimpleRendezvousServer_WebSocket_IsWebSocket(SimpleRendezvousServerToken token)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)token;
	return(ILibWebServer_WebSocket_GetDataType(session) != ILibWebServer_WebSocket_DataType_UNKNOWN ? 1 : 0);
}