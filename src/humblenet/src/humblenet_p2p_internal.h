#ifndef HUMBLENET_P2P_INTERNAL
#define HUMBLENET_P2P_INTERNAL

#include "humblenet.h"
#include "libsocket.h"

#include "humblenet_p2p_signaling.h"

#include <memory>

// TODO: should have a way to disable this on release builds
#define LOG printf
#define TRACE( ... )

enum InOrOut
{
	Incoming
	, Outgoing
};

// Connection status
typedef enum ConnectionStatus {
	HUMBLENET_CONNECTION_CLOSED,
	HUMBLENET_CONNECTION_CONNECTING,
	HUMBLENET_CONNECTION_CONNECTED,
} ConnectionStatus;

struct Connection {
	InOrOut inOrOut;
	ConnectionStatus status;

	// peerId of the other peer
	// 0 if not a p2p connection
	PeerId otherPeer;

	std::vector<char> recvBuffer;

	ha_bool writable;

	struct internal_socket_t* socket;

	Connection( InOrOut inOrOut_, struct internal_socket_t *s = NULL)
	: inOrOut(inOrOut_)
	, status(HUMBLENET_CONNECTION_CONNECTING)
	, otherPeer(0)
	, writable(true)
	, socket(NULL)
	{
	}
};

typedef struct HumbleNetState {
	// established connections indexed by socket id
	// pointer not owned
	std::unordered_map<internal_socket_t *, Connection *> connections;

	// new incoming connections waiting accept
	std::unordered_set<Connection *> pendingNewConnections;

	// connections with pending data in buffer
	// pointer not owned
	std::unordered_set<Connection *> pendingDataConnections;

	// connections which have been closed by remote and need user notification
	std::unordered_set<Connection *> remoteClosedConnections;

	// pending outgoing (not established) p2p connections
	// pointer not owned (owned by whoever called humblenet_connect_peer)
	std::unordered_map<PeerId, Connection *> pendingPeerConnectionsOut;

	// pending incoming P2P connections
	// these ARE owned (until humblenet_connection_accept which removes them)
	std::unordered_map<PeerId, Connection *> pendingPeerConnectionsIn;

	// outgoing pending (not established) alias connections
	// pointer not owned
	std::unordered_map<std::string, Connection *> pendingAliasConnectionsOut;

	// map of peers that are blacklisted, value is when they they were blacklisted
	// incoming peers are added to this list when they are disconnected.
	// this is to prevent anemic connection attempts.
	std::unordered_map<PeerId, uint64_t> peerBlacklist;

	PeerId myPeerId;

	std::unordered_map<LobbyId, humblenet::Lobby> lobbies;

	std::unique_ptr<humblenet::P2PSignalConnection> p2pConn;

	std::string signalingServerAddr;
	std::string gameToken;
	std::string gameSecret;
	std::string authToken;
	std::string reconnectToken;
	std::vector<humblenet::ICEServer> iceServers;

	ha_bool webRTCSupported;

	internal_context_t *context;

	HumbleNetState()
	:  myPeerId(0)
	, webRTCSupported(false)
	, context(NULL)
	{
	}
} HumbleNetState;

extern HumbleNetState humbleNetState;

#define INIT_GUARD( msg, check, ... ) \
if( ! (check) ) { humblenet_set_error( msg ); return __VA_ARGS__ ; }

// Initialize P2P system with websocket layer
ha_bool internal_p2p_register_protocol();

// internal...
void internal_poll_io();
void humblenet_connection_set_closed( Connection* conn );
bool is_peer_blacklisted( PeerId peer );
void blacklist_peer( PeerId peer );
void signal();

typedef void(*timer_callback_t)(void* data);
void humblenet_timer( timer_callback_t callback, int timeout, void* data);

void humblenet_lock();
void humblenet_unlock();

struct Guard {
    Guard() {
        humblenet_lock();
    }

    ~Guard() {
        humblenet_unlock();
    }
};

struct UnGuard {
    UnGuard() {
        humblenet_unlock();
    }

    ~UnGuard() {
        humblenet_lock();
    }
};


#ifdef EMSCRIPTEN
    #define HUMBLENET_GUARD()
    #define HUMBLENET_UNGUARD()
#else
    #define HUMBLENET_GUARD() Guard lock
#define HUMBLENET_UNGUARD() UnGuard unlock
#endif

/*
 * Shut down the P2P networking library
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_p2p_shutdown();

/*
 * Get connection status
 */
ConnectionStatus humblenet_connection_status(Connection *connection);


/*
 * Connect to server server_addr
 */
Connection *humblenet_connect_websocket(const char *server_addr);


/*
 * Connect to peer peer_id
 */
Connection *humblenet_connect_peer(PeerId peer_id);


/*
 * Close the Connection
 */
void humblenet_connection_close(Connection *connection);


/*
 * Returns true if connection has data to read, false otherwise
 */
ha_bool humblenet_connection_is_readable(Connection *connection);


/*
 * Returns true if connection can be written into, false otherwise
 */
ha_bool humblenet_connection_is_writable(Connection *connection);


/*
 * Send data through Connection
 * Returns the number of bytes sent, -1 on error
 */
int humblenet_connection_write(Connection *connection, const void *buf, uint32_t bufsize);


/*
 * Receive data through Connection
 * Returns the number of bytes received, -1 on error
 */
int humblenet_connection_read(Connection *connection, void *buf, uint32_t bufsize);


/*
 * Receive data through Connection
 * Returns a malloc() -allocated buffer
 * You are responsible for freeing it
 * sets *retval to number of bytes in buffer or negative on error
 */
void *humblenet_connection_read_all(Connection *connection, int *retval);


/*
 * Poll all connections
 * timeout_ms Timeout in milliseconds
 * 0 = return immediately if no pending connections
 * negative = wait indefinitely
 * returns a connection with readable data, NULL if no such connections
 */
Connection *humblenet_poll_all(int timeout_ms);


/*
 * Poll a single connection
 * connection to poll
 * timeout_ms Timeout in milliseconds
 * 0 = return immediately if no pending connections
 * negative = wait indefinitely
 * returns true if readable
 */
ha_bool humblenet_connection_poll(Connection *connection, int timeout_ms);


/*
 * Accepts new connections
 * returns a new Connection (which you now own) or NULL if no new connections
 * Humblenet will include this connection in future humblenet_connection_poll calls
 */
Connection *humblenet_connection_accept();


/*
 * Get the PeerId of this connection
 * returns PeerId or 0 if not a p2p connection
 */
PeerId humblenet_connection_get_peer_id(Connection *connection);

uint64_t sys_milliseconds();

#endif // HUMBLENET_P2P_INTERNAL
