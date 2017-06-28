#ifndef EMSCRIPTEN 

#ifdef _MSC_VER
// silence warnings about sscanf
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "libwebrtc.h"

#include "libpoll.h"

#include <cassert>

//#include <map>
#include <string>
#include <vector>

extern "C" {
#include <ILibAsyncSocket.h>
#include <ILibParsers.h>
#include <ILibWebRTC.h>
#include <ILibWrapperWebRTC.h>
}  // extern "C"

#ifdef WIN32
#if !defined(snprintf) && (_MSC_PLATFORM_TOOLSET <= 120)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif

#endif

#define LOG printf

struct libwebrtc_context {
	void* chain;
	ILibWrapper_WebRTC_ConnectionFactory factory;
	std::vector<std::string> stunServers;
	lwrtc_callback_function callback;
};

// User data fields stored in ILibWrapper_XXXX_SetUserData
//
// user1 will be the libwebrtc_context
// user3 will be the users data

#ifdef HUMBLENET_LOAD_WEBRTC
namespace Microstack {
#endif

// This is called when a WebRTC Connection is established or disconnected
void WebRTCConnectionStatus(ILibWrapper_WebRTC_Connection webRTCConnection, int connected) {
	assert(webRTCConnection != NULL);
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	libwebrtc_connection* conn = (libwebrtc_connection*)webRTCConnection;
	
	ILibWrapper_WebRTC_Connection_GetUserData(webRTCConnection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );
	
	if( connected )
		ctx->callback( ctx, conn, NULL, LWRTC_CALLBACK_ESTABLISHED, user_data, NULL, 0);
	else {
		// TODO: We really should notify all the channels as closed as well, as we can't rely on microstack to do it as it occurs after this call.

		// Clear the user data from the Connection, so additional callbacks wont be fired, as an DCs that are active get closed AFTER this call.
		ILibWrapper_WebRTC_Connection_SetUserData(webRTCConnection, NULL, NULL, NULL);

		ctx->callback( ctx, conn, NULL, LWRTC_CALLBACK_DISCONNECTED, user_data, NULL, 0);
		// for all intensive purposes the connection object is destroyed at this point. We dont have a specific callback,
		// but since this was called it WILL be destroyed when this call returns.
		ctx->callback( ctx, conn, NULL, LWRTC_CALLBACK_DESTROY, user_data, NULL, 0);
	}
}

// this is called when the connection can resume sending data.
void WebRTCConnectionSendOk(ILibWrapper_WebRTC_Connection connection) {
	LOG("WebRTCConnectionSendOk %p\n", connection);
}

// this is called for each ice candidate. if candidate is null, no additional candidates could be found.
void WebRTCOnIceCandidate(ILibWrapper_WebRTC_Connection webRTCConnection, struct sockaddr_in6* candidate) {
	assert(webRTCConnection != NULL);
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	libwebrtc_connection* conn = (libwebrtc_connection*)webRTCConnection;

	ILibWrapper_WebRTC_Connection_GetUserData(webRTCConnection, (void**)&ctx, NULL, &user_data);
	
	// no additional candidates found.
	if( candidate == NULL )
		return;
	
	char* offer = ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToLocalSDP( webRTCConnection, candidate );
	if( offer == NULL ) {
		ctx->callback(ctx, conn, NULL, LWRTC_CALLBACK_ERROR, user_data, NULL, 0 );
		LOG("Offer failed, closing socket\n");
		return;
	}
	free(offer);
	
	char address[225];
	
	ILibInet_ntop2((struct sockaddr*)candidate, address, 255);
	
	const char* candidateFormat = "candidate:%d %d UDP %d %s %u typ host";
	char buf[255] = {0};
	
	snprintf(buf, sizeof(buf), candidateFormat, 0, 1, 2128609535-1, address, ntohs(candidate->sin6_port));
	buf[sizeof(buf)-1] = 0;
	
	ctx->callback(ctx, conn, NULL, LWRTC_CALLBACK_ICE_CANDIDATE, user_data, buf, strlen(buf) );
}

// this is called when a channel receives data.
void WebRTCOnDataChannelData(struct ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen) {
	assert(dataChannel != NULL);
	assert(data != NULL);
	assert(dataLen > 0);
	
	void* user_data = dataChannel->userData;
	libwebrtc_context* ctx = NULL;
	libwebrtc_connection* conn = (libwebrtc_connection*)dataChannel->parent;
	libwebrtc_data_channel* channel = (libwebrtc_data_channel*)dataChannel;
	
	ILibWrapper_WebRTC_Connection_GetUserData(dataChannel->parent, (void**)&ctx, NULL, NULL);//&user_data);
	assert( ctx != NULL );
	
	ctx->callback( ctx, conn, channel, LWRTC_CALLBACK_CHANNEL_RECEIVE, user_data, data, dataLen );
}

// This is called when the DataChannel is closed
void WebRTCOnDataChannelClosed(ILibWrapper_WebRTC_DataChannel *dataChannel)
{
	assert(dataChannel != NULL);
	
	void* user_data = dataChannel->userData;
	libwebrtc_context* ctx = NULL;
	libwebrtc_connection* conn = (libwebrtc_connection*)dataChannel->parent;
	libwebrtc_data_channel* channel = (libwebrtc_data_channel*)dataChannel;
	
	ILibWrapper_WebRTC_Connection_GetUserData(dataChannel->parent, (void**)&ctx, NULL, NULL);//&user_data);
	if( ! ctx )
		return;
	
	// NOTE: When channels are auto-closed due to connection being destroyed, the channel_closed callback will occur AFTER the connection closed callback...    
	ctx->callback( ctx, conn, channel, LWRTC_CALLBACK_CHANNEL_CLOSED, user_data, dataChannel->channelName, strlen(dataChannel->channelName) );
}

// This is called when the remote side creates a data channel
void WebRTCDataChannelAccept(ILibWrapper_WebRTC_Connection webRTCConnection, ILibWrapper_WebRTC_DataChannel *dataChannel) {
	assert( webRTCConnection != NULL );
	assert( dataChannel != NULL );
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	libwebrtc_connection* conn = (libwebrtc_connection*)webRTCConnection;
	libwebrtc_data_channel* channel = (libwebrtc_data_channel*)dataChannel;
	
	ILibWrapper_WebRTC_Connection_GetUserData(webRTCConnection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );

	dataChannel->OnBinaryData = &WebRTCOnDataChannelData;
	dataChannel->OnClosed = &WebRTCOnDataChannelClosed;
	
	// use the parent user_data initially.
	dataChannel->userData = user_data;
	
	ctx->callback( ctx, conn, channel, LWRTC_CALLBACK_CHANNEL_ACCEPTED, user_data, dataChannel->channelName, strlen(dataChannel->channelName) );
}

// This is called when the remote ACK's our DataChannel creation request
void WebRTCOnDataChannelAck(ILibWrapper_WebRTC_DataChannel *dataChannel)
{
	assert( dataChannel != NULL );
	assert( dataChannel->parent != NULL );
	
	void* user_data = dataChannel->userData;
	libwebrtc_context* ctx = NULL;
	libwebrtc_connection* conn = (libwebrtc_connection*)dataChannel->parent;
	libwebrtc_data_channel* channel = (libwebrtc_data_channel*)dataChannel;
	
	ILibWrapper_WebRTC_Connection_GetUserData(dataChannel->parent, (void**)&ctx, NULL, NULL);//&Nuser_data);
	assert( ctx != NULL );
	
	dataChannel->OnBinaryData = &WebRTCOnDataChannelData;
	dataChannel->OnClosed = &WebRTCOnDataChannelClosed;
	
	ctx->callback( ctx, conn, channel, LWRTC_CALLBACK_CHANNEL_CONNECTED, user_data, dataChannel->channelName, strlen(dataChannel->channelName) );
}

//
// Start of public API implementation
//

struct libwebrtc_context* libwebrtc_create_context(lwrtc_callback_function callback) {
	assert( callback != NULL );
	libwebrtc_context* ctx = new libwebrtc_context();
	
