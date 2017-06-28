#ifndef EMSCRIPTEN

#include "libwebsockets_native.h"

#include "libpoll.h"
#include <cstring>

#include <vector>
#include <string>

// Stupid hack - libwebsockets uses its own POLL defines on Windows
#ifdef _WIN32
#define LWS_POLLHUP (FD_CLOSE)
#define LWS_POLLIN (FD_READ | FD_ACCEPT)
#define LWS_POLLOUT (FD_WRITE)
#else
#define LWS_POLLHUP (POLLHUP|POLLERR)
#define LWS_POLLIN POLLIN
#define LWS_POLLOUT POLLOUT
#endif

#define LOG printf

static_assert(sizeof(libwebsocket_pollfd) == sizeof(pollfd), "pollfd struct size mismatch!");

struct LibWebSocket_Module : public poll_module_t {

	LibWebSocket_Module(libwebsocket_protocols* protocols) {
		PreSelect = (poll_pre_select)&OnPreSelect;
		PostSelect = (poll_post_select)&OnPostSelect;
		Destroy = (poll_pre_destroy)OnPreDestroy;

		// AAAAAIEEEEE!!!
		assert( protocols[0].callback != &InterceptCallback );
		
		this->protocols = protocols;
		delegate = protocols[0].callback;
		this->protocols[0].callback = &InterceptCallback;
	}

	~LibWebSocket_Module() {

	}

	int callback(struct libwebsocket_context *context
						   , struct libwebsocket *wsi
						   , enum libwebsocket_callback_reasons reason
						   , void *user, void *in, size_t len) {
		switch (reason) {
			case LWS_CALLBACK_PROTOCOL_INIT: {
				this->context = context;
			} break;

			case LWS_CALLBACK_ADD_POLL_FD: {
				struct libwebsocket_pollargs *pa = (struct libwebsocket_pollargs *)in;

				struct libwebsocket_pollfd newfd;
				memset(&newfd, 0, sizeof(newfd));
				newfd.fd = pa->fd;
				newfd.events = pa->events;
				newfd.revents = 0;

				pollfds.push_back(newfd);

				//LOG("LWS_CALLBACK_ADD_POLL_FD fd: %d events: 0x%x wsi: %p\n", pa->fd, pa->events, wsi);

			} break;

			case LWS_CALLBACK_DEL_POLL_FD: {
				struct libwebsocket_pollargs *pa = (struct libwebsocket_pollargs *)in;

				// TODO: more efficient lookup
				for (unsigned int i = 0; i < pollfds.size(); i++) {
					if (pollfds[i].fd == pa->fd) {
						// move last element over deleted element
						memmove(&pollfds[i], &pollfds.back(), sizeof(pollfds[i]));
						pollfds.pop_back();
						break;
					}
				}

				//LOG("LWS_CALLBACK_DEL_POLL_FD fd: %d events: 0x%x wsi: %p\n", pa->fd, pa->events, wsi);

			} break;

			case LWS_CALLBACK_CHANGE_MODE_POLL_FD: {
				struct libwebsocket_pollargs *pa = (struct libwebsocket_pollargs *)in;
				// TODO: more efficient lookup
				for (unsigned int i = 0; i < pollfds.size(); i++) {
					if (pollfds[i].fd == pa->fd) {
						pollfds[i].events = pa->events;
						break;
					}
				}
				//LOG("LWS_CALLBACK_CHANGE_MODE_POLL_FD fd: %d events: 0x%x wsi: %p\n", pa->fd, pa->events, wsi);

			} break;

			case LWS_CALLBACK_UNLOCK_POLL: {
				// poll fds were jsut modified, need to interrupt any poll in process so we pick up the changes.
				poll_interrupt();
			} break;

			default:
				break;
		}
		return !delegate ? 0 : delegate(context,wsi,reason,user,in,len);
	}

private:
	libwebsocket_context* context;
	libwebsocket_protocols* protocols;
	callback_function* delegate;
	std::vector<struct libwebsocket_pollfd> pollfds;

	static void OnPreSelect(LibWebSocket_Module* self, fd_set* readset, fd_set* writeset, fd_set* errorset, int* blocktime ) {
		// Add all the websocket FDs.

		for (const auto &pollfd : self->pollfds) {
			if (pollfd.events & LWS_POLLIN) {
				FD_SET(pollfd.fd, readset);
			}
			if (pollfd.events & LWS_POLLOUT) {
				FD_SET(pollfd.fd, writeset);
			}
			FD_SET(pollfd.fd, errorset);
		}
	}

