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

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(_MINCORE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(WINSOCK2)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(WINSOCK1)
#include <winsock.h>
#include <wininet.h>
#endif

#include "ILibParsers.h"
#include "ILibAsyncServerSocket.h"
#include "ILibAsyncSocket.h"

#define DEBUGSTATEMENT(x)

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))

typedef struct ILibAsyncServerSocketModule
{
	ILibChain_Link ChainLink;

	int MaxConnection;
	void **AsyncSockets;
	ILibServerScope scope;

	SOCKET ListenSocket;
	unsigned short portNumber, initialPortNumber;
	int listening;
	int loopbackFlag;

	ILibAsyncServerSocket_OnReceive OnReceive;
	ILibAsyncServerSocket_OnConnect OnConnect;
	ILibAsyncServerSocket_OnDisconnect OnDisconnect;
	ILibAsyncServerSocket_OnInterrupt OnInterrupt;
	ILibAsyncServerSocket_OnSendOK OnSendOK;

	void *Tag;
	int Tag2;
	#ifndef MICROSTACK_NOTLS
	ILibAsyncServerSocket_OnSSL OnSSLContext;
	SSL_CTX *ssl_ctx;
#ifdef MICROSTACK_TLS_DETECT
	int TLSDetectEnabled;
#endif
	#endif
}ILibAsyncServerSocketModule;
struct ILibAsyncServerSocket_Data
{
	struct ILibAsyncServerSocketModule *module;
	ILibAsyncServerSocket_BufferReAllocated Callback;
	void *user;
};

const int ILibMemory_ASYNCSERVERSOCKET_CONTAINERSIZE = (const int)sizeof(ILibAsyncServerSocketModule);


