
#include "humblenet_p2p_internal.h"
#include "humblenet_utils.h"

#include <cassert>
#include <map>

#define VIRTUAL_PEER 0x80000000

static BidiractionalMap<PeerId, std::string> virtualPeerNames;
static BidiractionalMap<PeerId, Connection*> virtualPeerConnections;

static PeerId nextVirtualPeer = 0;

static std::string virtualName;

ha_bool internal_alias_register( const char* name ) {
	if( !name || !name[0] ) {
		humblenet_set_error("No name or empty name provided");
		return 0;
	}

	return humblenet::sendAliasRegister(humbleNetState.p2pConn.get(), name);
}

ha_bool internal_alias_unregister( const char* name ) {
	if (name && !name[0] ) {
		humblenet_set_error("Empty name provided");
		return 0;
	}

	if (!name) name = "";

	return humblenet::sendAliasUnregister(humbleNetState.p2pConn.get(), name);
}

PeerId internal_alias_lookup( const char* name ) {
	auto it = virtualPeerNames.find( name );
	if( ! virtualPeerNames.is_end(it) ) {
		return it->second;
	}

	// allocate a new VPeerId
	PeerId vpeer = (++nextVirtualPeer) | VIRTUAL_PEER;
	virtualPeerNames.insert( vpeer, name );

	return vpeer;
}

void internal_alias_resolved_to( const std::string& alias, PeerId peer ) {
	Connection* connection = NULL;

	auto it = humbleNetState.pendingAliasConnectionsOut.find(alias);
	if (it == humbleNetState.pendingAliasConnectionsOut.end()) {
		LOG("Got resolve message for alias \"%s\" which we're not connecting to\n", alias.c_str());
		return;
	} else {
		connection = it->second;
		humbleNetState.pendingAliasConnectionsOut.erase(it);
	}

	assert(connection != NULL);

	if( peer == 0 ) {
		LOG("AliasError: unable to resolve \"%s\"\n", alias.c_str());
		
		humblenet_connection_set_closed( connection );
		return;
	}

	connection->otherPeer = peer;

	humbleNetState.pendingPeerConnectionsOut.emplace(peer, connection);

	if( is_peer_blacklisted(peer) ) {
		humblenet_set_error("peer blacklisted");
		LOG("humblenet_connect_peer: peer blacklisted %u\n", peer);

		humblenet_connection_set_closed(connection);
	} else {
		int ret;
		{
			HUMBLENET_UNGUARD();
			connection->socket = internal_create_webrtc(humbleNetState.context);
			internal_set_data( connection->socket, connection);

			ret = internal_create_offer(connection->socket );
		}

		if( !ret) {
			LOG("Unable to create offer, aborting connection to alias\n");
			humblenet_connection_set_closed( connection );
		}
	}

}

ha_bool internal_alias_is_virtual_peer( PeerId peer ) {
	return (peer & VIRTUAL_PEER) == VIRTUAL_PEER;
}

PeerId internal_alias_get_virtual_peer( Connection* conn ) {
	auto it = virtualPeerConnections.find( conn );
	if( virtualPeerConnections.is_end(it) )
		return 0;
	else
		return it->second;
}

Connection* internal_alias_find_connection( PeerId peer ) {
	if( ! internal_alias_is_virtual_peer(peer) ) {
		return NULL;
	}

	// resolve the VPeer
	auto it = virtualPeerConnections.find( peer );
	if( virtualPeerConnections.is_end(it) ) {
		return NULL;
	} else if( humblenet_connection_status( it->second ) == HUMBLENET_CONNECTION_CLOSED ) {
		virtualPeerConnections.erase( it );
		return NULL;
	} else {
		return it->second;
	}
}

Connection* internal_alias_create_connection( PeerId peer ) {
	Connection* conn = internal_alias_find_connection( peer );
	if( conn == NULL ) {
		auto nit = virtualPeerNames.find( peer );
		if( virtualPeerNames.is_end( nit ) ) {
			// Invalid VPeerId !!!
			humblenet_set_error("Not a valid VPeerId");
			return NULL;
		}

		std::string name = nit->second;

		if (!sendAliasLookup(humbleNetState.p2pConn.get(), name)) {
			return NULL;
		}

		conn = new Connection(Outgoing);

		humbleNetState.pendingAliasConnectionsOut.emplace(name, conn);

		LOG("Establishing a connection to \"%s\"...\n", nit->second.c_str() );

		virtualPeerConnections.insert( peer, conn );
	}

	return conn;
}

void internal_alias_remove_connection( Connection* conn ) {
	virtualPeerConnections.erase( conn );
}


