/*   
Copyright 2006 - 2016 Intel Corporation

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

/*
Real LMS code can be found here: http://software.intel.com/en-us/articles/download-the-latest-intel-amt-open-source-drivers
*/

#if !defined(_NOHECI)

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
#include "ILibAsyncSocket.h"
#include "ILibAsyncServerSocket.h"
#include "ILibCrypto.h"

#ifndef NOCOMMANDER
#include "ILibWebClient.h"
#include "ILibWebServer.h"
#include "ILibLMS-WebSite.h"
#endif

#ifdef WIN32
    #include "../microlms/heci/heciwin.h"
	#include "WinBase.h"
#endif
#ifdef _POSIX
    #include "../heci/HECILinux.h"
#endif
#include "../microlms/heci/PTHICommand.h"
#include "../microlms/heci/LMEConnection.h"

#if defined(WIN32) && !defined(snprintf) && (_MSC_PLATFORM_TOOLSET <= 120)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))
#define LMS_MAX_CONNECTIONS 6 // Maximum is 6, above this channel open starts to fail.
#define LMS_MAX_SESSIONS 32
#define LMS_MIN_SESSIONID 8
//#define _DEBUGLMS
#ifdef _DEBUGLMS
#define LMSDEBUG(...) printf(__VA_ARGS__);
#else
#define LMSDEBUG(...)
#endif

struct ILibLMS_StateModule* IlibExternLMS = NULL;	// Glocal pointer to the LMS module. Since we can only have one of these modules running, a global pointer is sometimes useful.
void ILibLMS_SetupConnection(struct ILibLMS_StateModule* module, int i);
void ILibLMS_LaunchHoldingSessions(struct ILibLMS_StateModule* module);
#ifndef NOCOMMANDER
void ILibLMS_ResumeWebSocket(struct ILibWebServer_Session *session);
#endif

// Each LMS session can be in one of these states.
enum LME_CHANNEL_STATUS {
	LME_CS_FREE = 0,						// The session slot is free, can be used at any time.
	LME_CS_PENDING_CONNECT = 1,				// A connection as been made to LMS and a notice has been send to Intel AMT asking for a CHANNEL OPEN.
	LME_CS_CONNECTED = 2,					// Intel AMT confirmed the connection and data can flow in both directions.
	LME_CS_PENDING_LMS_DISCONNECT = 3,		// The connection to LMS was disconnected, Intel AMT has been notified and we are waitting for Intel AMT confirmation.
	LME_CS_PENDING_AMT_DISCONNECT = 4,		// Intel AMT decided to close the connection. We are disconnecting the LMS TCP connection.
	LME_CS_HOLDING = 5,						// We got too much data from the LMS TCP connection, more than Intel AMT can handle. We are holding the connection until AMT can handle more.
	LME_CS_CONNECTION_WAIT = 6				// A TCP connection to LMS was made, but there all LMS connections are currently busy. Waitting for one to free up.
};

// This is the structure for a session
struct LMEChannel
{
	struct ILibLMS_StateModule* parent;		// Pointer to the parent module
	int ourid;								// The identifier of this channel on our side, this is the same as the index in the "Channel Sessions[LMS_MAX_SESSIONS];" array below.
	int amtid;								// The Intel AMT identifier for this channel.
	enum LME_CHANNEL_STATUS status;			// Current status of the channel.
	int sockettype;							// Type of connected socket: 0 = TCP, 1 = WebSocket
	void* socketmodule;						// The microstack associated with the LMS connection.
	int txwindow;							// Transmit window.
	int rxwindow;							// Receive window.
	unsigned short localport;				// The Intel AMT connection port.
	int errorcount;							// Open channel error count.
	char* pending;							// Buffer pointing to pending data that needs to be sent to Intel AMT, used for websocket only.
	int pendingcount;						// Buffer pointing to pending data size that needs to be sent to Intel AMT, used for websocket only.
	int pendingptr;							// Pointer to start of the pending buffer.
};

enum LME_VERSION_HANDSHAKING {
	LME_NOT_INITIATED,
	LME_INITIATED,
	LME_AGREED
};

enum LME_SERVICE_STATUS {
	LME_NOT_STARTED,
	LME_STARTED
};

// This is the Intel AMT LMS chain module structure
struct ILibLMS_StateModule
{
	void (*PreSelect)(void* object, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);

	void *Chain;										// Pointer to the chain 
	void *Server1;										// Microstack TCP server for port 16992
	void *Server2;										// Microstack TCP server for port 16993

	struct LMEConnection MeConnection;					// The MEI connection structure
	struct LMEChannel Sessions[LMS_MAX_SESSIONS];		// List of sessions
#ifndef NOCOMMANDER
	ILibWebServer_ServerToken WebServer;				// LMS Web Server
#endif
	enum LME_VERSION_HANDSHAKING handshakingStatus;
	enum LME_SERVICE_STATUS pfwdService;
	unsigned int AmtProtVersionMajor;					// Intel AMT MEI major version
	unsigned int AmtProtVersionMinor;					// Intel AMT MEI minor version
	char* WebDir;										// Web Directory
	int NextSlotId;										// Next time we search for an empty slot, start with this one
	int PendingAmtConnection;							// Currently pending connection session, used so we never issue two channel open to AMT at the same time
	sem_t Lock;											// Global lock, this is needed because MEI listener is a different thread from the microstack thread
};


#ifdef WIN32

void __fastcall ILibLMS_setregistryA(char* name, char* value)
{
	HKEY hKey;
#ifdef _WINSERVICE
	// If running as a Windows Service, save the key in LOCAL_MACHINE
	if (RegCreateKey(HKEY_LOCAL_MACHINE, L"Software\\Open Source\\MicroLMS", &hKey) == ERROR_SUCCESS)
#else
	// If running in Console mode, save the key in CURRENT_USER
	if (RegCreateKey(HKEY_CURRENT_USER, L"Software\\Open Source\\MicroLMS", &hKey) == ERROR_SUCCESS)
#endif
	{
		RegSetValueExA(hKey, name, 0, REG_SZ, (BYTE*)value, (DWORD)strnlen_s(value, 65535));
		RegCloseKey(hKey);
	}
}

