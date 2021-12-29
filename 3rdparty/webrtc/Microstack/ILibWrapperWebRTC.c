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

#ifndef MICROSTACK_NOTLS

#if defined(WINSOCK2)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "ILibParsers.h"
#include "ILibAsyncSocket.h"
#include "ILibWebRTC.h"
#include "ILibWrapperWebRTC.h"
#include "ILibRemoteLogging.h"

#if defined(WIN32) && !defined(strncpy) && _MSC_VER < 1900
#define strncpy(dst, src, len) strcpy_s(dst, len, src)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined( SSL_CTX_set_ecdh_auto )
// use it
#else
#define SSL_CTX_set_ecdh_auto( ctx, IGNORED ) \
SSL_CTX_set_tmp_ecdh(ctx, EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
#endif

#define ILibWrapper_WebRTC_ConnectionFactory_ConnectionBucketSize 16	// *MUST* be a power of 2
#define ILibWrapper_WebRTC_Connection_DataChannelsBucketSize 16			// *MUST* be a power of 2
#define ILibWrapper_WebRTC_MaxStunServerPathLength 255

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))
#define INET_SOCKADDR_PORT(x) (x->sa_family==AF_INET6?(unsigned short)(((struct sockaddr_in6*)x)->sin6_port):(unsigned short)(((struct sockaddr_in*)x)->sin_port))

#define ILibBlockToSDP_CREDENTIALS_ONLY 0x00
#define ILibBlockToSDP_CREDENTIALS_ONLY_WITH_ALLOCATE (void*)0x01

char g_SessionRandom[32];
unsigned int g_SessionRandomId;
unsigned int g_nextiv = 0;
int ILibWrapper_WebRTC_ConnectionFactoryIndex = -1;

typedef struct
{
	ILibChain_Link ChainLink;

	ILibWrapper_WebRTC_ConnectionFactory_STUNHandler mStunHandler;
	ILibWrapper_WebRTC_ConnectionFactory mStunModule; 
	ILibSparseArray Connections;
	int NextConnectionID;
	struct util_cert selfcert;
	struct util_cert selftlscert;
	struct util_cert selftlsclientcert;
	SSL_CTX* ctx;
	char tlsServerCertThumbprint[32];
	char g_selfid[UTIL_HASHSIZE];

	ILibWebRTC_TURN_ConnectFlags mTurnSetting;
	struct sockaddr_in6 mTurnServer;
}ILibWrapper_WebRTC_ConnectionFactoryStruct;

typedef struct ILibWrapper_WebRTC_ConnectionStruct
{
	void *dtlsSession;
	void *User1, *User2, *User3;
	ILibWrapper_WebRTC_ConnectionFactoryStruct *mFactory;
	char** stunServerList;
	char* stunServerFlags;
	int stunServerListLength;
	int stunIndex;
	int isConnected;
	void *extraMemory;

	int id;
	char localUsername[9];
	char localPassword[33];

	char *remoteUsername;
	char* remotePassword;
	char *remoteOfferBlock;
	int remoteOfferBlockLen;

	char* offerBlock;
	int offerBlockLen;
	int isOfferInitiator;
	int isDtlsClient;

	ILibSparseArray DataChannels;

	ILibWrapper_WebRTC_OnConnectionCandidate OnCandidates;
	ILibWrapper_WebRTC_Connection_OnConnect OnConnected;
	ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannel;
	ILibWrapper_WebRTC_Connection_OnSendOK OnSendOK;

	ILibWrapper_WebRTC_Connection_Debug_OnEvent OnDebugEvent;
}ILibWrapper_WebRTC_ConnectionStruct;

// Prototypes:
const int ILibMemory_WebRTC_Connection_CONTAINERSIZE = sizeof(ILibWrapper_WebRTC_ConnectionStruct);
int ILibWrapper_WebRTC_PerformStun(ILibWrapper_WebRTC_ConnectionStruct *connection);
char* ILibWrapper_WebRTC_Connection_GetLocalUsername(ILibWrapper_WebRTC_Connection connection)
{
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->localUsername);
}
void* ILibWrapper_WebRTC_Connection_GetStunModule(ILibWrapper_WebRTC_Connection connection)
{
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->mFactory->mStunModule);
}

ILibTransport_DoneState ILibWrapper_ILibTransport_SendSink(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done)
{
	ILibTransport_DoneState retVal = ILibWrapper_WebRTC_DataChannel_Send((ILibWrapper_WebRTC_DataChannel*)transport, buffer, bufferLength);

	UNREFERENCED_PARAMETER(done);

	if (ownership == ILibTransport_MemoryOwnership_CHAIN) free(buffer);
	return(retVal);
}
unsigned int ILibWrapper_ILibTransport_PendingBytesPtr(void *transport)
{
	return(ILibSCTP_GetPendingBytesToSend(((ILibWrapper_WebRTC_ConnectionStruct*)((ILibWrapper_WebRTC_DataChannel*)transport)->parent)->dtlsSession));
}
void ILibWrapper_InitializeDataChannel_Transport(ILibWrapper_WebRTC_DataChannel *dataChannel)
{
	dataChannel->Chain = ((ILibWrapper_WebRTC_ConnectionStruct*)dataChannel->parent)->mFactory->ChainLink.ParentChain;
	dataChannel->IdentifierFlags = ILibTransports_WebRTC_DataChannel;
	dataChannel->ClosePtr = (ILibTransport_ClosePtr)ILibWrapper_WebRTC_DataChannel_Close;
	dataChannel->SendPtr = (ILibTransport_SendPtr)ILibWrapper_ILibTransport_SendSink;
	dataChannel->PendingBytesPtr = (ILibTransport_PendingBytesToSendPtr)ILibWrapper_ILibTransport_PendingBytesPtr;
}

short ILibWrapper_ReadShort(char* buffer, int offset)
{
	return(ntohs(((short*)(buffer+offset))[0]));
}
int ILibWrapper_ReadInt(char* buffer, int offset)
{
	return(ntohl(((int*)(buffer+offset))[0]));
}
char* ILibWrapper_ReadString(char* buffer, int offset, int length)
{
	char* retVal;
	if((retVal = (char*)malloc(length+1)) == NULL) {ILIBCRITICALEXIT(254);}
	memcpy_s(retVal,length + 1,  buffer+offset, length);
	retVal[length] = 0;
	return(retVal);
}

unsigned short ILibWrapper_GetNextStreamId(ILibWrapper_WebRTC_ConnectionStruct *connection)
{
	unsigned short id = 0;
	int done = 0;

	ILibSparseArray_Lock(connection->DataChannels);

	while(done==0)
	{
		done = 1;
		while(++id % 2 == (connection->isDtlsClient==0 ? 0 : 1));
		if(ILibSparseArray_Get(connection->DataChannels, (int)id)!=NULL) {done=0;}
	}
	ILibSparseArray_UnLock(connection->DataChannels);

	return(id);
}

