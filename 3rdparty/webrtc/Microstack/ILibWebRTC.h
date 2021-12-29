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

#if !defined(___ILIBWEBRTC___) && !defined(MICROSTACK_NOTLS)
#define ___ILIBWEBRTC___
#include "ILibAsyncSocket.h"
#include <openssl/ssl.h>

/*! \file ILibWrapperWebRTC.h
\brief Microstack API for WebRTC Functionality
*/

/*! \defgroup ILibWebRTC ILibWebRTC Module */
/*! @{ */

#define ILibTransports_Raw_WebRTC 0x50

typedef enum ILibWebRTC_SDP_Flags
{
	ILibWebRTC_SDP_Flags_DTLS_SERVER = 0x2
} ILibWebRTC_SDP_Flags;

typedef enum ILibStun_Results
{
	ILibStun_Results_Unknown = 0,					//!< Cannot determine NAT Type
	ILibStun_Results_No_NAT = 1,					//!< No NAT Detected
	ILibStun_Results_Full_Cone_NAT = 2,				//!< NAT Type: FULL Cone
	ILibStun_Results_Symetric_NAT = 3,				//!< NAT Type: Symmetric
	ILibStun_Results_Restricted_NAT = 4,			//!< NAT Type: Address Restricted
	ILibStun_Results_Port_Restricted_NAT = 5,		//!< NAT Type: Port Restricted
	ILibStun_Results_RFC5780_NOT_IMPLEMENTED = 10,	//!< Server does not support RFC5780
	ILibStun_Results_Public_Interface = 11,			//!< Local interface is publically accessible
} ILibStun_Results;

typedef enum ILibWebRTC_TURN_ConnectFlags
{
	ILibWebRTC_TURN_DISABLED = 0,		//!< No TURN support
	ILibWebRTC_TURN_ENABLED = 1,		//!< Always connect the control channel, but only use the server when necessary
	ILibWebRTC_TURN_ALWAYS_RELAY = 2	//!< Always relay all connections
} ILibWebRTC_TURN_ConnectFlags;

typedef enum ILibWebRTC_DataChannel_ReliabilityModes
{
	ILibWebRTC_DataChannel_ReliabilityMode_RELIABLE = 0x00,								//!< Reliable Transport [DEFAULT]
	ILibWebRTC_DataChannel_ReliabilityMode_RELIABLE_UNORDERED = 0x80,					//!< Unordered Reliable Transport
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_REXMIT = 0x01,				//!< Partial Reliable via a Max-Retransmit Attribute
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_REXMIT_UNORDERED = 0x81,	//!< Partial Reliable/Unordered via a Max-Retransmit Attribute
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_TIMED = 0x02,				//!< Partial Reliable via  Timeout for Re-transmits
	ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_TIMED_UNORDERED = 0x82,		//!< Parital Reliable/Unordered via Timeout for Re-transmits
}ILibWebRTC_DataChannel_ReliabilityModes;

#define ILibStun_SlotToChar(val) (val>26?(val+97):(val+65))
#define ILibStun_CharToSlot(val) (val>=97?(val-97):(val-65))

/*! \defgroup STUN_ICE STUN & ICE Related Methods */
/*! @{ */
typedef void(*ILibStunClient_OnResult)(void* StunModule, ILibStun_Results Result, struct sockaddr_in* PublicIP, void *user);
void ILibStunClient_SetOptions(void* StunModule, SSL_CTX* securityContext, char* certThumbprintSha256);
void* ILibStunClient_Start(void *Chain, unsigned short LocalPort, ILibStunClient_OnResult OnResult);
SSL_CTX* ILibStunClient_GetSSLCTX(void *StunModule);
void ILibStunClient_PerformNATBehaviorDiscovery(void* StunModule, struct sockaddr_in* StunServer, void *user);
void ILibStunClient_PerformStun(void* StunModule, struct sockaddr_in* StunServer, void *user);
void ILibStunClient_SendData(void* StunModule, struct sockaddr* target, char* data, int datalen, enum ILibAsyncSocket_MemoryOwnership UserFree);
int ILibStun_SetIceOffer(void* StunModule, char* iceOffer, int iceOfferLen, char** answer);
int ILibStun_SetIceOffer2(void *StunModule, char* iceOffer, int iceOfferLen, char *username, int usernameLength, char* password, int passwordLength, char** answer);
int ILibStun_GenerateIceOffer(void* StunModule, char** offer, char* userName, char* password);
void ILibStun_ClearIceState(void* stunModule, int iceSlot);
int ILibStun_GetExistingIceOfferIndex(void* stunModule, char *iceOffer, int iceOfferLen);
/*! @} */

/*! \defgroup ORTC ORTC Related Methods */
/*! @{ */
void ILibORTC_GetLocalParameters(void *stunModule, char* username, char** password, int *passwordLength, char** certHash, int* certHashLength);
void ILibORTC_SetRemoteParameters(void* stunModule, char *username, int usernameLen, char *password, int passwordLen, char *certHash, int certHashLen, char *localUserName);
void ILibORTC_AddRemoteCandidate(void *stunModule, char* localUsername, struct sockaddr_in6 *candidate);
/*! @} */