int __fastcall ILibLMS_getregistryA(char* name, char** value)
{
	HKEY hKey;
	DWORD len = 0;
#ifdef _WINSERVICE
	// If running as a Windows Service, open the key in LOCAL_MACHINE
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Open Source\\MicroLMS", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) { *value = NULL; return 0; }
#else
	// If running in Console mode, save the key in CURRENT_USER
	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Open Source\\MicroLMS", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) { *value = NULL; return 0; }
#endif
	if (RegQueryValueExA(hKey, name, NULL, NULL, NULL, &len) != ERROR_SUCCESS || len == 0) { *value = NULL; RegCloseKey(hKey); return 0; }
	if ((*value = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	if (RegQueryValueExA(hKey, name, NULL, NULL, (LPBYTE)(*value), &len) != ERROR_SUCCESS || len == 0) { free(*value); *value = NULL; RegCloseKey(hKey); return 0; }
	RegCloseKey(hKey);
	return len - 1;
}

int __fastcall ILibLMS_deleteregistryA(char* name)
{
	HKEY hKey;
#ifdef _WINSERVICE
	// If running as a Windows Service, open the key in LOCAL_MACHINE
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Open Source\\MicroLMS", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) { return 0; }
#else
	// If running in Console mode, save the key in CURRENT_USER
	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Open Source\\MicroLMS", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) { return 0; }
#endif
	RegDeleteValueA(hKey, name);
	RegCloseKey(hKey);
	return 1;
}
#endif

int GetFreeSlot(struct ILibLMS_StateModule* module) {
	int i, j;
	for (i = 0; i < LMS_MAX_SESSIONS; i++)
	{
		j = (module->NextSlotId + i) % LMS_MAX_SESSIONS;
		if (module->Sessions[j].status == LME_CS_FREE)
		{
			module->NextSlotId = j + 1;
			return j;
		}
	}
	return -1;
}

int SlotFromChannelId(int channel) {
	if (channel < LMS_MIN_SESSIONID || channel >= (LMS_MAX_SESSIONS + LMS_MIN_SESSIONID)) { LMSDEBUG("Invalid ChannelId: %d\r\n", channel); return 0; }
	return channel - LMS_MIN_SESSIONID;
}

// This function is called each time a MEI message is received from Intel AMT
void ILibLMS_MEICallback(struct LMEConnection* lmemodule, void *param, void *rxBuffer, unsigned int len)
{
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)param;
	int disconnects = 0; // This is a counter of how many sessions are freed up. We use this at the end to place HOLDING sessions into free slots.

	// Happens when the chain is being destroyed, don't call anything chain related.
	if (rxBuffer == NULL) return;

	sem_wait(&(module->Lock));
	//LMSDEBUG("ILibLMS_MEICallback %d\r\n", ((unsigned char*)rxBuffer)[0]);

	switch (((unsigned char*)rxBuffer)[0])
	{
	case APF_GLOBAL_REQUEST: // 80
		{
			
			int request = 0;
			unsigned char *pCurrent;
			APF_GENERIC_HEADER *pHeader = (APF_GENERIC_HEADER *)rxBuffer;

			pHeader->StringLength = ntohl(pHeader->StringLength);

			if (pHeader->StringLength == APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_TCP_FORWARD_REQUEST) && memcmp(pHeader->String, APF_GLOBAL_REQUEST_STR_TCP_FORWARD_REQUEST, APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_TCP_FORWARD_REQUEST)) == 0) { request = 1; }
			else if (pHeader->StringLength == APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_TCP_FORWARD_CANCEL_REQUEST) && memcmp(pHeader->String, APF_GLOBAL_REQUEST_STR_TCP_FORWARD_CANCEL_REQUEST, APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_TCP_FORWARD_CANCEL_REQUEST)) == 0) { request = 2; }
			else if (pHeader->StringLength == APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_UDP_SEND_TO) && memcmp(pHeader->String, APF_GLOBAL_REQUEST_STR_UDP_SEND_TO, APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_UDP_SEND_TO)) == 0) { request = 3; }

			if (request == 1 || request == 2)
			{
				int port = 0;
				unsigned int len2;
				unsigned int bytesRead = len;
				unsigned int hsize=0;
				if (request==1)
					hsize = sizeof(APF_GENERIC_HEADER) + APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_TCP_FORWARD_REQUEST) + sizeof(UINT8);
				else if (request==2)
					hsize = sizeof(APF_GENERIC_HEADER) + APF_STR_SIZE_OF(APF_GLOBAL_REQUEST_STR_TCP_FORWARD_CANCEL_REQUEST) + sizeof(UINT8);

				pCurrent = (unsigned char*)rxBuffer + hsize;
				bytesRead -= hsize;
				if (bytesRead < sizeof(unsigned int)) { 
					LME_Deinit(lmemodule); 
					sem_post(&(module->Lock)); 
					return; 
				}

				len2 = ntohl(*((unsigned int *)pCurrent));
				pCurrent += sizeof(unsigned int);
				if (bytesRead < (sizeof(unsigned int) + len2 + sizeof(unsigned int))) { 
					LME_Deinit(lmemodule); 
					sem_post(&(module->Lock)); 
					return; 
				}

				// addr = (char*)pCurrent;
				pCurrent += len2;
				port = ntohl(*((unsigned int *)pCurrent));

				// TODO: Look at port number

				// Confirm the new open port
				LME_TcpForwardReplySuccess(lmemodule, port);
			}
			else if (request == 3)
			{
				// Send a UDP packet
				// TODO: Send UDP
			}
			else{
				// do nothing
			}
		}
		break;
	case APF_CHANNEL_OPEN: // (90) Sent by Intel AMT when a channel needs to be open from Intel AMT. This is not common, but WSMAN events are a good example of channel coming from AMT.
		{
			unsigned int len2;
			unsigned char *pCurrent;
			struct LMEChannelOpenRequestMessage channelOpenRequest;
			APF_GENERIC_HEADER *pHeader = (APF_GENERIC_HEADER *)rxBuffer;

			if (len < sizeof(APF_GENERIC_HEADER) + ntohl(pHeader->StringLength) + 7 + (5 * sizeof(UINT32))) { LME_Deinit(lmemodule); sem_post(&(module->Lock)); return; }
			pCurrent = (unsigned char*)rxBuffer + sizeof(APF_GENERIC_HEADER) + APF_STR_SIZE_OF(APF_OPEN_CHANNEL_REQUEST_DIRECT);

			channelOpenRequest.ChannelType = APF_CHANNEL_DIRECT;
			channelOpenRequest.SenderChannel = ntohl(*((UINT32 *)pCurrent));
			pCurrent += sizeof(UINT32);
			channelOpenRequest.InitialWindow = ntohl(*((UINT32 *)pCurrent));
			pCurrent += 2 * sizeof(UINT32);
			len2 = ntohl(*((UINT32 *)pCurrent));
			pCurrent += sizeof(UINT32);
			channelOpenRequest.Address = (char*)pCurrent;
			pCurrent += len2;
			channelOpenRequest.Port = ntohl(*((UINT32 *)pCurrent));
			pCurrent += sizeof(UINT32);

			//printf("APF_CHANNEL_OPEN - %d, %s, %s:%d\r\n", channelOpenRequest.SenderChannel, (char*)pHeader->String, channelOpenRequest.Address, channelOpenRequest.Port);

			// if (ntohl(pHeader->StringLength) == APF_STR_SIZE_OF(APF_OPEN_CHANNEL_REQUEST_DIRECT) && memcmp((char*)pHeader->String, APF_OPEN_CHANNEL_REQUEST_DIRECT, APF_STR_SIZE_OF(APF_OPEN_CHANNEL_REQUEST_DIRECT)) == 0) { }

			LME_ChannelOpenReplyFailure(lmemodule, channelOpenRequest.SenderChannel, OPEN_FAILURE_REASON_CONNECT_FAILED);
		}
		break;
	case APF_DISCONNECT: // (1) Intel AMT wants to completely disconnect. Not sure when this happens.
		{
			// First, we decode the message.
			LMSDEBUG("LME requested to disconnect with reason code 0x%08x\r\n", ((APF_DISCONNECT_REASON_CODE)ntohl(((APF_DISCONNECT_MESSAGE *)rxBuffer)->ReasonCode)));
			//printf("APF_DISCONNECT %d\r\n", ReasonCode);
			LME_Deinit(lmemodule);
		}
		break;
	case APF_SERVICE_REQUEST: // (5)
		{
			int service = 0;
			APF_SERVICE_REQUEST_MESSAGE *pMessage = (APF_SERVICE_REQUEST_MESSAGE *)rxBuffer;
			pMessage->ServiceNameLength = ntohl(pMessage->ServiceNameLength);
			if (pMessage->ServiceNameLength == 18) {
				if (memcmp(pMessage->ServiceName, "pfwd@amt.intel.com", 18) == 0) service = 1;
				else if (memcmp(pMessage->ServiceName, "auth@amt.intel.com", 18) == 0) service = 2;
			}

			if (service > 0)
			{
				if (service == 1)
				{
					LME_ServiceAccept(lmemodule, "pfwd@amt.intel.com");
					module->pfwdService = LME_STARTED;
				}
				else if (service == 2)
				{
					LME_ServiceAccept(lmemodule, "auth@amt.intel.com");
				}
			} else {
				LMSDEBUG("APF_SERVICE_REQUEST - APF_DISCONNECT_SERVICE_NOT_AVAILABLE\r\n");
				LME_Disconnect(lmemodule, APF_DISCONNECT_SERVICE_NOT_AVAILABLE);
				LME_Deinit(lmemodule);
				sem_post(&(module->Lock));
				return;
			}
		}
		break;
	case APF_CHANNEL_OPEN_CONFIRMATION: // (91) Intel AMT confirmation to an APF_CHANNEL_OPEN request.
		{
			// First, we decode the message.
			struct LMEChannel* channel;
			APF_CHANNEL_OPEN_CONFIRMATION_MESSAGE *pMessage = (APF_CHANNEL_OPEN_CONFIRMATION_MESSAGE *)rxBuffer;
			struct LMEChannelOpenReplySuccessMessage channelOpenReply;
			channelOpenReply.RecipientChannel = ntohl(pMessage->RecipientChannel);				// This is the identifier on our side.
			channelOpenReply.SenderChannel = ntohl(pMessage->SenderChannel);					// This is the identifier on the Intel AMT side.
			channelOpenReply.InitialWindow = ntohl(pMessage->InitialWindowSize);			    // This is the starting window size for flow control.
			//printf("APF_CHANNEL_OPEN_CONFIRMATION %d\r\n", channelOpenReply.RecipientChannel);
			channel = &(module->Sessions[SlotFromChannelId(channelOpenReply.RecipientChannel)]);// Get the current session for this message.
			ILibLifeTime_Remove(ILibGetBaseTimer(module->Chain), channel);						// Clear the timer
			module->PendingAmtConnection = 0;
			if (channel == NULL) break;															// This should never happen.
			if (channelOpenReply.RecipientChannel != (unsigned int)(channel->ourid)) { LMSDEBUG("APF_CHANNEL_OPEN_CONFIRMATION_MESSAGE - Channel Mismatch %d != %d\r\n", channelOpenReply.RecipientChannel, channel->ourid); break; }

			LMSDEBUG("MEI OPEN OK OUR:%d AMT:%d\r\n", channelOpenReply.RecipientChannel, channelOpenReply.SenderChannel);

			if (channel->status == LME_CS_PENDING_CONNECT)										// If the channel is in PENDING_CONNECT mode, move the session to connected state.
			{
				channel->amtid = channelOpenReply.SenderChannel;								// We have to set the Intel AMT identifier for this session.
				channel->txwindow = channelOpenReply.InitialWindow;								// Set the session txwindow.
				channel->status = LME_CS_CONNECTED;												// Now set the session as CONNECTED.
				LMSDEBUG("Channel %d now CONNECTED by AMT %p\r\n", channel->ourid, channel->socketmodule);
				if (channel->sockettype == 0)													// If the current socket is PAUSED, lets resume it so data can start flowing again.
				{
					ILibAsyncSocket_Resume(channel->socketmodule);								// TCP socket resume
				}
#ifndef NOCOMMANDER
				else
				{
					ILibLMS_ResumeWebSocket(channel->socketmodule);								// Web socket resume
				}
#endif
			}
			else if (channel->status == LME_CS_PENDING_LMS_DISCONNECT)							// If the channel is in PENDING_DISCONNECT, we have to disconnect the session now. Happens when we disconnect while connection is pending. We don't want to stop a channel during connection, that is bad.
			{
				channel->amtid = channelOpenReply.SenderChannel;								// We have to set the Intel AMT identifier for this session.
				LME_ChannelClose(&(module->MeConnection), channel->amtid, channel->ourid);		// Send the Intel AMT close. We keep the channel in LME_CS_PENDING_LMS_DISCONNECT state until the close is confirmed.
				LMSDEBUG("Channel %d now CONNECTED by AMT %p, but CLOSING it now\r\n", channel->ourid, channel->socketmodule);
			}
			else
			{
				// Here, we get an APF_CHANNEL_OPEN in an unexpected state, this should never happen.
				//printf("Channel %d, unexpected CONNECTED by AMT %d\r\n", channel->ourid, channel->socketmodule);
			}
			sem_post(&(module->Lock));
			ILibLMS_LaunchHoldingSessions(module);
			return;
		}
		break;
	case APF_CHANNEL_OPEN_FAILURE: // (92) Intel AMT rejected our connection attempt.
		{
			// First, we decode the message.
			struct LMEChannel* channel;
			APF_CHANNEL_OPEN_FAILURE_MESSAGE *pMessage = (APF_CHANNEL_OPEN_FAILURE_MESSAGE *)rxBuffer;
			struct LMEChannelOpenReplyFailureMessage channelOpenReply;
			channelOpenReply.RecipientChannel = ntohl(pMessage->RecipientChannel);				// This is the identifier on our side.
			channelOpenReply.ReasonCode = (OPEN_FAILURE_REASON)(ntohl(pMessage->ReasonCode));	// Get the error reason code.
			//printf("APF_CHANNEL_OPEN_FAILURE %d, %d\r\n", channelOpenReply.RecipientChannel, channelOpenReply.ReasonCode);
			channel = &(module->Sessions[SlotFromChannelId(channelOpenReply.RecipientChannel)]);// Get the current session for this message.
			ILibLifeTime_Remove(ILibGetBaseTimer(module->Chain), channel);						// Clear the timer
			module->PendingAmtConnection = 0;
			if (channel == NULL) break;															// This should never happen.
			if (channelOpenReply.RecipientChannel != (unsigned int)(channel->ourid)) { LMSDEBUG("APF_CHANNEL_OPEN_FAILURE - Channel Mismatch %d != %d\r\n", channelOpenReply.RecipientChannel, channel->ourid); break; }

			LMSDEBUG("**OPEN FAIL OUR:%d ERR:%d, ERRCNT:%d\r\n", channelOpenReply.RecipientChannel, channelOpenReply.ReasonCode, channel->errorcount);

			if (channel->errorcount++ >= 10 || channel->status == LME_CS_PENDING_LMS_DISCONNECT)
			{
				// Fail connection
				LMSDEBUG("Failed Connection - Channel %d now FREE by AMT, status = %d\r\n", channel->ourid, channel->status);
				channel->status = LME_CS_FREE;
				sem_post(&(module->Lock));
				ILibAsyncSocket_Disconnect(channel->socketmodule);
				ILibLMS_LaunchHoldingSessions(module);
				return;
			}
			else
			{
				// Try again
				ILibLMS_SetupConnection(module, SlotFromChannelId(channelOpenReply.RecipientChannel));
			}
		}
		break;
	case APF_CHANNEL_CLOSE: // (97) Intel AMT is closing this channel, we need to disconnect the LMS TCP connection
		{
			// First, we decode the message.
			struct LMEChannel* channel;
			APF_CHANNEL_CLOSE_MESSAGE *pMessage = (APF_CHANNEL_CLOSE_MESSAGE *)rxBuffer;
			struct LMEChannelCloseMessage channelClose;
			channelClose.RecipientChannel = ntohl(pMessage->RecipientChannel);					// This is the identifier on our side.
			// if (channelClose.RecipientChannel == 0) { printf("APF_CHANNEL_CLOSE channel %d\r\n"); break; }
			//printf("APF_CHANNEL_CLOSE channel %d\r\n", channelClose.RecipientChannel);
			channel = &(module->Sessions[SlotFromChannelId(channelClose.RecipientChannel)]);	// Get the current session for this message.
			if (channel == NULL) break;															// This should never happen, but it does. We get close on channel id 0 all the time.
			if (channelClose.RecipientChannel != (unsigned int)(channel->ourid)) { LMSDEBUG("APF_CHANNEL_CLOSE - Channel Mismatch %d != %d\r\n", channelClose.RecipientChannel, channel->ourid); break; }

			LMSDEBUG("CLOSE OUR:%d\r\n", channelClose.RecipientChannel);

			if (channel->status == LME_CS_CONNECTED)
			{
				if (ILibAsyncSocket_IsConnected(channel->socketmodule))
				{
					channel->status = LME_CS_PENDING_AMT_DISCONNECT;
					LMSDEBUG("Channel %d now PENDING_AMT_DISCONNECT by AMT, calling microstack disconnect %p\r\n", channel->ourid, channel->socketmodule);
					LME_ChannelClose(lmemodule, channel->amtid, channel->ourid);
					sem_post(&(module->Lock));
					if (channel->sockettype == 0)													// If the current socket is PAUSED, lets resume it so data can start flowing again.
					{
						ILibAsyncSocket_Disconnect(channel->socketmodule);							// TCP socket close
					}
#ifndef NOCOMMANDER
					else
					{
						ILibWebServer_WebSocket_Close(channel->socketmodule);						// Web socket close
					}
#endif
					sem_wait(&(module->Lock));
					channel->status = LME_CS_FREE;
					disconnects++;
					LMSDEBUG("Channel %d now FREE by AMT\r\n", channel->ourid);
				}
				else
				{
					channel->status = LME_CS_FREE;
					disconnects++;
					LMSDEBUG("Channel %d now FREE by AMT\r\n", channel->ourid);
				}
			}
			else if (channel->status == LME_CS_PENDING_LMS_DISCONNECT)
			{
				channel->status = LME_CS_FREE;
				disconnects++;
				LMSDEBUG("Channel %d now FREE by AMT\r\n", channel->ourid);
			}
			else
			{
				LMSDEBUG("Channel %d CLOSE, UNEXPECTED STATE %d\r\n", channel->ourid, channel->status);
			}
		}
		break;
	case APF_CHANNEL_DATA: // (94) Intel AMT is sending data that we must relay into an LMS TCP connection.
		{
			struct LMEChannel* channel;
			APF_CHANNEL_DATA_MESSAGE *pMessage = (APF_CHANNEL_DATA_MESSAGE *)rxBuffer;
			struct LMEChannelDataMessage channelData;
			enum ILibAsyncSocket_SendStatus r;
			channelData.MessageType = APF_CHANNEL_DATA;
			channelData.RecipientChannel = ntohl(pMessage->RecipientChannel);
			channelData.DataLength = ntohl(pMessage->DataLength);
			channelData.Data = (unsigned char*)rxBuffer + sizeof(APF_CHANNEL_DATA_MESSAGE);
			channel = &(module->Sessions[SlotFromChannelId(channelData.RecipientChannel)]);

			if (channel == NULL || channel->socketmodule == NULL || channel->status != LME_CS_CONNECTED)
			{
				sem_post(&(module->Lock));
				break;
			}

			if (channel->sockettype == 0)
			{
				r = ILibAsyncSocket_Send(channel->socketmodule, (char*)(channelData.Data), channelData.DataLength, ILibAsyncSocket_MemoryOwnership_USER); // TCP socket
			}
#ifndef NOCOMMANDER
			else
			{
				r = ILibWebServer_WebSocket_Send(channel->socketmodule, (char*)(channelData.Data), channelData.DataLength, ILibWebServer_WebSocket_DataType_TEXT, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete); // Web socket
			}
#endif

			channel->rxwindow += channelData.DataLength;
			if (r == ILibAsyncSocket_ALL_DATA_SENT && channel->rxwindow > 1024) { 
				LME_ChannelWindowAdjust(lmemodule, channel->amtid, channel->rxwindow); channel->rxwindow = 0; 
			}
		}
		break;
	case APF_CHANNEL_WINDOW_ADJUST: // 93
		{
			struct LMEChannel* channel;
			APF_WINDOW_ADJUST_MESSAGE *pMessage = (APF_WINDOW_ADJUST_MESSAGE *)rxBuffer;
			struct LMEChannelWindowAdjustMessage channelWindowAdjust;
			channelWindowAdjust.MessageType = APF_CHANNEL_WINDOW_ADJUST;
			channelWindowAdjust.RecipientChannel = ntohl(pMessage->RecipientChannel);
			channelWindowAdjust.BytesToAdd = ntohl(pMessage->BytesToAdd);
			channel = &(module->Sessions[SlotFromChannelId(channelWindowAdjust.RecipientChannel)]);
			if (channel == NULL || channel->status == LME_CS_FREE){
				sem_post(&(module->Lock));
				break;
			}
			channel->txwindow += channelWindowAdjust.BytesToAdd;
			if (channel->sockettype == 0)
			{
				ILibAsyncSocket_Resume(channel->socketmodule); // TCP socket
			}
#ifndef NOCOMMANDER
			else
			{
				ILibLMS_ResumeWebSocket(channel->socketmodule);	// Web socket resume
			}
#endif
		}
		break;
	case APF_PROTOCOLVERSION: // 192
		{
			APF_PROTOCOL_VERSION_MESSAGE *pMessage = (APF_PROTOCOL_VERSION_MESSAGE *)rxBuffer;
			struct LMEProtocolVersionMessage protVersion;
			protVersion.MajorVersion = ntohl(pMessage->MajorVersion);
			protVersion.MinorVersion = ntohl(pMessage->MinorVersion);
			protVersion.TriggerReason = (APF_TRIGGER_REASON)ntohl(pMessage->TriggerReason);

			switch (module->handshakingStatus)
			{
			case LME_AGREED:
			case LME_NOT_INITIATED:
				{
					LME_ProtocolVersion(lmemodule, 1, 0, protVersion.TriggerReason);
				}
			case LME_INITIATED:
				if (protVersion.MajorVersion != 1 || protVersion.MinorVersion != 0)
				{
					LMSDEBUG("LME Version %d.%d is not supported.\r\n", protVersion.MajorVersion, protVersion.MinorVersion);
					LME_Disconnect(lmemodule, APF_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED);
					LME_Deinit(lmemodule);
					sem_post(&(module->Lock)); 
					return;
				}
				module->AmtProtVersionMajor = protVersion.MajorVersion;
				module->AmtProtVersionMinor = protVersion.MinorVersion;
				module->handshakingStatus = LME_AGREED;
				break;
			default:
				LME_Disconnect(lmemodule, APF_DISCONNECT_BY_APPLICATION);
				LME_Deinit(lmemodule);
				break;
			}
		}
		break;
	case APF_USERAUTH_REQUEST: // 50
		{
			//printf("APF_USERAUTH_REQUEST\r\n");
			// _lme.UserAuthSuccess();
		}
		break;
	default:
		// Unknown request.
		//printf("**Unknown LME command: %d\r\n", ((unsigned char*)rxBuffer)[0]);
		LMSDEBUG("**Unknown LME command: %d\r\n", ((unsigned char*)rxBuffer)[0]);
		LME_Disconnect(lmemodule, APF_DISCONNECT_PROTOCOL_ERROR);
		LME_Deinit(lmemodule);
		break;
	}

	sem_post(&(module->Lock));
	if (disconnects > 0) ILibLMS_LaunchHoldingSessions(module); // If disconnects is set to anything, we have free session slots we can fill up.
}