	static void OnPostSelect(LibWebSocket_Module* self, int slct, fd_set* readset, fd_set* writeset, fd_set* errorset) {
		// If there was nothing triggered, no need to check.
		if( slct <= 0 )
			return;

		// Process all the triggered FDs
		int retval;
		for (auto &pollfd : self->pollfds) {
			if (FD_ISSET(pollfd.fd, readset) || FD_ISSET(pollfd.fd, writeset) || FD_ISSET(pollfd.fd, errorset) ) {
				pollfd.revents = (FD_ISSET(pollfd.fd, readset) ? LWS_POLLIN : 0)
				| (FD_ISSET(pollfd.fd, writeset) ? LWS_POLLOUT : 0)
				| (FD_ISSET(pollfd.fd, errorset) ? LWS_POLLHUP : 0);
				retval = libwebsocket_service_fd(self->context, &pollfd);
				if (retval < 0) {
					LOG("error in libwebsocket_service_fd: %d\n", retval);
					// keep going... TODO: should we?
				} else if (pollfd.revents != 0) {
					LOG("error: libwebsocket_service_fd thinks it's not our socket\n");
				}

				// if we ve already found that number of matches bail
				if( --slct <= 0 )
					break;
			}
		}
	}

	static void OnPreDestroy( LibWebSocket_Module* self ) {
		// replace the original protocol callback
		for( libwebsocket_protocols* p = self->protocols; p && p->name; ++p )
		{
			if( p->callback == InterceptCallback )
				p->callback = self->delegate;
		}
	}

	static int InterceptCallback(struct libwebsocket_context *context
							 , struct libwebsocket *wsi
							 , enum libwebsocket_callback_reasons reason
							 , void *user, void *in, size_t len){

		LibWebSocket_Module* module = (LibWebSocket_Module*)libwebsocket_context_user( context );

		return module->callback( context, wsi, reason, user, in, len );
	}
};

static int poll_extension_callback(struct libwebsocket_context *context,
										  struct libwebsocket_extension *ext,
										  struct libwebsocket *wsi,
										  enum libwebsocket_extension_callback_reasons reason,
										  void *user, void *in, size_t len)
{
	LibWebSocket_Module* module = (LibWebSocket_Module*)libwebsocket_context_user( context );
	if( reason == LWS_EXT_CALLBACK_SERVER_CONTEXT_DESTRUCT ) {
		poll_destroy_module( module );
	}

	return 0;
}

static libwebsocket_extension poll_extension[] = {
	{ "poll", &poll_extension_callback, 0, NULL }
	,{ NULL, NULL, 0, NULL }
};


struct libwebsocket_context* libwebsocket_create_context_extended( lws_context_creation_info* info ) {
	assert( info->user == NULL );

	LibWebSocket_Module* module = new (malloc(sizeof(LibWebSocket_Module))) LibWebSocket_Module(info->protocols);
	info->user = module;
	// hook in our default protocol handler (will delegate to the user provided)
	info->extensions = poll_extension;

	// insert into poll chain...
	poll_init();
	poll_add_module( module );

	return libwebsocket_create_context(info);
}


bool parseServerAddress(std::string server_string, std::string &realaddr, std::string &path, int &port, bool &use_ssl) {
	// precondition: port must contain a valid default port number
	// expecting format [<protocol>://]<server>[:<port>][/<path>]

	std::string::size_type protocolPos = server_string.find("://");
	if (protocolPos != std::string::npos) {
		std::string::size_type ptype = server_string.find("wss");
		// protocol must be first thing
		if (ptype == 0 && protocolPos == 3) {
			use_ssl = true;
		} else {
			ptype = server_string.find("ws");
			if (ptype != 0 || protocolPos != 2) {
				// invalid protocol
				return false;
			}
		}
		server_string = server_string.substr(protocolPos + 3);
	}

	std::string::size_type semicolonPos = server_string.find(':');
	std::string::size_type slashPos = server_string.find('/');
	std::string portstring;

	if (semicolonPos == 0 || slashPos == 0) {
		// only port or path supplied
		return false;
	}
	if (semicolonPos != std::string::npos) {
		// address and port
		realaddr = server_string.substr(0, semicolonPos);
	} else if (slashPos != std::string::npos) {
		// address and path
		realaddr = server_string.substr(0, slashPos);
	} else if (server_string != "") {
		// address only
		realaddr = server_string;
	}
	if (slashPos != std::string::npos && semicolonPos != std::string::npos) {
		// port and path
		portstring = server_string.substr(semicolonPos + 1, slashPos - semicolonPos - 1);
		port = atoi(portstring.c_str());
	} else if (semicolonPos != std::string::npos) {
		// port only
		portstring = server_string.substr(semicolonPos + 1);
		port = atoi(portstring.c_str());
	}
	if (port == 0) {
		port = use_ssl ? 443 : 80;
	}
	if (port <= 0 || port > 65535) {
		// invalid port number
		return false;
	}
	if (slashPos != std::string::npos) {
		// path
		path = server_string.substr(slashPos);
	}

	// postcondition: realaddr, path and port contain valid values
	return true;
}

struct libwebsocket* libwebsocket_client_connect_extended(struct libwebsocket_context* context, const char* url, const char* protocol, void* user_data ) {
	std::string realaddr;
	int port = 0;
	std::string path;
	bool use_ssl = false;
	std::string server_string(url);

	if (!parseServerAddress(server_string, realaddr, path, port, use_ssl)) {
		return NULL;
	}

	return libwebsocket_client_connect_extended(context, realaddr.c_str(), port
												, use_ssl, path.c_str(), realaddr.c_str()
												, realaddr.c_str(), protocol
												, -1, user_data);
}

#endif
