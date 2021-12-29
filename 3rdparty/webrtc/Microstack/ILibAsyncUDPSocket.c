/*   
Copyright 2006 - 2017 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifdef MEMORY_CHECK
	#define MEMCHECK(x) x
#else
	#define MEMCHECK(x)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(_MINCORE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(WINSOCK2)
	#include <winsock2.h>
	#include <ws2ipdef.h>
#elif defined(WINSOCK1)
	#include <winsock.h>
	#include <wininet.h>
#endif

#include "ILibParsers.h"
#include "ILibAsyncUDPSocket.h"
#include "ILibAsyncSocket.h"

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))
#define INET_SOCKADDR_PORT(x) (x->sa_family==AF_INET6?(unsigned short)(((struct sockaddr_in6*)x)->sin6_port):(unsigned short)(((struct sockaddr_in*)x)->sin_port))
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

struct ILibAsyncUDPSocket_Data
{
	void *user1;
	void *user2;

	ILibAsyncSocket_SocketModule UDPSocket;
	unsigned short BoundPortNumber;

	ILibAsyncUDPSocket_OnData OnData;
	ILibAsyncUDPSocket_OnSendOK OnSendOK;
};

void ILibAsyncUDPSocket_OnDataSink(ILibAsyncSocket_SocketModule socketModule, char* buffer, int *p_beginPointer, int endPointer, ILibAsyncSocket_OnInterrupt* OnInterrupt, void **user, int *PAUSE)
{
	struct ILibAsyncUDPSocket_Data *data = (struct ILibAsyncUDPSocket_Data*)*user;
	char RemoteAddress[8 + sizeof(struct sockaddr_in6)];
	UNREFERENCED_PARAMETER( OnInterrupt );
	UNREFERENCED_PARAMETER( RemoteAddress );
	ILibAsyncSocket_GetRemoteInterface(socketModule, (struct sockaddr*)&RemoteAddress);
	((int*)(RemoteAddress + sizeof(struct sockaddr_in6)))[0] = 4;
	((int*)(RemoteAddress + sizeof(struct sockaddr_in6)))[1] = 0;

	if (data->OnData != NULL)
	{
		data->OnData(
			socketModule,
			buffer, 
			endPointer,
			(struct sockaddr_in6*)&RemoteAddress,
			data->user1, 
			data->user2, 
			PAUSE);
	}
	*p_beginPointer = endPointer;
}

void ILibAsyncUDPSocket_OnSendOKSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
	struct ILibAsyncUDPSocket_Data *data = (struct ILibAsyncUDPSocket_Data*)user;
	if (data->OnSendOK!=NULL)
	{
		data->OnSendOK(socketModule, data->user1, data->user2);
	}
}

void ILibAsyncUDPSocket_OnDisconnect(ILibAsyncSocket_SocketModule socketModule, void *user)
{
	UNREFERENCED_PARAMETER( socketModule );
	free(user);
}
/*! \fn ILibAsyncUDPSocket_SocketModule ILibAsyncUDPSocket_CreateEx(void *Chain, int BufferSize, int localInterface, unsigned short localPortStartRange, unsigned short localPortEndRange, enum ILibAsyncUDPSocket_Reuse reuse, ILibAsyncUDPSocket_OnData OnData, ILibAsyncUDPSocket_OnSendOK OnSendOK, void *user)
	\brief Creates a new instance of an ILibAsyncUDPSocket module, using a random port number between \a localPortStartRange and \a localPortEndRange inclusive.
	\param Chain The chain to add this object to. (Chain must <B>not</B> not be running)
	\param BufferSize The size of the buffer to use
	\param localInterface The IP address to bind this socket to, in network order
	\param localPortStartRange The begin range to select a port number from (host order)
	\param localPortEndRange The end range to select a port number from (host order)
	\param reuse Reuse type
	\param OnData The handler to receive data
	\param OnSendOK The handler to receive notification that pending sends have completed
	\param user User object to associate with this object
	\returns The ILibAsyncUDPSocket_SocketModule handle that was created
*/
ILibAsyncUDPSocket_SocketModule ILibAsyncUDPSocket_CreateEx(void *Chain, int BufferSize, struct sockaddr *localInterface, enum ILibAsyncUDPSocket_Reuse reuse, ILibAsyncUDPSocket_OnData OnData, ILibAsyncUDPSocket_OnSendOK OnSendOK, void *user)
{
	int off = 0;
	SOCKET sock;
	int ra = (int)reuse;
	void *RetVal = NULL;
	struct ILibAsyncUDPSocket_Data *data;
	#ifdef WINSOCK2
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	#endif

	// Initialize the UDP socket data structure
	data = (struct ILibAsyncUDPSocket_Data*)malloc(sizeof(struct ILibAsyncUDPSocket_Data));
	if (data == NULL) return NULL;
	memset(data, 0, sizeof(struct ILibAsyncUDPSocket_Data));
	data->OnData = OnData;
	data->OnSendOK = OnSendOK;
	data->user1 = user;

	// Create a new socket & set REUSE if needed. If it's IPv6, use the same socket for both IPv4 and IPv6.
	if ((sock = socket(localInterface->sa_family, SOCK_DGRAM, IPPROTO_UDP)) == -1) { free(data); return 0; }
	if (reuse == ILibAsyncUDPSocket_Reuse_SHARED) if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(ra)) != 0) ILIBCRITICALERREXIT(253);
