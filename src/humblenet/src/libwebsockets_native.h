#ifndef LIBWEBSOCKETS_NATIVE_H
#define LIBWEBSOCKETS_NATIVE_H

#include <libwebsockets.h>

struct libwebsocket_context* libwebsocket_create_context_extended( struct lws_context_creation_info* info );
struct libwebsocket* libwebsocket_client_connect_extended(struct libwebsocket_context* context, const char* url, const char* protocol, void* user_data ); 

#endif // LIBWEBSOCKETS_NATIVE_H