/*! \fn ILibAsyncServerSocket_GetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule)
\brief Returns the user Tag associated with the AsyncServer
\param ILibAsyncSocketModule The ILibAsyncServerSocket to query
\returns The user Tag
*/
void *ILibAsyncServerSocket_GetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule)
{
	return (((struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule)->Tag);
}
/*! \fn ILibAsyncServerSocket_SetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *tag)
\brief Sets the user Tag associated with the AsyncServer
\param ILibAsyncSocketModule The ILibAsyncServerSocket to save the tag to
\param tag The value to save
*/
void ILibAsyncServerSocket_SetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *tag)
{
	((struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule)->Tag = tag;
}

/*! \fn ILibAsyncServerSocket_GetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule)
\brief Returns the user Tag associated with the AsyncServer
\param ILibAsyncSocketModule The ILibAsyncServerSocket to query
\returns The user Tag
*/
int ILibAsyncServerSocket_GetTag2(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule)
{
	return(((struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule)->Tag2);
}
/*! \fn ILibAsyncServerSocket_SetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *tag)
\brief Sets the user Tag associated with the AsyncServer
\param ILibAsyncSocketModule The ILibAsyncServerSocket to save the tag to
\param tag The value to save
*/
void ILibAsyncServerSocket_SetTag2(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, int tag)
{
	((struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule)->Tag2 = tag;
}

//
// Internal method called by ILibAsyncSocket, to signal an interrupt condition
//
// <param name="socketModule">The ILibAsyncServerSocket that was interrupted</param>
// <param name="user">The associated user tag</param>
void ILibAsyncServerSocket_OnInterruptSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;
	if (data == NULL) return;
	if (data->module->OnInterrupt != NULL) data->module->OnInterrupt(data->module, socketModule, data->user);
	free(user);
}
//
// Chain PreSelect handler
//
// <param name="socketModule"></param>
// <param name="readset"></param>
// <param name="writeset"></param>
// <param name="errorset"></param>
// <param name="blocktime"></param>
void ILibAsyncServerSocket_PreSelect(void* socketModule, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)socketModule;
	int flags,i;

	UNREFERENCED_PARAMETER( writeset );
	UNREFERENCED_PARAMETER( errorset );
	UNREFERENCED_PARAMETER( blocktime );

	//
	// The socket isn't put in listening mode, until the chain is started.
	// If this variable == 0, that means we need to do that.
	//
	if (module->listening == 0)
	{
		//
		// Set the socket to non-block mode, so we can play nice and share the thread
		//
#ifdef _WIN32_WCE
		flags = 1;
		ioctlsocket(module->ListenSocket, FIONBIO, &flags);
#elif WIN32
		flags = 1;
		ioctlsocket(module->ListenSocket, FIONBIO, (u_long *)(&flags));
#elif _POSIX
		flags = fcntl(module->ListenSocket, F_GETFL,0);
		fcntl(module->ListenSocket, F_SETFL, O_NONBLOCK | flags);
#endif

		//
		// Put the socket in Listen, and add it to the fdset for the Select loop
		//
		module->listening = 1;
		listen(module->ListenSocket, 4);
		#if defined(WIN32)
		#pragma warning( push, 3 ) // warning C4127: conditional expression is constant
		#endif
		FD_SET(module->ListenSocket, readset);
		#if defined(WIN32)
		#pragma warning( pop )
		#endif
	}
	else if (module->ListenSocket != ~0)
	{
		// Only put the ListenSocket in the readset, if we are able to handle a new socket
		for(i = 0; i < module->MaxConnection; ++i)
		{
			if (ILibAsyncSocket_IsFree(module->AsyncSockets[i]) != 0)
			{
				#if defined(WIN32)
				#pragma warning( push, 3 ) // warning C4127: conditional expression is constant
				#endif
				FD_SET(module->ListenSocket, readset);
				#if defined(WIN32)
				#pragma warning( pop )
				#endif
				break;
			}
		}
	}
}
/*! \fn ILibAsyncServerSocket_SetReAllocateNotificationCallback(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, ILibAsyncServerSocket_BufferReAllocated Callback)
\brief Set the callback handler for when the internal data buffer has been resized
\param AsyncServerSocketToken The ILibAsyncServerSocket to query
\param ConnectionToken The specific connection to set the callback with
\param Callback The callback handler to set
*/
void ILibAsyncServerSocket_SetReAllocateNotificationCallback(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, ILibAsyncServerSocket_BufferReAllocated Callback)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)ILibAsyncSocket_GetUser(ConnectionToken);
	UNREFERENCED_PARAMETER( AsyncServerSocketToken );
	if (data != NULL) data->Callback = Callback;
}

//
// Chain PostSelect handler
//
// <param name="socketModule"></param>
// <param name="slct"></param>
// <param name="readset"></param>
// <param name="writeset"></param>
// <param name="errorset"></param>
void ILibAsyncServerSocket_PostSelect(void* socketModule, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset)
{
	struct ILibAsyncServerSocket_Data *data;
	struct sockaddr_in6 addr;
	//struct sockaddr_in6 receivingAddress;

#ifdef _POSIX
	socklen_t addrlen;
	//socklen_t receivingAddressLength = sizeof(struct sockaddr_in6);
#else
	int addrlen;
	//int receivingAddressLength = sizeof(struct sockaddr_in6);
#endif

	struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)socketModule;
	int i,flags;
#ifdef _WIN32_WCE
	SOCKET NewSocket;
#elif WIN32
	SOCKET NewSocket;
#elif defined( _POSIX)
	int NewSocket;
