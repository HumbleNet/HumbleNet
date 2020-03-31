#include "humblenet_p2p_internal.h"
#include "humblenet_alias.h"
#include "humblenet_events_internal.h"


namespace humblenet {
	ha_bool P2PSignalConnection::sendMessage(const uint8_t* buff, size_t length)
	{
		if (this->wsi == nullptr) {
			// this really shouldn't happen but apparently it can
			// if the game hasn't properly opened a signaling connection
			return false;
		}

		{
			HUMBLENET_UNGUARD();

			int ret = internal_write_socket(this->wsi, (const void*)buff, length );

			return ret == length;
		}
	}

	ha_bool P2PSignalConnection::processMsg(const HumblePeer::Message* msg)
	{
		auto msgType = msg->message_type();

		switch (msgType) {
			case HumblePeer::MessageType::P2POffer:
			{
				auto p2p = reinterpret_cast<const HumblePeer::P2POffer*>(msg->message());
				PeerId peer = static_cast<PeerId>(p2p->peerId());

				if (humbleNetState.pendingPeerConnectionsIn.find(peer) != humbleNetState.pendingPeerConnectionsIn.end()) {
					// error, already a pending incoming connection to this peer
					LOG("p2pSignalProcess: already a pending connection to peer %u\n", peer);
					return true;
				}

				bool emulated = ( p2p->flags() & 0x1 );

				if (emulated || !humbleNetState.webRTCSupported) {
					// webrtc connections not supported
					assert(humbleNetState.p2pConn);
					sendPeerRefused(humbleNetState.p2pConn.get(), peer);
				}

				Connection *connection = new Connection(Incoming);
				connection->status = HUMBLENET_CONNECTION_CONNECTING;
				connection->otherPeer = peer;
				{
					HUMBLENET_UNGUARD();
					connection->socket = internal_create_webrtc(humbleNetState.context);
				}
				internal_set_data( connection->socket, connection );

				humbleNetState.pendingPeerConnectionsIn.insert(std::make_pair(peer, connection));

				auto offer = p2p->offer();

				LOG("P2PConnect SDP got %u's offer = \"%s\"\n", peer, offer->c_str());
				int ret;
				{
					HUMBLENET_UNGUARD();
					ret = internal_set_offer( connection->socket, offer->c_str() );
				}
				if( ! ret )
				{
					humblenet_connection_close( connection );

					sendPeerRefused(humbleNetState.p2pConn.get(), peer);
				}
			}
				break;

			case HumblePeer::MessageType::P2PAnswer:
			{
				auto p2p = reinterpret_cast<const HumblePeer::P2PAnswer*>(msg->message());
				PeerId peer = static_cast<PeerId>(p2p->peerId());

				auto it = humbleNetState.pendingPeerConnectionsOut.find(peer);
				if (it == humbleNetState.pendingPeerConnectionsOut.end()) {
					LOG("P2PResponse for connection we didn't initiate: %u\n", peer);
					return true;
				}

				Connection *conn = it->second;
				assert(conn != NULL);
				assert(conn->otherPeer == peer);

				assert(conn->socket != NULL);
				// TODO: deal with _CLOSED
				assert(conn->status == HUMBLENET_CONNECTION_CONNECTING);

				auto offer = p2p->offer();

				LOG("P2PResponse SDP got %u's response offer = \"%s\"\n", peer, offer->c_str());
				int ret;
				{
					HUMBLENET_UNGUARD();
					ret = internal_set_answer( conn->socket, offer->c_str() );
				}
				if( !ret ) {
					humblenet_connection_set_closed( conn );
				}
			}
				break;

			case HumblePeer::MessageType::HelloClient:
			{
				auto hello = reinterpret_cast<const HumblePeer::HelloClient*>(msg->message());
				PeerId peer = static_cast<PeerId>(hello->peerId());

				if (humbleNetState.myPeerId != 0) {
					LOG("Error: got HelloClient but we already have a peer id\n");
					return true;
				}
				LOG("My peer id is %u\n", peer);
				humbleNetState.myPeerId = peer;

				humbleNetState.iceServers.clear();

				if (hello->iceServers()) {
					auto iceList = hello->iceServers();
					for (const auto& it : *iceList) {
						if (it->type() == HumblePeer::ICEServerType::STUNServer) {
							humbleNetState.iceServers.emplace_back(it->server()->str());
						} else if (it->type() == HumblePeer::ICEServerType::TURNServer) {
							auto username = it->username();
							auto pass = it->password();
							if (pass && username) {
								humbleNetState.iceServers.emplace_back(it->server()->str(), username->str(), pass->str());
							}
						}
					}
				} else {
					LOG("No STUN/TURN credentials provided by the server\n");
				}

				std::vector<const char*> stunServers;
				for (auto& it : humbleNetState.iceServers) {
					if (it.type == HumblePeer::ICEServerType::STUNServer) {
						stunServers.emplace_back(it.server.c_str());
					}
				}
				internal_set_stun_servers(humbleNetState.context, stunServers.data(), stunServers.size());
				//            internal_set_turn_server( server.c_str(), username.c_str(), password.c_str() );

				HumbleNet_Event event = {HUMBLENET_EVENT_P2P_CONNECTED};
				humblenet_event_push( event );
				event.type = HUMBLENET_EVENT_P2P_ASSIGN_PEER;
				event.peer.peer_id = humbleNetState.myPeerId;
				humblenet_event_push( event );
			}
				break;

			case HumblePeer::MessageType::ICECandidate:
			{
				auto iceCandidate = reinterpret_cast<const HumblePeer::ICECandidate*>(msg->message());
				PeerId peer = static_cast<PeerId>(iceCandidate->peerId());

				auto it = humbleNetState.pendingPeerConnectionsIn.find( peer );

				if( it == humbleNetState.pendingPeerConnectionsIn.end() )
				{
					it = humbleNetState.pendingPeerConnectionsOut.find( peer );
					if( it == humbleNetState.pendingPeerConnectionsOut.end() )
					{
						// no connection waiting on a peer.
						return true;
					}
				}

				if( it->second->socket && it->second->status == HUMBLENET_CONNECTION_CONNECTING ) {
					auto offer = iceCandidate->offer();
					LOG("Got ice candidate from peer: %d, %s\n", it->second->otherPeer, offer->c_str() );

					{
						HUMBLENET_UNGUARD();

						internal_add_ice_candidate( it->second->socket, offer->c_str() );
					}
				}
			}
				break;

			case HumblePeer::MessageType::P2PReject:
			{
				auto reject = reinterpret_cast<const HumblePeer::P2PReject*>(msg->message());
				PeerId peer = static_cast<PeerId>(reject->peerId());

				switch (reject->reason()) {
					case HumblePeer::P2PRejectReason::NotFound: {
						LOG( "Peer %u does not exist\n", peer );
						HumbleNet_Event event = {HUMBLENET_EVENT_PEER_NOT_FOUND};
						event.peer.peer_id = peer;
						humblenet_event_push( event );
					}
						break;
					case HumblePeer::P2PRejectReason::PeerRefused: {
						LOG( "Peer %u rejected our connection\n", peer );
						HumbleNet_Event event = {HUMBLENET_EVENT_PEER_REJECTED};
						event.peer.peer_id = peer;
						humblenet_event_push( event );
					}
						break;
				}

				auto it = humbleNetState.pendingPeerConnectionsOut.find(peer);
				if (it != humbleNetState.pendingPeerConnectionsOut.end()) {
					Connection *conn = it->second;
					assert(conn != NULL);

					blacklist_peer(peer);
					humblenet_connection_set_closed(conn);
				}
			}
				break;

			case HumblePeer::MessageType::P2PConnected:
			{
				auto connect = reinterpret_cast<const HumblePeer::P2PConnected*>(msg->message());

				LOG("Established connection to peer %u", connect->peerId());

				HumbleNet_Event event = {HUMBLENET_EVENT_PEER_CONNECTED};
				event.peer.peer_id = connect->peerId();
				humblenet_event_push( event );
			}
				break;

			case HumblePeer::MessageType::P2PDisconnect:
			{
				auto disconnect = reinterpret_cast<const HumblePeer::P2PDisconnect*>(msg->message());

				LOG("Disconnecting peer %u", disconnect->peerId());

				HumbleNet_Event event = {HUMBLENET_EVENT_PEER_DISCONNECTED};
				event.peer.peer_id = disconnect->peerId();
				humblenet_event_push( event );
			}
				break;

			case HumblePeer::MessageType::P2PRelayData:
			{
				auto relay = reinterpret_cast<const HumblePeer::P2PRelayData*>(msg->message());
				auto peer = relay->peerId();
				auto data = relay->data();

				LOG("Got %d bytes relayed from peer %u\n", data->Length(), peer );

				// Sequentially look for the other peer
				auto it = humbleNetState.connections.begin();
				for( ; it != humbleNetState.connections.end(); ++it ) {
					if( it->second->otherPeer == peer )
						break;
				}
				if( it == humbleNetState.connections.end() ) {
					LOG("Peer %u does not exist\n", peer);
				} else {
					Connection* conn = it->second;

					conn->recvBuffer.insert(conn->recvBuffer.end()
							, reinterpret_cast<const char *>(data->Data())
							, reinterpret_cast<const char *>(data->Data()) + data->Length());
					humbleNetState.pendingDataConnections.insert( conn );

					signal();
				}
			}
				break;
			// region Alias handling
			case HumblePeer::MessageType::AliasResolved:
			{
				auto resolved = reinterpret_cast<const HumblePeer::AliasResolved*>(msg->message());

				internal_alias_resolved_to( resolved->alias()->c_str(), resolved->peerId() );

				HumbleNet_Event event = {HUMBLENET_EVENT_ALIAS_RESOLVED};
				event.common.request_id = msg->requestId();
				if (resolved->peerId()) {
					event.peer.peer_id = resolved->peerId();
				} else {
					event.type = HUMBLENET_EVENT_ALIAS_NOT_FOUND;
#pragma message ("TODO provide better error message that maybe includes the hostname?")
					strcpy(event.error.error, "Failed to resolve alias");
				}
				humblenet_event_push(event);
			}
				break;
			case HumblePeer::MessageType::AliasRegisterError:
			{
				auto message = reinterpret_cast<const HumblePeer::Error*>(msg->message());

				HumbleNet_Event event = {HUMBLENET_EVENT_ALIAS_REGISTER_ERROR};
				event.common.request_id = msg->requestId();
#pragma message ("TODO provide better error/alo message including contents from peer server?")
				strcpy(event.error.error, "Failed to register alias");
				humblenet_event_push(event);
			}
				break;
			case HumblePeer::MessageType::AliasRegisterSuccess:
			{
				auto message = reinterpret_cast<const HumblePeer::Success*>(msg->message());

				HumbleNet_Event event = {HUMBLENET_EVENT_ALIAS_REGISTER_SUCCESS};
				event.common.request_id = msg->requestId();
				humblenet_event_push(event);
			}
				break;
			// endregion
			default:
				LOG("p2pSignalProcess unhandled %s\n", HumblePeer::EnumNameMessageType(msgType));
				break;
		}

		return true;
	}
}