int ILibWrapper_SdpToBlock(char* sdp, int sdpLen, int *isActive, char **username, char **password, char **block)
{
	struct parser_result *pr;
	struct parser_result_field *f;

	int ptr;
	int blockLen;
	void* candidates = NULL;
	char* lines;
	char* dtlshash = NULL;
	int dtlsHashLen = 0;
	int candidatecount = 0;
	int BlockFlags = 0;
	int passwordLen = 0, usernameLen = 0;
	*isActive = 0;
	*username = NULL;
	*password = NULL;


	ILibCreateStack(&candidates);

	lines = ILibString_Replace(sdp, sdpLen, "\n", 1, "\r", 1);
	pr = ILibParseString(lines, 0, sdpLen, "\r", 1);
	f = pr->FirstResult;
	while(f!=NULL)
	{
		if(f->datalength == 0) { f = f->NextResult; continue; }

		f->data[f->datalength] = 0;
		if(strcmp(f->data, "a=setup:passive")==0)
        {			
			BlockFlags |= ILibWebRTC_SDP_Flags_DTLS_SERVER;
        }
		else if(strcmp(f->data, "a=setup:active")==0 || strcmp(f->data, "a=setup:actpass")==0)
		{
			*isActive = 1;
		}

		if (f->datalength > 12 && strncmp(f->data, "a=ice-ufrag:", 12) == 0) { *username = f->data + 12; usernameLen = (int)strnlen_s(*username, f->datalength - 12); }
		if (f->datalength > 10 && strncmp(f->data, "a=ice-pwd:", 10) == 0) { *password = f->data + 10; passwordLen = (int)strnlen_s(*password, f->datalength - 10); }

		if(f->datalength > 22 && strncmp(f->data, "a=fingerprint:sha-256 ", 22)==0)
		{
			char* tmp = ILibString_Replace(f->data + 22, f->datalength - 22, ":", 1, "", 0);
			dtlsHashLen = util_hexToBuf(tmp, (int)strnlen_s(tmp, f->datalength - 22), tmp);
			dtlshash = tmp;
		}

		if(f->datalength > 12 && strncmp(f->data, "a=candidate:", 12)==0)
        {
			struct parser_result* pr2 = ILibParseString(f->data, 0, f->datalength, " ", 1);

			if(pr2->FirstResult->NextResult->datalength == 1 && pr2->FirstResult->NextResult->data[0] == '1' && pr2->FirstResult->NextResult->NextResult->datalength == 3 && strncasecmp(pr2->FirstResult->NextResult->NextResult->data, "UDP", 3)==0)
			//if(pr2->FirstResult->NextResult->NextResult->datalength == 3 && strncasecmp(pr2->FirstResult->NextResult->NextResult->data, "UDP", 3)==0)
			{
				char* candidateData;
				struct parser_result *pr3;
				char *tmp = pr2->FirstResult->NextResult->NextResult->NextResult->NextResult->NextResult->data;
				unsigned short port;

				tmp[pr2->FirstResult->NextResult->NextResult->NextResult->NextResult->NextResult->datalength] = 0;
				port = (unsigned short)atoi(tmp);
				
				pr3 = ILibParseString(pr2->FirstResult->NextResult->NextResult->NextResult->NextResult->data, 0, pr2->FirstResult->NextResult->NextResult->NextResult->NextResult->datalength, ".", 1);					
				if (pr3->NumResults == 4)
				{
					candidateData = pr3->FirstResult->data;
					pr3->FirstResult->data[pr3->FirstResult->datalength] = 0;
					candidateData[0] = (char)atoi(pr3->FirstResult->data);
					pr3->FirstResult->NextResult->data[pr3->FirstResult->NextResult->datalength] = 0;
					candidateData[1] = (char)atoi(pr3->FirstResult->NextResult->data);
					pr3->FirstResult->NextResult->NextResult->data[pr3->FirstResult->NextResult->NextResult->datalength] = 0;
					candidateData[2] = (char)atoi(pr3->FirstResult->NextResult->NextResult->data);
					pr3->FirstResult->NextResult->NextResult->NextResult->data[pr3->FirstResult->NextResult->NextResult->NextResult->datalength] = 0;
					candidateData[3] = (char)atoi(pr3->FirstResult->NextResult->NextResult->NextResult->data);

					((unsigned short*)candidateData)[2] = htons(port);
					candidateData[6] = 0;

					ILibPushStack(&candidates, candidateData);
					++candidatecount;
				}
				ILibDestructParserResults(pr3);
			}

			ILibDestructParserResults(pr2);
        }
		f = f->NextResult;
    }

	if (*username == NULL || *password == NULL || dtlshash == NULL)
    {
		*block = NULL;
		return(0);
    }

	
	blockLen = 6 + usernameLen+1 + passwordLen+1  + dtlsHashLen + 1 + (candidatecount*6)+1;
	if((*block = (char*)malloc(blockLen))==NULL){ILIBCRITICALEXIT(254);}
	ptr = 0;

	((unsigned short*)*block+ptr)[0] = htons(1);
	ptr += 2;

	((unsigned int*)(*block+ptr))[0] = htonl(BlockFlags);
	ptr += 4;

	(*block)[ptr] = (char)usernameLen;
	ptr += 1;
	memcpy_s(*block + ptr, blockLen - ptr, *username, usernameLen);
	ptr += usernameLen;

	(*block)[ptr] = (char)passwordLen;
	ptr += 1;
	memcpy_s(*block + ptr, blockLen - ptr, *password, passwordLen);
	ptr += passwordLen;

	(*block)[ptr] = (char)dtlsHashLen;
	ptr += 1;
	memcpy_s(*block + ptr, blockLen - ptr, dtlshash, dtlsHashLen);
	ptr += dtlsHashLen;

	(*block)[ptr] = (char)candidatecount;
	ptr += 1;
	while(ILibPeekStack(&candidates)!=NULL)
	{
		memcpy_s(*block + ptr, blockLen - ptr, ILibPopStack(&candidates), 6);
		ptr += 6;
	}

	ILibDestructParserResults(pr);
	free(lines);
	if(dtlshash!=NULL) {free(dtlshash);}
	return(ptr);
}

int ILibWrapper_BlockToSDPEx(char* block, int blockLen, char** username, char** password, char **sdp, const char* serverReflexiveCandidateAddress, unsigned short serverReflexiveCandidatePort)
{
	char sdpTemplate1[] = "v=0\r\no=MeshAgent %u 0 IN IP4 0.0.0.0\r\ns=SIP Call\r\nt=0 0\r\na=ice-ufrag:%s\r\na=ice-pwd:%s\r\na=fingerprint:sha-256 %s\r\nm=application 1 DTLS/SCTP 5000\r\nc=IN IP4 0.0.0.0\r\na=sctpmap:5000 webrtc-datachannel 16\r\na=setup:%s\r\n";
	char sdpTemplateRelay[] = "a=candidate:%d %d UDP %d %s %d typ relay raddr %u.%u.%u.%u %d\r\n";
	char sdpTemplateLocalCand[] = "a=candidate:%d %d UDP %d %u.%u.%u.%u %u typ host\r\n";
	char sdpTemplateSrflxCand[] = "a=candidate:%d %d UDP %d %s %u typ srflx raddr %u.%u.%u.%u rport %u\r\n";

	int sdpLen;
	int c,x,i,ptr;
	char junk[4];
	struct sockaddr_in6 *relayEndPoint = NULL;
	char relayAddressString[128];
	unsigned short relayAddressPort = 0;
	
	// Decode the block

	//This value is commented out to prevent a compiler warning for unused variables, but I left it here to show how to fetch it
	//
    //short blockversion = ILibWrapper_ReadShort(block, 0);
	//

    int blockflags = ILibWrapper_ReadInt(block, 2);	
	int isActive = (blockflags & ILibWebRTC_SDP_Flags_DTLS_SERVER) == ILibWebRTC_SDP_Flags_DTLS_SERVER ? 0:1;

	int usernamelen = (int)block[6];
	int passwordlen = (int)block[7 + usernamelen];
	int dtlshashlen = (int)block[8 + usernamelen + passwordlen];
	int candidatecount = (int)block[9 + usernamelen + passwordlen + dtlshashlen];
	char dtlshash[128];
	int rlen = 10 + usernamelen + passwordlen + dtlshashlen + (candidatecount * 6);
	char* candidates = block + 10 + usernamelen + passwordlen + dtlshashlen;

	if (sdp == ILibBlockToSDP_CREDENTIALS_ONLY)
	{
		// Special case, used only by Mesh Agent. Target is a fixed length array
		memcpy_s(username, usernamelen, block + 7, usernamelen);
		memcpy_s(password, passwordlen, block + 8 + usernamelen, passwordlen);
		return(0);
	}
	else if (sdp == ILibBlockToSDP_CREDENTIALS_ONLY_WITH_ALLOCATE)
	{
		*username = block + 7;
		*password = block + 8 + usernamelen;
		(*username)[usernamelen] = 0;
		(*password)[passwordlen] = 0;
		return(0);
	}

    *username = ILibWrapper_ReadString(block, 7, usernamelen);            
    *password = ILibWrapper_ReadString(block, 8 + usernamelen, passwordlen);

	util_tohex2(block + 9 + usernamelen + passwordlen, dtlshashlen, dtlshash);

	if (rlen < blockLen)
    {
		relayEndPoint = (struct sockaddr_in6*)(block+rlen+1);
		ILibInet_ntop2((struct sockaddr*)relayEndPoint, relayAddressString, 128);
		switch(((struct sockaddr_in*)relayEndPoint)->sin_family)
		{
			case AF_INET:
				relayAddressPort = ntohs(((struct sockaddr_in*)relayEndPoint)->sin_port);
				break;
			case AF_INET6:
				relayAddressPort = ntohs(relayEndPoint->sin6_port);
				break;
			default:
				relayEndPoint = NULL;
		}
    }


	// Build the sdp
	sdpLen = 2 + (int)sizeof(sdpTemplate1) + usernamelen + passwordlen + 7 + (dtlshashlen*3) + 7 + (2 *   (candidatecount * (12 + 21 + (int)sizeof(sdpTemplateLocalCand)))  );
	if(serverReflexiveCandidateAddress!=NULL)
	{
		sdpLen += (2* (12 + 21 + (int)sizeof(sdpTemplateSrflxCand)));
	}
	if(relayEndPoint!=NULL)
	{
		sdpLen += (2* (12 + 21 + (int)sizeof(sdpTemplateRelay)));
	}

	if((*sdp = (char*)malloc(sdpLen))==NULL){ILIBCRITICALEXIT(254);}

	util_random(4, junk);

	x = sprintf_s(*sdp, sdpLen, sdpTemplate1, ((unsigned int*)junk)[0]%1000000, *username, *password, dtlshash, isActive==0?"passive":"actpass");

	for(c = 1; c <= 2; ++c)
	{
		for (i = 0; i < candidatecount; i++)
		{
			ptr = i*6;
			x += sprintf_s(*sdp+x, sdpLen-x, sdpTemplateLocalCand, i, c, 2128609535-i, (unsigned char)candidates[ptr], (unsigned char)candidates[ptr+1], (unsigned char)candidates[ptr+2], (unsigned char)candidates[ptr+3], ntohs(((unsigned short*)(candidates+ptr+4))[0]));
		}
		if(serverReflexiveCandidateAddress!=NULL)
		{
			x += sprintf_s(*sdp+x, sdpLen-x, sdpTemplateSrflxCand, i, c, 2128609535-i, serverReflexiveCandidateAddress, serverReflexiveCandidatePort, (unsigned char)candidates[0], (unsigned char)candidates[1], (unsigned char)candidates[2], (unsigned char)candidates[3], ntohs(((unsigned short*)(candidates+4))[0]));
			++i;
		}
		if(relayEndPoint!=NULL)
		{
			x += sprintf_s(*sdp+x, sdpLen-x, sdpTemplateRelay, i, c, 2128609535-i, relayAddressString, relayAddressPort,  (unsigned char)candidates[0], (unsigned char)candidates[1], (unsigned char)candidates[2], (unsigned char)candidates[3]);
		}
	}
	return(x);
}