	ctx->chain = poll_init();
	ctx->factory = ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory(ctx->chain, 0);
	ctx->callback = callback;
	
	return ctx;
}

void libwebrtc_destroy_context( struct libwebrtc_context* ctx ) {
	ILibChain_SafeRemove( ctx->chain, ctx->factory);
	
	delete ctx;
}

void libwebrtc_set_stun_servers( struct libwebrtc_context* ctx, const char** servers, int count)
{
	ctx->stunServers.clear();
	for( int i = 0; i < count; ++i ) {
		ctx->stunServers.push_back( *servers );
		servers++;
	}
}

struct libwebrtc_connection* libwebrtc_create_connection_extended( struct libwebrtc_context* ctx, void* user_data )
{
	ILibWrapper_WebRTC_Connection conn = ILibWrapper_WebRTC_ConnectionFactory_CreateConnection(ctx->factory, &WebRTCConnectionStatus, &WebRTCDataChannelAccept, &WebRTCConnectionSendOk);
	
	std::vector<const char*> stunServers;
	for( std::vector<std::string>::iterator it = ctx->stunServers.begin(); it != ctx->stunServers.end(); ++it ) {
		stunServers.push_back( it->c_str() );
	}
	ILibWrapper_WebRTC_Connection_SetStunServers(conn, (char**)&stunServers[0], stunServers.size());
	
