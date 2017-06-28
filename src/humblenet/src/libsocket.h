//
//  libsocket.h
//  humblenet
//
//  Created by Chris Rudd on 3/11/16.
//
//

#ifndef LIBSOCKET_H
#define LIBSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

struct internal_socket_t;
struct internal_context_t;
   
struct internal_callbacks_t {
    // called to return the local sdp offer
    int (*on_sdp)( internal_socket_t*, const char* offer, void* user_data );
    // called when a local candidate is discovered
    int (*on_ice_candidate)( internal_socket_t*, const char* candidate, void* user_data);
    
    // called for incoming connections to indicate the connection process is completed.
    int (*on_accept) (internal_socket_t*, void* user_data);
    // called for outgoing connections to indicate the connection process is completed.
    int (*on_connect) (internal_socket_t*, void* user_data);
    // called when an incoming datachannel is established
    int (*on_accept_channel)(internal_socket_t*, const char* name, void* user_data);
    // called when an outgoing datachannel is established
    int (*on_connect_channel)(internal_socket_t*, const char* name, void* user_data);
    // called each time data is received.
    int (*on_data)( internal_socket_t* s, const void* data, int len, void* user_data );
    // called to indicate the connection is wriable.
    int (*on_writable) (internal_socket_t*, void* user_data);
    // called when the conenction is terminated (from either side)
    int (*on_disconnect)( internal_socket_t* s, void* user_data );
    // called when socket object will be destroyed.
    int (*on_destroy)( internal_socket_t* s, void* user_data );
};

internal_context_t* internal_init(internal_callbacks_t*);
void internal_deinit(internal_context_t*);
    
bool internal_supports_webRTC( internal_context_t *);
void internal_register_protocol( internal_context_t*, const char* protocol, internal_callbacks_t* callbacks );
    
internal_socket_t* internal_connect_websocket( const char* addr, const char* protocol );
void internal_set_stun_servers( internal_context_t*, const char** servers, int count);
internal_socket_t* internal_create_webrtc(internal_context_t *);
int internal_create_offer( internal_socket_t* socket );
int internal_set_offer( internal_socket_t* socket, const char* offer );
int internal_set_answer( internal_socket_t* socket, const char* offer );
int internal_create_channel( internal_socket_t* socket, const char* name );
int internal_add_ice_candidate( internal_socket_t*, const char* candidate );
    
void internal_set_data( internal_socket_t*, void* user_data);
void internal_set_callbacks(internal_socket_t* socket, internal_callbacks_t* callbacks );
int internal_write_socket( internal_socket_t*, const void* buf, int len );
void internal_close_socket( internal_socket_t* );
    
#ifdef __cplusplus
}
#endif

#endif /* LIBSOCKET_H */
