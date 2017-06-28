#include "libsocket.h"

#include <map>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>

#ifdef EMSCRIPTEN
#include "libwebsockets_asmjs.h"
#else
#include "libwebsockets_native.h"	// SKIP_AMALGAMATOR_INCLUDE
#include "cert_pem.h"				// SKIP_AMALGAMATOR_INCLUDE
#include <openssl/ssl.h>
#endif

#include "libwebrtc.h"

// TODO: should have a way to disable this on release builds
#define LOG printf

struct internal_socket_t {
	bool owner;
	bool closing;			// if this is set, ignore close attempts as the close process has already been initiated.
	void* user_data;
	internal_callbacks_t callbacks;
	
	// web socket connection info
	struct libwebsocket *wsi;
	std::string url;
	
	// webrtc connection info
	struct libwebrtc_connection* webrtc;
	struct libwebrtc_data_channel* webrtc_channel;
	
	internal_socket_t(bool owner=true)
	:owner(owner)
	,closing(false)
	,user_data(NULL)
	,wsi(NULL)
	,webrtc(NULL)
	,webrtc_channel(NULL)
	{}
	
	~internal_socket_t(){
		assert( owner );
	}
};

struct internal_context_t {
	internal_callbacks_t callbacks;
	
	std::map<std::string,internal_callbacks_t> protocols;
	
	std::string turnServer;
	std::string turnUsername;
	std::string turnPassword;
	std::vector<std::string> stunServerList;
	
	// websocket
	struct libwebsocket_context *websocket;

	// webrtc
	struct libwebrtc_context* webrtc;
};

static internal_context_t* g_context;

int websocket_protocol(struct libwebsocket_context *context
					   , struct libwebsocket *wsi
					   , enum libwebsocket_callback_reasons reason
					   , void *user, void *in, size_t len) {

//	LOG("%p %p %d %p\n", context, wsi, reason, user );

	internal_socket_t* socket = reinterpret_cast<internal_socket_t*>( user );

	int ret = 0;
	
	switch (reason) {
		case LWS_CALLBACK_WSI_CREATE: {
			socket->wsi = wsi;
		}
		break;

		case LWS_CALLBACK_WSI_DESTROY: {
			ret = socket->callbacks.on_destroy( socket, socket->user_data );
			if( socket->owner )
				delete socket;
		}
		break;

		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
			if( socket ) {
				socket->closing = true;
				ret = socket->callbacks.on_disconnect( socket, socket->user_data );
			}
		} break;
			
		case LWS_CALLBACK_ESTABLISHED:
		{
			ret = socket->callbacks.on_accept( socket, socket->user_data );
		}
		break;
			
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
		{
			 ret = socket->callbacks.on_connect( socket, socket->user_data );
		}
		break;
			
		case LWS_CALLBACK_CLOSED:
		{
			socket->closing = true;
			ret = socket->callbacks.on_disconnect( socket, socket->user_data );
		}
		break;
			
		case LWS_CALLBACK_RECEIVE:
		case LWS_CALLBACK_CLIENT_RECEIVE:
		{
			ret = socket->callbacks.on_data( socket, in, len, socket->user_data );
		}
		break;
			
		case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: {
			std::map<std::string, internal_callbacks_t>::iterator it = g_context->protocols.find( std::string( (const char*)in, len ) );
			if( it == g_context->protocols.end() ) {
				LOG("Unknown protocol: %s\n", (const char*)in);
				return -1;
			}
			
			socket->callbacks = it->second;
			return 0;
		} break;
			
		case LWS_CALLBACK_CLIENT_WRITEABLE:
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			ret = socket->callbacks.on_writable( socket, socket->user_data );
		}
		break;

#if !defined(EMSCRIPTEN)
		case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
		{
			int i, count = 0;
			SSL_CTX *ctx = (SSL_CTX*)user;
			BIO *in = NULL;
			X509 *x = NULL;
			in = BIO_new_mem_buf((void*)cert_pem, cert_pem_len);
			if (in == NULL) {
				break;
			}

			X509_STORE *store = SSL_CTX_get_cert_store( ctx );

			for (;;) {
				x = PEM_read_bio_X509_AUX(in, NULL, NULL, NULL);
				if (x == NULL) {
					if ((ERR_GET_REASON(ERR_peek_last_error()) == PEM_R_NO_START_LINE) && (count > 0)) {
						ERR_clear_error();
						break;
					} else {
						OPENSSL_PUT_ERROR(X509, ERR_R_PEM_LIB);
						break;
					}
				}
				i = X509_STORE_add_cert(store, x);
				if (!i) break;
				count++;
				X509_free(x);
				x = NULL;
			}
		}
			break;
#endif

		default:
			//LOG("callback_humblenet %p %p %u %p %p %u\n", context, wsi, reason, user, in, static_cast<unsigned int>(len));
			break;
	}
	
	return ret;
}