#endif

	UNREFERENCED_PARAMETER( slct );
	UNREFERENCED_PARAMETER( writeset );
	UNREFERENCED_PARAMETER( errorset );

	if (FD_ISSET(module->ListenSocket, readset) != 0)
	{
		//
		// There are pending TCP connection requests
		//
		for(i = 0; i < module->MaxConnection; ++i)
		{
			//
			// Check to see if we have available resources to handle this connection request
			//
			if (ILibAsyncSocket_IsFree(module->AsyncSockets[i]) != 0)
			{
				addrlen = sizeof(addr);
				NewSocket = accept(module->ListenSocket, (struct sockaddr*)&addr, &addrlen); // Klocwork claims we could lose the resource acquired fom the declaration, but that is not possible in this case
				//printf("Accept NewSocket=%d\r\n", NewSocket);

				// This code rejects connections that are from out-of-scope addresses (Outside the subnet, outside local host...)
				// It needs to be updated to IPv6.
				/*
				if (NewSocket != ~0)
				{
				switch(module->scope)
				{
				case ILibServerScope_LocalLoopback:
				// Check that the caller ip address is the same as the receive IP address
				getsockname(NewSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);
				if (((struct sockaddr_in*)&receivingAddress)->sin_addr.s_addr != ((struct sockaddr_in*)&addr)->sin_addr.s_addr)   // TODO: NOT IPv6 COMPILANT!!!!!!!!!!!!!!!!!!!!!!!!
				{
				#if defined(WIN32) || defined(_WIN32_WCE)
				closesocket(NewSocket);
				#else
				close(NewSocket);
				#endif
				NewSocket = ~0;
				}
				break;
				case ILibServerScope_LocalSegment:
				getsockname(NewSocket, (struct sockaddr*)&receivingAddress, &receivingAddressLength);
				break;
				default:
				break;
				}
				}
				*/
				if (NewSocket != ~0)
				{
					//printf("Accepting new connection, socket = %d\r\n", NewSocket);
					//
					// Set this new socket to non-blocking mode, so we can play nice and share thread
					//
#ifdef _WIN32_WCE
					flags = 1;
					ioctlsocket(NewSocket ,FIONBIO, &flags);
#elif WIN32
					flags = 1;
					ioctlsocket(NewSocket, FIONBIO, (u_long *)(&flags));
#elif _POSIX
					flags = fcntl(NewSocket, F_GETFL,0);
					fcntl(NewSocket, F_SETFL, O_NONBLOCK|flags);
#endif
					//
					// Instantiate a module to contain all the data about this connection
					//
					if ((data = (struct ILibAsyncServerSocket_Data*)malloc(sizeof(struct ILibAsyncServerSocket_Data))) == NULL) ILIBCRITICALEXIT(254);
					memset(data, 0, sizeof(struct ILibAsyncServerSocket_Data));
					data->module = (struct ILibAsyncServerSocketModule*)socketModule;

					ILibAsyncSocket_UseThisSocket(module->AsyncSockets[i], NewSocket, &ILibAsyncServerSocket_OnInterruptSink, data);
					ILibAsyncSocket_SetRemoteAddress(module->AsyncSockets[i], (struct sockaddr*)&addr);

					#ifndef MICROSTACK_NOTLS
					if (module->ssl_ctx != NULL)
					{
						// Accept a new TLS connection
#ifdef MICROSTACK_TLS_DETECT
						SSL* ctx = ILibAsyncSocket_SetSSLContext(module->AsyncSockets[i], module->ssl_ctx, module->TLSDetectEnabled == 0 ? ILibAsyncSocket_TLS_Mode_Server : ILibAsyncSocket_TLS_Mode_Server_with_TLSDetectLogic);
#else
						SSL* ctx = ILibAsyncSocket_SetSSLContext(module->AsyncSockets[i], module->ssl_ctx, ILibAsyncSocket_TLS_Mode_Server);
#endif
						if (ctx != NULL && module->OnSSLContext != NULL) { module->OnSSLContext(module, module->AsyncSockets[i], ctx, &(data->user)); }
					}
					else
					#endif	
					if (module->OnConnect != NULL)
					{
						// Notify the user about this new connection
						module->OnConnect(module, module->AsyncSockets[i], &(data->user));
					}
				}
				else {break;}
			}
		}
	}
} // Klocwork claims that we could lose the resource acquired in the declaration, but that is not possible in this case
//
// Chain Destroy handler
//
// <param name="socketModule"></param>
void ILibAsyncServerSocket_Destroy(void *socketModule)
{
	struct ILibAsyncServerSocketModule *module =(struct ILibAsyncServerSocketModule*)socketModule;

	free(module->AsyncSockets);
	if (module->ListenSocket != (SOCKET)~0)
	{
#ifdef _WIN32_WCE
		closesocket(module->ListenSocket);
#elif WIN32
		closesocket(module->ListenSocket);
#elif _POSIX
		close(module->ListenSocket);
#endif
		module->ListenSocket = (SOCKET)~0;
	}
}

