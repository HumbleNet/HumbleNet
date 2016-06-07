#include <signal.h>

#include <cstdlib>
#include <cstring>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <windows.h>
#else  // posix
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>

#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

#include <libwebsockets.h>

#include "humblenet.h"
#include "humblepeer.h"

#include "game.h"
#include "p2p_connection.h"
#include "server.h"
#include "logging.h"
#include "config.h"
#include "game_db.h"
#include "peer_db.h"

using namespace humblenet;

namespace humblenet {

	ha_bool sendP2PMessage(P2PSignalConnection *conn, const uint8_t *buff, size_t length) {
		conn->sendMessage(buff, length);
		return true;
	}

}  // namespace humblenet

static std::unique_ptr<Server> peerServer;


static ha_bool p2pSignalProcess(const humblenet::HumblePeer::Message *msg, void *user_data) {
	return reinterpret_cast<P2PSignalConnection *>(user_data)->processMsg(msg);
}

int callback_default(struct libwebsocket_context *context
				  , struct libwebsocket *wsi
				  , enum libwebsocket_callback_reasons reason
				  , void *user, void *in, size_t len) {


	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		break;

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
		break;

	case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
		break;

	case LWS_CALLBACK_PROTOCOL_INIT:
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		// we don't care
		break;

	case LWS_CALLBACK_WSI_CREATE:
		break;

	case LWS_CALLBACK_WSI_DESTROY:
		break;

	case LWS_CALLBACK_GET_THREAD_ID:
		// ignore
		break;

	case LWS_CALLBACK_ADD_POLL_FD:
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		break;

	case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
		break;

	case LWS_CALLBACK_LOCK_POLL:
		break;

	case LWS_CALLBACK_UNLOCK_POLL:
		break;

	default:
		LOG_WARNING("callback_default %p %p %u %p %p %u\n", context, wsi, reason, user, in, len);
		break;
	}

	return 0;
}


int callback_humblepeer(struct libwebsocket_context *context
				  , struct libwebsocket *wsi
				  , enum libwebsocket_callback_reasons reason
				  , void *user, void *in, size_t len) {


	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		{
			std::unique_ptr<P2PSignalConnection> conn(new P2PSignalConnection(peerServer.get()));
			conn->wsi = wsi;

			struct sockaddr_storage addr;
			socklen_t len = sizeof(addr);
			size_t bufsize = std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1;
			std::vector<char> ipstr(bufsize, 0);
			int port;

			int socket = libwebsocket_get_socket_fd(wsi);
			getpeername(socket, (struct sockaddr*)&addr, &len);

			if (addr.ss_family == AF_INET) {
				struct sockaddr_in *s = (struct sockaddr_in*)&addr;
				port = ntohs(s->sin_port);
				inet_ntop(AF_INET, &s->sin_addr, &ipstr[0], INET_ADDRSTRLEN);
			}
			else { // AF_INET6
				struct sockaddr_in6 *s = (struct sockaddr_in6*)&addr;
				port = ntohs(s->sin6_port);
				inet_ntop(AF_INET6, &s->sin6_addr, &ipstr[0], INET6_ADDRSTRLEN);
			}

			conn->url = std::string(&ipstr[0]);
			conn->url += std::string(":");
			conn->url += std::to_string(port);

			LOG_INFO("New connection from \"%s\"\n", conn->url.c_str());

			peerServer->signalConnections.emplace(wsi, std::move(conn));
		}

		break;


	case LWS_CALLBACK_CLOSED:
		{
			auto it = peerServer->signalConnections.find(wsi);
			if (it == peerServer->signalConnections.end()) {
				// Tried to close nonexistent signal connection
				LOG_ERROR("Tried to close a signaling connection which doesn't appear to exist\n");
				return 0;
			}

			P2PSignalConnection *conn = it->second.get();
			assert(conn != NULL);

			LOG_INFO("Closing connection to peer %u (%s)\n", conn->peerId, conn->url.c_str());

			if (conn->peerId != 0) {
				// remove it from list of peers
				assert(conn->game != NULL);

				// update the peers state.
				peerServer->savePeerState( conn );

				auto it2 = conn->game->peers.find(conn->peerId);
				// if peerId is valid (nonzero)
				// this MUST exist
				assert(it2 != conn->game->peers.end());
				conn->game->peers.erase(it2);

				// remove any aliases to this peer
				conn->game->erasePeerAliases(conn->peerId);
			}

			// and finally remove from list of signal connections
			// this is unique_ptr so it also destroys the object
			peerServer->signalConnections.erase(it);
		}

		break;

	case LWS_CALLBACK_RECEIVE:
		{
			auto it = peerServer->signalConnections.find(wsi);
			if (it == peerServer->signalConnections.end()) {
				// Receive on nonexistent signal connection
				return 0;
			}

			char *inBuf = reinterpret_cast<char *>(in);
			it->second->recvBuf.insert(it->second->recvBuf.end(), inBuf, inBuf + len);

			// function which will parse recvBuf
			ha_bool retval = parseMessage(it->second->recvBuf, p2pSignalProcess, it->second.get());
			if (!retval) {
				// error in parsing, close connection
				LOG_ERROR("Error in parsing message from \"%s\"\n", it->second->url.c_str());
				return -1;
			}
		}

		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			assert(wsi != NULL);
			auto it = peerServer->signalConnections.find(wsi);
			if (it == peerServer->signalConnections.end()) {
				// nonexistent signal connection, close it
				return -1;
			}

			P2PSignalConnection *conn = it->second.get();
			if (conn->wsi != wsi) {
				// this connection is not the one currently active in humbleNetState.
				// this one must be obsolete, close it
				return -1;
			}

			if (conn->sendBuf.empty()) {
				// no data in sendBuf
				return 0;
			}

			size_t bufsize = conn->sendBuf.size();
			std::vector<unsigned char> sendbuf(LWS_SEND_BUFFER_PRE_PADDING + bufsize + LWS_SEND_BUFFER_POST_PADDING, 0);
			memcpy(&sendbuf[LWS_SEND_BUFFER_PRE_PADDING], &conn->sendBuf[0], bufsize);
			int retval = libwebsocket_write(conn->wsi, &sendbuf[LWS_SEND_BUFFER_PRE_PADDING], bufsize, LWS_WRITE_BINARY);
			if (retval < 0) {
				// error while sending, close the connection
				return -1;
			}

			// successful write
			conn->sendBuf.erase(conn->sendBuf.begin(), conn->sendBuf.begin() + retval);
			// check if it was only partial
			if (!conn->sendBuf.empty()) {
				libwebsocket_callback_on_writable(context, conn->wsi);
			}
		}
		break;

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		break;

	case LWS_CALLBACK_PROTOCOL_INIT:
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		// we don't care
		break;

	default:
		LOG_WARNING("callback_humblepeer %p %p %u %p %p %u\n", context, wsi, reason, user, in, len);
		break;
	}

	return 0;
}