void ILibWrapper_BlockToUserNamePWD(char* block, int blockLen, char **username, int *usernameLen, char** password, int *passwordLen)
{
	UNREFERENCED_PARAMETER(blockLen);
	*usernameLen = (int)block[6];
	*passwordLen = (int)block[7 + *usernameLen];

    *username = ILibWrapper_ReadString(block, 7, *usernameLen);            
    *password = ILibWrapper_ReadString(block, 8 + *usernameLen, *passwordLen);
}
int ILibWrapper_BlockToSDP(char* block, int blockLen, int isActive, char** username, char** password, char **sdp)
{
	UNREFERENCED_PARAMETER(isActive);
	return(ILibWrapper_BlockToSDPEx(block, blockLen, username, password, sdp, NULL, 0));
}
void ILibWrapper_WebRTC_InitializeCrypto(ILibWrapper_WebRTC_ConnectionFactoryStruct *factory, struct util_cert *tls)
{
	int l = 32;

	if (factory->ctx == NULL)
	{
		// Init SSL
		util_openssl_init();

		// Init DTLS
#ifdef _MINCORE
		factory->ctx = SSL_CTX_new(DTLSv1_method());
#else
		factory->ctx = SSL_CTX_new(DTLS_method());
		//SSL_CTX_set_ecdh_auto(factory->ctx, 1); // Turn on elliptic curve for dTLS, this is required starting with Firefox 39. (DEPRECATED for OpenSSL/1.1.x)
#endif
		if (ILibWrapper_WebRTC_ConnectionFactoryIndex < 0)
		{
			ILibWrapper_WebRTC_ConnectionFactoryIndex = SSL_CTX_get_ex_new_index(0, "ILibWrapper_WebRTC_ConnectionFactoryStruct index", NULL, NULL, NULL);
		}
		SSL_CTX_set_ex_data(factory->ctx, ILibWrapper_WebRTC_ConnectionFactoryIndex, factory);
	}

	if (tls != ILibWebWrapperWebRTC_ConnectionFactory_INIT_CRYPTO_LATER && factory->selftlscert.x509 == NULL)
	{
		// Init Certs
		if (tls == NULL)
		{
			util_mkCert(NULL, &(factory->selfcert), 2048, 10000, "localhost", CERTIFICATE_ROOT, NULL);
			util_keyhash(factory->selfcert, factory->g_selfid);
			util_mkCert(&(factory->selfcert), &(factory->selftlscert), 2048, 10000, "localhost", CERTIFICATE_TLS_SERVER, NULL);
		}
		else
		{
			memcpy_s(&(factory->selftlscert), sizeof(struct util_cert), tls, sizeof(struct util_cert));
		}

		SSL_CTX_use_certificate(factory->ctx, factory->selftlscert.x509);
		SSL_CTX_use_PrivateKey(factory->ctx, factory->selftlscert.pkey);
		X509_digest(factory->selftlscert.x509, EVP_get_digestbyname("sha256"), (unsigned char*)factory->tlsServerCertThumbprint, (unsigned int*)&l);

		if (factory->mStunModule != NULL) { ILibStunClient_SetOptions(factory->mStunModule, factory->ctx, factory->tlsServerCertThumbprint); }
	}
}
void ILibWrapper_WebRTC_UnInitializeCrypto(ILibWrapper_WebRTC_ConnectionFactoryStruct *factory)
{
	// UnInit DTLS
	SSL_CTX_free(factory->ctx);
	factory->ctx = NULL;

	// UnInit Certs
	if (factory->selfcert.x509 != NULL)
	{
		util_freecert(&(factory->selftlsclientcert));
		util_freecert(&(factory->selftlscert));
		util_freecert(&(factory->selfcert));
	}

	memset(&(factory->selftlsclientcert),0,sizeof(struct util_cert));
	memset(&(factory->selftlscert),0,sizeof(struct util_cert));
	memset(&(factory->selfcert),0,sizeof(struct util_cert));

	
	// UnInit SSL
	util_openssl_uninit();
}

void ILibWrapper_WebRTC_ConnectionFactory_OnDestroyEx(ILibSparseArray sender, int index, void *value, void *user)
{
	UNREFERENCED_PARAMETER(user);
	UNREFERENCED_PARAMETER(index);
	UNREFERENCED_PARAMETER(sender);

	ILibWrapper_WebRTC_Connection_DestroyConnection((ILibWrapper_WebRTC_Connection)value);
}
void ILibWrapper_WebRTC_ConnectionFactory_OnDestroy(void *obj)
{
	ILibWrapper_WebRTC_ConnectionFactoryStruct *cfs = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)obj;

	ILibSparseArray_DestroyEx(cfs->Connections, ILibWrapper_WebRTC_ConnectionFactory_OnDestroyEx, cfs);
	ILibWrapper_WebRTC_UnInitializeCrypto((ILibWrapper_WebRTC_ConnectionFactoryStruct*)obj);
}