//
// Internal method dispatched by the OnData event of the underlying ILibAsyncSocket
//
// <param name="socketModule"></param>
// <param name="buffer"></param>
// <param name="p_beginPointer"></param>
// <param name="endPointer"></param>
// <param name="OnInterrupt"></param>
// <param name="user"></param>
// <param name="PAUSE"></param>
void ILibAsyncServerSocket_OnData(ILibAsyncSocket_SocketModule socketModule,char* buffer,int *p_beginPointer, int endPointer,void (**OnInterrupt)(void *AsyncSocketMoudle, void *user),void **user, int *PAUSE)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)(*user);
	int bpointer = *p_beginPointer;

	UNREFERENCED_PARAMETER( OnInterrupt );

	// Pass the received data up
	if (data != NULL && data->module->OnReceive != NULL)
	{
		data->module->OnReceive(data->module, socketModule, buffer, &bpointer, endPointer, &(data->module->OnInterrupt), &(data->user), PAUSE);
		if (ILibAsyncSocket_IsFree(socketModule))
		{
			*p_beginPointer = endPointer;
		}
		else
		{
			*p_beginPointer = bpointer;
		}
	}
}
// 
// Internal method dispatched by the OnConnect event of the underlying ILibAsyncSocket. In this case, it will only occur when TLS connection is established.
// 
// <param name="socketModule"></param>
// <param name="user"></param>
void ILibAsyncServerSocket_OnConnectSink(ILibAsyncSocket_SocketModule socketModule, int Connected, void *user)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;

	if (data == NULL) return;
	if (Connected == 0)
	{
		// Connection Failed, clean up
		free(data);
		return;
	}

	if (data->module->OnConnect != NULL) data->module->OnConnect(data->module, socketModule, &(data->user));
}
// 
// Internal method dispatched by the OnDisconnect event of the underlying ILibAsyncSocket
// 
// <param name="socketModule"></param>
// <param name="user"></param>
void ILibAsyncServerSocket_OnDisconnectSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;

	// Pass this Disconnect event up
	if (data == NULL) return;
	if (data->module->OnDisconnect != NULL) data->module->OnDisconnect(data->module, socketModule, data->user);

	// If the chain is shutting down, we need to free some resources
	if (ILibIsChainBeingDestroyed(data->module->ChainLink.ParentChain) == 0) free(data);
}
// 
// Internal method dispatched by the OnSendOK event of the underlying ILibAsyncSocket
// 
// <param name="socketModule"></param>
// <param name="user"></param>
void ILibAsyncServerSocket_OnSendOKSink(ILibAsyncSocket_SocketModule socketModule, void *user)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;

	// Pass the OnSendOK event up
	if (data != NULL && data->module->OnSendOK != NULL) data->module->OnSendOK(data->module, socketModule, data->user);
}
// 
// Internal method dispatched by ILibAsyncSocket, to signal that the buffers have been reallocated
// 
// <param name="ConnectionToken">The ILibAsyncSocket sender</param>
// <param name="user">The ILibAsyncServerSocket_Data object</param>
// <param name="offSet">The offset to the new buffer location</param>
void ILibAsyncServerSocket_OnBufferReAllocated(ILibAsyncSocket_SocketModule ConnectionToken, void *user, ptrdiff_t offSet)
{
	struct ILibAsyncServerSocket_Data *data = (struct ILibAsyncServerSocket_Data*)user;
	if (data!=NULL && data->Callback!=NULL)
	{
		//
		// If someone above us, has registered for this callback, we need to fire it,
		// with the correct user object
		//
		data->Callback(data->module,ConnectionToken,data->user,offSet);
	}
}

