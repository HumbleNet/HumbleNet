#include "humblenet.h"
#include "humblenet_p2p.h"
#include "humblenet_p2p_internal.h"
#include "humblenet_alias.h"

#define NOMINMAX

#include <cassert>

#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#if defined(WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
	#include <sys/time.h>
#endif

#if defined(EMSCRIPTEN)
	#include <emscripten/emscripten.h>
#endif

#include "humblepeer.h"

#include "libsocket.h"

#include "humblenet_p2p_internal.h"
#include "humblenet_utils.h"

#define USE_STUN

HumbleNetState humbleNetState;

#ifdef WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
uint64_t sys_milliseconds(void)
{
	FILETIME ft;
	ULARGE_INTEGER temp;

	GetSystemTimeAsFileTime(&ft);
	memcpy(&temp, &ft, sizeof(temp));

	return (temp.QuadPart - DELTA_EPOCH_IN_MICROSECS) / 100000;
}

#else
uint64_t sys_milliseconds (void)
{
	struct timeval tp;
	struct timezone tzp;
	static uint64_t	secbase = 0;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	return (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
}
#endif

void signal();

void blacklist_peer( PeerId peer ) {
	humbleNetState.peerBlacklist.insert( std::make_pair( peer, sys_milliseconds() + 5000 ) );
}

/*
 * Deterimine if the peer is currently blacklisted.
 *
 * Side effect: will clear expired blacklist entries.
 */
bool is_peer_blacklisted( PeerId peer_id ) {
	auto it = humbleNetState.peerBlacklist.find(peer_id);
	if( it == humbleNetState.peerBlacklist.end() )
		return false;
	
	if( it->second <= sys_milliseconds() ) {
		humbleNetState.peerBlacklist.erase(it);
		return true;
	}

	return true;
}

/*
 * See if we can try a connection to this peer
 *
 * Side effect: sets error to reason on failure
 */
bool can_try_peer( PeerId peer_id ) {
	if (!humbleNetState.p2pConn) {
		humblenet_set_error("Signaling connection not established");
		// no signaling connection
		return false;
	}
	if (humbleNetState.myPeerId == 0) {
		humblenet_set_error("No peer ID from server");
		return false;
	}
	if (humbleNetState.pendingPeerConnectionsOut.find(peer_id)
		!= humbleNetState.pendingPeerConnectionsOut.end()) {
		// error, already a pending outgoing connection to this peer
		humblenet_set_error("already a pending connection to peer");
		LOG("humblenet_connect_peer: already a pending connection to peer %u\n", peer_id);
		return false;
	}
	if( is_peer_blacklisted(peer_id) ) {
		// peer is currently black listed
		humblenet_set_error("peer blacklisted");
		LOG("humblenet_connect_peer: peer blacklisted %u\n", peer_id);
		return false;
	}
	
	return true;
}


// BEGIN CONNECTION HANDLING

void humblenet_connection_set_closed( Connection* conn ) {
	if( conn->socket ) {
		// clear our state first so callbacks dont work on us.
		humbleNetState.connections.erase(conn->socket);
		internal_set_data(conn->socket, NULL );
		{
			HUMBLENET_UNGUARD();
			internal_close_socket(conn->socket);
		}
		conn->socket = NULL;
	}

	if( conn->inOrOut == Incoming ) {
		blacklist_peer( conn->otherPeer );
	}
	
	conn->status = HUMBLENET_CONNECTION_CLOSED;

	LOG("Marking connections closed: %u\n", conn->otherPeer );

	// make sure were not in any lists...
	erase_value( humbleNetState.connections, conn );
	humbleNetState.pendingNewConnections.erase( conn );
	humbleNetState.pendingDataConnections.erase( conn );
//	humbleNetState.remoteClosedConnections.erase( conn );
	erase_value( humbleNetState.pendingPeerConnectionsOut, conn );
	erase_value( humbleNetState.pendingPeerConnectionsIn, conn );
	erase_value( humbleNetState.pendingAliasConnectionsOut, conn );

	// mark it as being close pending.
	humbleNetState.remoteClosedConnections.insert( conn );

	signal();
}


ha_bool humblenet_connection_is_readable(Connection *connection) {
	return !connection->recvBuffer.empty();
}


ha_bool humblenet_connection_is_writable(Connection *connection) {
	assert(connection != NULL);
	if (connection->status == HUMBLENET_CONNECTION_CONNECTED) {
		return connection->writable;
	}
	return false;
}


int humblenet_connection_write(Connection *connection, const void *buf, uint32_t bufsize) {
	assert(connection != NULL);

	switch (connection->status) {
		case HUMBLENET_CONNECTION_CLOSED:
			// connection has been closed
			assert(connection->socket == NULL);
			return -1;

		case HUMBLENET_CONNECTION_CONNECTING:
			assert(connection->socket != NULL);
			return 0;

		case HUMBLENET_CONNECTION_CONNECTED:
			assert(connection->socket != NULL);

			const char* use_relay = humblenet_get_hint("p2p_use_relay");
			if( use_relay && *use_relay == '1' ) {
				if( ! sendP2PRelayData( humbleNetState.p2pConn.get(), connection->otherPeer, buf, bufsize ) ) {
					return -1;
				}
				return bufsize;
			}
		{
			HUMBLENET_UNGUARD();
			return internal_write_socket( connection->socket, buf, bufsize );
		}
	}
	return -1;
}


int humblenet_connection_read(Connection *connection, void *buf, uint32_t bufsize) {
	assert(connection != NULL);

	switch (connection->status) {
		case HUMBLENET_CONNECTION_CLOSED:
			// connection has been closed
			assert(connection->socket == NULL);
			return -1;

		case HUMBLENET_CONNECTION_CONNECTING:
			assert(connection->socket != NULL);
			return 0;

		case HUMBLENET_CONNECTION_CONNECTED:
			assert(connection->socket != NULL);

			if( connection->recvBuffer.empty() )
				return 0;

			bufsize = std::min<uint32_t>(bufsize, connection->recvBuffer.size() );
			memcpy(buf, &connection->recvBuffer[0], bufsize);
			connection->recvBuffer.erase(connection->recvBuffer.begin()
										 , connection->recvBuffer.begin() + bufsize);

			if( ! connection->recvBuffer.empty() )
				humbleNetState.pendingDataConnections.insert(connection);
			else
				humbleNetState.pendingDataConnections.erase(connection);

			return bufsize;
	}
	return -1;
}

ConnectionStatus humblenet_connection_status(Connection *connection) {
	assert(connection != NULL);

	return connection->status;
}

Connection *humblenet_connect_websocket(const char *server_addr) {
	humblenet_set_error("Websocket support deprecated");
	return NULL;
}


Connection *humblenet_connect_peer(PeerId peer_id) {
	if( ! can_try_peer( peer_id ) )
		return NULL;

	Connection *connection = new Connection(Outgoing);
	connection->otherPeer = peer_id;
	{
		HUMBLENET_UNGUARD();
		connection->socket = internal_create_webrtc(humbleNetState.context);
	}
	internal_set_data(connection->socket, connection);

	humbleNetState.pendingPeerConnectionsOut.emplace(peer_id, connection);

	int ret;

	{
		HUMBLENET_UNGUARD();
		ret = internal_create_offer( connection->socket );
	}

	if( ! ret ) {
		LOG("Unable to generate sdp offer to peer: %d\n", peer_id);
		humblenet_set_error("Unable to generate sdp offer");
		humblenet_connection_close(connection);
		return NULL;
	}

	LOG("connecting to peer: %d\n", peer_id);
	return connection;
}


void humblenet_connection_close(Connection *connection) {
	assert(connection != NULL);

	humblenet_connection_set_closed(connection);

	internal_alias_remove_connection( connection );

	humbleNetState.remoteClosedConnections.erase(connection);

	delete connection;
}

PeerId humblenet_connection_get_peer_id(Connection *connection) {
	assert(connection != NULL);

	return connection->otherPeer;
}


// called to return the sdp offer
int on_sdp( internal_socket_t* s, const char* offer, void* user_data ) {
	HUMBLENET_GUARD();

	Connection* conn = reinterpret_cast<Connection*>(user_data);

	if( conn == NULL ) {
		LOG("on_sdp: Got socket w/o state?\n");
		return -1;
	}

	assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );

	// no trickle ICE
	int flags = 0x2;

	if( conn->inOrOut == Incoming ) {
		LOG("P2PConnect SDP sent %u response offer = \"%s\"\n", conn->otherPeer, offer);
		if( ! sendP2PResponse(humbleNetState.p2pConn.get(), conn->otherPeer, offer) ) {
			return -1;
		}
	} else {
		LOG("outgoing SDP sent %u offer: \"%s\"\n", conn->otherPeer, offer);
		if( ! sendP2PConnect(humbleNetState.p2pConn.get(), conn->otherPeer, flags, offer) ) {
			return -1;
		}
	}
	return 0;
}