void ILibLMS_OnReceive(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, char* buffer, int *p_beginPointer, int endPointer, ILibAsyncServerSocket_OnInterrupt *OnInterrupt, void **user, int *PAUSE)
{
	int r, maxread = endPointer;
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)ILibAsyncServerSocket_GetTag(AsyncServerSocketModule);
	struct LMEChannel* channel = (struct LMEChannel*)*user;

	UNREFERENCED_PARAMETER( AsyncServerSocketModule );
	UNREFERENCED_PARAMETER( ConnectionToken );
	UNREFERENCED_PARAMETER( OnInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );

	if (channel == NULL) return;
	sem_wait(&(module->Lock));
	if (channel->socketmodule != ConnectionToken) { sem_post(&(module->Lock)); ILibAsyncSocket_Disconnect(ConnectionToken); return; }
	if (channel->txwindow < endPointer) maxread = channel->txwindow;
	if (channel->status != LME_CS_CONNECTED || maxread == 0) { *PAUSE = 1; sem_post(&(module->Lock)); return; }
	r = LME_ChannelData(&(module->MeConnection), channel->amtid, maxread, (unsigned char*)buffer);
	LMSDEBUG("ILibLMS_OnReceive, status = %d, txwindow = %d, endPointer = %d, r = %d\r\n", channel->status, channel->txwindow, endPointer, r);
	if (r != maxread)
	{
		LMSDEBUG("ILibLMS_OnReceive, DISCONNECT %d\r\n", channel->ourid);
		sem_post(&(module->Lock));
		ILibAsyncSocket_Disconnect(ConnectionToken); // Drop the connection
		return;
	}
	channel->txwindow -= maxread;
	*p_beginPointer = maxread;
	sem_post(&(module->Lock)); 
}

