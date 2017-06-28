//
//  humblenet_socket.h
//
//  Created by Chris Rudd on 3/9/16.
//

#ifndef HUMBLENET_SOCKET_H
#define HUMBLENET_SOCKET_H

#include "humblenet_p2p.h"

// TODO: Support transparent mode? -- use setsockopt to attach humblenet to a specific socket?

#if defined EMSCRIPTEN && !defined NO_NATIVE_SOCKETS
	#define NO_NATIVE_SOCKETS
#endif

#ifdef NO_NATIVE_SOCKETS
	#include <netdb.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#include <arpa/inet.h>
#else
	#include <unistd.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/param.h>
	#include <sys/ioctl.h>
	#include <sys/uio.h>
	#include <errno.h>
#endif

#include <string.h>

#if !defined(HUMBLENET_strncasecmp)
#if defined(WIN32)
	#define HUMBLENET_strncasecmp _strnicmp
#else
	#include <strings.h>
	#define HUMBLENET_strncasecmp strncasecmp
#endif
#endif

#define HUMBLENET_SOCKET		-2
#define AF_HNET                 AF_MAX + 1

#define PF_HNET                 AF_HNET

struct hostent* hs_gethostbyname( const char* s );
int hs_getaddrinfo( const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res );
void hs_freeaddrinfo( struct addrinfo* res );

int hs_socket( int af, int type, int protocol );
int hs_select( int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int hs_recvfrom( int sock, void* buffer, int length, int flags, struct sockaddr* addr, uint32_t* addr_len );
int hs_sendto( int sock, const void* buffer, int length, int flags, struct sockaddr* addr, uint32_t addr_len );

#ifdef HUMBLENET_SOCKET_IMPL

int hs_socket( int af, int type, int protocol ) {
	if( af == PF_HNET && type == SOCK_DGRAM )
		return HUMBLENET_SOCKET;
#ifndef NO_NATIVE_SOCKETS
	else if( af != PF_HNET )
		return socket( af, type, protocol );
	else
#endif
	
#ifdef _WIN32
	WSASetLastError( WSAPROTONOTSUPPORTED );
#else
	errno = EINVAL;
#endif
	return INVALID_SOCKET;
}

int hs_recvfrom( int sock, void* buffer, int length, int flags, struct sockaddr* addr, uint32_t* addr_len ) {
	if( sock == HUMBLENET_SOCKET )
	{
		PeerId peer;
		int ret =  humblenet_p2p_recvfrom(buffer, length, &peer, 0 );
		((struct sockaddr_in*)addr)->sin_addr.s_addr = htonl(peer);
		// we dont support ports...yet....
		((struct sockaddr_in*)addr)->sin_port = 0;
		((struct sockaddr_in*)addr)->sin_family = AF_HNET;
		if( ret > 0 )
			return ret;
		else if( ret == -1 ) {
			errno = ECONNRESET;
			return ret;
		} else {
			errno = EWOULDBLOCK;
			ret = -1;
		}
		return ret;
	}
#ifndef NO_NATIVE_SOCKETS
	else
		return recvfrom(sock, buffer, length, flags, addr, addr_len );
#endif
}

int hs_sendto( int sock, const void* buffer, int length, int flags, struct sockaddr* addr, uint32_t addr_len ) {
	if( sock == HUMBLENET_SOCKET )
	{
		int ret = humblenet_p2p_sendto( buffer, length, ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr), SEND_RELIABLE, 0 );
		if( ret == -1 )
			errno = ECONNRESET;
		return ret;
	}
#ifndef NO_NATIVE_SOCKETS
	else
		return sendto( sock, buffer, length, flags, addr, addr_len );
#endif
	
	errno = EINVAL;
	return -1;
}

int hs_select( int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
#ifdef EMSCRIPTEN
	return humblenet_p2p_wait(0) ? 1 : 0;
#else
	return humblenet_p2p_select( nfds, readfds, writefds, exceptfds, timeout );
#endif
}

static const char* humblenet_domain = ".humblenet";
static int   humblenet_domain_length = 10;

