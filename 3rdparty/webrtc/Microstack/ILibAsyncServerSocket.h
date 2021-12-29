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

#ifndef ___ILibAsyncServerSocket___
#define ___ILibAsyncServerSocket___

/*! \file ILibAsyncServerSocket.h 
	\brief MicroStack APIs for TCP Server Functionality
*/

/*! \defgroup ILibAsyncServerSocket ILibAsyncServerSocket Module
	\{
*/

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.H>
#elif defined(_POSIX)
#if !defined(__APPLE__) && !defined(_VX_CPU)
#include <malloc.h>
#endif
#endif

#include "ILibAsyncSocket.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \typedef ILibAsyncServerSocket_ServerModule
	\brief The handle for an ILibAsyncServerSocket module
*/
typedef void* ILibAsyncServerSocket_ServerModule;

/*! \typedef ILibAsyncServerSocket_ConnectionToken
	\brief Connection state, for a connected session
*/
typedef ILibAsyncSocket_SocketModule ILibAsyncServerSocket_ConnectionToken;

/*! \typedef ILibAsyncServerSocket_BufferReAllocated
	\brief BufferReAllocation Handler
	\param AsyncServerSocketToken The ILibAsyncServerSocket token
	\param ConnectionToken The ILibAsyncServerSocket_Connection token
	\param user The User object
	\param newOffset The buffer has shifted by this offset
*/
typedef void (*ILibAsyncServerSocket_BufferReAllocated)(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user, ptrdiff_t newOffset);
void ILibAsyncServerSocket_SetReAllocateNotificationCallback(ILibAsyncServerSocket_ServerModule AsyncServerSocketToken, ILibAsyncServerSocket_ConnectionToken ConnectionToken, ILibAsyncServerSocket_BufferReAllocated Callback);

/*! \typedef ILibAsyncServerSocket_OnInterrupt
	\brief Handler for when a session was interrupted by a call to ILibStopChain
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user
*/
typedef void (*ILibAsyncServerSocket_OnInterrupt)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user);
/*! \typedef ILibAsyncServerSocket_OnReceive
	\brief Handler for when data is received
	\par
	<B>Note on memory handling:</B>
	When you process the received buffer, you must advance \a p_beginPointer the number of bytes that you 
	have processed. If \a p_beginPointer does not equal \a endPointer when this method completes,
	the system will continue to reclaim any memory that has already been processed, and call this method again
	until no more memory has been processed. If no memory has been processed, and more data has been received
	on the network, the buffer will be automatically grown (according to a specific alogrythm), to accomodate any new data.
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param buffer The data that was received
	\param[in,out] p_beginPointer The start index of the data that was received
	\param endPointer The end index of the data that was received
	\param[in,out] OnInterrupt Set this pointer to receive notification if this session is interrupted
	\param[in,out] user Set a custom user object
	\param[out] PAUSE Flag to indicate if the system should continue reading data off the network
*/
typedef void (*ILibAsyncServerSocket_OnReceive)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, char* buffer, int *p_beginPointer, int endPointer, ILibAsyncServerSocket_OnInterrupt *OnInterrupt, void **user, int *PAUSE);
/*! \typedef ILibAsyncServerSocket_OnConnect
	\brief Handler for when a connection is made
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param[in,out] user Set a user object to associate with this connection
*/
typedef void (*ILibAsyncServerSocket_OnConnect)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void **user);
/*! \typedef ILibAsyncServerSocket_OnDisconnect
	\brief Handler for when a connection is terminated normally
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user User object that was associated with this connection
*/
typedef void (*ILibAsyncServerSocket_OnDisconnect)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user);
/*! \typedef ILibAsyncServerSocket_OnSendOK
	\brief Handler for when pending send operations have completed
	\par
	This handler will only be called if a call to \a ILibAsyncServerSocket_Send returned a value greater
	than 0, which indicates that not all of the data could be sent.
	\param AsyncServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user User object that was associated with this connection
*/
typedef void (*ILibAsyncServerSocket_OnSendOK)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user);

extern const int ILibMemory_ASYNCSERVERSOCKET_CONTAINERSIZE;