	ILibWrapper_WebRTC_Connection_SetUserData(conn, ctx, NULL, user_data);

	return (libwebrtc_connection*)conn;
}

struct libwebrtc_data_channel* libwebrtc_create_channel( struct libwebrtc_connection* c, const char* name )
{
	ILibWrapper_WebRTC_Connection connection = (ILibWrapper_WebRTC_Connection)c;
	
	if( ! ILibWrapper_WebRTC_Connection_IsConnected( connection ) )
		return NULL;
	
	ILibWrapper_WebRTC_DataChannel* channel = ILibWrapper_WebRTC_DataChannel_Create(connection, (char*)name, strlen(name), &WebRTCOnDataChannelAck );
	
	// Initially use parent user_data
	ILibWrapper_WebRTC_Connection_GetUserData(connection, NULL, NULL, &channel->userData);
	
	return (libwebrtc_data_channel*)channel;
}

int libwebrtc_create_offer( struct libwebrtc_connection* c ) {
	ILibWrapper_WebRTC_Connection connection = (ILibWrapper_WebRTC_Connection)c;
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	
	ILibWrapper_WebRTC_Connection_GetUserData(connection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );

	
	char* offer = ILibWrapper_WebRTC_Connection_GenerateOffer(connection, &WebRTCOnIceCandidate );
	if( offer == NULL )
		return 0;
	
	// always send this offer, as the callback will only send additianal candidates
	if( ctx->callback(ctx, c, NULL, LWRTC_CALLBACK_LOCAL_DESCRIPTION, user_data, offer, strlen(offer)) ) {
		return 0;
	}
	
	return 1;
}

int libwebrtc_set_offer( struct libwebrtc_connection* conn, const char* sdp ) {
	ILibWrapper_WebRTC_Connection connection = (ILibWrapper_WebRTC_Connection)conn;
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	
	ILibWrapper_WebRTC_Connection_GetUserData(connection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );

	// Microstack requires at least one candidate
	std::string tmp;
	if( strstr(sdp, "a=candidate") == NULL ) {
		tmp = sdp;
		tmp += "a=candidate:0 1 UDP 2128609534 0.0.0.0 0 typ host\n";
		sdp = tmp.c_str();
	}

	char* offer = ILibWrapper_WebRTC_Connection_SetOffer(connection, (char*)sdp, strlen(sdp), &WebRTCOnIceCandidate);
	if( offer == NULL )
		return 0;
	
	// always send this offer, as the callback will only send additianal candidates
	ctx->callback(ctx, conn, NULL, LWRTC_CALLBACK_LOCAL_DESCRIPTION, user_data, offer, strlen(offer));
	
	return 1;
}

int libwebrtc_set_answer( struct libwebrtc_connection* conn, const char* sdp ) {
	ILibWrapper_WebRTC_Connection connection = (ILibWrapper_WebRTC_Connection)conn;
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	
	ILibWrapper_WebRTC_Connection_GetUserData(connection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );
	
	// Microstack requires at least one candidate
	std::string tmp;
	if( strstr(sdp, "a=candidate") == NULL ) {
		tmp = sdp;
		tmp += "a=candidate:0 1 UDP 2128609534 0.0.0.0 0 typ host\n";
		sdp = tmp.c_str();
	}

	char* offer = ILibWrapper_WebRTC_Connection_SetOffer(connection, (char*)sdp, strlen(sdp), NULL); // no CB as this is the answer
	if( offer == NULL )
		return 0;

	return 1;
}

extern "C" char* ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToRemoteSDP(ILibWrapper_WebRTC_Connection connection, const char*address, int port);

int libwebrtc_add_ice_candidate( struct libwebrtc_connection* conn, const char* candidate ) {
	ILibWrapper_WebRTC_Connection connection = (ILibWrapper_WebRTC_Connection)conn;

	// updating the block seems to be causing some issues. disabled for now.
//    if(true)return 1;
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	
	ILibWrapper_WebRTC_Connection_GetUserData(connection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );

	char address[255] = {0};
	int port = 0;
	
	// candidate:0 1 UDP 2128609534 0.0.0.0 0 typ host
	sscanf(candidate, "%*s %*d %*s %*d %s %d %*s %*s", address, &port);
	
	// todo, parse candidate into struct sockaddr_in6 and add to the offer...
	char* offer = ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToRemoteSDP(connection, address, port );
	::libwebrtc_set_answer(conn, offer );
	free(offer);
	
	return 1;
}

int libwebrtc_write( struct libwebrtc_data_channel* dc, const void* data, int len ) {
	ILibWrapper_WebRTC_DataChannel* channel = (ILibWrapper_WebRTC_DataChannel*)dc;
	
	ILibTransport_DoneState retval = ILibWrapper_WebRTC_DataChannel_Send(channel, (char*)(data), len);
	if (retval == ILibTransport_DoneState_ERROR) {
		return -1;
	}
	return len;
}

void libwebrtc_close_channel( struct libwebrtc_data_channel* channel ) {
	ILibWrapper_WebRTC_DataChannel* dataChannel = (ILibWrapper_WebRTC_DataChannel*)channel;
	
	void* user_data = dataChannel->userData;
	libwebrtc_context* ctx = NULL;
	//libwebrtc_connection* conn = (libwebrtc_connection*)dataChannel->parent;
	
	ILibWrapper_WebRTC_Connection_GetUserData(dataChannel->parent, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );

	// still unsure if this will trigger a channelClosed or not.
	ILibWrapper_WebRTC_DataChannel_Close(dataChannel);
	
	// uncomment this if its not triggered by above...
	//ctx->callback( ctx, conn, channel, LWRTC_CALLBACK_CHANNEL_CLOSED, user_data, 0, 0 );
}

void libwebrtc_close_connection( struct libwebrtc_connection* conn ) {
	ILibWrapper_WebRTC_Connection connection = (ILibWrapper_WebRTC_Connection)conn;
	
	void* user_data = NULL;
	libwebrtc_context* ctx = NULL;
	
	ILibWrapper_WebRTC_Connection_GetUserData(connection, (void**)&ctx, NULL, &user_data);
	assert( ctx != NULL );

	// this will trigger the callback for any existing channels.
	ILibWrapper_WebRTC_Connection_Disconnect(connection);
	// the connection object is now destroyed
}

#ifdef HUMBLENET_LOAD_WEBRTC
} // end namepsace
#endif

#endif