void ILibWrapper_WebRTC_Connection_DestroyConnectionEx(ILibSparseArray sender, int index, void *value, void *user)
{
	ILibWrapper_WebRTC_DataChannel *dc = (ILibWrapper_WebRTC_DataChannel*)value;
	UNREFERENCED_PARAMETER(user);
	UNREFERENCED_PARAMETER(index);
	UNREFERENCED_PARAMETER(sender);

	if(dc->OnClosed!=NULL) {dc->OnClosed(dc);}
	free(dc->channelName);
	free(dc);
}
void ILibWrapper_WebRTC_Connection_DestroyConnection(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*)connection;

	if(obj==NULL) {return;}

	
	if(ILibIsChainBeingDestroyed(obj->mFactory->ChainLink.ParentChain)==0)
	{
		ILibSparseArray_Remove(obj->mFactory->Connections, obj->id);

		// Chain is not being destroyed, so we can disconnect DTLS		
		if(obj->dtlsSession!=NULL)
		{
			ILibSCTP_Close(obj->dtlsSession);
		}
		else
		{
			if(obj->offerBlock!=NULL && obj->offerBlockLen>0)
			{
				// Clear the ICE State for the local Offer
				ILibStun_ClearIceState(obj->mFactory->mStunModule, ILibStun_CharToSlot(obj->offerBlock[7]));
			}
			if(obj->remoteOfferBlock!=NULL && obj->remoteOfferBlockLen>0)
			{
				// Clear the ICE State for the remote Offer
				ILibStun_ClearIceState(obj->mFactory->mStunModule, ILibStun_CharToSlot(obj->remoteOfferBlock[7]));
			}
		}
	}

	ILibWebRTC_SetUserObject(obj->mFactory->mStunModule, obj->localUsername, NULL);

	if(obj->stunServerList!=NULL && obj->stunServerListLength>0)
	{
		int i;
		for(i=0;i<obj->stunServerListLength;++i)
		{
			free(obj->stunServerList[i]);
		}
		free(obj->stunServerList);
		obj->stunServerList = NULL;
	}

	if(obj->offerBlock!=NULL)
	{
		free(obj->offerBlock);
		obj->offerBlock = NULL;
	}

	if(obj->remoteOfferBlock!=NULL)
	{
		free(obj->remoteOfferBlock);
		obj->remoteOfferBlock = NULL;
	}

	ILibSparseArray_DestroyEx(obj->DataChannels, &ILibWrapper_WebRTC_Connection_DestroyConnectionEx, obj);
	if(obj->stunServerFlags!=NULL)
	{
		free(obj->stunServerFlags);
	}

	free(obj);
}


void ILibWrapper_WebRTC_Connection_Disconnect(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*)connection;

	if(obj->dtlsSession!=NULL)
	{
		// If DTLS was connected, this will call OnDisconnect later
		ILibSCTP_Close(obj->dtlsSession);
	}
	else
	{
		// No DTLS session, so just clean up the connection object

		if(obj->offerBlock!=NULL && obj->offerBlockLen>0)
		{
			// Clear the ICE State for the local Offer
			ILibStun_ClearIceState(obj->mFactory->mStunModule, ILibStun_CharToSlot(obj->offerBlock[7]));
		}
		if(obj->remoteOfferBlock!=NULL && obj->remoteOfferBlockLen>0)
		{
			// Clear the ICE State for the remote Offer
			ILibStun_ClearIceState(obj->mFactory->mStunModule, ILibStun_CharToSlot(obj->remoteOfferBlock[7]));
		}

		if(obj->OnConnected!=NULL) {obj->OnConnected(connection, 0);}
		ILibWrapper_WebRTC_Connection_DestroyConnection(connection);
	}
}

void ILibWrapper_WebRTC_OnStunResult(void* StunModule, ILibStun_Results Result, struct sockaddr_in* PublicIP, void *user)
{
	ILibWrapper_WebRTC_ConnectionStruct *connection;
	ILibWrapper_WebRTC_ConnectionFactoryStruct *factory = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)SSL_CTX_get_ex_data(ILibStunClient_GetSSLCTX(StunModule), ILibWrapper_WebRTC_ConnectionFactoryIndex);

	// This is a StunResult for an operation called on the Factory object, not the Connection object
	if (factory == user) 
	{
		if (factory->mStunHandler != NULL) { factory->mStunHandler(factory, Result, PublicIP); }
		return;
	}
	connection = (ILibWrapper_WebRTC_ConnectionStruct*)user;
	connection->stunServerFlags[connection->stunIndex] = 1;

	switch(Result)
	{
		case ILibStun_Results_Unknown:
			// The STUN Server we used is no good, so try with a different server
			connection->stunServerFlags[connection->stunIndex] = 2;
			if(ILibWrapper_WebRTC_PerformStun(connection)!=0 && connection->OnCandidates!=NULL)
			{
				// No more STUN servers to try, so give up
				connection->OnCandidates(connection, NULL);
			}
			break;
		case ILibStun_Results_Public_Interface:
			if(connection->OnCandidates!=NULL)
			{
				connection->OnCandidates(connection, (struct sockaddr_in6*)PublicIP);
			}
			break;
		case ILibStun_Results_No_NAT:
			// Don't need to do anything if there was no NAT, as the local candidates will be good enough
			if(connection->OnCandidates!=NULL)
			{
				connection->OnCandidates(connection, NULL);
			}
			break;
		default:
			break;
	}
}