#ifndef MICROSTACK_NOTLS
void* ILibAsyncServerSocket_GetSSL_CTX(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule)
{
	return(((struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule)->ssl_ctx);
}
#ifdef MICROSTACK_TLS_DETECT
void ILibAsyncServerSocket_SetSSL_CTX(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *ssl_ctx, int enableTLSDetect)
#else
void ILibAsyncServerSocket_SetSSL_CTX(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *ssl_ctx)
#endif
{
	if (ILibAsyncSocketModule != NULL && ssl_ctx != NULL)
	{
		struct ILibAsyncServerSocketModule *module = (struct ILibAsyncServerSocketModule*)ILibAsyncSocketModule;
		module->ssl_ctx = (SSL_CTX *)ssl_ctx;
#ifdef MICROSTACK_TLS_DETECT
		module->TLSDetectEnabled = enableTLSDetect;
#endif
	}
}
#endif

void ILibAsyncServerSocket_ResumeListeningSink(void *chain, void* user)
{
	ILibAsyncServerSocketModule *m = (ILibAsyncServerSocketModule*)user;

	int ra = 1;
	int off = 0;
	int receivingAddressLength = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 localif;
	struct sockaddr_in6 localAddress;

	UNREFERENCED_PARAMETER(chain);

	memset(&localif, 0, sizeof(struct sockaddr_in6));
	if (m->loopbackFlag != 2 && ILibDetectIPv6Support())
	{
		// Setup the IPv6 any or loopback address, this socket will also work for IPv4 traffic on IPv6 stack
		localif.sin6_family = AF_INET6;
		localif.sin6_addr = (m->loopbackFlag != 0 ? in6addr_loopback : in6addr_any);
		localif.sin6_port = htons(m->initialPortNumber);
	}
	else
	{
		// IPv4-only detected
		localif.sin6_family = AF_INET;
#ifdef WINSOCK2
		((struct sockaddr_in*)&localif)->sin_addr.S_un.S_addr = htonl((m->loopbackFlag != 0 ? INADDR_LOOPBACK : INADDR_ANY));
#else 
		((struct sockaddr_in*)&localif)->sin_addr.s_addr = htonl((m->loopbackFlag != 0 ? INADDR_LOOPBACK : INADDR_ANY));
#endif
		((struct sockaddr_in*)&localif)->sin_port = htons(m->initialPortNumber);
	}

	// Get our listening socket
	if ((m->ListenSocket = socket(localif.sin6_family, SOCK_STREAM, IPPROTO_TCP)) == -1) { m->ListenSocket = (SOCKET)~0;  return; }

	// Setup the IPv6 & IPv4 support on same socket
	if (localif.sin6_family == AF_INET6) if (setsockopt(m->ListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off)) != 0) ILIBCRITICALERREXIT(253);

#ifdef SO_NOSIGPIPE
	setsockopt(m->ListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&ra, sizeof(int));  // Turn off SIGPIPE if writing to disconnected socket
#endif

#if defined(WIN32)
	// On Windows. Lets make sure no one else can bind to this addr/port. This stops socket hijacking (not a problem on Linux).
	if (setsockopt(m->ListenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&ra, sizeof(int)) != 0) ILIBCRITICALERREXIT(253);
#else
	// On Linux. Setting the re-use on a TCP socket allows reuse of the socket even in timeout state. Allows for fast stop/start (Not a problem on Windows).
	if (setsockopt(m->ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(int)) != 0) ILIBCRITICALERREXIT(253);
#endif

	// Bind the socket
#if defined(WIN32)
	if (bind(m->ListenSocket, (struct sockaddr*)&localif, INET_SOCKADDR_LENGTH(localif.sin6_family)) != 0) { closesocket(m->ListenSocket); m->ListenSocket = (SOCKET)~0; return; }
#else
	if (bind(m->ListenSocket, (struct sockaddr*)&localif, INET_SOCKADDR_LENGTH(localif.sin6_family)) != 0) { close(m->ListenSocket); m->ListenSocket = (SOCKET)~0; return; }
#endif

	// Fetch the local port number
#if defined(WINSOCK2)
	getsockname(m->ListenSocket, (struct sockaddr*)&localAddress, (int*)&receivingAddressLength);
#else
	getsockname(m->ListenSocket, (struct sockaddr*)&localAddress, (socklen_t*)&receivingAddressLength);
#endif
	if (localAddress.sin6_family == AF_INET6) m->portNumber = ntohs(localAddress.sin6_port); else m->portNumber = ntohs(((struct sockaddr_in*)&localAddress)->sin_port);
	m->listening = 0;
}
void ILibAsyncServerSocket_StopListeningSink(void *chain, void* user)
{
	ILibAsyncServerSocketModule *m = (ILibAsyncServerSocketModule*)user;
	UNREFERENCED_PARAMETER(chain);

	if (m->ListenSocket != (SOCKET)~0)
	{
#ifdef WIN32
		closesocket(m->ListenSocket);
#else
		close(m->ListenSocket);
#endif
		m->ListenSocket = (SOCKET)~0;
	}
}
//! Take the server socket out of listening mode, rejecting new incoming connection requests
/*!
	\param module ILibAsyncServerSocket_ServerModule Server Listening Module
*/
void ILibAsyncServerSocket_StopListening(ILibAsyncServerSocket_ServerModule module)
{
	ILibAsyncServerSocketModule *m = (ILibAsyncServerSocketModule*)module;
	ILibChain_RunOnMicrostackThread(m->ChainLink.ParentChain, ILibAsyncServerSocket_StopListeningSink, m);
}
//! Put the server socket back in listening mode, to allow new incoming connection requests
/*!
	\param module ILibAsyncServerSocket_ServerModule Server Listening Module
*/
void ILibAsyncServerSocket_ResumeListening(ILibAsyncServerSocket_ServerModule module)
{
	ILibAsyncServerSocketModule *m = (ILibAsyncServerSocketModule*)module;
	ILibChain_RunOnMicrostackThread(m->ChainLink.ParentChain, ILibAsyncServerSocket_ResumeListeningSink, m);
}

