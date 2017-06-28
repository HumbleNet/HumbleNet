#ifndef EMSCRIPTEN
#error "This should only be used under emscripten"
#endif

#ifndef LIBWEBSOCKETS_ASMJS_H
#define LIBWEBSOCKETS_ASMJS_H

// ABI compatible subset of libwebsockets

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

enum libwebsocket_callback_reasons {
    LWS_CALLBACK_ESTABLISHED = 1,
    LWS_CALLBACK_CLIENT_CONNECTION_ERROR = 2,
//    LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH,
    LWS_CALLBACK_CLIENT_ESTABLISHED = 3,
    LWS_CALLBACK_CLOSED = 4,
 //   LWS_CALLBACK_CLOSED_HTTP,
    LWS_CALLBACK_RECEIVE = 5,
    LWS_CALLBACK_CLIENT_RECEIVE = 6,
 //   LWS_CALLBACK_CLIENT_RECEIVE_PONG,
    LWS_CALLBACK_CLIENT_WRITEABLE = 7,
    LWS_CALLBACK_SERVER_WRITEABLE = 8,
 //   LWS_CALLBACK_HTTP,
 //   LWS_CALLBACK_HTTP_BODY,
 //   LWS_CALLBACK_HTTP_BODY_COMPLETION,
 //   LWS_CALLBACK_HTTP_FILE_COMPLETION,
 //   LWS_CALLBACK_HTTP_WRITEABLE,
 //   LWS_CALLBACK_FILTER_NETWORK_CONNECTION,
 //   LWS_CALLBACK_FILTER_HTTP_CONNECTION,
 //   LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED,
    LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION = 9,
 //   LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS,
//    LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS,
//    LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION,
//    LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,
//    LWS_CALLBACK_CONFIRM_EXTENSION_OKAY,
//    LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED,
//    LWS_CALLBACK_PROTOCOL_INIT,
//    LWS_CALLBACK_PROTOCOL_DESTROY,
    LWS_CALLBACK_WSI_CREATE = 10, /* always protocol[0] */
    LWS_CALLBACK_WSI_DESTROY = 11, /* always protocol[0] */
//    LWS_CALLBACK_GET_THREAD_ID,
    
    /* external poll() management support */
//    LWS_CALLBACK_ADD_POLL_FD,
//    LWS_CALLBACK_DEL_POLL_FD,
//    LWS_CALLBACK_CHANGE_MODE_POLL_FD,
//    LWS_CALLBACK_LOCK_POLL,
//    LWS_CALLBACK_UNLOCK_POLL,

};    

#define CONTEXT_PORT_NO_LISTEN 0
    
#define LWS_SEND_BUFFER_PRE_PADDING 0
#define LWS_SEND_BUFFER_POST_PADDING 0
    
enum libwebsocket_write_protocol {
//        LWS_WRITE_TEXT,
        LWS_WRITE_BINARY,
};
    
typedef int (callback_function)(struct libwebsocket_context *context,
    struct libwebsocket *wsi,
			 enum libwebsocket_callback_reasons reason, void *user,
    void *in, size_t len);
    
struct libwebsocket_protocols {
    const char *name;
    callback_function *callback;
    size_t per_session_data_size;
    size_t rx_buffer_size;
    int no_buffer_all_partial_tx;
    
    /*
     * below are filled in on server init and can be left uninitialized,
     * no need for user to use them directly either
     */
    
    struct libwebsocket_context *owning_server;
    int protocol_index;
};

struct lws_context_creation_info {
    int port;
    const char *iface;
    struct libwebsocket_protocols *protocols;
    struct libwebsocket_extension *extensions;
    struct lws_token_limits *token_limits;
    const char *ssl_cert_filepath;
    const char *ssl_private_key_filepath;
    const char *ssl_ca_filepath;
    const char *ssl_cipher_list;
    const char *http_proxy_address;
    unsigned int http_proxy_port;
    int gid;
    int uid;
    unsigned int options;
    void *user;
    int ka_time;
    int ka_probes;
    int ka_interval;
};

struct libwebsocket_context* libwebsocket_create_context( struct lws_context_creation_info* info );
struct libwebsocket_context* libwebsocket_create_context_extended( struct lws_context_creation_info* info );
    
void libwebsocket_context_destroy(struct libwebsocket_context* ctx );

struct libwebsocket* libwebsocket_client_connect_extended( struct libwebsocket_context*, const char* url, const char* protocol, void* user_data );
int libwebsocket_write( struct libwebsocket* socket, const void* data, int len, enum libwebsocket_write_protocol protocol );
void libwebsocket_callback_on_writable( struct libwebsocket_context* context, struct libwebsocket* socket );
    
#ifdef __cplusplus
}
#endif

#endif // LIBWEBSOCKETS_ASMJS_H

