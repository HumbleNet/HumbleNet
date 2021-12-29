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

#ifndef MICROSTACK_NOTLS
#ifndef ___ILibWrapper_WebRTC___
#define ___ILibWrapper_WebRTC___

#include "ILibAsyncSocket.h"
#include "ILibCrypto.h"

#define ILibWebWrapperWebRTC_ConnectionFactory_INIT_CRYPTO_LATER (void*)0x01
/*! \file ILibWrapperWebRTC.h
\brief Microstack API/Abstraction for WebRTC Functionality
*/

/*! \defgroup ILibWrapperWebRTC ILibWrapperWebRTC Module */
/*! @{ */

#define ILibTransports_WebRTC_DataChannel 0x51
extern const int ILibMemory_WebRTC_Connection_CONTAINERSIZE;

/** Factory object that is used to create WebRTC Connections. */
typedef void* ILibWrapper_WebRTC_ConnectionFactory;
/** Object encapsulating WebRTC Peer Connections. */
typedef void* ILibWrapper_WebRTC_Connection;

struct ILibWrapper_WebRTC_DataChannel;

//! Handler for when a remote peer has ACK'ed a Create Data Channel request
/*
	\param dataChannel The data channel that the peer has AcK'ed
*/
typedef void(*ILibWrapper_WebRTC_DataChannel_OnDataChannelAck)(struct ILibWrapper_WebRTC_DataChannel* dataChannel);
//! Handler for when data is received on a Data Channel
/*! 
	\param dataChannel The Data Channel on which data has arrived
	\param data Received data
	\param dataLen Number of bytes received
*/
typedef void(*ILibWrapper_WebRTC_DataChannel_OnData)(struct ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen);
//! Raw Handler for when data is received on a Data Channel
/*!
	\param dataChannel The Data Channel on which data has arrived
	\param data Received Data
	\param dataLen Number of bytes received
	\param dataType Kind of Data received. 51 = String, 53 = Binary, etc.
*/
typedef void(*ILibWrapper_WebRTC_DataChannel_OnRawData)(struct ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen, int dataType);
//! Handler for when a data channel is closed
/*! 
	\param dataChannel The data channel that was closed
*/
typedef void(*ILibWrapper_WebRTC_DataChannel_OnClosed)(struct ILibWrapper_WebRTC_DataChannel* dataChannel);
/** DataChannel abstraction used to send/receive peer-to-peer data. */
typedef struct ILibWrapper_WebRTC_DataChannel
{	
	ILibWrapper_WebRTC_DataChannel_OnData OnBinaryData; //!< Binary Data Event Handler
	ILibWrapper_WebRTC_DataChannel_OnData OnStringData; //!< String Data Event Handler
	ILibWrapper_WebRTC_DataChannel_OnRawData OnRawData; //!< Raw Data Event Handler
	void* Chain; //!< Microstack Chain to which this object resides
	void* ReservedMemory; //!< RESERVED
	ILibTransport_SendPtr SendPtr; //!< RESERVED
	ILibTransport_ClosePtr ClosePtr; //!< RESERVED
	ILibTransport_PendingBytesToSendPtr PendingBytesPtr; //!< RESERVED
	ILibTransport_OnSendOK TransportSendOKPtr; //!< RESERVED
	unsigned int IdentifierFlags; //!< RESERVED
	/*
	*
	* DO NOT MODIFY STRUCT DEFINITION ABOVE THIS COMMENT BLOCK
	*
	*/
	unsigned short streamId; //!< Stream Identifier for this Data Channel
	char* channelName; //!< Friendly Name for this Data Channel

	ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAck; //!< Event triggered when Data Channel creation is ACKnowledged by the remote peer.
	ILibWrapper_WebRTC_DataChannel_OnClosed OnClosed; //!< Event triggered when the Data Channel has been closed.
	ILibWrapper_WebRTC_Connection parent; //!< The Peer Connection object that owns this data channel object
	void* userData; //!< User specified state object
}ILibWrapper_WebRTC_DataChannel;

//! Handler for new ICE candidates
/*!
	\param connection The RTC Peer Connection object that called
	\param candidate The ICE candidate
*/
typedef void(*ILibWrapper_WebRTC_OnConnectionCandidate)(ILibWrapper_WebRTC_Connection connection, struct sockaddr_in6* candidate);
//! Handler for when the connection state of an SCTP session has changed
/*!
	\param connection The RTC Peer Connection parent object
	\param connected 0 = Disconnected, 1 = Connected
*/
typedef void(*ILibWrapper_WebRTC_Connection_OnConnect)(ILibWrapper_WebRTC_Connection connection, int connected);
//! Handler for when a new Data Channel was established
/*!
	\param connection Parent RTC Peer Connection object
	\param dataChannel The Data Channel object that was created
*/
typedef void(*ILibWrapper_WebRTC_Connection_OnDataChannel)(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_DataChannel *dataChannel);
//! Handler for when all the buffered send operations for a Peer Connection has been flushed
/*!
	\param connection The RTC Peer Connection object whose send buffers have been flushed
*/
typedef void(*ILibWrapper_WebRTC_Connection_OnSendOK)(ILibWrapper_WebRTC_Connection connection);