/*! \defgroup General General Methods */
/*! @{ */
typedef enum
{
	ILibWebRTC_DataChannel_CloseStatus_OK = 0,					//!< SCTP/RECONFIG Result: OK
	ILibWebRTC_DataChannel_CloseStatus_ALREADY_PENDING = 1,		//!< SCTP/RECONFIG Result: Already Pending
	ILibWebRTC_DataChannel_CloseStatus_ERROR = 254,				//!< SCTP/RECONFIG Result: ERROR
	ILibWebRTC_DataChannel_CloseStatus_NOT_SUPPORTED = 255,		//!< SCTP/RECONFIG Not supported by Peer
}ILibWebRTC_DataChannel_CloseStatus;

int ILibWebRTC_IsDtlsInitiator(void* dtlsSession);
void ILibWebRTC_OpenDataChannel(void *WebRTCModule, unsigned short streamId, char* channelName, int channelNameLength);
void ILibWebRTC_CloseDataChannel_ALL(void *WebRTCModule);
ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannel(void *WebRTCModule, unsigned short streamId);
ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannelEx(void *WebRTCModule, unsigned short *streamIds, int streamIdsCount);

//! Handler for new Data Channel Associations
/*!
	\param StunModule Local Stun/ICE Client
	\param WebRTCModule Peer Connection Session
	\param StreamId Data Channel ID
	\param ChannelName Name of the Data Channel
	\param ChannelNameLength Length of the Channel Name
	\return 0 = Respond with ACK, Non-Zero = DO NOT ACK
*/
typedef int(*ILibWebRTC_OnDataChannel)(void *StunModule, void* WebRTCModule, unsigned short StreamId, char* ChannelName, int ChannelNameLength);
//! Handler for when an Associated Data Channel is closed
/*!
	\param StunModule Local Stun/ICE Client
	\param WebRTCModule Peer Connection Session
	\param StreamId Data Channel Stream ID
*/
typedef void(*ILibWebRTC_OnDataChannelClosed)(void *StunModule, void* WebRTCModule, unsigned short StreamId);
//! Handler for when a Data Channel Creation request was ACK'ed by the peer
/*!
	\param StunModule Local Stun/ICE Client
	\param WebRTCModule Peer Connection Session
	\param StreamId Data Channel Stream ID
*/
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
/*! @} */

/*! \defgroup SCTP SCTP Related Methods */
/*! @{ */
typedef void(*ILibSCTP_OnConnect)(void *StunModule, void* SctpSession, int connected);
typedef void(*ILibSCTP_OnData)(void* StunModule, void* SctpSession, unsigned short streamId, int pid, char* buffer, int bufferLen, void** user);
typedef void(*ILibSCTP_OnSendOK)(void* StunModule, void* SctpSession, void* user);

int ILibSCTP_DoesPeerSupportFeature(void* SctpSession, int feature);
void ILibSCTP_Pause(void* SctpSession);
void ILibSCTP_Resume(void* SctpSession);

void ILibSCTP_SetCallbacks(void* StunModule, ILibSCTP_OnConnect onconnect, ILibSCTP_OnData ondata, ILibSCTP_OnSendOK onsendok);
ILibTransport_DoneState ILibSCTP_Send(void* SctpSession, unsigned short streamId, char* data, int datalen);
ILibTransport_DoneState ILibSCTP_SendEx(void* SctpSession, unsigned short streamId, char* data, int datalen, int dataType);
int ILibSCTP_GetPendingBytesToSend(void* SctpSession);
void ILibSCTP_Close(void* SctpSession);

void ILibSCTP_SetUser(void* SctpSession, void* user);
void* ILibSCTP_GetUser(void* SctpSession);
void ILibSCTP_SetUser2(void* SctpSession, void* user);
void* ILibSCTP_GetUser2(void* SctpSession);
void ILibSCTP_SetUser3(void* SctpSession, int user);
int ILibSCTP_GetUser3(void* SctpSession);
void ILibSCTP_SetUser4(void* SctpSession, int user);
int ILibSCTP_GetUser4(void* SctpSession);
/*! @} */


// Debug
void ILib_Stun_SendAttributeChangeRequest(void* StunModule, struct sockaddr* StunServer, int flags);
// Note: In "ILibStunClient_Start" you can pass NULL in the LocalInterface for INADDR_ANY.


/*! \defgroup TURN TURN Related Methods */
/*! @{ */
typedef void* ILibTURN_ClientModule;
ILibTURN_ClientModule ILibStun_GetTurnClient(void *stunModule);