void ILibAsyncServerSocket_RemoveFromChainSink(void *chain, void *user)
{
	ILibAsyncServerSocketModule *module = (ILibAsyncServerSocketModule*)user;
	int i;

	for (i = 0; i < module->MaxConnection; ++i)
	{
		ILibChain_SafeRemoveEx(chain, module->AsyncSockets[i]);
	}
	ILibChain_SafeRemoveEx(chain, module);
}
void ILibAsyncServerSocket_RemoveFromChain(ILibAsyncServerSocket_ServerModule serverModule)
{
	ILibChain_RunOnMicrostackThreadEx(((ILibAsyncServerSocketModule*)serverModule)->ChainLink.ParentChain, ILibAsyncServerSocket_RemoveFromChainSink, serverModule);
}

/*! \fn ILibCreateAsyncServerSocketModule(void *Chain, int MaxConnections, int PortNumber, int initialBufferSize, ILibAsyncServerSocket_OnConnect OnConnect,ILibAsyncServerSocket_OnDisconnect OnDisconnect,ILibAsyncServerSocket_OnReceive OnReceive,ILibAsyncServerSocket_OnInterrupt OnInterrupt, ILibAsyncServerSocket_OnSendOK OnSendOK)
\brief Instantiates a new ILibAsyncServerSocket
\param Chain The chain to add this module to. (Chain must <B>not</B> be running)
\param MaxConnections The max number of simultaneous connections that will be allowed
\param PortNumber The port number to bind to. 0 will select a random port
\param initialBufferSize The initial size of the receive buffer
\param loopbackFlag 0 to bind to ANY, 1 to bind to IPv6 loopback first, 2 to bind to IPv4 loopback first.
\param OnConnect Function Pointer that triggers when a connection is established
\param OnDisconnect Function Pointer that triggers when a connection is closed
\param OnReceive Function Pointer that triggers when data is received
\param OnInterrupt Function Pointer that triggers when connection interrupted
\param OnSendOK Function Pointer that triggers when pending sends are complete
\param ServerAutoFreeMemorySize Size of AutoFreeMemory on Server to co-allocate
\param SessionAutoFreeMemorySize Size of AutoFreeMemory on Session to co-allocate
\returns An ILibAsyncServerSocket module
*/
ILibAsyncServerSocket_ServerModule ILibCreateAsyncServerSocketModuleWithMemory(void *Chain, int MaxConnections, unsigned short PortNumber, int initialBufferSize, int loopbackFlag, ILibAsyncServerSocket_OnConnect OnConnect, ILibAsyncServerSocket_OnDisconnect OnDisconnect, ILibAsyncServerSocket_OnReceive OnReceive, ILibAsyncServerSocket_OnInterrupt OnInterrupt, ILibAsyncServerSocket_OnSendOK OnSendOK, int ServerUserMappedMemorySize, int SessionUserMappedMemorySize)
{
	int i;
	int ra = 1;
	int off = 0;
	int receivingAddressLength = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 localif;
	struct sockaddr_in6 localAddress;
	struct ILibAsyncServerSocketModule *RetVal;

	memset(&localif, 0, sizeof(struct sockaddr_in6));
	if (loopbackFlag != 2 && ILibDetectIPv6Support())
	{
		// Setup the IPv6 any or loopback address, this socket will also work for IPv4 traffic on IPv6 stack
		localif.sin6_family = AF_INET6;
		localif.sin6_addr = (loopbackFlag != 0?in6addr_loopback:in6addr_any);
		localif.sin6_port = htons(PortNumber);
	}
	else
	{
		// IPv4-only detected
		localif.sin6_family = AF_INET;
#ifdef WINSOCK2
		((struct sockaddr_in*)&localif)->sin_addr.S_un.S_addr = htonl((loopbackFlag != 0?INADDR_LOOPBACK:INADDR_ANY));
#else 
		((struct sockaddr_in*)&localif)->sin_addr.s_addr = htonl((loopbackFlag != 0?INADDR_LOOPBACK:INADDR_ANY));
#endif
		((struct sockaddr_in*)&localif)->sin_port = htons(PortNumber);
	}

	// Instantiate a new AsyncServer module
	RetVal = (struct ILibAsyncServerSocketModule*)ILibChain_Link_Allocate(sizeof(struct ILibAsyncServerSocketModule), ServerUserMappedMemorySize);
	
	RetVal->ChainLink.PreSelectHandler = &ILibAsyncServerSocket_PreSelect;
	RetVal->ChainLink.PostSelectHandler = &ILibAsyncServerSocket_PostSelect;
	RetVal->ChainLink.DestroyHandler = &ILibAsyncServerSocket_Destroy;
	RetVal->ChainLink.ParentChain = Chain;
	RetVal->OnConnect = OnConnect;
	RetVal->OnDisconnect = OnDisconnect;
	RetVal->OnInterrupt = OnInterrupt;
	RetVal->OnSendOK = OnSendOK;
	RetVal->OnReceive = OnReceive;
	RetVal->MaxConnection = MaxConnections;
	RetVal->AsyncSockets = (void**)malloc(MaxConnections * sizeof(void*));
	if (RetVal->AsyncSockets == NULL) { free(RetVal); ILIBMARKPOSITION(253); return NULL; }
	RetVal->portNumber = (unsigned short)PortNumber;
	RetVal->loopbackFlag = loopbackFlag;
	RetVal->initialPortNumber = PortNumber;

	// Get our listening socket
	if ((RetVal->ListenSocket = socket(localif.sin6_family, SOCK_STREAM, IPPROTO_TCP)) == -1) { free(RetVal->AsyncSockets); free(RetVal); return 0; }

	// Setup the IPv6 & IPv4 support on same socket
	if (localif.sin6_family == AF_INET6) if (setsockopt(RetVal->ListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off)) != 0) ILIBCRITICALERREXIT(253);