void ILibLMS_SetupConnectionTimeout(void *obj)
{
	struct LMEChannel* channel = (struct LMEChannel*)obj;
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)channel->parent;
	if (module == NULL) return;

	sem_wait(&(module->Lock));
	module->PendingAmtConnection = 0;
	if (channel->errorcount++ >= 10 || channel->status == LME_CS_PENDING_LMS_DISCONNECT)
	{
		// Fail connection
		LMSDEBUG("Connection Timeout - Channel %d now FREE by TIMEOUT, status = %d\r\n", channel->ourid, channel->status);
		channel->status = LME_CS_FREE;
		sem_post(&(module->Lock));
		ILibAsyncSocket_Disconnect(channel->socketmodule);
		ILibLMS_LaunchHoldingSessions(module);
		return;
	}
	else
	{
		// Try again
		LMSDEBUG("Connection Timeout - Trying again channel %d, status = %d\r\n", channel->ourid, channel->status);
		//printf("Connection Timeout - Trying again channel %d, status = %d\r\n", channel->ourid, channel->status);
		ILibLMS_SetupConnection(channel->parent, channel->ourid - LMS_MIN_SESSIONID); // Try the connection again.
	}
	sem_post(&(module->Lock));
}

void ILibLMS_SetupConnection(struct ILibLMS_StateModule* module, int i)
{
	int rport = 0;
	char* laddr = NULL;
	//char tmp[256];
	struct sockaddr_in6 remoteAddress;

	if (module->Sessions[i].sockettype == 0)
	{
		// Fetch the socket remote TCP address
		ILibAsyncSocket_GetRemoteInterface(module->Sessions[i].socketmodule, (struct sockaddr*)&remoteAddress);
		if (remoteAddress.sin6_family == AF_INET6)
		{
			//ILibInet_ntop2(&remoteAddress, tmp, 256);
			//laddr = tmp;
			laddr = "::1"; // TODO: decode this properly into a string
			rport = ntohs(remoteAddress.sin6_port);
		}
		else
		{
			//ILibInet_ntop2(&remoteAddress, tmp, 256);
			//laddr = tmp;
			laddr = "127.0.0.1"; // TODO: decode this properly into a string
			rport = ntohs(((struct sockaddr_in*)(&remoteAddress))->sin_port);
		}
	}
	else
	{
		// Fetch the socket remote web socket address
		laddr = "127.0.0.1"; // TODO: decode this properly into a string
		rport = 123; // TODO: decode the remote port
	}

	// Add a timer, this will be used to retry the connection if AMT does not respond in time.
	ILibLifeTime_AddEx(ILibGetBaseTimer(module->Chain), &(module->Sessions[i]), 1000, ILibLMS_SetupConnectionTimeout, NULL);

	// Setup a new LME session
	module->PendingAmtConnection = (i + LMS_MIN_SESSIONID);
	LME_ChannelOpenForwardedRequest(&(module->MeConnection), (unsigned int)module->Sessions[i].ourid, laddr, module->Sessions[i].localport, laddr, rport);
	LMSDEBUG("ILibLMS_OnReceive, CONNECT, Slot %d, Id %d\r\n", i, module->Sessions[i].ourid);
}

void ILibLMS_LaunchHoldingSessions(struct ILibLMS_StateModule* module)
{
	int i, activecount = 0, launchcount;

	// If we currently have a pending connection, don't launch any new ones.
	if (module->PendingAmtConnection != 0) return;
	sem_wait(&(module->Lock));

	// Count the number of active sessionsLMSDEBUG
	activecount = LMS_MAX_SESSIONS;
	for (i = 0; i < LMS_MAX_SESSIONS; i++) { if (module->Sessions[i].status == LME_CS_FREE || module->Sessions[i].status == LME_CS_CONNECTION_WAIT) { activecount--; } }
	if (activecount >= LMS_MAX_CONNECTIONS) { sem_post(&(module->Lock)); return; }

	launchcount = LMS_MAX_CONNECTIONS - activecount;
	if (launchcount > 1) launchcount = 1; // Only launch one at a time
	for (i = 0; i < LMS_MAX_SESSIONS; i++)
	{
		if (module->Sessions[i].status == LME_CS_CONNECTION_WAIT)
		{
			module->Sessions[i].status = LME_CS_PENDING_CONNECT;
			LMSDEBUG("ILibLMS_OnConnect %d (RELEASE)\r\n", i + LMS_MIN_SESSIONID);
			//printf("ILibLMS_OnConnect %d (RELEASE)\r\n", i + LMS_MIN_SESSIONID);
			ILibLMS_SetupConnection(module, i);
			launchcount--;
		}
		if (launchcount == 0) break;
	}
	sem_post(&(module->Lock));
}

void ILibLMS_OnConnect(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void **user)
{
	int i, activecount = 0;
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)ILibAsyncServerSocket_GetTag(AsyncServerSocketModule);

	sem_wait(&(module->Lock));

	// Count the number of active sessions
	activecount = LMS_MAX_SESSIONS;
	for (i = 0; i < LMS_MAX_SESSIONS; i++) { 
		if (module->Sessions[i].status == LME_CS_FREE || module->Sessions[i].status == LME_CS_CONNECTION_WAIT) { 
			activecount--; 
		} 
	}
	
	// Look for an empty session
	if ((i = GetFreeSlot(module)) == -1)
	{
		sem_post(&(module->Lock)); 
		LMSDEBUG("ILibLMS_OnConnect NO SESSION SLOTS AVAILABLE\r\n");
		ILibAsyncSocket_Disconnect(ConnectionToken); // Drop the connection
		return;
	}

	// Clear the channel
	memset(&(module->Sessions[i]), 0, sizeof(struct LMEChannel));
	module->Sessions[i].parent = module;
	module->Sessions[i].amtid = -1;
	module->Sessions[i].ourid = (i + LMS_MIN_SESSIONID); // Don't use low number channel ID's
	module->Sessions[i].socketmodule = ConnectionToken;
	module->Sessions[i].localport = ILibAsyncServerSocket_GetPortNumber(AsyncServerSocketModule);
	*user = &(module->Sessions[i]);
	LMSDEBUG("New TCP connection using SLOT %d, ID %d\r\n", i, module->Sessions[i].ourid);

	if (activecount >= LMS_MAX_CONNECTIONS || module->PendingAmtConnection != 0)
	{
		module->Sessions[i].status = LME_CS_CONNECTION_WAIT;
		LMSDEBUG("ILibLMS_OnConnect %d (HOLDING)\r\n", i);
	}
	else
	{
		module->Sessions[i].status = LME_CS_PENDING_CONNECT;
		LMSDEBUG("ILibLMS_OnConnect TCP %d\r\n", i + LMS_MIN_SESSIONID);
		//printf("ILibLMS_OnConnect TCP %d\r\n", i + LMS_MIN_SESSIONID);
		ILibLMS_SetupConnection(module, i);
	}

	sem_post(&(module->Lock));
}

void ILibLMS_OnDisconnect(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user)
{
	int disconnects = 0;
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)ILibAsyncServerSocket_GetTag(AsyncServerSocketModule);
	struct LMEChannel* channel = (struct LMEChannel*)user;
	UNREFERENCED_PARAMETER( ConnectionToken );

	sem_wait(&(module->Lock)); 

	if (channel == NULL || channel->socketmodule != ConnectionToken)
	{
		LMSDEBUG("****ILibLMS_OnDisconnect EXIT\r\n");
		sem_post(&(module->Lock));
		return;
	}

	LMSDEBUG("ILibLMS_OnDisconnect, Channel %d, %p\r\n", channel->ourid, ConnectionToken);

	if (channel->status == LME_CS_CONNECTED)
	{
		channel->status = LME_CS_PENDING_LMS_DISCONNECT;
		if (channel->amtid!=-1 && !LME_ChannelClose(&(module->MeConnection), channel->amtid, channel->ourid))
		{
			channel->status = LME_CS_FREE;
			disconnects++;
			LMSDEBUG("Channel %d now FREE by LMS because of failed close\r\n", channel->ourid);
		}
		else
		{
			LMSDEBUG("Channel %d now PENDING_LMS_DISCONNECT by LMS\r\n", channel->ourid);
			//LMSDEBUG("LME_ChannelClose OK\r\n");
		}
	}
	else if (channel->status == LME_CS_PENDING_CONNECT)
	{
		channel->status = LME_CS_PENDING_LMS_DISCONNECT;
		LMSDEBUG("Channel %d now PENDING_DISCONNECT by LMS\r\n", channel->ourid);
	}
	else if (channel->status == LME_CS_CONNECTION_WAIT)
	{
		channel->status = LME_CS_FREE;
		disconnects++;
		LMSDEBUG("Channel %d now FREE by LMS\r\n", channel->ourid);
	}
	LMSDEBUG("ILibLMS_OnDisconnect, DISCONNECT %d, status = %d\r\n", channel->ourid, channel->status);
	sem_post(&(module->Lock));
	
	if (disconnects > 0) ILibLMS_LaunchHoldingSessions(module);
}