int ILibWrapper_WebRTC_Connection_IsConnected(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;

	return(obj!=NULL?obj->isConnected:0);
}
void ILibWrapper_WebRTC_Connection_CloseAllDataChannels(ILibWrapper_WebRTC_Connection connection)
{
	ILibWebRTC_CloseDataChannel_ALL(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->dtlsSession);
}
void ILibWrapper_WebRTC_OnConnectSink(void *StunModule, void* module, int connected)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) ILibWebRTC_GetUserObjectFromDtlsSession(module);
	if(obj==NULL) {return;}

	UNREFERENCED_PARAMETER(StunModule);

	obj->dtlsSession = module;
	obj->isConnected = connected;

	obj->isDtlsClient = ILibWebRTC_IsDtlsInitiator(module);

	if(obj->OnConnected!=NULL)
	{
		obj->OnConnected(obj, connected);
	}

	if(connected==0)
	{
		if(ILibIsChainBeingDestroyed(obj->mFactory->ChainLink.ParentChain)==0)
		{
			// Connection was closed, so let's clean up, but only if the chain is still running. It's possible that the previous callback caused the layer above
			// us to initiate a chain shutdown. In that case, skip this method call, becuase otherwise we may end up calling it twice, which will cause a crash.
			ILibWrapper_WebRTC_Connection_DestroyConnection(obj);
		}
	}
}
void ILibWrapper_WebRTC_OnDataSink(void* StunModule, void* module, unsigned short streamId, int pid, char* buffer, int bufferLen, void** user)
{
	ILibWrapper_WebRTC_DataChannel *dc = NULL;
	ILibWrapper_WebRTC_ConnectionStruct *connection = (ILibWrapper_WebRTC_ConnectionStruct*)ILibWebRTC_GetUserObjectFromDtlsSession(module);

	UNREFERENCED_PARAMETER(user);
	UNREFERENCED_PARAMETER(StunModule);

	ILibRemoteLogging_printf(ILibChainGetLogger(((ILibChain_Link*)StunModule)->ParentChain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "DataChannel[%u] OnData: PID(%d) Bytes(%d)", streamId, pid, bufferLen);

	dc = (ILibWrapper_WebRTC_DataChannel*)ILibSparseArray_Get(connection->DataChannels, (int)streamId);

	if(dc!=NULL)
	{
		if(dc->OnRawData!=NULL)	{dc->OnRawData(dc, buffer, bufferLen, pid);}
		if(dc->OnStringData!=NULL && pid == 51) {dc->OnStringData(dc, buffer, bufferLen);}
		if(dc->OnBinaryData!=NULL && pid == 53) {dc->OnBinaryData(dc, buffer, bufferLen);}
	}
}
void ILibWrapper_WebRTC_OnSendOK_EnumerateSink(ILibSparseArray sender, int index, void *value, void *user)
{
	ILibWrapper_WebRTC_DataChannel *dc = (ILibWrapper_WebRTC_DataChannel*)value;

	if (dc != NULL && dc->TransportSendOKPtr != NULL) { dc->TransportSendOKPtr(dc); }
}
void ILibWrapper_WebRTC_OnSendOKSink(void* StunModule, void* module, void* user)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) ILibWebRTC_GetUserObjectFromDtlsSession(module);
	UNREFERENCED_PARAMETER(user);
	UNREFERENCED_PARAMETER(StunModule);

	if(obj != NULL && obj->OnSendOK != NULL)
	{
		obj->OnSendOK(obj);
	}
	if (obj != NULL) { ILibSparseArray_Enumerate(obj->DataChannels, ILibWrapper_WebRTC_OnSendOK_EnumerateSink, obj); }
}
void ILibWrapper_WebRTC_OnDataChannelClosed(void *StunModule, void* WebRTCModule, unsigned short StreamId)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) ILibWebRTC_GetUserObjectFromDtlsSession(WebRTCModule);
	ILibWrapper_WebRTC_DataChannel* dataChannel = NULL;
	
	UNREFERENCED_PARAMETER(StunModule);

	ILibSparseArray_Lock(obj->DataChannels);
	dataChannel = (ILibWrapper_WebRTC_DataChannel*)ILibSparseArray_Remove(obj->DataChannels, (int)StreamId);
	ILibSparseArray_UnLock(obj->DataChannels);
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->mFactory->ChainLink.ParentChain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "[ILibWrapperWebRTC] Data Channel Signaled to Close: [%s]:%u", dataChannel != NULL ? dataChannel->channelName : "UNKNOWN", StreamId);

	if(dataChannel!=NULL) {ILibWrapper_WebRTC_Connection_DestroyConnectionEx(obj->DataChannels, (int)StreamId, dataChannel, obj);}
}
int ILibWrapper_WebRTC_OnDataChannel(void *StunModule, void* WebRTCModule, unsigned short StreamId, char* ChannelName, int ChannelNameLength)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) ILibWebRTC_GetUserObjectFromDtlsSession(WebRTCModule);
	ILibWrapper_WebRTC_DataChannel* dataChannel = NULL;

	UNREFERENCED_PARAMETER(StunModule);

	ILibSparseArray_Lock(obj->DataChannels);
	if((dataChannel = (ILibWrapper_WebRTC_DataChannel*)ILibSparseArray_Get(obj->DataChannels, (int)StreamId))==NULL)
	{
		dataChannel = (ILibWrapper_WebRTC_DataChannel*)ILibChain_Link_Allocate(sizeof(ILibWrapper_WebRTC_DataChannel), ILibMemory_GetExtraMemorySize(obj->extraMemory));

		dataChannel->parent = obj;
		dataChannel->streamId = StreamId;
		if((dataChannel->channelName = (char*)malloc(ChannelNameLength+1))==NULL){ILIBCRITICALEXIT(254);}
		ChannelName[ChannelNameLength] = 0;
		dataChannel->channelName[ChannelNameLength] = 0;
		memcpy_s(dataChannel->channelName, ChannelNameLength + 1, ChannelName, ChannelNameLength);
		ILibSparseArray_Add(obj->DataChannels, (int)StreamId, dataChannel);
		ILibWrapper_InitializeDataChannel_Transport(dataChannel);
	}
	ILibSparseArray_UnLock(obj->DataChannels);

	if(obj->OnDataChannel!=NULL)
	{			
		obj->OnDataChannel(obj, dataChannel);
	}

	return(0); // We always return 0, becuase we want the default ACK behavior that is handled underneath
}
void ILibWrapper_WebRTC_OnDataChannelAck(void *StunModule, void* WebRTCModule, unsigned short StreamId)
{
	ILibWrapper_WebRTC_ConnectionStruct *connection = (ILibWrapper_WebRTC_ConnectionStruct*)ILibWebRTC_GetUserObjectFromDtlsSession(WebRTCModule);
	ILibWrapper_WebRTC_DataChannel *dataChannel;

	UNREFERENCED_PARAMETER(StunModule);

	if(connection!=NULL)
	{
		ILibSparseArray_Lock(connection->DataChannels);
		dataChannel = (ILibWrapper_WebRTC_DataChannel*)ILibSparseArray_Get(connection->DataChannels, (int)StreamId);
		ILibSparseArray_UnLock(connection->DataChannels);

		if(dataChannel!=NULL && dataChannel->OnAck!=NULL)  {dataChannel->OnAck(dataChannel);}
	}
}
void ILibWrapper_WebRTC_OnOfferUpdated(void* stunModule, char* iceOffer, int iceOfferLen)
{
	UNREFERENCED_PARAMETER(stunModule);
	UNREFERENCED_PARAMETER(iceOffer);
	UNREFERENCED_PARAMETER(iceOfferLen);
}