int webrtc_protocol(struct libwebrtc_context *context,
							  struct libwebrtc_connection *connection, struct libwebrtc_data_channel* channel,
							  enum libwebrtc_callback_reasons reason, void *user,
							  void *in, int len)
{
	internal_socket_t* socket = reinterpret_cast<internal_socket_t*>( user );

//	LOG("%p %p %p %d %p\n", context, connection, channel, reason, user );

	int ret = 0;
	
	switch( reason ) {
		case LWRTC_CALLBACK_LOCAL_DESCRIPTION:
			ret = socket->callbacks.on_sdp( socket, (const char*)in, socket->user_data );
			break;

		case LWRTC_CALLBACK_ICE_CANDIDATE:
			ret = socket->callbacks.on_ice_candidate( socket, (const char*)in, socket->user_data );
			break;

		case LWRTC_CALLBACK_ESTABLISHED:
			ret = socket->callbacks.on_connect( socket, socket->user_data );
			break;

		case LWRTC_CALLBACK_DISCONNECTED:
			socket->closing = true;
			ret = socket->callbacks.on_disconnect( socket, socket->user_data );
			break;

		case LWRTC_CALLBACK_CHANNEL_ACCEPTED:
			socket->webrtc_channel = channel;
			ret = socket->callbacks.on_accept_channel( socket, (const char*)in, socket->user_data );
			break;

		case LWRTC_CALLBACK_CHANNEL_CONNECTED:
			ret = socket->callbacks.on_connect_channel( socket, (const char*)in, socket->user_data );
			break;

		case LWRTC_CALLBACK_CHANNEL_RECEIVE:
			ret = socket->callbacks.on_data( socket, in, len, socket->user_data );
			break;

		case LWRTC_CALLBACK_CHANNEL_CLOSED:
			socket->webrtc_channel = NULL;

			// we are 1-1 DC -> channel, otherwise we would delegate this up.
			// socket->callbacks.on_disconnect_channel( socket, socket->user_data );

			// instead we simply close the connection as well
			if( !socket->closing && socket->webrtc )
				libwebrtc_close_connection( socket->webrtc );

			break;
			
		case LWRTC_CALLBACK_DESTROY:
			socket->callbacks.on_destroy( socket, socket->user_data );
			if( socket->owner )
				delete socket;
			break;
		case LWRTC_CALLBACK_ERROR:
			break;
	}

	return ret;
}


#define MAX_PROTOCOLS 16

struct libwebsocket_protocols protocols[MAX_PROTOCOLS] = {
	{ "default", websocket_protocol, sizeof(internal_socket_t) }
	,{ NULL, NULL, 0 }
};

int ok_callback() {
	LOG("Ok_CB\n");
	return 0;
}

int err_callback() {
	return -1;
}

void sanitize_callbacks( internal_callbacks_t& callbacks ) {
	intptr_t* ptr = (intptr_t*)&callbacks;

	for( int i = 0; i < sizeof(internal_callbacks_t)/sizeof(void*); ++i, ++ptr ) {
		if( !*ptr ) {
			LOG("Sanitize callback #%d\n", i+1 );
			*ptr = (intptr_t)&ok_callback;
		}
	}
}

internal_context_t* internal_init(internal_callbacks_t* callbacks) {
	internal_context_t* ctx = new internal_context_t();

	if( callbacks )
		ctx->callbacks= *callbacks;

	sanitize_callbacks( ctx->callbacks );

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	info.protocols = protocols;
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.gid = -1;
	info.uid = -1;
#if 0
#if defined __APPLE__ || defined(__linux__)
	// test a few wll known locations
	const char* certs[] = { "./cert.pem", "/etc/openssl/cert.pem", "/opt/local/etc/openssl/cert.pem", "/etc/pki/tls/cert.pem" , NULL };
	for( const char** test = certs; *test; test++ ) {
		if( access( *test, F_OK ) != -1 ) {
			info.ssl_ca_filepath = *test;
			break;
		}
	}
#elif defined(WIN32)
	info.ssl_ca_filepath = "cert.pem";
#endif
#endif

	ctx->websocket = libwebsocket_create_context_extended(&info);
	ctx->webrtc = libwebrtc_create_context(&webrtc_protocol);

	g_context = ctx;

	return ctx;
}

bool internal_supports_webRTC(internal_context_t* ctx) {
	return ctx->webrtc != NULL;
}

void internal_set_stun_servers( internal_context_t* ctx, const char** servers, int count){
	if( ctx->webrtc )
		libwebrtc_set_stun_servers( ctx->webrtc, servers, count );

	ctx->stunServerList.clear();
	for( int i = 0; i < count; ++i ) {
		ctx->stunServerList.push_back( *servers );
		servers++;
	}
}

void internal_set_callbacks(internal_socket_t* socket, internal_callbacks_t* callbacks ) {
	if( ! callbacks ) {
		socket->callbacks = g_context->callbacks;
	} else {
		socket->callbacks = *callbacks;
		sanitize_callbacks( socket->callbacks );
	}
}