void ILibLMS_OnSendOK(ILibAsyncServerSocket_ServerModule AsyncServerSocketModule, ILibAsyncServerSocket_ConnectionToken ConnectionToken, void *user)
{
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)ILibAsyncServerSocket_GetTag(AsyncServerSocketModule);
	struct LMEChannel* channel = (struct LMEChannel*)user;
	UNREFERENCED_PARAMETER( ConnectionToken );

	// Ok to send more on this socket, adjust the window
	sem_wait(&(module->Lock)); 
	if (channel->rxwindow != 0)
	{
		LMSDEBUG("LME_ChannelWindowAdjust id=%d, rxwindow=%d\r\n", channel->amtid, channel->rxwindow);
		LME_ChannelWindowAdjust(&(module->MeConnection), channel->amtid, channel->rxwindow);
		channel->rxwindow = 0;
	}
	sem_post(&(module->Lock));
}

// Private method called when the chain is destroyed, we want to do our cleanup here
void ILibLMS_Destroy(void *object)
{
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)object;
	UNREFERENCED_PARAMETER( object );

	sem_wait(&(module->Lock)); 
	LME_Disconnect(&(module->MeConnection), APF_DISCONNECT_BY_APPLICATION);
	LME_Exit(&(module->MeConnection));
	sem_destroy(&(module->Lock));
	if (IlibExternLMS == module) { IlibExternLMS = NULL; }	// Clear the global reference to the the LMS module.
}

#ifndef NOCOMMANDER
void ILibLMS_WebSocketOnSendOK(struct ILibWebServer_Session *sender)
{
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)sender->User;
	struct LMEChannel* channel = (struct LMEChannel*)sender->User2;

	// Ok to send more on this socket, adjust the window
	sem_wait(&(module->Lock));
	if (channel->rxwindow != 0)
	{
		LMSDEBUG("LME_ChannelWindowAdjust id=%d, rxwindow=%d\r\n", channel->amtid, channel->rxwindow);
		LME_ChannelWindowAdjust(&(module->MeConnection), channel->amtid, channel->rxwindow);
		channel->rxwindow = 0;
	}
	sem_post(&(module->Lock));
}

void ILibLMS_ResumeWebSocket(struct ILibWebServer_Session *session)
{
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)session->User;
	struct LMEChannel* channel = (struct LMEChannel*)session->User2;
	LMSDEBUG("ILibLMS_ResumeWebSocket %d, %p\r\n", channel->ourid, channel->socketmodule);

	if (channel->pending != NULL)
	{
		int r, maxread = channel->pendingcount;
		if (channel->txwindow < maxread) { maxread = channel->txwindow; }
		if (channel->status != LME_CS_CONNECTED || maxread == 0) return;
		r = LME_ChannelData(&(module->MeConnection), channel->amtid, maxread, (unsigned char*)(channel->pending + channel->pendingptr));
		LMSDEBUG("ILibLMS_OnReceive, status = %d, txwindow = %d, endPointer = %d, r = %d\r\n", channel->status, channel->txwindow, maxread, r);
		if (r != maxread) return;
		channel->txwindow -= maxread;
		if (maxread != channel->pendingcount)
		{
			// More data still to send
			channel->pendingptr += maxread;
			channel->pendingcount -= maxread;
		}
		else
		{
			// We are done
			free(channel->pending);
			channel->pending = 0;
			channel->pendingcount = 0;
			channel->pendingptr = 0;
			LMSDEBUG("ILibWebServer_Resume %d, %p\r\n", channel->ourid, channel->socketmodule);
			ILibWebServer_Resume(channel->socketmodule);								// Web socket resume
		}
	}
	else
	{
		LMSDEBUG("ILibWebServer_Resume %d, %p\r\n", channel->ourid, channel->socketmodule);
		ILibWebServer_Resume(channel->socketmodule);								// Web socket resume
	}
}
#endif

// Get Intel(R) AMT Version
int info_GetAmtVersion()
{
	int len;
	HECI_VERSION heci_version;
	CODE_VERSIONS codeVersions;

	// Look for HECI driver
	if (heci_Init(NULL, 0) == 0 || heci_GetHeciVersion(NULL, &heci_version) == 0) { return 0; }

	// Find the Intel AMT version and use that if possible
	if (pthi_GetCodeVersions(&codeVersions) == 0)
	{
		for (len = 0; len < (int)(codeVersions.VersionsCount); len++)
		{
			if (strcmp((char*)(codeVersions.Versions[len].Description.String), "AMT") == 0)
			{
				int xa, xb, xc;
				if (sscanf_s((char*)(codeVersions.Versions[len].Version.String), "%d.%d.%d", &xa, &xb, &xc) == 3)
				{
					return (((unsigned long)xa) << 16) | (((unsigned long)xb) << 8) | ((unsigned long)xc);
				}
			}
		}
	}

	return 0;
}

// Get Intel(R) AMT information
int info_GetMeInformation(char** data, int loginmode)
{
	int len, controlmode;
	HECI_VERSION heci_version;
	CFG_PROVISIONING_MODE provisioningmode;
	AMT_PROVISIONING_STATE provisioningstate;
	CODE_VERSIONS codeVersions;
	AMT_EHBC_STATE ehbcstate = EHBC_STATE_DISABLED;
	AMT_BOOLEAN b;
	AMT_ANSI_STRING dnsSuffix;
	UINT8 DedicatedMac[6];
	UINT8 HostMac[6];
	AMT_HASH_HANDLES hashHandles;
	CERTHASH_ENTRY hashEntry;
	unsigned long version = 0;
	unsigned int i;
	int flags = 0;
	int ptr = 0;

	// Setup the response
	*data = (char*)malloc(24000);
	((unsigned short*)(*data))[0] = 1;
	ptr += 2;

	// Put login mode
	ptr += snprintf(*data + ptr, 24000 - ptr, "{\"LoginMode\":%d,", loginmode);

	// Look for HECI driver
	if (heci_Init(NULL, 0) == 0 || heci_GetHeciVersion(NULL, &heci_version) == 0) { return ptr; }
	ptr += snprintf(*data + ptr, 24000 - ptr, "\"MeiVersion\":\"%d.%d.%d\",\"Versions\":{", heci_version.major, heci_version.minor, heci_version.hotfix);

	// Find the Intel AMT version and use that if possible
	if (pthi_GetCodeVersions(&codeVersions) == 0)
	{
		int first = 1;
		for (len = 0; len < (int)(codeVersions.VersionsCount); len++)
		{
			if (first == 0) { ptr += snprintf(*data + ptr, 24000 - ptr, ","); } else { first = 0; }
			ptr += snprintf(*data + ptr, 24000 - ptr, "\"%s\":\"%s\"", (char*)(codeVersions.Versions[len].Description.String), (char*)(codeVersions.Versions[len].Version.String));

			if (strcmp((char*)(codeVersions.Versions[len].Description.String), "AMT") == 0)
			{
				int xa, xb, xc;
				if (sscanf((char*)(codeVersions.Versions[len].Version.String), "%d.%d.%d", &xa, &xb, &xc) == 3)
				{
					version = (((unsigned long)xa) << 16) | (((unsigned long)xb) << 8) | ((unsigned long)xc);
				}
			}
		}
	}
	ptr += snprintf(*data + ptr, 24000 - ptr, "},");

	// Get provisioning state & mode
	pthi_GetProvisioningState(&provisioningstate);
	pthi_GetProvisioningMode(&provisioningmode, &b);

	// Get EHBC settings
	if (version > 0x00080100 && pthi_GetStateEHBC(&ehbcstate) == 0)
	{
		// Add EHBC flag
		if (ehbcstate == 1) flags += 0x00000001;			// EHBC enabled
	}

	// Get current control mode
	if (version > 0x00060100 && pthi_GetControlMode(&controlmode) == 0) {
		if (controlmode == 1) { flags += 0x00000002; }			// Client Control Mode enabled
		else if (controlmode == 2) { flags += 0x00000004; }		// Admin Control Mode enabled
	}

	// Get wired MAC addresses
	if (pthi_GetMacAddresses(DedicatedMac, HostMac) != 0) { heci_Init(NULL, 0); } // If the call fails, we need to re-initalize HECI in all cases. But this case does happen.

	// Get the current DNS suffix
	if (pthi_GetDnsSuffix(&dnsSuffix) == 0 && dnsSuffix.Length != 0)
	{
		ptr += snprintf(*data + ptr, 24000 - ptr, "\"DnsSuffix\":\"%s\",", dnsSuffix.Buffer);
	}

	// Get OS Hostname, this is used to set the computer name within Intel AMT at activation time.
	if (gethostname(ILibScratchPad, sizeof(ILibScratchPad)) == 0)
	{
		ptr += snprintf(*data + ptr, 24000 - ptr, "\"OsHostname\":\"%s\",", ILibScratchPad);
	}

	/*
	// Get OS FQDN
	for (k = 0; k < ComputerNameMax; k++)
	{
		i = sizeof(ILibScratchPad);
		if (GetComputerNameExA(k, ILibScratchPad, &i) != 0)
		{
			ptr += snprintf(*data + ptr, 24000 - ptr, "\"osname%d\":\"%s\",", k, ILibScratchPad);
		}
	}
	*/

	// Enumerate all provisioning hashes
	memset(&hashHandles, 0, sizeof(AMT_HASH_HANDLES));
	if (pthi_EnumerateHashHandles(&hashHandles) == 0)
	{
		int first = 1;
		ptr += snprintf(*data + ptr, 24000 - ptr, "\"TrustedHashes\":[");
		for (i = 0; i < hashHandles.Length; i++)
		{
			if (pthi_GetCertificateHashEntry(hashHandles.Handles[i], &hashEntry) == 0)
			{
				int hl = 0;
				char hexbuf[500];
				char namebuf[500];

				if (hashEntry.HashAlgorithm == CERT_HASH_ALGORITHM_MD5) hl = 16;
				else if (hashEntry.HashAlgorithm == CERT_HASH_ALGORITHM_SHA1) hl = 20;
				else if (hashEntry.HashAlgorithm == CERT_HASH_ALGORITHM_SHA256) hl = 32;
				else if (hashEntry.HashAlgorithm == CERT_HASH_ALGORITHM_SHA512) hl = 64;
				util_tohex((char*)hashEntry.CertificateHash, hl, hexbuf);

				if (first == 0) { ptr += snprintf(*data + ptr, 24000 - ptr, ","); } else { first = 0; }
				memcpy_s(namebuf, 500, hashEntry.Name.Buffer, hashEntry.Name.Length);
				namebuf[hashEntry.Name.Length] = 0;
				ptr += snprintf(*data + ptr, 24000 - ptr, "{\"Active\":%d,\"Default\":%d,\"HashAlgorithm\":%d,\"Name\":\"%s\",\"Hash\":\"%s\"}", hashEntry.IsActive, hashEntry.IsDefault, hashEntry.HashAlgorithm, namebuf, hexbuf);
			}
		}
		ptr += snprintf(*data + ptr, 24000 - ptr, "],");
	}

	// Encode the command
	ptr += snprintf(*data + ptr, 24000 - ptr, "\"Flags\":%d,\"ProvisioningMode\":%d,\"ProvisioningState\":%d}", flags, provisioningmode, provisioningstate);
	(*data)[ptr] = 0;
	return ptr;
}