#define ILibCreateAsyncServerSocketModule(Chain, MaxConnections, PortNumber, initialBufferSize, loopbackFlag, OnConnect, OnDisconnect, OnReceive, OnInterrupt, OnSendOK) ILibCreateAsyncServerSocketModuleWithMemory(Chain, MaxConnections, PortNumber, initialBufferSize, loopbackFlag, OnConnect, OnDisconnect, OnReceive, OnInterrupt, OnSendOK, 0, 0)
ILibAsyncServerSocket_ServerModule ILibCreateAsyncServerSocketModuleWithMemory(void *Chain, int MaxConnections, unsigned short PortNumber, int initialBufferSize, int loopbackFlag, ILibAsyncServerSocket_OnConnect OnConnect, ILibAsyncServerSocket_OnDisconnect OnDisconnect, ILibAsyncServerSocket_OnReceive OnReceive, ILibAsyncServerSocket_OnInterrupt OnInterrupt, ILibAsyncServerSocket_OnSendOK OnSendOK, int ServerUserMappedMemorySize, int SessionUserMappedMemorySize);

void *ILibAsyncServerSocket_GetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule);
void ILibAsyncServerSocket_SetTag(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *user);
int ILibAsyncServerSocket_GetTag2(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule);
void ILibAsyncServerSocket_SetTag2(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, int user);
#ifndef MICROSTACK_NOTLS
	typedef void(*ILibAsyncServerSocket_OnSSL)(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, void* ConnectionToken, SSL* ctx, void **user);
	void ILibAsyncServerSocket_SSL_SetSink(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_OnSSL handler);
	void* ILibAsyncServerSocket_GetSSL_CTX(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule);
	#ifdef MICROSTACK_TLS_DETECT
		void ILibAsyncServerSocket_SetSSL_CTX(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *ssl_ctx, int enableTLSDetect);
	#else
		void ILibAsyncServerSocket_SetSSL_CTX(ILibAsyncServerSocket_ServerModule ILibAsyncSocketModule, void *ssl_ctx);
	#endif
#endif

void ILibAsyncServerSocket_StopListening(ILibAsyncServerSocket_ServerModule module);
void ILibAsyncServerSocket_ResumeListening(ILibAsyncServerSocket_ServerModule module);

unsigned short ILibAsyncServerSocket_GetPortNumber(ILibAsyncServerSocket_ServerModule ServerSocketModule);

/*! \def ILibAsyncServerSocket_Send
	\brief Sends data onto the TCP stream
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param buffer The data to be sent
	\param bufferLength The length of \a buffer
	\param UserFreeBuffer The \a ILibAsyncSocket_MemoryOwnership enumeration, that identifies how the memory pointer to by \a buffer is to be handled
	\returns \a ILibAsyncSocket_SendStatus indicating the send status
*/
#define ILibAsyncServerSocket_Send(ServerSocketModule, ConnectionToken, buffer, bufferLength, UserFreeBuffer) ILibAsyncSocket_Send(ConnectionToken, buffer, bufferLength, UserFreeBuffer)

/*! \def ILibAsyncServerSocket_Disconnect
	\brief Disconnects a TCP stream
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define ILibAsyncServerSocket_Disconnect(ServerSocketModule, ConnectionToken) ILibAsyncSocket_Disconnect(ConnectionToken)
/*! \def ILibAsyncServerSocket_GetPendingBytesToSend
	\brief Gets the outstanding number of bytes to be sent
*/
#define ILibAsyncServerSocket_GetPendingBytesToSend(ServerSocketModule, ConnectionToken) ILibAsyncSocket_GetPendingBytesToSend(ConnectionToken)
/*! \def ILibAsyncServerSocket_GetTotalBytesSent
	\brief Gets the total number of bytes that have been sent
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define ILibAsyncServerSocket_GetTotalBytesSent(ServerSocketModule, ConnectionToken) ILibAsyncSocket_GetTotalBytesSent(ConnectionToken)
/*! \def ILibAsyncServerSocket_ResetTotalBytesSent
	\brief Resets the total bytes sent counter
	\param ServerSocketModule The parent ILibAsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define ILibAsyncServerSocket_ResetTotalBytesSent(ServerSocketModule, ConnectionToken) ILibAsyncSocket_ResetTotalBytesSent(ConnectionToken)

#ifdef __cplusplus
}
#endif

#endif