void internal_register_protocol( internal_context_t* ctx, const char* name, internal_callbacks_t* callbacks ) {
	internal_callbacks_t cb = *callbacks;
	sanitize_callbacks( cb );

	ctx->protocols.insert( std::make_pair( std::string(name), cb ) );
	// TODO: Sanatize callbacks

	libwebsocket_protocols* protocol = protocols + ctx->protocols.size();
	// make sure the next protocols is "empty"
	*(protocol + 1) = *protocol;
	// now copy the prior record
	*protocol = *(protocol-1);
	// and update the name
	protocol->name = name;
	protocol->protocol_index++;
}

void internal_deinit(internal_context_t* ctx) {
	// how to destroy connection factory
	if (!ctx) return;

	libwebsocket_context_destroy( ctx->websocket );
	if( ctx->webrtc )
		libwebrtc_destroy_context( ctx->webrtc );

	delete ctx;
}

void internal_set_data(internal_socket_t* socket, void* user_data) {
	socket->user_data = user_data;
}

void * internal_get_data(internal_socket_t* socket ) {
	return socket->user_data;
}

internal_socket_t* internal_connect_websocket( const char *server_addr, const char* protocol ) {
	internal_socket_t* socket = new internal_socket_t(true);

	socket->wsi = libwebsocket_client_connect_extended(g_context->websocket, server_addr, protocol, socket );
	if (socket->wsi == NULL) {
		delete socket;
		return NULL;
	}

	socket->callbacks = g_context->protocols.find( protocol )->second;

	socket->url = server_addr;

	return socket;
}

internal_socket_t* internal_create_webrtc(internal_context_t* ctx) {
	if( ! ctx->webrtc )
		return NULL;

	internal_socket_t* socket = new internal_socket_t(true);

	socket->webrtc = libwebrtc_create_connection_extended( ctx->webrtc, socket );
	socket->callbacks = ctx->callbacks;

	return socket;
}

int internal_create_offer(internal_socket_t* socket ) {
	assert( socket->webrtc != NULL );
	assert( socket->webrtc_channel == NULL );

	if( ! libwebrtc_create_offer( socket->webrtc ) )
		return 0;

	return 1;
}

int internal_set_offer( internal_socket_t* socket, const char* inOffer ){
	assert( socket->webrtc != NULL );
	assert( socket->webrtc_channel == NULL );

	if( ! libwebrtc_set_offer( socket->webrtc, inOffer ) )
		return 0;
	else
		return 1;
}

int internal_set_answer( internal_socket_t* socket, const char* inOffer ){
	assert( socket->webrtc != NULL );
	assert( socket->webrtc_channel == NULL );

	if( ! libwebrtc_set_answer( socket->webrtc, inOffer ) )
		return 0;
	else
		return 1;
}

int internal_add_ice_candidate( internal_socket_t* socket, const char* candidate ) {
	assert( socket->webrtc != NULL );
	return libwebrtc_add_ice_candidate( socket->webrtc, candidate );
}

int internal_create_channel( internal_socket_t* socket, const char* name ){
	assert( socket->webrtc != NULL );
	assert( socket->webrtc_channel == NULL );

	socket->webrtc_channel = libwebrtc_create_channel(socket->webrtc, name );

	if( !socket->webrtc_channel )
	   return 0;

	return 1;
}

void internal_close_socket( internal_socket_t* socket ) {
	if( socket->closing )
		// socket clos process has already started, ignore the request.
		return;
	else if( socket->wsi ) {
		libwebsocket_callback_on_writable(g_context->websocket, socket->wsi);
		// TODO: How do we clean up ?
	} else if( socket->webrtc ) {
		// this will trigger the destruction of the channel and thus the destruction of our socket object.
		socket->closing = true;
		libwebrtc_close_connection( socket->webrtc );
	} else {
		assert( "Destroyed socket passed to close" == NULL );
	}
}

int internal_write_socket(internal_socket_t* socket, const void *buf, int bufsize) {
	if( socket->wsi ) {
		// TODO: Should this buffer the data like the docuemntation states and only write on the writable callback ?
		
#if LWS_SEND_BUFFER_PRE_PADDING == 0 && LWS_SEND_BUFFER_POST_PADDING == 0
		int retval = libwebsocket_write(socket->wsi, buf, bufsize, LWS_WRITE_BINARY);
#else
		// libwebsocket requires the caller to allocate the frame prefix/suffix storage.
		std::vector<unsigned char> sendbuf(LWS_SEND_BUFFER_PRE_PADDING + bufsize + LWS_SEND_BUFFER_POST_PADDING, 0);
		memcpy(&sendbuf[LWS_SEND_BUFFER_PRE_PADDING], buf, bufsize);

		int retval = libwebsocket_write(socket->wsi, &sendbuf[LWS_SEND_BUFFER_PRE_PADDING], bufsize, LWS_WRITE_BINARY);
#endif
		
		// mark it non-writable and tell websocket to inform us when it's writable again
		//connection->writable = false;
		if( retval > 0 ) {
			libwebsocket_callback_on_writable(g_context->websocket, socket->wsi);
		}

		return retval;

	} else if( socket->webrtc && socket->webrtc_channel ) {
		return libwebrtc_write( socket->webrtc_channel, buf, bufsize );
	}

	// bad/disconnected socket
	return -1;
}

