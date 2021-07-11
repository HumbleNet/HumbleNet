#include "p2p_connection.h"

#include "logging.h"
#include "game.h"
#include "server.h"

#include "humblenet_utils.h"


namespace humblenet {
	ha_bool P2PSignalConnection::processMsg(const HumblePeer::Message* msg)
	{
		auto msgType = msg->message_type();

		// we don't accept anything but HelloServer from a peer which hasn't sent one yet
		if (!this->peerId) {
			if (msgType != HumblePeer::MessageType::HelloServer) {
				LOG_WARNING( "Got non-HelloServer message (%s) from non-authenticated peer \"%s\"\n",
							 HumblePeer::EnumNameMessageType( msgType ), this->url.c_str());
				// TODO: send error message
				return false;
			}
		}

		switch (msgType) {
			// region P2P offer handling
			case HumblePeer::MessageType::P2POffer: {
				auto p2p = reinterpret_cast<const HumblePeer::P2POffer*>(msg->message());
				auto peer = p2p->peerId();

				// look up target peer, send message on
				// TODO: should ensure there's not already a negotiation going

				bool emulated = (p2p->flags() & 0x1);

				LOG_INFO( "P2POffer from peer %u (%s) to peer %u\n", this->peerId, url.c_str(), peer );
				if (emulated) {
					LOG_INFO( ", emulated connections not allowed\n" );
					// TODO: better error message
					sendNoSuchPeer( this, peer );
					return true;
				}

				auto it = game->peers.find( peer );
				if (it == game->peers.end()) {
					LOG_WARNING( ", no such peer\n" );
					sendNoSuchPeer( this, peer );
				} else {
					P2PSignalConnection* otherPeer = it->second;
					assert( otherPeer != NULL );

					// if not emulated check if otherPeer supports WebRTC
					// to avoid unnecessary round trip
					if (!emulated && !otherPeer->webRTCsupport) {
						// webrtc connections not supported
						LOG_INFO( "(%s), refusing because target doesn't support WebRTC\n", otherPeer->url.c_str());
						sendPeerRefused( this, peer );
						return true;
					}

					LOG_INFO( "(%s)\n", otherPeer->url.c_str());

					// TODO: should check that it doesn't exist already
					this->connectedPeers.insert( otherPeer );

					// set peer id to originator so target knows who wants to connect
					sendP2POffer( otherPeer, this->peerId, p2p->flags(), p2p->offer()->c_str());
				}

			}
				break;

			case HumblePeer::MessageType::P2PAnswer: {
				auto p2p = reinterpret_cast<const HumblePeer::P2PAnswer*>(msg->message());
				auto peer = p2p->peerId();

				// look up target peer, send message on
				// TODO: should only send if peers are currently negotiating connection

				auto it = game->peers.find( peer );

				if (it == game->peers.end()) {
					LOG_WARNING( "P2PResponse from peer %u (%s) to nonexistent peer %u\n", this->peerId,
								 this->url.c_str(), peer );
					sendNoSuchPeer( this, peer );
					return true;
				} else {
					P2PSignalConnection* otherPeer = it->second;
					assert( otherPeer != NULL );
					assert( otherPeer->peerId == peer );

					bool otherPeerConnected = (otherPeer->connectedPeers.find( this ) !=
											   otherPeer->connectedPeers.end());
					if (!otherPeerConnected) {
						// we got P2PResponse but there's been no P2PConnect from
						// the peer we're supposed to respond to
						// client is either confused or malicious
						LOG_WARNING(
								"P2PResponse from peer %u (%s) to peer %u (%s) who has not requested a P2P connection\n",
								this->peerId, this->url.c_str(), otherPeer->peerId, otherPeer->url.c_str());
						sendNoSuchPeer( this, peer );
						return true;
					}

					// TODO: should assert it's not there yet
					this->connectedPeers.insert( otherPeer );

					// set peer id to originator so target knows who wants to connect
					sendP2PResponse( otherPeer, this->peerId, p2p->offer()->c_str());
				}
			}
				break;

			case HumblePeer::MessageType::ICECandidate: {
				auto p2p = reinterpret_cast<const HumblePeer::ICECandidate*>(msg->message());
				auto peer = p2p->peerId();

				// look up target peer, send message on
				// TODO: should only send if peers are currently negotiating connection

				auto it = game->peers.find( peer );

				if (it == game->peers.end()) {
					LOG_WARNING( "ICE Candidate: no such peer\n" );
					sendNoSuchPeer( this, peer );
				} else {
					P2PSignalConnection* otherPeer = it->second;
					assert( otherPeer != NULL );
					sendICECandidate( otherPeer, this->peerId, p2p->offer()->c_str());
				}

			}
				break;

			case HumblePeer::MessageType::P2PReject: {
				auto p2p = reinterpret_cast<const HumblePeer::P2PReject*>(msg->message());
				auto peer = p2p->peerId();

				auto it = game->peers.find( peer );
				// TODO: check there's such a connection attempt ongoing
				if (it == game->peers.end()) {
					switch (p2p->reason()) {
						case HumblePeer::P2PRejectReason::PeerRefused:
							LOG_WARNING( "Peer %u (%s) tried to refuse connection from nonexistent peer %u\n", peerId,
										 url.c_str(), peer );
							break;
						case HumblePeer::P2PRejectReason::NotFound:
							LOG_WARNING( "Peer %u (%s) sent unexpected NotFound to non existent peer %u", peerId,
										 url.c_str(), peer );
							break;
					}
					break;
				} else {
					P2PSignalConnection* otherPeer = it->second;
					assert( otherPeer != NULL );
					switch (p2p->reason()) {
						case HumblePeer::P2PRejectReason::PeerRefused:
							LOG_INFO( "Peer %u (%s) refused connection from peer %u (%s)\n", this->peerId,
									  this->url.c_str(), otherPeer->peerId, otherPeer->url.c_str());
							sendPeerRefused( otherPeer, this->peerId );
							break;
						case HumblePeer::P2PRejectReason::NotFound:
							LOG_WARNING( "Peer %u (%s) setnt unexpected NotFound from peer %u (%s)\n", this->peerId,
										 this->url.c_str(), otherPeer->peerId, otherPeer->url.c_str());
							sendPeerRefused( otherPeer, this->peerId );
							break;
					}
				}
			}
				break;
				// endregion

				// region Hello
			case HumblePeer::MessageType::HelloServer: {
				auto hello = reinterpret_cast<const HumblePeer::HelloServer*>(msg->message());

				if (peerId != 0) {
					LOG_ERROR( "Got hello HelloServer from client which already has a peer ID (%u)\n", peerId );
					return true;
				}

				if ((hello->flags() & 0x01) == 0) {
					LOG_ERROR( "Client %s does not support WebRTC\n", url.c_str());
					return true;
				}

#pragma message ("TODO handle user authentication")

				this->game = peerServer->getVerifiedGame( hello );
				if (this->game == NULL) {
					// invalid game
					return false;
				}

				auto attributes = hello->attributes();
				const flatbuffers::String* platform = nullptr;
				if (attributes) {
					auto platform_lookup = attributes->LookupByKey( "platform" );
					if (platform_lookup) {
						platform = platform_lookup->value();
					}
				}

				// generate peer id for this peer
				peerId = this->game->generateNewPeerId();
				LOG_INFO( "Got hello from \"%s\" (peer %u, game %u, platform: %s)\n", url.c_str(), peerId,
						  this->game->gameId, platform ? platform->c_str() : "" );

				game->peers.insert( std::make_pair( peerId, this ));
				this->webRTCsupport = true;
				this->trickleICE = !(hello->flags() & 0x2);

				// send STUN/TURN server credential if client supports webrtc
				// ';' separator between server, username and password, like this:
				// "server;username;password"
				std::vector<ICEServer> iceServers;
				peerServer->populateStunServers( iceServers );

				// send hello to client
				std::string reconnectToken = "";
#pragma message ("TODO implement reconnect tokens")
				sendHelloClient( this, peerId, reconnectToken, iceServers );
			}
				break;

			case HumblePeer::MessageType::HelloClient:
				// TODO: log address
				LOG_ERROR( "Got hello HelloClient, not supposed to happen\n" );
				break;
				// endregion

				// region P2P connect/disconnect (unused)
			case HumblePeer::MessageType::P2PConnected:
				// TODO: log address
				LOG_INFO( "P2PConnected from peer %u\n", peerId );
				// TODO: clean up ongoing negotiations lit
				break;
			case HumblePeer::MessageType::P2PDisconnect:
				LOG_INFO( "P2PDisconnect from peer %u\n", peerId );
				break;
				// endregion

				// region P2P relay
			case HumblePeer::MessageType::P2PRelayData: {
				auto relay = reinterpret_cast<const HumblePeer::P2PRelayData*>(msg->message());
				auto peer = relay->peerId();
				auto data = relay->data();

				LOG_INFO( "P2PRelayData relaying %d bytes from peer %u to %u\n", data->Length(), this->peerId, peer );

				auto it = game->peers.find( peer );

				if (it == game->peers.end()) {
					LOG_WARNING( ", no such peer\n" );
					sendNoSuchPeer( this, peer );
				} else {
					P2PSignalConnection* otherPeer = it->second;
					assert( otherPeer != NULL );
					sendP2PRelayData( otherPeer, this->peerId, data->Data(), data->Length());
				}
			}
				break;
				// endregion

				// region Alias processing
			case HumblePeer::MessageType::AliasRegister: {
				auto reg = reinterpret_cast<const HumblePeer::AliasRegister*>(msg->message());
				auto alias = reg->alias();

				auto existing = game->aliases.find( alias->c_str());

				if (existing != game->aliases.end() && existing->second != peerId) {
					LOG_INFO(
							"Rejecting peer %u's request to register alias '%s' which is already registered to peer %u\n",
							peerId, alias->c_str(), existing->second );
					sendError( this, msg->requestId(), "Alias already registered",
							   HumblePeer::MessageType::AliasRegisterError );
				} else {
					if (existing == game->aliases.end()) {
						game->aliases.insert( std::make_pair( alias->c_str(), peerId ));
					}
					LOG_INFO( "Registering alias '%s' to peer %u\n", alias->c_str(), peerId );
					sendSuccess( this, msg->requestId(), HumblePeer::MessageType::AliasRegisterSuccess );
				}
			}
				break;

			case HumblePeer::MessageType::AliasUnregister: {
				auto unreg = reinterpret_cast<const HumblePeer::AliasUnregister*>(msg->message());
				auto alias = unreg->alias();

				if (alias) {
					auto existing = game->aliases.find( alias->c_str());

					if (existing != game->aliases.end() && existing->second == peerId) {
						game->aliases.erase( existing );

						LOG_INFO( "Unregistering alias '%s' for peer %u\n", alias->c_str(), peerId );

						sendSuccess( this, msg->requestId(), HumblePeer::MessageType::AliasUnregisterSuccess );
					} else {
						LOG_INFO( "Rejecting unregister of alias '%s' for peer %u\n", alias->c_str(), peerId );

						sendError( this, msg->requestId(), "Unregister failed",
								   HumblePeer::MessageType::AliasUnregisterError );
					}
				} else {
					erase_value( game->aliases, peerId );

					LOG_INFO( "Unregistering all aliases for peer for peer %u\n", peerId );

					sendSuccess( this, msg->requestId(), HumblePeer::MessageType::AliasUnregisterSuccess );
				}
			}
				break;

			case HumblePeer::MessageType::AliasLookup: {
				auto lookup = reinterpret_cast<const HumblePeer::AliasLookup*>(msg->message());
				auto alias = lookup->alias();

				auto existing = game->aliases.find( alias->c_str());

				if (existing != game->aliases.end()) {
					LOG_INFO( "Lookup of alias '%s' for peer %u resolved to peer %u\n", alias->c_str(), peerId,
							  existing->second );
					sendAliasResolved( this, alias->c_str(), existing->second, msg->requestId());
				} else {
					LOG_INFO( "Lookup of alias '%s' for peer %u failed. No alias registered\n", alias->c_str(),
							  peerId );
					sendAliasResolved( this, alias->c_str(), 0, msg->requestId());
				}
			}
				break;
				// endregion

				// region Lobby system
			case HumblePeer::MessageType::LobbyCreate: {
				auto lobby_msg = reinterpret_cast<const HumblePeer::LobbyCreate*>(msg->message());
				auto lobbyId = game->generateNewLobbyId();

				game->lobbies.emplace( std::piecewise_construct,
									   std::forward_as_tuple( lobbyId ),
									   std::forward_as_tuple( lobbyId, peerId,
															  static_cast<humblenet_LobbyType>(lobby_msg->lobbyType()),
															  lobby_msg->maxMembers()));

				const auto& lobby = game->lobbies.at( lobbyId );

				sendLobbyDidCreate( this, msg->requestId(), lobby );
			}
				break;
			case HumblePeer::MessageType::LobbyJoin: {
				auto lobby_msg = reinterpret_cast<const HumblePeer::LobbyJoin*>(msg->message());

				auto it = game->lobbies.find( lobby_msg->lobbyId());

				if (it == game->lobbies.end()) {
					sendError( this, msg->requestId(), "Lobby does not exist",
							   HumblePeer::MessageType::LobbyJoinError );
				} else {
					it->second.addPeer( peerId );

					const auto& memberDetails = it->second.members.at( peerId );

					sendLobbyDidJoinSelf( this, msg->requestId(), it->second, peerId );

					// notify other members
					for (const auto& mit : it->second.members) {
						if (mit.first == peerId) continue;

						auto p = this->game->peers.find( mit.first );
						if (p != this->game->peers.end()) {
							sendLobbyDidJoin( p->second, it->second.lobbyId, peerId, memberDetails.attributes );

							sendLobbyDidJoin( this, it->second.lobbyId, mit.first, mit.second.attributes );
						}
					}
				}
			}
				break;
			case HumblePeer::MessageType::LobbyLeave: {
				auto lobby_msg = reinterpret_cast<const HumblePeer::LobbyLeave*>(msg->message());

				auto it = game->lobbies.find( lobby_msg->lobbyId());

				if (it == game->lobbies.end()) {
					sendError( this, msg->requestId(), "Lobby does not exist",
							   HumblePeer::MessageType::LobbyLeaveError );
				} else {
					it->second.members.erase( peerId );

					sendLobbyDidLeave( this, msg->requestId(), it->second.lobbyId, peerId );

					// notify other members
					for (const auto& mit : it->second.members) {
						auto p = this->game->peers.find( mit.first );
						if (p != this->game->peers.end()) {
							sendLobbyDidLeave( p->second, 0, it->second.lobbyId, peerId );
						}
					}

					if (it->second.ownerPeerId == peerId) {
						// set it to any other member of the lobby
						it->second.ownerPeerId = it->second.members.begin()->first;
#pragma message("TODO send change owner message")
					}
				}
			}
				break;
			case HumblePeer::MessageType::LobbyUpdate: {
				auto lobby_msg = reinterpret_cast<const HumblePeer::LobbyUpdate*>(msg->message());

				auto it = game->lobbies.find( lobby_msg->lobbyId());

				if (it == game->lobbies.end()) {
					sendError( this, msg->requestId(), "Lobby does not exist",
							   HumblePeer::MessageType::LobbyUpdateError );
				} else if (this->peerId != it->second.ownerPeerId) {
					sendError( this, msg->requestId(), "Lobby can only be updated by the lobby owner",
							   HumblePeer::MessageType::LobbyUpdateError );
				} else {
					AttributeMap attributes;

					if (lobby_msg->lobbyType()) {
						it->second.type = static_cast<humblenet_LobbyType>(lobby_msg->lobbyType());
					}

					if (lobby_msg->maxMembers()) {
						it->second.maxMembers = lobby_msg->maxMembers();
					}

					auto mode = HumblePeer::AttributeMode::Merge;

					if (lobby_msg->attributeSet()) {
						auto attribs = lobby_msg->attributeSet()->attributes();
						if (attribs) {
							for (const auto& pit : *attribs) {
								auto key = pit->key();
								auto value = pit->value();

								attributes.emplace( key->str(), value->str());
							}

							mode = lobby_msg->attributeSet()->mode();

							it->second.applyAttributes( attributes,
														lobby_msg->attributeSet()->mode() ==
														HumblePeer::AttributeMode::Merge );
						}
					}

					sendLobbyDidUpdate( this, msg->requestId(), lobby_msg->lobbyId(),
										static_cast<humblenet_LobbyType>(lobby_msg->lobbyType()),
										lobby_msg->maxMembers(),
										mode, attributes );

					for (const auto& mit : it->second.members) {
						if (mit.first == peerId) continue;

						auto p = this->game->peers.find( mit.first );
						if (p != this->game->peers.end()) {
							sendLobbyDidUpdate( p->second, 0, lobby_msg->lobbyId(),
												static_cast<humblenet_LobbyType>(lobby_msg->lobbyType()),
												lobby_msg->maxMembers(),
												mode, attributes );
						}
					}
				}
			}
				break;
			case HumblePeer::MessageType::LobbyMemberUpdate: {
				auto lobby_msg = reinterpret_cast<const HumblePeer::LobbyMemberUpdate*>(msg->message());

				auto it = game->lobbies.find( lobby_msg->lobbyId());

				if (it == game->lobbies.end()) {
					sendError( this, msg->requestId(), "Lobby does not exist",
							   HumblePeer::MessageType::LobbyMemberUpdateError );
				} else if (this->peerId != lobby_msg->peerId()) {
					sendError( this, msg->requestId(), "Lobby member can only be updated by the member",
							   HumblePeer::MessageType::LobbyMemberUpdateError );
				} else {
					AttributeMap attributes;

					if (lobby_msg->attributeSet()) {
						auto attribs = lobby_msg->attributeSet()->attributes();
						if (attribs) {
							auto mit = it->second.members.find( lobby_msg->peerId());
							if (mit == it->second.members.end()) {
								sendError( this, msg->requestId(), "Lobby member does not exist",
										   HumblePeer::MessageType::LobbyMemberUpdateError );
								break;
							}

							for (const auto& pit : *attribs) {
								auto key = pit->key();
								auto value = pit->value();

								attributes.emplace( key->str(), value->str());
							}

							mit->second.applyAttributes( attributes,
														 lobby_msg->attributeSet()->mode() ==
														 HumblePeer::AttributeMode::Merge );
						}
					}
					sendLobbyMemberDidUpdate( this, msg->requestId(), lobby_msg->lobbyId(), lobby_msg->peerId(),
											  lobby_msg->attributeSet()->mode(), attributes );

					for (const auto& mit : it->second.members) {
						if (mit.first == peerId) continue;

						auto p = this->game->peers.find( mit.first );
						if (p != this->game->peers.end()) {
							sendLobbyMemberDidUpdate( p->second, 0, lobby_msg->lobbyId(), lobby_msg->peerId(),
													  lobby_msg->attributeSet()->mode(), attributes );
						}
					}
				}
			}
				break;
				// endregion

			default:
				LOG_WARNING( "Unhandled P2P Message: %s\n", HumblePeer::EnumNameMessageType( msgType ));
				break;
		}

		return true;
	}


	void P2PSignalConnection::sendMessage(const uint8_t* buff, size_t length)
	{
		bool wasEmpty = this->sendBuf.empty();
		this->sendBuf.insert( this->sendBuf.end(), buff, buff + length );
		if (wasEmpty) {
			peerServer->triggerWrite( this->wsi );
		}
	}

}