#ifdef SO_NOSIGPIPE
	setsockopt(RetVal->ListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&ra, sizeof(int));  // Turn off SIGPIPE if writing to disconnected socket
#endif

#if defined(WIN32)
	// On Windows. Lets make sure no one else can bind to this addr/port. This stops socket hijacking (not a problem on Linux).
	if (setsockopt(RetVal->ListenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&ra, sizeof(int)) != 0) ILIBCRITICALERREXIT(253);
#else
	// On Linux. Setting the re-use on a TCP socket allows reuse of the socket even in timeout state. Allows for fast stop/start (Not a problem on Windows).
	if (setsockopt(RetVal->ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(int)) != 0) ILIBCRITICALERREXIT(253);
#endif

	// Bind the socket
#if defined(WIN32)
	if (bind(RetVal->ListenSocket, (struct sockaddr*)&localif, INET_SOCKADDR_LENGTH(localif.sin6_family)) != 0) { closesocket(RetVal->ListenSocket); free(RetVal->AsyncSockets); free(RetVal); return 0; }
#else
	if (bind(RetVal->ListenSocket, (struct sockaddr*)&localif, INET_SOCKADDR_LENGTH(localif.sin6_family)) != 0) { close(RetVal->ListenSocket); free(RetVal->AsyncSockets); free(RetVal); return 0; }