// TODO: would one callback be enough?
struct libwebsocket_protocols protocols[] = {
	  { "default", callback_default, 1 }
	, { "humblepeer", callback_humblepeer, 1 }
	, { NULL, NULL, 0 }
};

void help(const std::string& prog, const std::string& error = "")
{
	if (!error.empty()) {
		std::cerr << "Error: " << error << "\n\n";
	}
	std::cerr
		<< "Humblenet peer match-making server\n"
		<< " " << prog << "[-h] [-c configfile]\n"
		<< "   -c     Configuration file to load (default peerServer.cfg)\n"
		<< "   -h     Displays this help\n"
		<< std::endl;
}

static bool keepGoing = true;

void sighandler(int sig)
{
	keepGoing = false;
}

int main(int argc, char *argv[]) {
	std::string configFile = "peerServer.cfg";
	tConfigOptions config;

	// Parse command line arguments
	for (int i = 1; i < argc; ++i) {
		std::string arg  = argv[i];
		if (arg == "-h") {
			help(argv[0]);
			exit(1);
		} else if (arg == "-c") {
			++i;
			if (i < argc) {
				configFile = argv[i];
			} else {
				help(argv[0], "No filename passed to -c option!");
				exit(2);
			}
		}
	}

	// Parse config file
	config.parseFile(configFile);

	if (config.daemon) {
#ifndef _WIN32
		pid_t pid = fork();
		if (pid < 0) {
			std::cerr << "Failed to fork!!!" << std::endl;
			exit(1);
		} else if (pid > 0) {
			// the Parent proces
			exit(0);
		}
		// the child process
		umask(0);

		// create new SID
		pid_t sid = setsid();
		if (sid < 0) {
			std::cerr << "Could not create SID!" << std::endl;
			exit(1);
		}

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
#endif // _WIN32
	}

	logFileOpen(config.logFile);

	if (!config.pidFile.empty()) {
		std::ofstream ofs(config.pidFile.c_str());
#ifndef _WIN32
		ofs << (int)getpid() << std::endl;
#endif // _WIN32
	}

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	std::shared_ptr<GameDB> gameDB;

	if (config.gameDB.empty()) {
		gameDB.reset(new GameDBAnonymous());
	} else if (config.gameDB.compare(0, 5, "flat:") == 0) {
		gameDB.reset(new GameDBFlatFile(config.gameDB.substr(5)));
	} else {
		LOG_WARNING("Unknown DB type: %s\n", config.gameDB.c_str());
		exit(1);
	}

	std::shared_ptr<PeerDB> peerDB;
	if (config.peerDB.empty()) {
		peerDB.reset(new PeerDBAnonymous());
	} else if (config.peerDB.compare(0, 5, "flat:") == 0) {
		peerDB.reset(new PeerDBFlatFile(config.peerDB.substr(5)));
	} else {
		LOG_WARNING("Unknown DB type: %s\n", config.peerDB.c_str());
		exit(1);
	}

	peerServer.reset(new Server(gameDB, peerDB));
	peerServer->stunServerAddress = config.stunServerAddress;

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	info.protocols = protocols;
	info.port = config.port;
	info.gid = -1;
	info.uid = -1;
#define WS_OPT(member, var) info.member = (!var.empty()) ? var.c_str() : NULL
	WS_OPT(iface, config.iface);
	WS_OPT(ssl_cert_filepath, config.sslCertFile);
	WS_OPT(ssl_private_key_filepath, config.sslPrivateKeyFile);
	WS_OPT(ssl_ca_filepath, config.sslCACertFile);
	WS_OPT(ssl_cipher_list, config.sslCipherList);
#undef WS_OPT

	peerServer->context = libwebsocket_create_context(&info);
	if (peerServer->context == NULL) {
		// TODO: error message
		exit(1);
	}

	while (keepGoing) {
		// use timeout so we will eventually process signals
		// but relatively long to reduce CPU load
		// TODO: configurable timeout
		libwebsocket_service(peerServer->context, 200);
	}

	LOG_INFO("Shutting down peer server...\n");
	peerServer->shutdown();
	peerServer.reset();

	if (!config.pidFile.empty()) {
#ifndef _WIN32
		unlink(config.pidFile.c_str());
#endif // _WIN32
	}

	return 0;
}
