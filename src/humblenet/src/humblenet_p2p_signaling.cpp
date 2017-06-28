#include "humblenet_p2p_internal.h"
#include "humblenet_alias.h"

#if defined(EMSCRIPTEN)
	#include <emscripten/emscripten.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
	#include <sys/utsname.h>
#endif

#if defined(WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#endif


static ha_bool p2pSignalProcess(const humblenet::HumblePeer::Message *msg, void *user_data);

ha_bool humblenet_signaling_connect() {
	using namespace humblenet;

	if (humbleNetState.p2pConn) {
		humbleNetState.p2pConn->disconnect();
		humbleNetState.p2pConn.reset();
		humbleNetState.myPeerId = 0;
	}

	LOG("connecting to signaling server \"%s\" with gameToken \"%s\" \n", humbleNetState.signalingServerAddr.c_str(), humbleNetState.gameToken.c_str());

	humbleNetState.p2pConn.reset(new P2PSignalConnection);
	humbleNetState.p2pConn->wsi = internal_connect_websocket(humbleNetState.signalingServerAddr.c_str(), "humblepeer");

	if (humbleNetState.p2pConn->wsi == NULL) {
		humbleNetState.p2pConn.reset();
		// TODO: can we get more specific reason ?
		humblenet_set_error("WebSocket connection to signaling server failed");
		return false;
	}

	LOG("new wsi: %p\n", humbleNetState.p2pConn->wsi);

	internal_poll_io();
	return true;
}

namespace humblenet {
	ha_bool sendP2PMessage(P2PSignalConnection *conn, const uint8_t *buff, size_t length)
	{
		if( conn == NULL ) {
			// this equates to trhe state where we never established / got disconencted
			// from the peer server.

			return false;
		}

		assert(conn != NULL);
		if (conn->wsi == NULL) {
			// this really shouldn't happen but apparently it can
			// if the game hasn't properly opened a signaling connection
			return false;
		}

		{
			HUMBLENET_UNGUARD();

			int ret = internal_write_socket(conn->wsi, (const void*)buff, length );

			return ret == length;
		}
	}

	// called for incomming connections to indicate the connection process is completed.
	int on_accept (internal_socket_t* s, void* user_data) {
		// we dont accept incomming connections
		return -1;
	}

	// called for outgoing commentions to indicate the connection process is completed.
	int on_connect (internal_socket_t* s, void* user_data) {
		HUMBLENET_GUARD();

		assert(s != NULL);
		if (!humbleNetState.p2pConn) {
			// there is no signaling connection in humbleNetState
			// this one must be obsolete, close it
			return -1;
		}

		// temporary copy, doesn't transfer ownership
		P2PSignalConnection *conn = humbleNetState.p2pConn.get();
		if (conn->wsi != s) {
			// this connection is not the one currently active in humbleNetState.
			// this one must be obsolete, close it
			return -1;
		}
		// platform info
		std::string pinfo;
#if defined(__APPLE__) || defined(__linux__)
		struct utsname buf;
		memset(&buf, 0, sizeof(struct utsname));
		int retval = uname(&buf);
		if (retval == 0)
		{
			pinfo.append("Sysname: ");
			pinfo.append(buf.sysname);
			pinfo.append(", Release: ");
			pinfo.append(buf.release);
			pinfo.append(", Version: ");
			pinfo.append(buf.version);
			pinfo.append(", Machine: ");
			pinfo.append(buf.machine);
		}
		else
		{
			LOG("Failed getting system info with uname\n");
		}
#elif defined(_WIN32)
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		pinfo.append("Windows " + std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion) + ", build " + std::to_string(osvi.dwBuildNumber));

		SYSTEM_INFO si;
		ZeroMemory(&si, sizeof(SYSTEM_INFO));
		GetSystemInfo(&si);
		switch (si.wProcessorArchitecture)
		{
			case PROCESSOR_ARCHITECTURE_INTEL:
				pinfo.append(", Intel x86");
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				pinfo.append(", Intel Itanium");
				break;
			case PROCESSOR_ARCHITECTURE_AMD64:
				pinfo.append(", AMD or Intel x64");
				break;
			default:
				pinfo.append(", non-desktop architecture");
				break;
		}
#elif defined(EMSCRIPTEN)
		char buff[512];
		int len = EM_ASM_INT({
			var buff = new Uint8Array(Module.HEAPU8.buffer, $0, $1);
			return stringToUTF8Array(navigator.userAgent, buff, 0, $1);
		}, buff, sizeof(buff));
		pinfo = buff;
#endif