// called to send a candidate
int on_ice_candidate( internal_socket_t* s, const char* offer, void* user_data ) {
	HUMBLENET_GUARD();

	Connection* conn = reinterpret_cast<Connection*>(user_data);
	
	if( conn == NULL ) {
		LOG("on_ice: Got socket w/o state?\n");
		return -1;
	}

	assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );

	LOG("Sending ice candidate to peer: %u, %s\n", conn->otherPeer, offer );
	if( ! sendICECandidate(humbleNetState.p2pConn.get(), conn->otherPeer, offer) ) {
		return -1;
	}

	return 0;
}

// called for incomming connections to indicate the connection process is completed.
int on_accept (internal_socket_t* s, void* user_data) {
	HUMBLENET_GUARD();

	auto it = humbleNetState.connections.find( s );

	// TODO: Rework this ?
	Connection* conn = NULL;
	if( it == humbleNetState.connections.end() ) {
		// its a websocket connection
		conn = new Connection( Incoming, s );

		// track the connection, ALL connections will reside here.
		humbleNetState.connections.insert( std::make_pair( s, conn ) );
	} else {
		// should be a webrtrc connection (as we create the socket inorder to start the process)
		conn = it->second;
	}

	// TODO: This should be "waiting for accept"
	conn->status = HUMBLENET_CONNECTION_CONNECTED;

	// expose it as an incomming connection to be accepted...
	humbleNetState.pendingNewConnections.insert( conn );

	LOG("accepted peer: %d\n", conn->otherPeer );

	signal();

	return 0;
}

