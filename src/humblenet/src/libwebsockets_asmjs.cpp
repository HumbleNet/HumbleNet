#ifdef EMSCRIPTEN

#include "libwebsockets_asmjs.h"

#include <emscripten.h>
#include <stdio.h>
// TODO: should have a way to disable this on release builds
#define LOG printf

struct libwebsocket_context {
	libwebsocket_protocols* protocols; // we dont have any state to manage.
};


extern "C" int EMSCRIPTEN_KEEPALIVE libwebsocket_helper( int protocol, struct libwebsocket_context *context,
														struct libwebsocket *wsi,
														enum libwebsocket_callback_reasons reason, void *user,
														void *in, size_t len ) {
	LOG("%d -> %d -> %p -> %d\n", protocol, reason, in, (int)len );

	if( reason == LWS_CALLBACK_WSI_DESTROY ) {
		context->protocols[protocol].callback( context, wsi, reason, user, in, len );
		// TODO See if we need to destroy the user_data..currently we dont allocate it, so we never would need to free it.
		return 0;
	}
	else
		return context->protocols[protocol].callback( context, wsi, reason, user, in, len );
}

struct libwebsocket_context* libwebsocket_create_context( struct lws_context_creation_info* info ){
	return libwebsocket_create_context_extended( info );
}

struct libwebsocket_context* libwebsocket_create_context_extended( struct lws_context_creation_info* info ) {
	struct libwebsocket_context* ctx = new libwebsocket_context();
	ctx->protocols = info->protocols;

	EM_ASM_({
		var libwebsocket = {};
		var ctx = $0;

		libwebsocket.sockets = new Map();
		libwebsocket.on_event = Module.cwrap('libwebsocket_helper', 'number', ['number', 'number', 'number', 'number', 'number', 'number', 'number']);
		libwebsocket.connect = function( url, protocol, user_data ) {
			try {
				var socket = new WebSocket(url,protocol);
				socket.binaryType = "arraybuffer";
				socket.user_data = user_data;
				socket.protocol_id = 0;

				socket.onopen = this.on_connect;
				socket.onmessage = this.on_message;
				socket.onclose = this.on_close;
				socket.onerror = this.on_error;
				socket.destroy = this.destroy;

				socket.id = this.sockets.size + 1;

				this.sockets.set( socket.id, socket );

				return socket;
			} catch(e) {
				Module.print("Socket creation failed:" + e);
				return 0;
			}
		};
		libwebsocket.on_connect = function() {
			var stack = stackSave();
			// filter protocol //
			var ret = libwebsocket.on_event( 0, ctx, this.id, 9, this.user_data, allocate(intArrayFromString(this.protocol), 'i8', ALLOC_STACK), this.protocol.length );
			if( !ret ) {
				// client established
				ret = libwebsocket.on_event( this.protocol_id, ctx, this.id, 3, this.user_data, 0, 0 );
			}
			if( ret ) {
				this.close();
			}
			stackRestore(stack);
		};
		libwebsocket.on_message = function(event) {
			var stack = stackSave();
			var len = event.data.byteLength;
			var ptr = allocate( len, 'i8', ALLOC_STACK);

			var data = new Uint8Array( event.data );

			for(var i =0, buf = ptr; i < len; ++i ) {
				setValue(buf, data[i], 'i8');
				buf++;
			}

			// client receive //
			if( libwebsocket.on_event( this.protocol_id, ctx, this.id, 6, this.user_data, ptr, len ) ) {
				this.close();
			}
			stackRestore(stack);
		};
		libwebsocket.on_close = function() {
			// closed //
			libwebsocket.on_event( this.protocol_id, ctx, this.id, 4, this.user_data, 0, 0 );
			this.destroy();
		};
		libwebsocket.on_error = function() {
			// client connection error //
			libwebsocket.on_event( this.protocol_id, ctx, this.id, 2, this.user_data, 0, 0 );
			// We do not destroy here as close will be called
		};
		libwebsocket.destroy = function() {
			libwebsocket.sockets.set( this.id, undefined );
			libwebsocket.on_event( this.protocol_id, ctx, this.id, 11, this.user_data, 0, 0 );
		};

		Module.__libwebsocket = libwebsocket;
	}, ctx  );

	return ctx;
}

void libwebsocket_context_destroy(struct libwebsocket_context* ctx ) {
	delete ctx;
}

struct libwebsocket* libwebsocket_client_connect_extended(struct libwebsocket_context* ctx , const char* url, const char* protocol, void* user_data ) {
	
	struct libwebsocket* s =  (struct libwebsocket*)EM_ASM_INT({
		var socket = Module.__libwebsocket.connect( UTF8ToString($0), UTF8ToString($1), $2);
		if( ! socket ) {
			return 0;
		}

		return socket.id;
	}, url, protocol, user_data);
	
	return s;
}

int libwebsocket_write( struct libwebsocket* socket, const void* data, int len, enum libwebsocket_write_protocol protocol ) {
	return EM_ASM_INT({
		var socket = Module.__libwebsocket.sockets.get( $0 );
		if( ! socket ) {
			return -1;
		}

		// alloc a Uint8Array backed by the incoming data.
		var data_in = new Uint8Array(Module.HEAPU8.buffer, $1, $2 );
		// allow the dest array
		var data = new Uint8Array($2);
		// set the dest from the src
		data.set(data_in);

		socket.send( data );

		return $2;

	}, socket, data, len );
}

void libwebsocket_callback_on_writable( struct libwebsocket_context* ctx, struct libwebsocket* socket ) {
	// no-op
}

#endif