typedef void(*ILibWrapper_WebRTC_Connection_Debug_OnEvent)(ILibWrapper_WebRTC_Connection connection, char* debugFieldName, int debugValue);


/**
* \defgroup WebRTC_ConnectionFactory WebRTC Connection Factory Methods
* @{
*/


//! Creates a Factory object that can create WebRTC Connection objects. 
/*!
	\paramn chain The microstack chain to associate with this factory
	\param localPort The local UDP port to bind, for ICE/DTLS. 0 = Random
	\return A new connection factory instance
*/
ILibWrapper_WebRTC_ConnectionFactory ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory(void* chain, unsigned short localPort);

//! Creates a Factory object that can create WebRTC Connection objects, using the specified certs 
/*!
\paramn chain The microstack chain to associate with this factory
\param localPort The local UDP port to bind, for ICE/DTLS. 0 = Random
\param tlsCert The certificate to use. ILibWebWrapperWebRTC_ConnectionFactory_INIT_CRYPTO_LATER to init crypto later
\return A new connection factory instance
*/
ILibWrapper_WebRTC_ConnectionFactory ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory2(void* chain, unsigned short localPort, struct util_cert *tlsCert);

//! Initializes the crypto for a ConnectionFactory object. (Note: Only valid if ILibWebWrapperWebRTC_ConnectionFactory_INIT_CRYPTO_LATER was used previously)
/*!
\paramn factory The ConnectionFactory object to initialize crypto on
\param tlsCert The certificate to use, or NULL to generate self certs
*/
void ILibWrapper_WebRTC_ConnectionFactory_InitCrypto(ILibWrapper_WebRTC_ConnectionFactory factory, struct util_cert *tlsCert);

void ILibWrapper_WebRTC_Connection_SetOnSendOK(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_Connection_OnSendOK SendOKSink);

SSL_CTX* ILibWrapper_WebRTC_ConnectionFactory_GetCTX(ILibWrapper_WebRTC_ConnectionFactory factory);
void* ILibWrapper_WebRTC_Connection_GetChain(ILibWrapper_WebRTC_Connection connection);

void ILibWrapper_WebRTC_ConnectionFactory_SendRaw(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr* target, char* data, int datalen, enum ILibAsyncSocket_MemoryOwnership UserFree);

//! Sets the TURN server to use for all WebRTC connections. 
/*!
	\param factory ConnectionFactory to configure
	\param turnServer The TURN Server to configure
	\param username Username for the TURN Server
	\param usernameLength Length of buffer holding username
	\param password Password for the TURN Server
	\param passwordLength Length of the buffer holding the password
	\param turnSetting TURN configuration settings
*/
void ILibWrapper_WebRTC_ConnectionFactory_SetTurnServer(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr_in6* turnServer, char* username, int usernameLength, char* password, int passwordLength, ILibWebRTC_TURN_ConnectFlags turnSetting);

typedef void(*ILibWrapper_WebRTC_ConnectionFactory_STUNHandler)(ILibWrapper_WebRTC_ConnectionFactory sender, ILibStun_Results Result, struct sockaddr_in* PublicIP);
void ILibWrapper_WebRTC_ConnectionFactory_PerformSTUN(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr_in* StunServer);
void ILibWrapper_WebRTC_ConnectionFactory_Set_STUNHandler(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_ConnectionFactory_STUNHandler handler);

//! Creates an unconnected WebRTC Connection. 
/*!
	\param factory ConnectionFactory to use to create the RTC Peer Connection
	\param OnConnectHandler Callback to trigger when the connection state has changed
	\param OnDataChannelHandler Callback to trigger when new Data Channels are established for the new RTC Peer Connection
	\param OnConnectionSendOK Callback to trigger when buffered Send data for the Peer Connection is flushed. 
	\return An unconnected RTC Peer Connection
*/
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnection(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK);
//! Creates an unconnected WebRTC Connection, with ILibMemory 
/*!
\param factory ConnectionFactory to use to create the RTC Peer Connection
\param OnConnectHandler Callback to trigger when the connection state has changed
\param OnDataChannelHandler Callback to trigger when new Data Channels are established for the new RTC Peer Connection
\param OnConnectionSendOK Callback to trigger when buffered Send data for the Peer Connection is flushed.
\param extraMemorySize The size of the extra memory partition to allocation
\return An unconnected RTC Peer Connection
*/
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnection2(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK, int extraMemorySize);