int ILibWrapper_WebRTC_ConnectionFactory_Bucketizer(int index)
{
	return(index & (ILibWrapper_WebRTC_ConnectionFactory_ConnectionBucketSize-1));
}
SSL_CTX* ILibWrapper_WebRTC_ConnectionFactory_GetCTX(ILibWrapper_WebRTC_ConnectionFactory factory)
{
	return(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->ctx);
}
void ILibWrapper_WebRTC_ConnectionFactory_Set_STUNHandler(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_ConnectionFactory_STUNHandler handler)
{
	((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunHandler = handler;
}

void ILibWrapper_WebRTC_ConnectionFactory_InitCrypto(ILibWrapper_WebRTC_ConnectionFactory factory, struct util_cert *tlsCert)
{
	ILibWrapper_WebRTC_InitializeCrypto((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory, tlsCert);
}
void ILibWrapper_WebRTC_ConnectionFactory_RemoveFromChainSink(void *chain, void *user)
{
	ILibWrapper_WebRTC_ConnectionFactoryStruct *data = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)user;
	ILibLinkedList links = ILibChain_GetLinks(chain);
	void *node = ILibLinkedList_GetNode_Search(links, NULL, data);
	ILibChain_Link *obj;
	void *turnClient = NULL;
	int finished = 0;

	// Factory Connection object was first object added, TurnClient object was last, so delete everything in between
	turnClient = ILibStun_GetTurnClient(data->mStunModule);

	while (node != NULL && finished == 0)
	{
		obj = (ILibChain_Link*)ILibLinkedList_GetDataFromNode(node);

		if (obj == turnClient) { finished = 1; }
		if (obj->DestroyHandler != NULL) { obj->DestroyHandler(obj); }
		free(obj);
		node = ILibLinkedList_Remove(node);
	}
}
void ILibWrapper_WebRTC_ConnectionFactory_RemoveFromChain(ILibWrapper_WebRTC_ConnectionFactory factory)
{
	// We are going to force a context switch, so we can make sure that we aren't doing this from a callback of one of the objects we're going to remove
	ILibWrapper_WebRTC_ConnectionFactoryStruct *data = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory;
	ILibChain_RunOnMicrostackThreadEx(data->ChainLink.ParentChain, ILibWrapper_WebRTC_ConnectionFactory_RemoveFromChainSink, data);
}
ILibWrapper_WebRTC_ConnectionFactory ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory2(void* chain, unsigned short localPort, struct util_cert *tlsCert)
{
	ILibWrapper_WebRTC_ConnectionFactoryStruct *retVal = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)malloc(sizeof(ILibWrapper_WebRTC_ConnectionFactoryStruct));
	if (retVal == NULL) { ILIBCRITICALEXIT(254); }

	memset(retVal, 0, sizeof(ILibWrapper_WebRTC_ConnectionFactoryStruct));
	retVal->ChainLink.DestroyHandler = &ILibWrapper_WebRTC_ConnectionFactory_OnDestroy;
	ILibAddToChain(chain, retVal);

	retVal->mStunModule = ILibStunClient_Start(chain, localPort, &ILibWrapper_WebRTC_OnStunResult);
	if (retVal->mStunModule == NULL) 
	{ 
		free(retVal); 
		ILibLinkedList_Remove_ByData(ILibChain_GetLinks(chain), retVal);
		return(NULL); 
	}

	ILibWrapper_WebRTC_InitializeCrypto(retVal, tlsCert);

	if (tlsCert != ILibWebWrapperWebRTC_ConnectionFactory_INIT_CRYPTO_LATER)
	{
		ILibStunClient_SetOptions(retVal->mStunModule, retVal->ctx, retVal->tlsServerCertThumbprint);
	}
	ILibSCTP_SetCallbacks(retVal->mStunModule, &ILibWrapper_WebRTC_OnConnectSink, &ILibWrapper_WebRTC_OnDataSink, &ILibWrapper_WebRTC_OnSendOKSink);
	ILibWebRTC_SetCallbacks(retVal->mStunModule, &ILibWrapper_WebRTC_OnDataChannel, &ILibWrapper_WebRTC_OnDataChannelClosed, &ILibWrapper_WebRTC_OnDataChannelAck, &ILibWrapper_WebRTC_OnOfferUpdated);

	retVal->Connections = ILibSparseArray_Create(ILibWrapper_WebRTC_ConnectionFactory_ConnectionBucketSize, &ILibWrapper_WebRTC_ConnectionFactory_Bucketizer);
	retVal->ChainLink.ParentChain = chain;

	return(retVal);
}

void ILibWrapper_WebRTC_Connection_SetOnSendOK(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_Connection_OnSendOK SendOKSink)
{
	((ILibWrapper_WebRTC_ConnectionStruct*)connection)->OnSendOK = SendOKSink;
}

ILibWrapper_WebRTC_ConnectionFactory ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory(void* chain, unsigned short localPort)
{
	return(ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionFactory2(chain, localPort, NULL));
}

int ILibWrapper_WebRTC_Connection_DataChannelBucketizer(int index)
{
	return(index & (ILibWrapper_WebRTC_Connection_DataChannelsBucketSize - 1));
}

int ILibWrapper_WebRTC_Connection_GetID(ILibWrapper_WebRTC_Connection connection)
{
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->id);
}
void ILibWrapper_WebRTC_ConnectionFactory_SendRaw(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr* target, char* data, int datalen, enum ILibAsyncSocket_MemoryOwnership UserFree)
{
	ILibStunClient_SendData(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, target, data, datalen, UserFree);
}
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionEx2(ILibWrapper_WebRTC_ConnectionFactory factory, char *iceOfferBlock, int iceOfferBlockLen, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK, int extraMemorySize)
{
	ILibWrapper_WebRTC_ConnectionStruct *retVal = NULL;
	char tmp[1];

	int iceSlot = ILibStun_GetExistingIceOfferIndex(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, iceOfferBlock, iceOfferBlockLen);
	if (iceSlot < 0)
	{
		// There is no outstanding offer from the remote peer
		retVal = ILibWrapper_WebRTC_ConnectionFactory_CreateConnection2(factory, OnConnectHandler, OnDataChannelHandler, OnConnectionSendOK, extraMemorySize);
		retVal->remoteOfferBlockLen = iceOfferBlockLen;
		if ((retVal->remoteOfferBlock = (char*)malloc(iceOfferBlockLen)) == NULL) { ILIBCRITICALEXIT(254); }
		memcpy_s(retVal->remoteOfferBlock, iceOfferBlockLen, iceOfferBlock, iceOfferBlockLen);
		retVal->offerBlockLen = ILibStun_SetIceOffer(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, iceOfferBlock, iceOfferBlockLen, &(retVal->offerBlock));
		ILibWrapper_BlockToSDP(retVal->remoteOfferBlock, iceOfferBlockLen, 0, (char**)&(retVal->remoteUsername), (char**)&(retVal->remotePassword), ILibBlockToSDP_CREDENTIALS_ONLY_WITH_ALLOCATE);
		ILibWrapper_BlockToSDP(retVal->offerBlock, retVal->offerBlockLen, 0, (char**)&(retVal->localUsername), (char**)&(retVal->localPassword), ILibBlockToSDP_CREDENTIALS_ONLY);

		ILibWebRTC_SetUserObject(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, retVal->localUsername, retVal);
	}
	else
	{
		// Outstanding offer from remote peer was found
		tmp[0] = (char)ILibStun_SlotToChar(iceSlot);
		retVal = ILibWebRTC_GetUserObject(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, tmp);

		if (retVal->remoteOfferBlock != NULL) { free(retVal->remoteOfferBlock); }
		if (retVal->offerBlock != NULL) { free(retVal->offerBlock); }

		retVal->remoteOfferBlockLen = iceOfferBlockLen;
		if ((retVal->remoteOfferBlock = (char*)malloc(iceOfferBlockLen)) == NULL) { ILIBCRITICALEXIT(254); }
		memcpy_s(retVal->remoteOfferBlock, iceOfferBlockLen, iceOfferBlock, iceOfferBlockLen);
		retVal->offerBlockLen = ILibStun_SetIceOffer(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, iceOfferBlock, iceOfferBlockLen, &(retVal->offerBlock));
	}

	return(retVal);
}
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionEx(ILibWrapper_WebRTC_ConnectionFactory factory, char *iceOfferBlock, int iceOfferBlockLen, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK)
{
	return(ILibWrapper_WebRTC_ConnectionFactory_CreateConnectionEx2(factory, iceOfferBlock, iceOfferBlockLen, OnConnectHandler, OnDataChannelHandler, OnConnectionSendOK, 0));
}

ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnection2(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK, int extraMemorySize)
{
	void *extraMemory;
	ILibWrapper_WebRTC_ConnectionStruct *retVal = (ILibWrapper_WebRTC_ConnectionStruct*)ILibMemory_Allocate(sizeof(ILibWrapper_WebRTC_ConnectionStruct), extraMemorySize, NULL, &extraMemory);
	retVal->extraMemory = extraMemory;

	retVal->OnConnected = OnConnectHandler;
	retVal->OnDataChannel = OnDataChannelHandler;
	retVal->OnSendOK = OnConnectionSendOK;

	retVal->mFactory = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory;

	ILibSparseArray_Lock(retVal->mFactory->Connections);
	retVal->id = (retVal->mFactory->NextConnectionID)++;
	ILibSparseArray_Add(retVal->mFactory->Connections, retVal->id, retVal);
	ILibSparseArray_UnLock(retVal->mFactory->Connections);

	retVal->DataChannels = ILibSparseArray_Create(ILibWrapper_WebRTC_Connection_DataChannelsBucketSize, &ILibWrapper_WebRTC_Connection_DataChannelBucketizer);

	return(retVal);
}
ILibWrapper_WebRTC_Connection ILibWrapper_WebRTC_ConnectionFactory_CreateConnection(ILibWrapper_WebRTC_ConnectionFactory factory, ILibWrapper_WebRTC_Connection_OnConnect OnConnectHandler, ILibWrapper_WebRTC_Connection_OnDataChannel OnDataChannelHandler, ILibWrapper_WebRTC_Connection_OnSendOK OnConnectionSendOK)
{
	return(ILibWrapper_WebRTC_ConnectionFactory_CreateConnection2(factory, OnConnectHandler, OnDataChannelHandler, OnConnectionSendOK, 0));
}
void ILibWrapper_WebRTC_Connection_SetStunServers(ILibWrapper_WebRTC_Connection connection, char** serverList, int serverLength)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
	int i;

	if(serverLength<=0){return;}

	if((obj->stunServerFlags = (char*)malloc(serverLength))==NULL){ILIBCRITICALEXIT(254);}
	memset(obj->stunServerFlags, 0, serverLength);

	obj->stunServerList = (char**)malloc(serverLength * sizeof(char*));
	obj->stunServerListLength = serverLength;
	for(i=0;i<serverLength;++i)
	{
		int sz = (int)strnlen_s(serverList[i], ILibWrapper_WebRTC_MaxStunServerPathLength);
		obj->stunServerList[i] = (char*)malloc(sz+1);
		(obj->stunServerList[i])[sz] = 0;
		memcpy_s(obj->stunServerList[i], sz + 1, serverList[i], sz);
	}
}