// called for outgoing connections to indicate the connection process is completed.
int on_connect (internal_socket_t* s, void* user_data) {
	HUMBLENET_GUARD();

	assert( user_data );

	auto it = humbleNetState.connections.find( s );

	// TODO: Rework this ?
	Connection* conn = reinterpret_cast<Connection*>(user_data);
	if( it == humbleNetState.connections.end() ) {
		// track the connection, ALL connections will reside here.
		humbleNetState.connections.insert( std::make_pair( s, conn ) );
	}

	assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );

	// outgoing always initiates the channel
	if( conn->inOrOut == Outgoing ) {
		if( ! internal_create_channel(s, "dataChannel") )
			return -1;
	}

	LOG("connected peer: %d\n", conn->otherPeer );

	return 0;
}

int on_accept_channel( internal_socket_t* s, const char* name, void* user_data ) {
	HUMBLENET_GUARD();

	assert( user_data );

	Connection* conn = reinterpret_cast<Connection*>(user_data);

	assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
	conn->status = HUMBLENET_CONNECTION_CONNECTED;

	LOG("accepted channel: %d:%s\n", conn->otherPeer, name );

	signal();

	return 0;
}

int on_connect_channel( internal_socket_t* s, const char* name, void* user_data ) {
	HUMBLENET_GUARD();

	assert( user_data );

	Connection* conn = reinterpret_cast<Connection*>(user_data);

	assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
	conn->status = HUMBLENET_CONNECTION_CONNECTED;

	LOG("connected channel: %d:%s\n", conn->otherPeer, name );
	return 0;
}

// called each time data is received.
int on_data( internal_socket_t* s, const void* data, int len, void* user_data ) {
	HUMBLENET_GUARD();

	// already disconnected from this socket.
	if( ! user_data )
		return -1;

	Connection* conn = reinterpret_cast<Connection*>(user_data);

	assert( conn->status == HUMBLENET_CONNECTION_CONNECTED );

	LOG("Received %d from peer %u\n", len, conn->otherPeer);

	if( conn->recvBuffer.empty() ) {
		assert( humbleNetState.pendingDataConnections.find(conn) == humbleNetState.pendingDataConnections.end() );
		humbleNetState.pendingDataConnections.insert(conn);
	}

	conn->recvBuffer.insert( conn->recvBuffer.end(), reinterpret_cast<const char*>(data),
							reinterpret_cast<const char*>(data)+len);

	signal();

	return 0;
}