		uint8_t flags = 0;
		if (humbleNetState.webRTCSupported) {
			// No trickle ICE on native
			flags = (0x1 | 0x2);
		}
		std::map<std::string, std::string> attributes;
		attributes.emplace("platform", pinfo);
		ha_bool helloSuccess = sendHelloServer(conn, flags, humbleNetState.gameToken, humbleNetState.gameSecret, humbleNetState.authToken, humbleNetState.reconnectToken, attributes);
		if (!helloSuccess) {
			// something went wrong, close the connection
			// don't humblenet_set_error, sendHelloServer should have done that
			humbleNetState.p2pConn.reset();
			return -1;
		}
		return 0;
	}

	// called each time data is received.
	int on_data( internal_socket_t* s, const void* data, int len, void* user_data ) {
		HUMBLENET_GUARD();

		assert(s != NULL);

		if (!humbleNetState.p2pConn) {
			// there is no signaling connection in humbleNetState
			// this one must be obsolete, close it
			return -1;
		}

		// temporary copy, doesn't transfer ownership
		P2PSignalConnection *conn = humbleNetState.p2pConn.get();
		if (conn->wsi != s) {
			// this connection is not the one currently active in humbleNetState.
			// this one must be obsolete, close it
			return -1;
		}

		//        LOG("Data: %d -> %s\n", len, std::string((const char*)data,len).c_str());

		conn->recvBuf.insert(conn->recvBuf.end()
							 , reinterpret_cast<const char *>(data)
							 , reinterpret_cast<const char *>(data) + len);
		ha_bool retval = parseMessage(conn->recvBuf, p2pSignalProcess, NULL);
		if (!retval) {
			// error while parsing a message, close the connection
			humbleNetState.p2pConn.reset();
			return -1;
		}
		return 0;
	}

	// called to indicate the connection is wriable.
	int on_writable (internal_socket_t* s, void* user_data) {
		HUMBLENET_GUARD();

		assert(s != NULL);
		if (!humbleNetState.p2pConn) {
			// there is no signaling connection in humbleNetState
			// this one must be obsolete, close it
			return -1;
		}

		// temporary copy, doesn't transfer ownership
		P2PSignalConnection *conn = humbleNetState.p2pConn.get();
		if (conn->wsi != s) {
			// this connection is not the one currently active in humbleNetState.
			// this one must be obsolete, close it
			return -1;
		}

		if (conn->sendBuf.empty()) {
			// no data in sendBuf
			return 0;
		}

		int retval = internal_write_socket( conn->wsi, &conn->sendBuf[0], conn->sendBuf.size() );
		if (retval < 0) {
			// error while sending, close the connection
			// TODO: should try to reopen after some time
			humbleNetState.p2pConn.reset();
			return -1;
		}

		// successful write
		conn->sendBuf.erase(conn->sendBuf.begin(), conn->sendBuf.begin() + retval);

		return 0;
	}

	// called when the conenction is terminated (from either side)
	int on_disconnect( internal_socket_t* s, void* user_data ) {
		HUMBLENET_GUARD();

		if( humbleNetState.p2pConn ) {
			if( s == humbleNetState.p2pConn->wsi ) {

				// handle retry...
				humbleNetState.p2pConn.reset();

				return 0;
			}
		}

		return 0;
	}


	void register_protocol(internal_context_t* context) {
		internal_callbacks_t callbacks;

		memset(&callbacks, 0, sizeof(callbacks));

		callbacks.on_accept = on_accept;
		callbacks.on_connect = on_connect;
		callbacks.on_data = on_data;
		callbacks.on_disconnect = on_disconnect;
		callbacks.on_writable = on_writable;
		callbacks.on_destroy = on_disconnect; // on connection failure to establish we only get a destroy, not a disconnect.

		internal_register_protocol( humbleNetState.context, "humblepeer", &callbacks );
	}
}

static ha_bool p2pSignalProcess(const humblenet::HumblePeer::Message *msg, void *user_data)
{
	using namespace humblenet;

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

			auto it = humbleNetState.pendingPeerConnectionsOut.find(peer);
			if (it == humbleNetState.pendingPeerConnectionsOut.end()) {
				switch (reject->reason()) {
					case HumblePeer::P2PRejectReason::NotFound:
						LOG("Peer %u does not exist\n", peer);
						break;
					case HumblePeer::P2PRejectReason::PeerRefused:
						LOG("Peer %u rejected our connection\n", peer);
						break;
				}
				return true;
			}

			Connection *conn = it->second;
			assert(conn != NULL);

			blacklist_peer(peer);
			humblenet_connection_set_closed(conn);
		}
			break;

		case HumblePeer::MessageType::P2PConnected:
		{
			auto connect = reinterpret_cast<const HumblePeer::P2PConnected*>(msg->message());
			
			LOG("Established connection to peer %u", connect->peerId());
		}
			break;
			
		case HumblePeer::MessageType::P2PDisconnect:
		{
			auto disconnect = reinterpret_cast<const HumblePeer::P2PDisconnect*>(msg->message());
			
			LOG("Disconnecting peer %u", disconnect->peerId());
		}
			break;
			
		case HumblePeer::MessageType::P2PRelayData:
		{
			auto relay = reinterpret_cast<const HumblePeer::P2PRelayData*>(msg->message());
			auto peer = relay->peerId();
			auto data = relay->data();
			
			LOG("Got %d bytes relayed from peer %u\n", data->Length(), peer );
			
			// Brute force...
			auto it = humbleNetState.connections.begin();
			for( it = humbleNetState.connections.begin(); it != humbleNetState.connections.end(); ++it ) {
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
		case HumblePeer::MessageType::AliasResolved:
		{
			auto reolved = reinterpret_cast<const HumblePeer::AliasResolved*>(msg->message());
			
			internal_alias_resolved_to( reolved->alias()->c_str(), reolved->peerId() );
		}
			break;
			
		default:
			LOG("p2pSignalProcess unhandled %s\n", HumblePeer::EnumNameMessageType(msgType));
			break;
	}
	
	return true;
}