#ifndef NOCOMMANDER
// Handle LMS control traffic
void ILibLMS_ProcessLmsControlCommand(struct ILibLMS_StateModule* module, struct ILibWebServer_Session *sender, char* cmd, int cmdlen)
{
	int auth = (int)(sender->User5);
	unsigned short cmdid = ((unsigned short*)(cmd))[0];
	((unsigned short*)(ILibScratchPad2))[0] = cmdid; // Useful for building the response

	UNREFERENCED_PARAMETER(module);
	UNREFERENCED_PARAMETER(cmdlen);

	switch (cmdid) {
		case 1: // Request basic Intel AMT information (CMD = 1)
		{
			char* data;
			int len = info_GetMeInformation(&data, auth);
			ILibWebServer_WebSocket_Send(sender, data, len, ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			free(data);
			break;
		}
		case 2: // Intel AMT MEI Unprovision (CMD = 2)
		{
			int mode;
			AMT_STATUS status;
			if (auth < 2 || heci_Init(NULL, 0) == 0) break; // Admin only
			mode = ((int*)(cmd + 2))[0];
			status = pthi_Unprovision((CFG_PROVISIONING_MODE)mode); // 1 = Enterprise
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6, ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
		case 3: // Intel AMT MEI GetLocalSystemAccount (CMD = 3)
		{
			AMT_STATUS status;
			LOCAL_SYSTEM_ACCOUNT* account = (LOCAL_SYSTEM_ACCOUNT*)(ILibScratchPad2 + 6);
			if (auth < 2 || heci_Init(NULL, 0) == 0) break; // Admin only
			status = pthi_GetLocalSystemAccount(account);
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6 + sizeof(LOCAL_SYSTEM_ACCOUNT), ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
		case 4: // Instruct Intel AMT to start remote configuration (CMD = 4)
		{
			unsigned int status = pthi_StartConfiguration();
			if (auth < 2) break; // Admin only
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6, ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
		case 5: // Instruct Intel AMT to stop remote configuration (CMD = 5)
		{
			unsigned int status = pthi_StopConfiguration();
			if (auth < 2) break; // Admin only
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6, ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
		case 6: // Instruct Intel AMT connect CIRA (CMD = 6)
		{
			unsigned int status = pthi_OpenUserInitiatedConnection();
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6, ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
		case 7: // Instruct Intel AMT disconnect CIRA (CMD = 7)
		{
			unsigned int status = pthi_CloseUserInitiatedConnection();
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6, ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
		case 8: // Get Intel AMT CIRA State (CMD = 8)
		{
			AMT_STATUS status;
			REMOTE_ACCESS_STATUS *remoteAccessStatus = (REMOTE_ACCESS_STATUS*)(ILibScratchPad2 + 6);
			if (heci_Init(NULL, 0) == 0) break;
			status = pthi_GetRemoteAccessConnectionStatus(remoteAccessStatus);
			((unsigned int*)(ILibScratchPad2 + 2))[0] = status;
			ILibWebServer_WebSocket_Send(sender, ILibScratchPad2, 6 + sizeof(REMOTE_ACCESS_STATUS), ILibWebServer_WebSocket_DataType_BINARY, ILibAsyncSocket_MemoryOwnership_USER, ILibWebServer_WebSocket_FragmentFlag_Complete);
			break;
		}
	}
}

#define RESPONSE_HEADER_TEMPLATE_JS				"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: text/javascript"
#define RESPONSE_HEADER_TEMPLATE_JSON			"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: text/html\r\nContent-Encoding: application/json"
#define RESPONSE_HEADER_TEMPLATE_HTML			"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: text/html"
#define RESPONSE_HEADER_TEMPLATE_HTML_GZ		"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: text/html\r\nContent-Encoding: gzip"
#define RESPONSE_HEADER_TEMPLATE_HTML_GZ_ETAG	"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: text/html\r\nContent-Encoding: gzip\r\nETag: %s"
#define RESPONSE_HEADER_TEMPLATE_TEXT			"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: text/plain"
#define RESPONSE_HEADER_TEMPLATE_BIN			"\r\nServer: MicroLMS\r\nCache-Control: max-age=0, no-cache\r\nX-Frame-Options: DENY\r\nContent-Type: application/octet-stream"
#define RESPONSE_HEADER_SERVER					"\r\nServer: MicroLMS\r\n"

// Perform HTTP digest auth when needed
// Returns: 0 if no login is possible, 1 if it's possible and 2 is logged in.
int ILibLMS_HttpServerPerformDigestAuth(struct ILibWebServer_Session *session)
{
#ifdef WIN32
	char* xusername;
	char* username = NULL;
	int usernamelen, passwordlen;
	char* password = NULL;
	int r = -1;

	usernamelen = ILibLMS_getregistryA("username", &username);
	passwordlen = ILibLMS_getregistryA("password", &password);
	if (usernamelen == 0 || passwordlen == 0) { r = 0; }
	if (r == -1) { if (usernamelen == 1 && username[0] == '*') r = 3; } // If username set to '*', we are always admin
	if (r == -1) { if (ILibWebServer_Digest_IsAuthenticated(session, "MicroLMS", 8) == 0) r = 1; }
	if (r == -1) { xusername = ILibWebServer_Digest_GetUsername(session); if (xusername == NULL || (int)strnlen_s(xusername, 4096) != usernamelen || strncasecmp(xusername, username, usernamelen) != 0) { r = 1; } }
	if (r == -1) { r = (ILibWebServer_Digest_ValidatePassword(session, password, passwordlen) == 1) ? 2 : 1; }
	if (username != NULL) free(username);
	if (password != NULL) free(password);
	return r;
#else
	return 0;
#endif
}

// Handles all of the HTTP server requests
void ILibLMS_HttpServerSessionReceiveSink(struct ILibWebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, ILibWebServer_DoneFlag done)
{
	int responseSent = 0;
	struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)sender->User;
	if (header == NULL || InterruptFlag != 0) return;

	//MSG("HTTP Server Request: %s %s\r\n", header->Directive, header->DirectiveObj);
	//LMSDEBUG("HTTP Server Request: %s %s\r\n", header->Directive, header->DirectiveObj);

	// This is the LMS relay web socket, all traffic routed to Intel AMT
	if (header->DirectiveObjLength >= 14 && strncasecmp(header->DirectiveObj, "/webrelay.ashx", 14) == 0)
	{
		// Web socket handeling
		switch (ILibWebServer_WebSocket_GetDataType(sender))
		{
			case ILibWebServer_WebSocket_DataType_TEXT:
			case ILibWebServer_WebSocket_DataType_BINARY:
			{
				struct LMEChannel* channel = (struct LMEChannel*)sender->User2;

				if (done == 1)
				{
					// We got a disconnection, process it
					int disconnects = 0;

					sem_wait(&(module->Lock));

					if (channel == NULL || channel->socketmodule != sender)
					{
						LMSDEBUG("****ILibLMS_OnDisconnect EXIT\r\n");
						sem_post(&(module->Lock));
						return;
					}

					LMSDEBUG("ILibLMS_OnDisconnect, Channel %d, %p\r\n", channel->ourid, sender);
					//LMSDEBUG("SOCK CLOSE OUR:%d AMT:%d\r\n", channel->ourid, channel->amtid);

					if (channel->status == LME_CS_CONNECTED)
					{
						channel->status = LME_CS_PENDING_LMS_DISCONNECT;
						if (!LME_ChannelClose(&(module->MeConnection), channel->amtid, channel->ourid))
						{
							LMSDEBUG("LME_ChannelClose FAILED\r\n");
							channel->status = LME_CS_FREE;
							disconnects++;
							LMSDEBUG("Channel %d now FREE by LMS because of failed close\r\n", channel->ourid);
						}
						else
						{
							LMSDEBUG("Channel %d now PENDING_LMS_DISCONNECT by LMS\r\n", channel->ourid);
							//LMSDEBUG("LME_ChannelClose OK\r\n");
						}
					}
					else if (channel->status == LME_CS_PENDING_CONNECT)
					{
						channel->status = LME_CS_PENDING_LMS_DISCONNECT;
						LMSDEBUG("Channel %d now PENDING_DISCONNECT by LMS\r\n", channel->ourid);
					}
					else if (channel->status == LME_CS_CONNECTION_WAIT)
					{
						channel->status = LME_CS_FREE;
						disconnects++;
						LMSDEBUG("Channel %d now FREE by LMS\r\n", channel->ourid);
					}
					LMSDEBUG("ILibLMS_OnDisconnect, DISCONNECT %d, status = %d\r\n", channel->ourid, channel->status);
					channel->socketmodule = NULL;
					sem_post(&(module->Lock));

					if (disconnects > 0) ILibLMS_LaunchHoldingSessions(module);
				}
				else
				{
					// We got binary data, process it
					int r, maxread = endPointer;

					if (channel == NULL) return;
					sem_wait(&(module->Lock));
					if (channel->socketmodule != sender) { sem_post(&(module->Lock)); ILibWebServer_WebSocket_Close(sender); return; }
					if (channel->txwindow < endPointer) maxread = channel->txwindow;
					if (channel->status != LME_CS_CONNECTED || maxread == 0)
					{
						// Pause the web socket and stuff the rest of the data from this fragment into a pending buffer
						LMSDEBUG("ILibWebServer_Pause %d, %p\r\n", channel->ourid, channel->socketmodule);
						ILibWebServer_Pause(sender);
						channel->pendingcount = endPointer;
						channel->pending = malloc(endPointer);
						channel->pendingptr = 0;
						memcpy_s(channel->pending, bodyBuffer, endPointer, endPointer);
						sem_post(&(module->Lock));
						return;
					}
					r = LME_ChannelData(&(module->MeConnection), channel->amtid, maxread, (unsigned char*)bodyBuffer);
					LMSDEBUG("ILibLMS_OnReceive, status = %d, txwindow = %d, endPointer = %d, r = %d\r\n", channel->status, channel->txwindow, endPointer, r);
					if (r != maxread)
					{
						LMSDEBUG("ILibLMS_OnReceive, DISCONNECT %d\r\n", channel->ourid);
						sem_post(&(module->Lock));
						ILibWebServer_WebSocket_Close(sender); // Drop the connection
						return;
					}
					channel->txwindow -= maxread;
					*beginPointer = maxread;
					if (maxread != endPointer)
					{
						// Pause the web socket and stuff the rest of the data from this fragment into a pending buffer
						LMSDEBUG("ILibWebServer_Pause %d, %p\r\n", channel->ourid, channel->socketmodule);
						ILibWebServer_Pause(sender);
						channel->pendingcount = endPointer - maxread;
						channel->pending = malloc(endPointer - maxread);
						channel->pendingptr = 0;
						memcpy_s(channel->pending, bodyBuffer + maxread, endPointer - maxread, channel->pendingcount);
					}
					sem_post(&(module->Lock));
				}
				break;
			}
			case ILibWebServer_WebSocket_DataType_REQUEST:
			{
				int i, sessionid = -1, activecount = 0;
				struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)sender->User;

				// Cross-site requests are not allowed
				if (ILibWebServer_IsCrossSiteRequest(sender) != NULL) {
					ILibWebServer_StreamHeader_Raw(sender, 400, "400 - Access Denied", RESPONSE_HEADER_TEMPLATE_HTML, ILibAsyncSocket_MemoryOwnership_STATIC);
					ILibWebServer_StreamBody(sender, "400 - Access Denied", 19, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
				}

				// If this is a websocket request, upgrade it.
				if (ILibWebServer_UpgradeWebSocket(sender, 8096) == 1) { ILibWebServer_StreamBody(sender, "404 - File not found", 20, ILibAsyncSocket_MemoryOwnership_STATIC, 1); return; }

				// Setup a connection to Guardpost
				sender->OnSendOK = &ILibLMS_WebSocketOnSendOK;
				
				sem_wait(&(module->Lock));

				// Count the number of active sessions
				activecount = LMS_MAX_SESSIONS;
				for (i = 0; i < LMS_MAX_SESSIONS; i++) { 
					if (module->Sessions[i].status == LME_CS_FREE || module->Sessions[i].status == LME_CS_CONNECTION_WAIT) { 
						activecount--; 
					} 
				}

				// Look for an empty session
				sessionid = GetFreeSlot(module);
				if (sessionid == -1)
				{
					sem_post(&(module->Lock));
					LMSDEBUG("ILibLMS_OnConnect NO SESSION SLOTS AVAILABLE\r\n");
					ILibWebServer_WebSocket_Close(sender); // Drop the connection
					return;
				}
				i = sessionid;

				// Clear the channel
				memset(&(module->Sessions[i]), 0, sizeof(struct LMEChannel));
				module->Sessions[i].amtid = -1;
				module->Sessions[i].ourid = (i + LMS_MIN_SESSIONID); // Don't use low channel ID's
				module->Sessions[i].sockettype = 1; // Websocket Type
				module->Sessions[i].socketmodule = sender;
				module->Sessions[i].localport = 16992; // Use this port, may use 16993 in the future if passed as URL argument.
				LMSDEBUG("New websocket connection using SLOT %d, ID %d\r\n", i, module->Sessions[i].ourid);
				sender->User2 = &(module->Sessions[i]);

				if (activecount >= LMS_MAX_CONNECTIONS || module->PendingAmtConnection != 0)
				{
					module->Sessions[i].status = LME_CS_CONNECTION_WAIT;
					LMSDEBUG("ILibLMS_OnConnect %d (HOLDING)\r\n", i);
				}
				else
				{
					module->Sessions[i].status = LME_CS_PENDING_CONNECT;
					//printf("ILibLMS_OnConnect WEB %d\r\n", i + LMS_MIN_SESSIONID);
					LMSDEBUG("ILibLMS_OnConnect WEB %d\r\n", i + LMS_MIN_SESSIONID);
					ILibLMS_SetupConnection(module, i);
				}

				sem_post(&(module->Lock));
				break;
			}
			case ILibWebServer_WebSocket_DataType_UNKNOWN:
			{
				// This is a normal HTTP request, don't handle this.
				ILibWebServer_StreamBody(sender, "404 - File not found", 20, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
				break;
			}
		}
		return;
	}

	// This is the LMS control channel, used for MEI and other things
	if (header->DirectiveObjLength >= 9 && strncasecmp(header->DirectiveObj, "/lms.ashx", 9) == 0)
	{
		// Web socket handeling
		int type = ILibWebServer_WebSocket_GetDataType(sender);
		switch (type)
		{
			case ILibWebServer_WebSocket_DataType_TEXT:
			case ILibWebServer_WebSocket_DataType_BINARY:
			{
				struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)sender->User;
				//struct LMEChannel* channel = (struct LMEChannel*)sender->User2;

				// Web socket disconnection
				if (done == 1)
				{
					//LMSDEBUG("LMS-CTRL: Disconnected.\r\n");
					return; 
				}
			
				// We got data, process it
				//LMSDEBUG("LMS-CTRL: Got %d bytes on LMS control socket, traffic echo.\r\n", endPointer);

				// Process LMS control command
				ILibLMS_ProcessLmsControlCommand(module, sender, bodyBuffer, endPointer);

				// Set the beginpointer to the end since we consumed all data.
				*beginPointer = endPointer;

				break;
			}
			case ILibWebServer_WebSocket_DataType_REQUEST:
			{
				// struct ILibLMS_StateModule* module = (struct ILibLMS_StateModule*)sender->User;

				// Store the authention state in the web socket session
				sender->User5 = ILibLMS_HttpServerPerformDigestAuth(sender);

				// Cross-site requests are not allowed
				if (ILibWebServer_IsCrossSiteRequest(sender) != NULL) {
					ILibWebServer_StreamHeader_Raw(sender, 400, "400 - Access Denied", RESPONSE_HEADER_TEMPLATE_HTML, ILibAsyncSocket_MemoryOwnership_STATIC);
					ILibWebServer_StreamBody(sender, "400 - Access Denied", 19, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
				}

				// If this is a websocket request, upgrade it.
				if (ILibWebServer_UpgradeWebSocket(sender, 8096) == 1) { ILibWebServer_StreamBody(sender, "404 - File not found", 20, ILibAsyncSocket_MemoryOwnership_STATIC, 1); return; }

				// If we plan on sending lots of data, we will need this. Probably not needed since we are localhost only and data is small.
				// sender->OnSendOK = &ILibLMS_CtrlWebSocketOnSendOK;

				break;
			}
			case ILibWebServer_WebSocket_DataType_UNKNOWN:
			{
				// This is a normal HTTP request, don't handle this.
				ILibWebServer_StreamBody(sender, "404 - File not found", 20, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
				break;
			}
		}
		return;
	}

	// If the query is not complete, stop here and wait for the completed body.
	if (done != ILibWebServer_DoneFlag_Done) return;

	// Cross-site requests are not allowed
	if (ILibWebServer_IsCrossSiteRequest(sender) != NULL) {
		ILibWebServer_StreamHeader_Raw(sender, 400, "400 - Access Denied", RESPONSE_HEADER_TEMPLATE_HTML, ILibAsyncSocket_MemoryOwnership_STATIC);
		ILibWebServer_StreamBody(sender, "400 - Access Denied", 19, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
	}

	if (header->DirectiveObjLength == 7 && strncasecmp(header->DirectiveObj, "/meinfo", 7) == 0)
	{
		char* data;
		int len = info_GetMeInformation(&data, ILibLMS_HttpServerPerformDigestAuth(sender));
		ILibWebServer_StreamHeader_Raw(sender, 200, "OK", RESPONSE_HEADER_TEMPLATE_JSON, ILibAsyncSocket_MemoryOwnership_STATIC);
		ILibWebServer_StreamBody(sender, data + 2, len - 2, ILibAsyncSocket_MemoryOwnership_USER, 1);
		free(data);
		responseSent = 1;
	}

	if (header->DirectiveObjLength == 9 && strncasecmp(header->DirectiveObj, "/channels", 9) == 0)
	{
		int i, ptr = 0;
		ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "<html><head><meta http-equiv=\"refresh\" content=\"1\"></head><body><pre>");

		for (i = 0; i < LMS_MAX_SESSIONS; i++)
		{
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "status: %d, ", module->Sessions[i].status);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "our: %d, ", module->Sessions[i].ourid);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "amt: %d, ", module->Sessions[i].amtid);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "type: %d, ", module->Sessions[i].sockettype);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "tx: %d, ", module->Sessions[i].txwindow);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "rx: %d, ", module->Sessions[i].rxwindow);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "port: %d, ", module->Sessions[i].localport);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "errors: %d, ", module->Sessions[i].errorcount);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "pending: %d, ", module->Sessions[i].pendingcount);
			ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "pendingptr: %d\r\n", module->Sessions[i].pendingptr);
		}

		ptr += snprintf(ILibScratchPad2 + ptr, sizeof(ILibScratchPad2) - ptr, "</pre></body></html>");
		ILibWebServer_StreamHeader_Raw(sender, 200, "OK", RESPONSE_HEADER_TEMPLATE_HTML, ILibAsyncSocket_MemoryOwnership_STATIC);
		ILibWebServer_StreamBody(sender, ILibScratchPad2, ptr, ILibAsyncSocket_MemoryOwnership_USER, 1);
		responseSent = 1;
	}
	
	if (responseSent == 0 && module->WebDir != NULL)
	{
#if !defined(WIN32)
		int i;
#endif
		int len = strnlen_s(module->WebDir, sizeof(ILibScratchPad));
		//int SessionAuth = 0;
		char* HeaderTemplate = RESPONSE_HEADER_TEMPLATE_BIN;
		char* tmp;

		if (header->DirectiveObjLength == 15 && strncasecmp(header->DirectiveObj, "/authindex.html", 15) == 0 && ILibLMS_HttpServerPerformDigestAuth(sender) == 1) {
			// Send authentication required response if the auth page is requested.
			ILibWebServer_Digest_SendUnauthorized(sender, "MicroLMS", 8, "<html><head><meta http-equiv='refresh' content='0; url = /index.html' /></head>Authentication failed. <a href='/authindex.html'>Retry here</a>.</html>", 151);
			return;
		}

		memcpy_s(ILibScratchPad, module->WebDir, sizeof(ILibScratchPad), len);
		if (header->DirectiveObjLength == 1 || (header->DirectiveObjLength == 11 && strncasecmp(header->DirectiveObj, "/index.html", 11) == 0) || (header->DirectiveObjLength == 15 && strncasecmp(header->DirectiveObj, "/authindex.html", 15) == 0))
		{
			memcpy_s(ILibScratchPad + len, "\\index.html", sizeof(ILibScratchPad), 11);
			len += 11;
		}
		else
		{
			memcpy_s(ILibScratchPad + len, header->DirectiveObj, sizeof(ILibScratchPad) - len, header->DirectiveObjLength);
			len += header->DirectiveObjLength;
		}
		ILibScratchPad[len] = 0;

#if !defined(WIN32)
		// Replace all "\" for "/" to make this a correct Linux path. Windows does not seem to mind about this.
		for (i = 0; i < len; i++) { if (ILibScratchPad[i] == '\\') ILibScratchPad[i] = '/'; }
#endif

		// Send normal response
		tmp = strrchr(ILibScratchPad, '.');
		if (tmp != NULL)
		{
			// Check the file type
			if (strncasecmp(tmp, ".html", 5) == 0) HeaderTemplate = RESPONSE_HEADER_TEMPLATE_HTML;
			if (strncasecmp(tmp, ".txt", 4) == 0) HeaderTemplate = RESPONSE_HEADER_TEMPLATE_TEXT;
			if (strncasecmp(tmp, ".json", 5) == 0) HeaderTemplate = RESPONSE_HEADER_TEMPLATE_JSON;
			if (strncasecmp(tmp, ".js", 3) == 0) HeaderTemplate = RESPONSE_HEADER_TEMPLATE_JS;
			if (strncasecmp(tmp, ".gz", 3) == 0) HeaderTemplate = RESPONSE_HEADER_TEMPLATE_HTML_GZ;

			// Read the file in the /web folder
			tmp = NULL;
			len = util_readfile(ILibScratchPad, &tmp, 2000000);
			if (len > 0 && tmp != NULL)
			{
				// Send the matching file in the /web folder
				ILibWebServer_StreamHeader_Raw(sender, 200, "200 - OK", HeaderTemplate, ILibAsyncSocket_MemoryOwnership_STATIC);
				ILibWebServer_StreamBody(sender, tmp, (int)len, ILibAsyncSocket_MemoryOwnership_CHAIN, 1);
				responseSent = 1;
			}
		}
	}

	if (responseSent == 0 && ((header->DirectiveObjLength == 1 && strncasecmp(header->DirectiveObj, "/", 1) == 0) || (header->DirectiveObjLength == 11 && strncasecmp(header->DirectiveObj, "/index.html", 11) == 0) || (header->DirectiveObjLength == 15 && strncasecmp(header->DirectiveObj, "/authindex.html", 15) == 0)))
	{
		char* etag = ILibGetHeaderLine(header, "if-none-match", 13);
		if (etag != NULL && strnlen_s(etag, 1024) == strnlen_s(IntelAmtWebApp_etag, 1023) && memcmp(etag, IntelAmtWebApp_etag, strnlen_s(IntelAmtWebApp_etag, 1024)) == 0)
		{
			// ETag match, send "NOT MODIFIED" response
			ILibWebServer_StreamHeader_Raw(sender, 304, "NOT MODIFIED", RESPONSE_HEADER_SERVER, ILibAsyncSocket_MemoryOwnership_STATIC);
			ILibWebServer_StreamBody(sender, NULL, 0, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
		}
		else
		{
			// Send the built-in web site
			char temp[2000];
			snprintf(temp, 2000, RESPONSE_HEADER_TEMPLATE_HTML_GZ_ETAG, IntelAmtWebApp_etag);
			ILibWebServer_StreamHeader_Raw(sender, 200, "OK", temp, ILibAsyncSocket_MemoryOwnership_USER);
			ILibWebServer_StreamBody(sender, (char*)IntelAmtWebApp, sizeof(IntelAmtWebApp), ILibAsyncSocket_MemoryOwnership_STATIC, 1);
		}
	}
	
	if (responseSent == 0)
	{
		// Unknown URL, 404 error
		ILibWebServer_StreamHeader_Raw(sender, 404, "404 - File not found", RESPONSE_HEADER_TEMPLATE_HTML, ILibAsyncSocket_MemoryOwnership_STATIC);
		ILibWebServer_StreamBody(sender, "404 - File not found", 20, ILibAsyncSocket_MemoryOwnership_STATIC, 1);
	}
}

