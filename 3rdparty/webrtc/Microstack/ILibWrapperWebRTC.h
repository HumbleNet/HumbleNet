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

#ifndef ___ILibWrapper_WebRTC___
#define ___ILibWrapper_WebRTC___

#include "ILibAsyncSocket.h"

#define ILibTransports_WebRTC_DataChannel 0x51

typedef void* ILibWrapper_WebRTC_ConnectionFactory;
typedef void* ILibWrapper_WebRTC_Connection;

struct ILibWrapper_WebRTC_DataChannel;
typedef void(*ILibWrapper_WebRTC_DataChannel_OnDataChannelAck)(struct ILibWrapper_WebRTC_DataChannel* dataChannel);
typedef void(*ILibWrapper_WebRTC_DataChannel_OnData)(struct ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen);
typedef void(*ILibWrapper_WebRTC_DataChannel_OnRawData)(struct ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen, int dataType);
typedef void(*ILibWrapper_WebRTC_DataChannel_OnClosed)(struct ILibWrapper_WebRTC_DataChannel* dataChannel);
typedef struct ILibWrapper_WebRTC_DataChannel
{	
	ILibWrapper_WebRTC_DataChannel_OnData OnBinaryData;
	ILibWrapper_WebRTC_DataChannel_OnData OnStringData;
	ILibWrapper_WebRTC_DataChannel_OnRawData OnRawData;
	void* Chain;
	ILibTransport_SendPtr SendPtr;
	ILibTransport_ClosePtr ClosePtr;
	ILibTransport_PendingBytesToSendPtr PendingBytesPtr;
	unsigned int IdentifierFlags;
	/*
	*
	* DO NOT MODIFY STRUCT DEFINITION ABOVE THIS COMMENT BLOCK
	*
	*/
	unsigned short streamId;
	char* channelName;

	ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAck;
	ILibWrapper_WebRTC_DataChannel_OnClosed OnClosed;
	ILibWrapper_WebRTC_Connection parent;
	void* userData;
}ILibWrapper_WebRTC_DataChannel;

typedef void(*ILibWrapper_WebRTC_OnConnectionCandidate)(ILibWrapper_WebRTC_Connection connection, struct sockaddr_in6* candidate);
typedef void(*ILibWrapper_WebRTC_Connection_OnConnect)(ILibWrapper_WebRTC_Connection connection, int connected);
typedef void(*ILibWrapper_WebRTC_Connection_OnDataChannel)(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_DataChannel *dataChannel);
typedef void(*ILibWrapper_WebRTC_Connection_OnSendOK)(ILibWrapper_WebRTC_Connection connection);

typedef void(*ILibWrapper_WebRTC_Connection_Debug_OnEvent)(ILibWrapper_WebRTC_Connection connection, char* debugFieldName, int debugValue);

// 
// WebRTC Connection Factory
//

// Creates a Factory object that can create WebRTC Connection objects.
ILibWrapper_WebRTC_ConnectionFactory ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory(void* chain, unsigned short localPort);

// Sets the TURN server to use for all WebRTC connections
void ILibWrapper_WebRTC_ConnectionFactory_SetTurnServer(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr_in6* turnServer, char* username, int usernameLength, char* password, int passwordLength, ILibWebRTC_TURN_ConnectFlags turnSetting);

// Creates an unconnected WebRTC Connection 
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnection(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK);




//
// WebRTC Connection
//

// Set the STUN Servers to use with the WebRTC Connection when gathering candidates
void ILibWrapper_WebRTC_Connection_SetStunServers(ILibWrapper_WebRTC_Connection connection, char** serverList, int serverLength);

// Non zero value if the underlying SCTP session is established
int ILibWrapper_WebRTC_Connection_IsConnected(ILibWrapper_WebRTC_Connection connection);

// Disconnects the unerlying SCTP session, if it is connected
void ILibWrapper_WebRTC_Connection_Disconnect(ILibWrapper_WebRTC_Connection connection);

// Destroys a WebRTC Connection
void ILibWrapper_WebRTC_Connection_DestroyConnection(ILibWrapper_WebRTC_Connection connection);

// Closes the WebRTC Data Channel
void ILibWrapper_WebRTC_DataChannel_Close(ILibWrapper_WebRTC_DataChannel* dataChannel);

// Creates a WebRTC Data Channel, using the next available Stream ID
ILibWrapper_WebRTC_DataChannel* ILibWrapper_WebRTC_DataChannel_Create(ILibWrapper_WebRTC_Connection connection, char* channelName, int channelNameLen, ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAckHandler);

// Creates a WebRTC Data Channel, using the specified Stream ID
ILibWrapper_WebRTC_DataChannel* ILibWrapper_WebRTC_DataChannel_CreateEx(ILibWrapper_WebRTC_Connection connection, char* channelName, int channelNameLen, unsigned short streamId, ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAckHandler);

void ILibWrapper_WebRTC_Connection_SetUserData(ILibWrapper_WebRTC_Connection connection, void *user1, void *user2, void *user3);
void ILibWrapper_WebRTC_Connection_GetUserData(ILibWrapper_WebRTC_Connection connection, void **user1, void **user2, void **user3);
int ILibWrapper_WebRTC_Connection_DoesPeerSupportUnreliableMode(ILibWrapper_WebRTC_Connection connection);
int ILibWrapper_WebRTC_Connection_SetReliabilityMode(ILibWrapper_WebRTC_Connection connection, int isOrdered, short maxRetransmits, short maxLifetime);

// WebRTC Connection Management

// Generate an SDP Offer (WebRTC Initiator)
char* ILibWrapper_WebRTC_Connection_GenerateOffer(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_OnConnectionCandidate onCandidates);

// Set an SDP Answer/Offer (WebRTC Receiver)
char* ILibWrapper_WebRTC_Connection_SetOffer(ILibWrapper_WebRTC_Connection connection, char* offer, int offerLen, ILibWrapper_WebRTC_OnConnectionCandidate onCandidates);

// Generate an udpated SDP offer containing the candidate specified
char* ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToLocalSDP(ILibWrapper_WebRTC_Connection connection, struct sockaddr_in6* candidate);

// Stop reading inbound data
void ILibWrapper_WebRTC_Connection_Pause(ILibWrapper_WebRTC_Connection connection); 

// Resume reading inbound data
void ILibWrapper_WebRTC_Connection_Resume(ILibWrapper_WebRTC_Connection connection); 


//
// WebRTC Data Channels
//

// Send Binary Data over the specified Data Channel
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_Send(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen);

// Send Arbitrary Data over the specified Data Channel. (Must specify the data type)
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_SendEx(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen, int dataType);

// Send String Data over the specified Data Channel
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_SendString(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen);

#ifdef _WEBRTCDEBUG
int ILibWrapper_WebRTC_Connection_Debug_Set(ILibWrapper_WebRTC_Connection connection, char* debugFieldName, ILibWrapper_WebRTC_Connection_Debug_OnEvent eventHandler);
void ILibWrapper_WebRTC_ConnectionFactory_SetSimulatedLossPercentage(ILibWrapper_WebRTC_ConnectionFactory factory, int lossPercentage);
#endif
#endif