#define IS_HUMBLENET_HOST( h ) ((strlen(h) > humblenet_domain_length) && (0 == HUMBLENET_strncasecmp(h+strlen(h)-humblenet_domain_length, humblenet_domain, humblenet_domain_length)))

int hs_getaddrinfo( const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res ) {
	static struct addrinfo info;
	static struct sockaddr_in addr;
	
	if( service == NULL && IS_HUMBLENET_HOST( node ) ) {
		// strip the domain suffix
		static char stemp[256] = {0};
		strncpy(stemp, node, sizeof(stemp)-1);
		stemp[ strlen(node) - humblenet_domain_length ] = '\0';
		node = stemp;
		
		memset(&info, 0, sizeof(info));
		memset(&addr, 0, sizeof(addr));
		
		info.ai_flags = 0x80000000;
		info.ai_family = AF_HNET;
		info.ai_socktype = SOCK_DGRAM;
		//info.ai_protocol = ??
		info.ai_addrlen = sizeof( addr );
		info.ai_addr = (struct sockaddr*)&addr;
		info.ai_canonname = "humble";
		
		addr.sin_family = info.ai_family;

		if( node[0] == 'p' && node[1] == 'e' && node[2] == 'e' && node[3] == 'r' && node[4] == '_' )
			addr.sin_addr.s_addr = htonl( atoi( node+5 ) );
		else
			addr.sin_addr.s_addr = htonl( humblenet_p2p_virtual_peer_for_alias( node ) );
			
		*res = &info;
		return 0;
	}
#ifndef NO_NATIVE_SOCKETS
	return getaddrinfo(node, service, hints, res);
#else
#ifdef _WIN32
	WSASetLastError( WSAHOST_NOT_FOUND );
#else
	h_errno = HOST_NOT_FOUND;
#endif
	return EAI_NONAME;
#endif
}

void hs_freeaddrinfo( struct addrinfo* res ) {
	// Look for out internally managed addrinfo...
	if( res && res->ai_flags == 0x80000000 )
		res = res->ai_next;
	if( res )
		freeaddrinfo( res );
}

struct hostent* hs_gethostbyname( const char* s ) {
	static struct hostent buff[5];
	static struct in_addr addr;
	static char* list[5] = {0,0,0,0,0};
	
	struct hostent* ret = NULL;
	
	if( IS_HUMBLENET_HOST( s ) ) {
		struct addrinfo* res = NULL;
		
		hs_getaddrinfo( s, NULL, NULL, &res );
		
		ret = buff;
		ret->h_addrtype = res->ai_family;
		ret->h_length = 4;
		ret->h_aliases = list+1;
		
		ret->h_addr_list = list;
		
		ret->h_addr_list[0] = (char*)&addr;
		
		addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
		
		hs_freeaddrinfo( res );
		
		return ret;
	}
#ifndef NO_NATIVE_SOCKETS
	return gethostbyname(s);
#else
#ifdef _WIN32
	WSASetLastError( WSAHOST_NOT_FOUND );
#else
	h_errno = HOST_NOT_FOUND;
#endif
	return NULL;
#endif
}

#endif

#define select			hs_select
#define recvfrom		hs_recvfrom
#define sendto			hs_sendto
#define gethostbyname	hs_gethostbyname
#define getaddrinfo		hs_getaddrinfo
#define freeaddrinfo	hs_freeaddrinfo
#define socket			hs_socket

// TODO: Some code redefines close -> closesocket while others do the opposite to remove the need for #ifdef around the closes.
// How shouls we do it? would it be best to just define both to hs_close ?

#ifdef _WIN32
#define closesocket( x )		( x == HUMBLENET_SOCKET ? 0 : closesocket( x ) )
#else
#define close( x )				( x == HUMBLENET_SOCKET ? 0 : close( x ) )
#endif

#define ioctl( x, ... )			( x == HUMBLENET_SOCKET ? 0 : ioctl( x, __VA_ARGS__ ) )
#define setsockopt( x, ... )	( x == HUMBLENET_SOCKET ? 0 : setsockopt( x, __VA_ARGS__ ) )
#define bind( x, ... )			( x == HUMBLENET_SOCKET ? 0 : bind( x, __VA_ARGS__ ) )

#endif /* HUMBLENET_SOCKET_H */