//! Fetches the associated WebRTC Connection object from the IceOfferBlock (creating a new one if it doesn't exist), and sets the remote offer
/*!
\param factory ConnectionFactory to use to create the RTC Peer Connection
\param iceOfferBlock The IceOfferBlock from MeshCentral
\param iceOfferBlockLen The length of the IceOfferBlock from MeshCentral
\param OnConnectHandler Callback to trigger when the connection state has changed
\param OnDataChannelHandler Callback to trigger when new Data Channels are established for the new RTC Peer Connection
\param OnConnectionSendOK Callback to trigger when buffered Send data for the Peer Connection is flushed.
\return An unconnected RTC Peer Connection
*/
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionEx(ILibWrapper_WebRTC_ConnectionFactory factory, char *iceOfferBlock, int iceOfferBlockLen, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK);
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionEx2(ILibWrapper_WebRTC_ConnectionFactory factory, char *iceOfferBlock, int iceOfferBlockLen, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK, int extraMemorySize);

/** @}*/


/**
* \defgroup WebRTC_Connection WebRTC Connection Methods
* @{
*/
int ILibWrapper_WebRTC_Connection_GetID(ILibWrapper_WebRTC_Connection connection);
char* ILibWrapper_WebRTC_Connection_GetLocalUsername(ILibWrapper_WebRTC_Connection connection);

ILibTransport* ILibWrapper_WebRTC_Connection_GetRawTransport(ILibWrapper_WebRTC_Connection connection);
//! Set the STUN Servers to use with the WebRTC Connection when gathering candidates
/*!
	\param connection RTC Peer Connection to configure
	\param serverList Array of NULL terminated strings, where each string is a STUN Server Hostname
	\param serverLength Size of the serverList array
*/
void ILibWrapper_WebRTC_Connection_SetStunServers(ILibWrapper_WebRTC_Connection connection, char** serverList, int serverLength);

//! Queries the SCTP connection state of an RTC Peer Connection
/*!
	\param connection The RTC Peer Connection to query
	\return 0 = Disconnected, Non-Zero = Connected
*/
int ILibWrapper_WebRTC_Connection_IsConnected(ILibWrapper_WebRTC_Connection connection);

//! Disconnects the unerlying SCTP session, if it is connected
/*!
	\param connection The RTC Peer Connection to disconnect
*/
void ILibWrapper_WebRTC_Connection_Disconnect(ILibWrapper_WebRTC_Connection connection);

//! Gets the underlying Local ICE Offer Block
/*!
\param connection The RTC Peer Connection to query
\param localOfferBlock the Local Offer Block associated with this connection
\return The length of localOfferBlock
*/
int ILibWrapper_WebRTC_Connection_GetLocalOfferBlock(ILibWrapper_WebRTC_Connection connection, char **localOfferBlock);

//! Destroys a WebRTC Connection
/*!
	\param connection The RTC Peer Connection to destroy
*/
void ILibWrapper_WebRTC_Connection_DestroyConnection(ILibWrapper_WebRTC_Connection connection);
void ILibWrapper_WebRTC_Connection_CloseAllDataChannels(ILibWrapper_WebRTC_Connection connection);

//! Closes the WebRTC Data Channel.
//! If SCTP/RECONFIG is supported, it will request to close the Data Channel before shutting down DTLS
/*!
	\ingroup WebRTC_DataChannel
	\param dataChannel The Data Channel to close.
*/
void ILibWrapper_WebRTC_DataChannel_Close(ILibWrapper_WebRTC_DataChannel* dataChannel);

//! Creates a WebRTC Data Channel, using the next available Stream ID
/*!
	\ingroup WebRTC_DataChannel
	\param connection The RTC Peer Connection on which to create a new data channel
	\param channelName Name of the data channel to create
	\param channelNameLen Length of channelName
	\param OnAckHandler Callback to trigger when the Remote Peer ACK's this request (Can be NULL)
	\return An un-Acknowledged Data Channel
*/
ILibWrapper_WebRTC_DataChannel* ILibWrapper_WebRTC_DataChannel_Create(ILibWrapper_WebRTC_Connection connection, char* channelName, int channelNameLen, ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAckHandler);

//! Creates a WebRTC Data Channel, using the specified Stream ID
/*!
	\ingroup WebRTC_DataChannel
	\param connection The RTC Peer Connection on which to create a new data channel
	\param channelName Name of the data channel to create
	\param channelNameLen Length of channelName
	\param streamId The stream identifier to associate with the Data Channel
	\param OnAckHandler Callback to trigger when the Remote Peer ACK's this request (Can be NULL)
	\return An un-Acknowledged Data Channel
*/
ILibWrapper_WebRTC_DataChannel* ILibWrapper_WebRTC_DataChannel_CreateEx(ILibWrapper_WebRTC_Connection connection, char* channelName, int channelNameLen, unsigned short streamId, ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAckHandler);

