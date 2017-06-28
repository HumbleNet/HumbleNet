/*
Copyright 2014 - 2015 Intel Corporation

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
#include "ILibRemoteLogging.h"


#define ILibTransports_Raw_WebRTC 0x50

typedef enum
{
	ILibWebRTC_SDP_Flags_DTLS_SERVER = 0x2
} ILibWebRTC_SDP_Flags;

typedef enum
{
	ILibStun_Results_Unknown = 0,
	ILibStun_Results_No_NAT = 1,
	ILibStun_Results_Full_Cone_NAT = 2,
	ILibStun_Results_Symetric_NAT = 3,
	ILibStun_Results_Restricted_NAT = 4,
	ILibStun_Results_Port_Restricted_NAT = 5,
	ILibStun_Results_RFC5780_NOT_IMPLEMENTED = 10,
	ILibStun_Results_Public_Interface = 11,
} ILibStun_Results;

typedef enum
{
	ILibWebRTC_TURN_DISABLED = 0,	
	ILibWebRTC_TURN_ENABLED = 1,	// Always connect the control channel, but only use the server when necessary
	ILibWebRTC_TURN_ALWAYS_RELAY = 2	// Always relay all connections
} ILibWebRTC_TURN_ConnectFlags;

typedef enum
{
	ILibWebRTC_DataChannel_ReliabilityMode_RELIABLE = 0x00,
	ILibWebRTC_DataChannel_ReliabilityMode_RELIABLE_UNORDERED = 0x80,
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_REXMIT = 0x01,
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_REXMIT_UNORDERED = 0x81,
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_TIMED = 0x02,
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_TIMED_UNORDERED = 0x82,
}ILibWebRTC_DataChannel_ReliabilityModes;

#define ILibStun_SlotToChar(val) (val>26?(val+97):(val+65))
#define ILibStun_CharToSlot(val) (val>=97?(val-97):(val-65))

// STUN & ICE related methods
typedef void(*ILibStunClient_OnResult)(void* StunModule, ILibStun_Results Result, struct sockaddr_in* PublicIP, void *user);
void ILibStunClient_SetOptions(void* StunModule, SSL_CTX* securityContext, char* certThumbprintSha256);
void* ILibStunClient_Start(void *Chain, unsigned short LocalPort, ILibStunClient_OnResult OnResult);
void ILibStunClient_PerformNATBehaviorDiscovery(void* StunModule, struct sockaddr_in* StunServer, void *user);
void ILibStunClient_PerformStun(void* StunModule, struct sockaddr_in* StunServer, void *user);
void ILibStunClient_SendData(void* StunModule, struct sockaddr* target, char* data, int datalen, enum ILibAsyncSocket_MemoryOwnership UserFree);
int ILibStun_SetIceOffer(void* StunModule, char* iceOffer, int iceOfferLen, char** answer);
int ILibStun_SetIceOffer2(void *StunModule, char* iceOffer, int iceOfferLen, char *username, int usernameLength, char* password, int passwordLength, char** answer);
int ILibStun_GenerateIceOffer(void* StunModule, char** offer, char* userName, char* password);
unsigned int ILibStun_CRC32(char *buf, int len);
int ILib_Stun_GetAttributeChangeRequestPacket(int flags, char* TransactionId, char* rbuffer);
int ILibStun_ProcessStunPacket(void* obj, char* buffer, int bufferLength, struct sockaddr_in6 *remoteInterface);
void ILibStun_ClearIceState(void* stunModule, int iceSlot);


// WebRTC Related Methods
typedef enum
{
	ILibWebRTC_DataChannel_CloseStatus_OK = 0,
	ILibWebRTC_DataChannel_CloseStatus_ALREADY_PENDING = 1,
	ILibWebRTC_DataChannel_CloseStatus_ERROR = 254,
	ILibWebRTC_DataChannel_CloseStatus_NOT_SUPPORTED = 255,
}ILibWebRTC_DataChannel_CloseStatus;

int ILibWebRTC_IsDtlsInitiator(void* dtlsSession);
void ILibWebRTC_OpenDataChannel(void *WebRTCModule, unsigned short streamId, char* channelName, int channelNameLength);
void ILibWebRTC_CloseDataChannel_ALL(void *WebRTCModule);
ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannel(void *WebRTCModule, unsigned short streamId);
ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannelEx(void *WebRTCModule, unsigned short *streamIds, int streamIdsCount);

typedef int(*ILibWebRTC_OnDataChannel)(void *StunModule, void* WebRTCModule, unsigned short StreamId, char* ChannelName, int ChannelNameLength);
typedef void(*ILibWebRTC_OnDataChannelClosed)(void *StunModule, void* WebRTCModule, unsigned short StreamId);
typedef void(*ILibWebRTC_OnDataChannelAck)(void *StunModule, void* WebRTCModule, unsigned short StreamId);
typedef void(*ILibWebRTC_OnOfferUpdated)(void* stunModule, char* iceOffer, int iceOfferLen);
void ILibWebRTC_SetCallbacks(void *StunModule, ILibWebRTC_OnDataChannel OnDataChannel, ILibWebRTC_OnDataChannelClosed OnDataChannelClosed, ILibWebRTC_OnDataChannelAck OnDataChannelAck, ILibWebRTC_OnOfferUpdated OnOfferUpdated);
void ILibStun_DTLS_GetIceUserName(void* WebRTCModule, char* username);
void ILibWebRTC_SetTurnServer(void* stunModule, struct sockaddr_in6* turnServer, char* username, int usernameLength, char* password, int passwordLength, ILibWebRTC_TURN_ConnectFlags turnFlags);
void ILibWebRTC_DisableConsentFreshness(void *stunModule);

void ILibWebRTC_SetUserObject(void *stunModule, char* localUsername, void *userObject);
void* ILibWebRTC_GetUserObject(void *stunModule, char* localUsername);
void* ILibWebRTC_GetUserObjectFromDtlsSession(void *webRTCSession);

#ifdef _WEBRTCDEBUG
// SCTP Instrumentation Methods/Helpers
typedef void(*ILibSCTP_OnSenderReceiverCreditsChanged)(void *StunModule, int dtlsSessionId, int senderCredits, int receiverCredits);
typedef void(*ILibSCTP_OnTSNChanged)(void *stunModule, int OutboundTSN, int CumulativeTSNAck);
typedef void (*ILibSCTP_OnSCTPDebug)(void* dtlsSession, char* debugField, int data);
void ILibSCTP_SetSenderReceiverCreditsCallback(void* stunModule, ILibSCTP_OnSenderReceiverCreditsChanged callback);
void ILibSCTP_SetSimulatedInboundLossPercentage(void *stunModule, int lossPercentage);
void ILibSCTP_SetTSNCallback(void *dtlsSession, ILibSCTP_OnTSNChanged tsnHandler);
int ILibSCTP_Debug_SetDebugCallback(void *dtlsSession, char* debugFieldName, ILibSCTP_OnSCTPDebug handler);
#endif

// SCTP related methods
typedef void(*ILibSCTP_OnConnect)(void *StunModule, void* module, int connected);
typedef void(*ILibSCTP_OnData)(void* StunModule, void* module, unsigned short streamId, int pid, char* buffer, int bufferLen, void** user);
typedef void(*ILibSCTP_OnSendOK)(void* StunModule, void* module, void* user);

int ILibSCTP_DoesPeerSupportFeature(void* module, int feature);
void ILibSCTP_Pause(void* module);
void ILibSCTP_Resume(void* module);

void ILibSCTP_SetCallbacks(void* StunModule, ILibSCTP_OnConnect onconnect, ILibSCTP_OnData ondata, ILibSCTP_OnSendOK onsendok);
ILibTransport_DoneState ILibSCTP_Send(void* module, unsigned short streamId, char* data, int datalen);
ILibTransport_DoneState ILibSCTP_SendEx(void* module, unsigned short streamId, char* data, int datalen, int dataType);
int ILibSCTP_GetPendingBytesToSend(void* module);
void ILibSCTP_Close(void* module);

void ILibSCTP_SetUser(void* module, void* user);
void* ILibSCTP_GetUser(void* module);
void ILibSCTP_SetUser2(void* module, void* user);
void* ILibSCTP_GetUser2(void* module);
void ILibSCTP_SetUser3(void* module, int user);
int ILibSCTP_GetUser3(void* module);
void ILibSCTP_SetUser4(void* module, int user);
int ILibSCTP_GetUser4(void* module);

// Debug
void ILib_Stun_SendAttributeChangeRequest(void* StunModule, struct sockaddr* StunServer, int flags);
// Note: In "ILibStunClient_Start" you can pass NULL in the LocalInterface for INADDR_ANY.

// TURN
typedef void* ILibTURN_ClientModule;
typedef void(*ILibTURN_OnConnectTurnHandler)(ILibTURN_ClientModule turnModule, int success);
typedef void(*ILibTURN_OnAllocateHandler)(ILibTURN_ClientModule turnModule, int Lifetime, struct sockaddr_in6* RelayedTransportAddress);
typedef void(*ILibTURN_OnCreatePermissionHandler)(ILibTURN_ClientModule turnModule, int success, void *user);
typedef void(*ILibTURN_OnDataIndicationHandler)(ILibTURN_ClientModule turnModule, struct sockaddr_in6* remotePeer, char* buffer, int offset, int length);
typedef void(*ILibTURN_OnCreateChannelBindingHandler)(ILibTURN_ClientModule turnModule, unsigned short channelNumber, int success, void* user);
typedef void(*ILibTURN_OnChannelDataHandler)(ILibTURN_ClientModule turnModule, unsigned short channelNumber, char* buffer, int offset, int length);
typedef enum 
{
	ILibTURN_TransportTypes_UDP = 17,
	ILibTURN_TransportTypes_TCP = 6 
}ILibTURN_TransportTypes;

ILibTURN_ClientModule ILibTURN_CreateTurnClient(void* chain, ILibTURN_OnConnectTurnHandler OnConnectTurn, ILibTURN_OnAllocateHandler OnAllocate, ILibTURN_OnDataIndicationHandler OnData, ILibTURN_OnChannelDataHandler OnChannelData);
void ILibTURN_SetTag(ILibTURN_ClientModule clientModule, void *tag);
void* ILibTURN_GetTag(ILibTURN_ClientModule clientModule);
void ILibTURN_ConnectTurnServer(ILibTURN_ClientModule turnModule, struct sockaddr_in* turnServer, char* username, int usernameLen, char* password, int passwordLen, struct sockaddr_in* proxyServer);
void ILibTURN_Allocate(ILibTURN_ClientModule turnModule, ILibTURN_TransportTypes transportType);
int ILibTURN_CreateXORMappedAddress(struct sockaddr_in6 *endpoint, char* buffer, char* transactionID);
void ILibTURN_GetXORMappedAddress(char* buffer, int bufferLen, char* transactionID, struct sockaddr_in6 *value);
void ILibTURN_CreatePermission(ILibTURN_ClientModule turnModule, struct sockaddr_in6* permissions, int permissionsLength, ILibTURN_OnCreatePermissionHandler result, void* user);
enum ILibAsyncSocket_SendStatus ILibTURN_SendIndication(ILibTURN_ClientModule turnModule, struct sockaddr_in6* remotePeer, char* buffer, int offset, int length);
void ILibTURN_CreateChannelBinding(ILibTURN_ClientModule turnModule, unsigned short channelNumber, struct sockaddr_in6* remotePeer, ILibTURN_OnCreateChannelBindingHandler result, void* user);
enum ILibAsyncSocket_SendStatus ILibTURN_SendChannelData(ILibTURN_ClientModule turnModule, unsigned short channelNumber, char* buffer, int offset, int length);
int ILibTURN_IsConnectedToServer(ILibTURN_ClientModule clientModule);
unsigned int ILibTURN_GetPendingBytesToSend(ILibTURN_ClientModule turnModule);

