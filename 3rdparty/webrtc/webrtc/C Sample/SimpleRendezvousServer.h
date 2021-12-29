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

#ifndef ___SimpleRendezvousServer___
#define ___SimpleRendezvousServer___

#include "../../microstack/ILibParsers.h"
#include "../../microstack/ILibAsyncSocket.h"

typedef enum 
{
	SimpleRendezvousServer_WebSocket_DataType_BINARY = 0x2,
	SimpleRendezvousServer_WebSocket_DataType_TEXT = 0x1
}SimpleRendezvousServer_WebSocket_DataTypes;

typedef enum
{
	SimpleRendezvousServer_DoneFlag_NotDone = 0,
	SimpleRendezvousServer_DoneFlag_Done = 1,
	SimpleRendezvousServer_DoneFlag_Partial = 10,
	SimpleRendezvousServer_DoneFlag_LastPartial = 11
}SimpleRendezvousServer_DoneFlag;

typedef enum
{
	SimpleRendezvousServer_FragmentFlag_Incomplete = 0,
	SimpleRendezvousServer_FragmentFlag_Complete = 1
}SimpleRendezvousServer_FragmentFlags;

typedef void* SimpleRendezvousServer;
typedef void* SimpleRendezvousServerToken;

typedef void(*SimpleRendezvousServer_OnGET)(SimpleRendezvousServer sender, SimpleRendezvousServerToken token, char* path, char* localReceiverAddress);
typedef void(*SimpleRendezvousServer_OnPOST)(SimpleRendezvousServer sender, SimpleRendezvousServerToken token, char* path, char* data, int dataLen);
typedef void (*SimpleRendezvousServer_OnWebSocket)(SimpleRendezvousServerToken sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int bodyBufferLen, SimpleRendezvousServer_WebSocket_DataTypes bodyBufferType, SimpleRendezvousServer_DoneFlag done);
typedef void (*SimpleRendezvousServer_OnWebSocketClose)(SimpleRendezvousServerToken sender);

#ifndef MICROSTACK_NOTLS
void SimpleRendezvousServer_SetSSL(SimpleRendezvousServer server, SSL_CTX* ctx);
#ifdef MICROSTACK_TLS_DETECT
int SimpleRendezvousServer_IsTLS(SimpleRendezvousServerToken token);
#endif
#endif

int SimpleRendezvousServer_IsAuthenticated(SimpleRendezvousServerToken token, char* realm, int realmLen);
char* SimpleRendezvousServer_GetUsername(SimpleRendezvousServerToken token);
int SimpleRendezvousServer_ValidatePassword(SimpleRendezvousServerToken token, char* password, int passwordLen);

SimpleRendezvousServer SimpleRendezvousServer_Create(void* chain, int localPort, SimpleRendezvousServer_OnGET GetHandler, SimpleRendezvousServer_OnPOST PostHandler);
void SimpleRendezvousServer_Respond(SimpleRendezvousServer server, SimpleRendezvousServerToken token, int success, char* htmlBody, int htmlBodyLength, enum ILibAsyncSocket_MemoryOwnership htmlBodyFlag);
void SimpleRendezvousServer_UpgradeWebSocket(SimpleRendezvousServerToken token, int autoReassembleMaxBufferSize, SimpleRendezvousServer_OnWebSocket Handler, SimpleRendezvousServer_OnWebSocketClose CloseHandler);
enum ILibAsyncSocket_SendStatus SimpleRendezvousServer_WebSocket_Send(SimpleRendezvousServerToken token, SimpleRendezvousServer_WebSocket_DataTypes bufferType, char* buffer, int bufferLen, enum ILibAsyncSocket_MemoryOwnership userFree, SimpleRendezvousServer_FragmentFlags fragmentFlag);
void SimpleRendezvousServer_WebSocket_Close(SimpleRendezvousServerToken token);

int SimpleRendezvousServer_WebSocket_IsRequest(SimpleRendezvousServerToken token);
int SimpleRendezvousServer_WebSocket_IsWebSocket(SimpleRendezvousServerToken token);
#endif