//! Sets Custom User State Data
/*!
	\param connection The RTC Peer Connection to set user data to
	\param user1
	\param user2
	\param user3
*/
void ILibWrapper_WebRTC_Connection_SetUserData(ILibWrapper_WebRTC_Connection connection, void *user1, void *user2, void *user3);
//! Retrieves stored Custom User State Data
/*!
	\param connection The RTC Peer Connection to fetch user data from
	\param user1
	\param user2
	\param user3
*/
void ILibWrapper_WebRTC_Connection_GetUserData(ILibWrapper_WebRTC_Connection connection, void **user1, void **user2, void **user3);
int ILibWrapper_WebRTC_Connection_DoesPeerSupportUnreliableMode(ILibWrapper_WebRTC_Connection connection);
int ILibWrapper_WebRTC_Connection_SetReliabilityMode(ILibWrapper_WebRTC_Connection connection, int isOrdered, short maxRetransmits, short maxLifetime);

// WebRTC Connection Management
void ILibWrapper_WebRTC_Connection_CreateLocalParameters_ICE(ILibWrapper_WebRTC_Connection connection);
void ILibWrapper_WebRTC_Connection_GetLocalParameters_ICE(ILibWrapper_WebRTC_Connection connection, char **username, int *usernameLen, char **password, int *passwordLen);
void ILibWrapper_WebRTC_Connection_GetLocalParameters_DTLS(ILibWrapper_WebRTC_Connection connection, char **hash, int *hashLen);

//! Generate an SDP Offer (WebRTC Initiator)
/*!
	\param connection The RTC Connection object to act as initiator
	\param onCandidates Callback triggered for each discovered ICE candidate
	\return SDP offer that contains only local candidates
*/
char* ILibWrapper_WebRTC_Connection_GenerateOffer(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_OnConnectionCandidate onCandidates);

//! Set an SDP Answer/Offer
/*!
	\param connection The RTC Connection object to set the response to
	\param offer The SDP Answer/Offer to set
	\param offerLen The size of the buffer holding the SDP
	\param onCandidates Callback to trigger when additional ICE candidates are discovered
	\return SDP Counter Offer, containing only local candidates, which can be returned to the sender
*/
char* ILibWrapper_WebRTC_Connection_SetOffer(ILibWrapper_WebRTC_Connection connection, char* offer, int offerLen, ILibWrapper_WebRTC_OnConnectionCandidate onCandidates);

//! Generate an udpated SDP offer containing the reflexive candidate specified
/*!
	\param connection The RTC Peer Connection that generated the reflexive candidate
	\param candidate The reflexive candidate
	\return SDP Offer containing the reflexive candidate
*/
char* ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToLocalSDP(ILibWrapper_WebRTC_Connection connection, struct sockaddr_in6* candidate);

// Pause processing inbound data, and buffer it
/*!
	\param connection The RTC Connection to pause
*/
void ILibWrapper_WebRTC_Connection_Pause(ILibWrapper_WebRTC_Connection connection); 

//! Resume processing inbound data
/*!
	\param connection The RTC Connection to resume
*/
void ILibWrapper_WebRTC_Connection_Resume(ILibWrapper_WebRTC_Connection connection); 
/** @}*/

/**
* \defgroup WebRTC_DataChannel WebRTC Data Channel Methods
* @{
*/

//! Send Binary Data over the specified Data Channel
/*!
	\param dataChannel The Data Channel to send the data on
	\param data	The binary data to send
	\param dataLen The size of the binary data buffer
	\return The Send status of the send operation
*/
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_Send(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen);

//! Send Arbitrary Data over the specified Data Channel.
/*!
	\param dataChannel The Data Channel to send the data on
	\param data	The data to send
	\param dataLen The size of the data buffer
	\param dataType The type of data to be sent (51 = String, 53 = Binary, etc)
	\return The Send status of the send operation
*/
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_SendEx(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen, int dataType);

//! Send String Data over the specified Data Channel
/*!
	\param dataChannel The Data Channel to send the data on
	\param data	The string data to send
	\param dataLen The size of the string data buffer
	\return The Send status of the send operation
*/
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_SendString(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen);

#ifdef _WEBRTCDEBUG
int ILibWrapper_WebRTC_Connection_Debug_Set(ILibWrapper_WebRTC_Connection connection, char* debugFieldName, ILibWrapper_WebRTC_Connection_Debug_OnEvent eventHandler);
void ILibWrapper_WebRTC_ConnectionFactory_SetSimulatedLossPercentage(ILibWrapper_WebRTC_ConnectionFactory factory, int lossPercentage);
#endif

/** @}*/
/** @}*/
#endif
#endif