//! Handler for TURN Server Connection Attempts
/*!
	\param turnModule TURN Client
	\param success 0 = FAIL, 1 = SUCCESS
*/
typedef void(*ILibTURN_OnConnectTurnHandler)(ILibTURN_ClientModule turnModule, int success);
//! Handler for TURN/ALLOCATE Response
/*!
	\param turnModule TURN Client
	\param Lifetime The cache timeout of the new allocation. If not renewed by this timeout, allocation will expire.
	\param RelayedTransportAddress The public IP Address/Port that is relayed
*/
typedef void(*ILibTURN_OnAllocateHandler)(ILibTURN_ClientModule turnModule, int Lifetime, struct sockaddr_in6* RelayedTransportAddress);
typedef void(*ILibTURN_OnRefreshHandler)(ILibTURN_ClientModule turnModule, int Lifetime, void *user);
//! Handler for TURN/PERMISSION Response
/*!
	\param turnModule TURN Client
	\param success 0 = FAIL, 1 = SUCCESS
	\param user Custom user state object
*/
typedef void(*ILibTURN_OnCreatePermissionHandler)(ILibTURN_ClientModule turnModule, int success, void *user);
//! Handler for received TURN/INDICATION
/*!
	\param turnModule TURN Client
	\param remotePeer Origin of received Indication
	\param buffer Data received
	\param offset Data Offset Received
	\param length Data Length
*/
typedef void(*ILibTURN_OnDataIndicationHandler)(ILibTURN_ClientModule turnModule, struct sockaddr_in6* remotePeer, char* buffer, int offset, int length);
//! Handler for TURN/CHANNELBIND Response
/*!
	\param turnModule TURN Client
	\param channelNumber Channel Number
	\param success 0 = FAIL, 1 = SUCCESS
	\param user Custom user state object
*/
typedef void(*ILibTURN_OnCreateChannelBindingHandler)(ILibTURN_ClientModule turnModule, unsigned short channelNumber, int success, void* user);
//! Handler for received TURN/CHANNELDATA
/*!
	\param turnModule TURN Client
	\param channelNumber Channel Number that received data
	\param buffer Received Data
	\param offset Received Data Offset
	\param length Received Data Length
*/
typedef void(*ILibTURN_OnChannelDataHandler)(ILibTURN_ClientModule turnModule, unsigned short channelNumber, char* buffer, int offset, int length);
typedef enum 
{
	ILibTURN_TransportTypes_UDP = 17,	//!< TURN Transport Type: UDP
	ILibTURN_TransportTypes_TCP = 6		//!< TURN Transport Type: TCP
}ILibTURN_TransportTypes;
//! Create a new TURN Client Instance
/*!
	\param chain Microstack Chain to add TURN Client to
	\param OnCennectTurn Event Handler for CONNECT Response
	\param OnAllocate Event Handler for ALLOCATE Response
	\param OnData Event Handler for INDICATION reception
	\param OnChannelData Event Handler for CHANNEL/DATA reception
	\return TURN Client Instance
*/
ILibTURN_ClientModule ILibTURN_CreateTurnClient(void* chain, ILibTURN_OnConnectTurnHandler OnConnectTurn, ILibTURN_OnAllocateHandler OnAllocate, ILibTURN_OnDataIndicationHandler OnData, ILibTURN_OnChannelDataHandler OnChannelData);
void ILibTURN_SetTag(ILibTURN_ClientModule clientModule, void *tag);
void* ILibTURN_GetTag(ILibTURN_ClientModule clientModule);
void ILibTURN_ConnectTurnServer(ILibTURN_ClientModule turnModule, struct sockaddr_in* turnServer, char* username, int usernameLen, char* password, int passwordLen, struct sockaddr_in* proxyServer);
void ILibTURN_Allocate(ILibTURN_ClientModule turnModule, ILibTURN_TransportTypes transportType);
void ILibTURN_RefreshAllocation(ILibTURN_ClientModule turnModule, ILibTURN_OnRefreshHandler handler, void *user);
int ILibTURN_CreateXORMappedAddress(struct sockaddr_in6 *endpoint, char* buffer, char* transactionID);
void ILibTURN_GetXORMappedAddress(char* buffer, int bufferLen, char* transactionID, struct sockaddr_in6 *value);
void ILibTURN_CreatePermission(ILibTURN_ClientModule turnModule, struct sockaddr_in6* permissions, int permissionsLength, ILibTURN_OnCreatePermissionHandler result, void* user);
enum ILibAsyncSocket_SendStatus ILibTURN_SendIndication(ILibTURN_ClientModule turnModule, struct sockaddr_in6* remotePeer, char* buffer, int offset, int length);
void ILibTURN_CreateChannelBinding(ILibTURN_ClientModule turnModule, unsigned short channelNumber, struct sockaddr_in6* remotePeer, ILibTURN_OnCreateChannelBindingHandler result, void* user);
enum ILibAsyncSocket_SendStatus ILibTURN_SendChannelData(ILibTURN_ClientModule turnModule, unsigned short channelNumber, char* buffer, int offset, int length);
int ILibTURN_IsConnectedToServer(ILibTURN_ClientModule clientModule);
unsigned int ILibTURN_GetPendingBytesToSend(ILibTURN_ClientModule turnModule);
/*! @} */
/*! @} */

#endif