// called to indicate the connection is wriable.
int on_writable (internal_socket_t* s, void* user_data) {
	HUMBLENET_GUARD();

	auto it = humbleNetState.connections.find( s );
	
	if( it != humbleNetState.connections.end() ) {
		// its not here anymore
		return -1;
	}

	Connection* conn = it->second;
	conn->writable = true;

	return 0;
}

// called when the conenction is terminated (from either side)
int on_disconnect( internal_socket_t* s, void* user_data ) {
	HUMBLENET_GUARD();

	// find Connection object
	auto it = humbleNetState.connections.find(s);

	if (it != humbleNetState.connections.end()) {
		// it's still in connections map, not user-initiated close
		// find Connection object and set wsi to NULL
		// to signal that it's dead
		// user code will then call humblenet_close to dispose of
		// the Connection object

		humblenet_connection_set_closed( it->second );
	} else if( user_data != NULL ) {
		// just to be sure...
		humblenet_connection_set_closed( reinterpret_cast<Connection*>( user_data ) );
	}
	// if it wasn't in the table, no need to do anything
	// humblenet_close has already disposed of everything else
	return 0;
}

int on_destroy( internal_socket_t* s, void* user_data ) {
	HUMBLENET_GUARD();

	// if no user_data attached, then nothing to cleanup.
	if( ! user_data )
		return 0;

	// make sure we are fully detached...
	Connection* conn = reinterpret_cast<Connection*>(user_data);
	humblenet_connection_set_closed( conn );

	return 0;
}

// END CONNECTION HANDLING

// BEGIN INITIALIZATION

uint32_t HUMBLENET_CALL humblenet_version() {
	return HUMBLENET_COMPILEDVERSION;
}

ha_bool HUMBLENET_CALL humblenet_init() {
	return true;
}

ha_bool internal_p2p_register_protocol() {
	internal_callbacks_t callbacks;

	memset(&callbacks, 0, sizeof(callbacks));

	callbacks.on_sdp = on_sdp;
	callbacks.on_ice_candidate = on_ice_candidate;
	callbacks.on_accept = on_accept;
	callbacks.on_connect = on_connect;
	callbacks.on_accept_channel = on_accept_channel;
	callbacks.on_connect_channel = on_connect_channel;
	callbacks.on_data = on_data;
	callbacks.on_disconnect = on_disconnect;
	callbacks.on_writable = on_writable;

	humbleNetState.context = internal_init(  &callbacks );
	if (humbleNetState.context == NULL) {
		return false;
	}

	humblenet::register_protocol(humbleNetState.context);

	humbleNetState.webRTCSupported = internal_supports_webRTC( humbleNetState.context );

	return true;
}


#ifndef EMSCRIPTEN
extern "C" void poll_deinit();
#endif

void HUMBLENET_CALL humblenet_shutdown() {
	{
		HUMBLENET_GUARD();

		// Destroy all connections
		for( auto it = humbleNetState.connections.begin(); it != humbleNetState.connections.end(); ) {
			Connection* conn = it->second;
			++it;
			humblenet_connection_close( conn );
		}
	}

	humblenet_p2p_shutdown();

#ifndef EMSCRIPTEN
	poll_deinit();
#endif

}

// END INITIALIZATION

// BEGIN MISC

ha_bool HUMBLENET_CALL humblenet_p2p_supported() {
	// This function should really be "humblenet_has_webrtc"
	// but could be removed entirely from the API
	return humbleNetState.webRTCSupported;
}

#if defined(WIN32)
#define THREAD_LOCAL __declspec(thread)
#else // Everyone else
#define THREAD_LOCAL __thread
#endif

THREAD_LOCAL const char* errorString = nullptr;

const char * HUMBLENET_CALL humblenet_get_error() {
	return errorString;
}


void HUMBLENET_CALL humblenet_set_error(const char *error) {
	errorString = error;
}


void HUMBLENET_CALL humblenet_clear_error() {
	errorString = nullptr;
}

// END MISC

// BEGIN DEPRECATED