#endif

	// Fetch the local port number
#if defined(WINSOCK2)
	getsockname(RetVal->ListenSocket, (struct sockaddr*)&localAddress, (int*)&receivingAddressLength);
#else
	getsockname(RetVal->ListenSocket, (struct sockaddr*)&localAddress, (socklen_t*)&receivingAddressLength);
#endif
	if (localAddress.sin6_family == AF_INET6) RetVal->portNumber = ntohs(localAddress.sin6_port); else RetVal->portNumber = ntohs(((struct sockaddr_in*)&localAddress)->sin_port);

	// Create our socket pool
	for(i = 0; i < MaxConnections; ++i)
	{
		RetVal->AsyncSockets[i] = ILibCreateAsyncSocketModuleWithMemory(Chain, initialBufferSize, &ILibAsyncServerSocket_OnData, &ILibAsyncServerSocket_OnConnectSink, &ILibAsyncServerSocket_OnDisconnectSink, &ILibAsyncServerSocket_OnSendOKSink, SessionUserMappedMemorySize);
		//
		// We want to know about any buffer reallocations, because anything above us may want to know
		//
		ILibAsyncSocket_SetReAllocateNotificationCallback(RetVal->AsyncSockets[i], &ILibAsyncServerSocket_OnBufferReAllocated);
	}
	ILibAddToChain(Chain, RetVal);

	return(RetVal);
}

/*! \fn ILibAsyncServerSocket_GetPortNumber(ILibAsyncServerSocket_ServerModule ServerSocketModule)
\brief Returns the port number the server is bound to
\param ServerSocketModule The ILibAsyncServer to query
\returns The listening port number
*/
unsigned short ILibAsyncServerSocket_GetPortNumber(ILibAsyncServerSocket_ServerModule ServerSocketModule)
{
	return(((struct ILibAsyncServerSocketModule*)ServerSocketModule)->portNumber);
}
#ifndef MICROSTACK_NOTLS
void ILibAsyncServerSocket_SSL_SetSink(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_OnSSL handler)
{
	((struct ILibAsyncServerSocketModule*)AsyncServerSocketModule)->OnSSLContext = handler;
}
#endif