#ifdef __APPLE__
	if (reuse == ILibAsyncUDPSocket_Reuse_SHARED) if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*)&ra, sizeof(ra)) != 0) ILIBCRITICALERREXIT(253);
#endif
	if (localInterface->sa_family == AF_INET6) if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off)) != 0) ILIBCRITICALERREXIT(253);

	// Attempt to bind the UDP socket
	#ifdef WIN32
	if ( bind(sock, localInterface, INET_SOCKADDR_LENGTH(localInterface->sa_family)) != 0 ) { closesocket(sock); free(data); return NULL; }
	#else
	if ( bind(sock, localInterface, INET_SOCKADDR_LENGTH(localInterface->sa_family)) != 0 ) { close(sock); free(data); return NULL; }
	#endif

	#ifdef WINSOCK2
	WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
	#endif

	// Set the BoundPortNumber
	if (localInterface->sa_family == AF_INET6) { data->BoundPortNumber = ntohs(((struct sockaddr_in6*)localInterface)->sin6_port); } else { data->BoundPortNumber = ntohs(((struct sockaddr_in*)localInterface)->sin_port); }

	// Create an Async Socket to handle the data
	RetVal = ILibCreateAsyncSocketModule(Chain, BufferSize, &ILibAsyncUDPSocket_OnDataSink, NULL, &ILibAsyncUDPSocket_OnDisconnect, &ILibAsyncUDPSocket_OnSendOKSink);
	if (RetVal == NULL)
	{
		#if defined(WIN32) || defined(_WIN32_WCE)
		closesocket(sock);
		#else
		close(sock);
		#endif
		free(data);
		return NULL;
	}
	ILibAsyncSocket_UseThisSocket(RetVal, sock, &ILibAsyncUDPSocket_OnDisconnect, data);
	return RetVal; // Klockwork claims we could be losing the resource acquired with the call to socket(), however, we aren't becuase we are saving it with the above call to ILibAsyncSocket_UseThisSocket()
}

SOCKET ILibAsyncUDPSocket_GetSocket(ILibAsyncUDPSocket_SocketModule module)
{
	return *((SOCKET*)ILibAsyncSocket_GetSocket(module));
}


void ILibAsyncUDPSocket_DropMulticastGroupV4(ILibAsyncUDPSocket_SocketModule module, struct sockaddr_in *multicastAddr, struct sockaddr *localAddr)
{
	struct ip_mreq mreq;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif

	// We start with the multicast structure
	memcpy_s(&mreq.imr_multiaddr, sizeof(mreq.imr_multiaddr), &(((struct sockaddr_in*)multicastAddr)->sin_addr), sizeof(mreq.imr_multiaddr));
#ifdef WIN32
	mreq.imr_interface.s_addr = ((struct sockaddr_in*)localAddr)->sin_addr.S_un.S_addr;
#else
	mreq.imr_interface.s_addr = ((struct sockaddr_in*)localAddr)->sin_addr.s_addr;
#endif
	setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
}

/*! \fn int ILibAsyncUDPSocket_JoinMulticastGroup(ILibAsyncUDPSocket_SocketModule module, int localInterface, int remoteInterface)
	\brief Joins a multicast group
	\param module The ILibAsyncUDPSocket_SocketModule to join the multicast group
	\param localInterface The local IP address in network order, to join the multicast group
	\param remoteInterface The multicast ip address in network order, to join
	\returns 0 = Success, Nonzero = Failure
*/
void ILibAsyncUDPSocket_JoinMulticastGroupV4(ILibAsyncUDPSocket_SocketModule module, struct sockaddr_in *multicastAddr, struct sockaddr *localAddr)
{
	struct ip_mreq mreq;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif

	// We start with the multicast structure
	memcpy_s(&mreq.imr_multiaddr, sizeof(mreq.imr_multiaddr), &(((struct sockaddr_in*)multicastAddr)->sin_addr), sizeof(mreq.imr_multiaddr));
#ifdef WIN32
	mreq.imr_interface.s_addr = ((struct sockaddr_in*)localAddr)->sin_addr.S_un.S_addr;
#else
	mreq.imr_interface.s_addr = ((struct sockaddr_in*)localAddr)->sin_addr.s_addr;
#endif
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
}

