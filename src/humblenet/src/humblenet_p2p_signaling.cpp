#include "humblenet_p2p_internal.h"

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
			// this equates to the state where we never established / got disconnected
			// from the peer server.

			return false;
		}

		return conn->sendMessage(buff, length);
	}

	// called for incoming connections to indicate the connection process is completed.
	int on_accept (internal_socket_t* s, void* user_data) {
		// we dont accept incoming connections
		return -1;
	}

	// called for outgoing connections to indicate the connection process is completed.
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

	// called to indicate the connection is writable.
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
	return reinterpret_cast<humblenet::P2PSignalConnection *>(user_data)->processMsg(msg);
}