// Called when an HTTP session is disconnected.
void ILibLMS_HttpServerSessionDisconnect(struct ILibWebServer_Session *session)
{
	UNREFERENCED_PARAMETER(session);
}

// Called when a new HTTP session connects
void ILibLMS_WebServer_OnSession(struct ILibWebServer_Session *SessionToken, void *User)
{
	SessionToken->OnReceive = &ILibLMS_HttpServerSessionReceiveSink;
	SessionToken->OnDisconnect = &ILibLMS_HttpServerSessionDisconnect;
	SessionToken->User = User;
}
#endif

// Create a new MicroLMS module.
struct ILibLMS_StateModule *ILibLMS_Create(void *Chain, char* SelfExe)
{
	struct ILibLMS_StateModule *module;

	// Allocate the new module
	module = (struct ILibLMS_StateModule*)malloc(sizeof(struct ILibLMS_StateModule));
	if (module == NULL) { PRINTERROR(); return NULL; }
	memset(module, 0, sizeof(struct ILibLMS_StateModule));
	
	// Setup the web folder
	if (SelfExe != NULL) {
#if defined(WIN32)
		char* tmp = strrchr(SelfExe, '\\');
		memcpy_s(ILibScratchPad, sizeof(ILibScratchPad), SelfExe, tmp - SelfExe);
		memcpy_s(ILibScratchPad, sizeof(ILibScratchPad) - (tmp - SelfExe) + (tmp - SelfExe), "\\lmsweb", 7);
		if ((module->WebDir = (char*)malloc(strnlen_s(ILibScratchPad, sizeof(ILibScratchPad)) + 1)) == NULL) ILIBCRITICALEXIT(254);
		memcpy_s(module->WebDir, strnlen_s(ILibScratchPad, sizeof(ILibScratchPad)) + 1, ILibScratchPad, strnlen_s(ILibScratchPad, sizeof(ILibScratchPad)) + 1);
#else
		char* tmp = strrchr(SelfExe, '/');
		memcpy_s(ILibScratchPad, sizeof(ILibScratchPad), SelfExe, tmp - SelfExe);
		memcpy_s(ILibScratchPad + (tmp - SelfExe), sizeof(ILibScratchPad) - (tmp - SelfExe), "/lmsweb", 7);
#endif
	}