void ILibWrapper_WebRTC_ConnectionFactory_PerformSTUN(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr_in* StunServer)
{
	ILibStunClient_PerformStun(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mStunModule, StunServer, factory);
}
int ILibWrapper_WebRTC_PerformStunEx(ILibWrapper_WebRTC_ConnectionStruct *connection, int MappingDetection)
{
	int i,delimiter;
	struct sockaddr_in6 stunServer;
	unsigned short port;
	char temp[255];
	char *host;

	if(connection->stunServerListLength > 0)
	{
		for(i=0;i<connection->stunServerListLength;++i)
		{
			if(connection->stunServerFlags[i] > 1) {continue;} // 0 = Unknown, 1 = Success, 2 = ERROR

			delimiter = ILibString_IndexOf(connection->stunServerList[i], (int)strnlen_s(connection->stunServerList[i], ILibWrapper_WebRTC_MaxStunServerPathLength), ":", 1);
			if(delimiter>0) 
			{
				memcpy_s(temp, sizeof(temp), connection->stunServerList[i], delimiter);
				temp[delimiter] = 0;
				host = temp;
				port = (unsigned short)atoi(connection->stunServerList[i] + delimiter + 1);
			}
			else
			{
				host = connection->stunServerList[i];
				port = 3478;
			}
			if(ILibResolve(host, "http", &stunServer)==0)
			{
				switch(stunServer.sin6_family)
				{
					case AF_INET:
						((struct sockaddr_in*)&stunServer)->sin_port = htons(port);
						break;
					case AF_INET6:
						stunServer.sin6_port = htons(port);
						break;
				}
				connection->stunIndex = i;
				if(MappingDetection == 0)
				{
					ILibStunClient_PerformStun(connection->mFactory->mStunModule, (struct sockaddr_in*)&stunServer, connection);
				}
				else
				{
					ILibStunClient_PerformNATBehaviorDiscovery(connection->mFactory->mStunModule, (struct sockaddr_in*)&stunServer, connection);
				}
				break;
			}
			else
			{
				connection->stunServerFlags[i] = 2; // Could not resolve, so skip, and use another server
			}
		}
		return(i<connection->stunServerListLength?0:1);
	}
	else
	{
		return(1);
	}
}
int ILibWrapper_WebRTC_PerformStun(ILibWrapper_WebRTC_ConnectionStruct *connection)
{
	return(ILibWrapper_WebRTC_PerformStunEx(connection, 0));
}


int ILibWrapper_WebRTC_Connection_DoesPeerSupportUnreliableMode(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
	return(ILibSCTP_DoesPeerSupportFeature(obj->dtlsSession, 0xC000));
}

char* ILibWrapper_WebRTC_Connection_SetOffer(ILibWrapper_WebRTC_Connection connection, char* offer, int offerLen, ILibWrapper_WebRTC_OnConnectionCandidate onCandidates)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
	char* sdp;
	int isActive;
	char *un,*up;

	if(obj->remoteOfferBlock!=NULL) {free(obj->remoteOfferBlock);}
	if(obj->offerBlock!=NULL) {free(obj->offerBlock);}
	
	obj->remoteOfferBlockLen = ILibWrapper_SdpToBlock(offer, offerLen, &isActive, &(obj->remoteUsername), &(obj->remotePassword), &(obj->remoteOfferBlock));

	offer[offerLen] = 0;
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->mFactory->ChainLink.ParentChain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "[ILibWrapperWebRTC] Set ICE/Offer: <br/>%s", offer);

	obj->offerBlockLen = ILibStun_SetIceOffer2(obj->mFactory->mStunModule, obj->remoteOfferBlock, obj->remoteOfferBlockLen, obj->offerBlock != NULL ? obj->localUsername : NULL, obj->offerBlock != NULL ? 8 : 0, obj->offerBlock != NULL ? obj->localPassword : NULL, obj->offerBlock != NULL ? 32 : 0, &obj->offerBlock);
	if( obj->offerBlockLen <= 0 ) {
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->mFactory->mChain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "[ILibWrapperWebRTC] Unable to set new offer");
		return NULL;
	}

	ILibWrapper_BlockToSDP(obj->offerBlock, obj->offerBlockLen, obj->isOfferInitiator, &un, &up, &sdp);

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->mFactory->ChainLink.ParentChain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "[ILibWrapperWebRTC] Return ICE/Response: <br/>%s", sdp);

	ILibWebRTC_SetUserObject(obj->mFactory->mStunModule, un, obj);
	strncpy(obj->localUsername, un, sizeof(obj->localUsername));
	strncpy(obj->localPassword, up, sizeof(obj->localPassword));

	free(un);
	free(up);

	if(onCandidates != NULL)
	{
		obj->OnCandidates = onCandidates;
		if(ILibWrapper_WebRTC_PerformStun(obj)!=0)
		{
			// No STUN Servers to try
			onCandidates(connection, NULL);
		}
	}

	return(sdp);
}

void ILibWrapper_WebRTC_Connection_SetRemoteParameters(ILibWrapper_WebRTC_Connection connection, char *username, int usernameLen, char *password, int passwordLen, char *certHash, int certHashLen)
{
	UNREFERENCED_PARAMETER(connection);
	UNREFERENCED_PARAMETER(username);
	UNREFERENCED_PARAMETER(usernameLen);
	UNREFERENCED_PARAMETER(password);
	UNREFERENCED_PARAMETER(passwordLen);
	UNREFERENCED_PARAMETER(certHash);
	UNREFERENCED_PARAMETER(certHashLen);
}

int ILibWrapper_WebRTC_Connection_GetLocalOfferBlock(ILibWrapper_WebRTC_Connection connection, char **localOfferBlock)
{
	*localOfferBlock = ((ILibWrapper_WebRTC_ConnectionStruct*)connection)->offerBlock;
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->offerBlockLen);
}
void ILibWrapper_WebRTC_Connection_CreateLocalParameters_ICE(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_Connection_GenerateOffer(connection, NULL);
}

void ILibWrapper_WebRTC_Connection_GetLocalParameters_ICE(ILibWrapper_WebRTC_Connection connection, char **username, int *usernameLen, char **password, int *passwordLen)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
	ILibWrapper_BlockToUserNamePWD(obj->offerBlock, obj->offerBlockLen, username, usernameLen, password, passwordLen);
}
void ILibWrapper_WebRTC_Connection_GetLocalParameters_DTLS(ILibWrapper_WebRTC_Connection connection, char **hash, int *hashLen)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
	*hashLen = (int)sizeof(obj->mFactory->tlsServerCertThumbprint);
	*hashLen = *hashLen * 2;
	*hashLen = *hashLen + (*hashLen / 2) + 1;

	*hash = (char*)malloc(*hashLen);
	util_tohex2(obj->mFactory->tlsServerCertThumbprint, (int)sizeof(obj->mFactory->tlsServerCertThumbprint), *hash);
	*hashLen = (int)strnlen_s(*hash, *hashLen);
}
void* ILibWrapper_WebRTC_Connection_GetExtraMemory(ILibWrapper_WebRTC_Connection connection)
{
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->extraMemory);
}
ILibTransport* ILibWrapper_WebRTC_Connection_GetRawTransport(ILibWrapper_WebRTC_Connection connection)
{
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->dtlsSession);
}
void* ILibWrapper_WebRTC_Connection_GetChain(ILibWrapper_WebRTC_Connection connection)
{
	return(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->mFactory->ChainLink.ParentChain);
}

char* ILibWrapper_WebRTC_Connection_GenerateOffer(ILibWrapper_WebRTC_Connection connection, ILibWrapper_WebRTC_OnConnectionCandidate onCandidates)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;

	char* offer;
	int offerLen = ILibStun_GenerateIceOffer(obj->mFactory->mStunModule, &offer, obj->localUsername, obj->localPassword);
	char *sdp;
	char* username,*password;

	if( offerLen <= 0 ) {
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->mFactory->mChain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "[ILibWrapperWebRTC] Unable to generate new offer");
		return NULL;
	}

	obj->isOfferInitiator = 1;
	ILibWrapper_BlockToSDP(offer, offerLen, obj->isOfferInitiator, &username, &password, &sdp);

	free(username);
	free(password);

	obj->offerBlock = offer;
	obj->offerBlockLen = offerLen;
	obj->OnCandidates = onCandidates;

	if(onCandidates != NULL)
	{
		if(ILibWrapper_WebRTC_PerformStun(obj)!=0)
		{
			// No STUN Servers to try
			onCandidates(connection, NULL);
		}
	}

	return(sdp);
}

char* ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToLocalSDP(ILibWrapper_WebRTC_Connection connection, struct sockaddr_in6* candidate)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
	char *sdp;
	char *username;
	char *password;
	char address[255];
	
	if(candidate!=NULL)
	{
		ILibInet_ntop2((struct sockaddr*)candidate, address, 255);
		ILibWrapper_BlockToSDPEx(obj->offerBlock, obj->offerBlockLen, &username, &password, &sdp, address, ntohs(candidate->sin6_port));
	}
	else
	{
		ILibWrapper_BlockToSDPEx(obj->offerBlock, obj->offerBlockLen, &username, &password, &sdp, NULL, 0);
	}
	

	free(username);
	free(password);

	return(sdp);
}

char* ILibWrapper_WebRTC_Connection_AddServerReflexiveCandidateToRemoteSDP(ILibWrapper_WebRTC_Connection connection, const char*address, int port)
{
    ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*) connection;
    char *sdp;
    char *username;
    char *password;
    
    ILibWrapper_BlockToSDPEx(obj->remoteOfferBlock, obj->remoteOfferBlockLen, &username, &password, &sdp, address, port);
    
    free(username);
    free(password);
    
    return(sdp);
}


void ILibWrapper_WebRTC_Connection_Pause(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_ConnectionStruct *cs = (ILibWrapper_WebRTC_ConnectionStruct*)connection;
	ILibSCTP_Pause(cs->dtlsSession);
}
void ILibWrapper_WebRTC_Connection_Resume(ILibWrapper_WebRTC_Connection connection)
{
	ILibWrapper_WebRTC_ConnectionStruct *cs = (ILibWrapper_WebRTC_ConnectionStruct*)connection;
	ILibSCTP_Resume(cs->dtlsSession);
}

ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_SendEx(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen, int dataType)
{
	ILibWrapper_WebRTC_ConnectionStruct *connection = (ILibWrapper_WebRTC_ConnectionStruct*)dataChannel->parent;

	return(ILibSCTP_SendEx(connection->dtlsSession, dataChannel->streamId, data, dataLen, dataType));
}
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_Send(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen)
{
	return(ILibWrapper_WebRTC_DataChannel_SendEx(dataChannel, data, dataLen, 53));
}
ILibTransport_DoneState ILibWrapper_WebRTC_DataChannel_SendString(ILibWrapper_WebRTC_DataChannel* dataChannel, char* data, int dataLen)
{
	return(ILibWrapper_WebRTC_DataChannel_SendEx(dataChannel, data, dataLen, 51));
}

ILibWrapper_WebRTC_DataChannel* ILibWrapper_WebRTC_DataChannel_Create(ILibWrapper_WebRTC_Connection connection, char* channelName, int channelNameLen, ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAckHandler)
{
	return(ILibWrapper_WebRTC_DataChannel_CreateEx(connection, channelName, channelNameLen, ILibWrapper_GetNextStreamId((ILibWrapper_WebRTC_ConnectionStruct*)connection), OnAckHandler));
}
void ILibWrapper_WebRTC_DataChannel_Close(ILibWrapper_WebRTC_DataChannel* dataChannel)
{
	ILibWrapper_WebRTC_ConnectionStruct *connection = (ILibWrapper_WebRTC_ConnectionStruct*)dataChannel->parent;
	ILibWebRTC_CloseDataChannel(connection->dtlsSession, dataChannel->streamId);
}
ILibWrapper_WebRTC_DataChannel* ILibWrapper_WebRTC_DataChannel_CreateEx(ILibWrapper_WebRTC_Connection connection, char* channelName, int channelNameLen, unsigned short streamId, ILibWrapper_WebRTC_DataChannel_OnDataChannelAck OnAckHandler)
{
	ILibWrapper_WebRTC_DataChannel *retVal = (ILibWrapper_WebRTC_DataChannel*)ILibChain_Link_Allocate(sizeof(ILibWrapper_WebRTC_DataChannel), ILibMemory_GetExtraMemorySize(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->extraMemory));

	retVal->parent = connection;
	if((retVal->channelName = (char*)malloc(channelNameLen+1))==NULL){ILIBCRITICALEXIT(254);}
	memcpy_s(retVal->channelName, channelNameLen + 1, channelName, channelNameLen);
	retVal->channelName[channelNameLen] = 0;

	retVal->streamId = streamId;
	retVal->OnAck = OnAckHandler;

	ILibSparseArray_Lock(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->DataChannels);
	ILibSparseArray_Add(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->DataChannels, retVal->streamId, retVal);
	ILibSparseArray_UnLock(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->DataChannels);

	ILibWrapper_InitializeDataChannel_Transport(retVal);

	ILibWebRTC_OpenDataChannel(((ILibWrapper_WebRTC_ConnectionStruct*)connection)->dtlsSession, streamId, channelName, channelNameLen);
	return(retVal);
}
void ILibWrapper_WebRTC_ConnectionFactory_SetTurnServer(ILibWrapper_WebRTC_ConnectionFactory factory, struct sockaddr_in6* turnServer, char* username, int usernameLength, char* password, int passwordLength, ILibWebRTC_TURN_ConnectFlags turnSetting)
{
	ILibWrapper_WebRTC_ConnectionFactoryStruct *cf = (ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory;
	cf->mTurnSetting = turnSetting;

	if(turnServer != NULL) {memcpy_s(&(cf->mTurnServer), sizeof(struct sockaddr_in6), turnServer, INET_SOCKADDR_LENGTH(turnServer->sin6_family));}

	if(turnSetting == ILibWebRTC_TURN_DISABLED)
	{
		ILibWebRTC_SetTurnServer(cf->mStunModule, NULL, NULL, 0, NULL, 0, ILibWebRTC_TURN_DISABLED);
	}
	else
	{
		ILibWebRTC_SetTurnServer(cf->mStunModule, turnServer, username, usernameLength, password, passwordLength, turnSetting);
	}
}
void ILibWrapper_WebRTC_Connection_SetUserData(ILibWrapper_WebRTC_Connection connection, void *user1, void *user2, void *user3)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*)connection;
	obj->User1 = user1;
	obj->User2 = user2;
	obj->User3 = user3;
}
void ILibWrapper_WebRTC_Connection_GetUserData(ILibWrapper_WebRTC_Connection connection, void **user1, void **user2, void **user3)
{
	ILibWrapper_WebRTC_ConnectionStruct *obj = (ILibWrapper_WebRTC_ConnectionStruct*)connection;
	if(user1!=NULL) {*user1 = obj->User1;}
	if(user2!=NULL) {*user2 = obj->User2;}
	if(user3!=NULL) {*user3 = obj->User3;}
}
void ILibWrapper_WebRTC_DataChannel_OnDebug(void *dtlsSession, char* debugField, int debugValue)
{
	ILibWrapper_WebRTC_ConnectionStruct *cs = (ILibWrapper_WebRTC_ConnectionStruct*)ILibWebRTC_GetUserObjectFromDtlsSession(dtlsSession);
	if(cs!=NULL && cs->OnDebugEvent!=NULL) {cs->OnDebugEvent(cs, debugField, debugValue);}
}
#ifdef _WEBRTCDEBUG
int ILibWrapper_WebRTC_Connection_Debug_Set(ILibWrapper_WebRTC_Connection connection, char* debugFieldName, ILibWrapper_WebRTC_Connection_Debug_OnEvent eventHandler)
{
	ILibWrapper_WebRTC_ConnectionStruct *cs = (ILibWrapper_WebRTC_ConnectionStruct*)connection;
	cs->OnDebugEvent = eventHandler;
	return(ILibSCTP_Debug_SetDebugCallback(cs->dtlsSession, debugFieldName, &ILibWrapper_WebRTC_DataChannel_OnDebug));
}
void ILibWrapper_WebRTC_ConnectionFactory_SetSimulatedLossPercentage(ILibWrapper_WebRTC_ConnectionFactory factory, int lossPercentage)
{
	ILibSCTP_SetSimulatedInboundLossPercentage(((ILibWrapper_WebRTC_ConnectionFactoryStruct*)factory)->mFactory, lossPercentage);
}
#endif
#endif