void ILibAsyncUDPSocket_JoinMulticastGroupV6(ILibAsyncUDPSocket_SocketModule module, struct sockaddr_in6 *multicastAddr, int ifIndex)
{
	struct ipv6_mreq mreq6;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif

	memcpy_s(&mreq6.ipv6mr_multiaddr, sizeof(mreq6.ipv6mr_multiaddr), &(((struct sockaddr_in6*)multicastAddr)->sin6_addr), sizeof(mreq6.ipv6mr_multiaddr));
    mreq6.ipv6mr_interface = ifIndex;
    setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*)&mreq6, sizeof(mreq6));		// IPV6_ADD_MEMBERSHIP
}

void ILibAsyncUDPSocket_DropMulticastGroupV6(ILibAsyncUDPSocket_SocketModule module, struct sockaddr_in6 *multicastAddr, int ifIndex)
{
	struct ipv6_mreq mreq6;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif

	memcpy_s(&mreq6.ipv6mr_multiaddr, sizeof(mreq6.ipv6mr_multiaddr), &(((struct sockaddr_in6*)multicastAddr)->sin6_addr), sizeof(mreq6.ipv6mr_multiaddr));
	mreq6.ipv6mr_interface = ifIndex;
	setsockopt(s, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char*)&mreq6, sizeof(mreq6));		// IPV6_ADD_MEMBERSHIP
}
/*! \fn int ILibAsyncUDPSocket_SetMulticastInterface(ILibAsyncUDPSocket_SocketModule module, int localInterface)
	\brief Sets the local interface to use, when multicasting
	\param module The ILibAsyncUDPSocket_SocketModule handle to set the interface on
	\param localInterface The local IP address in network order, to use when multicasting
	\returns 0 = Success, Nonzero = Failure
*/
void ILibAsyncUDPSocket_SetMulticastInterface(ILibAsyncUDPSocket_SocketModule module, struct sockaddr *localInterface)
{
#if defined(WIN32)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif
	if (localInterface->sa_family == AF_INET6)
	{
		// Uses the interface index of the desired outgoing interface
		if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char*)&(((struct sockaddr_in6*)localInterface)->sin6_scope_id), sizeof(((struct sockaddr_in6*) localInterface)->sin6_scope_id)) != 0) ILIBCRITICALERREXIT(253);
	}
	else
	{
		if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char*)&(((struct sockaddr_in*)localInterface)->sin_addr.s_addr), sizeof(struct in_addr)) != 0) ILIBCRITICALERREXIT(253);
	}
}

/*! \fn int ILibAsyncUDPSocket_SetMulticastTTL(ILibAsyncUDPSocket_SocketModule module, unsigned char TTL)
	\brief Sets the Multicast TTL value
	\param module The ILibAsyncUDPSocket_SocketModule handle to set the Multicast TTL value
	\param TTL The Multicast-TTL value to use
	\returns 0 = Success, Nonzero = Failure
*/
void ILibAsyncUDPSocket_SetMulticastTTL(ILibAsyncUDPSocket_SocketModule module, int TTL)
{
	struct sockaddr_in6 localAddress;
#if defined(__SYMBIAN32__)
	return(0);
#else
	#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif
	ILibAsyncSocket_GetLocalInterface(module, (struct sockaddr*)&localAddress);
	if (setsockopt(s, localAddress.sin6_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP, localAddress.sin6_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL, (char*)&TTL, sizeof(TTL)) != 0) ILIBCRITICALERREXIT(253);
#endif
}

void ILibAsyncUDPSocket_SetMulticastLoopback(ILibAsyncUDPSocket_SocketModule module, int loopback)
{
	struct sockaddr_in6 localAddress;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)ILibAsyncSocket_GetSocket(module));
#else
	int s = *((int*)ILibAsyncSocket_GetSocket(module));
#endif

	ILibAsyncSocket_GetLocalInterface(module, (struct sockaddr*)&localAddress);
	if (setsockopt(s, localAddress.sin6_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP, localAddress.sin6_family == PF_INET6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback)) != 0) ILIBCRITICALERREXIT(253);
}