	{
		char localName[256] = "\0";
		struct hostent* s = NULL;

		// Setup MEI normal commands and set the local FQDN
		if (heci_Init(NULL, 0) == 0) { free(module); return NULL; }
		gethostname(localName, sizeof(localName));
		s = gethostbyname(localName);
		if (s != NULL) pthi_SetHostFQDN(s->h_name);
	}

	// Setup MEI with LMS interface, if we can't return null
	if (LME_Init(&(module->MeConnection), &ILibLMS_MEICallback, module) == 0) { 
		free(module); 
		return NULL; 
	}

	// Setup the module
	module->Destroy = &ILibLMS_Destroy;
	module->Chain = Chain;
	sem_init(&(module->Lock), 0, 1);

	// TCP servers
	module->Server1 = ILibCreateAsyncServerSocketModule(Chain, LMS_MAX_CONNECTIONS, 16992, 4096, 2, &ILibLMS_OnConnect, &ILibLMS_OnDisconnect, &ILibLMS_OnReceive, NULL, &ILibLMS_OnSendOK);
	ILibAsyncServerSocket_SetTag(module->Server1, module);
	module->Server2 = ILibCreateAsyncServerSocketModule(Chain, LMS_MAX_CONNECTIONS, 16993, 4096, 2, &ILibLMS_OnConnect, &ILibLMS_OnDisconnect, &ILibLMS_OnReceive, NULL, &ILibLMS_OnSendOK);
	ILibAsyncServerSocket_SetTag(module->Server2, module);

#ifndef NOCOMMANDER
	// Web Server
	module->WebServer = ILibWebServer_CreateEx(Chain, 8, 16994, 2, &ILibLMS_WebServer_OnSession, module);
#endif

	IlibExternLMS = module;						// Set the global reference to the LMS module.
	ILibAddToChain(Chain, module);				// Add this module to the chain.
	return module;
}

#endif