void *humblenet_connection_read_all(Connection *connection, int *retval) {
	assert(connection != NULL);

	if (connection->status == HUMBLENET_CONNECTION_CONNECTING) {
		// not open yet
		return 0;
	}

	if (connection->recvBuffer.empty()) {
		// must not be in poll-returnable connections anymore
		assert(humbleNetState.pendingDataConnections.find(connection) == humbleNetState.pendingDataConnections.end());

		if (connection->status == HUMBLENET_CONNECTION_CLOSED) {
			// TODO: Connection should contain reason it was closed and we should report that
			humblenet_set_error("Connection closed");
			*retval = -1;
			return NULL;
		}

		*retval = 0;
		return NULL;
	}

	size_t dataSize = connection->recvBuffer.size();
	void *buf = malloc(dataSize);

	if (buf == NULL) {
		humblenet_set_error("Memory allocation failed");
		*retval = -1;
		return NULL;
	}

	memcpy(buf, &connection->recvBuffer[0], dataSize);
	connection->recvBuffer.clear();

	auto it = humbleNetState.pendingDataConnections.find(connection);
	assert(it != humbleNetState.pendingDataConnections.end());
	humbleNetState.pendingDataConnections.erase(it);

	*retval = dataSize;
	return buf;
}


Connection *humblenet_poll_all(int timeout_ms) {
	{
		// check for connections closed by remote and notify user
		auto it = humbleNetState.remoteClosedConnections.begin();
		if (it != humbleNetState.remoteClosedConnections.end()) {
			Connection *conn = *it;
			humbleNetState.remoteClosedConnections.erase(it);
			return conn;
		}

		it = humbleNetState.pendingDataConnections.begin();
		if (it != humbleNetState.pendingDataConnections.end()) {
			// don't remove it from the set, _recv will do that
			Connection *conn = *it;
			assert(conn != NULL);
			assert(!conn->recvBuffer.empty());
			return conn;
		}
	}

	// call service if no outstanding data on any connection
	internal_poll_io(/*timeout_ms*/);

	{
		auto it = humbleNetState.pendingDataConnections.begin();
		if (it != humbleNetState.pendingDataConnections.end()) {
			// don't remove it from the set, _recv will do that
			Connection *conn = *it;
			assert(conn != NULL);
			assert(!conn->recvBuffer.empty());
			return conn;
		}
	}

	return NULL;
}

Connection *humblenet_connection_accept() {
	{
		auto it = humbleNetState.pendingNewConnections.begin();

		if (it != humbleNetState.pendingNewConnections.end()) {
			Connection *conn = *it;
			humbleNetState.pendingNewConnections.erase(it);
			return conn;
		}
	}

	// call service so something happens
	// TODO: if we replace poll loop with our own we could poll only the master socket
	internal_poll_io();

	{
		auto it = humbleNetState.pendingNewConnections.begin();
		if (it != humbleNetState.pendingNewConnections.end()) {
			Connection *conn = *it;
			humbleNetState.pendingNewConnections.erase(it);
			return conn;
		}
	}

	return NULL;
}

// END DEPRECATED

static std::unordered_map<std::string, std::string> hints;

/*
 * Set the value of a hint
 */
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_set_hint(const char* name, const char* value) {
    HUMBLENET_GUARD();

	auto it = hints.find( name );
	if( it != hints.end() )
		it->second = value;
	else
		hints.insert( std::make_pair( name, value ) );
	return 1;
}

/*
 * Get the value of a hint
 */
HUMBLENET_API const char* HUMBLENET_CALL humblenet_get_hint(const char* name) {
	HUMBLENET_GUARD();

	auto it = hints.find( name );
	if( it != hints.end() )
		return it->second.c_str();
	else
		return NULL;
}

#ifndef EMSCRIPTEN

#include "libpoll.h"	// SKIP_AMALGAMATOR_INCLUDE

void humblenet_lock() {
	poll_lock();
}

void humblenet_unlock() {
	poll_unlock();
}

void signal() {
	poll_interrupt();
}

void humblenet_timer( timer_callback_t callback, int timeout, void* data)
{
	poll_timeout( callback, timeout, data );
}

#else

void humblenet_lock() {
}

void humblenet_unlock() {
}

void signal () {
}

void humblenet_timer( timer_callback_t callback, int timeout, void* data)
{
	EM_ASM_({
		window.setTimeout( function(){
			Runtime.dynCall('vi', $0, [$2]);
		}, $1);

	}, callback, timeout, data );
}

#endif


