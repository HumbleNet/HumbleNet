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

#if defined (__APPLE__)
#include <sys/uio.h>
#include <sys/mount.h>
#include <pthread.h>
//#include <CoreServices/CoreServices.h>
#else 
#ifdef NACL
#include <ifaddrs.h>
#include "chrome/nacl.h"
#endif
#if defined(_POSIX)
#ifndef _VX_CPU
#include <sys/statfs.h>
#endif
#include <pthread.h>
#include <execinfo.h>
#endif
#endif 

#ifdef _MINCORE
#define strncmp(a,b,c) strcmp(a,b)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(_MINCORE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#ifdef WIN32
	#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
	//silence warnings about inet_addr and WSASocketA
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#endif

	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>

	#ifndef _MINCORE
	#include <Wspiapi.h>
	#endif

	#include <Dbghelp.h>
#endif

#include <time.h>

#ifdef __APPLE__
#include <semaphore.h>
#include <ifaddrs.h>
#include <sys/sysctl.h>
#endif

#if defined(WIN32) && !defined(WSA_FLAG_NO_HANDLE_INHERIT)
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

#include "ILibParsers.h"
#include "ILibRemoteLogging.h"
#include "ILibCrypto.h"

#define MINPORTNUMBER 50000
#define PORTNUMBERRANGE 15000
#define UPNP_MAX_WAIT 86400			// 24 Hours

#ifdef _REMOTELOGGINGSERVER
#include "ILibWebServer.h"
#endif

// This is to compile on Windows XP, not needed at runtime
#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif

#if defined(WIN32) || defined(_WIN32_WCE)
/*static*/ sem_t ILibChainLock = { 0 };
#else
#ifndef errno
extern int errno;
#endif
/*static*/ sem_t ILibChainLock;
#endif

#ifdef WIN32
char* ILibCriticalLogFilename = NULL;
#else
char* ILibCriticalLogFilename = NULL;
#endif

#ifdef _MINCORE // Win32 mincore needs these defines
CONST IN6_ADDR in6addr_any = { { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };
CONST IN6_ADDR in6addr_loopback = { { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } };
#endif

char ILibScratchPad[4096];   // General buffer
char ILibScratchPad2[65536]; // Often used for UDP packet processing
void* gILibChain = NULL;	 // Global Chain Instance used for Remote Logging when a chain instance isn't otherwise exposed

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))

long long ILibGetUptime();
/*static*/ int ILibChainLock_RefCounter = 0;

static int malloc_counter = 0;
void* dbg_malloc(int sz)
{
	++malloc_counter;
	return((void*)malloc(sz));
}
void dbg_free(void* ptr)
{
	--malloc_counter;
	free(ptr);	
}
int dbg_GetCount()
{
	return(malloc_counter);
}

unsigned long long ILibHTONLL(unsigned long long v)
{
	{ union { unsigned long lv[2]; unsigned long long llv; } u; u.lv[0] = htonl(v >> 32); u.lv[1] = htonl(v & 0xFFFFFFFFULL); return u.llv; }
}
unsigned long long ILibNTOHLL(unsigned long long v)
{
	{ union { unsigned long lv[2]; unsigned long long llv; } u; u.llv = v; return ((unsigned long long)ntohl(u.lv[0]) << 32) | (unsigned long long)ntohl(u.lv[1]); }
}

//! Returns X where 2^X = Specified Integer
/*!
	\param number Integer value to use to solve equation
	\return X where 2^X = Specified Integer
*/
int ILibWhichPowerOfTwo(int number)
{
	int retVal = 0;
	if(number < 1) {return(-1);}
	while((number = number >> 1) != 0) {++retVal;}
	return(retVal);
}


//
// All of the following structures are meant to be private internal structures
//
typedef struct ILibSparseArray_Node
{
	int index;
	void* ptr;
}ILibSparseArray_Node;
typedef struct ILibSparseArray_Root
{
	ILibSparseArray_Node* bucket;
	int bucketSize;
	ILibSparseArray_Bucketizer bucketizer;
	sem_t LOCK;
	int userMemorySize;
}ILibSparseArray_Root;
const int ILibMemory_SparseArray_CONTAINERSIZE = sizeof(ILibSparseArray_Root);

typedef enum ILibHashtable_Flags
{
	ILibHashtable_Flags_NONE = 0x00,
	ILibHashtable_Flags_ADD = 0x01,
	ILibHashtable_Flags_REMOVE = 0x02
}ILibHashtable_Flags;
typedef struct ILibHashtable_Node
{
	struct ILibHashtable_Node* next;
	struct ILibHashtable_Node* prev;
	void* Key1;
	char* Key2;
	int Key2Len;
	void *Data;
}ILibHashtable_Node;
typedef struct ILibHashtable_Root
{
	ILibSparseArray table;
	ILibHashtable_Hash_Func hashFunc;
	ILibSparseArray_Bucketizer bucketizer;
}ILibHashtable_Root;



struct ILibStackNode
{
	void *Data;
	struct ILibStackNode *Next;
};
struct ILibQueueNode
{
	struct ILibStackNode *Head;
	struct ILibStackNode *Tail;
	sem_t LOCK;
};
struct HashNode_Root
{
	struct HashNode *Root;
	int CaseInSensitive;
	sem_t LOCK;
};
struct HashNode
{
	struct HashNode *Next;
	struct HashNode *Prev;
	int KeyHash;
	char *KeyValue;
	int KeyLength;
	void *Data;
	int DataEx;
};
struct HashNodeEnumerator
{
	struct HashNode *node;
};
typedef struct ILibLinkedListNode
{
	void *Data;
	struct ILibLinkedListNode_Root *Root;
	struct ILibLinkedListNode *Next;
	struct ILibLinkedListNode *Previous;
}ILibLinkedListNode;
typedef struct ILibLinkedListNode_Root
{
	sem_t LOCK;
	long count;
	void* Tag;
	struct ILibLinkedListNode *Head;
	struct ILibLinkedListNode *Tail;
	void *ExtraMemory;
}ILibLinkedListNode_Root;

struct ILibReaderWriterLock_Data
{
	ILibChain_PreSelect Pre;
	ILibChain_PostSelect Post;
	ILibChain_Destroy Destroy;

	sem_t WriteLock;
	sem_t ReadLock;
	sem_t CounterLock;
	sem_t ExitLock;
	int ActiveReaders;
	int WaitingReaders;
	int PendingWriters;
	int Exit;
};
//! Get the amount of pending data in the send buffer of the ILibTransport Abstraction
/*!
	\param transport ILibTransport Abstraction to query
	\return Number of pending bytes
*/
unsigned int ILibTransport_PendingBytesToSend(void *transport)
{
	return(((ILibTransport*)transport)->PendingBytesPtr(transport));
}
//! Send data on an ILibTransport Abstraction
/*!
	\param transport ILibTransport Abstraction
	\param buffer Data to send
	\param bufferLength Data Length to send
	\param ownership Who will free the data to send?
	\param done Data complete?
	\return Send Status
*/
ILibTransport_DoneState ILibTransport_Send(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done)
{
	return(((ILibTransport*)transport)->SendPtr(transport, buffer, bufferLength, ownership, done));
}
//! Close an ILibTransport Abstraction
/*!
	\param transport ILibTransport Abstraction to close
*/
void ILibTransport_Close(void *transport)
{
	if (transport != NULL) { ((ILibTransport*)transport)->ClosePtr(transport); }
}

#ifdef WIN32
int ILibGetLocalIPAddressNetMask(unsigned int address)
{
	SOCKET s;
	DWORD bytesReturned;
	SOCKADDR_IN* pAddrInet;
	INTERFACE_INFO localAddr[10];  // Assume there will be no more than 10 IP interfaces 
	int numLocalAddr; 
	int i;

	if ((s = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0)) == INVALID_SOCKET)
	{
		fprintf(stderr, "Socket creation failed\n");
		return(0);
	}

	// Enumerate all IP interfaces
	if (WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, &localAddr, sizeof(localAddr), &bytesReturned, NULL, NULL) != SOCKET_ERROR)
	{
		fprintf(stderr, "WSAIoctl fails with error %d\n", (int)GetLastError());
		closesocket(s);
		return(0);
	}

	closesocket(s);

	// Display interface information
	numLocalAddr = (bytesReturned/sizeof(INTERFACE_INFO));
	for (i=0; i<numLocalAddr; i++) 
	{

		pAddrInet = (SOCKADDR_IN*)&localAddr[i].iiAddress;
		if (pAddrInet->sin_addr.S_un.S_addr == address)
		{
			pAddrInet = (SOCKADDR_IN*)&localAddr[i].iiNetmask;
			return(pAddrInet->sin_addr.s_addr);
		}
	}
	return(0);
}
#endif

/*
#if defined(__APPLE__)
// #if defined (__IPHONE_OS_VERSION_MIN_REQUIRED)  // lame check for iPhone

// several places the same check for interface, two functions identical, but returning 
// slightly different form of interfaces

// Helper functions
typedef void * (* FN_GETMEM) (int ctr);
typedef void (* FN_STOREVAL) (int ctr, void * p, struct ifaddrs * ifa);

static int IsMacIPv4Address (struct ifaddrs * ifa);
static int GetMacIPv4AddressesCount (struct ifaddrs * ifa);
static void StoreValInt (int ctr, void * p, struct ifaddrs * ifa);
static void * GetAddressMemInt (int ctr);
static void StoreValStruct (int ctr, void * p, struct ifaddrs * ifa);
static void * GetAddressMemStruct (int ctr);

int IsMacIPv4Address(struct ifaddrs * ifa)
{
	// This should pick up eth0.. (desktop) or en0.. (mac / iphone)
#if defined (__IPHONE_OS_VERSION_MIN_REQUIRED)
	return (ifa->ifa_addr->sa_family == AF_INET &&      // is it IP4
		strncmp (ifa->ifa_name, "pdp_ip", 6) != 0 &&	// iPhone is using it for 3G
		strncmp (ifa->ifa_name, "lo0", 3) != 0);		// is it not lo0
#else
	return (ifa->ifa_addr->sa_family == AF_INET &&      // is it IP4
		strncmp (ifa->ifa_name, "lo0", 3) != 0);		// is it not lo0
#endif
}

int GetMacIPv4AddressesCount(struct ifaddrs * pifa)
{
	int ctr = 0;
	while (pifa != NULL)
	{
		if (IsMacIPv4Address (pifa)) ctr++;
		pifa = pifa->ifa_next;
	}
	return ctr;
}

void StoreValInt(int ctr, void * p, struct ifaddrs * pifa)
{
	if (!p || !pifa) return;
	*((int *)p + ctr) = ((struct sockaddr_in *)pifa->ifa_addr)->sin_addr.s_addr;
}

void* GetAddressMemInt(int ctr)
{
	int * p;
	if (0 >= ctr) return NULL;
	if ((p = (int *)malloc (sizeof (int) * ctr)) == NULL) ILIBCRITICALEXIT(254);
	return (void *)p;
}

void StoreValStruct(int ctr, void * p, struct ifaddrs * pifa)
{
	if (!p || !pifa) return;
	memcpy ((struct sockaddr_in *)p + ctr, (struct sockaddr_in *)pifa->ifa_addr, sizeof(struct sockaddr_in));
}

void* GetAddressMemStruct(int ctr)
{
	struct sockaddr_in *p;
	if (0 >= ctr) return NULL;
	if ((p = (struct sockaddr_in *)malloc (sizeof (struct sockaddr_in) * ctr)) == NULL) ILIBCRITICALEXIT(254);
	return (void *)p;
}

int GetMacIPv4Addresses(void** pp, FN_GETMEM fnGetMem, FN_STOREVAL fnStoreVal)
{
	int ctr = 0;
	struct ifaddrs * ifaddr, * ifa;
	*pp = NULL;

	if (getifaddrs(&ifaddr) != 0) return 0;

	// Count valid IPv4 interfaces
	if (0 < (ctr = GetMacIPv4AddressesCount (ifaddr)))
	{
		*pp = (*fnGetMem)(ctr);

		ctr = 0;
		ifa = ifaddr;
		// Copy valid IPv4 interfaces addresses
		while (NULL != ifa) 
		{
			if (IsMacIPv4Address(ifa))
			{
				(*fnStoreVal)(ctr, *pp, ifa);
				ctr++;
			}

			ifa = ifa->ifa_next;
		}
	}

	freeifaddrs(ifaddr);
	return ctr;
}
#endif
*/

//! Fetches the list of valid IPv4 adapters
/*!
	\param[out] addresslist sockaddr_in array of adapters
	\param includeloopback [0 = Include Loopback, 1 = Exclude Loopback]
	\return number of adapters
*/
int ILibGetLocalIPv4AddressList(struct sockaddr_in** addresslist, int includeloopback)
{
#ifdef WIN32
	DWORD i, j = 0;
	SOCKET sock;
	DWORD interfaces_count;
	INTERFACE_INFO interfaces[128];

	// Fetch the list of interfaces
	*addresslist = NULL;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, &interfaces, 128 * sizeof(INTERFACE_INFO), &interfaces_count, NULL, NULL) != 0) { interfaces_count = 0; }

	if (interfaces_count > 0)
	{
		interfaces_count = interfaces_count / sizeof(INTERFACE_INFO);
		for (i = 0; i < interfaces_count; i++) { if (includeloopback != 0 || interfaces[i].iiAddress.AddressIn.sin_addr.S_un.S_addr != 0x0100007F) j++; } // M S Static Analysis says this could read outside readable range, but that is only true if WSAioct returns the wrong value in interfaces_count

		// Allocate the IPv4 list and copy the elements into it
		if ((*addresslist = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * j)) == NULL) ILIBCRITICALEXIT(254);
		j = 0;
		for (i = 0; i < interfaces_count; i++)
		{
			if (includeloopback != 0 || interfaces[i].iiAddress.AddressIn.sin_addr.S_un.S_addr != 0x0100007F)
			{
				memcpy_s(&((*addresslist)[j++]), sizeof(struct sockaddr_in), &(interfaces[i].iiAddress.AddressIn), sizeof(struct sockaddr_in));
			}
		}
	}
	closesocket(sock);
	return j;
#elif __APPLE__
	//
	// Enumerate local interfaces
	//
	//return GetMacIPv4Addresses ((void **)addresslist, GetAddressMemStruct, StoreValStruct);
	int ctr = 0;
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1) { return 0; }

	// Count valid IPv4 interfaces
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET && (includeloopback != 0 || ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr != htonl(INADDR_LOOPBACK))) ctr++;
	}
	if ((*addresslist = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * ctr)) == NULL) ILIBCRITICALEXIT(254);

	// Copy valid IPv4 interfaces addresses
	ctr = 0;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET && (includeloopback != 0 || ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr != htonl(INADDR_LOOPBACK)))
		{
			memcpy_s(&((*addresslist)[ctr]), sizeof(struct sockaddr_in), ifa->ifa_addr, sizeof(struct sockaddr_in));
			ctr++;
		}
	}

	freeifaddrs(ifaddr);
	return ctr;
#elif NACL
	int cnt, ctr, i, withLoopBackCnt;
	struct sockaddr_in * addressesNacl=NULL;
	cnt = ni_getCurrentIpV4s(&addressesNacl);
	if(NULL==addressesNacl)
	{
		*addresslist=NULL;
		ni_releaseCurrentIpV4s();
		return 0;
	}
	withLoopBackCnt=0;
	for (i=0; i<cnt; i++)
	{
		if (includeloopback != 0 || ((struct sockaddr_in)addressesNacl[i]).sin_addr.s_addr != htonl(INADDR_LOOPBACK))
		{
			withLoopBackCnt++;
		}
	}
	if ((*addresslist = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * withLoopBackCnt)) == NULL) ILIBCRITICALEXIT(254);
	ctr = 0;
	for (i=0; i<cnt; i++)
	{
		if (includeloopback != 0 || ((struct sockaddr_in)addressesNacl[i]).sin_addr.s_addr != htonl(INADDR_LOOPBACK))
		{
			memcpy_s(&((*addresslist)[ctr]), sizeof(struct sockaddr_in), &((addressesNacl)[i]), sizeof(struct sockaddr_in));
			//printf("saddr = %d, %s: %d\n", ((*addresslist)[ctr]).sin_family, inet_ntoa(((*addresslist)[ctr]).sin_addr), ((*addresslist)[ctr]).sin_port); fflush(stdout);
			ctr++;
		}
	}
	ni_releaseCurrentIpV4s();
	return ctr;



#elif defined(_POSIX)
	//
	// Posix will also use an Ioctl call to get the IPAddress List
	//
	char szBuffer[16*sizeof(struct ifreq)];
	struct ifconf ifConf;
	struct ifreq ifReq;
	int nResult;
	int LocalSock;
	struct sockaddr_in LocalAddr;
	//int tempresults[16];
	int ctr = 0;
	int i;
	if ((*addresslist = malloc(sizeof(struct sockaddr_in) * 32)) == NULL) ILIBCRITICALEXIT(254); // TODO: Support more than 32 adapters

	// Create an unbound datagram socket to do the SIOCGIFADDR ioctl on.
	if ((LocalSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "socket(..) error in ILibGetLocalIPv4AddressList()");
		ILIBCRITICALERREXIT(253);
	}
	// Get the interface configuration information...
	ifConf.ifc_len = sizeof szBuffer;
	ifConf.ifc_ifcu.ifcu_buf = (caddr_t)szBuffer;
	nResult = ioctl(LocalSock, SIOCGIFCONF, &ifConf);
	
	if (nResult < 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ioctl(SIOCGIFCONF) error in ILibGetLocalIPv4AddressList()");
		ILIBCRITICALERREXIT(253);
	}
	// Cycle through the list of interfaces looking for IP addresses.
	for (i = 0;(i < ifConf.ifc_len);)
	{
		struct ifreq *pifReq = (struct ifreq *)((caddr_t)ifConf.ifc_req + i);
		i += sizeof *pifReq;
		// See if this is the sort of interface we want to deal with.
		strcpy (ifReq.ifr_name, pifReq -> ifr_name);
		if (ioctl(LocalSock, SIOCGIFFLAGS, &ifReq) < 0)
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ioctl(SIOCGIFFLAGS) error in ILibGetLocalIPv4AddressList()");
			ILIBCRITICALERREXIT(253);
		}
		// Skip loopback, point-to-point and down interfaces, except don't skip down interfaces
		// if we're trying to get a list of configurable interfaces.
		if ((ifReq.ifr_flags & IFF_LOOPBACK) || (!(ifReq.ifr_flags & IFF_UP)))
		{
			continue;
		}	
		if (pifReq->ifr_addr.sa_family == AF_INET)
		{
			// Get a pointer to the address.
			memcpy_s(&LocalAddr, sizeof(LocalAddr), &pifReq -> ifr_addr, sizeof pifReq -> ifr_addr);
			if (includeloopback != 0 || LocalAddr.sin_addr.s_addr != htonl(INADDR_LOOPBACK))
			{
				//tempresults[ctr] = LocalAddr.sin_addr.s_addr;
				memcpy_s(&((*addresslist)[ctr]), sizeof(struct sockaddr_in), &LocalAddr, sizeof(struct sockaddr_in));
				++ctr;
			}
		}
	}
	close(LocalSock);
	//*pp_int = (int*)malloc(sizeof(int)*(ctr));
	//memcpy(*pp_int,tempresults,sizeof(int)*ctr);

	return ctr;
#endif
}

/*
// Get the list of valid IPv6 adapters, skip loopback and other junk adapters.
int ILibGetLocalIPv6AddressList(struct sockaddr_in6** addresslist)
{
#if defined(WINSOCK2)
PIP_ADAPTER_ADDRESSES pAddresses = NULL;
PIP_ADAPTER_ADDRESSES pAddressesPtr = NULL;
PIP_ADAPTER_UNICAST_ADDRESS_LH AddrPtr;
ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
int AddrCount = 0;
struct sockaddr_in6 *ptr;

// Perform first call
pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
if (GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
{
free(pAddresses);
pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
}

// Perform second call
if (GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) != NO_ERROR)
{
free(pAddresses);
return 0;
}

// Count the total number of local IPv6 addresses
pAddressesPtr = pAddresses;
while (pAddressesPtr != NULL)
{
// Skip any loopback adapters
if (pAddressesPtr->PhysicalAddressLength != 0 && pAddressesPtr->NoMulticast == 0)
{
// For each adapter
AddrPtr = pAddressesPtr->FirstUnicastAddress;
while (AddrPtr != NULL)
{
AddrCount++;
AddrPtr = AddrPtr->Next;
}
}
pAddressesPtr = pAddressesPtr->Next;
}

// Allocate the array of IPv6 addresses
*addresslist = ptr = malloc(AddrCount * sizeof(struct sockaddr_in6));

// Copy each address into the array
pAddressesPtr = pAddresses;
while (pAddressesPtr != NULL)
{
// Skip any loopback adapters
if (pAddressesPtr->PhysicalAddressLength != 0 && pAddressesPtr->NoMulticast == 0)
{
// For each adapter
AddrPtr = pAddressesPtr->FirstUnicastAddress;
while (AddrPtr != NULL)
{
memcpy(ptr, AddrPtr->Address.lpSockaddr, sizeof(struct sockaddr_in6));
ptr++;
AddrPtr = AddrPtr->Next;
}
}
pAddressesPtr = pAddressesPtr->Next;
}

free(pAddresses);
return AddrCount;
#endif
}
*/

//! Get the list of valid IPv6 adapter indexes, skip loopback and other junk adapters.
/*!
	\param[out] indexlist List of valid IPv6 Adapter indexes
	\return number of adapters
*/
int ILibGetLocalIPv6IndexList(int** indexlist)
{
#ifdef _MINCORE
	return 0;
#else
#if defined(WIN32)
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pAddressesPtr = NULL;
	ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
	int AddrCount = 0;
	int* ptr;

	// Perform first call
	if ((pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen)) == NULL) ILIBCRITICALEXIT(254);
	if (GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAddresses);
		if ((pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen)) == NULL) ILIBCRITICALEXIT(254);
	}

	// Perform second call
	if (GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) != NO_ERROR)
	{
		free(pAddresses);
		return 0;
	}

	// Count the total number of local IPv6 addresses
	pAddressesPtr = pAddresses;
	while (pAddressesPtr != NULL)
	{
		// Skip any loopback adapters
		if (pAddressesPtr->PhysicalAddressLength != 0 && pAddressesPtr->ReceiveOnly != 1 && pAddressesPtr->NoMulticast != 1 && pAddressesPtr->OperStatus == IfOperStatusUp)
		{
			// For each adapter
			AddrCount++;
		}
		pAddressesPtr = pAddressesPtr->Next;
	}

	// Allocate the array of IPv6 addresses
	if ((*indexlist = ptr = (int*)malloc(AddrCount * sizeof(int))) == NULL) ILIBCRITICALEXIT(254);

	// Copy each address into the array
	pAddressesPtr = pAddresses;
	while (pAddressesPtr != NULL)
	{
		// Skip any loopback adapters
		if (pAddressesPtr->PhysicalAddressLength != 0 && pAddressesPtr->ReceiveOnly != 1 && pAddressesPtr->NoMulticast != 1 && pAddressesPtr->OperStatus == IfOperStatusUp)
		{
			*ptr = pAddressesPtr->IfIndex;
			ptr++;
		}
		pAddressesPtr = pAddressesPtr->Next;
	}

	free(pAddresses);
	return AddrCount;
#elif defined (_POSIX) || defined (__APPLE__)
	*indexlist = (int *)malloc(sizeof(int));
	*indexlist[0] = 0;
	return 1; // TODO
#endif
#endif
}

//! Get the list of valid IPv6 adapters, skip loopback and other junk adapters.
/*!
	\param[out] list sockaddr_in6 array of valid IPv6 adapters
	\return number of adapters
*/
int ILibGetLocalIPv6List(struct sockaddr_in6** list)
{
#ifdef _MINCORE
	return 0;
#else
#if defined(WIN32)
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES pAddressesPtr = NULL;
	ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
	int AddrCount = 0;
	struct sockaddr_in6* ptr;
	*list = NULL;

	// Perform first call
	pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
	if (GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAddresses);
		pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
	}

	// Perform second call
	if (GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST, NULL, pAddresses, &outBufLen) != NO_ERROR)
	{
		free(pAddresses);
		return 0; // Call failed, return no IPv6 addresses. This happens on Windows XP.
	}

	// Count the total number of local IPv6 addresses
	pAddressesPtr = pAddresses;
	while (pAddressesPtr != NULL)
	{
		// Skip any loopback adapters
		if (pAddressesPtr->PhysicalAddressLength != 0 && pAddressesPtr->ReceiveOnly != 1 && pAddressesPtr->NoMulticast != 1 && pAddressesPtr->OperStatus == IfOperStatusUp)
		{
			// For each adapter
			AddrCount++;
		}
		pAddressesPtr = pAddressesPtr->Next;
	}

	if (AddrCount > 0)
	{
		// Allocate the array of IPv6 addresses
		*list = ptr = (struct sockaddr_in6*)malloc(AddrCount * sizeof(struct sockaddr_in6));

		// Copy each address into the array
		pAddressesPtr = pAddresses;
		while (pAddressesPtr != NULL)
		{
			// Skip any loopback adapters
			if (pAddressesPtr->PhysicalAddressLength != 0 && pAddressesPtr->ReceiveOnly != 1 && pAddressesPtr->NoMulticast != 1 && pAddressesPtr->OperStatus == IfOperStatusUp)
			{
				memcpy_s(ptr, sizeof(struct sockaddr_in6), pAddressesPtr->FirstUnicastAddress->Address.lpSockaddr, pAddressesPtr->FirstUnicastAddress->Address.iSockaddrLength);
				ptr++;
			}
			pAddressesPtr = pAddressesPtr->Next;
		}
	}

	free(pAddresses);
	return AddrCount;
#elif defined (_POSIX) || defined (__APPLE__)
	//*indexlist = malloc(sizeof(int));
	//*indexlist[0] = 0;
	//return 1; // TODO
	*list = NULL;
	return 0;
#endif
#endif
}


/*! \fn ILibGetLocalIPAddressList(int** pp_int)
\brief Gets a list of IP Addresses
\par
\b NOTE: \a pp_int must be freed
\param[out] pp_int Array of IP Addresses
\return Number of IP Addresses returned
*/
int ILibGetLocalIPAddressList(int** pp_int)
{
#if defined(NACL)
	return 0;
	//
	// Winsock2 will use an Ioctl call to fetch the IPAddress list
	//
#elif defined(WIN32)
	int i;
	char buffer[16 * sizeof(SOCKET_ADDRESS_LIST)];
	DWORD bufferSize;
	SOCKET sock;
	LPSOCKADDR addr;

	*pp_int = NULL;
	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) == -1) return 0;

	if (WSAIoctl(sock, SIO_ADDRESS_LIST_QUERY, NULL, 0, buffer, 16 * sizeof(SOCKET_ADDRESS_LIST), &bufferSize, NULL, NULL) == SOCKET_ERROR)
	{
		closesocket(sock);
		return 0;
	}

	if ((*pp_int = (int*)malloc(sizeof(int) * (1 + ((SOCKET_ADDRESS_LIST*)buffer)->iAddressCount))) == NULL) ILIBCRITICALEXIT(254);
	addr = ((((SOCKET_ADDRESS_LIST *)buffer))->Address)->lpSockaddr;
	for (i = 0; i < ((SOCKET_ADDRESS_LIST*)buffer)->iAddressCount && i < 15; ++i)
	{
		(*pp_int)[i] = (((struct sockaddr_in*)addr)[i]).sin_addr.S_un.S_addr;
	}
	(*pp_int)[i] = inet_addr("127.0.0.1");
	closesocket(sock);
	return(1 + ((SOCKET_ADDRESS_LIST*)buffer)->iAddressCount);
#elif defined(__APPLE__)
	//
	// Enumerate local interfaces
	// Tested on Mac/iPhone, not yet on Mac OSX
	//return GetMacIPv4Addresses((void **)pp_i+nt, GetAddressMemInt, StoreValInt);

	int ctr = 0;
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1) return 0;

	// Count valid IPv4 interfaces
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET && ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) ctr++;
	}
	if ((*pp_int = (int*)malloc(sizeof(int) * ctr)) == NULL) ILIBCRITICALEXIT(254);

	// Copy valid IPv4 interfaces addresses
	ctr = 0;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET && ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
		{
			(*pp_int)[ctr] = ((struct sockaddr_in*)(ifa->ifa_addr))->sin_addr.s_addr;
			ctr++;
		}
	}

	freeifaddrs(ifaddr);
	return ctr;
#elif defined(_POSIX)
	//
	// Posix will also use an Ioctl call to get the IPAddress List
	//
	char szBuffer[16*sizeof(struct ifreq)];
	struct ifconf ifConf;
	struct ifreq ifReq;
	int nResult;
	int LocalSock;
	struct sockaddr_in LocalAddr;
	int tempresults[16];
	int ctr=0;
	int i;
	/* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. */
	if ((LocalSock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "socket(...) error in ILibGetLocalIPAddressList()");
		ILIBCRITICALERREXIT(253);
	}
	/* Get the interface configuration information... */
	ifConf.ifc_len = sizeof szBuffer;
	ifConf.ifc_ifcu.ifcu_buf = (caddr_t)szBuffer;
	nResult = ioctl(LocalSock, SIOCGIFCONF, &ifConf);
	if (nResult < 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ioctl(SIOCGIFCONF) error in ILibGetLocalIPAddressList()");
		ILIBCRITICALERREXIT(253);
	}
	/* Cycle through the list of interfaces looking for IP addresses. */
	for (i = 0;(i < ifConf.ifc_len);)
	{
		struct ifreq *pifReq = (struct ifreq *)((caddr_t)ifConf.ifc_req + i);
		i += sizeof *pifReq;
		/* See if this is the sort of interface we want to deal with. */
		strcpy (ifReq.ifr_name, pifReq -> ifr_name);
		if (ioctl (LocalSock, SIOCGIFFLAGS, &ifReq) < 0)
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ioctl(SIOCGIFFLAGS) error in ILibGetLocalIPAddressList()");
			ILIBCRITICALERREXIT(253);
		}
		/* Skip loopback, point-to-point and down interfaces, */
		/* except don't skip down interfaces */
		/* if we're trying to get a list of configurable interfaces. */
		if ((ifReq.ifr_flags & IFF_LOOPBACK) || (!(ifReq.ifr_flags & IFF_UP)))
		{
			continue;
		}	
		if (pifReq -> ifr_addr.sa_family == AF_INET)
		{
			/* Get a pointer to the address... */
			memcpy_s (&LocalAddr, sizeof(LocalAddr), &pifReq -> ifr_addr, sizeof pifReq -> ifr_addr);
			if (LocalAddr.sin_addr.s_addr != htonl (INADDR_LOOPBACK))
			{
				tempresults[ctr] = LocalAddr.sin_addr.s_addr;
				++ctr;
			}
		}
	}
	close(LocalSock);
	if ((*pp_int = (int*)malloc(sizeof(int)*(ctr))) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(*pp_int, sizeof(int)*ctr, tempresults, sizeof(int)*ctr);
	return(ctr);
#endif
}



struct ILibChain_SubChain
{
	ILibChain_PreSelect PreSelect;
	ILibChain_PostSelect PostSelect;
	ILibChain_Destroy Destroy;

	void *subChain;
};

typedef struct LifeTimeMonitorData
{
	long long ExpirationTick;
	void *data;
	ILibLifeTime_OnCallback CallbackPtr;
	ILibLifeTime_OnCallback DestroyPtr;
}LifeTimeMonitorData;
struct ILibLifeTime
{
	ILibChain_Link ChainLink;
	struct LifeTimeMonitorData *LM;
	long long NextTriggerTick;

	void *Reserved;

	void *ObjectList;
	int ObjectCount;
};

typedef struct ILibChain_Link_Hook
{
	ILibChain_EventHookHandler Handler;
	int MaxTimeout;
}ILibChain_Link_Hook;
struct ILibBaseChain_SafeData
{
	void *Chain;
	void *Object;
};

typedef enum ILibChain_ContinuationStates
{
	ILibChain_ContinuationState_INACTIVE	= 0,
	ILibChain_ContinuationState_CONTINUE	= 1,
	ILibChain_ContinuationState_END_CONTINUE= 2
}ILibChain_ContinuationStates;

typedef struct ILibBaseChain
{
	int TerminateFlag;
	int RunningFlag;
#ifdef _REMOTELOGGING
	ILibRemoteLogging ChainLogger;
#ifdef _REMOTELOGGINGSERVER
	void* LoggingWebServer;
	ILibTransport* LoggingWebServerFileTransport;
#endif
#endif
	/*
	*
	* DO NOT MODIFY STRUCT DEFINITION ABOVE THIS COMMENT BLOCK
	*
	*/


#if defined(WIN32)
	DWORD ChainThreadID;
	HANDLE ChainProcessHandle;
	HANDLE MicrostackThreadHandle;
	CONTEXT MicrostackThreadContext;
#else
	pthread_t ChainThreadID;
#endif
#if defined(WIN32)
	SOCKET TerminateSock;
#else
	FILE *TerminateReadPipe;
	FILE *TerminateWritePipe;
#endif

	void *Timer;
	void *Reserved;
	ILibLinkedList Links;
	ILibLinkedList LinksPendingDelete;
	ILibHashtable ChainStash;
	ILibChain_ContinuationStates continuationState;
	unsigned int PreSelectCount;
	unsigned int PostSelectCount;
	void *WatchDogThread;
#ifdef WIN32
	HANDLE WatchDogTerminator;
#else
	int WatchDogTerminator[2];
#endif
	void *node;
}ILibBaseChain;

const int ILibMemory_CHAIN_CONTAINERSIZE = sizeof(ILibBaseChain);

void* ILibMemory_Allocate(int containerSize, int extraMemorySize, void** allocatedContainer, void **extraMemory)
{
	char* retVal = (char*)malloc(containerSize + extraMemorySize + (extraMemorySize > 0 ? 4 : 0));
	if (retVal == NULL) { ILIBCRITICALEXIT(254); }
	memset(retVal, 0, containerSize + extraMemorySize + (extraMemorySize > 0 ? 4 : 0));
	if (extraMemorySize > 0)
	{
		((int*)(retVal + containerSize))[0] = extraMemorySize;
		if (extraMemory != NULL) { *extraMemory = (retVal + containerSize + 4); }
	}
	else
	{
		if (extraMemory != NULL) { *extraMemory = NULL; }
	}
	if (allocatedContainer != NULL) { *allocatedContainer = retVal; }
	return(retVal);
}
ILibExportMethod void* ILibMemory_GetExtraMemory(void *container, int containerSize)
{
	return((char*)container + 4 + containerSize);
}
int ILibMemory_GetExtraMemorySize(void* extraMemory)
{
	if (extraMemory == NULL) { return(0); }
	return(((int*)((char*)extraMemory - 4))[0]);
}

ILibChain_Link* ILibChain_Link_Allocate(int structSize, int extraMemorySize)
{
	ILibChain_Link *retVal;
	void* extraMemory;
	ILibMemory_Allocate(structSize, extraMemorySize, (void**)&retVal, &extraMemory);
	retVal->ExtraMemoryPtr = extraMemory;
	return(retVal);
}
int ILibChain_Link_GetExtraMemorySize(ILibChain_Link* link)
{
	return(ILibMemory_GetExtraMemorySize(link->ExtraMemoryPtr));
}
//! Get the base Hashtable for this chain

/*!
	\b Note: Hashtable is created upon first use
	\param chain Microstack Chain to fetch the hashtable from
	\return Associated Hashtable
*/
ILibHashtable ILibChain_GetBaseHashtable(void* chain)
{
	struct ILibBaseChain *b = (struct ILibBaseChain*)chain;
	if(b->ChainStash == NULL) {b->ChainStash = ILibHashtable_Create();}
	return(b->ChainStash);
}

void ILibChain_OnDestroyEvent_Sink(void *object)
{
	ILibChain_Link *link = (ILibChain_Link*)object;
	ILibChain_DestroyEvent e = (ILibChain_DestroyEvent)((void**)link->ExtraMemoryPtr)[0];

	if (e != NULL)
	{
		e(link->ParentChain, ((void**)link->ExtraMemoryPtr)[1]);
	}
}
//! Add an event handler to be dispatched when the Microstack Chain is shutdown
/*! 
	\param chain Microstack Chain to add an event handler to
	\param sink	Event Handler to dispatch on shutdown
	\param user Custom user state data to dispatch
*/
void ILibChain_OnDestroyEvent_AddHandler(void *chain, ILibChain_DestroyEvent sink, void *user)
{
	ILibChain_Link *link = ILibChain_Link_Allocate(sizeof(ILibChain_Link), 2 * sizeof(void*));
	link->ParentChain = chain;
	link->DestroyHandler = (ILibChain_Destroy)&ILibChain_OnDestroyEvent_Sink;
	((void**)link->ExtraMemoryPtr)[0] = sink;
	((void**)link->ExtraMemoryPtr)[1] = user;

	if (ILibIsChainRunning(chain) == 0)
	{
		ILibAddToChain(chain, link);
	}
	else
	{
		ILibChain_SafeAdd(chain, link);
	}
}
void ILibChain_OnStartEvent_Sink(void* object, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	ILibChain_Link* link = (ILibChain_Link*)object;
	ILibChain_StartEvent e;

	UNREFERENCED_PARAMETER(readset);
	UNREFERENCED_PARAMETER(writeset);
	UNREFERENCED_PARAMETER(errorset);
	UNREFERENCED_PARAMETER(blocktime);

	e = (ILibChain_StartEvent)((void**)link->ExtraMemoryPtr)[0];
	if (e != NULL)
	{
		e(link->ParentChain, ((void**)link->ExtraMemoryPtr)[1]);
	}

	// Adding ourselves to be deleted, since we don't need to be called again.
	// Don't need to lock anything, because we're on the Microstack Thread
	ILibLinkedList_AddTail(((ILibBaseChain*)link->ParentChain)->LinksPendingDelete, object); 
}
//! Add an event handler to the Microstack Chain to be dispatched on startup of the chain
/*!
	\param chain Microstack Chain to add the event handler to
	\param sink Event Handler to be dispatched on startup
	\param user Custom user state data to dispatch
*/
void ILibChain_OnStartEvent_AddHandler(void *chain, ILibChain_StartEvent sink, void *user)
{
	ILibChain_Link *link = ILibChain_Link_Allocate(sizeof(ILibChain_Link), 2 * sizeof(void*));
	link->ParentChain = chain;
	((void**)link->ExtraMemoryPtr)[0] = sink;
	((void**)link->ExtraMemoryPtr)[1] = user;
	link->PreSelectHandler = ILibChain_OnStartEvent_Sink;

	if (ILibIsChainRunning(chain) == 0)
	{
		ILibAddToChain(chain, link);
	}
	else
	{
		ILibChain_SafeAdd(chain, link);
	}
}

#if defined(_REMOTELOGGING) && defined(_REMOTELOGGINGSERVER)
unsigned char ILibDefaultLogger_HTML[4783] =
{
	0x1F,0x8B,0x08,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0xED,0x3C,0xEB,0x72,0xDB,0x3A,0x73,0xFF,0x3B,0xD3,0x77,0x40,0x98,0x39,0xC7,0x52,0xA3,0xFB,0xCD,0xB6,0x64,0x39,
	0x63,0x4B,0x4A,0xE2,0xF9,0x1C,0x27,0xB1,0x9C,0xE4,0x9C,0x39,0x73,0x9A,0xA1,0x44,0x48,0xE2,0x17,0x8A,0x54,0x49,0xCA,0xB6,0x8E,0xEB,0x27,0xEB,0x8F,0x3E,0x52,0x5F,
	0xA1,0xBB,0x00,0x48,0x82,0x24,0x48,0x5A,0x4E,0x7A,0x99,0x69,0xE5,0x4C,0x4C,0x82,0x8B,0xBD,0x61,0x77,0xB1,0xBB,0x84,0xFC,0x1F,0xFF,0xF6,0xEF,0x27,0x2F,0xC6,0x1F,
	0x46,0x37,0xBF,0x7F,0x9C,0x90,0x95,0xBF,0xB6,0xC8,0xC7,0xCF,0xE7,0x97,0x17,0x23,0xA2,0x55,0xEB,0xF5,0xAF,0xED,0x51,0xBD,0x3E,0xBE,0x19,0x93,0xDF,0xDE,0xDD,0xBC,
	0xBF,0x24,0xCD,0x5A,0x83,0xDC,0xB8,0xBA,0xED,0x99,0xBE,0xE9,0xD8,0xBA,0x55,0xAF,0x4F,0xAE,0x34,0xA2,0xAD,0x7C,0x7F,0xD3,0xAF,0xD7,0xEF,0xEE,0xEE,0x6A,0x77,0xED,
	0x9A,0xE3,0x2E,0xEB,0x37,0xD7,0xF5,0x7B,0xC4,0xD5,0xC4,0xC9,0xE2,0xB2,0xEA,0x4B,0x33,0x6B,0x86,0x6F,0x68,0xA7,0xE4,0x04,0x9F,0xE0,0x2F,0xAA,0x1B,0xF0,0x6B,0x4D,
	0x7D,0x9D,0xCC,0x1D,0xDB,0xA7,0xB6,0x3F,0xD4,0x7C,0x7A,0xEF,0xD7,0x11,0x60,0x40,0xE6,0x2B,0xDD,0xF5,0xA8,0x3F,0xDC,0xFA,0x8B,0xEA,0x91,0x46,0x90,0x60,0x95,0xFE,
	0xCB,0xD6,0xBC,0x1D,0x6A,0x23,0x0E,0x5E,0xBD,0xD9,0x6D,0xA8,0x16,0xE0,0x90,0x01,0x7E,0xAB,0x7E,0x3E,0xAB,0x8E,0x9C,0xF5,0x46,0xF7,0xCD,0x99,0x45,0xB5,0x88,0xC0,
	0xC5,0x64,0x38,0x19,0xBF,0x9D,0x84,0xB3,0x6C,0x7D,0x4D,0x87,0xDA,0xC2,0x71,0xD7,0xBA,0x5F,0x35,0xA8,0x4F,0xE7,0xC8,0xAC,0x26,0x73,0x64,0xD1,0xCD,0xCA,0xB1,0xE9,
	0xD0,0x76,0x70,0x96,0x6F,0xFA,0x16,0x3D,0xFD,0x4A,0x67,0xD7,0x37,0x23,0x32,0xA6,0xB3,0xED,0xF2,0xA4,0xCE,0xC7,0xC8,0x89,0xE7,0xEF,0xF0,0xF7,0xCC,0x31,0x76,0x0F,
	0x6B,0xDD,0x5D,0x9A,0x76,0xBF,0x31,0xD8,0xE8,0x86,0x61,0xDA,0x4B,0xB8,0x9A,0x39,0xAE,0x41,0x5D,0xB8,0x98,0x3B,0x96,0xE3,0xF6,0x67,0x96,0x3E,0xFF,0x3E,0x58,0x00,
	0xA1,0xAA,0x67,0xFE,0x45,0xFB,0xCD,0xF6,0xE6,0x9E,0xDF,0x2E,0xF4,0xB5,0x69,0xED,0xFA,0xDA,0x8D,0x0B,0xF8,0xE7,0x2B,0xEA,0x93,0xF7,0x53,0xAD,0x42,0xCE,0x5C,0x53,
	0xB7,0x2A,0xE4,0x1D,0xB5,0x6E,0xA9,0x6F,0xCE,0xF5,0x0A,0xF1,0x40,0xBB,0x55,0x8F,0xBA,0xE6,0x62,0x30,0x03,0x64,0x4B,0xD7,0xD9,0xDA,0x46,0x95,0xA3,0x7F,0x69,0xB4,
	0x8D,0x63,0xA3,0x37,0x78,0x7C,0x89,0xB2,0xE8,0xA6,0x4D,0xDD,0x87,0x34,0xD0,0x62,0xB1,0x18,0xDC,0x99,0x86,0xBF,0xEA,0x37,0x1B,0x8D,0x06,0x30,0x10,0xF0,0x4D,0xF4,
	0xAD,0xEF,0x08,0x96,0xAB,0xBE,0xB3,0x09,0xF9,0xAF,0xBA,0xE6,0x72,0xE5,0xF7,0x9B,0x9B,0x7B,0xE2,0x39,0x96,0x69,0x90,0x97,0xB3,0x43,0xFC,0x09,0x1E,0xCF,0x1C,0xDF,
	0x77,0xD6,0x11,0xB8,0x45,0x17,0x2A,0xE8,0x48,0x2D,0x8F,0x2F,0xD7,0xBA,0xE7,0xA3,0x39,0x3C,0x70,0x4E,0x18,0x65,0x85,0xFE,0x9C,0x5B,0xEA,0x2E,0x2C,0xE7,0x8E,0x03,
	0xA0,0xA9,0x54,0x75,0xCB,0x5C,0xDA,0x7D,0xC6,0x91,0x42,0x03,0x8D,0x76,0x4F,0x08,0x77,0xDC,0x43,0xD9,0x22,0x88,0xBE,0xBB,0x9C,0x95,0x3A,0xDD,0xCA,0x51,0xAF,0xD2,
	0x6C,0x1F,0x96,0x07,0x44,0x7A,0x54,0x5D,0x3B,0x7F,0x55,0x2D,0xD0,0x97,0xEE,0x56,0x97,0xAE,0x6E,0x98,0x60,0x07,0x25,0x94,0xA2,0x42,0x60,0x96,0x1E,0x4D,0xAB,0x34,
	0xCB,0xA4,0xF1,0x8B,0x18,0x6D,0x54,0xBA,0xCD,0x4A,0xB3,0xD1,0xC2,0xC1,0xD6,0xF1,0x2F,0x09,0x94,0x77,0x74,0xF6,0xDD,0xF4,0x25,0x74,0x0C,0x7D,0x85,0x20,0x5A,0x02,
	0xDA,0x05,0x24,0x28,0x03,0xBF,0x64,0xDC,0x57,0x3D,0xB8,0x2E,0x01,0xFA,0x14,0xCD,0x72,0x0C,0x02,0x68,0x55,0x92,0x0C,0x94,0xD5,0xD4,0xF7,0x90,0xE9,0x09,0x22,0x39,
	0x3F,0x19,0xDF,0xDA,0xFB,0xB9,0x08,0x93,0xC8,0x7C,0x87,0xEB,0x78,0x3F,0x84,0x0B,0xD3,0xF2,0xC1,0x67,0x37,0xAE,0xB3,0x34,0x8D,0xFE,0xF8,0xB7,0x8B,0xB5,0xBE,0xA4,
	0x2C,0x1E,0x62,0xCC,0xA8,0xBD,0x37,0xE7,0xAE,0xE3,0x39,0x0B,0xBF,0x16,0xD2,0x21,0x9E,0xAF,0xBB,0xFE,0x08,0x57,0xC8,0xF3,0xDD,0xE1,0xC1,0xCB,0x96,0xD1,0xED,0x1D,
	0x1D,0x1F,0x54,0x08,0xB5,0x0D,0x69,0xB8,0xD1,0x68,0xB7,0x7B,0xBD,0x83,0xCA,0x5B,0x31,0x11,0x83,0xD9,0xB0,0x49,0x80,0x26,0x7A,0xAC,0xB5,0x5D,0xDB,0xDF,0xAC,0x87,
	0x8D,0xC3,0xA3,0x67,0xDF,0xA5,0x16,0xC4,0xB2,0x5B,0x3A,0x00,0x07,0xD0,0xFD,0x3E,0x6A,0x26,0x30,0xED,0xB6,0xEC,0xB6,0x91,0xBB,0x90,0x66,0x37,0x66,0xF2,0xB2,0xC7,
	0x3F,0xBE,0x5C,0x38,0x0E,0xC8,0xF5,0x30,0xB7,0x40,0x47,0x7D,0x70,0xD9,0x55,0xC2,0xBB,0x64,0xB7,0x91,0x3C,0x6D,0x0E,0x8C,0x52,0x57,0x81,0xB5,0xD9,0x6C,0x1F,0xF7,
	0x5A,0x01,0x75,0x16,0x30,0x90,0x7C,0x70,0x2F,0x82,0x02,0x0E,0x05,0xB4,0x89,0xFE,0x20,0x71,0xC4,0x68,0x18,0x74,0xEE,0xB8,0x3A,0x93,0x17,0x50,0x53,0x17,0x97,0x50,
	0x82,0xEF,0xAF,0x90,0xC7,0xBC,0x59,0xB6,0x83,0x13,0x42,0xC4,0xE8,0xFF,0x39,0x88,0x6B,0x2C,0x5E,0xB7,0x1F,0xD2,0xF2,0xF1,0xF9,0x77,0x2B,0xD3,0xA7,0x0A,0x59,0x8F,
	0x1A,0xF8,0xC3,0x43,0xF5,0x1D,0x65,0xD1,0x70,0xE6,0x58,0x46,0x80,0xB0,0xF7,0x20,0x6B,0xA1,0x95,0xD6,0x42,0x4B,0xB9,0x2E,0xA3,0x06,0xFE,0x00,0x12,0x30,0x39,0x7A,
	0xE3,0x38,0xD6,0x4C,0x77,0x65,0xD6,0xD8,0x92,0xA7,0xA7,0x4D,0x3A,0x93,0xE3,0xC9,0xA1,0x14,0xA8,0xAB,0x8C,0x89,0x3E,0x8B,0xB6,0xF2,0x30,0x5F,0x51,0xA4,0xFD,0x58,
	0xBB,0xF7,0xF8,0x3E,0xA7,0xD8,0x10,0x26,0x0D,0xFC,0x09,0xCD,0x08,0x56,0x8C,0xB4,0x1A,0xF8,0x5F,0x70,0x15,0x6E,0x03,0x60,0xB6,0x5B,0x0F,0xB6,0x0D,0x86,0x71,0x61,
	0xDE,0x53,0x03,0x35,0xF2,0x20,0xEF,0x60,0x73,0x67,0xEB,0x9A,0x14,0xE2,0x9C,0x26,0xAE,0x88,0x4D,0xEF,0x60,0x27,0x5B,0x3B,0xB6,0xE3,0x6D,0xF4,0x39,0x2E,0x82,0x69,
	0x2F,0x1C,0xD0,0xBA,0xBB,0x8B,0xA9,0xAD,0xC9,0xB0,0x5E,0x9E,0x9D,0x4F,0x2E,0x6F,0x7E,0xBB,0x79,0x20,0xF2,0x96,0xF9,0x58,0x9B,0xDE,0x7C,0xBE,0xFA,0x76,0x31,0x9A,
	0x7C,0x7B,0xFF,0x61,0xFC,0xF9,0x72,0x12,0x3D,0xDE,0x22,0xC6,0xF1,0xCD,0xE5,0x34,0xF1,0xC4,0xD0,0x5D,0x90,0x93,0x52,0x1B,0x27,0x8F,0x6E,0x3E,0x2A,0x1E,0xDF,0x9A,
	0x8E,0x05,0x3B,0xED,0x63,0xED,0x6C,0xFA,0xFB,0xD5,0x68,0xFA,0x61,0xF4,0xB7,0xC9,0x4D,0x02,0x0C,0x5C,0x7D,0x37,0x73,0x9D,0x3B,0x1B,0xA0,0xBE,0x4E,0xCE,0x93,0xD4,
	0xC5,0x93,0xB7,0x93,0xAB,0xC9,0xF5,0xC5,0x28,0x49,0x82,0xD2,0xCD,0xC6,0xB4,0xBF,0x03,0xC0,0xE7,0xAB,0xBF,0x5D,0x7D,0xF8,0x7A,0x95,0x44,0x4E,0xD1,0x82,0x2E,0x3F,
	0xBC,0xFD,0x76,0x39,0xF9,0x32,0xB9,0xFC,0xD6,0x7C,0x20,0xF2,0x6D,0x2B,0x7E,0xDB,0x8E,0xDF,0x76,0xE2,0xB7,0xDD,0xF8,0x6D,0x2F,0xB8,0xFD,0x72,0x31,0xBD,0x38,0x47,
	0x92,0x86,0xE9,0x6D,0x2C,0x7D,0xD7,0x37,0x6D,0xE1,0x0A,0xF8,0xF4,0xDD,0xC5,0x78,0x3C,0xB9,0x8A,0x1E,0x72,0x6F,0xAA,0xB1,0x54,0xE8,0xDE,0x7F,0x4F,0xED,0xAD,0x64,
	0x2F,0xFD,0x97,0x6F,0x8E,0xF1,0x07,0xCC,0xE1,0xBE,0xEA,0xAD,0x74,0x03,0x22,0x47,0x83,0x40,0xD4,0x01,0x0B,0xE3,0x11,0x96,0x34,0x2A,0xE2,0x5F,0xAD,0x0D,0x61,0x4D,
	0x64,0x3F,0x52,0x26,0x30,0x9F,0xCF,0x07,0x71,0x62,0x61,0xB8,0xD3,0x67,0x00,0xB3,0x05,0xEF,0xE3,0x99,0x07,0xCB,0x21,0xE0,0x97,0xE9,0xF9,0xC2,0xBC,0x19,0x78,0x2A,
	0xE8,0x61,0x7C,0x59,0x9B,0xB6,0xB0,0xF5,0x2E,0x0F,0x8C,0xF7,0x81,0xE9,0xB3,0xFB,0xBF,0xAA,0x26,0x44,0x80,0x7B,0x78,0xD8,0x90,0x73,0xAF,0x23,0x60,0x0B,0x44,0x5D,
	0xA3,0xA4,0x41,0xF8,0xE8,0x74,0x3A,0x83,0xB8,0xA2,0xAA,0x33,0xCB,0x01,0x03,0x0C,0x0C,0x95,0xB1,0x75,0x24,0x39,0x38,0x4F,0x8D,0xE4,0x91,0x9C,0x40,0xA8,0x0C,0x5E,
	0x22,0xEE,0x76,0x7F,0x19,0xCC,0xB7,0xAE,0xC7,0xEC,0x66,0xA1,0x6F,0x2D,0x3F,0x0A,0xCF,0x2B,0xD3,0x30,0xC0,0x8E,0xD3,0x3B,0x43,0xC0,0x7E,0x22,0x4C,0x1E,0xE3,0x8F,
	0x9C,0xFA,0x30,0xB9,0x1E,0x6B,0x6B,0x58,0xCF,0x44,0x88,0xF9,0xA1,0xE8,0x27,0x94,0xCC,0x92,0xC8,0x15,0x1F,0x6F,0xF7,0xE0,0x5A,0xC8,0xB1,0x71,0x4C,0x86,0x3C,0xAE,
	0x73,0x79,0x3B,0x93,0x55,0xC6,0x97,0x43,0xB0,0x28,0x04,0x52,0xE4,0x77,0xE3,0xA3,0xEE,0xF8,0x4D,0x24,0x4B,0x6B,0x6F,0x61,0x1A,0xB3,0x2E,0x3D,0x9E,0xFF,0x77,0x0A,
	0xD3,0x2A,0x94,0xE6,0xA4,0x2E,0xEA,0x88,0x93,0xBA,0xA8,0x91,0xB0,0xA0,0x20,0x8E,0x0D,0xD8,0x8D,0xA1,0x66,0x2E,0x48,0xC9,0x87,0x5C,0xC1,0x59,0x94,0x58,0xA6,0xB1,
	0xDD,0x94,0xC9,0x8B,0xE1,0x90,0x1C,0xE0,0xCE,0xB6,0x00,0x23,0x35,0x0E,0xCA,0x44,0x3C,0x29,0x95,0x07,0x58,0xB3,0x18,0xE6,0x2D,0x31,0x61,0x6A,0x58,0x08,0xC8,0x83,
	0x41,0xF2,0xAD,0x11,0x46,0x76,0xA8,0x09,0x79,0x7B,0x28,0x6F,0x58,0x1A,0xFC,0x92,0x34,0xC0,0x00,0x85,0x98,0x24,0x89,0x2E,0xCF,0x17,0xA2,0xCD,0x8F,0xF0,0x27,0xEE,
	0x37,0x6C,0x2F,0x49,0xA8,0x49,0x3B,0x85,0x1A,0xCA,0x75,0xEC,0xE5,0xE9,0x09,0xAA,0x36,0x44,0x1E,0xAA,0xB9,0xD3,0x4B,0xD4,0x48,0xB9,0x35,0x91,0x76,0xCA,0xF3,0x33,
	0x1F,0x74,0x4D,0xAE,0xA9,0x6E,0xF9,0xE6,0x9A,0x92,0x4B,0x67,0x09,0x51,0x03,0xCA,0x36,0xC4,0x73,0x8A,0xDA,0xE6,0x14,0xEB,0x20,0xCE,0x8F,0x0A,0xD5,0x4D,0xC8,0xD4,
	0xEC,0x3C,0x41,0x28,0x04,0xDA,0x47,0xA8,0x13,0xD8,0x3A,0x6D,0xB6,0x76,0xAC,0xF0,0x5C,0x81,0x80,0x1A,0xCA,0x01,0xA3,0xA7,0x59,0x52,0x49,0xC2,0xB1,0x89,0xCE,0x06,
	0x6C,0x91,0xD5,0xB3,0x3A,0x14,0xC8,0x01,0x4F,0xD2,0x72,0x0B,0x81,0x5B,0x98,0x30,0x40,0x39,0x4C,0x2D,0x4B,0xC8,0x35,0xD4,0x1A,0xFC,0x1E,0x37,0xF0,0xF0,0xDE,0xD2,
	0x3D,0x0F,0x64,0x8A,0x52,0x17,0x86,0xDB,0xC5,0xFF,0x0C,0x46,0x71,0x0A,0x75,0xF4,0xDC,0xA7,0xC6,0x7B,0xC7,0xD8,0x5A,0xD4,0x0B,0xA7,0x08,0xB7,0xD0,0x4E,0x03,0x00,
	0x22,0x20,0xA0,0xAC,0x36,0xA2,0xE9,0x5F,0xA8,0x3B,0xC3,0x88,0xB7,0xE3,0x60,0xE9,0xE9,0x21,0x40,0x9F,0x34,0xE3,0x53,0x47,0x98,0xE3,0x66,0x4D,0x63,0x0F,0xD1,0x24,
	0x12,0xF4,0x3E,0xEA,0x5B,0x8F,0x66,0x4D,0x62,0x0F,0xE3,0xF0,0x6F,0x40,0x72,0x61,0x58,0x69,0x78,0x7C,0x28,0xC0,0xEB,0x4C,0x27,0x75,0xA6,0xF5,0xF4,0xB2,0x6C,0xA0,
	0xB8,0xF8,0x26,0x5A,0x0F,0x71,0xC7,0xE5,0xF5,0x80,0x34,0x76,0xE0,0xD2,0x85,0x4B,0xBD,0xD5,0xF9,0x16,0x36,0x14,0x7B,0x6C,0xDE,0x1E,0xC4,0xAD,0x96,0xD7,0xC4,0x7C,
	0x87,0xE4,0x89,0x28,0xFA,0x9B,0xBC,0xDB,0x82,0xCD,0x98,0xF6,0x66,0x0B,0x45,0x27,0x56,0x1E,0xDA,0x8C,0x21,0xD2,0xC8,0xAD,0x0E,0x59,0xD4,0x50,0xBB,0xE6,0xE8,0x35,
	0x88,0x3B,0x73,0xCB,0x9C,0x7F,0x1F,0x6A,0x82,0x20,0x8B,0x29,0x12,0xE3,0xA7,0x27,0xAB,0x66,0x64,0x8C,0xF8,0x68,0xD5,0x0C,0x9F,0x6F,0x44,0x88,0x31,0x6D,0x59,0x1C,
	0x70,0x48,0x7F,0xEB,0xC1,0x5D,0x32,0xE6,0x74,0x1A,0x69,0x26,0xF9,0x34,0x16,0xD3,0x41,0x11,0x2C,0xA8,0x87,0xD3,0x82,0x04,0x20,0xE9,0x75,0xAC,0x6C,0x89,0x1C,0x0C,
	0x45,0x47,0x44,0xA1,0xDF,0x70,0x06,0x70,0xB3,0x08,0x1D,0x87,0x24,0x3C,0x45,0xED,0x37,0xB0,0xC4,0x5F,0x21,0x7F,0x70,0xEE,0xB4,0x6C,0x97,0xE9,0x31,0x21,0x82,0x70,
	0x59,0xDD,0xF5,0x3D,0x88,0x41,0x16,0x5B,0xBB,0x04,0x36,0x5E,0xD8,0x44,0x5E,0x98,0xEB,0x65,0xCD,0x86,0x82,0x66,0xC2,0xCB,0x0E,0xEE,0x39,0xCA,0xD0,0x16,0x92,0xF5,
	0x43,0x62,0xFD,0x43,0x13,0x4E,0x83,0x73,0xFB,0xE1,0xA1,0xCE,0xC2,0x6B,0x28,0x70,0x77,0xDA,0xE9,0x85,0xCD,0x7B,0x65,0x90,0x79,0x10,0xDD,0x27,0x27,0x7A,0x30,0x35,
	0x09,0x49,0x56,0x60,0x2F,0x43,0xD6,0x27,0xF4,0xFA,0xF5,0x3A,0x66,0xF9,0xB0,0x05,0x7A,0x2B,0x5C,0x41,0x57,0xB7,0x20,0xAB,0x5C,0x6B,0xA7,0xAA,0xD1,0x93,0xBA,0x7E,
	0x9A,0xEB,0x2C,0x09,0x25,0x4A,0xE9,0x69,0xE8,0x79,0xF2,0x98,0x00,0x0D,0x9E,0xB0,0x14,0x49,0xB2,0x6A,0x1F,0x9C,0xD6,0xA2,0xAC,0x91,0xC7,0x03,0x4F,0xE9,0x60,0xEA,
	0x6F,0xED,0x8B,0x39,0x1D,0xAD,0xE8,0xFC,0x3B,0x64,0xB7,0x58,0xB9,0xDF,0x02,0x7F,0xE5,0xD0,0x61,0x58,0x3C,0x8B,0x03,0x69,0xC2,0x8B,0xE6,0xE1,0xBD,0xA1,0xFB,0x7A,
	0x75,0xBE,0x06,0xD0,0xC6,0x7D,0xA3,0xA5,0x89,0x96,0x61,0x9F,0x60,0xED,0x52,0x87,0xDA,0xE5,0x64,0xE6,0xC6,0xF6,0x9C,0x3D,0x18,0x1C,0xFB,0x96,0x97,0xCF,0x9D,0x0C,
	0x51,0xC0,0x5A,0x27,0x62,0x0D,0x0B,0xA7,0x1F,0x60,0x6B,0x3A,0xF7,0x37,0x05,0x4A,0x93,0x20,0x0A,0xD8,0x3A,0x92,0x34,0x06,0x05,0xDB,0x0F,0xB0,0x75,0xE6,0xED,0xEC,
	0xF9,0x14,0xF2,0x76,0xEA,0xE7,0x73,0xA7,0x00,0xCC,0x67,0xB2,0xD3,0x90,0x33,0x8C,0x3E,0x91,0x10,0xFC,0x00,0xBF,0x20,0xF6,0x94,0xBA,0x10,0x3E,0xF2,0xB9,0x4D,0x81,
	0xE5,0xF3,0x7A,0x94,0xE0,0x35,0x9C,0xFE,0x03,0x9C,0xBE,0xA5,0x90,0x4B,0x9A,0xF3,0x7C,0x3E,0x13,0x40,0xF9,0x5C,0x42,0x4C,0x8B,0xB3,0x29,0x66,0xCB,0x4C,0x26,0xFC,
	0xFF,0x36,0xD8,0xF8,0x33,0x23,0x40,0x10,0xA2,0x54,0xFB,0x4A,0x96,0xB4,0x10,0xE8,0x2F,0x41,0x18,0xAB,0xD4,0x8C,0x84,0x8A,0xA5,0x18,0x4F,0xD4,0x5A,0x88,0xA7,0xA5,
	0xC4,0xD3,0xDA,0x1B,0x4F,0x5B,0x89,0xA7,0xBD,0x37,0x9E,0x8E,0x12,0x4F,0x67,0x6F,0x3C,0x5D,0x25,0x9E,0x6E,0xCE,0x82,0x2D,0xA2,0x54,0xE9,0xA7,0x2E,0x19,0x37,0x50,
	0x29,0x11,0x2B,0x1D,0x48,0x37,0x13,0x1B,0x77,0x11,0x43,0x6D,0xA3,0x69,0xB8,0x94,0x99,0x9E,0x8A,0x07,0x4F,0x55,0x10,0xA4,0x4A,0xD4,0x47,0xBC,0x6F,0x2C,0x7D,0xE9,
	0x95,0x80,0xDE,0x35,0x8E,0x10,0x76,0xFB,0x74,0x24,0xBA,0x01,0x7C,0x21,0x1A,0x8E,0x41,0x37,0x30,0x57,0x25,0x2C,0xA1,0x7C,0x22,0x0E,0xD6,0xE2,0x95,0x90,0x84,0x19,
	0x6F,0x0A,0x4B,0xF0,0x0B,0xCB,0x4D,0x56,0x7D,0xE2,0x8B,0xBA,0x13,0xC8,0x5E,0xCC,0x4D,0x90,0x24,0xB2,0xF7,0x73,0x7F,0xD7,0x6F,0x75,0x3E,0xAA,0x9D,0xFE,0xE3,0x3F,
	0x10,0xF1,0xB9,0x05,0xAC,0x1F,0xCF,0x3E,0x4F,0x27,0x63,0x32,0x24,0x0B,0xDD,0xF2,0xE8,0x20,0xFE,0xF0,0xCE,0x63,0x91,0x31,0x31,0x0A,0xAB,0x6E,0xF3,0xBC,0x5F,0x3D,
	0x6B,0xF4,0x1E,0xF1,0x3D,0x10,0xED,0xEA,0xC3,0xD5,0x44,0xEB,0x13,0xD8,0x1A,0x1A,0x15,0xA2,0x5D,0x7E,0x78,0xFB,0x76,0x72,0xCD,0xEF,0xC1,0x45,0xB5,0xA0,0x23,0xC8,
	0x47,0xC0,0xD9,0x34,0xDC,0xCC,0xF8,0x5D,0x07,0x9F,0xC3,0x1E,0xC2,0xEF,0x8E,0xE0,0x4E,0x6A,0xF1,0xB1,0xC1,0x0E,0xA2,0xFC,0x3A,0x39,0x9F,0x4E,0xAE,0xBF,0x08,0xAC,
	0x47,0x38,0x24,0x5A,0x79,0x6C,0x00,0x42,0x13,0x79,0x4C,0x30,0xC7,0xB9,0xF8,0xF6,0xE6,0xF2,0xEC,0xED,0x94,0x73,0x39,0xB9,0x3A,0x3B,0xBF,0x9C,0x04,0x13,0x00,0xC5,
	0xF5,0x64,0x3A,0xB9,0xF9,0xF6,0xE6,0x42,0x0C,0xB6,0xC4,0xE0,0xD9,0x58,0x10,0x96,0x60,0x10,0x8B,0xA0,0x0D,0xA4,0xE2,0x94,0x42,0xDF,0x02,0x32,0x4D,0xE0,0x22,0x7A,
	0xBA,0xD8,0xDA,0xAC,0x97,0x4B,0x3E,0x95,0xEE,0xCB,0xC0,0x82,0x4B,0xFD,0xAD,0x6B,0x13,0xC3,0x99,0x6F,0xA1,0x00,0xF1,0x6B,0x4B,0xEA,0x4F,0x2C,0x8A,0x97,0xE7,0xBB,
	0x0B,0x03,0x60,0x06,0xE4,0x91,0xD4,0xEB,0x44,0xFB,0xA4,0xA9,0x90,0x4C,0x63,0x58,0x10,0x27,0xEF,0x6A,0xE3,0xAC,0xC4,0x87,0x23,0xE1,0xAE,0xAA,0x42,0x35,0x29,0xDD,
	0x57,0xC8,0x0E,0xB1,0x31,0x34,0xE0,0xCB,0xCC,0x83,0x40,0x82,0x17,0xBB,0x04,0x3A,0x81,0x8A,0x32,0x1F,0x53,0xE1,0xFA,0x22,0x70,0x15,0x7F,0x04,0xAA,0x5B,0xD3,0x33,
	0x63,0xB8,0x1E,0xA2,0x4B,0xFC,0x60,0x1B,0x05,0x94,0x39,0x24,0x61,0xDB,0xA4,0x9C,0x04,0xC1,0x8F,0xD0,0x43,0x89,0xE9,0xA5,0x26,0xC2,0x11,0x4E,0x3B,0xC0,0x90,0x74,
	0x40,0x5E,0x73,0xA3,0x25,0x7D,0xE2,0xBB,0x5B,0x5A,0x1E,0xC4,0x31,0x3C,0xC6,0x6F,0x29,0x40,0xC6,0x47,0x14,0x14,0x13,0x94,0x90,0xCB,0xD7,0xE4,0xE0,0x00,0x28,0x70,
	0x92,0x39,0x34,0x1E,0x89,0x4A,0x75,0x67,0xF1,0x65,0x30,0xC1,0xDF,0x5C,0xF6,0x6A,0xFF,0xD5,0x90,0x24,0xD6,0x41,0xE8,0x4E,0xDF,0x6C,0xA8,0x6D,0xA8,0x70,0xBD,0xCB,
	0xC2,0x95,0x42,0x15,0xE0,0xC2,0x20,0xA2,0xC2,0x34,0x2A,0xB2,0x56,0xEF,0x7C,0x37,0xC2,0x90,0x76,0xA5,0xAF,0x29,0xB7,0x5A,0x25,0x1A,0x61,0x18,0x99,0xEB,0x8C,0xCE,
	0x43,0x05,0x46,0xE0,0x92,0xD1,0x4D,0x68,0x10,0x2A,0x1A,0x52,0x42,0x38,0x13,0x00,0x1A,0x64,0x00,0xBF,0x4F,0xC2,0x39,0x35,0x8B,0xDA,0x4B,0x7F,0x05,0x83,0xAF,0x5E,
	0x95,0x0B,0x17,0x2F,0x98,0xF5,0x87,0xF9,0x27,0x77,0x1B,0x69,0x21,0x77,0x39,0x0B,0xA7,0x74,0xE8,0x0B,0xDB,0x6F,0xB7,0x6E,0x9C,0xA9,0xEF,0x96,0x6E,0x25,0x5D,0xC1,
	0x3D,0xEC,0x53,0xB5,0x85,0xEB,0xAC,0x47,0x2B,0xDD,0x1D,0x39,0x06,0x2D,0x95,0x6E,0xC9,0xE9,0x29,0x69,0x75,0xCA,0xE4,0x57,0x88,0x1F,0x6F,0xDE,0x54,0x08,0x1F,0x69,
	0xF6,0x92,0x23,0x47,0xD1,0xC0,0xAD,0xB8,0xCA,0xD0,0x2D,0x90,0x6F,0xF6,0xF6,0x21,0xBF,0x17,0xEA,0xA3,0xA7,0x61,0x2E,0x40,0x84,0x3B,0xE2,0xF9,0xCE,0xA7,0x25,0x4C,
	0x22,0x2B,0x64,0xE3,0xBB,0x12,0x3E,0x36,0x58,0x9B,0x0B,0x54,0x67,0x7E,0x09,0x1F,0xE7,0x20,0x9A,0xAE,0x1C,0xD7,0x7F,0x3A,0x26,0x72,0x72,0x82,0x22,0xBF,0x22,0x8A,
	0x87,0x30,0xDA,0xCC,0xA1,0x04,0xF2,0xEF,0x41,0xE7,0x9F,0x60,0x19,0x0F,0x0F,0x0F,0x5B,0xB8,0x98,0xAF,0x94,0x30,0x8C,0x1C,0xC0,0xF5,0xBA,0xDD,0x76,0x1E,0x50,0x0B,
	0x81,0x5A,0xDD,0x5E,0x36,0xD7,0xED,0x1C,0xAE,0x2F,0x1D,0xC8,0xA9,0x94,0x6C,0x67,0xF1,0x7D,0xD8,0x6A,0x74,0x0F,0xBB,0xC7,0x9D,0x46,0xFB,0xF0,0xB8,0x75,0x78,0x9C,
	0xCB,0x1B,0x13,0xA0,0x75,0xD4,0xEC,0x1C,0x76,0x8E,0x0F,0x7B,0x87,0xCD,0x46,0xAF,0x5B,0x28,0x4A,0xB3,0x71,0x7C,0xDC,0x6D,0x36,0x7B,0x2D,0x50,0x4F,0x1E,0x70,0x1B,
	0x81,0x3B,0xAD,0xE3,0xCE,0x71,0xEF,0xB0,0x75,0x9C,0x23,0x7E,0xE7,0x89,0xDA,0xEE,0x3E,0x45,0xDB,0xBD,0x42,0x6D,0x1F,0x72,0x6D,0x2B,0xF4,0x3D,0x65,0x87,0x05,0xD6,
	0x6B,0xDD,0x36,0x4A,0x50,0x1B,0x55,0x18,0x82,0xFC,0x28,0xE7,0x01,0xCA,0xA1,0xEC,0xB5,0x30,0x2F,0x20,0x3D,0x48,0x83,0xCF,0x4C,0x1B,0xC0,0x6D,0x7A,0xC7,0xBC,0xF1,
	0xCC,0x75,0xF5,0x5D,0x09,0x50,0x88,0x78,0x97,0x8C,0x91,0x79,0x11,0x93,0x07,0xCC,0x68,0x2E,0x8F,0x95,0x60,0x1E,0x40,0x02,0x02,0x21,0x80,0xE0,0x33,0x49,0x78,0x33,
	0xC8,0x41,0x46,0x8E,0x0D,0xE5,0x1B,0xB6,0xC4,0xD1,0xF3,0x89,0xEF,0xE0,0x14,0xDD,0xDD,0x65,0x12,0x16,0x39,0x64,0xCD,0x03,0xF5,0x94,0x00,0x56,0x66,0x53,0xAD,0xC8,
	0x64,0x1E,0x9E,0xAB,0xC2,0x35,0x6F,0x3D,0xA3,0x50,0x90,0x66,0xC6,0xD2,0x2C,0xFC,0x60,0xCE,0xF0,0xA9,0x94,0x6A,0xFD,0x94,0x6B,0xAC,0x4A,0x60,0x19,0x44,0x88,0xE2,
	0x5F,0x87,0x98,0xBA,0x86,0x6F,0xAA,0x07,0xC9,0x5C,0x40,0xE0,0x8A,0x35,0x6A,0xF2,0x10,0x61,0x3A,0x3B,0x88,0xED,0xF0,0x32,0x47,0x72,0x5F,0x25,0x97,0x1D,0x48,0x83,
	0x33,0xB1,0xA8,0xFA,0x1F,0x79,0xC8,0xA4,0x2C,0x3A,0x4B,0xBC,0x74,0x93,0x22,0x0F,0x61,0x98,0x81,0x67,0xB2,0x98,0x6C,0x26,0xE4,0x61,0x13,0xC9,0x3B,0xC7,0x15,0xC7,
	0x26,0x7B,0x17,0x82,0xF2,0x3C,0xBE,0x22,0x3B,0x8F,0x9C,0xDA,0xD7,0xA4,0x04,0x1D,0x3D,0x4A,0x02,0x13,0x34,0x13,0xA3,0x58,0x94,0xE0,0xF6,0x10,0x66,0xED,0xB8,0xED,
	0x28,0x4C,0x5A,0x15,0x69,0x47,0xA2,0x5E,0xCB,0xB3,0x55,0xC8,0xC3,0xA4,0x56,0x34,0x54,0x10,0x5A,0xD2,0x5F,0xCF,0x0C,0x2C,0x1C,0x4B,0xDA,0x1F,0xA3,0xCB,0xC9,0xD9,
	0xF5,0x64,0xFC,0xA7,0x56,0xEC,0x2A,0xEF,0x4C,0x83,0x7E,0xB6,0x79,0x19,0x6D,0xE4,0x50,0x2F,0x72,0x03,0x4C,0x8F,0x59,0x56,0xCC,0xF2,0x44,0xC8,0xD0,0xB4,0xC4,0x79,
	0x0D,0xE4,0x98,0x55,0xF4,0xE5,0xC1,0x3E,0x5E,0x91,0xC6,0x2B,0x9D,0xF4,0x90,0x70,0x66,0xD8,0xA2,0xDA,0x49,0x14,0xCC,0x46,0xE7,0x43,0x8A,0x91,0xE6,0xFA,0x4C,0x1A,
	0x77,0xFA,0x6C,0x49,0x31,0x89,0x1C,0x1F,0x4A,0x13,0x88,0x8E,0xA5,0x14,0x23,0xCE,0x74,0xA6,0x34,0xDA,0xF8,0x99,0x96,0x0C,0xD4,0x6A,0xAB,0xBA,0x61,0xF6,0xC4,0x5E,
	0x99,0x5D,0x53,0x0F,0xB2,0xFC,0x1C,0xCB,0x0A,0xFB,0x07,0x2F,0xF8,0xD5,0x20,0x6D,0xF6,0xF2,0x8B,0x39,0xC8,0x69,0xC5,0x8C,0xD7,0x50,0x44,0x33,0xE4,0x1A,0xD4,0x49,
	0x1C,0x46,0x2B,0x27,0x7D,0xE2,0x53,0x7C,0x36,0x88,0x1B,0xD4,0x17,0x40,0x31,0x42,0x14,0xBC,0xA8,0x67,0xA8,0x82,0x77,0x78,0x83,0x7C,0xA7,0x4D,0xF7,0x9E,0x4C,0x23,
	0xE8,0x34,0x65,0x7B,0x31,0x28,0x36,0xD9,0x0C,0xAB,0x08,0xC5,0xA7,0x58,0x37,0x0D,0x69,0x79,0xC8,0x8B,0x52,0x6C,0x24,0x09,0xBE,0x6F,0x7C,0xE3,0x5D,0x0B,0x11,0xC4,
	0x44,0x66,0x1E,0x27,0xF9,0x9A,0x35,0x5A,0x08,0xEF,0xBF,0x48,0xD1,0xEC,0x29,0x6A,0x91,0x7B,0xC6,0x4A,0xB5,0x90,0x34,0x36,0xFC,0x44,0x3B,0x32,0x16,0x6E,0x8C,0x1D,
	0xCC,0x65,0x60,0x3F,0xAF,0x41,0x6A,0xB3,0xBF,0x86,0xD2,0xD8,0x17,0x98,0x13,0x84,0xC8,0x23,0x59,0x15,0x01,0x9C,0x8B,0xDE,0x4C,0xA1,0x91,0x55,0xCD,0x99,0x8D,0xA9,
	0x99,0x51,0x28,0xA7,0xA9,0xE3,0xDA,0xCB,0x0D,0xCF,0x8C,0x75,0x0F,0xDB,0xAC,0x21,0x23,0x81,0xFA,0x94,0xC9,0x09,0x7B,0x54,0xDB,0xB8,0xEC,0xF7,0x98,0x9F,0x1C,0x2A,
	0x13,0xD5,0x68,0x29,0x49,0x29,0x9A,0x8E,0xC7,0x8A,0x3F,0xBA,0xCE,0x46,0x5F,0xB2,0xD7,0x7C,0xC1,0xFC,0xC4,0x70,0xA9,0x5C,0xE0,0x12,0x21,0xEB,0xD6,0xAD,0x55,0xE8,
	0x0A,0x72,0x7B,0x0B,0xE0,0x15,0x7E,0x9F,0x3C,0x00,0x00,0x21,0x48,0xEA,0x37,0x6B,0x60,0xBA,0x30,0x2F,0xE5,0x36,0xA0,0xE5,0xF8,0xFB,0x01,0x49,0xCF,0x69,0x73,0xE0,
	0xD9,0xAC,0x22,0xD5,0x35,0x59,0xD7,0x0D,0xD3,0xDC,0x21,0xE9,0x0D,0xC8,0xAB,0x57,0x66,0x71,0x2B,0x80,0xC5,0xCD,0xE8,0x0C,0x1E,0x72,0x68,0x56,0x38,0x0A,0xE0,0x14,
	0x03,0x0D,0x3F,0x53,0xC6,0xC2,0x8C,0x88,0xA6,0xB9,0xDD,0xA3,0xC4,0xF6,0x5C,0xE0,0xF2,0xD8,0x2E,0xAD,0x14,0xE5,0x24,0xFF,0xDB,0x6D,0x48,0x64,0x31,0x73,0x28,0x1B,
	0x2A,0x64,0xED,0x2D,0x2B,0xDC,0x63,0xF3,0x93,0x78,0x83,0x85,0x75,0x58,0x31,0x4D,0x53,0xF0,0x07,0x58,0x8A,0xBB,0x7E,0x9F,0xCE,0x12,0xF9,0x95,0xD4,0x61,0x3F,0x08,
	0x0E,0xA5,0x1E,0x90,0x53,0x66,0x77,0xC8,0x1D,0xFC,0xD2,0x4E,0xEA,0x61,0x2F,0x3D,0xB5,0x94,0xF8,0xE1,0xC5,0x72,0x7A,0x8D,0xD3,0x3C,0xF2,0xBD,0x28,0x2A,0xB0,0x45,
	0xB5,0x34,0x06,0xCD,0x11,0x0B,0x32,0x43,0x72,0xB7,0x82,0x6D,0x83,0x6C,0x70,0x33,0x33,0x54,0x86,0x6C,0xA1,0xE3,0x8D,0x02,0x35,0x10,0xE9,0x50,0xA9,0xA6,0x36,0x7C,
	0xDB,0xF4,0x4D,0xDD,0xFA,0x82,0x3D,0x53,0xD3,0xE2,0x6E,0xC8,0x43,0x17,0x9A,0xAB,0x2A,0x1A,0xBE,0x16,0x68,0xD9,0x89,0x52,0x6C,0x6E,0xA3,0x15,0x93,0xE8,0x14,0x69,
	0x4A,0x03,0xDE,0x9D,0xE9,0xCF,0x57,0x02,0x6B,0xB1,0xF7,0xCC,0x21,0xCC,0xB3,0x4E,0x7E,0x3F,0xFD,0x0C,0x3F,0xF9,0x12,0xAA,0x66,0xCC,0x5C,0xAA,0x7F,0x57,0x3C,0x0B,
	0x28,0x75,0xF6,0xA5,0xD4,0x7A,0x2E,0xA5,0xA3,0x7D,0x29,0xB5,0x9F,0x49,0xA9,0xD9,0xD8,0x97,0x52,0xE7,0x99,0x94,0x5A,0x7B,0x53,0xEA,0xEE,0x4B,0x49,0x1C,0x84,0xFD,
	0x2F,0xB5,0x87,0x94,0x37,0x06,0x56,0xCB,0x7C,0xFC,0x9F,0xF9,0xAB,0x99,0xC6,0x13,0xAD,0x57,0x2E,0xFF,0x33,0xB8,0x8E,0x42,0x55,0xB2,0x42,0x7A,0xCE,0x32,0x04,0x6D,
	0x82,0x42,0x62,0x72,0xD9,0xF4,0x5C,0x42,0x58,0x25,0x15,0x4B,0x25,0x95,0x52,0xCF,0x25,0x24,0x95,0x4C,0x85,0xF4,0x14,0xE5,0xD5,0x73,0xC9,0x86,0xBD,0x88,0x42,0xA2,
	0x52,0xC9,0xF5,0x5C,0x62,0xA2,0xBC,0x2A,0x24,0x95,0x28,0xC3,0x7E,0xAA,0x07,0x45,0x54,0xE2,0xDF,0x4F,0xF8,0x21,0xFF,0xC9,0xDD,0x48,0x71,0xF3,0x14,0x54,0x5F,0xC9,
	0x0E,0xFC,0x4A,0xB1,0x1F,0xC1,0xFE,0x1A,0xEC,0xB7,0xB8,0x83,0xE7,0x6C,0xB7,0xAA,0x3C,0x22,0x3C,0x0E,0x9D,0x74,0xD6,0xF0,0xBD,0x93,0x78,0x5D,0x0E,0xE2,0xAF,0x20,
	0x83,0xB2,0xE8,0x28,0xCA,0xCD,0x53,0x3B,0x66,0xD0,0x5B,0x99,0x78,0x78,0x32,0xCD,0xF4,0x56,0xD8,0xBB,0x14,0xEF,0xB1,0x81,0x5A,0xAD,0x56,0x4B,0xED,0x7E,0xA2,0x6F,
	0x29,0x1A,0xAE,0x58,0xD1,0xB3,0xFB,0x92,0x76,0x87,0x87,0xE3,0x50,0xAA,0x3B,0xA6,0xA2,0x9A,0xE5,0xCC,0x59,0x6A,0x54,0xC3,0x53,0xBD,0x28,0x66,0x3D,0x0B,0x57,0x8D,
	0xB7,0x4A,0xF1,0x6B,0x63,0xB8,0x68,0x3A,0x36,0x70,0x67,0xDB,0xC5,0x82,0xBA,0xC9,0x15,0x0B,0x26,0x38,0xB6,0xB3,0xA1,0xD8,0xF4,0x0D,0xF5,0x02,0xD9,0x9A,0x8F,0x69,
	0x86,0xFC,0x12,0x1E,0xDF,0x63,0x0E,0x42,0x19,0xC3,0x27,0x5C,0x2A,0xDE,0x4E,0xE5,0x29,0x38,0x20,0xF0,0x4A,0xE9,0xE6,0x42,0x44,0x6D,0x6E,0x39,0x1E,0x2D,0x22,0xC7,
	0xDF,0xF9,0x87,0xF4,0x46,0xA1,0x1A,0x21,0xD3,0xF1,0x7C,0x41,0x35,0x93,0xC4,0x9A,0x7A,0x9E,0xBE,0x4C,0x13,0x29,0x8C,0xD0,0xAC,0xFA,0x13,0xCB,0x81,0x65,0x3B,0xBE,
	0xDF,0xA0,0x6E,0x2A,0xA3,0x65,0x56,0x54,0xE3,0x47,0xEF,0xE3,0x44,0xCA,0x69,0x40,0x05,0x99,0x80,0x14,0xEF,0xCD,0xD3,0x1A,0xD8,0xE1,0x12,0x18,0x07,0x2D,0xE2,0x77,
	0x3A,0xD4,0xF0,0xD9,0x58,0xD8,0x2E,0x34,0x94,0xDE,0x55,0x01,0xDA,0x0A,0x69,0xA8,0x98,0x0E,0x65,0x14,0x15,0x6E,0x62,0x4E,0x2B,0x6B,0x0E,0xA6,0x9F,0x7C,0xB7,0xFB,
	0x35,0xD8,0xED,0x30,0x5D,0x56,0x6E,0x7C,0x05,0x72,0xE3,0x27,0x96,0xC0,0xE3,0x5B,0x00,0x6F,0x3B,0xE3,0xCD,0xFE,0x52,0xA7,0x1C,0x64,0xF3,0x19,0xAC,0x3C,0xAA,0x87,
	0xD3,0x2F,0xD1,0x9F,0xC0,0x07,0x14,0x1D,0x73,0x30,0x95,0xF0,0x8D,0x0A,0xE3,0x87,0x51,0x4F,0xB3,0xB5,0x07,0x3F,0x8A,0xA1,0x45,0x0D,0x0F,0xED,0x9C,0x79,0xE7,0xCC,
	0x3B,0xF9,0x2B,0xCD,0x12,0x5A,0xD9,0xB9,0xE5,0xCC,0x4A,0x7F,0x80,0x71,0xB2,0x26,0xC6,0x9F,0x29,0x32,0x8F,0xF9,0x21,0x2C,0x4F,0x82,0xFC,0x77,0x43,0x3C,0x87,0x61,
	0x33,0xF6,0xC8,0x5C,0x78,0x7F,0x28,0x63,0xAF,0x40,0x2B,0x11,0x96,0x35,0x24,0xAA,0x4E,0xD2,0xDE,0x0B,0x24,0x98,0x4C,0xBC,0xCF,0x6D,0x94,0x33,0x30,0x15,0x60,0x0B,
	0x25,0xC1,0x6E,0x55,0x86,0x0C,0xF2,0xE7,0x53,0x49,0x75,0x04,0x4C,0x6E,0x27,0x25,0x8F,0x26,0x65,0x7D,0xB2,0xF6,0x5E,0x15,0x67,0xCD,0x9F,0xC2,0x19,0x8B,0xD7,0x3F,
	0xCE,0x58,0x86,0xB7,0x65,0x0C,0x67,0x6C,0xFE,0x8A,0xCB,0xF8,0x8B,0x38,0x79,0xE7,0xC8,0x34,0xD8,0xFC,0x77,0x0B,0xCA,0xA5,0xC8,0x79,0x67,0x90,0x05,0x9F,0xF5,0x3A,
	0x20,0x0B,0xBE,0xA0,0xD3,0x9F,0x35,0x2D,0xB7,0x7B,0xAF,0x9C,0xF4,0x33,0x9A,0xB7,0x60,0x5D,0xED,0x72,0x51,0x56,0x94,0xCA,0x74,0x4A,0x4F,0x68,0x58,0x67,0xB6,0xD3,
	0x92,0x80,0x4F,0xE8,0x6E,0x16,0x34,0xC0,0xD3,0x7D,0x0A,0x3C,0x5E,0x03,0x5A,0x0B,0xB3,0x36,0x71,0xDC,0xE6,0x8D,0xEB,0xAC,0x3F,0xE2,0x57,0xF2,0x82,0xE6,0x15,0xA4,
	0x04,0xBF,0x91,0x6A,0x04,0x87,0x27,0x1A,0x6B,0xFC,0x6B,0x18,0x97,0xEC,0x6F,0x0A,0x44,0x70,0xBF,0x67,0xC0,0xDD,0x38,0x1B,0x65,0x7B,0x0B,0x39,0xF8,0xF5,0x57,0xCE,
	0xC9,0x0B,0xC8,0x1F,0xB6,0x96,0x15,0xDC,0xD7,0x4C,0xF6,0xE6,0x24,0xF5,0x55,0xA7,0xA7,0x65,0x23,0x42,0x5F,0xF8,0xC2,0x01,0x73,0x64,0x6C,0x4A,0xC7,0x74,0xA8,0xDA,
	0x96,0x70,0x9E,0x0B,0xB4,0x38,0x74,0x8A,0x2E,0x9E,0xA6,0x3A,0xC7,0xAF,0x18,0x82,0x72,0x47,0x16,0xFE,0x91,0x81,0x6B,0x00,0x50,0x26,0x39,0x71,0xEA,0xE2,0xEC,0x12,
	0xFB,0xEB,0x14,0x43,0x46,0x81,0x5F,0x43,0x3E,0xBA,0xB9,0x57,0x15,0x04,0xCA,0xE9,0xBE,0xB3,0x09,0x66,0xE3,0xE5,0x7E,0x93,0xA3,0x73,0x53,0x1A,0xFB,0xD2,0x6D,0x72,
	0xA2,0xE2,0x68,0xDD,0x93,0x17,0x28,0xD9,0x4B,0x7E,0xFE,0x02,0xC5,0xBD,0xA1,0x78,0x89,0x52,0x94,0xFF,0x7F,0x89,0xD4,0x4B,0x24,0x7F,0x69,0xAF,0x78,0x79,0xA4,0x77,
	0xE6,0x3F,0x91,0x87,0xD8,0xCB,0xC2,0x42,0x1E,0x14,0xEF,0x39,0x7F,0x22,0x2F,0xF2,0xF7,0x0B,0x9F,0x6F,0xAE,0xC9,0x60,0x5B,0x6C,0xB0,0x31,0xBA,0xFF,0x87,0x8C,0xF5,
	0x7F,0xF2,0x4D,0x08,0x7E,0x82,0x93,0x75,0xA9,0x7D,0x93,0xB5,0x58,0x14,0x9B,0x39,0x7F,0x2F,0xC4,0xFE,0xBE,0x4C,0xC9,0x64,0x22,0x63,0x65,0xA3,0xFB,0x34,0xE7,0x1C,
	0x07,0x03,0xFB,0x43,0x4B,0x7E,0x15,0x5D,0xE3,0x47,0xB6,0x60,0x2E,0x9E,0x3C,0xC6,0x3F,0xAA,0xC1,0x4E,0x1F,0x2F,0x5D,0xBA,0x3B,0x28,0x3C,0x44,0x12,0xFB,0xCA,0x82,
	0x44,0x3A,0xB9,0x48,0xEC,0xF3,0x63,0x89,0x0F,0x9E,0xA2,0x4F,0x26,0x39,0x0A,0x8E,0xE2,0x5F,0x80,0xC8,0xD4,0xC6,0x33,0xCF,0x04,0x5D,0x40,0xEA,0xA5,0x48,0xB4,0x4E,
	0xEA,0xFC,0x1B,0x12,0xA7,0xFF,0x09,0xDE,0x39,0xEA,0x1B,0x6C,0x4D,0x00,0x00
};
void ILibDefaultLogger_WebSocket_OnReceive(struct ILibWebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, ILibWebServer_DoneFlag done)
{
	ILibRemoteLogging logger = ILibChainGetLogger(ILibTransportChain(sender));

	if (logger == NULL) { return; }
	if (done == ILibWebServer_DoneFlag_Done)
	{
		// Web Socket Disconnected
		ILibRemoteLogging_DeleteUserContext(logger, sender); 
		return;
	}
	if (done != ILibWebServer_DoneFlag_NotDone) { *beginPointer = endPointer; return; } // We're only going to handle the case where we receive a full fragment

	if (ILibRemoteLogging_Dispatch(logger, bodyBuffer, endPointer, sender) < 0)
	{
		ILibWebServer_WebSocket_Close(sender);
	}

	*beginPointer = endPointer;
}
void ILibDefaultLogger_Session_OnReceive(struct ILibWebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, ILibWebServer_DoneFlag done)
{
	ILibRemoteLogging logger = ILibChainGetLogger(ILibTransportChain(sender));
	if (done != ILibWebServer_DoneFlag_Done || logger == NULL) { return; }

	switch (ILibWebServer_WebSocket_GetDataType(sender))
	{
	case ILibWebServer_WebSocket_DataType_UNKNOWN:
		if (header->DirectiveLength == 3 && strncasecmp(header->Directive, "GET", 3) == 0)
		{
			if (header->DirectiveObjLength == 1 && strncasecmp(header->DirectiveObj, "/", 1) == 0)
			{
				int flen;
				char* f;

				flen = ILibReadFileFromDiskEx(&f, "web\\index.html");
				if (flen == 0)
				{
					ILibWebServer_StreamHeader_Raw(sender, 200, "OK", "\r\nContent-Type: text/html\r\nContent-Encoding: gzip", ILibAsyncSocket_MemoryOwnership_STATIC);
					ILibWebServer_StreamBody(sender, (char*)ILibDefaultLogger_HTML, sizeof(ILibDefaultLogger_HTML), ILibAsyncSocket_MemoryOwnership_STATIC, ILibWebServer_DoneFlag_Done);
				}
				else
				{
					ILibWebServer_StreamHeader_Raw(sender, 200, "OK", "\r\nContent-Type: text/html", ILibAsyncSocket_MemoryOwnership_STATIC);
					ILibWebServer_StreamBody(sender, f, flen, ILibAsyncSocket_MemoryOwnership_CHAIN, ILibWebServer_DoneFlag_Done);
				}
			}
			else
			{
				ILibWebServer_StreamHeader_Raw(sender, 404, "Not Found", NULL, ILibAsyncSocket_MemoryOwnership_STATIC);
				ILibWebServer_StreamBody(sender, NULL, 0, ILibAsyncSocket_MemoryOwnership_STATIC, ILibWebServer_DoneFlag_Done);
			}
		}
		else
		{
			ILibWebServer_StreamHeader_Raw(sender, 400, "Bad Request", NULL, ILibAsyncSocket_MemoryOwnership_STATIC);
			ILibWebServer_StreamBody(sender, NULL, 0, ILibAsyncSocket_MemoryOwnership_STATIC, ILibWebServer_DoneFlag_Done);
		}
		break;
	case ILibWebServer_WebSocket_DataType_REQUEST:
		if (ILibWebServer_IsCrossSiteRequest(sender) == NULL)
		{
			sender->OnReceive = &ILibDefaultLogger_WebSocket_OnReceive;
			ILibWebServer_UpgradeWebSocket(sender, 8192);
		}
		else
		{
			ILibRemoteLogging_printf(logger, ILibRemoteLogging_Modules_Microstack_Web, ILibRemoteLogging_Flags_VerbosityLevel_1, "Cross-Site Request!");
			ILibWebServer_StreamHeader_Raw(sender, 400, "Bad Request", NULL, ILibAsyncSocket_MemoryOwnership_STATIC);
			ILibWebServer_StreamBody(sender, NULL, 0, ILibAsyncSocket_MemoryOwnership_STATIC, ILibWebServer_DoneFlag_Done);
		}
		break;
	default:
		ILibWebServer_StreamHeader_Raw(sender, 400, "Bad Request", NULL, ILibAsyncSocket_MemoryOwnership_STATIC);
		ILibWebServer_StreamBody(sender, NULL, 0, ILibAsyncSocket_MemoryOwnership_STATIC, ILibWebServer_DoneFlag_Done);
		break;
	}
}
void ILibDefaultLogger_OnSession(struct ILibWebServer_Session *SessionToken, void *User)
{
	SessionToken->OnReceive = &ILibDefaultLogger_Session_OnReceive;
}
void ILibDefaultLogger_OnWrite(ILibRemoteLogging module, char* data, int dataLen, void *userContext)
{
	ILibTransport_Send(userContext, data, dataLen, ILibTransport_MemoryOwnership_USER, ILibTransport_DoneState_COMPLETE);
}

//! Initializes and starts a default ILibRemoteLogging module
/*!
\param chain Microstack chain to associate the logger with
\param portNumber Local port number to bind the WebListener to, which will serve the Web Interface (0 = random port)
\param path Path to the file to use for logging
\return The locally bound port number allocated to the WebListener
*/
ILibExportMethod unsigned short ILibStartDefaultLoggerEx(void* chain, unsigned short portNumber, char *path)
{
	((ILibBaseChain*)chain)->ChainLogger = ILibRemoteLogging_Create(ILibDefaultLogger_OnWrite);
	((ILibBaseChain*)chain)->LoggingWebServer = ILibWebServer_Create(chain, 5, portNumber, ILibDefaultLogger_OnSession, NULL);

	if (path != NULL)
	{
		((ILibBaseChain*)chain)->LoggingWebServerFileTransport = ILibRemoteLogging_CreateFileTransport(((ILibBaseChain*)chain)->ChainLogger, ILibRemoteLogging_Modules_UNKNOWN, ILibRemoteLogging_Flags_NONE, path, -1);
	}

	ILibForceUnBlockChain(chain);
	return(ILibWebServer_GetPortNumber(((ILibBaseChain*)chain)->LoggingWebServer));
}
#endif


//
// Do not Modify this method.
// This decompresses compressed string blocks, which is used to store the
// various description documents
//
char* ILibDecompressString(unsigned char* CurrentCompressed, const int bufferLength, const int DecompressedLength)
{
	unsigned char *RetVal = (unsigned char*)malloc(DecompressedLength+1);
	unsigned char *CurrentUnCompressed = RetVal;
	unsigned char *EndPtr = RetVal + DecompressedLength;
	int offset,length;
	if (RetVal == NULL) ILIBCRITICALEXIT(254);

	UNREFERENCED_PARAMETER( bufferLength );

	do
	{
		// UnCompressed Data Block
		memcpy_s(CurrentUnCompressed, (int)*CurrentCompressed, CurrentCompressed + 1, (int)*CurrentCompressed);
		MEMCHECK(assert((int)*CurrentCompressed <= (DecompressedLength+1) );)

		CurrentUnCompressed += (int)*CurrentCompressed;
		CurrentCompressed += 1+((int)*CurrentCompressed);

		// CompressedBlock
#ifdef REQUIRES_MEMORY_ALIGNMENT
		length = (unsigned short)((*(CurrentCompressed)) & 63);
		offset = ((unsigned short)(*(CurrentCompressed+1))<<2) + (((unsigned short)(*(CurrentCompressed))) >> 6);
#else
		length = (*((unsigned short*)(CurrentCompressed)))&((unsigned short)63);
		offset = (*((unsigned short*)(CurrentCompressed)))>>6;
#endif
		memcpy_s(CurrentUnCompressed, length, CurrentUnCompressed - offset, length);
		MEMCHECK(assert(length <= (DecompressedLength+1)-(CurrentUnCompressed - RetVal));)

		CurrentCompressed += 2;
		CurrentUnCompressed += length;
	} while (CurrentUnCompressed < EndPtr);

	RetVal[DecompressedLength] = 0;
	return((char*)RetVal);
}

void ILibChain_RunOnMicrostackThreadSink(void *obj)
{
	void* chain = ((void**)obj)[0];
	ILibChain_StartEvent handler = (ILibChain_StartEvent)((void**)obj)[1];
	void* user = ((void**)obj)[2];

	if (ILibIsChainBeingDestroyed(chain) == 0)
	{
		// Only Dispatch if the chain is still running
		if (handler != NULL) { handler(chain, user); }
	}
	free(obj);
}
//! Dispatch an operation to the Microstack Chain thread
/*!
	\param chain Microstack Chain to dispatch to
	\param handler Event to dispatch on the microstack thread
	\param user Custom user data to dispatch to the microstack thread
*/
void ILibChain_RunOnMicrostackThreadEx(void *chain, ILibChain_StartEvent handler, void *user)
{
	void** value = NULL;
	
	value = (void**)ILibMemory_Allocate(3 * sizeof(void*), 0, NULL, NULL);

	value[0] = chain;
	value[1] = handler;
	value[2] = user;
	ILibLifeTime_AddEx(ILibGetBaseTimer(chain), value, 0, &ILibChain_RunOnMicrostackThreadSink, &ILibChain_RunOnMicrostackThreadSink);
}

void ILibChain_Safe_Destroy(void *object)
{
	free(object);
}
void ILibChain_SafeAddSink(void *object)
{
	struct ILibBaseChain_SafeData *data = (struct ILibBaseChain_SafeData*)object;

	ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_2, "ILibChain_SafeAdd: %p [Pre: %p, Post: %p, Destroy: %p]", (void*)data->Object, (void*)((ILibChain_Link*)data->Object)->PreSelectHandler, (void*)((ILibChain_Link*)data->Object)->PostSelectHandler, (void*)((ILibChain_Link*)data->Object)->DestroyHandler);

	ILibAddToChain(data->Chain,data->Object);
	free(data);
}

/*! \fn void ILibChain_SafeAdd(void *chain, void *object)
\brief Dynamically add a link to a chain that is already running.
\par
\b Note: All objects added to the chain must extend/implement ILibChain
\param chain The chain to add the link to
\param object The link to add to the chain
*/
void ILibChain_SafeAdd(void *chain, void *object)
{
	struct ILibBaseChain *baseChain = (struct ILibBaseChain*)chain;
	struct ILibBaseChain_SafeData *data;
	if ((data = (struct ILibBaseChain_SafeData*)malloc(sizeof(struct ILibBaseChain_SafeData))) == NULL) ILIBCRITICALEXIT(254);
	memset(data, 0, sizeof(struct ILibBaseChain_SafeData));
	data->Chain = chain;
	data->Object = object;

	ILibLifeTime_Add(baseChain->Timer, data, 0, &ILibChain_SafeAddSink, &ILibChain_Safe_Destroy);
}
void ILibChain_SafeRemoveEx(void *chain, void *object)
{
	ILibLinkedList links = ILibChain_GetLinks(chain);
	void *node = ILibLinkedList_GetNode_Search(links, NULL, object);
	ILibChain_Link *link = (ILibChain_Link*)ILibLinkedList_GetDataFromNode(node);

	if (link != NULL)
	{
		if (link->DestroyHandler != NULL) { link->DestroyHandler(link); }
		free(link);
		ILibLinkedList_Remove(node);
	}
}
/*! \fn void ILibChain_SafeRemove(void *chain, void *object)
\brief Dynamically remove a link from a chain that is already running.
\param chain The chain to remove a link from
\param object The link to remove
*/
void ILibChain_SafeRemove(void *chain, void *object)
{
	if (ILibIsChainBeingDestroyed(chain) == 0)
	{
		//
		// We are always going to context switch to microstack thread, even if we are already on microstack thread,
		// because we cannot remove an item from the chain, if the call originated from the link that is being removed.
		// By forcing a context switch, we will gaurantee that the call originates from the BaseTimer link.
		//
		ILibRemoteLogging_printf(ILibChainGetLogger(chain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_2, "ILibChain_SafeRemove (%p)", (void*)object);
		ILibChain_RunOnMicrostackThreadEx(chain, ILibChain_SafeRemoveEx, object);
	}
}
//! Get the Links of a Chain object
/*!
	\param chain Chain object to query
	\return ILibLinkedList data structure holding the links
*/
ILibLinkedList ILibChain_GetLinks(void *chain)
{
	return(((ILibBaseChain*)chain)->Links);
}

void *ILibCreateChainEx(int extraMemorySize)
{
	struct ILibBaseChain *RetVal;

#if defined(WIN32) || defined(_WIN32_WCE)
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 0);


	if (WSAStartup(wVersionRequested, &wsaData) != 0) { ILIBCRITICALEXIT(1); }
#endif
	RetVal = ILibMemory_Allocate(sizeof(ILibBaseChain), extraMemorySize, NULL, NULL);

	RetVal->Links = ILibLinkedList_CreateEx(sizeof(ILibChain_Link_Hook));
	RetVal->LinksPendingDelete = ILibLinkedList_Create();

#if defined(WIN32) || defined(_WIN32_WCE)
	RetVal->TerminateFlag = 1;
	RetVal->TerminateSock = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT);
	RetVal->ChainProcessHandle = GetCurrentProcess();
	if (!SymInitialize(RetVal->ChainProcessHandle, NULL, TRUE)) { RetVal->ChainProcessHandle = NULL; }
#endif

	RetVal->TerminateFlag = 0;
	if (ILibChainLock_RefCounter == 0)
	{
		sem_init(&ILibChainLock, 0, 1);
	}
	ILibChainLock_RefCounter++;

	RetVal->Timer = ILibCreateLifeTime(RetVal);

	return(RetVal);
}

/*! \fn ILibCreateChain()
\brief Creates an empty Chain
\return Chain
*/
ILibExportMethod void *ILibCreateChain()
{
	return(ILibCreateChainEx(0));
}

/*! \fn ILibAddToChain(void *Chain, void *object)
\brief Add links to the chain
\par
\b Notes:
<P>  	Do <B>NOT</B> use this method to add a link to a chain that is already running.<br> All objects added to the chain must extend/implement ILibChain.
\param Chain The chain to add the link to
\param object The link to add to the chain
*/
void ILibAddToChain(void *Chain, void *object)
{
	//
	// Add link to the end of the chain (Linked List)
	//
	ILibLinkedList_AddTail(((ILibBaseChain*)Chain)->Links, object);
	((ILibChain_Link*)object)->ParentChain = Chain;
}

//! Return the base timer for this chain. Most of the time, new timers probably don't need to be created
/*!
	\param chain Microstack Chain to fetch the timer from
	\return Base Timer
*/
void* ILibGetBaseTimer(void *chain)
{
	//return ILibCreateLifeTime(chain);
	return ((struct ILibBaseChain*)chain)->Timer;
}

#ifdef WIN32
void __stdcall ILibForceUnBlockChain_APC(ULONG_PTR obj)
{
	struct ILibBaseChain *c = (struct ILibBaseChain*)obj;
	if (c->TerminateSock != ~0)
	{
		closesocket(c->TerminateSock);
	}
	c->TerminateSock = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT);
}
#endif
/*! \fn ILibForceUnBlockChain(void *Chain)
\brief Forces a Chain to unblock, and check for pending operations
\param Chain The chain to unblock
*/
void ILibForceUnBlockChain(void* Chain)
{
	struct ILibBaseChain *c = (struct ILibBaseChain*)Chain;

#if defined(WIN32)
	QueueUserAPC((PAPCFUNC)ILibForceUnBlockChain_APC, c->MicrostackThreadHandle !=NULL ? c->MicrostackThreadHandle : GetCurrentThread(), (ULONG_PTR)c);
#else
	//
	// Writing data on the pipe will trigger the select on Posix
	//
	if (c->TerminateWritePipe != NULL)
	{
		fprintf(c->TerminateWritePipe," ");
		fflush(c->TerminateWritePipe);
	}
#endif
}

/*! \fn void ILibChain_DestroyEx(void *subChain)
\brief Destroys a chain or subchain that was never started.
\par
<B>Do NOT</B> call this on a chain that is running, or has been stopped.
<br><B>Do NOT</B> call this on a subchain that was already added to another chain.
\param subChain The chain or subchain to destroy
*/
void ILibChain_DestroyEx(void *subChain)
{
	ILibChain_Link* module;
	void* node;

	if (subChain == NULL) return;
	((ILibBaseChain*)subChain)->TerminateFlag = 1;

	node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)subChain)->Links);
	while (node != NULL && (module = (ILibChain_Link*)ILibLinkedList_GetDataFromNode(node)) != NULL)
	{
		if (module->DestroyHandler != NULL) { module->DestroyHandler((void*)module); }
		free(module);
		node = ILibLinkedList_GetNextNode(node);
	}
	ILibLinkedList_Destroy(((ILibBaseChain*)subChain)->Links);
	ILibLinkedList_Destroy(((ILibBaseChain*)subChain)->LinksPendingDelete);

#ifdef _REMOTELOGGINGSERVER
	ILibRemoteLogging_Destroy(((ILibBaseChain*)subChain)->ChainLogger);
	if(((ILibBaseChain*)subChain)->LoggingWebServerFileTransport != NULL) { ILibTransport_Close(((ILibBaseChain*)subChain)->LoggingWebServerFileTransport); }
#endif

	free(subChain);
}
ILibChain_EventHookToken ILibChain_SetEventHook(void* chainLinkObject, int maxTimeout, ILibChain_EventHookHandler handler)
{
	ILibChain_Link_Hook *hook;
	ILibChain_Link *link = (ILibChain_Link*)chainLinkObject;
	ILibBaseChain *bChain = (ILibBaseChain*)link->ParentChain;
	void *node = ILibLinkedList_GetNode_Search(bChain->Links, NULL, chainLinkObject);
	hook = (ILibChain_Link_Hook*)ILibLinkedList_GetExtendedMemory(node);
	if (node != NULL && hook != NULL)
	{
		if (maxTimeout <= 0 && hook != NULL) { memset(hook, 0, sizeof(ILibChain_Link_Hook)); return(NULL); }
		hook->MaxTimeout = maxTimeout;
		hook->Handler = handler;
		return(node);
	}
	else
	{
		return(NULL);
	}
}
void ILibChain_UpdateEventHook(ILibChain_EventHookToken token, int maxTimeout)
{
	ILibChain_Link_Hook *hook = (ILibChain_Link_Hook*)ILibLinkedList_GetExtendedMemory(token);
	if (hook == NULL) { return; }
	if (maxTimeout > 0)
	{
		hook->MaxTimeout = maxTimeout;
	}
	else
	{
		memset(hook, 0, sizeof(ILibChain_Link_Hook));
	}
}
ILibExportMethod void ILibChain_Continue(void *chain, ILibChain_Link **modules, int moduleCount, int maxTimeout)
{
	int i;
	ILibBaseChain *root = (ILibBaseChain*)chain;
	int slct = 0, v = 0, vX = 0;
	struct timeval tv;
	long endTime;
	fd_set readset;
	fd_set errorset;
	fd_set writeset;
	void *currentNode;

	if (root->continuationState != ILibChain_ContinuationState_INACTIVE) { return; }
	root->continuationState = ILibChain_ContinuationState_CONTINUE;
	currentNode = root->node;

	gettimeofday(&tv, NULL);
	endTime = tv.tv_sec + maxTimeout;

	ILibRemoteLogging_printf(ILibChainGetLogger(chain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ContinueChain...");

	while (root->TerminateFlag == 0 && root->continuationState == ILibChain_ContinuationState_CONTINUE)
	{
		slct = 0;
		FD_ZERO(&readset);
		FD_ZERO(&errorset);
		FD_ZERO(&writeset);

		gettimeofday(&tv, NULL);

		if (tv.tv_sec >= endTime)
		{
			break;
		}
		
		tv.tv_sec = endTime - tv.tv_sec;
		tv.tv_usec = 0;
		v = (int)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));

		for (i = 0; i < moduleCount; ++i)
		{
			if (modules[i]->PreSelectHandler != NULL)
			{
				root->node = modules[i];
				vX = v;
				modules[i]->PreSelectHandler((void*)modules[i], &readset, &writeset, &errorset, &vX);
				if (vX < v) { v = vX; }
			}
		}
		tv.tv_sec = v / 1000;
		tv.tv_usec = 1000 * (v % 1000);

#if defined(WIN32) || defined(_WIN32_WCE)
		//
		// Put the Terminate socket in the FDSET, for ILibForceUnblockChain
		//

		FD_SET(root->TerminateSock, &errorset);
#else
		//
		// Put the Read end of the Pipe in the FDSET, for ILibForceUnBlockChain
		//
		FD_SET(fileno(root->TerminateReadPipe), &readset);
#endif


		slct = select(FD_SETSIZE, &readset, &writeset, &errorset, &tv);
		if (slct == -1)
		{
			//
			// If the select simply timed out, we need to clear these sets
			//
			FD_ZERO(&readset);
			FD_ZERO(&writeset);
			FD_ZERO(&errorset);
		}

#ifndef WIN32
		if (FD_ISSET(fileno(root->TerminateReadPipe), &readset))
		{
			//
			// Empty the pipe
			//
			while (fgetc(root->TerminateReadPipe) != EOF)
			{
			}
		}
#endif

		for (i = 0; i < moduleCount && root->continuationState == ILibChain_ContinuationState_CONTINUE; ++i)
		{
			if (modules[i]->PostSelectHandler != NULL)
			{
				root->node = modules[i];
				modules[i]->PostSelectHandler((void*)modules[i], slct, &readset, &writeset, &errorset);
			}
		}

	}
	ILibRemoteLogging_printf(ILibChainGetLogger(chain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "ContinueChain...Ending...");

	root->continuationState = ILibChain_ContinuationState_INACTIVE;
	root->node = currentNode;
}

ILibExportMethod void ILibChain_EndContinue(void *chain)
{
	((ILibBaseChain*)chain)->continuationState = ILibChain_ContinuationState_END_CONTINUE;
	ILibForceUnBlockChain(chain);
}

#if defined(WIN32)
int ILib_WindowsExceptionFilter(int exceptionCode, void *exceptionInfo, CONTEXT *exceptionContext)
{
	if (IsDebuggerPresent()) { return(EXCEPTION_CONTINUE_SEARCH); }
	if (exceptionCode == EXCEPTION_ACCESS_VIOLATION || exceptionCode == EXCEPTION_STACK_OVERFLOW || exceptionCode == EXCEPTION_INVALID_HANDLE)
	{
		memcpy_s(exceptionContext, sizeof(CONTEXT), ((EXCEPTION_POINTERS*)exceptionInfo)->ContextRecord, sizeof(CONTEXT));
		return(EXCEPTION_EXECUTE_HANDLER);
	}
	return(EXCEPTION_CONTINUE_SEARCH);
}

void ILib_WindowsExceptionDebug(CONTEXT *exceptionContext)
{
	char buffer[4096];
	STACKFRAME64 StackFrame;
	char symBuffer[4096];
	char imgBuffer[4096];
	DWORD MachineType;
	int len = 0;

	ZeroMemory(&StackFrame, sizeof(STACKFRAME64));
	buffer[0] = 0;

#ifndef WIN64
	MachineType = IMAGE_FILE_MACHINE_I386;
	StackFrame.AddrPC.Offset = exceptionContext->Eip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = exceptionContext->Ebp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = exceptionContext->Esp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#else
	MachineType = IMAGE_FILE_MACHINE_AMD64;
	StackFrame.AddrPC.Offset = exceptionContext->Rip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = exceptionContext->Rsp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = exceptionContext->Rsp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#endif


	if (StackWalk64(
		MachineType,
		GetCurrentProcess(),
		GetCurrentThread(),
		&StackFrame,
		MachineType == IMAGE_FILE_MACHINE_I386
		? NULL
		: exceptionContext,
		NULL,
		SymFunctionTableAccess64,
		SymGetModuleBase64,
		NULL))
	{

		if (StackFrame.AddrPC.Offset != 0)
		{
			//
			// Valid frame.
			//
			PIMAGEHLP_LINE64 pimg = (PIMAGEHLP_LINE64)imgBuffer;
			DWORD64 tmp = 0;
			DWORD tmp2;
			PSYMBOL_INFO psym = (PSYMBOL_INFO)symBuffer;
			psym->SizeOfStruct = sizeof(SYMBOL_INFO);
			psym->MaxNameLen = MAX_SYM_NAME;
			pimg->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

			len = sprintf_s(buffer, sizeof(buffer), "FATAL EXCEPTION @ ");

			if (SymFromAddr(GetCurrentProcess(), StackFrame.AddrPC.Offset, &tmp, psym))
			{
				len += sprintf_s(buffer + len, sizeof(buffer) - len, "[%s", (char*)(psym->Name));
				if (SymGetLineFromAddr64(GetCurrentProcess(), StackFrame.AddrPC.Offset, &tmp2, pimg))
				{
					len += sprintf_s(buffer + len, sizeof(buffer) - len, " => %s:%d]\n", (char*)(pimg->FileName), pimg->LineNumber);
				}
				else
				{
					len += sprintf_s(buffer + len, sizeof(buffer) - len, "]\n");
				}
			}
			else
			{
				util_tohex((char*)&(StackFrame.AddrPC.Offset), sizeof(DWORD64), imgBuffer);
				len += sprintf_s(buffer + len, sizeof(buffer) - len, "[FuncAddr: 0x%s]\n", imgBuffer);
			}
		}
	}
	
	ILIBCRITICALEXITMSG(254, buffer);
}
#elif defined(_POSIX)
#include <execinfo.h>
char ILib_POSIX_CrashParamBuffer[5 * sizeof(void*)];
void ILib_POSIX_CrashHandler(int code)
{
	char msgBuffer[16384];
	int msgLen = 0;
	void *buffer[100];
	char **strings;
	int sz, i, c;
	char *ptr;

	signal(SIGSEGV, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);

	sz = backtrace(buffer, 100);
	strings = backtrace_symbols(buffer, sz);

	if (code == SIGSEGV || code == SIGABRT)
	{
		memcpy_s(msgBuffer + msgLen, sizeof(msgBuffer) - msgLen, "** CRASH **\n", 12);
		msgLen += 12;
	}
	else if (code == 254)
	{
		memcpy_s(msgBuffer + msgLen, sizeof(msgBuffer) - msgLen, "** CRITICAL EXIT **\n", 20);
		msgLen += 20;
	}
	else
	{
		memcpy_s(msgBuffer + msgLen, sizeof(msgBuffer) - msgLen, "** Microstack Thread STUCK **\n", 30);
		msgLen += 30;
	}

	for (i = 1; i < sz; ++i)
	{
		pid_t pid;
		int fd[2];
		size_t symlen = strnlen_s(strings[i], sizeof(msgBuffer));

		memcpy_s(msgBuffer + msgLen, sizeof(msgBuffer) - msgLen, strings[i], symlen);
		msgLen += symlen;
		msgBuffer[msgLen] = '\n';
		msgLen += 1;


		ptr = strings[i];
		for (c = 0; c < (int)symlen; ++c)
		{
			if ((strings[i])[c] == '[') { ptr = strings[i] + c + 1; }
			if ((strings[i])[c] == ']') { (strings[i])[c] = 0; }
			if ((strings[i])[c] == 0) { break; }
		}
		if (pipe(fd) == 0)
		{
			pid = vfork();
			if (pid == 0)
			{
				dup2(fd[1], STDOUT_FILENO);
				close(fd[1]);

				((char**)ILib_POSIX_CrashParamBuffer)[1] = ptr;
				execv("/usr/bin/addr2line", (char**)ILib_POSIX_CrashParamBuffer);
				if (write(STDOUT_FILENO, "??:0", 4)) {}
				exit(0);
			}
			if (pid > 0)
			{
				char tmp[8192];
				int len;

				len = read(fd[0], tmp, 8192);
				if (len > 0 && tmp[0] != '?')
				{
					memcpy_s(msgBuffer + msgLen, sizeof(msgBuffer) - msgLen, "=> ", 3);
					msgLen += 3;
					memcpy_s(msgBuffer + msgLen, sizeof(msgBuffer) - msgLen, tmp, len);
					msgLen += len;
				}
				close(fd[0]);
			}
		}
	}
	msgBuffer[msgLen] = 0;
	ILIBCRITICALEXITMSG(254, msgBuffer);
}
char* ILib_POSIX_InstallCrashHandler(char *exename)
{
	char *retVal = NULL;
	if ((retVal = realpath(exename, NULL)) != NULL)
	{
		((char**)ILib_POSIX_CrashParamBuffer)[0] = "addr2line";
		((char**)ILib_POSIX_CrashParamBuffer)[2] = "-e";
		((char**)ILib_POSIX_CrashParamBuffer)[3] = retVal;
		((char**)ILib_POSIX_CrashParamBuffer)[4] = NULL;
		signal(SIGSEGV, ILib_POSIX_CrashHandler);
	}
	return(retVal);
}
#endif

char* ILibChain_Debug(void *chain, char* buffer, int bufferLen)
{
	char *retVal = NULL;
	ILibBaseChain *bChain = (ILibBaseChain*)chain;
#if defined(WIN32)
	char symBuffer[4096];
	char imgBuffer[4096];
	DWORD MachineType;
	STACKFRAME64 StackFrame;
	DWORD64 addrOffset;
	int len = 0;
	CONTEXT ctx;

	ZeroMemory(&StackFrame, sizeof(STACKFRAME64));
	if (chain != NULL)
	{
		if (bChain->MicrostackThreadHandle != NULL)
		{
			SuspendThread(bChain->MicrostackThreadHandle);
			memset(&(bChain->MicrostackThreadContext), 0, sizeof(CONTEXT));
			bChain->MicrostackThreadContext.ContextFlags = CONTEXT_FULL;
			if (GetThreadContext(bChain->MicrostackThreadHandle, &(bChain->MicrostackThreadContext)) == 0)
			{
				len = GetLastError();
			}
			ResumeThread(bChain->MicrostackThreadHandle);
		}
	}
	else
	{
		RtlCaptureContext(&ctx);
	}

	if (len != 0) { return(NULL); }

#ifndef WIN64
	MachineType = IMAGE_FILE_MACHINE_I386;
	StackFrame.AddrPC.Offset = bChain != NULL ? bChain->MicrostackThreadContext.Eip : ctx.Eip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = bChain != NULL ? bChain->MicrostackThreadContext.Ebp : ctx.Ebp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = bChain != NULL ? bChain->MicrostackThreadContext.Esp : ctx.Esp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#else
	MachineType = IMAGE_FILE_MACHINE_AMD64;
	StackFrame.AddrPC.Offset = bChain != NULL ? bChain->MicrostackThreadContext.Rip : ctx.Rip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = bChain != NULL ? bChain->MicrostackThreadContext.Rsp : ctx.Rsp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = bChain != NULL ? bChain->MicrostackThreadContext.Rsp : ctx.Rsp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
#endif

	buffer[0] = 0;
	retVal = buffer;

	while (1)
	{
		if (!StackWalk64(
			MachineType,
			bChain != NULL ? bChain->ChainProcessHandle : GetCurrentProcess(),
			bChain != NULL ? bChain->MicrostackThreadHandle : GetCurrentThread(),
			&StackFrame,
			MachineType == IMAGE_FILE_MACHINE_I386
			? NULL
			: (bChain!=NULL ? &(bChain->MicrostackThreadContext) : &ctx),
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL))
		{
			//
			// Maybe it failed, maybe we have finished walking the stack.
			//
			break;
		}

		if (StackFrame.AddrPC.Offset != 0)
		{
			//
			// Valid frame.
			//
			addrOffset = StackFrame.AddrPC.Offset;
			{
				PIMAGEHLP_LINE64 pimg = (PIMAGEHLP_LINE64)imgBuffer;
				DWORD64 tmp = 0;
				DWORD tmp2;
				PSYMBOL_INFO psym = (PSYMBOL_INFO)symBuffer;
				psym->SizeOfStruct = sizeof(SYMBOL_INFO);
				psym->MaxNameLen = MAX_SYM_NAME;
				pimg->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				
				if (SymFromAddr(bChain!=NULL && bChain->ChainProcessHandle != NULL ? bChain->ChainProcessHandle : GetCurrentProcess(), addrOffset, &tmp, psym))
				{
					len += sprintf_s(buffer + len, bufferLen - len, "[%s", (char*)(psym->Name));
					if (SymGetLineFromAddr64(bChain != NULL && bChain->ChainProcessHandle != NULL ? bChain->ChainProcessHandle : GetCurrentProcess(), addrOffset, &tmp2, pimg))
					{
						len += sprintf_s(buffer + len, bufferLen - len, " => %s:%d]\n", (char*)(pimg->FileName), pimg->LineNumber);
					}
					else
					{
						len += sprintf_s(buffer + len, bufferLen - len, "]\n");
					}
				}
				else
				{
					char addrBuffer[255];
					util_tohex((char*)&addrOffset, sizeof(DWORD64), addrBuffer);
					len += sprintf_s(buffer + len, bufferLen - len, "[FuncAddr: 0x%s]\n", addrBuffer);
				}
			}
			if (bChain != NULL && bChain->MicrostackThreadHandle == NULL)
			{
				break;
			}
		}
		else
		{
			//
			// Base reached.
			//
			break;
		}
	}

#else
	pthread_kill(bChain->ChainThreadID, SIGUSR1);
	return(NULL);
#endif
	return(retVal);
}


#ifdef ILibChain_WATCHDOG_TIMEOUT
void ILibChain_WatchDogStart(void *obj)
{
	char msg[4096];
	ILibBaseChain *chain = (ILibBaseChain*)obj;
	
#ifndef WIN32
	fd_set readset, writeset, errorset;
	int slct;
	struct timeval tv;
	long stamp = ILibGetTimeStamp();
	tv.tv_usec = 0;
	tv.tv_sec = ILibChain_WATCHDOG_TIMEOUT / 1000;
#endif
	int Pre1=0, Pre2=0, Post1=0, Post2=0;
	sprintf_s(msg, sizeof(msg), "Microstack STUCK: @ ");
	
#ifdef WIN32
	while (WaitForSingleObject(chain->WatchDogTerminator, ILibChain_WATCHDOG_TIMEOUT) == WAIT_TIMEOUT)
#else
	FD_ZERO(&readset);
	FD_ZERO(&errorset);
	FD_ZERO(&writeset);
	FD_SET(chain->WatchDogTerminator[0], &readset);
	while((slct = select(FD_SETSIZE, &readset, &writeset, &errorset, &tv)) == 0 || slct == EINTR)
#endif
	{
#ifndef WIN32
		tv.tv_usec = 0;
		tv.tv_sec = ILibChain_WATCHDOG_TIMEOUT / 1000;

		if (slct == EINTR)
		{
			slct = (int)(ILibGetTimeStamp() - stamp);
			if (slct < ILibChain_WATCHDOG_TIMEOUT)
			{
				tv.tv_sec = (ILibChain_WATCHDOG_TIMEOUT - slct) / 1000;
				continue;
			}
		}
		stamp = ILibGetTimeStamp();
		FD_ZERO(&readset);
		FD_ZERO(&errorset);
		FD_ZERO(&writeset);
		FD_SET(chain->WatchDogTerminator[0], &readset);
#endif
		Pre1 = chain->PreSelectCount;
		Post1 = chain->PostSelectCount;

		if (Pre1 == Post1)
		{
			if (Pre1 == Pre2 && Pre2 != 0)
			{
				// STUCK!
#ifdef WIN32
				ILibChain_Debug(chain, msg+20, sizeof(msg)-20);
				ILIBCRITICALEXITMSG(254, msg);
#else
				pthread_kill(chain->ChainThreadID, SIGUSR1);
				break;
#endif
			}
			else
			{
				Pre2 = Pre1;
				Post2 = Post1;
			}
		}
	}
}
#endif

/*! \fn ILibStartChain(void *Chain)
\brief Starts a Chain
\par
This method will use the current thread. This thread will be refered to as the
microstack thread. All events and processing will be done on this thread. This method
will not return until ILibStopChain is called.
\param Chain The chain to start
*/
ILibExportMethod void ILibStartChain(void *Chain)
{
	ILibBaseChain *chain = (ILibBaseChain*)Chain;
	ILibChain_Link *module;
	ILibChain_Link_Hook *nodeHook;

	fd_set readset;
	fd_set errorset;
	fd_set writeset;

	struct timeval tv;
	int slct;
	int v, vX;
#if !defined(WIN32) && !defined(_WIN32_WCE)
	int TerminatePipe[2];
	int flags;
#endif

	if (Chain == NULL) { return; }
	chain->PreSelectCount = chain->PostSelectCount = 0;

#if defined(WIN32)
	chain->ChainThreadID = GetCurrentThreadId();
	chain->ChainProcessHandle = GetCurrentProcess();
	chain->MicrostackThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, chain->ChainThreadID);
#else
	chain->ChainThreadID = pthread_self();
#endif

	if (gILibChain == NULL) {gILibChain = Chain;} // Set the global instance if it's not already set
#if defined(ILibChain_WATCHDOG_TIMEOUT)
#ifdef WIN32
	chain->WatchDogTerminator = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
	if (pipe(chain->WatchDogTerminator) == 0)
	{
		flags = fcntl(chain->WatchDogTerminator[0], F_GETFL, 0);
		fcntl(chain->WatchDogTerminator[0], F_SETFL, O_NONBLOCK | flags);
		signal(SIGUSR1, ILib_POSIX_CrashHandler);
	}
#endif
	chain->WatchDogThread = ILibSpawnNormalThread(ILibChain_WatchDogStart, chain);
#endif

	//
	// Use this thread as if it's our own. Keep looping until we are signaled to stop
	//
	FD_ZERO(&readset);
	FD_ZERO(&errorset);
	FD_ZERO(&writeset);

#if !defined(WIN32) && !defined(_WIN32_WCE)
	// 
	// For posix, we need to use a pipe to force unblock the select loop
	//
	if (pipe(TerminatePipe) == -1) {} // TODO: Pipe error
	flags = fcntl(TerminatePipe[0],F_GETFL,0);
	//
	// We need to set the pipe to nonblock, so we can blindly empty the pipe
	//
	fcntl(TerminatePipe[0],F_SETFL,O_NONBLOCK|flags);
	((struct ILibBaseChain*)Chain)->TerminateReadPipe = fdopen(TerminatePipe[0],"r");
	((struct ILibBaseChain*)Chain)->TerminateWritePipe = fdopen(TerminatePipe[1],"w");
#endif

	chain->RunningFlag = 1;
	while (chain->TerminateFlag == 0)
	{
		slct = 0;
		FD_ZERO(&readset);
		FD_ZERO(&errorset);
		FD_ZERO(&writeset);
		tv.tv_sec = UPNP_MAX_WAIT;
		tv.tv_usec = 0;

		//
		// Iterate through all the PreSelect function pointers in the chain
		//
		chain->node = ILibLinkedList_GetNode_Head(chain->Links);
		v = (int)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
		while(chain->node!=NULL && (module=(ILibChain_Link*)ILibLinkedList_GetDataFromNode(chain->node))!=NULL)
		{
			if(module->PreSelectHandler != NULL)
			{
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
				vX = v;
				module->PreSelectHandler((void*)module, &readset, &writeset, &errorset, &vX);
				if (vX < v) { v = vX; }
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
			}
			nodeHook = (ILibChain_Link_Hook*)ILibLinkedList_GetExtendedMemory(chain->node);
			if (nodeHook->MaxTimeout > 0 && nodeHook->MaxTimeout < v) { v = nodeHook->MaxTimeout; }
			chain->node = ILibLinkedList_GetNextNode(chain->node);
		}
		tv.tv_sec =  v / 1000;
		tv.tv_usec = 1000 * (v % 1000);


#if defined(WIN32) || defined(_WIN32_WCE)
		//
		// Add the fake socket, for ILibForceUnBlockChain
		//
		FD_SET(chain->TerminateSock, &errorset);
#else
		//
		// Put the Read end of the Pipe in the FDSET, for ILibForceUnBlockChain
		//
		FD_SET(TerminatePipe[0], &readset);
#endif
		sem_wait(&ILibChainLock);
		while(ILibLinkedList_GetCount(((ILibBaseChain*)Chain)->LinksPendingDelete) > 0)
		{
			chain->node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->LinksPendingDelete);
			module = (ILibChain_Link*)ILibLinkedList_GetDataFromNode(chain->node);
			ILibLinkedList_Remove_ByData(((ILibBaseChain*)Chain)->Links, module);
			ILibLinkedList_Remove(chain->node);
			if (module != NULL)
			{
				if (module->DestroyHandler != NULL) { module->DestroyHandler((void*)module); }
				free(module);
			}
		}
		sem_post(&ILibChainLock);

		//
		// The actual Select Statement
		//
		chain->PreSelectCount++;
		slct = select(FD_SETSIZE, &readset, &writeset, &errorset, &tv);
		chain->PostSelectCount++;

		if (slct == -1)
		{
			//
			// If the select simply timed out, we need to clear these sets
			//
			FD_ZERO(&readset);
			FD_ZERO(&writeset);
			FD_ZERO(&errorset);
		}

#ifndef WIN32
		if (FD_ISSET(TerminatePipe[0], &readset))
		{
			//
			// Empty the pipe
			//
			while (fgetc(((struct ILibBaseChain*)Chain)->TerminateReadPipe)!=EOF)
			{
			}
		}
#endif
		//
		// Iterate through all of the PostSelect in the chain
		//
		chain->node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
		while(chain->node!=NULL && (module=(ILibChain_Link*)ILibLinkedList_GetDataFromNode(chain->node))!=NULL)
		{
			if (module->PostSelectHandler != NULL)
			{
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
				module->PostSelectHandler((void*)module, slct, &readset, &writeset, &errorset);
#ifdef MEMORY_CHECK
#ifdef WIN32
				//_CrtCheckMemory();
#endif
#endif
			}
			nodeHook = (ILibChain_Link_Hook*)ILibLinkedList_GetExtendedMemory(chain->node);
			if (nodeHook->Handler != NULL) { nodeHook->Handler(module, chain->node); }
			chain->node = ILibLinkedList_GetNextNode(chain->node);
		}
	}

	//
	// This loop will start, when the Chain was signaled to quit. Clean up the chain by iterating
	// through all the Destroy methods. Not all modules in the chain will have a destroy method,
	// but call the ones that do.
	//
	if (chain->WatchDogThread != NULL)
	{
#ifdef WIN32
		SetEvent(chain->WatchDogTerminator);
#else
		if (write(chain->WatchDogTerminator[1], " ", 1)) {}
#endif
	}

	// Because many modules in the chain are using the base chain timer which is the first node
	// in the chain in the base timer), these modules may start cleaning up their timers. So, we
	// are going to make an exception and Destroy the base timer last, something that would usualy
	// be destroyed first since it's at the start of the chain. This will allow modules to clean
	// up nicely.
	//
	chain->node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
	if(chain->node != NULL) {chain->node = ILibLinkedList_GetNextNode(chain->node);} // Skip the base timer which is the first element in the chain.

	//
	// Set the Terminate Flag to 1, so that ILibIsChainBeingDestroyed returns non-zero when we start cleanup
	//
	((struct ILibBaseChain*)Chain)->TerminateFlag = 1;

	while(chain->node!=NULL && (module=(ILibChain_Link*)ILibLinkedList_GetDataFromNode(chain->node))!=NULL)
	{
		//
		// If this module has a destroy method, call it.
		//
		if (module->DestroyHandler != NULL) module->DestroyHandler((void*)module);
		//
		// After calling the Destroy, we free the module and move to the next
		//
		free(module);
		chain->node = ILibLinkedList_Remove(chain->node);
	}

	if (gILibChain ==  Chain) { gILibChain = NULL; } // Reset the global instance if it was set

	//
	// Go back and destroy the base timer for this chain, the first element in the chain.
	//
	chain->node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->Links);
	if (chain->node!=NULL && (module=(ILibChain_Link*)ILibLinkedList_GetDataFromNode(chain->node))!=NULL)
	{
		module->DestroyHandler((void*)module);
		free(module);
		((ILibBaseChain*)Chain)->Timer = NULL;
	}

	ILibLinkedList_Destroy(((ILibBaseChain*)Chain)->Links);

	chain->node = ILibLinkedList_GetNode_Head(((ILibBaseChain*)Chain)->LinksPendingDelete);
	while(chain->node != NULL && (module = (ILibChain_Link*)ILibLinkedList_GetDataFromNode(chain->node)) != NULL)
	{
		if(module->DestroyHandler != NULL) {module->DestroyHandler((void*)module);}
		free(module);
		chain->node = ILibLinkedList_GetNextNode(chain->node);
	}
	ILibLinkedList_Destroy(((ILibBaseChain*)Chain)->LinksPendingDelete);

#ifdef _REMOTELOGGINGSERVER
	if (((ILibBaseChain*)Chain)->LoggingWebServer != NULL && ((ILibBaseChain*)Chain)->ChainLogger != NULL) 
	{
		if (((ILibBaseChain*)Chain)->LoggingWebServerFileTransport != NULL) { ILibTransport_Close(((ILibBaseChain*)Chain)->LoggingWebServerFileTransport); }
		ILibRemoteLogging_Destroy(((ILibBaseChain*)Chain)->ChainLogger); 
	}
#endif
	if(((struct ILibBaseChain*)Chain)->ChainStash != NULL) {ILibHashtable_Destroy(((struct ILibBaseChain*)Chain)->ChainStash);}

	//
	// Now we actually free the chain, before we just freed what the chain pointed to.
	//
#if !defined(WIN32) && !defined(_WIN32_WCE)
	//
	// Free the pipe resources
	//
	fclose(((ILibBaseChain*)Chain)->TerminateReadPipe);
	fclose(((ILibBaseChain*)Chain)->TerminateWritePipe);
	((ILibBaseChain*)Chain)->TerminateReadPipe=0;
	((ILibBaseChain*)Chain)->TerminateWritePipe=0;
#endif
#if defined(WIN32)
	if (((ILibBaseChain*)Chain)->TerminateSock != ~0)
	{
		closesocket(((ILibBaseChain*)Chain)->TerminateSock);
		((ILibBaseChain*)Chain)->TerminateSock = (SOCKET)~0;
	}
#endif

#ifdef WIN32
	WSACleanup();
#endif
	if (ILibChainLock_RefCounter == 1)
	{
		sem_destroy(&ILibChainLock);
	}
	--ILibChainLock_RefCounter;
	free(Chain);
}

/*! \fn ILibStopChain(void *Chain)
\brief Stops a chain
\par
This will signal the microstack thread to shutdown. When the chain cleans itself up, 
the thread that is blocked on ILibStartChain will return.
\param Chain The Chain to stop
*/
ILibExportMethod void ILibStopChain(void *Chain)
{
	((struct ILibBaseChain*)Chain)->TerminateFlag = 2;
	ILibForceUnBlockChain(Chain);
}

/*! \fn ILibDestructXMLNodeList(struct ILibXMLNode *node)
\brief Frees resources from an XMLNodeList tree that was returned from ILibParseXML
\param node The XML Tree to clean up
*/
void ILibDestructXMLNodeList(struct ILibXMLNode *node)
{
	struct ILibXMLNode *temp;
	while (node!=NULL)
	{
		temp = node->Next;
		if (node->Reserved2!=NULL)
		{
			// If there was a namespace table, delete it
			ILibDestroyHashTree(node->Reserved2);
		}
		free(node);
		node = temp;
	}
}

/*! \fn ILibDestructXMLAttributeList(struct ILibXMLAttribute *attribute)
\brief Frees resources from an AttributeList that was returned from ILibGetXMLAttributes
\param attribute The Attribute Tree to clean up
*/
void ILibDestructXMLAttributeList(struct ILibXMLAttribute *attribute)
{
	struct ILibXMLAttribute *temp;
	while (attribute!=NULL)
	{
		temp = attribute->Next;
		free(attribute);
		attribute = temp;
	}
}

/*! \fn ILibProcessXMLNodeList(struct ILibXMLNode *nodeList)
\brief Pro-process an XML node list
\par
Checks XML for validity, while at the same time populate helper properties on each node,
such as Parent, Peer, etc, to aid in XML parsing.
\param nodeList The XML Tree to process
\return 0 if the XML is valid, nonzero otherwise
*/
int ILibProcessXMLNodeList(struct ILibXMLNode *nodeList)
{
	int RetVal = 0;
	struct ILibXMLNode *current = nodeList;
	struct ILibXMLNode *temp;
	void *TagStack;

	ILibCreateStack(&TagStack);

	//
	// Iterate through the node list, and setup all the pointers
	// such that all StartElements have pointers to EndElements,
	// And all StartElements have pointers to siblings and parents.
	//
	while (current != NULL)
	{
		if (current->Name == NULL) { ILibClearStack(&TagStack); return -1; }
		if (memcmp(current->Name, "!", 1) == 0)
		{
			// Comment
			temp = current;
			current = (struct ILibXMLNode*)ILibPeekStack(&TagStack);
			if (current!=NULL)
			{
				current->Next = temp->Next;
			}
			else
			{
				current = temp;
			}
		}
		else if (current->StartTag!=0)
		{
			// Start Tag
			current->Parent = (struct ILibXMLNode*)ILibPeekStack(&TagStack);
			ILibPushStack(&TagStack,current);
		}
		else
		{
			// Close Tag

			//
			// Check to see if there is supposed to be an EndElement
			//
			temp = (struct ILibXMLNode*)ILibPopStack(&TagStack);
			if (temp != NULL)
			{
				//
				// Checking to see if this EndElement is correct in scope
				//
				if (temp->NameLength == current->NameLength && memcmp(temp->Name, current->Name, current->NameLength)==0)
				{
					//
					// Now that we know this EndElement is correct, set the Peer
					// pointers of the previous sibling
					//
					if (current->Next != NULL)
					{
						if (current->Next->StartTag != 0)
						{
							temp->Peer = current->Next;
						}
					}
					temp->ClosingTag = current;
					current->StartingTag = temp;
				}
				else
				{
					// Illegal Close Tag Order
					RetVal = -2;
					break;
				}
			}
			else
			{
				// Illegal Close Tag
				RetVal = -1;
				break;
			}
		}
		current = current->Next;
	}

	//
	// If there are still elements in the stack, that means not all the StartElements
	// have associated EndElements, which means this XML is not valid XML.
	//
	if (TagStack!=NULL)
	{
		// Incomplete XML
		RetVal = -3;
		ILibClearStack(&TagStack);
	}

	return(RetVal);
}

/*! \fn ILibXML_LookupNamespace(struct ILibXMLNode *currentLocation, char *prefix, int prefixLength)
\brief Resolves a namespace prefix from the scope of the given node
\param currentLocation The node used to start the resolve
\param prefix The namespace prefix to resolve
\param prefixLength The lenght of the prefix
\return The resolved namespace. NULL if unable to resolve
*/
char* ILibXML_LookupNamespace(struct ILibXMLNode *currentLocation, char *prefix, int prefixLength)
{
	struct ILibXMLNode *temp = currentLocation;
	int done = 0;
	char* RetVal = "";

	//
	// If the specified Prefix is zero length, we interpret that to mean
	// they want to lookup the default namespace
	//
	if (prefixLength == 0)
	{
		//
		// This is the default namespace prefix
		//
		prefix = "xmlns";
		prefixLength = 5;
	}

	//
	// From the current node, keep traversing up the parents, until we find a match.
	// Each step we go up, is a step wider in scope.
	//
	do
	{
		if (temp->Reserved2 != NULL)
		{
			if (ILibHasEntry(temp->Reserved2, prefix, prefixLength) != 0)
			{
				//
				// As soon as we find the namespace declaration, stop
				// iterating the tree, as it would be a waste of time
				//
				RetVal = (char*)ILibGetEntry(temp->Reserved2, prefix, prefixLength);
				done = 1;
			}
		}
		temp = temp->Parent;
	} while (temp != NULL && done == 0);
	return RetVal;
}

/*! \fn ILibXML_BuildNamespaceLookupTable(struct ILibXMLNode *node)
\brief Builds the lookup table used by ILibXML_LookupNamespace
\param node This node will be the highest scoped
*/
void ILibXML_BuildNamespaceLookupTable(struct ILibXMLNode *node)
{
	struct ILibXMLAttribute *attr,*currentAttr;
	struct ILibXMLNode *current = node;

	//
	// Iterate through all the StartElements, and build a table of the declared namespaces
	//
	while (current != NULL)
	{
		if (current->StartTag != 0)
		{
			//
			// Reserved2 is the HashTable containing the fully qualified namespace
			// keyed by the namespace prefix
			//
			current->Reserved2 = ILibInitHashTree();
			currentAttr = attr = ILibGetXMLAttributes(current);
			if (attr != NULL)
			{
				//
				// Iterate through all the attributes to find namespace declarations
				//
				while (currentAttr != NULL)
				{
					if (currentAttr->NameLength == 5 && currentAttr->Name != NULL && memcmp(currentAttr->Name, "xmlns", 5) == 0)
					{
						// Default Namespace Declaration
						currentAttr->Value[currentAttr->ValueLength] = 0;
						ILibAddEntry(current->Reserved2, "xmlns", 5, currentAttr->Value);
					}
					else if (currentAttr->PrefixLength == 5 && currentAttr->Prefix != NULL && memcmp(currentAttr->Prefix, "xmlns", 5) == 0)
					{
						// Other Namespace Declaration
						currentAttr->Value[currentAttr->ValueLength] = 0;
						ILibAddEntry(current->Reserved2, currentAttr->Name, currentAttr->NameLength, currentAttr->Value);
					}
					currentAttr=currentAttr->Next;
				}
				ILibDestructXMLAttributeList(attr);
			}
		}
		current = current->Next;
	}
}


/*! \fn ILibReadInnerXML(struct ILibXMLNode *node, char **RetVal)
\brief Reads the data segment from an ILibXMLNode
\par
The data is a pointer into the original string that the XML was read from.
\param node The node to read the data from
\param[out] RetVal The data
\return The length of the data read
*/
int ILibReadInnerXML(struct ILibXMLNode *node, char **RetVal)
{
	struct ILibXMLNode *x = node;
	int length = 0;
	void *TagStack;
	*RetVal = NULL;

	//
	// Starting with the current StartElement, we use this stack to find the matching
	// EndElement, so we can figure out what we need to return
	//
	ILibCreateStack(&TagStack);
	do
	{
		if (x == NULL)
		{
			ILibClearStack(&TagStack);
			return(0);
		}
		if (x->StartTag != 0) { ILibPushStack(&TagStack, x); }

		x = x->Next;

		if (x==NULL)
		{
			ILibClearStack(&TagStack);
			return(0);
		}
	}while (!(x->StartTag==0 && ILibPopStack(&TagStack)==node && x->NameLength==node->NameLength && memcmp(x->Name,node->Name,node->NameLength)==0));

	//
	// The Reserved fields of the StartElement and EndElement are used as pointers representing
	// the data segment of the XML
	//
	length = (int)((char*)x->Reserved - (char*)node->Reserved - 1);
	if (length<0) {length=0;}
	*RetVal = (char*)node->Reserved;
	return(length);
}

/*! \fn ILibGetXMLAttributes(struct ILibXMLNode *node)
\brief Reads the attributes from an XML node
\param node The node to read the attributes from
\return A linked list of attributes
*/
struct ILibXMLAttribute *ILibGetXMLAttributes(struct ILibXMLNode *node)
{
	struct ILibXMLAttribute *RetVal = NULL;
	struct ILibXMLAttribute *current = NULL;
	char *c;
	int EndReserved = (node->EmptyTag==0)?1:2;
	int i;
	int CheckName = node->Name[node->NameLength]==0?1:0;

	struct parser_result *xml;
	struct parser_result_field *field,*field2;
	struct parser_result *temp2;
	struct parser_result *temp3;


	//
	// The reserved field is used to show where the data segments start and stop. We
	// can also use them to figure out where the attributes start and stop
	//
	c = (char*)node->Reserved - 1;
	while (*c!='<')
	{
		//
		// The Reserved field of the StartElement points to the first character after
		// the '>' of the StartElement. Just work our way backwards to find the start of
		// the StartElement
		//
		c = c -1;
	}
	c = c +1;

	//
	// Now that we isolated the string in between the '<' and the '>' we can parse the
	// string as delimited by ' ', because thats what delineates attributes. We need
	// to use ILibParseStringAdv because these attributes can be within quotation marks
	//
	//
	// But before we start, replace linefeeds and carriage-return-linefeeds to spaces
	//
	for(i=0;i<(int)((char*)node->Reserved - c -EndReserved);++i)
	{
		if (c[i]==10 || c[i]==13 || c[i]==9 || c[i]==0)
		{
			c[i]=' ';
		}
	}
	xml = ILibParseStringAdv(c,0,(int)((char*)node->Reserved - c -EndReserved)," ",1);
	field = xml->FirstResult;
	//
	// We skip the first token, because the first token, is the Element name
	//
	if (field != NULL) {field = field->NextResult;}

	//
	// Iterate through all the other tokens, as these are all attributes
	//
	while (field != NULL)
	{
		if (field->datalength > 0)
		{
			if (RetVal == NULL)
			{
				//
				// If we haven't already created an Attribute node, create it now
				//
				if ((RetVal = (struct ILibXMLAttribute*)malloc(sizeof(struct ILibXMLAttribute))) == NULL) ILIBCRITICALEXIT(254);
				RetVal->Next = NULL;
			}
			else
			{
				//
				// We already created an Attribute node, so simply create a new one, and
				// attach it on the beginning of the old one.
				//
				if ((current = (struct ILibXMLAttribute*)malloc(sizeof(struct ILibXMLAttribute))) == NULL) ILIBCRITICALEXIT(254);
				current->Next = RetVal;
				RetVal = current;
			}

			//
			// Parse each token by the ':'
			// If this results in more than one token, we can figure that the first token
			// is the namespace prefix
			//
			temp2 = ILibParseStringAdv(field->data,0,field->datalength,":",1);
			if (temp2->NumResults == 1)
			{
				//
				// This attribute has no prefix, so just parse on the '='
				// The first token is the attribute name, the other is the value
				//
				RetVal->Prefix = NULL;
				RetVal->PrefixLength = 0;
				temp3 = ILibParseStringAdv(field->data,0,field->datalength,"=",1);
				if (temp3->NumResults == 1)
				{
					//
					// There were whitespaces around the '='
					//
					field2 = field->NextResult;
					while (field2!=NULL)
					{
						if (!(field2->datalength==1 && memcmp(field2->data,"=",1)==0) && field2->datalength>0)
						{
							ILibDestructParserResults(temp3);
							temp3 = ILibParseStringAdv(field->data,0,(int)((field2->data-field->data)+field2->datalength),"=",1);
							field = field2;
							break;
						}
						field2 = field2->NextResult;
					}
				}
			}
			else
			{
				//
				// Since there is a namespace prefix, seperate that out, and parse the remainder
				// on the '=' to figure out what the attribute name and value are
				//
				RetVal->Prefix = temp2->FirstResult->data;
				RetVal->PrefixLength = temp2->FirstResult->datalength;
				temp3 = ILibParseStringAdv(field->data,RetVal->PrefixLength+1,field->datalength-RetVal->PrefixLength-1,"=",1);
				if (temp3->NumResults==1)
				{
					//
					// There were whitespaces around the '='
					//
					field2 = field->NextResult;
					while (field2!=NULL)
					{
						if (!(field2->datalength==1 && memcmp(field2->data,"=",1)==0) && field2->datalength>0)
						{
							ILibDestructParserResults(temp3);
							temp3 = ILibParseStringAdv(field->data,RetVal->PrefixLength+1,(int)((field2->data-field->data)+field2->datalength-RetVal->PrefixLength-1),"=",1);
							field = field2;
							break;
						}
						field2 = field2->NextResult;
					}
				}			
			}
			//
			// After attaching the pointers, we can free the results, as all the data
			// is a pointer into the original string. We can think of the nodes we created
			// as templates. Once done, we don't need them anymore.
			//
			ILibDestructParserResults(temp2);
			RetVal->Parent = node;
			RetVal->Name = temp3->FirstResult->data;
			RetVal->Value = temp3->LastResult->data;
			RetVal->NameLength = ILibTrimString(&(RetVal->Name),temp3->FirstResult->datalength);
			RetVal->ValueLength = ILibTrimString(&(RetVal->Value),temp3->LastResult->datalength);
			// 
			// Remove the Quote or Apostraphe if it exists
			//
			if (RetVal->ValueLength>=2 && (RetVal->Value[0]=='"' || RetVal->Value[0]=='\''))
			{
				RetVal->Value += 1;
				RetVal->ValueLength -= 2;
			}
			ILibDestructParserResults(temp3);
		}
		field = field->NextResult;
	}

	ILibDestructParserResults(xml);
	if (CheckName)
	{
		node->Name[node->NameLength]=0;
	}
	return(RetVal);
}

/*! \fn ILibParseXML(char *buffer, int offset, int length)
\brief Parses an XML string.
\par
The strings are never copied. Everything is referenced via pointers into the original buffer
\param buffer The string to parse
\param offset starting index of \a buffer
\param length Length of \a buffer
\return A tree of ILibXMLNodes, representing the XML document
*/
struct ILibXMLNode *ILibParseXML(char *buffer, int offset, int length)
{
	struct parser_result *xml;
	struct parser_result_field *field;
	struct parser_result *temp2;
	struct parser_result *temp3;
	char* TagName;
	int TagNameLength;
	int StartTag;
	int EmptyTag;
	int i;
	int wsi;

	struct ILibXMLNode *RetVal = NULL;
	struct ILibXMLNode *current = NULL;
	struct ILibXMLNode *x = NULL;

	char *NSTag;
	int NSTagLength;

	char *CommentEnd = 0;
	int CommentIndex;

	//
	// Even though "technically" the first character of an XML document must be <
	// we're going to be nice, and not enforce that
	//
	while (buffer[offset]!='<' && length>0)
	{
		++offset;
		--length;
	}

	if (length==0)
	{
		// Garbage in Garbage out :)
		if ((RetVal = (struct ILibXMLNode*)malloc(sizeof(struct ILibXMLNode))) == NULL) ILIBCRITICALEXIT(254);
		memset(RetVal,0,sizeof(struct ILibXMLNode));
		return(RetVal);
	}

	//
	// All XML Elements start with a '<' character. If we delineate the string with 
	// this character, we can go from there.
	//
	xml = ILibParseString(buffer,offset,length,"<",1);
	field = xml->FirstResult;
	while (field!=NULL)
	{
		//
		// Ignore the XML declarator
		//
		if (field->datalength !=0 && memcmp(field->data,"?",1)!=0 && (field->data > CommentEnd))
		{
			if (field->datalength>3 && memcmp(field->data,"!--",3)==0)
			{
				//
				// XML Comment, find where it ends
				//
				CommentIndex = 3;
				while (field->data+CommentIndex < buffer+offset+length && memcmp(field->data+CommentIndex,"-->",3)!=0)
				{
					++CommentIndex;
				}
				CommentEnd = field->data + CommentIndex;
				field = field->NextResult;
				continue;
			}
			else
			{
				EmptyTag = 0;
				if (memcmp(field->data,"/",1)==0)
				{
					//
					// The first character after the '<' was a '/', so we know this is the
					// EndElement
					//
					StartTag = 0;
					field->data = field->data+1;
					field->datalength -= 1;
					//
					// If we look for the '>' we can find the end of this element
					//
					temp2 = ILibParseString(field->data,0,field->datalength,">",1);
				}
				else
				{
					//
					// The first character after the '<' was not a '/' so we know this is a 
					// StartElement
					//
					StartTag = -1;
					//
					// If we look for the '>' we can find the end of this element
					//
					temp2 = ILibParseString(field->data,0,field->datalength,">",1);
					if (temp2->FirstResult->datalength>0 && temp2->FirstResult->data[temp2->FirstResult->datalength-1]=='/')
					{
						//
						// If this element ended with a '/' this is an EmptyElement
						//
						EmptyTag = -1;
					}
				}
			}
			//
			// Parsing on the ' ', we can isolate the Element name from the attributes. 
			// The first token, being the element name
			//
			wsi = ILibString_IndexOfFirstWhiteSpace(temp2->FirstResult->data,temp2->FirstResult->datalength);
			//
			// Now that we have the token that contains the element name, we need to parse on the ":"
			// because we need to figure out what the namespace qualifiers are
			//
			temp3 = ILibParseString(temp2->FirstResult->data,0,wsi!=-1?wsi:temp2->FirstResult->datalength,":",1);
			if (temp3->NumResults==1)
			{
				//
				// If there is only one token, there was no namespace prefix. 
				// The whole token is the attribute name
				//
				NSTag = NULL;
				NSTagLength = 0;
				TagName = temp3->FirstResult->data;
				TagNameLength = temp3->FirstResult->datalength;
			}
			else
			{
				//
				// The first token is the namespace prefix, the second is the attribute name
				//
				NSTag = temp3->FirstResult->data;
				NSTagLength = temp3->FirstResult->datalength;
				TagName = temp3->FirstResult->NextResult->data; // Note: Klocwork reports that this may be NULL, but that is not possible, becuase NumResults is > 1 in this block
				TagNameLength = temp3->FirstResult->NextResult->datalength;
			}
			ILibDestructParserResults(temp3);

			//
			// Iterate through the tag name, to figure out what the exact length is, as
			// well as check to see if its an empty element
			//
			for(i=0;i<TagNameLength;++i)
			{
				if ( (TagName[i]==' ')||(TagName[i]=='/')||(TagName[i]=='>')||(TagName[i]=='\t')||(TagName[i]=='\r')||(TagName[i]=='\n') )
				{
					if (i!=0)
					{
						if (TagName[i]=='/')
						{
							EmptyTag = -1;
						}
						TagNameLength = i;
						break;
					}
				}
			}

			if (TagNameLength!=0)
			{
				//
				// Instantiate a new ILibXMLNode for this element
				//
				if ((x = (struct ILibXMLNode*)malloc(sizeof(struct ILibXMLNode))) == NULL) ILIBCRITICALEXIT(254);
				memset(x,0,sizeof(struct ILibXMLNode));
				x->Name = TagName;
				x->NameLength = TagNameLength;
				x->StartTag = StartTag;
				x->NSTag = NSTag;
				x->NSLength = NSTagLength;				

				if (StartTag==0)
				{
					//
					// The Reserved field of StartElements point to te first character before
					// the '<'.
					//
					x->Reserved = field->data;
					do
					{
						x->Reserved = ((char*)x->Reserved) - 1;
					}while (*((char*)x->Reserved)=='<');
				}
				else
				{
					//
					// The Reserved field of EndElements point to the end of the element
					//
					x->Reserved = temp2->LastResult->data;
				}

				if (RetVal==NULL)
				{
					RetVal = x;
				}
				else
				{
					current->Next = x;
				}
				current = x;
				if (EmptyTag!=0)
				{
					//
					// If this was an empty element, we need to create a bogus EndElement, 
					// just so the tree is consistent. No point in introducing unnecessary complexity
					//
					if ((x = (struct ILibXMLNode*)malloc(sizeof(struct ILibXMLNode))) == NULL) ILIBCRITICALEXIT(254);
					memset(x,0,sizeof(struct ILibXMLNode));
					x->Name = TagName;
					x->NameLength = TagNameLength;
					x->NSTag = NSTag;
					x->NSLength = NSTagLength;

					x->Reserved = current->Reserved;
					current->EmptyTag = -1;
					current->Next = x;
					current = x;
				}
			}

			ILibDestructParserResults(temp2);
		}
		field = field->NextResult;
	}

	ILibDestructParserResults(xml);
	return(RetVal);
}

/*! \fn ILibQueue_Create()
\brief Create an empty Queue
\return An empty queue
*/
ILibQueue ILibQueue_Create()
{
	return((ILibQueue)ILibLinkedList_Create());
}

/*! \fn ILibQueue_Lock(void *q)
\brief Locks a queue
\param q The queue to lock
*/
void ILibQueue_Lock(ILibQueue q)
{
	ILibLinkedList_Lock((ILibLinkedList)q);
}

/*! \fn ILibQueue_UnLock(void *q)
\brief Unlocks a queue
\param q The queue to unlock
*/
void ILibQueue_UnLock(ILibQueue q)
{
	ILibLinkedList_UnLock((ILibLinkedList)q);
}

/*! \fn ILibQueue_Destroy(void *q)
\brief Frees the resources associated with a queue
\param q The queue to free
*/
void ILibQueue_Destroy(ILibQueue q)
{
	ILibLinkedList_Destroy((ILibLinkedList)q);
}

/*! \fn ILibQueue_IsEmpty(void *q)
\brief Checks to see if a queue is empty
\param q The queue to check
\return zero value if not empty, non-zero if empty
*/
int ILibQueue_IsEmpty(ILibQueue q)
{
	return(ILibLinkedList_GetNode_Head((ILibLinkedList)q)==NULL?1:0);
}

/*! \fn ILibQueue_EnQueue(void *q, void *data)
\brief Add an item to the queue
\param q The queue to add to
\param data The data to add to the queue
*/
void ILibQueue_EnQueue(ILibQueue q, void *data)
{
	ILibLinkedList_AddTail((ILibLinkedList)q,data);
}
/*! \fn ILibQueue_DeQueue(void *q)
\brief Removes an item from the queue
\param q The queue to pop the item from
\return The item popped off the queue. NULL if empty
*/
void *ILibQueue_DeQueue(ILibQueue q)
{
	void *RetVal = NULL;
	void *node;

	node = ILibLinkedList_GetNode_Head((ILibLinkedList)q);
	if (node!=NULL)
	{
		RetVal = ILibLinkedList_GetDataFromNode(node);
		ILibLinkedList_Remove(node);
	}
	return(RetVal);
}

/*! \fn ILibQueue_PeekQueue(void *q)
\brief Peeks at an item from the queue
\param q The queue to peek an item from
\return The item from the queue. NULL if empty
*/
void *ILibQueue_PeekQueue(ILibQueue q)
{
	void *RetVal = NULL;
	void *node;

	node = ILibLinkedList_GetNode_Head((ILibLinkedList)q);
	if (node!=NULL)
	{
		RetVal = ILibLinkedList_GetDataFromNode(node);
	}
	return(RetVal);
}

/*! \fn ILibQueue_GetCount(void *q)
\brief Returns the number of items in the queue.
\param q The queue to query
\return The item count in the queue.
*/
long ILibQueue_GetCount(ILibQueue q)
{
	return ILibLinkedList_GetCount((ILibLinkedList)q);
}

/*! \fn ILibCreateStack(void **TheStack)
\brief Creates an empty Stack
\par
This module uses a void* that is preinitialized to NULL, eg:<br>
<i>
void *stack = NULL;<br>
ILibCreateStack(&stack);<br>
</i>
\param TheStack A void* to use for the stack. Simply pass in a void* by reference
*/
void ILibCreateStack(void **TheStack)
{
	*TheStack = NULL;
}

/*! \fn ILibPushStack(void **TheStack, void *data)
\brief Push an item onto the stack
\param TheStack The stack to push to
\param data The data to push onto the stack
*/
void ILibPushStack(void **TheStack, void *data)
{
	struct ILibStackNode *RetVal;
	if ((RetVal = (struct ILibStackNode*)malloc(sizeof(struct ILibStackNode))) == NULL) ILIBCRITICALEXIT(254);
	RetVal->Data = data;
	RetVal->Next = *TheStack;
	*TheStack = RetVal;
}

/*! \fn ILibPopStack(void **TheStack)
\brief Pop an item from the stack
\param TheStack The stack to pop from
\return The item that was popped from the stack
*/
void *ILibPopStack(void **TheStack)
{
	void *RetVal = NULL;
	void *Temp;
	if (*TheStack!=NULL)
	{
		RetVal = ((struct ILibStackNode*)*TheStack)->Data;
		Temp = *TheStack;
		*TheStack = ((struct ILibStackNode*)*TheStack)->Next;
		free(Temp);
	}
	return(RetVal);
}

/*! \fn ILibPeekStack(void **TheStack)
\brief Peek at an item from the stack
\param TheStack The stack to peek from
\return The item that is currently on the top of the stack
*/
void *ILibPeekStack(void **TheStack)
{
	void *RetVal = NULL;
	if (*TheStack!=NULL)
	{
		RetVal = ((struct ILibStackNode*)*TheStack)->Data;
	}
	return(RetVal);
}

/*! \fn ILibClearStack(void **TheStack)
\brief Clears all the items from the stack
\param TheStack The stack to clear
*/
void ILibClearStack(void **TheStack)
{
	void *Temp = *TheStack;
	do
	{
		ILibPopStack(&Temp);
	}while (Temp!=NULL);
	*TheStack = NULL;
}

/*! \fn ILibHashTree_Lock(void *hashtree)
\brief Locks a HashTree
\param hashtree The HashTree to lock
*/
void ILibHashTree_Lock(void *hashtree)
{
	struct HashNode_Root *r = (struct HashNode_Root*)hashtree;
	sem_wait(&(r->LOCK));
}

/*! \fn ILibHashTree_UnLock(void *hashtree)
\brief Unlocks a HashTree
\param hashtree The HashTree to unlock
*/
void ILibHashTree_UnLock(void *hashtree)
{
	struct HashNode_Root *r = (struct HashNode_Root*)hashtree;
	sem_post(&(r->LOCK));
}

/*! \fn ILibDestroyHashTree(void *tree)
\brief Frees resources associated with a HashTree
\param tree The HashTree to free
*/
void ILibDestroyHashTree(void *tree)
{
	struct HashNode_Root *r = (struct HashNode_Root*)tree;
	struct HashNode *c = r->Root;
	struct HashNode *n;

	sem_destroy(&(r->LOCK));
	while (c != NULL)
	{
		//
		// Iterate through each node, and free all the resources
		//
		n = c->Next;
		if (c->KeyValue!=NULL) free(c->KeyValue);
		free(c);
		c = n;
	}
	free(r);
}

/*! \fn ILibHashTree_GetEnumerator(void *tree)
\brief Returns an Enumerator for a HashTree
\par
Functionally identicle to an IDictionaryEnumerator in .NET
\param tree The HashTree to get an enumerator for
\return An enumerator
*/
void *ILibHashTree_GetEnumerator(void *tree)
{
	//
	// The enumerator is basically a state machine that keeps track of which node we are at
	// in the tree. So initialize it to the root.
	//
	struct HashNodeEnumerator *en;
	if ((en = (struct HashNodeEnumerator*)malloc(sizeof(struct HashNodeEnumerator))) == NULL) ILIBCRITICALEXIT(254);
	en->node = ((struct HashNode_Root*)tree)->Root;
	return en;
}

/*! \fn ILibHashTree_DestroyEnumerator(void *tree_enumerator)
\brief Frees resources associated with an Enumerator created by ILibHashTree_GetEnumerator
\param tree_enumerator The enumerator to free
*/
void ILibHashTree_DestroyEnumerator(void *tree_enumerator)
{
	//
	// The enumerator just contains a pointer, so we can just free the enumerator
	//
	free(tree_enumerator);
}

/*! \fn ILibHashTree_MoveNext(void *tree_enumerator)
\brief Advances an enumerator to the next item
\param tree_enumerator The enumerator to advance
\return A zero value if successful, nonzero if no more items
*/
int ILibHashTree_MoveNext(void *tree_enumerator)
{
	struct HashNodeEnumerator *en = (struct HashNodeEnumerator*)tree_enumerator;
	if (en->node != NULL)
	{
		//
		// Advance the enumerator to point to the next node. If there is a node
		// return 0, else return 1
		//
		en->node = en->node->Next;
		return(en->node != NULL?0:1);
	}
	else
	{
		//
		// There are no more nodes, so just return 1
		//
		return 1;
	}
}

/*! \fn ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data)
\brief Reads from the current item of an enumerator
\param tree_enumerator The enumerator to read from
\param[out] key The key of the current item
\param[out] keyLength The length of the key of the current item
\param[out] data The data of the current item
*/
void ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data)
{
	struct HashNodeEnumerator *en = (struct HashNodeEnumerator*)tree_enumerator;

	//
	// All we do, is just assign the pointers.
	//
	if (key != NULL) {*key = en->node->KeyValue;}
	if (keyLength != NULL) {*keyLength = en->node->KeyLength;}
	if (data != NULL) {*data = en->node->Data;}
}
/*! \fn void ILibHashTree_GetValueEx(void *tree_enumerator, char **key, int *keyLength, void **data, int *dataEx)
\brief Reads from the current item of an enumerator
\param tree_enumerator The enumerator to read from
\param[out] key The key of the current item
\param[out] keyLength The length of the key of the current item
\param[out] data The data of the current item
\param[out] dataEx The extended data of the current item
*/
void ILibHashTree_GetValueEx(void *tree_enumerator, char **key, int *keyLength, void **data, int *dataEx)
{
	struct HashNodeEnumerator *en = (struct HashNodeEnumerator*)tree_enumerator;

	//
	// All we do, is just assign the pointers.
	//
	if (key != NULL) {*key = en->node->KeyValue;}
	if (keyLength != NULL) {*keyLength = en->node->KeyLength;}
	if (data != NULL) {*data = en->node->Data;}
	if (dataEx != NULL) {*dataEx = en->node->DataEx;}
}

/*! \fn ILibInitHashTree()
\brief Creates an empty ILibHashTree, whose keys are <B>case sensitive</B>.
\return An empty ILibHashTree
*/
void* ILibInitHashTree()
{
	struct HashNode_Root *Root;
	struct HashNode *RetVal;
	if ((Root = (struct  HashNode_Root*)malloc(sizeof(struct HashNode_Root))) == NULL) ILIBCRITICALEXIT(254);
	if ((RetVal = (struct HashNode*)malloc(sizeof(struct HashNode))) == NULL) ILIBCRITICALEXIT(254);
	memset(RetVal, 0, sizeof(struct HashNode));
	memset(Root, 0, sizeof(struct HashNode_Root));
	Root->Root = RetVal;
	sem_init(&(Root->LOCK), 0, 1);
	return(Root);
}
/*! \fn void* ILibInitHashTree_CaseInSensitive()
\brief Creates an empty ILibHashTree, whose keys are <B>case insensitive</B>.
\return An empty ILibHashTree
*/
void* ILibInitHashTree_CaseInSensitive()
{
	struct HashNode_Root *Root = (struct  HashNode_Root*)ILibInitHashTree();
	Root->CaseInSensitive = 1;
	return(Root);
}


void ILibToUpper(const char *in, int inLength, char *out)
{
	int i;

	for(i = 0; i < inLength; ++i)
	{
		if (in[i]>=97 && in[i]<=122)
		{
			//LOWER
			out[i] = in[i]-32;
		}
		else
		{
			//CAP
			out[i] = in[i];
		}
	}
}
void ILibToLower(const char *in, int inLength, char *out)
{
	int i;

	for(i = 0; i < inLength; ++i)
	{
		if (in[i] >= 65 && in[i] <= 90)
		{
			//CAP
			out[i] = in[i]+32;
		}
		else
		{
			//LOWER
			out[i] = in[i];
		}
	}
}

int ILibGetHashValueEx(char *key, int keylength, int caseInSensitiveText)
{
	int HashValue=0;
	char TempValue[4];

	if (keylength <= 4)
	{
		//
		// If the key length is <= 4, the hash is just the key expressed as an integer
		//
		memset(TempValue, 0, 4);
		if (caseInSensitiveText==0)
		{
			memcpy_s(TempValue, sizeof(TempValue), key, keylength);
		}
		else
		{
			ILibToLower(key, keylength, TempValue);
		}
		MEMCHECK(assert(keylength <= 4);)

		HashValue = *((int*)TempValue);
	}
	else
	{
		//
		// If the key length is >4, the hash is the first 4 bytes XOR with the last 4
		//

		if (caseInSensitiveText == 0)
		{
			memcpy_s(TempValue, sizeof(TempValue), key, 4);
		}
		else
		{
			ILibToLower(key, 4, TempValue);
		}
		HashValue = *((int*)TempValue);
		if (caseInSensitiveText==0)
		{
			memcpy_s(TempValue, sizeof(TempValue), (char*)key+(keylength-4),4);
		}
		else
		{
			ILibToLower((char*)key+(keylength-4),4,TempValue);
		}
		HashValue = HashValue^(*((int*)TempValue));


		//
		// If the key length is >= 10, the hash is also XOR with the middle 4 bytes
		//
		if (keylength>=10)
		{
			if (caseInSensitiveText==0)
			{
				memcpy_s(TempValue, sizeof(TempValue), (char*)key+(keylength/2),4);
			}
			else
			{
				ILibToLower((char*)key+(keylength/2),4,TempValue);
			}
			HashValue = HashValue^(*((int*)TempValue));
		}
	}
	return(HashValue);
}

/*! \fn ILibGetHashValue(char *key, int keylength)
\brief Calculates a numeric Hash from a given string
\par
Used by ILibHashTree methods
\param key The string to hash
\param keylength The length of the string to hash
\return A hash value
*/
int ILibGetHashValue(char *key, int keylength)
{
	return(ILibGetHashValueEx(key,keylength,0));
}



//
// Determines if a key entry exists in a HashTree, and creates it if requested
//
struct HashNode* ILibFindEntry(void *hashtree, void *key, int keylength, int create)
{
	struct HashNode_Root *root = (struct HashNode_Root*)hashtree;
	struct HashNode *current = root->Root;
	int HashValue = ILibGetHashValueEx(key, keylength, root->CaseInSensitive);
	int done = 0;

	if (keylength == 0){return(NULL);}

	//
	// Iterate through our tree to see if we can find this key entry
	//
	while (done == 0)
	{
		//
		// Integer compares are very fast, this will weed out most non-matches
		//
		if (current->KeyHash == HashValue)
		{
			//
			// Verify this is really a match
			//
			if (root->CaseInSensitive == 0)
			{
				if (current->KeyLength == keylength && memcmp(current->KeyValue, key, keylength) == 0)
				{
					return current;
				}
			}
			else
			{
				if (current->KeyLength == keylength && strncasecmp(current->KeyValue, key, keylength) == 0)
				{
					return current;
				}
			}
		}

		if (current->Next != NULL)
		{
			current = current->Next;
		}
		else if (create != 0)
		{
			//
			// If there is no match, and the create flag is set, we need to create an entry
			//
			if ((current->Next = (struct HashNode*)malloc(sizeof(struct HashNode))) == NULL) ILIBCRITICALEXIT(254);
			memset(current->Next,0,sizeof(struct HashNode));
			current->Next->Prev = current;
			current->Next->KeyHash = HashValue;
			if ((current->Next->KeyValue = (void*)malloc(keylength + 1)) == NULL) ILIBCRITICALEXIT(254);
			memcpy_s(current->Next->KeyValue, keylength + 1, key ,keylength);
			current->Next->KeyValue[keylength] = 0; 
			current->Next->KeyLength = keylength;
			return(current->Next);
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}

/*! \fn ILibHasEntry(void *hashtree, char* key, int keylength)
\brief Determines if a key entry exists in a HashTree
\param hashtree The HashTree to operate on
\param key The key
\param keylength The length of the key
\return 0 if does not exist, nonzero otherwise
*/
int ILibHasEntry(void *hashtree, char* key, int keylength)
{
	//
	// This can be duplicated by calling Find entry, but setting the create flag to false
	//
	return(ILibFindEntry(hashtree, key, keylength, 0) != NULL?1:0);
}

/*! \fn ILibAddEntry(void* hashtree, char* key, int keylength, void *value)
\brief Adds an item to the HashTree
\param hashtree The HashTree to operate on
\param key The key
\param keylength The length of the key
\param value The data to add into the HashTree
*/
void ILibAddEntry(void* hashtree, char* key, int keylength, void *value)
{
	//
	// This can be duplicated by calling FindEntry, and setting create to true
	//
	struct HashNode* n = ILibFindEntry(hashtree, key, keylength, 1);
	if (n != NULL) n->Data = value;
}
/*! \fn ILibAddEntryEx(void* hashtree, char* key, int keylength, void *value, int valueLength)
\brief Adds an extended item to the HashTree
\param hashtree The HashTree to operate on
\param key The key
\param keylength The length of the key
\param value The data to add into the HashTree
\param valueEx An optional int value
*/
void ILibAddEntryEx(void* hashtree, char* key, int keylength, void *value, int valueEx)
{
	//
	// This can be duplicated by calling FindEntry, and setting create to true
	//
	struct HashNode* n = ILibFindEntry(hashtree, key, keylength, 1);
	if (n != NULL)
	{
		n->Data = value;
		n->DataEx = valueEx;
	}
}


/*! \fn ILibGetEntry(void *hashtree, char* key, int keylength)
\brief Gets an item from a HashTree
\param hashtree The HashTree to operate on
\param key The key
\param keylength The length of the key
\return The data in the HashTree. NULL if key does not exist
*/
void* ILibGetEntry(void *hashtree, char* key, int keylength)
{
	//
	// This can be duplicated by calling FindEntry and setting create to false.
	// If a match is found, just return the data
	//
	struct HashNode* n = ILibFindEntry(hashtree, key, keylength, 0);
	if (n == NULL)
	{
		return(NULL);
	}
	else
	{
		return(n->Data);
	}
}
/*! \fn void ILibGetEntryEx(void *hashtree, char *key, int keyLength, void **value, int* valueEx)
\brief Gets an extended item from a HashTree
\param hashtree The HashTree to operate on
\param key The key
\param keyLength The length of the key
\param[out] value The data in the HashTree. NULL if key does not exist
\param[out] valueEx The extended data in the HashTree.
*/
ILibExportMethod void ILibGetEntryEx(void *hashtree, char *key, int keyLength, void **value, int* valueEx)
{
	//
	// This can be duplicated by calling FindEntry and setting create to false.
	// If a match is found, just return the data
	//
	struct HashNode* n = ILibFindEntry(hashtree, key, keyLength, 0);
	if (n == NULL)
	{
		*value = NULL;
		*valueEx = 0;
	}
	else
	{
		*value = n->Data;
		*valueEx = n->DataEx;
	}
}

/*! \fn ILibDeleteEntry(void *hashtree, char* key, int keylength)
\brief Deletes a keyed item from the HashTree
\param hashtree The HashTree to operate on
\param key The key
\param keylength The length of the key
*/
void ILibDeleteEntry(void *hashtree, char* key, int keylength)
{
	//
	// First find the entry
	//
	struct HashNode* n = ILibFindEntry(hashtree,key,keylength,0);
	if (n != NULL)
	{
		//
		// Then remove it from the tree
		//
		n->Prev->Next = n->Next;
		if (n->Next != NULL) n->Next->Prev = n->Prev;
		free(n->KeyValue);
		free(n);
	}
}

/*! \fn ILibGetLong(char *TestValue, int TestValueLength, long* NumericValue)
\brief Reads a long value from a string, in a validating fashion
\param TestValue The string to read from
\param TestValueLength The length of the string
\param NumericValue The long value extracted from the string
\return 0 if succesful, nonzero if there was an error
*/
int ILibGetLong(char *TestValue, int TestValueLength, long* NumericValue)
{
	char* StopString;
	char* TempBuffer;

	if ((TempBuffer = (char*)malloc(1+sizeof(char)*TestValueLength)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(TempBuffer, 1 + sizeof(char)*TestValueLength, TestValue, TestValueLength);
	TempBuffer[TestValueLength] = '\0';
	*NumericValue = strtol(TempBuffer,&StopString,10);
	if (*StopString != '\0')
	{
		//
		// If strtol stopped somewhere other than the end, there was an error
		//
		free(TempBuffer);
		return(-1);
	}
	else
	{
		//
		// Now just check errno to see if there was an error reported
		//
		free(TempBuffer);
		if (errno!=ERANGE) return(0); else return(-1);
	}
}

/*! \fn int ILibGetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue)
\brief Reads an unsigned long value from a string, in a validating fashion
\param TestValue The string to read from
\param TestValueLength The length of the string
\param NumericValue The long value extracted from the string
\return 0 if succesful, nonzero if there was an error
*/
int ILibGetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue)
{
	char* StopString;
	char* TempBuffer;

	if ((TempBuffer = (char*)malloc(1 + sizeof(char)*TestValueLength)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(TempBuffer, 1 + sizeof(char)*TestValueLength, (char*)TestValue, TestValueLength);
	TempBuffer[TestValueLength] = '\0';
	*NumericValue = strtoul(TempBuffer, &StopString, 10);

	if (*StopString!='\0')
	{
		free(TempBuffer);
		return(-1);
	}
	else
	{
		free(TempBuffer);
#ifdef _WIN32_WCE
		// Not Supported on PPC
		return(0);
#else
		if (errno!=ERANGE)
		{
			if (memcmp(TestValue,"-",1) == 0) return(-1);
			return(0);
		}
		else
		{
			return(-1);
		}
#endif
	}
}


//
// Determines if a buffer offset is a delimiter
//
int ILibIsDelimiter (const char* buffer, int offset, int buffersize, const char* Delimiter, int DelimiterLength)
{
	//
	// For simplicity sake, we'll assume a match unless proven otherwise
	//
	int i=0;
	int RetVal = 1;
	if (DelimiterLength>buffersize)
	{
		//
		// If the offset plus delimiter length is greater than the buffersize
		// There can't possible be a match, so don't bother looking
		//
		return(0);
	}

	for(i=0;i<DelimiterLength;++i)
	{
		if (buffer[offset+i]!=Delimiter[i])
		{
			//
			// Uh oh! Can't possibly be a match now!
			//
			RetVal = 0;
			break;
		}
	}
	return(RetVal);
}

/*! \fn ILibParseStringAdv(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength)
\brief Parses a string into a linked list of tokens.
\par
Differs from \a ILibParseString, in that this method ignores characters contained within
quotation marks, whereas \a ILibParseString does not.
\param buffer The buffer to parse
\param offset The offset of the buffer to start parsing
\param length The length of the buffer to parse
\param Delimiter The delimiter
\param DelimiterLength The length of the delimiter
\return A list of tokens
*/
struct parser_result* ILibParseStringAdv (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength)
{
	struct parser_result* RetVal;
	int i=0;	
	char* Token = NULL;
	int TokenLength = 0;
	struct parser_result_field *p_resultfield;
	int Ignore = 0;
	char StringDelimiter=0;

	if ((RetVal = (struct parser_result*)malloc(sizeof(struct parser_result))) == NULL) ILIBCRITICALEXIT(254);
	RetVal->FirstResult = NULL;
	RetVal->NumResults = 0;

	//
	// By default we will always return at least one token, which will be the
	// entire string if the delimiter is not found.
	//
	// Iterate through the string to find delimiters
	//
	Token = buffer + offset;
	for(i = offset;i < (length+offset);++i)
	{
		if (StringDelimiter == 0)
		{
			if (buffer[i] == '"') 
			{
				//
				// Ignore everything inside double quotes
				//
				StringDelimiter = '"';
				Ignore = 1;
			}
			else
			{
				if (buffer[i] == '\'')
				{
					//
					// Ignore everything inside single quotes
					//
					StringDelimiter = '\'';
					Ignore = 1;
				}
			}
		}
		else
		{
			//
			// Once we isolated everything inside double or single quotes, we can get
			// on with the real parsing
			//
			if (buffer[i] == StringDelimiter)
			{
				Ignore = ((Ignore == 0)?1:0);
			}
		}
		if (Ignore == 0 && ILibIsDelimiter(buffer, i, length, Delimiter, DelimiterLength))
		{
			//
			// We found a delimiter in the string
			//
			if ((p_resultfield = (struct parser_result_field*)malloc(sizeof(struct parser_result_field))) == NULL) ILIBCRITICALEXIT(253);
			p_resultfield->data = Token;
			p_resultfield->datalength = TokenLength;
			p_resultfield->NextResult = NULL;
			if (RetVal->FirstResult != NULL)
			{
				RetVal->LastResult->NextResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}
			else
			{
				RetVal->FirstResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}

			//
			// After we populate the values, we advance the token to after the delimiter
			// to prep for the next token
			//
			++RetVal->NumResults;
			i = i + DelimiterLength -1;
			Token = Token + TokenLength + DelimiterLength;
			TokenLength = 0;	
		}
		else
		{
			//
			// No match yet, so just increment this counter
			//
			++TokenLength;
		}
	}

	//
	// Create a result for the last token, since it won't be caught in the above loop
	// because if there are no more delimiters, than the entire last portion of the string since the 
	// last delimiter is the token
	//
	if ((p_resultfield = (struct parser_result_field*)malloc(sizeof(struct parser_result_field))) == NULL) ILIBCRITICALEXIT(254);
	p_resultfield->data = Token;
	p_resultfield->datalength = TokenLength;
	p_resultfield->NextResult = NULL;
	if (RetVal->FirstResult != NULL)
	{
		RetVal->LastResult->NextResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}
	else
	{
		RetVal->FirstResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}	
	++RetVal->NumResults;

	return(RetVal);
}

/*! \fn ILibTrimString(char **theString, int length)
\brief Trims leading and trailing whitespace characters
\param theString The string to trim
\param length Length of \a theString
\return Length of the trimmed string
*/
int ILibTrimString(char **theString, int length)
{
	while (length > 0 && ((*theString)[0] == 9 || (*theString)[0] == 32)) { length--; (*theString)++; }					// Remove any blank chars at the start
	while (length > 0 && ((*theString)[length - 1] == 9 || (*theString)[length - 1] == 32)) { length--; }				// Remove any blank chars at the end
	return length;
}

/*! \fn ILibParseString(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength)
\brief Parses a string into a linked list of tokens.
\par
Differs from \a ILibParseStringAdv, in that this method does not ignore characters contained within
quotation marks, whereas \a ILibParseStringAdv does.
\param buffer The buffer to parse
\param offset The offset of the buffer to start parsing
\param length The length of the buffer to parse
\param Delimiter The delimiter
\param DelimiterLength The length of the delimiter
\return A list of tokens
*/
struct parser_result* ILibParseString(char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength)
{
	int i = 0;
	char* Token = NULL;
	int TokenLength = 0;
	struct parser_result* RetVal;
	struct parser_result_field *p_resultfield;

	if ((RetVal = (struct parser_result*)malloc(sizeof(struct parser_result))) == NULL) ILIBCRITICALEXIT(254);
	RetVal->FirstResult = NULL;
	RetVal->NumResults = 0;

	//
	// By default we will always return at least one token, which will be the
	// entire string if the delimiter is not found.
	//
	// Iterate through the string to find delimiters
	//
	Token = buffer + offset;
	for(i = offset; i < length; ++i)
	{
		if (ILibIsDelimiter(buffer, i, length, Delimiter, DelimiterLength))
		{
			//
			// We found a delimiter in the string
			//
			if ((p_resultfield = (struct parser_result_field*)malloc(sizeof(struct parser_result_field))) == NULL) ILIBCRITICALEXIT(254);
			p_resultfield->data = Token;
			p_resultfield->datalength = TokenLength;
			p_resultfield->NextResult = NULL;
			if (RetVal->FirstResult != NULL)
			{
				RetVal->LastResult->NextResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}
			else
			{
				RetVal->FirstResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}

			//
			// After we populate the values, we advance the token to after the delimiter
			// to prep for the next token
			//
			++RetVal->NumResults;
			i = i + DelimiterLength -1;
			Token = Token + TokenLength + DelimiterLength;
			TokenLength = 0;	
		}
		else
		{
			//
			// No match yet, so just increment this counter
			//
			++TokenLength;
		}
	}

	//
	// Create a result for the last token, since it won't be caught in the above loop
	// because if there are no more delimiters, than the entire last portion of the string since the 
	// last delimiter is the token
	//
	if ((p_resultfield = (struct parser_result_field*)malloc(sizeof(struct parser_result_field))) == NULL) ILIBCRITICALEXIT(254);
	p_resultfield->data = Token;
	p_resultfield->datalength = TokenLength;
	p_resultfield->NextResult = NULL;
	if (RetVal->FirstResult != NULL)
	{
		RetVal->LastResult->NextResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}
	else
	{
		RetVal->FirstResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}	
	++RetVal->NumResults;

	return(RetVal);
}

parser_result_field* ILibParseString_GetResultIndex(parser_result* r, int index)
{
	int i;
	parser_result_field *retVal = r->FirstResult;

	for (i = 0; i < index - 1; ++i)
	{
		retVal = retVal != NULL ? retVal->NextResult : NULL;
	}
	return(retVal);
}

/*! \fn ILibDestructParserResults(struct parser_result *result)
\brief Frees resources associated with the list of tokens returned from ILibParseString and ILibParseStringAdv.
\param result The list of tokens to free
*/
void ILibDestructParserResults(struct parser_result *result)
{
	//
	// All of these nodes only contain pointers
	// so we just need to iterate through all the nodes and free them
	//
	struct parser_result_field *node = result->FirstResult;
	struct parser_result_field *temp;

	while (node != NULL)
	{
		temp = node->NextResult;
		free(node);
		node = temp;
	}
	free(result);
}

/*! \fn ILibDestructPacket(struct packetheader *packet)
\brief Frees resources associated with a Packet that was created either by \a ILibCreateEmptyPacket or \a ILibParsePacket
\param packet The packet to free
*/
void ILibDestructPacket(struct packetheader *packet)
{
	struct packetheader_field_node *node = packet->FirstField;	 // TODO: *********** CRASH ON EXIT SOMETIMES OCCURS HERE
	struct packetheader_field_node *nextnode;

	//
	// Iterate through all the headers
	//
	while (node != NULL)
	{
		nextnode = node->NextField;
		if (node->UserAllocStrings  != 0)
		{
			//
			// If the user allocated the string, then we need to free it.
			// Otherwise these are just pointers into others strings, in which
			// case we don't want to free them
			//
			free(node->Field);
			free(node->FieldData);
		}
		free(node);
		node = nextnode;
	}
	if (packet->UserAllocStrings != 0)
	{
		//
		// If this flag was set, it means the used ILibCreateEmptyPacket,
		// and set these fields manually, which means the string was copied.
		// In which case, we need to free the strings
		//
		if (packet->StatusData != NULL) {free(packet->StatusData);}
		if (packet->Directive != NULL) {free(packet->Directive);}
		if (packet->Reserved == NULL && packet->DirectiveObj != NULL) {free(packet->DirectiveObj);}
		if (packet->Reserved != NULL) {free(packet->Reserved);}
		if (packet->Body != NULL) free(packet->Body);
	}
	if (packet->UserAllocVersion != 0)
	{
		free(packet->Version);
	}
	ILibDestroyHashTree(packet->HeaderTable);
	free(packet);
}

/*! \fn ILibHTTPEscape(char* outdata, const char* data)
\brief Escapes a string according to HTTP Specifications.
\par
The string you would want to escape would typically be the string used in the Path portion
of an HTTP request. eg:<br>
GET foo/bar.txt HTTP/1.1<br>
<br>
\b Note: It should be noted that the output buffer needs to be allocated prior to calling this method.
The required space can be determined by calling \a ILibHTTPEscapeLength.
\param outdata The escaped string
\param[in,out] data The string to escape
\return The length of the escaped string
*/
int ILibHTTPEscapeEx(char* outdata, const char* data, size_t dataLen)
{
	int i = 0;
	int x = 0;
	char hex[4];

	while (data[x] != 0 && x < (int)dataLen)
	{
		if ( (data[x]>=63 && data[x]<=90) || (data[x]>=97 && data[x]<=122) || (data[x]>=47 && data[x]<=57) \
			|| data[x]==59 || data[x]==47 || data[x]==63 || data[x]==58 || data[x]==64 || data[x]==61 \
			|| data[x]==43 || data[x]==36 || data[x]==45 || data[x]==95 || data[x]==46 || data[x]==42)
		{
			//
			// These are all the allowed values for HTTP. If it's one of these characters, we're ok
			//
			outdata[i] = data[x];
			++i;
		}
		else
		{
			//
			// If it wasn't one of these characters, then we need to escape it
			//
			sprintf_s(hex, 4, "%02X", (unsigned char)data[x]);
			outdata[i] = '%';
			outdata[i+1] = hex[0];
			outdata[i+2] = hex[1];
			i+=3;
		}
		++x;
	}
	outdata[i] = 0;
	return (i+1);
}

/*! \fn ILibHTTPEscapeLength(const char* data)
\brief Determines the buffer space required to HTTP escape a particular string.
\param data Calculates the length requirements as if \a data was escaped
\return The minimum required length
*/
int ILibHTTPEscapeLengthEx(const char* data, size_t dataLen)
{
	int i=0;
	int x=0;
	while (data[x] != 0 && i < (int)dataLen)
	{
		if ((data[x] >= 63 && data[x] <= 90) || (data[x] >= 97 && data[x] <= 122) || (data[x] >= 47 && data[x] <= 57) \
			|| data[x] == 59 || data[x] == 47 || data[x] == 63 || data[x] == 58 || data[x] == 64 || data[x] == 61 \
			|| data[x] == 43 || data[x] == 36 || data[x] == 45 || data[x] == 95 || data[x] == 46 || data[x] == 42)
		{
			// No need to escape
			++i;
		}
		else
		{
			// Need to escape
			i += 3;
		}
		++x;
	}
	return(i+1);
}

/*! \fn ILibInPlaceHTTPUnEscape(char* data)
\brief Unescapes a given string according to HTTP encoding rules
\par
The escaped representation of a string is always longer than the unescaped version
so this method will overwrite the escaped string, with the unescaped result.
\param[in,out] data The buffer to unescape
\return The length of the unescaped string
*/
int ILibInPlaceHTTPUnEscapeEx(char* data, int length)
{
	char hex[3];
	char *stp;
	int src_x=0;
	int dst_x=0;

	hex[2]=0;

	while (src_x<length)
	{
		if (strncmp(data+src_x,"%",1)==0)
		{
			//
			// Since we encountered a '%' we know this is an escaped character
			//
			hex[0] = data[src_x+1];
			hex[1] = data[src_x+2];
			data[dst_x] = (char)strtol(hex,&stp,16);
			dst_x += 1;
			src_x += 3;
		}
		else if (src_x!=dst_x)
		{
			//
			// This doesn't need to be unescaped. If we didn't unescape anything previously
			// there is no need to copy the string either
			//
			data[dst_x] = data[src_x];
			src_x += 1;
			dst_x += 1;
		}
		else
		{
			//
			// This doesn't need to be unescaped, however we need to copy the string
			//
			src_x += 1;
			dst_x += 1;
		}
	}
	return(dst_x);
}

//! Parses a string into an packet structure.
//! None of the strings are copied, so the lifetime of all the values are bound
//! to the lifetime of the underlying string that is parsed.
/*!
\param buffer The buffer to parse
\param offset The offset of the buffer to start parsing
\param length The length of the buffer to parse
\return packetheader structure
*/
struct packetheader* ILibParsePacketHeader(char* buffer, int offset, int length)
{
	struct packetheader *RetVal;
	struct parser_result *_packet;
	struct parser_result *p;
	struct parser_result *StartLine;
	struct parser_result_field *HeaderLine;
	struct parser_result_field *f;
	char *tempbuffer, *tmp;
	struct packetheader_field_node *node = NULL;
	int i = 0;

	if ((RetVal = (struct packetheader*)malloc(sizeof(struct packetheader))) == NULL) ILIBCRITICALEXIT(254);
	memset(RetVal,0,sizeof(struct packetheader));
	RetVal->HeaderTable = ILibInitHashTree_CaseInSensitive();

	//
	// All the headers are delineated with a CRLF, so we parse on that
	//
	p = (struct parser_result*)ILibParseString(buffer, offset, length, "\r\n", 2);
	_packet = p;
	f = p->FirstResult;
	//
	// The first token is where we can figure out the Method, Path, Version, etc.
	//
	StartLine = (struct parser_result*)ILibParseString(f->data, 0, f->datalength, " ", 1);
	HeaderLine = f->NextResult;
	if (memcmp(StartLine->FirstResult->data, "HTTP/", 5) == 0 && StartLine->FirstResult->NextResult != NULL)
	{
		//
		// If the StartLine starts with HTTP/, then we know this is a response packet.
		// We parse on the '/' character to determine the Version, as it follows. 
		// eg: HTTP/1.1 200 OK
		//
		p = (struct parser_result*)ILibParseString(StartLine->FirstResult->data, 0, StartLine->FirstResult->datalength, "/", 1);
		RetVal->Version = p->LastResult->data;
		RetVal->VersionLength = p->LastResult->datalength;
		RetVal->Version[RetVal->VersionLength] = 0;
		ILibDestructParserResults(p);
		if ((tempbuffer = (char*)malloc(1+sizeof(char)*(StartLine->FirstResult->NextResult->datalength))) == NULL) ILIBCRITICALEXIT(254);
		memcpy_s(tempbuffer,1 + StartLine->FirstResult->NextResult->datalength, StartLine->FirstResult->NextResult->data, StartLine->FirstResult->NextResult->datalength);
		MEMCHECK(assert(StartLine->FirstResult->NextResult->datalength <= 1+(int)sizeof(char)*(StartLine->FirstResult->NextResult->datalength));) 

			//
			// The other tokens contain the Status code and data
			//
			tempbuffer[StartLine->FirstResult->NextResult->datalength] = '\0';
		RetVal->StatusCode = (int)atoi(tempbuffer);
		free(tempbuffer);
		RetVal->StatusData = StartLine->FirstResult->NextResult->NextResult->data;
		RetVal->StatusDataLength = StartLine->FirstResult->NextResult->NextResult->datalength;
	}
	else
	{

		//
		// If the packet didn't start with HTTP/ then we know it's a request packet
		// eg: GET /index.html HTTP/1.1
		// The method (or directive), is the first token, and the Path
		// (or DirectiveObj) is the second, and version in the 3rd.
		//
		RetVal->Directive = StartLine->FirstResult->data;
		RetVal->DirectiveLength = StartLine->FirstResult->datalength;
		if (StartLine->FirstResult->NextResult!=NULL)
		{
			RetVal->DirectiveObj = StartLine->FirstResult->NextResult->data;
			RetVal->DirectiveObjLength = StartLine->FirstResult->NextResult->datalength;
		}
		else
		{
			// Invalid packet
			ILibDestructParserResults(_packet);
			ILibDestructParserResults(StartLine);
			ILibDestructPacket(RetVal);
			return(NULL);
		}

		RetVal->StatusCode = -1;
		//
		// We parse the last token on '/' to find the version
		//
		p = (struct parser_result*)ILibParseString(StartLine->LastResult->data, 0, StartLine->LastResult->datalength, "/", 1);
		RetVal->Version = p->LastResult->data;
		RetVal->VersionLength = p->LastResult->datalength;
		RetVal->Version[RetVal->VersionLength] = 0;
		ILibDestructParserResults(p);

		RetVal->Directive[RetVal->DirectiveLength] = '\0';
		RetVal->DirectiveObj[RetVal->DirectiveObjLength] = '\0';
	}
	//
	// Headerline starts with the second token. Then we iterate through the rest of the tokens
	//
	while (HeaderLine != NULL)
	{
		if (HeaderLine->datalength == 0 || HeaderLine->data == NULL)
		{
			//
			// An empty line signals the end of the headers
			//
			break;
		}
		if (node != NULL && (HeaderLine->data[0] == ' ' || HeaderLine->data[0] == 9))
		{
			//
			// This is a multi-line continuation
			//
			if (node->UserAllocStrings == 0)
			{
				tempbuffer = node->FieldData;
				if ((node->FieldData = (char*)malloc(node->FieldDataLength + HeaderLine->datalength)) == NULL) ILIBCRITICALEXIT(254);
				memcpy_s(node->FieldData, node->FieldDataLength + HeaderLine->datalength, tempbuffer, node->FieldDataLength);

				tempbuffer = node->Field;
				if ((node->Field = (char*)malloc(node->FieldLength + 1)) == NULL) ILIBCRITICALEXIT(254);
				memcpy_s(node->Field, node->FieldLength + 1, tempbuffer, node->FieldLength);

				node->UserAllocStrings = -1;
			}
			else
			{
				if ((tmp = (char*)realloc(node->FieldData, node->FieldDataLength + HeaderLine->datalength)) == NULL) ILIBCRITICALEXIT(254);
				node->FieldData = tmp;
			}
			memcpy_s(node->FieldData+node->FieldDataLength, HeaderLine->datalength, HeaderLine->data + 1, HeaderLine->datalength - 1);
			node->FieldDataLength += (HeaderLine->datalength-1);
		}
		else
		{
			//
			// Instantiate a new header entry for each new token
			//
			if ((node = (struct packetheader_field_node*)malloc(sizeof(struct packetheader_field_node))) == NULL) ILIBCRITICALEXIT(254);
			memset(node, 0, sizeof(struct packetheader_field_node));
			for(i = 0; i < HeaderLine->datalength; ++i)
			{
				if (*((HeaderLine->data) + i) == ':')
				{
					node->Field = HeaderLine->data;
					node->FieldLength = i;
					node->FieldData = HeaderLine->data + i + 1;
					node->FieldDataLength = (HeaderLine->datalength)-i-1;
					break;
				}
			}
			if (node->Field == NULL)
			{
				//
				// Invalid header line. Let's just ignore it and move on
				//
				free(node);
				node = NULL;
				HeaderLine = HeaderLine->NextResult;
				continue;
			}
			//
			// We need to do white space processing, because we need to ignore them in the
			// headers
			// So do a 'trim' operation
			//
			node->FieldDataLength = ILibTrimString(&(node->FieldData),node->FieldDataLength);			
			node->Field[node->FieldLength] = '\0';
			node->FieldData[node->FieldDataLength] = '\0';

			//
			// Since we are parsing an existing string, we set this flag to zero, so that it doesn't
			// get freed
			//
			node->UserAllocStrings = 0;
			node->NextField = NULL;

			if (RetVal->FirstField==NULL)
			{
				//
				// If there aren't any headers yet, this will be the first
				//
				RetVal->FirstField = node;
				RetVal->LastField = node;
			}
			else
			{
				//
				// There are already headers, so link this in the tail
				//
				RetVal->LastField->NextField = node; // Note: Klocwork says LastField could be NULL/dereferenced, but LastField is never going to be NULL.
			}
			RetVal->LastField = node;
			ILibAddEntryEx(RetVal->HeaderTable,node->Field,node->FieldLength,node->FieldData,node->FieldDataLength);
		}
		HeaderLine = HeaderLine->NextResult;
	}
	ILibDestructParserResults(_packet);
	ILibDestructParserResults(StartLine);
	return(RetVal);
}

/*! \fn ILibFragmentTextLength(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength)
\brief Determines the buffer size required to fragment a string
\param text The string to fragment
\param textLength Length of \a text
\param delimiter Line delimiter
\param delimiterLength Length of \a delimiter
\param tokenLength The maximum size of each fragment or token
\return The length of the buffer required to call \a ILibFragmentText
*/
int ILibFragmentTextLength(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength)
{
	int RetVal;

	UNREFERENCED_PARAMETER( text );
	UNREFERENCED_PARAMETER( delimiter );

	RetVal = textLength + (((textLength/tokenLength)==1?0:(textLength/tokenLength))*delimiterLength);
	return(RetVal);
}

/*! \fn ILibFragmentText(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength, char **RetVal)
\brief Fragments a string into multiple tokens
\param text The string to fragment
\param textLength Length of \a text
\param delimiter Line delimiter
\param delimiterLength Length of \a delimiter
\param tokenLength The maximum size of each fragment or token
\param RetVal The buffer to store the resultant string
\return The length of the written string
*/
int ILibFragmentText(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength, char **RetVal)
{
	char *Buffer;
	int i=0,i2=0;
	int BufferSize = 0;
	int allocSize = ILibFragmentTextLength(text, textLength, delimiter, delimiterLength, tokenLength);
	if ((*RetVal = (char*)malloc(allocSize)) == NULL) ILIBCRITICALEXIT(254);

	Buffer = *RetVal;

	i2 = textLength;
	while (i2!=0)
	{
		if (i2!=textLength)
		{
			memcpy_s(Buffer+i, allocSize - i, delimiter,delimiterLength);
			i+=delimiterLength;
			BufferSize += delimiterLength;
		}
		memcpy_s(Buffer+i, allocSize - i, text + (textLength-i2),i2>tokenLength?tokenLength:i2);
		i+=i2>tokenLength?tokenLength:i2;
		BufferSize += i2>tokenLength?tokenLength:i2;
		i2 -= i2>tokenLength?tokenLength:i2;
	}
	return(BufferSize);
}

/*! \fn ILibGetRawPacket(struct packetheader* packet,char **RetVal)
\brief Converts a packetheader structure into a raw char* buffer
\par
\b Note: The returned buffer must be freed
\param packet The packetheader struture to convert
\param[out] RetVal The output char* buffer
\return The length of the output buffer
*/
int ILibGetRawPacket(struct packetheader* packet, char **RetVal)
{
	int i,i2;
	int BufferSize = 0;
	char* Buffer, *temp;

	void *en;

	char *Field;
	int FieldLength;
	void *FieldData;
	int FieldDataLength;

	if (packet->StatusCode != -1)
	{
		BufferSize = 12 + packet->VersionLength + packet->StatusDataLength;
		//
		// HTTP/1.1 200 OK\r\n
		// 12 is the total number of literal characters. Just add Version and StatusData
		// HTTP/ OK \r\n
		//
	}
	else
	{
		BufferSize = packet->DirectiveLength + packet->DirectiveObjLength + 12;
		//
		// GET / HTTP/1.1\r\n 
		// This is calculating the length for a request packet.
		// ToDo: This isn't completely correct
		// But it will work as long as the version is not > 9.9
		// It should also add the length of the Version, but it's not critical.
	}

	en = ILibHashTree_GetEnumerator(packet->HeaderTable);
	while (ILibHashTree_MoveNext(en)==0)
	{
		ILibHashTree_GetValueEx(en,&Field,&FieldLength,(void**)&FieldData,&FieldDataLength);
		if (FieldDataLength < 0) { continue; }
		//
		// A conservative estimate adding the lengths of the header name and value, plus
		// 4 characters for the ':' and CRLF
		//
		BufferSize += FieldLength + FieldDataLength + 4;

		//
		// If the header is longer than MAX_HEADER_LENGTH, we need to break it up
		// into multiple lines, so we need to calculate the space needed for the
		// delimiters.
		//
		if (FieldDataLength>MAX_HEADER_LENGTH)
		{
			BufferSize += ILibFragmentTextLength(FieldData, FieldDataLength, "\r\n ", 3, MAX_HEADER_LENGTH);
		}
	}
	ILibHashTree_DestroyEnumerator(en);

	//
	// Another conservative estimate adding in the packet body length plus a padding of 3
	// for the empty line
	//
	BufferSize += (3 + packet->BodyLength);

	//
	// Allocate the buffer
	//
	if ((Buffer = *RetVal = (char*)malloc(BufferSize)) == NULL) ILIBCRITICALEXIT(254);
	if (packet->StatusCode != -1)
	{
		//
		// Write the response
		//
		memcpy_s(Buffer, BufferSize, "HTTP/", 5);
		memcpy_s(Buffer + 5, BufferSize - 5, packet->Version, packet->VersionLength);
		i = 5 + packet->VersionLength;

		i += sprintf_s(Buffer + i, BufferSize - i, " %d ", packet->StatusCode);
		memcpy_s(Buffer + i, BufferSize - i, packet->StatusData ,packet->StatusDataLength);
		i += packet->StatusDataLength;

		memcpy_s(Buffer + i, BufferSize - i, "\r\n", 2);
		i += 2;
		/* HTTP/1.1 200 OK\r\n */
	}
	else
	{
		//
		// Write the Request
		//
		memcpy_s(Buffer, BufferSize, packet->Directive,packet->DirectiveLength);
		i = packet->DirectiveLength;
		memcpy_s(Buffer+i, BufferSize - i, " ",1);
		i+=1;
		memcpy_s(Buffer+i, BufferSize - i, packet->DirectiveObj,packet->DirectiveObjLength);
		i+=packet->DirectiveObjLength;
		memcpy_s(Buffer+i, BufferSize - i, " HTTP/",6);
		i+=6;
		memcpy_s(Buffer+i, BufferSize - i, packet->Version,packet->VersionLength);
		i+=packet->VersionLength;
		memcpy_s(Buffer+i, BufferSize - i, "\r\n",2);
		i+=2;
		/* GET / HTTP/1.1\r\n */
	}

	en = ILibHashTree_GetEnumerator(packet->HeaderTable);
	while (ILibHashTree_MoveNext(en)==0)
	{
		ILibHashTree_GetValueEx(en,&Field,&FieldLength,(void**)&FieldData,&FieldDataLength);
		if (FieldDataLength < 0) { continue; }

		//
		// Write each header
		//
		memcpy_s(Buffer+i, BufferSize - i, Field,FieldLength);
		i+=FieldLength;
		memcpy_s(Buffer+i, BufferSize - i, ": ",2);
		i+=2;

		if (ILibFragmentTextLength(FieldData,FieldDataLength,"\r\n ",3, MAX_HEADER_LENGTH)>FieldDataLength)
		{
			// Fragment this
			i2 = ILibFragmentText(FieldData,FieldDataLength,"\r\n ",3, MAX_HEADER_LENGTH,&temp);
			memcpy_s(Buffer+i, BufferSize - i, temp,i2);
			i += i2;
			free(temp);
		}
		else
		{
			// No need to fragment this
			memcpy_s(Buffer+i, BufferSize - i, FieldData,FieldDataLength);
			i += FieldDataLength;
		}

		memcpy_s(Buffer+i, BufferSize - i, "\r\n",2);
		i+=2;
	}
	ILibHashTree_DestroyEnumerator(en);

	//
	// Write the empty line
	//
	memcpy_s(Buffer+i, BufferSize - i, "\r\n",2);
	i+=2;

	//
	// Write the body
	//
	memcpy_s(Buffer+i, BufferSize - i, packet->Body,packet->BodyLength);
	i+=packet->BodyLength;
	Buffer[i] = '\0';

	return(i);
}

// TCP: SOCK_STREAM, IPPROTO_TCP
// UDP: SOCK_DGRAM, IPPROTO_UDP
SOCKET ILibGetSocket(struct sockaddr *localif, int type, int protocol)
{
	int off = 0;
	SOCKET sock;
	if (localif->sa_family == AF_INET6 && g_ILibDetectIPv6Support == 0) { ILIBMARKPOSITION(1); return 0; }
	if ((sock = socket(localif->sa_family, type, protocol)) == -1) { ILIBMARKPOSITION(2); return 0; }
#ifdef NACL
	if (localif->sa_family == AF_INET6) if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off)) != 0) return 0;
#else
	if (localif->sa_family == AF_INET6) if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&off, sizeof(off)) != 0) ILIBCRITICALERREXIT(253);
#endif
#ifdef SO_NOSIGPIPE
	{
		int set = 1;
		setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));  // Turn off SIGPIPE if writing to disconnected socket
	}
#endif
#if defined(WIN32)
	if (bind(sock, localif, INET_SOCKADDR_LENGTH(localif->sa_family)) != 0) { ILIBMARKPOSITION(3); closesocket(sock); return 0; }
#else
	if (bind(sock, localif, INET_SOCKADDR_LENGTH(localif->sa_family)) != 0) { ILIBMARKPOSITION(4); close(sock); return 0; }
#endif
	return sock;
}


//! Parses a URI string, into its IP Address, Port Number, and Path components
/*!
\b Note: The Address and Path components must be freed
\param URI The URI to parse
\param[out] Addr The Address component
\param Port The Port component. Default is 80
\param[out] Path The Path component
\param[in,out] AddrStruct sockaddr_in6 structure holding the address and port [Can be NULL]
*/
ILibParseUriResult ILibParseUriEx (const char* URI, size_t URILen, char** Addr, unsigned short* Port, char** Path, struct sockaddr_in6* AddrStruct)
{
	struct parser_result *result, *result2, *result3;
	char *TempString, *TempString2;
	int TempStringLength, TempStringLength2;
	unsigned short lport;
	char* laddr = NULL;

	ILibParseUriResult retVal = ILibParseUriResult_UNKNOWN_SCHEME;

	// A scheme has the format xxx://yyy , so if we parse on ://, we can extract the path info
	result = ILibParseString((char *)URI, 0, (int)URILen, "://", 3);

	// Check the Scheme
	switch (result->FirstResult->datalength)
	{
		case 2:
			if (strncmp(result->FirstResult->data, "ws", 2) == 0 || strncmp(result->FirstResult->data, "WS", 2) == 0) { retVal = ILibParseUriResult_NO_TLS; }
			break;
		case 3:
			if (strncmp(result->FirstResult->data, "wss", 3) == 0 || strncmp(result->FirstResult->data, "WSS", 3) == 0) { retVal = ILibParseUriResult_TLS; }
			break;
		case 4:
			if (strncmp(result->FirstResult->data, "http", 4) == 0 || strncmp(result->FirstResult->data, "HTTP", 4) == 0) { retVal = ILibParseUriResult_NO_TLS; }
			break;
		case 5:
			if (strncmp(result->FirstResult->data, "https", 5) == 0 || strncmp(result->FirstResult->data, "HTTPS", 5) == 0) { retVal = ILibParseUriResult_TLS; }
			break;
		default:
			retVal = ILibParseUriResult_UNKNOWN_SCHEME;
			break;
	}

	TempString = result->LastResult->data;
	TempStringLength = result->LastResult->datalength;

	// Parse Path. The first '/' will occur after the IPAddress:Port combination
	result2 = ILibParseString(TempString, 0, TempStringLength,"/",1);
	TempStringLength2 = TempStringLength-result2->FirstResult->datalength;
	if (Path != NULL)
	{
		if ((*Path = (char*)malloc(TempStringLength2 + 1)) == NULL) ILIBCRITICALEXIT(254);
		memcpy_s(*Path, TempStringLength2 + 1, TempString + (result2->FirstResult->datalength), TempStringLength2);
		(*Path)[TempStringLength2] = '\0';
	}

	// Parse Port Number
	result3 = ILibParseString(result2->FirstResult->data, 0, result2->FirstResult->datalength, ":", 1);
	if (result3->NumResults == 1)
	{
		// The default port if non is specified, assuming HTTP(s) or WS(s)
		lport = (retVal == ILibParseUriResult_TLS) ? 443 : 80;
	}
	else
	{
		// If a port was specified, use that
		if ((TempString2 = (char*)malloc(result3->LastResult->datalength + 1)) == NULL) ILIBCRITICALEXIT(254);
		memcpy_s(TempString2, result3->LastResult->datalength + 1, result3->LastResult->data, result3->LastResult->datalength);
		TempString2[result3->LastResult->datalength] = '\0';
		lport = (unsigned short)atoi(TempString2);
		free(TempString2);
	}

	// Parse IP Address
	if (result3->FirstResult->data[0] == '[')
	{
		// This is an IPv6 address
		TempStringLength2 = ILibString_IndexOf(result2->FirstResult->data, result2->FirstResult->datalength, "]", 1);
		if (TempStringLength2 > 0)
		{
			if ((laddr = (char*)malloc(TempStringLength2 + 2)) == NULL) ILIBCRITICALEXIT(254);
			memcpy_s(laddr, TempStringLength2 + 2, result3->FirstResult->data, TempStringLength2 + 1);
			(laddr)[TempStringLength2 + 1] = '\0';
		}
	}
	else
	{
		// This is an IPv4 address
		TempStringLength2 = result3->FirstResult->datalength;
		if ((laddr = (char*)malloc(TempStringLength2 + 1)) == NULL) ILIBCRITICALEXIT(254);
		memcpy_s(laddr, TempStringLength2 + 1, result3->FirstResult->data, TempStringLength2);
		(laddr)[TempStringLength2] = '\0';
	}

	// Cleanup
	ILibDestructParserResults(result3);
	ILibDestructParserResults(result2);
	ILibDestructParserResults(result);

	// Convert to sockaddr if needed
	if (AddrStruct != NULL)
	{
		// Convert the string address into a sockaddr
		memset(AddrStruct, 0, sizeof(struct sockaddr_in6));
		if (laddr != NULL && laddr[0] == '[')
		{
			// IPv6
			AddrStruct->sin6_family = AF_INET6;
			ILibInet_pton(AF_INET6, laddr + 1, &(AddrStruct->sin6_addr));
			AddrStruct->sin6_port = (unsigned short)htons(lport);
		}
		else
		{
			// IPv4
			AddrStruct->sin6_family = AF_INET;
			if (ILibInet_pton(AF_INET, laddr, &(((struct sockaddr_in*)AddrStruct)->sin_addr)) == 0)
			{
				// This was not an IP Address, but a DNS name
				if (ILibResolveEx(laddr, lport, AddrStruct) != 0)
				{
					// Failed to resolve
					AddrStruct->sin6_family = AF_UNSPEC;
				}
			}
			((struct sockaddr_in*)AddrStruct)->sin_port = (unsigned short)htons(lport);
		}
	}

	if (Port != NULL) *Port = lport;
	if (Addr != NULL) *Addr = laddr; else if (laddr != NULL) free(laddr);
	return(retVal);
}

/*! \fn ILibCreateEmptyPacket()
\brief Creates an empty packetheader structure
\return An empty packet
*/
struct packetheader *ILibCreateEmptyPacket()
{
	struct packetheader *RetVal;
	if ((RetVal = (struct packetheader*)malloc(sizeof(struct packetheader))) == NULL) ILIBCRITICALEXIT(254);
	memset(RetVal,0,sizeof(struct packetheader));

	RetVal->UserAllocStrings = -1;
	RetVal->StatusCode = -1;
	RetVal->Version = "1.0";
	RetVal->VersionLength = 3;
	RetVal->HeaderTable = ILibInitHashTree_CaseInSensitive();

	return(RetVal);
}

/*! \fn ILibClonePacket(struct packetheader *packet)
\brief Creates a Deep Copy of a packet structure
\par
Because ILibParsePacketHeader does not copy any data, the data will become invalid
once the data is flushed. This method is used to preserve the data.
\param packet The packet to clone
\return A cloned packet structure
*/
struct packetheader* ILibClonePacket(struct packetheader *packet)
{
	struct packetheader *RetVal = ILibCreateEmptyPacket();
	struct packetheader_field_node *n;

	RetVal->ClonedPacket = 1;

	// Copy the addresses
	memcpy_s(&(RetVal->ReceivingAddress), sizeof(RetVal->ReceivingAddress), &(packet->ReceivingAddress), INET_SOCKADDR_LENGTH(((struct sockaddr_in6*)(packet->ReceivingAddress))->sin6_family));
	memcpy_s(&(RetVal->Source), sizeof(RetVal->Source), &(packet->Source), INET_SOCKADDR_LENGTH(((struct sockaddr_in6*)(packet->ReceivingAddress))->sin6_family));

	// These three calls will result in the fields being copied
	ILibSetDirective(
		RetVal,
		packet->Directive,
		packet->DirectiveLength,
		packet->DirectiveObj,
		packet->DirectiveObjLength);

	ILibSetStatusCode(
		RetVal,
		packet->StatusCode,
		packet->StatusData,
		packet->StatusDataLength);

	ILibSetVersion(RetVal,packet->Version,packet->VersionLength);

	// Iterate through each header, and copy them
	n = packet->FirstField;
	while (n!=NULL)
	{
		ILibAddHeaderLine(
			RetVal,
			n->Field,
			n->FieldLength,
			n->FieldData,
			n->FieldDataLength);
		n = n->NextField;
	}
	return(RetVal);
}

/*! \fn ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength)
\brief Sets the version of a packetheader structure. The Default version is 1.0
\param packet The packet to modify
\param Version The version string to write. eg: 1.1
\param VersionLength The length of the \a Version
*/
void ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength)
{
	if (packet->UserAllocVersion!=0) {free(packet->Version);}
	packet->UserAllocVersion = 1;
	if ((packet->Version = (char*)malloc(1+VersionLength)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(packet->Version, 1 + VersionLength, Version, VersionLength);
	packet->Version[VersionLength] = '\0';
}

/*! \fn ILibSetStatusCode(struct packetheader *packet, int StatusCode, char *StatusData, int StatusDataLength)
\brief Sets the status code of a packetheader structure
\param packet The packet to modify
\param StatusCode The status code, eg: 200
\param StatusData The status string, eg: OK
\param StatusDataLength The length of \a StatusData
*/
void ILibSetStatusCode(struct packetheader *packet, int StatusCode, char *StatusData, int StatusDataLength)
{
	if (StatusDataLength < 0) { StatusDataLength = (int)strnlen_s(StatusData, 255); }
	packet->StatusCode = StatusCode;
	if ((packet->StatusData = (char*)malloc(StatusDataLength+1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(packet->StatusData, StatusDataLength + 1, StatusData, StatusDataLength);
	packet->StatusData[StatusDataLength] = '\0';
	packet->StatusDataLength = StatusDataLength;
}

/*! \fn ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength)
\brief Sets the /a Method and /a Path of a packetheader structure
\param packet The packet to modify
\param Directive The Method to write, eg: \b GET
\param DirectiveLength The length of \a Directive
\param DirectiveObj The path component of the method, eg: \b /index.html
\param DirectiveObjLength The length of \a DirectiveObj
*/
void ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength)
{
	if (DirectiveLength < 0)DirectiveLength = (int)strnlen_s(Directive, 255);
	if (DirectiveObjLength < 0)DirectiveObjLength = (int)strnlen_s(DirectiveObj, 255);

	if ((packet->Directive = (char*)malloc(DirectiveLength+1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(packet->Directive, DirectiveLength + 1, Directive,DirectiveLength);
	packet->Directive[DirectiveLength] = '\0';
	packet->DirectiveLength = DirectiveLength;

	if ((packet->DirectiveObj = (char*)malloc(DirectiveObjLength+1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(packet->DirectiveObj, DirectiveObjLength + 1, DirectiveObj, DirectiveObjLength);
	packet->DirectiveObj[DirectiveObjLength] = '\0';
	packet->DirectiveObjLength = DirectiveObjLength;
	packet->UserAllocStrings = -1;
}

void ILibHTTPPacket_Stash_Put(ILibHTTPPacket *packet, char* key, int keyLen, void *data)
{
	if (keyLen < 0) { keyLen = (int)strnlen_s(key, 255); }
	ILibAddEntryEx(packet->HeaderTable, key, keyLen, data, -1);
}
int ILibHTTPPacket_Stash_HasKey(ILibHTTPPacket *packet, char* key, int keyLen)
{
	return(ILibHasEntry(packet->HeaderTable, key, keyLen));
}
void* ILibHTTPPacket_Stash_Get(ILibHTTPPacket *packet, char* key, int keyLen)
{
	if (keyLen < 0) { keyLen = (int)strnlen_s(key, 255); }
	return(ILibGetEntry(packet->HeaderTable, key, keyLen));
}

/*! \fn void ILibDeleteHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength)
\brief Removes an HTTP header entry from a packetheader structure
\param packet The packet to modify
\param FieldName The header name, eg: \b CONTENT-TYPE
\param FieldNameLength The length of the \a FieldName
*/
void ILibDeleteHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength)
{
	ILibDeleteEntry(packet->HeaderTable,FieldName,FieldNameLength);
}
/*! \fn ILibAddHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength, char* FieldData, int FieldDataLength)
\brief Adds an HTTP header entry into a packetheader structure
\param packet The packet to modify
\param FieldName The header name, eg: \b CONTENT-TYPE
\param FieldNameLength The length of the \a FieldName
\param FieldData The header value, eg: \b text/xml
\param FieldDataLength The length of the \a FieldData
*/
void ILibAddHeaderLine(struct packetheader *packet, const char* FieldName, int FieldNameLength, const char* FieldData, int FieldDataLength)
{
	struct packetheader_field_node *node;
	if (FieldNameLength < 0) { FieldNameLength = (int)strnlen_s(FieldName, 255); }
	if (FieldDataLength < 0) { FieldDataLength = (int)strnlen_s(FieldData, 255); }

	//
	// Create the Header Node
	//
	if ((node = (struct packetheader_field_node*)malloc(sizeof(struct packetheader_field_node))) == NULL) ILIBCRITICALEXIT(254);
	node->UserAllocStrings = -1;
	if ((node->Field = (char*)malloc(FieldNameLength+1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(node->Field, FieldNameLength + 1, (char*)FieldName, FieldNameLength);
	node->Field[FieldNameLength] = '\0';
	node->FieldLength = FieldNameLength;

	if ((node->FieldData = (char*)malloc(FieldDataLength+1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(node->FieldData, FieldDataLength + 1, (char*)FieldData, FieldDataLength);
	node->FieldData[FieldDataLength] = '\0';
	node->FieldDataLength = FieldDataLength;

	node->NextField = NULL;

	ILibAddEntryEx(packet->HeaderTable,node->Field,node->FieldLength,node->FieldData,node->FieldDataLength);

	//
	// And attach it to the linked list
	//
	if (packet->LastField!=NULL)
	{
		packet->LastField->NextField = node;
		packet->LastField = node;
	}
	else
	{
		packet->LastField = node;
		packet->FirstField = node;
	}
}

char* ILibUrl_GetHost(char *url, int urlLen)
{
	int startX, endX;
	char *retVal = NULL;
	urlLen = urlLen >= 0 ? urlLen : (int)strnlen_s(url, sizeof(ILibScratchPad));

	startX = ILibString_IndexOf(url, urlLen, "://", 3);
	if (startX < 0) { startX = 0; } else { startX += 3; }

	endX = ILibString_IndexOf(url + startX, urlLen - startX, "/", 1);
	retVal = url + startX;
	retVal[endX > 0 ? endX : urlLen - startX] = 0;

	return(retVal);
}

/*! \fn ILibGetHeaderLineEx(struct packetheader *packet, char* FieldName, int FieldNameLength, int *len)
\brief Retrieves an HTTP header value from a packet structure
\par
\param packet The packet to introspect
\param FieldName The header name to lookup
\param FieldNameLength The length of \a FieldName
\param len The length of the return value. Can be NULL
\return The header value. NULL if not found
*/
char* ILibGetHeaderLineEx(struct packetheader *packet, char* FieldName, int FieldNameLength, int *len)
{
	void* RetVal = NULL;
	int valLength;

	ILibGetEntryEx(packet->HeaderTable,FieldName,FieldNameLength,(void**)&RetVal,&valLength);
	if (valLength!=0)
	{
		((char*)RetVal)[valLength]=0;
	}
	if (len != NULL) { *len = valLength; }
	return((char*)RetVal);
}
char* ILibGetHeaderLineSP_Next(char* PreviousValue, char* FieldName, int FieldNameLength)
{
	packetheader_field_node *header = (packetheader_field_node*)(PreviousValue - sizeof(void*));
	char *retVal = ILibScratchPad + sizeof(void*);

	while (header != NULL)
	{
		if (FieldNameLength == header->FieldLength && strncasecmp(FieldName, header->Field, FieldNameLength) == 0)
		{
			((void**)ILibScratchPad)[0] = header->NextField;
			memcpy_s(retVal, sizeof(ILibScratchPad) - sizeof(void*), header->FieldData, header->FieldDataLength);
			retVal[header->FieldDataLength] = 0;
			return(retVal);
		}
		header = header->NextField;
	}
	return(NULL);
}

/*! \fn ILibGetHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength)
\brief Retrieves an HTTP header value from a packet structure, copied into the ScratchPad
\par
\param packet The packet to introspect
\param FieldName The header name to lookup
\param FieldNameLength The length of \a FieldName
\return The header value. NULL if not found
*/
char* ILibGetHeaderLineSP(struct packetheader *packet, char* FieldName, int FieldNameLength)
{
	char* retVal = ILibScratchPad + sizeof(void*);
	((void**)ILibScratchPad)[0] = packet->FirstField;;

	return(ILibGetHeaderLineSP_Next(retVal, FieldName, FieldNameLength));
}

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/* encode 3 8-bit binary bytes as 4 '6-bit' characters */
void ILibencodeblock( unsigned char in[3], unsigned char out[4], int len )
{
	out[0] = cb64[ in[0] >> 2 ];
	out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
	out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/*! \fn int ILibBase64EncodeLength(const int inputLen)
\brief Returns the length the ILibBase64Encode function would use a stream adding padding and line breaks as per spec.
\par
\param input The length of the en-encoded data
\return The length of the encoded stream
*/
int ILibBase64EncodeLength(const int inputLen)
{
	return ((inputLen * 4) / 3) + 5;
}

/*! \fn ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output)
\brief Base64 encode a stream adding padding and line breaks as per spec.
\par
\b Note: The encoded stream must be freed if **ouput is passed in as NULL
\param input The stream to encode
\param inputlen The length of \a input
\param[in,out] output The encoded stream. if *output is not NULL, then the base64 will be copied there, if it is NULL, then the function will malloc
\return The length of the encoded stream
*/
int ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* out;
	unsigned char* in;

	if (*output == NULL) { if ((*output = (unsigned char*)malloc(((inputlen * 4) / 3) + 5)) == NULL) ILIBCRITICALEXIT(254); }
	out = *output;
	in = input;

	if (input == NULL || inputlen == 0) { *output = NULL; return 0; }
	while ((in+3) <= (input+inputlen)) { ILibencodeblock(in, out, 3); in += 3; out += 4; }
	if ((input+inputlen)-in == 1) { ILibencodeblock(in, out, 1); out += 4; }
	else if ((input+inputlen)-in == 2) { ILibencodeblock(in, out, 2); out += 4; }
	*out = 0;

	return (int)(out-*output);
}

/* Decode 4 '6-bit' characters into 3 8-bit binary bytes */
void ILibdecodeblock( unsigned char in[4], unsigned char out[3] )
{
	out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/*! \fn int ILibBase64DecodeLength(const int inputLen)
\brief Returns the length the ILibBase64Decode function would use to store the decoded string value
\par
\param input The length of the en-encoded data
\return The length of the decoded stream
*/
int ILibBase64DecodeLength(const int inputLen)
{
	return ((inputLen * 3) / 4) + 4;
}

/*! \fn ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output)
\brief Decode a base64 encoded stream discarding padding, line breaks and noise
\par
\b Note: The encoded stream must be freed if **ouput is passed in as NULL
\param input The stream to decode
\param inputlen The length of \a input
\param[in,out] The encoded stream. if *output is not NULL, then the base64 will be copied there, if it is NULL, then the function will malloc
\return The length of the decoded stream
*/
int ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* inptr;
	unsigned char* out;
	unsigned char v;
	unsigned char in[4];
	int i, len;

	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}
	if(*output==NULL)
		if ((*output = (unsigned char*)malloc(((inputlen * 3) / 4) + 4)) == NULL) ILIBCRITICALEXIT(254);
	out = *output;
	inptr = input;

	memset(in, 0, 4);
	while (inptr <= (input + inputlen))
	{
		for(len = 0, i = 0; i < 4 && inptr <= (input + inputlen); i++)
		{
			v = 0;
			while ( inptr <= (input+inputlen) && v == 0 ) {
				v = (unsigned char) *inptr;
				inptr++;
				v = (unsigned char)((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
				if (v) {
					v = (unsigned char)((v == '$') ? 0 : v - 61);
				}
			}
			if (inptr <= (input + inputlen)) {
				len++;
				if (v) {
					in[i] = (unsigned char)(v - 1);
				}
			}
			else {
				in[i] = 0;
			}
		}
		if (len)
		{
			ILibdecodeblock(in, out);
			out += len - 1;
		}
	}
	*out = 0;
	return (int)(out-*output);
}

/*! \fn ILibInPlaceXmlUnEscape(char* data)
\brief Unescapes a string according to XML parsing rules
\par
Since an escaped XML string is always larger than its unescaped form, this method
will overwrite the escaped string with the unescaped string, while decoding.
\param data The XML string to unescape
\return The length of the unescaped XML string
*/
int ILibInPlaceXmlUnEscapeEx(char* data, size_t dataLen)
{
	char* end = data + dataLen;
	char* i = data;              /* src */
	char* j = data;              /* dest */

	//
	// Iterate through the string to find escaped sequences
	//
	while (j < end)
	{
		if (j[0] == '&' && j[1] == 'q' && j[2] == 'u' && j[3] == 'o' && j[4] == 't' && j[5] == ';')   // &quot;
		{
			// Double Quote
			i[0] = '"';
			j += 5;
		}
		else if (j[0] == '&' && j[1] == 'a' && j[2] == 'p' && j[3] == 'o' && j[4] == 's' && j[5] == ';')   // &apos;
		{
			// Single Quote (apostrophe)
			i[0] = '\'';
			j += 5;
		}
		else if (j[0] == '&' && j[1] == 'a' && j[2] == 'm' && j[3] == 'p' && j[4] == ';')   // &amp;
		{
			// Ampersand
			i[0] = '&';
			j += 4;
		}
		else if (j[0] == '&' && j[1] == 'l' && j[2] == 't' && j[3] == ';')   // &lt;
		{
			// Less Than
			i[0] = '<';
			j += 3;
		}
		else if (j[0] == '&' && j[1] == 'g' && j[2] == 't' && j[3] == ';')   // &gt;
		{
			// Greater Than
			i[0] = '>';
			j += 3;
		}
		else
		{
			i[0] = j[0];
		}
		i++;
		j++;
	}
	i[0] = '\0';
	return (int)(i - data);
}

/*! \fn ILibXmlEscapeLength(const char* data)
\brief Calculates the minimum required buffer space, to escape an xml string
\par
\b Note: This calculation does not include space for a null terminator
\param data The XML string to calculate buffer requirments with
\return The minimum required buffer size
*/
int ILibXmlEscapeLengthEx(const char* data, size_t dataLen)
{
	int i = 0, j = 0;
	while (data[i] != 0 && i < (int)dataLen)
	{
		switch (data[i])
		{
		case '"':
			j += 6;
			break;
		case '\'':
			j += 6;
			break;
		case '<':
			j += 4;
			break;
		case '>':
			j += 4;
			break;
		case '&':
			j += 5;
			break;
		default:
			j++;
		}
		i++;
	}
	return j;
}


/*! \fn ILibXmlEscape(char* outdata, const char* indata)
\brief Escapes a string according to XML parsing rules
\par
\b Note: \a outdata must be pre-allocated and freed
\param outdata The escaped XML string
\param indata The string to escape
\return The length of the escaped string
*/
int ILibXmlEscapeEx(char* outdata, const char* indata, size_t indataLen)
{
	int i=0;
	char* out;

	out = outdata;

	for (i=0; i < (int)indataLen; i++)
	{
		if (indata[i] == '"')
		{
			memcpy_s(out, 6, "&quot;", 6);
			out = out + 6;
		}
		else
			if (indata[i] == '\'')
			{
				memcpy_s(out, 6, "&apos;", 6);
				out = out + 6;
			}
			else
				if (indata[i] == '<')
				{
					memcpy_s(out, 4, "&lt;", 4);
					out = out + 4;
				}
				else
					if (indata[i] == '>')
					{
						memcpy_s(out, 4, "&gt;", 4);
						out = out + 4;
					}
					else
						if (indata[i] == '&')
						{
							memcpy_s(out, 5, "&amp;", 5);
							out = out + 5;
						}
						else
						{
							out[0] = indata[i];
							out++;
						}
	}

	out[0] = 0;

	return (int)(out - outdata);
}

// Return the number of milliseconds until trigger, -1 if not found.
long long ILibLifeTime_GetExpiration(void *LifetimeMonitorObject, void *data)
{
	void *node;
	struct LifeTimeMonitorData *temp;
	struct ILibLifeTime *LifeTimeMonitor = (struct ILibLifeTime*)LifetimeMonitorObject;

	node = ILibLinkedList_GetNode_Head(LifeTimeMonitor->ObjectList);
	while (node != NULL)
	{
		temp = (struct LifeTimeMonitorData*)ILibLinkedList_GetDataFromNode(node);
		if (temp->data == data) return temp->ExpirationTick;
		node = ILibLinkedList_GetNextNode(node);
	}
	return -1;
}

/*! \fn ILibLifeTime_AddEx(void *LifetimeMonitorObject,void *data, int ms, void* Callback, void* Destroy)
\brief Registers a timed callback with millisecond granularity
\param LifetimeMonitorObject The \a ILibLifeTime object to add the timed callback to
\param data The data object to associate with the timed callback
\param ms The number of milliseconds for the timed callback
\param Callback The callback function pointer to trigger when the specified time elapses
\param Destroy The abort function pointer, which triggers all non-triggered timed callbacks, upon shutdown
*/
void ILibLifeTime_AddEx(void *LifetimeMonitorObject,void *data, int ms, ILibLifeTime_OnCallback Callback, ILibLifeTime_OnCallback Destroy)
{
	struct LifeTimeMonitorData *temp;
	struct LifeTimeMonitorData *ltms;
	struct ILibLifeTime *LifeTimeMonitor = (struct ILibLifeTime*)LifetimeMonitorObject;
	void *node;

	if ((ltms = (struct LifeTimeMonitorData*)malloc(sizeof(struct LifeTimeMonitorData))) == NULL) ILIBCRITICALEXIT(254);
	memset(ltms,0,sizeof(struct LifeTimeMonitorData));

	//
	// Set the trigger time
	//
	ltms->data = data;
	ltms->ExpirationTick = ms != 0 ? (ILibGetUptime() + (long long)(ms)) : 0;

	//
	// Set the callback handlers
	//
	ltms->CallbackPtr = Callback;
	ltms->DestroyPtr = Destroy;

	ILibLinkedList_Lock(LifeTimeMonitor->ObjectList);

	// Add the node to the list
	node = ILibLinkedList_GetNode_Head(LifeTimeMonitor->ObjectList);
	if (node == NULL)
	{
		ILibLinkedList_AddTail(LifeTimeMonitor->ObjectList, ltms);
		ILibForceUnBlockChain(LifeTimeMonitor->ChainLink.ParentChain);
	}
	else
	{
		while (node != NULL)
		{
			temp = (struct LifeTimeMonitorData*)ILibLinkedList_GetDataFromNode(node);
			if (ltms->ExpirationTick < temp->ExpirationTick)
			{
				ILibLinkedList_InsertBefore(node, ltms);
				break;
			}
			node = ILibLinkedList_GetNextNode(node);
		}
		if (node == NULL)
		{
			ILibLinkedList_AddTail(LifeTimeMonitor->ObjectList,ltms);
		}
		else if (ILibLinkedList_GetDataFromNode(ILibLinkedList_GetNode_Head(LifeTimeMonitor->ObjectList)) == ltms)
		{
			ILibForceUnBlockChain(LifeTimeMonitor->ChainLink.ParentChain);
		}
	}

	// If this notification is sooner than the existing one, replace it.
	if (LifeTimeMonitor->NextTriggerTick > ltms->ExpirationTick) LifeTimeMonitor->NextTriggerTick = ltms->ExpirationTick;

	ILibLinkedList_UnLock(LifeTimeMonitor->ObjectList);
}

//
// An internal method used by the ILibLifeTime methods
// 
void ILibLifeTime_Check(void *LifeTimeMonitorObject, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	void *node;
	int removed;
	void *EventQueue;
	long long CurrentTick;
	struct ILibLinkedListNode_Root root;
	struct LifeTimeMonitorData *EVT, *Temp = NULL;
	struct ILibLifeTime *LifeTimeMonitor = (struct ILibLifeTime*)LifeTimeMonitorObject;

	UNREFERENCED_PARAMETER( readset );
	UNREFERENCED_PARAMETER( writeset );
	UNREFERENCED_PARAMETER( errorset );


	//
	// Get the current tick count for reference
	//
	CurrentTick = ILibGetUptime();

	//
	// This will speed things up by skipping the timer check
	//
	if(LifeTimeMonitor->NextTriggerTick !=0 && ((LifeTimeMonitor->NextTriggerTick > CurrentTick) && (LifeTimeMonitor->NextTriggerTick != -1) && (*blocktime > (int)(LifeTimeMonitor->NextTriggerTick - CurrentTick))))
	{
		*blocktime = (int)(LifeTimeMonitor->NextTriggerTick - CurrentTick);
		return;
	}
	LifeTimeMonitor->NextTriggerTick = -1;

	// This is an optimization. We are going to create the root of this linked list on the stack instead of the heap.
	// This also fixes a crash with malloc returns NULL if this is created in the heap - No idea why this occurs.
	memset(&root, 0, sizeof(struct ILibLinkedListNode_Root));
	EventQueue = (void*)&root;

	ILibLinkedList_Lock(LifeTimeMonitor->Reserved);
	ILibLinkedList_Lock(LifeTimeMonitor->ObjectList);

	while (ILibQueue_DeQueue(LifeTimeMonitor->Reserved) != NULL);

	node = ILibLinkedList_GetNode_Head(LifeTimeMonitor->ObjectList);
	while (node != NULL)
	{
		Temp = (struct LifeTimeMonitorData*)ILibLinkedList_GetDataFromNode(node);
		if (Temp->ExpirationTick == 0 || Temp->ExpirationTick < CurrentTick)
		{
			ILibQueue_EnQueue(EventQueue, Temp);
			node = ILibLinkedList_Remove(node);
		}
		else
		{
			if (LifeTimeMonitor->NextTriggerTick == -1 || Temp->ExpirationTick < LifeTimeMonitor->NextTriggerTick) LifeTimeMonitor->NextTriggerTick = Temp->ExpirationTick; // Save the next smallest value
			node = ILibLinkedList_GetNextNode(node);
		}
	}

	ILibLinkedList_UnLock(LifeTimeMonitor->ObjectList);
	ILibLinkedList_UnLock(LifeTimeMonitor->Reserved);

	//
	// Iterate through all the triggers that we need to fire
	//
	node = ILibQueue_DeQueue(EventQueue);
	while (node != NULL)
	{
		//
		// Check to see if the item to be fired, is in the remove list.
		// If it is, that means we shouldn't fire this item anymore.
		//
		ILibLinkedList_Lock(LifeTimeMonitor->Reserved);
		removed = ILibLinkedList_Remove_ByData(LifeTimeMonitor->Reserved,node);
		ILibLinkedList_UnLock(LifeTimeMonitor->Reserved);

		EVT = (struct LifeTimeMonitorData*)node;
		if (removed == 0)
		{
			// Trigger the callback
			EVT->CallbackPtr(EVT->data);
		}
		else
		{
			//
			// This item was flagged for removal, so intead of triggering it,
			// call the destroy pointer if it exists
			//
			if (EVT->DestroyPtr != NULL) { EVT->DestroyPtr(EVT->data); }
		}

		free(EVT);
		node = ILibQueue_DeQueue(EventQueue);
	}

	// Compute how much time until next trigger
	if (LifeTimeMonitor->NextTriggerTick != -1 && *blocktime > (int)(LifeTimeMonitor->NextTriggerTick - CurrentTick))
	{
		int delta = (int)(LifeTimeMonitor->NextTriggerTick - CurrentTick);
		if (delta > 1000) *blocktime = 1000; else *blocktime = delta;
	}
}

/*! \fn ILibLifeTime_Remove(void *LifeTimeToken, void *data)
\brief Removes a timed callback from an \a ILibLifeTime module
\param LifeTimeToken The \a ILibLifeTime object to remove the callback from
\param data The data object to remove
*/
void ILibLifeTime_Remove(void *LifeTimeToken, void *data)
{
	void *node;
	int removed = 0;
	struct LifeTimeMonitorData *evt;
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	void *EventQueue;

	if (UPnPLifeTime->ObjectList == NULL) return;
	EventQueue = ILibQueue_Create();
	ILibLinkedList_Lock(UPnPLifeTime->ObjectList);

	node = ILibLinkedList_GetNode_Head(UPnPLifeTime->ObjectList);
	if (node != NULL)
	{
		while (node != NULL)
		{
			evt = (struct LifeTimeMonitorData*)ILibLinkedList_GetDataFromNode(node);
			if (evt->data == data)
			{
				ILibQueue_EnQueue(EventQueue, evt);
				node = ILibLinkedList_Remove(node);
				removed = 1;
			}
			else
			{
				node = ILibLinkedList_GetNextNode(node);
			}
		}
		if (removed == 0)
		{
			//
			// The item wasn't in the list, so maybe it is pending to be triggered
			//
			ILibLinkedList_Lock(UPnPLifeTime->Reserved);
			ILibLinkedList_AddTail(UPnPLifeTime->Reserved, data);
			ILibLinkedList_UnLock(UPnPLifeTime->Reserved);
		}
	}
	ILibLinkedList_UnLock(UPnPLifeTime->ObjectList);

	//
	// Iterate through each node that is to be removed
	//
	evt = (struct LifeTimeMonitorData*)ILibQueue_DeQueue(EventQueue);
	while (evt != NULL)
	{
		if (evt->DestroyPtr != NULL) {evt->DestroyPtr(evt->data);}
		free(evt);
		evt = (struct LifeTimeMonitorData*)ILibQueue_DeQueue(EventQueue);
	}
	ILibQueue_Destroy(EventQueue);
}

/*! \fn ILibLifeTime_Flush(void *LifeTimeToken)
\brief Flushes all timed callbacks from an ILibLifeTime module
\param LifeTimeToken The \a ILibLifeTime object to flush items from
*/
void ILibLifeTime_Flush(void *LifeTimeToken)
{
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	struct LifeTimeMonitorData *temp;

	ILibLinkedList_Lock(UPnPLifeTime->ObjectList);

	temp = (struct LifeTimeMonitorData*)ILibQueue_DeQueue(UPnPLifeTime->ObjectList);
	while (temp != NULL)
	{
		if (temp->DestroyPtr != NULL) temp->DestroyPtr(temp->data);
		free(temp);
		temp = (struct LifeTimeMonitorData*)ILibQueue_DeQueue(UPnPLifeTime->ObjectList);
	}
	ILibLinkedList_UnLock(UPnPLifeTime->ObjectList);
}

//
// An internal method used by the ILibLifeTime methods
//
void ILibLifeTime_Destroy(void *LifeTimeToken)
{
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	ILibLifeTime_Flush(LifeTimeToken);
	ILibLinkedList_Destroy(UPnPLifeTime->ObjectList);
	ILibQueue_Destroy(UPnPLifeTime->Reserved);
	UPnPLifeTime->ObjectCount = 0;
	UPnPLifeTime->ObjectList = NULL;
}

/*! \fn ILibCreateLifeTime(void *Chain)
\brief Creates an empty ILibLifeTime container for Timed Callbacks.
\par
\b Note: All events are triggered on the MicroStack thread. Developers must \b NEVER block this thread!
\param Chain The chain to add the \a ILibLifeTime to
\return An \a ILibLifeTime token, which is used to add/remove callbacks
*/
void *ILibCreateLifeTime(void *Chain)
{
	struct ILibLifeTime *RetVal;
	if ((RetVal = (struct ILibLifeTime*)malloc(sizeof(struct ILibLifeTime))) == NULL) ILIBCRITICALEXIT(254);
	memset(RetVal,0,sizeof(struct ILibLifeTime));

	RetVal->ObjectList = ILibLinkedList_Create();
	RetVal->ChainLink.PreSelectHandler = &ILibLifeTime_Check;
	RetVal->ChainLink.DestroyHandler = &ILibLifeTime_Destroy;
	RetVal->ChainLink.ParentChain = Chain;
	RetVal->Reserved = ILibQueue_Create();

	ILibAddToChain(Chain, RetVal);
	return((void*)RetVal);
}


long ILibLifeTime_Count(void* LifeTimeToken)
{
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	return ILibQueue_GetCount(UPnPLifeTime->ObjectList);
}

/*! \fn ILibFindEntryInTable(char *Entry, char **Table)
\brief Find the index in \a Table that contains \a Entry.
\param Entry The char* to find
\param Table Array of char*, where the last entry is NULL
\return the index into \a Table, that contains \a Entry
*/
int ILibFindEntryInTable(char *Entry, char **Table)
{
	int i = 0;

	while (Table[i]!=NULL)
	{
		if (strcmp(Entry,Table[i])==0) return(i);
		++i;
	}

	return(-1);
}

void ILibLinkedList_SetData(void *LinkedList_Node, void *data)
{
	((struct ILibLinkedListNode*)LinkedList_Node)->Data = data;
}
//! Use the given comparer to insert data into the list. 
//! A Duplicate (according to comparer) will result in the chooser being dispatched to determine which value to keep
/*!
	\param LinkedList ILibLinkedList to perform the insert operation on
	\param comparer ILibLinkedList_Comparer handler to dispatch to perform the compare operation
	\param chooser ILibLinkedList_Chooser handler to dispatch to determine which value to keep, if a duplicate entry is detected
	\param data The data to insert
	\param user Custom user data to pass along with dispatched calls
	\return The LinkedList node that was inserted
*/
void* ILibLinkedList_SortedInsertEx(void* LinkedList, ILibLinkedList_Comparer comparer, ILibLinkedList_Chooser chooser, void *data, void *user)
{
	void* node = ILibLinkedList_GetNode_Head(LinkedList);
	if(node == NULL)
	{
		node = ILibLinkedList_AddHead(LinkedList, chooser(NULL, data, user));
	}
	else
	{
		while(node != NULL)
		{
			if(comparer(ILibLinkedList_GetDataFromNode(node), data) == 0)
			{
				// Duplicate. Replace entry				
				ILibLinkedList_SetData(node, chooser(ILibLinkedList_GetDataFromNode(node), data, user));
				break;
			}
			else if(comparer(ILibLinkedList_GetDataFromNode(node), data) < 0)
			{
				node = ILibLinkedList_InsertBefore(node, chooser(NULL, data, user));
				break;
			}
			node = ILibLinkedList_GetNextNode(node);
		}
		if(node == NULL)
		{
			node = ILibLinkedList_AddTail(LinkedList, chooser(NULL, data, user));			
		}
	}
	return(node);
}

void* ILibLinkedList_SortedInsert_DefaultChooser(void *oldObject, void *newObject, void *user)
{
	*((void**)user) = oldObject;
	return(newObject);
}


//! Use the given comparer to insert data into the list. 
//! A Duplicate (according to comparer) will result in the value being updated, and the old value being returned
/*!
	\param LinkedList ILibLinkedList to perform the insert operation on
	\param comparer ILibLinkedList_Comparer handler to dispatch to perform the compare operation
	\param data The data to insert
	\return Duplicates are not allowed, so if the comparison resulted in a duplicate, the old value will be returned, and the new value overwritten
*/
void* ILibLinkedList_SortedInsert(void* LinkedList, ILibLinkedList_Comparer comparer, void *data)
{
	void *retVal = NULL;
	ILibLinkedList_SortedInsertEx(LinkedList, comparer, &ILibLinkedList_SortedInsert_DefaultChooser, data, &retVal);
	return(retVal);
}
//! Use the given comparer to return the Linked List node that matches with the specified object
/*!
	\param LinkedList The ILibLinkedList to search
	\param comparer The ILibLinkedList_Comparer handler to dispatch to perform the comparison
	\param matchWith object to search
	\return First matching node [NULL if none found]
*/
void* ILibLinkedList_GetNode_Search(void* LinkedList, ILibLinkedList_Comparer comparer, void *matchWith)
{
	void *retVal = NULL;
	void *node = ILibLinkedList_GetNode_Head(LinkedList);
	while(node != NULL)
	{
		if((comparer!=NULL && comparer(ILibLinkedList_GetDataFromNode(node), matchWith)==0) || (comparer == NULL && ILibLinkedList_GetDataFromNode(node) == matchWith))
		{
			retVal = node;
			break;
		}
		node = ILibLinkedList_GetNextNode(node);
	}
	return(retVal);
}

/*! \fn ILibLinkedList_CreateEx(int userMemorySize)
\brief Create an empty Linked List Data Structure
\param userMemorySize Amount of memory to allocate for user data when allocating a linked list node
\return Empty Linked List
*/
void* ILibLinkedList_CreateEx(int userMemorySize)
{
	struct ILibLinkedListNode_Root *root;
	void *mem;

	ILibMemory_Allocate(sizeof(ILibLinkedListNode_Root), userMemorySize, (void**)&root, &mem);
	root->ExtraMemory = mem;
	
	sem_init(&(root->LOCK), 0, 1);
	return root;
}


void* ILibLinkedList_Create()
{
	return(ILibLinkedList_CreateEx(0));
}
//! Associate custom user data with a linked list object
/*!
	\param list ILibLinkedList to associate
	\param tag Custom user data to associate
*/
void ILibLinkedList_SetTag(ILibLinkedList list, void *tag)
{
	((struct ILibLinkedListNode_Root*)list)->Tag = tag;
}
//! Fetch the associated custom user data from a linked list
/*!
	\param list ILibLinkedList to query
	\return Associated custom user data [NULL if none associated]
*/
void* ILibLinkedList_GetTag(ILibLinkedList list)
{
	return(((struct ILibLinkedListNode_Root*)list)->Tag);
}

void* ILibLinkedList_AllocateNode(void *LinkedList)
{
	void* newNode;
	ILibMemory_Allocate(sizeof(ILibLinkedListNode), ILibMemory_GetExtraMemorySize(((ILibLinkedListNode_Root*)LinkedList)->ExtraMemory), &newNode, NULL);
	return(newNode);
}

void* ILibLinkedList_GetExtendedMemory(void* LinkedList_Node)
{
	if (LinkedList_Node == NULL) { return(NULL); }
	return(ILibMemory_GetExtraMemory(LinkedList_Node, sizeof(ILibLinkedListNode)));
}
/*! \fn ILibLinkedList_ShallowCopy(void *LinkedList)
\brief Create a shallow copy of a linked list. That is, the structure is copied, but none of the data contents are copied. The pointer values are just copied.
\param LinkedList The linked list to copy
\return The copy of the supplied linked list
*/
void* ILibLinkedList_ShallowCopy(void *LinkedList)
{
	void *RetVal = ILibLinkedList_Create();
	void *node = ILibLinkedList_GetNode_Head(LinkedList);
	while (node != NULL)
	{
		ILibLinkedList_AddTail(RetVal, ILibLinkedList_GetDataFromNode(node));
		node = ILibLinkedList_GetNextNode(node);
	}
	return(RetVal);
}

/*! \fn ILibLinkedList_GetNode_Head(void *LinkedList)
\brief Returns the Head node of a linked list data structure
\param LinkedList The linked list
\return The first node of the linked list
*/
void* ILibLinkedList_GetNode_Head(void *LinkedList)
{
	return(((struct ILibLinkedListNode_Root*)LinkedList)->Head);
}

/*! \fn ILibLinkedList_GetNode_Tail(void *LinkedList)
\brief Returns the Tail node of a linked list data structure
\param LinkedList The linked list
\return The last node of the linked list
*/
void* ILibLinkedList_GetNode_Tail(void *LinkedList)
{
	return(((struct ILibLinkedListNode_Root*)LinkedList)->Tail);
}

/*! \fn ILibLinkedList_GetNextNode(void *LinkedList_Node)
\brief Returns the next node, from the specified linked list node
\param LinkedList_Node The current linked list node
\return The next adjacent node of the current one
*/
void* ILibLinkedList_GetNextNode(void *LinkedList_Node)
{
	return(((struct ILibLinkedListNode*)LinkedList_Node)->Next);
}

/*! \fn ILibLinkedList_GetPreviousNode(void *LinkedList_Node)
\brief Returns the previous node, from the specified linked list node
\param LinkedList_Node The current linked list node
\return The previous adjacent node of the current one
*/
void* ILibLinkedList_GetPreviousNode(void *LinkedList_Node)
{
	return(((struct ILibLinkedListNode*)LinkedList_Node)->Previous);
}

/*! \fn ILibLinkedList_GetDataFromNode(void *LinkedList_Node)
\brief Returns the data pointed to by a linked list node
\param LinkedList_Node The current linked list node
\return The data pointer
*/
void *ILibLinkedList_GetDataFromNode(void *LinkedList_Node)
{
	return(LinkedList_Node != NULL ? (((struct ILibLinkedListNode*)LinkedList_Node)->Data) : NULL);
}

/*! \fn ILibLinkedList_InsertBefore(void *LinkedList_Node, void *data)
\brief Creates a new element, and inserts it before the given node
\param LinkedList_Node The linked list node
\param data The data pointer to be referenced
\return The LinkedList node that was inserted
*/
void* ILibLinkedList_InsertBefore(void *LinkedList_Node, void *data)
{
	struct ILibLinkedListNode_Root *r = ((struct ILibLinkedListNode*)LinkedList_Node)->Root;
	struct ILibLinkedListNode *n = (struct ILibLinkedListNode*) LinkedList_Node;
	struct ILibLinkedListNode *newNode;

	newNode = ILibLinkedList_AllocateNode(r);
	newNode->Data = data;
	newNode->Root = r;

	//
	// Attach ourselved before the specified node
	//
	newNode->Next = n;
	newNode->Previous = n->Previous;
	if (newNode->Previous!=NULL)
	{
		newNode->Previous->Next = newNode;
	}
	n->Previous = newNode;
	//
	// If we are the first node, we need to adjust the head
	//
	if (r->Head==n)
	{
		r->Head = newNode;
	}
	++r->count;
	return(newNode);
}

/*! \fn ILibLinkedList_InsertAfter(void *LinkedList_Node, void *data)
\brief Creates a new element, and appends it after the given node
\param LinkedList_Node The linked list node
\param data The data pointer to be referenced
\return The LinkedList Node that was inserted
*/
void* ILibLinkedList_InsertAfter(void *LinkedList_Node, void *data)
{
	struct ILibLinkedListNode_Root *r = ((struct ILibLinkedListNode*)LinkedList_Node)->Root;
	struct ILibLinkedListNode *n = (struct ILibLinkedListNode*) LinkedList_Node;
	struct ILibLinkedListNode *newNode;

	newNode = ILibLinkedList_AllocateNode(r);
	newNode->Data = data;
	newNode->Root = r;

	//
	// Attach ourselved after the specified node
	//
	newNode->Next = n->Next;
	n->Next = newNode;
	newNode->Previous = n;
	if (newNode->Next!=NULL) newNode->Next->Previous = newNode;

	//
	// If we are the last node, we need to adjust the tail
	//
	if (r->Tail==n) r->Tail = newNode;
	++r->count;
	return(newNode);
}

/*! \fn ILibLinkedList_Remove(void *LinkedList_Node)
\brief Removes the given node from a linked list data structure
\param LinkedList_Node The linked list node to remove
\return The next node
*/
void* ILibLinkedList_Remove(void *LinkedList_Node)
{
	struct ILibLinkedListNode_Root *r;
	struct ILibLinkedListNode *n;
	void* RetVal;

	if (LinkedList_Node == NULL) { return(NULL); }
	r = ((struct ILibLinkedListNode*)LinkedList_Node)->Root;
	n = (struct ILibLinkedListNode*) LinkedList_Node;

	RetVal = n->Next;

	if (n->Previous!=NULL)
	{
		n->Previous->Next = n->Next;
	}
	if (n->Next!=NULL)
	{
		n->Next->Previous = n->Previous;
	}
	if (r->Head==n)
	{
		r->Head = n->Next;
	}
	if (r->Tail==n)
	{
		if (n->Next==NULL)
		{
			r->Tail = n->Previous;
		}
		else
		{
			r->Tail = n->Next;
		}
	}
	--r->count;
	free(n);
	return(RetVal);
}

/*! \fn ILibLinkedList_Remove_ByData(void *LinkedList, void *data)
\brief Removes a node from the Linked list, via comparison
\par
Given a data pointer, will traverse the linked list data structure, deleting
elements that point to this data pointer.
\param LinkedList The linked list to traverse
\param data The data pointer to compare
\return Non-zero if an item was removed
*/
int ILibLinkedList_Remove_ByData(void *LinkedList, void *data)
{
	void *node;
	int RetVal=0;

	node = ILibLinkedList_GetNode_Head(LinkedList);
	while (node!=NULL)
	{
		if (ILibLinkedList_GetDataFromNode(node)==data)
		{
			++RetVal;
			node = ILibLinkedList_Remove(node);
		}
		else
		{
			node = ILibLinkedList_GetNextNode(node);
		}
	}
	return(RetVal);
}

/*! \fn ILibLinkedList_AddHead(void *LinkedList, void *data)
\brief Creates a new element, and inserts it at the top of the linked list.
\param LinkedList The linked list
\param data The data pointer to reference
*/
void* ILibLinkedList_AddHead(void *LinkedList, void *data)
{
	struct ILibLinkedListNode_Root *r = (struct ILibLinkedListNode_Root*)LinkedList;
	struct ILibLinkedListNode *newNode;

	newNode = ILibLinkedList_AllocateNode(r);
	newNode->Data = data;
	newNode->Root = r;
	newNode->Previous = NULL;

	newNode->Next = r->Head;
	if (r->Head!=NULL) r->Head->Previous = newNode;
	r->Head = newNode;
	if (r->Tail==NULL) r->Tail = newNode;
	++r->count;
	return(newNode);
}

/*! \fn ILibLinkedList_AddTail(void *LinkedList, void *data)
\brief Creates a new element, and appends it to the end of the linked list
\param LinkedList The linked list
\param data The data pointer to reference
*/
void* ILibLinkedList_AddTail(void *LinkedList, void *data)
{
	struct ILibLinkedListNode_Root *r = (struct ILibLinkedListNode_Root*)LinkedList;
	struct ILibLinkedListNode *newNode;

	newNode = ILibLinkedList_AllocateNode(r);
	newNode->Data = data;
	newNode->Root = r;
	newNode->Next = NULL;

	newNode->Previous = r->Tail;
	if (r->Tail != NULL) r->Tail->Next = newNode;
	r->Tail = newNode;
	if (r->Head == NULL) r->Head = newNode;
	++r->count;
	return(newNode);
}

/*! \fn ILibLinkedList_Lock(void *LinkedList)
\brief Locks the linked list with a non-recursive lock
\param LinkedList The linked list
*/
void ILibLinkedList_Lock(void *LinkedList)
{
	struct ILibLinkedListNode_Root *r = (struct ILibLinkedListNode_Root*)LinkedList;
	sem_wait(&(r->LOCK));
}

/*! \fn void ILibLinkedList_UnLock(void *LinkedList)
\brief Unlocks the linked list's non-recursive lock
\param LinkedList The linked list
*/
void ILibLinkedList_UnLock(void *LinkedList)
{
	struct ILibLinkedListNode_Root *r = (struct ILibLinkedListNode_Root*)LinkedList;
	sem_post(&(r->LOCK));
}


/*! \fn ILibLinkedList_Destroy(void *LinkedList)
\brief Frees the resources used by the linked list.
\par
\b Note: The data pointer referenced needs to be freed by the user if required
\param LinkedList The linked list
*/
void ILibLinkedList_Destroy(void *LinkedList)
{
	struct ILibLinkedListNode_Root *r = (struct ILibLinkedListNode_Root*)LinkedList;
	while (r->Head != NULL) ILibLinkedList_Remove(ILibLinkedList_GetNode_Head(LinkedList));
	sem_destroy(&(r->LOCK));
	free(r);
}


/*! \fn ILibLinkedList_GetCount(void *LinkedList)
\brief Returns the number of nodes in the linked list
\param LinkedList The linked list
\return Number of elements in the linked list
*/
long ILibLinkedList_GetCount(void *LinkedList)
{
	return(((struct ILibLinkedListNode_Root*)LinkedList)->count);
}

int ILibLinkedList_GetIndex(void *node)
{
	ILibLinkedListNode *current = (ILibLinkedListNode*)ILibLinkedList_GetNode_Head(((ILibLinkedListNode*)node)->Root);
	int i = 0;
	
	while (current != NULL && current != node)
	{
		++i;
		current = current->Next;
	}
	return(current != NULL ? i : -1);
}

typedef struct ILibHashtable_ClearCallbackStruct
{
	ILibHashtable source;
	ILibHashtable_OnDestroy onClear;
	void *user;
}ILibHashtable_ClearCallbackStruct;

int ILibHashtable_DefaultBucketizer(int value)
{
	// Convert 4 bytes to 1 byte
	unsigned char tmp[4];
	int retVal = 0;

	((unsigned int*)tmp)[0] = value;
	retVal = (int)(tmp[0] ^ tmp[1] ^ tmp[2] ^ tmp[3]); //Klocwork is being retarded, and doesn't realize an unsigned int is 4 bytes

	return(retVal);
}
int ILibHashtable_DefaultHashFunc(void* Key1, char* Key2, int Key2Len)
{
	char tmp[4];
	int i;
	int retVal = 0;
	union{int i; void* p;}u;

	u.p = Key1;
	if(Key1!=NULL) {retVal ^= u.i;}
	if (Key2 != NULL)
	{
		if (Key2Len < 5)
		{
			((int*)tmp)[0] = 0;
			for (i = 0; i < Key2Len; ++i)
			{
				tmp[i] = Key2[i];
			}
			retVal ^= ((int*)tmp)[0];
		}
		
		if (Key2Len > 4)
		{
			((int*)tmp)[0] = 0;
			for (i = 0; i < 4; ++i)
			{
				tmp[i] = Key2[Key2Len - 1 - i];
			}
			retVal ^= ((int*)tmp)[0];
			
			if (Key2Len > 12)
			{
				int x = Key2Len / 2;
				((int*)tmp)[0] = 0;
				for (i = 0; i < 4; ++i)
				{
					tmp[i] = Key2[i + x];
				}
				retVal ^= ((int*)tmp)[0];
			}
		}
	}
	return(retVal & 0x7FFFFFFF);
}
//! Create an Advanced Hashtable using the default Hashing Function, and default SparseArray/Bucketizer
/*!
	\return Hashtable
*/
ILibHashtable ILibHashtable_Create()
{
	ILibHashtable_Root *retVal;
	
	if((retVal = (ILibHashtable_Root*)malloc(sizeof(ILibHashtable_Root))) == NULL) {ILIBCRITICALEXIT(254);}
	memset(retVal, 0, sizeof(ILibHashtable_Root));
	
	ILibHashtable_ChangeHashFunc(retVal, &ILibHashtable_DefaultHashFunc);
	ILibHashtable_ChangeBucketizer(retVal, 256, &ILibHashtable_DefaultBucketizer);
	
	return(retVal);
}
void ILibHashtable_DestroyEx2(ILibSparseArray sender, int index, void *value, void *user)
{
	ILibHashtable_ClearCallbackStruct *state = (ILibHashtable_ClearCallbackStruct*)user;
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(index);

	if(value != NULL)
	{
		ILibHashtable_Node *tmp, *node = (ILibHashtable_Node*)value;
		ILibHashtable_OnDestroy onDestroy = state->onClear;
		while(node != NULL)
		{
			if(onDestroy!=NULL) {onDestroy(state->source, node->Key1, node->Key2, node->Key2Len, node->Data, state->user);}
			tmp = node->next;
			if(node->Key2 != NULL) { free(node->Key2); }
			free(node);
			node = tmp;
		}
	}
}
//! Free the resources associated with an Advanced Hashtable

//! Dispatches event handler for each element that is destroyed/cleared
/*!
	\param table Hashtable
	\param onDestroy Event handler to dispatch for destroyed/cleared elements
	\param user Custom User state data
*/
void ILibHashtable_DestroyEx(ILibHashtable table, ILibHashtable_OnDestroy onDestroy, void *user)
{
	ILibHashtable_Root *root = (ILibHashtable_Root*)table;
	ILibHashtable_ClearCallbackStruct state;
	memset(&state, 0, sizeof(ILibHashtable_ClearCallbackStruct));
	
	state.source = table;
	state.onClear = onDestroy;
	state.user = user;

	ILibSparseArray_DestroyEx(root->table, &ILibHashtable_DestroyEx2, &state);
	free(root);
}

void ILibHashtable_EnumerateSink(ILibSparseArray sender, int index, void *value, void *user)
{
	ILibHashtable_ClearCallbackStruct *state = (ILibHashtable_ClearCallbackStruct*)user;
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(index);

	if (value != NULL)
	{
		ILibHashtable_Node *node = (ILibHashtable_Node*)value;
		ILibHashtable_OnDestroy onEnumerate = state->onClear;
		while (node != NULL)
		{
			if (onEnumerate != NULL) { onEnumerate(state->source, node->Key1, node->Key2, node->Key2Len, node->Data, state->user); }
			node = node->next;
		}
	}
}

//! Enumerates the Hashtable
/*!
	\param table Hashtable to enumerate
	\param onEnumerate The callback to dispatch for each item
	\param user User state object to pass thru to the callback
*/
void ILibHashtable_Enumerate(ILibHashtable table, ILibHashtable_OnDestroy onEnumerate, void *user)
{
	ILibHashtable_ClearCallbackStruct state;
	memset(&state, 0, sizeof(ILibHashtable_ClearCallbackStruct));

	state.source = table;
	state.onClear = onEnumerate;
	state.user = user;

	ILibSparseArray_Enumerate(((ILibHashtable_Root*)table)->table, ILibHashtable_EnumerateSink, &state);
}

//! Clear the Hashtable, with a callback for each removed item
/*!
	\param table Hashtable to clear
	\param onClear The callback to dispatch for each item removed
	\param user User state object to pass thru to the callback
*/
void ILibHashtable_ClearEx(ILibHashtable table, ILibHashtable_OnDestroy onClear, void *user)
{
	ILibHashtable_ClearCallbackStruct state;
	memset(&state, 0, sizeof(ILibHashtable_ClearCallbackStruct));

	state.source = table;
	state.onClear = onClear;
	state.user = user;
	ILibSparseArray_ClearEx(((ILibHashtable_Root*)table)->table, ILibHashtable_DestroyEx2, &state);
}
//! Clear the Hashtable
/*!
	\param table Hashtable to clear
*/
void ILibHashtable_Clear(ILibHashtable table)
{
	ILibSparseArray_ClearEx(((ILibHashtable_Root*)table)->table, NULL, NULL);
}
//! Change the hashing function used by the specified hashtable
/*!
	\param table Hashtable to modify
	\param hashFunc Handler for the hashing function to use
*/
void ILibHashtable_ChangeHashFunc(ILibHashtable table, ILibHashtable_Hash_Func hashFunc)
{
	((ILibHashtable_Root*)table)->hashFunc = hashFunc;
}
//! Change the bucketizer function used by the underlying Sparse Array for the specified Hashtable
/*!
	\param table Hashtable to modify
	\param bucketizer Handler for the bucketizer function to use
*/
void ILibHashtable_ChangeBucketizer(ILibHashtable table, int bucketCount, ILibSparseArray_Bucketizer bucketizer)
{
	if(((ILibHashtable_Root*)table)->table != NULL) {ILibSparseArray_Destroy(((ILibHashtable_Root*)table)->table);}
	((ILibHashtable_Root*)table)->table = ILibSparseArray_Create(bucketCount, bucketizer);
}
ILibHashtable_Node* ILibHashtable_CreateNode(void* Key1, char* Key2, int Key2Len, void* Data)
{
	ILibHashtable_Node *node;

	if((node = (ILibHashtable_Node*)malloc(sizeof(ILibHashtable_Node))) == NULL) {ILIBCRITICALEXIT(254);}
	memset(node, 0, sizeof(ILibHashtable_Node));
	node->Data = Data;
	node->Key1 = Key1;
	node->Key2Len = Key2Len;
	if(Key2Len > 0)
	{
		if((node->Key2 = (char*)malloc(Key2Len))==NULL) {ILIBCRITICALEXIT(254);}
		memcpy_s(node->Key2, Key2Len, Key2, Key2Len);
	}
	return(node);
}

ILibHashtable_Node* ILibHashtable_GetEx(ILibHashtable table, void *Key1, char* Key2, int Key2Len, ILibHashtable_Flags flags)
{
	ILibHashtable_Node *retVal = NULL;
	ILibHashtable_Root* root = (ILibHashtable_Root*)table;
	int hash = root->hashFunc(Key1, Key2, Key2Len);
	ILibHashtable_Node *node = (ILibHashtable_Node*)ILibSparseArray_Get(root->table, hash);

	if(node == NULL)
	{
		// Entry doesn't exist, so create a new entry
		if((flags & ILibHashtable_Flags_ADD) == ILibHashtable_Flags_ADD) 
		{
			retVal = ILibHashtable_CreateNode(Key1, Key2, Key2Len, NULL);
			ILibSparseArray_Add(root->table, hash, retVal);
		}
	}
	else
	{
		// There is a hash entry, so lets enumerate to see if we really have a match
		ILibHashtable_Node *prev = NULL;
		while(node!=NULL)
		{
			if(node->Key1 == Key1 && node->Key2Len == Key2Len && memcmp(node->Key2, Key2, Key2Len)==0)
			{
				break;
			}
			prev = node;
			node = node->next;
		}
		if(node == NULL)
		{
			// There were no matches in the hashes that were returned

			if((flags & ILibHashtable_Flags_ADD) == ILibHashtable_Flags_ADD) 
			{
				// Create a new entry, and append ourself to the hash results
				retVal = ILibHashtable_CreateNode(Key1, Key2, Key2Len, NULL);
				prev->next = retVal;
				retVal->prev = prev;
			}
		}
		else
		{
			// There was a match! Update the value and return the old value
			retVal = node;
			if((flags & ILibHashtable_Flags_REMOVE) == ILibHashtable_Flags_REMOVE) 
			{
				if(node->prev == NULL)
				{
					// This is the first entry, so we'll have to remove the entry from the SparseArray
					ILibSparseArray_Remove(root->table, hash);
				}
				else
				{
					node->prev->next = node->next;
					if(node->next != NULL) {node->next->prev = node->prev;}
				}
			}			
		}
	}
	return(retVal);
}
//! Add/Modify an entry in the Hashtable

//! Key1 and Key2 can both be NULL, but not at the same time.
/*!
	\param table Hashtable to perform the operation on
	\param Key1 Address Key [Can be NULL]
	\param Key2 String Key [Can be NULL]
	\param Key2Len String Key LEngth
	\param Data New Element Value
	\return Old Element Value [NULL if it didn't exist]
*/
void* ILibHashtable_Put(ILibHashtable table, void *Key1, char* Key2, int Key2Len, void* Data)
{	
	ILibHashtable_Node *node = ILibHashtable_GetEx(table, Key1, Key2, Key2Len, ILibHashtable_Flags_ADD);
	void *retVal = node->Data;
	node->Data = Data;
	return(retVal);
}
//! Get the specified Element Value associated with the specified Key(s).

//! Key1 and Key2 can both be NULL, but not at the same time.
/*!
	\param table Hashtable to fetch the value from
	\param Key1 Address Key [Can be NULL]
	\param Key2 String Key [Can be NULL]
	\param Key2Len String Key Length
	\return Element Value [NULL if it doesn't exist]
*/
void* ILibHashtable_Get(ILibHashtable table, void *Key1, char* Key2, int Key2Len)
{
	ILibHashtable_Node *node = ILibHashtable_GetEx(table, Key1, Key2, Key2Len, ILibHashtable_Flags_NONE);
	return(node != NULL ? node->Data : NULL);
}
//! Remove an entry from the hashtable

//! Both Key1 and Key can be NULL, but not at the same time
/*!
	\param table Hashtable to remove the entry from
	\param Key1 Address Key [Can be NULL]
	\param Key2 String Key [Can be NULL]
	\param Key2Len String Key Length
	\return Element Value that was removed [NULL if it didn't exist]
*/
void* ILibHashtable_Remove(ILibHashtable table, void *Key1, char* Key2, int Key2Len)
{
	ILibHashtable_Node *node = ILibHashtable_GetEx(table, Key1, Key2, Key2Len, ILibHashtable_Flags_REMOVE);
	void *retVal = node != NULL ? node->Data : NULL;
	if(node != NULL)
	{
		if(node->Key2 != NULL) {free(node->Key2);}
		free(node);
	}
	return(retVal);
}
//! Use the specified hashtable as a synchronization lock, and acquire it
/*!
	\param table Hashtable to use as a synchronization lock
*/
void ILibHashtable_Lock(ILibHashtable table)
{
	ILibSparseArray_Lock(((ILibHashtable_Root*)table)->table);
}
//! Use the specified hashtable as a synchronization lock, and release it
/*!
	\param table Hashtable to use as a synchronization lock
*/
void ILibHashtable_UnLock(ILibHashtable table)
{
	ILibSparseArray_UnLock(((ILibHashtable_Root*)table)->table);
}


//! Allocates a new SparseArray using the specified number of buckets and the given bucketizer method, which maps indexes to buckets.
/*!
	\param numberOfBuckets Number of buckets to initialize
	\param bucketizer Event handler to be triggered whenever an index value needs to be hashed into a bucket value
	\param userMemorySize Size of extra memory to allocate for user state
	\return ILibSparseArray object
*/
ILibSparseArray ILibSparseArray_CreateWithUserMemory(int numberOfBuckets, ILibSparseArray_Bucketizer bucketizer, int userMemorySize)
{
	ILibSparseArray_Root *retVal = (ILibSparseArray_Root*)ILibMemory_Allocate(sizeof(ILibSparseArray_Root), userMemorySize, NULL, NULL);

	sem_init(&(retVal->LOCK), 0, 1);
	retVal->bucketSize = numberOfBuckets;
	retVal->bucketizer = bucketizer;
	retVal->bucket = (ILibSparseArray_Node*)malloc(numberOfBuckets * sizeof(ILibSparseArray_Node));
	retVal->userMemorySize = userMemorySize;
	memset(retVal->bucket, 0, numberOfBuckets * sizeof(ILibSparseArray_Node));

	return(retVal);
}

//! Allocates a new SparseArray using the same parameters as an existing SparseArray
/*!
	\param source The ILibSparseArray to duplicate parameters from
	\return New ILibSparseArray object
*/
ILibSparseArray ILibSparseArray_CreateEx(ILibSparseArray source)
{
	ILibSparseArray retVal = ILibSparseArray_CreateWithUserMemory(((ILibSparseArray_Root*)source)->bucketSize, ((ILibSparseArray_Root*)source)->bucketizer, ((ILibSparseArray_Root*)source)->userMemorySize);
	return(retVal);
}


int ILibSparseArray_Comparer(void *obj1, void *obj2)
{
	if(((ILibSparseArray_Node*)obj1)->index == ((ILibSparseArray_Node*)obj2)->index)
	{
		return(0);
	}
	else
	{
		return(((ILibSparseArray_Node*)obj1)->index < ((ILibSparseArray_Node*)obj2)->index ? -1 : 1);
	}
}
//! Populates the "index" in the SparseArray. If that index is already defined, the new value is saved, and the old value is returned. 
/*!
	\param sarray Sparse Array Object
	\param index Index to initialize/set
	\param data Value to set to the index
	\return Old value of index if it was initialized, NULL otherwise
*/
void* ILibSparseArray_Add(ILibSparseArray sarray, int index, void *data)
{
	void* retVal = NULL;
	ILibSparseArray_Root *root = (ILibSparseArray_Root*)sarray;
	int i = root->bucketizer(index);

	if(root->bucket[i].index == 0 && root->bucket[i].ptr == NULL)
	{
		// No Entry Exists
		root->bucket[i].index = index;
		root->bucket[i].ptr = data;
	}
	else if(root->bucket[i].index < 0)
	{
		// Need to use Linked List
		ILibSparseArray_Node* n = (ILibSparseArray_Node*)malloc(sizeof(ILibSparseArray_Node));
		n->index = index;
		n->ptr = data;
		n = (ILibSparseArray_Node*)ILibLinkedList_SortedInsert(root->bucket[i].ptr, &ILibSparseArray_Comparer, n);
		if(n!=NULL)
		{
			// This duplicates an entry already in the list... Updated with new value, pass back the old
			retVal = n->ptr;
			free(n);
		}
	}
	else 
	{
		// No Linked List... We either need to create one, or check to see if this is possibly a duplicate entry
		if(root->bucket[i].index == index)
		{
			// Today's our lucky day! We can just replace the value! (No return value)
			retVal = root->bucket[i].ptr;
			root->bucket[i].ptr = data;
		}
		else
		{
			// We need to create a linked list, add the old value, then insert our new value (No return value)
			ILibSparseArray_Node* n = (ILibSparseArray_Node*)malloc(sizeof(ILibSparseArray_Node));
			n->index = root->bucket[i].index;
			n->ptr = root->bucket[i].ptr;

			root->bucket[i].index = -1;
			root->bucket[i].ptr = ILibLinkedList_Create();
			ILibLinkedList_AddHead(root->bucket[i].ptr, n);

			n = (ILibSparseArray_Node*)malloc(sizeof(ILibSparseArray_Node));
			n->index = index;
			n->ptr = data;
			ILibLinkedList_SortedInsert(root->bucket[i].ptr, &ILibSparseArray_Comparer, n);
		}
	}
	return(retVal);
}

void* ILibSparseArray_GetEx(ILibSparseArray sarray, int index, int remove)
{
	ILibSparseArray_Root *root = (ILibSparseArray_Root*)sarray;
	void *retVal = NULL;
	int i = root->bucketizer(index);

	if(root->bucket[i].index == index)
	{
		// Direct Match
		retVal = root->bucket[i].ptr;
		if(remove!=0)
		{
			root->bucket[i].ptr = NULL;
			root->bucket[i].index = 0;
		}
	}
	else if(root->bucket[i].index < 0)
	{
		// Need to check the Linked List
		void *listNode = ILibLinkedList_GetNode_Search(root->bucket[i].ptr, &ILibSparseArray_Comparer, (void*)&index);
		retVal = listNode != NULL ? ((ILibSparseArray_Node*)ILibLinkedList_GetDataFromNode(listNode))->ptr : NULL;
		if(remove!=0 && listNode!=NULL)
		{
			free(ILibLinkedList_GetDataFromNode(listNode));
			ILibLinkedList_Remove(listNode);			
			if(ILibLinkedList_GetCount(root->bucket[i].ptr)==0)
			{
				ILibLinkedList_Destroy(root->bucket[i].ptr);
				root->bucket[i].ptr = NULL;
				root->bucket[i].index = 0;
			}
		}
	}
	else
	{
		// Whatever is in this bucket doesn't match what we're looking for
		retVal = NULL;
	}
	return(retVal);
}
//! Fetches the value at the index. NULL if the index is not defined
/*!
	\param sarray Sparse Array Object
	\param index Index to fetch
	\return Value at specified index, NULL if it was not defined
*/
void* ILibSparseArray_Get(ILibSparseArray sarray, int index)
{
	return(ILibSparseArray_GetEx(sarray, index, 0));
}
//! Removes an index from the SparseArray (if it exists), and returns the associated value if it does exist.
/*!
	\param sarray Sparse Array Object
	\param index Index value to remove
	\return Value defined at Index, NULL if it was not defined
*/
void* ILibSparseArray_Remove(ILibSparseArray sarray, int index)
{
	return(ILibSparseArray_GetEx(sarray, index, 1));
}
//! Clones the contents of the given Sparse Array into a new Sparse Array
/*!
	\param sarray The Sparse Array to clone
	\return Cloned Sparse Array
*/
ILibSparseArray ILibSparseArray_Move(ILibSparseArray sarray)
{
	ILibSparseArray_Root *root = (ILibSparseArray_Root*)sarray;
	ILibSparseArray_Root *retVal = (ILibSparseArray_Root*)ILibSparseArray_CreateEx(sarray);
	memcpy_s(retVal->bucket, root->bucketSize * sizeof(ILibSparseArray_Node), root->bucket, root->bucketSize * sizeof(ILibSparseArray_Node));
	return(retVal);
}
//! Clears the SparseArray, and dispatches an event for each defined index
/*!
	\param sarray Sparse Array Object
	\param onClear Event to dispatch for defined indexes
	\param user Custom user state data
	\param nonZeroWillDelete If this is zero, will just enumerato, otherwise will delete
*/
void ILibSparseArray_ClearEx2(ILibSparseArray sarray, ILibSparseArray_OnValue onClear, void *user, int nonZeroWillDelete)
{
	ILibSparseArray_Root *root = (ILibSparseArray_Root*)sarray;
	int i;

	for(i=0;i<root->bucketSize;++i)
	{
		if(root->bucket[i].ptr != NULL)
		{
			if(root->bucket[i].index < 0)
			{
				// There is a linked list here
				void *node = ILibLinkedList_GetNode_Head(root->bucket[i].ptr);
				while(node != NULL)
				{
					ILibSparseArray_Node *sn = (ILibSparseArray_Node*)ILibLinkedList_GetDataFromNode(node);
					if(onClear!=NULL) { onClear(sarray, sn->index, sn->ptr, user);}
					if (nonZeroWillDelete != 0) { free(sn); }
					node = ILibLinkedList_GetNextNode(node);
				}
				if (nonZeroWillDelete != 0) { ILibLinkedList_Destroy(root->bucket[i].ptr); }
			}
			else
			{
				// No Linked List, just a direct map
				if(onClear!=NULL)
				{
					onClear(sarray, root->bucket[i].index, root->bucket[i].ptr, user);
				}
			}
			if (nonZeroWillDelete != 0)
			{
				root->bucket[i].index = 0;
				root->bucket[i].ptr = NULL;
			}
		}
	}
}
//! Frees the resources used by an ILibSparseArray, and dispatches an event for each defined index
/*!
	\param sarray Sparse Array Object
	\param onDestroy Event handler to dispatch for each defined entry
	\param user Custom user state data
*/
void ILibSparseArray_DestroyEx(ILibSparseArray sarray, ILibSparseArray_OnValue onDestroy, void *user)
{
	ILibSparseArray_ClearEx(sarray, onDestroy, user);
	sem_destroy(&((ILibSparseArray_Root*)sarray)->LOCK);
	free(((ILibSparseArray_Root*)sarray)->bucket);
	free(sarray);
}
//! Frees the resources used by an ILibSparseArray
/*!
	\param sarray Sparse Array Object to free
*/
void ILibSparseArray_Destroy(ILibSparseArray sarray)
{
	ILibSparseArray_DestroyEx(sarray, NULL, NULL);
}
//! Use the Sparse Array as a synchronization lock, and acquire it
/*!
	\param sarray Sparse Array to use as a synchronization lock
*/
void ILibSparseArray_Lock(ILibSparseArray sarray)
{
	sem_wait(&(((ILibSparseArray_Root*)sarray)->LOCK));
}
//! Use the Sparse Array as a synchronization lock, and release it
/*!
	\param sarray Sparse Array to use as a synchronization lock
*/
void ILibSparseArray_UnLock(ILibSparseArray sarray)
{
	sem_post(&(((ILibSparseArray_Root*)sarray)->LOCK));
}

int ILibString_IndexOfFirstWhiteSpace(const char *inString, int inStringLength)
{
	//CR, LF, space, tab
	int i = 0;
	for(i = 0;i < inStringLength; ++i)
	{
		if (inString[i] == 13 || inString[i] == 10 || inString[i] == 9 || inString[i] == 32) return(i);
	}
	return(-1);
}

/*! \fn ILibString_EndsWithEx(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength, int caseSensitive)
\brief Determines if a string ends with a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param endWithString The substring to match
\param endWithStringLength The length of \a startsWithString
\param caseSensitive 0 if the matching is case-insensitive
\return Non-zero if the string starts with the substring
*/
int ILibString_EndsWithEx(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength, int caseSensitive)
{
	int RetVal = 0;
	if (inStringLength < 0) { inStringLength = (int)strnlen_s(inString, sizeof(ILibScratchPad)); }
	if (endWithStringLength < 0) { endWithStringLength = (int)strnlen_s(endWithString, sizeof(ILibScratchPad)); }

	if (inStringLength>=endWithStringLength)
	{
		if (caseSensitive!=0 && memcmp(inString+inStringLength-endWithStringLength,endWithString,endWithStringLength)==0) RetVal = 1;
		else if (caseSensitive==0 && strncasecmp(inString+inStringLength-endWithStringLength,endWithString,endWithStringLength)==0) RetVal = 1;
	}
	return(RetVal);
}
/*! \fn ILibString_EndsWith(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength)
\brief Determines if a string ends with a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param endWithString The substring to match
\param endWithStringLength The length of \a startsWithString
\return Non-zero if the string starts with the substring
*/
int ILibString_EndsWith(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength)
{
	return(ILibString_EndsWithEx(inString,inStringLength,endWithString,endWithStringLength,1));
}
/*! \fn ILibString_StartsWithEx(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength, int caseSensitive)
\brief Determines if a string starts with a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param startsWithString The substring to match
\param startsWithStringLength The length of \a startsWithString
\param caseSensitive Non-zero if match is to be case sensitive
\return Non-zero if the string starts with the substring
*/
int ILibString_StartsWithEx(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength, int caseSensitive)
{
	int RetVal = 0;
	if (inStringLength>=startsWithStringLength)
	{
		if (caseSensitive!=0 && memcmp(inString,startsWithString,startsWithStringLength)==0) RetVal = 1;
		else if (caseSensitive==0 && strncasecmp(inString,startsWithString,startsWithStringLength)==0) RetVal = 1;
	}
	return(RetVal);
}
/*! \fn ILibString_StartsWith(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength)
\brief Determines if a string starts with a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param startsWithString The substring to match
\param startsWithStringLength The length of \a startsWithString
\return Non-zero if the string starts with the substring
*/
int ILibString_StartsWith(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength)
{
	return(ILibString_StartsWithEx(inString,inStringLength,startsWithString,startsWithStringLength,1));
}
/*! \fn ILibString_IndexOfEx(const char *inString, int inStringLength, const char *indexOf, int indexOfLength,  int caseSensitive)
\brief Returns the position index of the first occurance of a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param indexOf The substring to search for
\param indexOfLength The length of \a lastIndexOf
\param caseSensitive Non-zero if the match is case sensitive
\return Position index of first occurance. -1 if the substring is not found
*/
int ILibString_IndexOfEx(const char *inString, int inStringLength, const char *indexOf, int indexOfLength,  int caseSensitive)
{
	int RetVal = -1;
	int index = 0;

	while (inStringLength-index >= indexOfLength)
	{
		if (caseSensitive!=0 && memcmp(inString+index,indexOf,indexOfLength)==0)
		{
			RetVal = index;
			break;
		}
		else if (caseSensitive==0 && strncasecmp(inString+index,indexOf,indexOfLength)==0)
		{
			RetVal = index;
			break;
		}
		++index;
	}
	return(RetVal);
}
/*! \fn ILibString_IndexOf(const char *inString, int inStringLength, const char *indexOf, int indexOfLength)
\brief Returns the position index of the first occurance of a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param indexOf The substring to search for
\param indexOfLength The length of \a lastIndexOf
\return Position index of first occurance. -1 if the substring is not found
*/
int ILibString_IndexOf(const char *inString, int inStringLength, const char *indexOf, int indexOfLength)
{
	return(ILibString_IndexOfEx(inString,inStringLength,indexOf,indexOfLength,1));
}
/*! \fn ILibString_LastIndexOfEx(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength, int caseSensitive)
\brief Returns the position index of the last occurance of a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param lastIndexOf The substring to search for
\param lastIndexOfLength The length of \a lastIndexOf
\param caseSensitive 0 for case insensitive matching, non-zero for case-sensitive matching
\return Position index of last occurance. -1 if the substring is not found
*/
int ILibString_LastIndexOfEx(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength, int caseSensitive)
{
	int RetVal = -1;
	int index = (inStringLength < 0 ? (int)strnlen_s(inString, sizeof(ILibScratchPad)) : inStringLength) - (lastIndexOfLength < 0 ? (int)strnlen_s(lastIndexOf, sizeof(ILibScratchPad)) : lastIndexOfLength);

	while (index >= 0)
	{
		if (caseSensitive!=0 && memcmp(inString+index,lastIndexOf,lastIndexOfLength)==0)
		{
			RetVal = index;
			break;
		}
		else if (caseSensitive==0 && strncasecmp(inString+index,lastIndexOf,lastIndexOfLength)==0)
		{
			RetVal = index;
			break;
		}
		--index;
	}
	return(RetVal);
}
/*! \fn ILibString_LastIndexOf(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength)
\brief Returns the position index of the last occurance of a given substring
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param lastIndexOf The substring to search for
\param lastIndexOfLength The length of \a lastIndexOf
\return Position index of last occurance. -1 if the substring is not found
*/
int ILibString_LastIndexOf(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength)
{
	return(ILibString_LastIndexOfEx(inString,inStringLength,lastIndexOf,lastIndexOfLength,1));
}
/*! \fn ILibString_Replace(const char *inString, int inStringLength, const char *replaceThis, int replaceThisLength, const char *replaceWithThis, int replaceWithThisLength)
\brief Replaces all occurances of a given substring that occur in the input string with another substring
\par
\b Note: \a Returned string must be freed
\param inString Pointer to char* to process
\param inStringLength The length of \a inString
\param replaceThis The substring to replace
\param replaceThisLength The length of \a replaceThis
\param replaceWithThis The substring to substitute for \a replaceThis
\param replaceWithThisLength The length of \a replaceWithThis
\return New string with replaced values
*/
char *ILibString_Replace(const char *inString, int inStringLength, const char *replaceThis, int replaceThisLength, const char *replaceWithThis, int replaceWithThisLength)
{
	char *RetVal;
	int RetValLength;
	struct parser_result *pr;
	struct parser_result_field *prf;
	int mallocSize;

	pr = ILibParseString((char*)inString, 0, inStringLength,(char*)replaceThis,replaceThisLength);
	RetValLength = (pr->NumResults-1) * replaceWithThisLength; // string that will be inserted
	RetValLength += (inStringLength - ((pr->NumResults-1) * replaceThisLength)); // Add the length of the rest of the string
	mallocSize = RetValLength + 1;

	if ((RetVal = (char*)malloc(mallocSize)) == NULL) ILIBCRITICALEXIT(254);
	RetVal[RetValLength]=0;

	RetValLength = 0;
	prf = pr->FirstResult;
	while (prf != NULL)
	{
		memcpy_s(RetVal + RetValLength, mallocSize - RetValLength, prf->data, prf->datalength);
		RetValLength += prf->datalength;

		if (prf->NextResult!=NULL)
		{
			memcpy_s(RetVal + RetValLength, mallocSize - RetValLength, (char*)replaceWithThis, replaceWithThisLength);
			RetValLength += replaceWithThisLength;
		}
		prf = prf->NextResult;
	}
	ILibDestructParserResults(pr);
	return(RetVal);
}
char* ILibString_Cat_s(char *destination, size_t destinationSize, char *source)
{
	size_t sourceLen = strnlen_s(source, destinationSize);
	int i;
	int x = -1;
	for (i = 0; i < (int)destinationSize - 1; ++i)
	{
		if (destination[i] == 0) { x = i; break; }
	}
	if (x < 0 || ((x + sourceLen + 1 )> destinationSize)) { ILIBCRITICALEXIT(254); }
	memcpy_s(destination + x, destinationSize - x, source, sourceLen);
	destination[x + sourceLen] = 0;
	return(destination);
}
#ifndef WIN32
#ifdef ILib_need_sprintf_s
int sprintf_s(void *dest, size_t destSize, char *format, ...)
{
	int len = 0;
	va_list argptr;

	va_start(argptr, format);
	len += vsnprintf(dest + len, destSize - len, format, argptr);
	va_end(argptr);

	return(len < destSize ? len : -1);
}
#endif
int ILibMemory_Copy_s(void *destination, size_t destinationSize, void *source, size_t sourceLength)
{
	if (destinationSize >= sourceLength)
	{
		memcpy(destination, source, sourceLength);
	}
	else
	{
		ILIBCRITICALEXIT(254);
	}
	return(0);
}
int ILibMemory_Move_s(void *destination, size_t destinationSize, void *source, size_t sourceLength)
{
	if (destinationSize >= sourceLength)
	{
		memmove(destination, source, sourceLength);
	}
	else
	{
		ILIBCRITICALEXIT(254);
	}
	return(0);
}
#endif
int ILibString_n_Copy_s(char *destination, size_t destinationSize, char *source, size_t maxCount)
{
	size_t count = strnlen_s(source, maxCount == (size_t)-1 ? (destinationSize-1) : maxCount);
	if ((count + 1) > destinationSize) 
	{
		ILIBCRITICALEXIT(254); 
	}
	memcpy_s(destination, destinationSize, source, count);
	destination[count] = 0;
	return(0);
}
int ILibString_Copy_s(char *destination, size_t destinationSize, char *source)
{
	size_t sourceLen = strnlen_s(source, destinationSize);

	if ((sourceLen + 1) > destinationSize) { ILIBCRITICALEXIT(254); }
	memcpy_s(destination, destinationSize, source, sourceLen);
	destination[sourceLen] = 0;
	return(0);
}

char* ILibString_Cat(const char *inString1, int inString1Len, const char *inString2, int inString2Len)
{
	char *RetVal;
	if (inString1Len < 0) { inString1Len = (int)strnlen_s(inString1, sizeof(ILibScratchPad)); }
	if (inString2Len < 0) { inString2Len = (int)strnlen_s(inString2, sizeof(ILibScratchPad)); }
	if ((RetVal = (char*)malloc(inString1Len + inString2Len+1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(RetVal, inString1Len + inString2Len + 1, (char*)inString1, inString1Len);
	memcpy_s(RetVal + inString1Len, inString2Len + 1, (char*)inString2, inString2Len);
	RetVal[inString1Len + inString2Len]=0;
	return(RetVal);
}
char* ILibString_Copy(const char *inString, int length)
{
	char *RetVal;
	if (length<0) length = (int)strnlen_s(inString, sizeof(ILibScratchPad));
	if ((RetVal = (char*)malloc(length + 1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy_s(RetVal, length + 1, (char*)inString, length);
	RetVal[length] = 0;
	return(RetVal);
}
/*! \fn ILibString_ToUpper(const char *inString, int length)
\brief Coverts the given string to upper case
\par
\b Note: \a Returned string must be freed
\param inString Pointer to char* to convert to upper case
\param length The length of \a inString
\return Converted string
*/
char *ILibString_ToUpper(const char *inString, int length)
{
	char *RetVal;
	if ((RetVal = (char*)malloc(length + 1)) == NULL) ILIBCRITICALEXIT(254);
	RetVal[length]=0;
	ILibToUpper(inString, length, RetVal);
	return(RetVal);
}
/*! \fn ILibString_ToLower(const char *inString, int length)
\brief Coverts the given string to lower case
\par
\b Note: \a Returned string must be freed
\param inString Pointer to char* to convert to lower case
\param length The length of \a inString
\return Converted string
*/
char *ILibString_ToLower(const char *inString, int length)
{
	char *RetVal;
	if ((RetVal = (char*)malloc(length + 1)) == NULL) ILIBCRITICALEXIT(254);
	RetVal[length] = 0;
	ILibToLower(inString, length, RetVal);
	return(RetVal);
}
/*! \fn ILibReadFileFromDiskEx(char **Target, char *FileName)
\brief Reads a file into a char *
\par
\b Note: \a Target must be freed
\param Target Pointer to char* that will contain the data
\param FileName Filename of the file to read
\return length of the data read
*/
int ILibReadFileFromDiskEx(char **Target, char *FileName)
{
	char *buffer;
	int SourceFileLength;
	FILE *SourceFile = NULL;

#ifdef WIN32
	fopen_s(&SourceFile, FileName, "rb");
#else
	SourceFile = fopen(FileName, "rb");
#endif
	if (SourceFile == NULL) return(0);

	fseek(SourceFile, 0, SEEK_END);
	SourceFileLength = (int)ftell(SourceFile);
	fseek(SourceFile, 0, SEEK_SET);
	if ((buffer = (char*)malloc(SourceFileLength)) == NULL) ILIBCRITICALEXIT(254);
	SourceFileLength = (int)fread(buffer, sizeof(char), (size_t)SourceFileLength,SourceFile);
	fclose(SourceFile);

	*Target = buffer;
	return(SourceFileLength);
}


/*! \fn ILibReadFileFromDisk(char *FileName)
\brief Reads a file into a char *
\par
\b Note: Data must be null terminated
\param FileName Filename of the file to read
\return data read
*/
char *ILibReadFileFromDisk(char *FileName)
{
	char *RetVal;
	ILibReadFileFromDiskEx(&RetVal,FileName);
	return(RetVal);
}

/*! \fn ILibWriteStringToDisk(char *FileName, char *data)
\brief Writes a null terminated string to disk
\par
\b Note: Files that already exist will be overwritten
\param FileName Filename of the file to write
\param data data to write
*/
void ILibWriteStringToDisk(char *FileName, char *data)
{
	ILibWriteStringToDiskEx(FileName, data, (int)strnlen_s(data, 65535));
}
void ILibWriteStringToDiskEx(char *FileName, char *data, int dataLen)
{
	FILE *SourceFile = NULL;

#ifdef WIN32
	fopen_s(&SourceFile, FileName, "wb");
#else
	SourceFile = fopen(FileName, "wb");
#endif
	
	if (SourceFile != NULL)
	{
		fwrite(data, sizeof(char), dataLen, SourceFile);
		fclose(SourceFile);
	}
}
void ILibAppendStringToDiskEx(char *FileName, char *data, int dataLen)
{
	FILE *SourceFile = NULL;

#ifdef WIN32
	fopen_s(&SourceFile, FileName, "ab");
#else
	SourceFile = fopen(FileName, "ab");
#endif

	if (SourceFile != NULL)
	{
		fwrite(data, sizeof(char), dataLen, SourceFile);
		fclose(SourceFile);
	}
}
void ILibDeleteFileFromDisk(char *FileName)
{
#if defined(WIN32) || defined(_WIN32_WCE)
	DeleteFile((LPCTSTR)FileName);
#elif defined(_POSIX) || defined (__APPLE__)
	remove(FileName);
#endif
}

void ILibGetDiskFreeSpace(void *i64FreeBytesToCaller, void *i64TotalBytes)
{
#if defined (WIN32)
	GetDiskFreeSpaceEx (NULL, (PULARGE_INTEGER)i64FreeBytesToCaller, (PULARGE_INTEGER)i64TotalBytes, NULL);
#elif defined (_VX_CPU)
	*((uint64_t *)i64FreeBytesToCaller) = 0;
	*((uint64_t *)i64TotalBytes)= 0;
#elif  (defined (_POSIX) || defined (__APPLE__)) && !defined(NACL)
	struct statfs stfs;
	statfs(".", &stfs);
	*((uint64_t *)i64FreeBytesToCaller) = (uint64_t)stfs.f_bavail * stfs.f_bsize;
	*((uint64_t *)i64TotalBytes)= (uint64_t)stfs.f_blocks * stfs.f_bsize;
#endif
}
long ILibGetTimeStamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}
int ILibIsLittleEndian() { int v = 1; return (((char*)&v)[0] == 1 ? 1 : 0); }
int ILibGetCurrentTimezoneOffset_Minutes()
{
#if defined(_WIN32)
	int offset;
	int dl = 0;
	long tz = 0;

	tzset();
	_get_timezone(&tz);
	offset = -1 * ((int)tz / 60);
	_get_daylight(&dl);
	offset += (dl * 60);
	return offset;
#elif defined(_WIN32_WCE)
	SYSTEMTIME st_utc, st_local;
	FILETIME ft_utc,ft_local;
	ULARGE_INTEGER u,l;

	//
	// WinCE doesn't support tzset(), so we need to instead
	// just compare SystemTime and LocalTime
	//
	GetSystemTime(&st_utc);
	GetLocalTime(&st_local);

	SystemTimeToFileTime(&st_utc,&ft_utc);
	SystemTimeToFileTime(&st_local,&ft_local);

	u.HighPart = ft_utc.dwHighDateTime;
	u.LowPart = ft_utc.dwLowDateTime;

	l.HighPart = ft_local.dwHighDateTime;
	l.LowPart = ft_local.dwLowDateTime;

	if (l.QuadPart > u.QuadPart)
	{
		return((int)((l.QuadPart - u.QuadPart ) / 10000000 / 60));
	}
	else
	{
		return(-1*(int)((u.QuadPart - l.QuadPart ) / 10000000 / 60));
	}
#elif defined(_ANDROID)
	return 0; // TODO
#else
	int offset;

	tzset();
	offset = -1 * ((int)timezone / 60);
	offset += (daylight * 60);
	return offset;
#endif
}
int ILibIsDaylightSavingsTime()
{
#if defined(_WIN32)
	int dl = 0;
	tzset();
	_get_daylight(&dl);
	return dl;
#elif defined(_WIN32_WCE)
	//
	// WinCE doesn't support this. We'll just pretend it's never daylight savings,
	// because under WinCE we only work with Local and System time.
	//
	return 0;
#elif defined(_ANDROID)
	return 0; // TODO
#else
	tzset();
	return daylight;
#endif
}


int ILibGetLocalTime(char *dest, int destLen)
{
	int retVal;
	struct timeval  tv;
#ifdef WIN32
	struct tm t;
#endif
	gettimeofday(&tv, NULL);
#ifdef WIN32
	localtime_s(&t, (time_t*)&(tv.tv_sec));
	retVal = (int)strftime(dest, destLen, "%Y-%m-%d %I:%M:%S %p", &t);
#else
	retVal = (int)strftime(dest, destLen, "%F %r", localtime((time_t*)(&(tv.tv_sec))));
#endif
	return(retVal);
}

#if defined(WIN32) || defined(_WIN32_WCE)
void ILibGetTimeOfDay(struct timeval *tp)
{
#if defined(_WIN32_WCE)
	//
	// WinCE Specific
	//
	SYSTEMTIME st;
	FILETIME ft;
	ULARGE_INTEGER currentTime,epochTime;
	DWORD EPOCH_HI = 27111902;
	DWORD EPOCH_LOW = 3577643008;
	ULONGLONG posixTime;

	//
	// Get the current time, in UTC
	//
	GetSystemTime((&st));
	SystemTimeToFileTime(&st,&ft);

	memset(&currentTime,0,sizeof(ULARGE_INTEGER));
	memset(&epochTime,0,sizeof(ULARGE_INTEGER));

	currentTime.LowPart = ft.dwLowDateTime;
	currentTime.HighPart = ft.dwHighDateTime;

	epochTime.LowPart = EPOCH_LOW;
	epochTime.HighPart = EPOCH_HI;

	//
	// WinCE represents time as number of 100ns intervals since Jan 1, 1601
	// But Posix represents it as number of sec/usec since Jan 1, 1970
	//
	posixTime = currentTime.QuadPart - epochTime.QuadPart;

	tp->tv_sec = (long)(posixTime / 10000000); // 100 ns intervals to second intervals
	tp->tv_usec = (long)((posixTime / 10) - (tp->tv_sec * 1000000)); 
#else
	__time64_t t;
	_time64(&t);
	tp->tv_usec = 0;
	tp->tv_sec = (long)t;
#endif
}
#endif

#ifdef _MINCORE
static long long ILibGetUptimeUpperEmulation1;
static long long ILibGetUptimeUpperEmulation2;
long long ILibGetUptime()
{
	// Windows XP with rollover prevention
	long long r;
	r = (long long)GetTickCount();
	if (r < ILibGetUptimeUpperEmulation1) ILibGetUptimeUpperEmulation2 += ((long long)1) << 32;
	ILibGetUptimeUpperEmulation1 = r;
	r += ILibGetUptimeUpperEmulation2;
	return r;
}
#elif WIN32
static int ILibGetUptimeFirst = 1;
static ULONGLONG (*pILibGetUptimeGetTickCount64)() = NULL;
static long long ILibGetUptimeUpperEmulation1;
static long long ILibGetUptimeUpperEmulation2;
long long ILibGetUptime()
{
	long long r;

	// Windows 7 & Vista
    if (ILibGetUptimeFirst) {
        HMODULE hlib = LoadLibrary(TEXT("KERNEL32.DLL"));
		if (hlib == NULL) return 0;
        pILibGetUptimeGetTickCount64 = (ULONGLONG(*)())GetProcAddress(hlib, "GetTickCount64");
        ILibGetUptimeFirst = 0;
    }
    if (pILibGetUptimeGetTickCount64 != NULL) return pILibGetUptimeGetTickCount64(); 

	// Windows XP with rollover prevention
	r = (long long)GetTickCount();  // Static Analyser reports this could roll over, but that's why this block is doing rollover prevention
	if (r < ILibGetUptimeUpperEmulation1) ILibGetUptimeUpperEmulation2 += ((long long)1) << 32;
	ILibGetUptimeUpperEmulation1 = r;
	r += ILibGetUptimeUpperEmulation2;
	return r;
}
#elif __APPLE__
long long ILibGetUptime()
{
	int mib[2];
	size_t size;
	struct timeval ts;
	time_t now;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof(ts);
	(void)time(&now);
	if (sysctl(mib, 2, &ts, &size, NULL, 0) == -1) ILIBCRITICALEXIT(254);
	return ((long long)(now - ts.tv_sec)) * 1000;
}
#elif _MIPS
long long ILibGetUptime()
{
	// On MIPS routers, we just used the current clock. This could messup if the router's clock is changed.
	time_t t;
	time(&t);
	return (t * 1000);
}
#elif NACL
//need to impl
long long ILibGetUptime()
{
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	
	long long ret =  (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
	return ret;
}
#else
long long ILibGetUptime()
{
	struct timespec ts; 
	memset(&ts, 0, sizeof ts);
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (((long long)ts.tv_sec) * 1000) + ((((long long)ts.tv_nsec) / 1000) % 1000);
}
#endif

int ILibReaderWriterLock_ReadLock(ILibReaderWriterLock rwLock)
{
	struct ILibReaderWriterLock_Data *rw = (struct ILibReaderWriterLock_Data*)rwLock;
	int BlockRead = 0;
	int RetVal = 0;

	int AlreadyExiting = 0;

	sem_wait(&(rw->CounterLock));
	AlreadyExiting = rw->Exit;
	if (rw->Exit==0)
	{
		BlockRead = rw->PendingWriters>0;
		if (BlockRead==0)
		{
			++rw->ActiveReaders;
		}
		else
		{
			++rw->WaitingReaders;
		}
	}
	sem_post(&(rw->CounterLock));

	if (BlockRead!=0)
	{
		sem_wait(&(rw->ReadLock));
	}

	//
	// We made it through!
	//
	RetVal = rw->Exit;
	if (AlreadyExiting == 0) { sem_post(&(rw->ExitLock)); }
	return(RetVal);
}
int ILibReaderWriterLock_ReadUnLock(ILibReaderWriterLock rwLock)
{
	struct ILibReaderWriterLock_Data *rw = (struct ILibReaderWriterLock_Data*)rwLock;
	int SignalWriter = 0;

	sem_wait(&(rw->CounterLock));

	if (rw->Exit!=0)
	{
		//
		// Unlock, and just abort
		//
		sem_post(&(rw->CounterLock));
		return(1);
	}


	--rw->ActiveReaders;
	SignalWriter = (rw->ActiveReaders==0 && rw->PendingWriters>0);
	sem_post(&(rw->CounterLock));

	if (SignalWriter) { sem_post(&(rw->WriteLock)); }

	sem_wait(&(rw->ExitLock)); // This won't block. We're just keeping the semaphore value stable.
	return(0);
}
int ILibReaderWriterLock_WriteLock(ILibReaderWriterLock rwLock)
{
	struct ILibReaderWriterLock_Data *rw = (struct ILibReaderWriterLock_Data*)rwLock;
	int BlockWrite = 0;
	int RetVal = 0;
	int AlreadyExiting = 0;

	sem_wait(&(rw->CounterLock));
	AlreadyExiting = rw->Exit;
	if (rw->Exit==0)
	{
		++rw->PendingWriters;
		BlockWrite = (rw->ActiveReaders>0 || rw->PendingWriters>1);
	}
	sem_post(&(rw->CounterLock));

	if (BlockWrite)
	{
		sem_wait(&(rw->WriteLock));
	}

	//
	// We made it through!
	//
	RetVal = rw->Exit;
	if (AlreadyExiting==0)
	{
		sem_post(&(rw->ExitLock));
	}
	return(RetVal);
}
int ILibReaderWriterLock_WriteUnLock(ILibReaderWriterLock rwLock)
{
	struct ILibReaderWriterLock_Data *rw = (struct ILibReaderWriterLock_Data*)rwLock;
	int SignalAnotherWriter = 0;
	int SignalReaders = 0;

	sem_wait(&(rw->CounterLock));

	if (rw->Exit!=0)
	{
		//
		// Unlock, and just abort
		//
		sem_post(&(rw->CounterLock));
		return(1);
	}

	--rw->PendingWriters;
	if (rw->PendingWriters>0)
	{
		//
		// Still more Writers, so flag it.
		//
		SignalAnotherWriter=1;
	}
	else
	{
		//
		// No more writers, so we can flag the waiting readers
		//
		while (rw->WaitingReaders>0)
		{
			--rw->WaitingReaders;
			++rw->ActiveReaders;
			++SignalReaders;
		}
	}
	sem_post(&(rw->CounterLock));

	if (SignalAnotherWriter)
	{
		sem_post(&(rw->WriteLock));
	}
	else
	{
		while (SignalReaders>0)
		{
			--SignalReaders;
			sem_post(&(rw->ReadLock));
		}
	}

	sem_wait(&(rw->ExitLock)); // This won't block. We're just keeping the semaphore value stable.
	return(0);
}
void ILibReaderWriterLock_Destroy2(ILibReaderWriterLock rwLock)
{
	struct ILibReaderWriterLock_Data *rw = (struct ILibReaderWriterLock_Data*)rwLock;

	int NumberOfWaitingLocks = 0;

	sem_wait(&(rw->CounterLock));
	rw->Exit = 1;

	NumberOfWaitingLocks += rw->WaitingReaders;
	if (rw->PendingWriters>1)
	{
		NumberOfWaitingLocks += (rw->PendingWriters-1);
	}

	while (rw->WaitingReaders>0)
	{
		sem_post(&(rw->ReadLock));
		--rw->WaitingReaders;
	}
	while (rw->PendingWriters>1)
	{
		sem_post(&(rw->WriteLock));
		--rw->PendingWriters;
	}
	sem_post(&(rw->CounterLock));


	while (NumberOfWaitingLocks>0)
	{
		sem_wait(&(rw->ExitLock));
		--NumberOfWaitingLocks;
	}

	sem_destroy(&(rw->ReadLock));
	sem_destroy(&(rw->WriteLock));
	sem_destroy(&(rw->CounterLock));
	sem_destroy(&(rw->ExitLock));
}
void ILibReaderWriterLock_Destroy(ILibReaderWriterLock rwLock)
{
	ILibReaderWriterLock_Destroy2(rwLock);
	free(rwLock);
}
ILibReaderWriterLock ILibReaderWriterLock_Create()
{
	struct ILibReaderWriterLock_Data *RetVal;
	if ((RetVal = (struct ILibReaderWriterLock_Data*)malloc(sizeof(struct ILibReaderWriterLock_Data))) == NULL) ILIBCRITICALEXIT(254);
	memset(RetVal,0,sizeof(struct ILibReaderWriterLock_Data));

	sem_init(&(RetVal->CounterLock),0,1);
	sem_init(&(RetVal->ReadLock),0,0);
	sem_init(&(RetVal->WriteLock),0,0);
	sem_init(&(RetVal->ExitLock),0,0);
	RetVal->Destroy = &ILibReaderWriterLock_Destroy2;
	return((ILibReaderWriterLock)RetVal);
}
ILibReaderWriterLock ILibReaderWriterLock_CreateEx(void *chain)
{
	struct ILibReaderWriterLock_Data *RetVal = (struct ILibReaderWriterLock_Data*)ILibReaderWriterLock_Create();
	ILibChain_SafeAdd(chain,RetVal);
	return(RetVal);
}


char* ILibTime_Serialize(time_t timeVal)
{
#if defined(_WIN32_WCE)
	struct tm *T;
	char *RetVal;
	if ((RetVal = (char*)malloc(20)) == NULL) ILIBCRITICALEXIT(254);
	T = gmtime(&timeVal);
	if (T == NULL) ILIBCRITICALEXIT(253);
	sprintf_s(RetVal, 20, "%04d-%02d-%02dT%02d:%02d:%02d",
		T->tm_year + 1900,
		T->tm_mon + 1,
		T->tm_mday,
		T->tm_hour, T->tm_min, T->tm_sec);
#elif defined(_WIN32)
	struct tm T;
	char *RetVal;
	if ((RetVal = (char*)malloc(20)) == NULL) ILIBCRITICALEXIT(254);
	if (localtime_s(&T, &timeVal) != 0) ILIBCRITICALEXIT(253);
	sprintf_s(RetVal, 20, "%04d-%02d-%02dT%02d:%02d:%02d",
		T.tm_year + 1900,
		T.tm_mon + 1,
		T.tm_mday,
		T.tm_hour, T.tm_min, T.tm_sec);
#else
	struct tm *T;
	char *RetVal;
	if ((RetVal = (char*)malloc(20)) == NULL) ILIBCRITICALEXIT(254);
	T = localtime(&timeVal);
	if (T == NULL) ILIBCRITICALEXIT(253);
	sprintf_s(RetVal, 20, "%04d-%02d-%02dT%02d:%02d:%02d",
		T->tm_year + 1900,
		T->tm_mon + 1,
		T->tm_mday,
		T->tm_hour, T->tm_min, T->tm_sec);
#endif

	return(RetVal);
}
int ILibTime_ValidateTimePortion(char *timeString)
{
	//
	// Verify the format is hh:mm:ss+hh:mm
	//						hh:mm:ss-hh:mm
	//						hh:mm:ssZ
	//						hh:mm:ss
	//
	char *temp;
	struct parser_result *pr;
	int RetVal = 0;
	int length = (int)strnlen_s(timeString, 20);
	if (length==8 || length==9 || length==12 || length==13 || length==14 || length==18)
	{
		pr = ILibParseString(timeString,0,length,":",1);
		if (pr->NumResults==3 || pr->NumResults==4)
		{
			if (pr->FirstResult->datalength==2 && pr->FirstResult->NextResult->datalength==2) // Klockwork says this could be NULL, but that is not possible because NumResults is 3 or 4
			{
				temp = ILibString_Copy(pr->FirstResult->data,pr->FirstResult->datalength);
				if (atoi(temp)<24 && atoi(temp)>=0)
				{
					//
					// hh is correct
					//	
					free(temp);
					temp = ILibString_Copy(pr->FirstResult->NextResult->data,pr->FirstResult->NextResult->datalength);
					if (atoi(temp)>=0 && atoi(temp)<60)
					{
						//
						// mm is correct
						//
						free(temp);
						temp = ILibString_Copy(pr->FirstResult->NextResult->NextResult->data,length-6);
						switch((int)strnlen_s(temp, length-6))
						{
						case 2: // ss
							if (!(atoi(temp)>=0 && atoi(temp)<60))
							{
								RetVal=1;
							}
							break;
						case 3: // ssZ
							if (temp[2]=='Z')
							{
								temp[2]=0;
								if (!(atoi(temp)>=0 && atoi(temp)<60))
								{
									RetVal=1;
								}
							}
							else
							{
								RetVal=1;
							}
							break;
						case 6: // ss.sss
						case 7: // ss.sssZ
							if (temp[2]=='.')
							{
								temp[2]=0;
								if (!(atoi(temp)>=0 && atoi(temp)<60))
								{
									RetVal = 1;
								} 
								else if (temp[6]!= 'Z' && temp[6]!=0)
								{
									RetVal = 1;
								}
							}
							else
							{
								RetVal = 1;
							}
							break;
						case 8: // ss+hh:mm || ss-hh:mm 
							if (temp[2]=='-' || temp[2]=='+')
							{
								temp[2] = 0;
								if (!(atoi(temp)>=0 && atoi(temp)<60 && atoi(temp+3)>=0 && atoi(temp+3)<24))
								{
									RetVal=1;
								}
							}
							else
							{
								RetVal=1;
							}
							break;
						case 12: // ss.sss+hh:mm || ss.sss-hh:mm
							if (temp[2]=='.' && temp[9]==':' && (temp[6]=='+' || temp[6]=='-'))
							{
								temp[2]=0;
								if (!(atoi(temp)>=0 && atoi(temp)<60))
								{
									RetVal = 1;
								} 
							}
							else
							{
								RetVal = 1;
							}
							break;
						default:
							RetVal=1;
							break;
						}
						if (RetVal==0 && pr->NumResults==4)
						{
							//
							// Check the last mm component
							//
							if (!(atoi(pr->LastResult->data)>=0 && atoi(pr->LastResult->data)<60))
							{
								RetVal=1;
							}
						}
					}
					else
					{
						RetVal=1;
					}
				}
				else
				{
					RetVal=1;
				}
				free(temp);
			}
			else
			{
				RetVal=1;
			}
		}
		else
		{
			RetVal=1;
		}

		ILibDestructParserResults(pr);
	}
	else
	{
		RetVal=1;
	}
	return(RetVal);
}
char* ILibTime_ValidateDatePortion(char *timeString)
{
	struct parser_result *pr;
	char *startTime;
	int errCode = 1;
	int timeStringLen = (int)strnlen_s(timeString, 255);
	char *RetVal = NULL;
	int t;

	if (timeStringLen == 10)
	{
		pr = ILibParseString(timeString,0, timeStringLen,"-",1);
		if (pr->NumResults==3)
		{
			// This means there it is in x-y-z format
			if (pr->FirstResult->datalength==4)
			{
				// This means it is in yyyy-x-z format
				if (pr->FirstResult->NextResult->datalength==2 && pr->LastResult->datalength==2) // Klockwork says this could be NULL, but that's not possible, as numresults is 3
				{
					// This means it is in yyyy-xx-zz format
					startTime = ILibString_Copy(pr->FirstResult->NextResult->data,pr->FirstResult->NextResult->datalength);
					if (atoi(startTime)<=12 && atoi(startTime)>0)
					{
						// This means it is in yyyy-mm-xx format
						free(startTime);
						startTime = ILibString_Copy(pr->LastResult->data,pr->LastResult->datalength);
						if (atoi(startTime)<=31 && atoi(startTime)>0)
						{
							// Everything in correct format
							errCode = 0;
							t = timeStringLen + 10;
							if ((RetVal = (char*)malloc(t)) == NULL) ILIBCRITICALEXIT(254);
							sprintf_s(RetVal, t, "%sT00:00:00", timeString);
						}
					}
					free(startTime);
				}
			}
		}
		ILibDestructParserResults(pr);
	}
	if (errCode==0)
	{
		return(RetVal);
	}
	else
	{
		return(NULL);
	}
}
int ILibTime_ParseEx(char *timeString, time_t *val)
{
	int errCode = 0;
	time_t RetVal = 0;
	struct parser_result *pr,*pr2;
	struct tm t;
	char *startTime;
	int timeStringLen = (int)strnlen_s(timeString, 255);
	int i;

	char *year=NULL,*month=NULL,*day=NULL;
	char *hour=NULL,*minute=NULL,*second=NULL;

	char *tempString;

	if (ILibString_IndexOf(timeString, timeStringLen,"T",1)==-1)
	{
		//
		// Doesn't have time Component, so format must be: yyyy-mm-dd
		//
		startTime = ILibTime_ValidateDatePortion(timeString);
		if (startTime==NULL)
		{
			errCode = 1;
		}
	}
	else
	{
		//
		// Verify the format is yyyy-mm-ddThh:mm:ss+hh:mm
		//						yyyy-mm-ddThh:mm:ss-hh:mm
		//						yyyy-mm-ddThh:mm:ssZ
		//						yyyy-mm-ddThh:mm:ss
		//
		i = ILibString_IndexOf(timeString, timeStringLen,"T",1);
		tempString = ILibString_Copy(timeString,i);
		startTime = ILibTime_ValidateDatePortion(tempString);
		free(tempString);
		if (startTime!=NULL)
		{
			free(startTime);
			//
			// Valid Date Portion, now check the time portion
			//
			if (ILibTime_ValidateTimePortion(timeString+i+1)!=0)
			{
				errCode = 1;
			}
		}
		else
		{
			//
			// Invalid Date portion
			//
			errCode = 1;
		}
		if (errCode==0)
		{
			startTime = ILibString_Copy(timeString, timeStringLen);
		}
	}


	//
	// If we got this far and errCode==0, then we're golden
	//
	if (errCode!=0)
	{
		return(1);
	}

	memset(&t, 0, sizeof(struct tm));
	t.tm_isdst = -1;

	pr = ILibParseString(startTime, 0, (int)strnlen_s(startTime, timeStringLen), "T", 1);
	if (pr->NumResults == 2)
	{
		pr2 = ILibParseString(pr->FirstResult->data, 0, pr->FirstResult->datalength,"-", 1);

		if (pr2->NumResults == 3)
		{
			year = pr2->FirstResult->data;
			year[pr2->FirstResult->datalength]=0;

			month = pr2->FirstResult->NextResult->data; // Klockwork reports this could be NULL, but that's not possible becuase NumResults is 3
			month[pr2->FirstResult->NextResult->datalength]=0;

			day = pr2->LastResult->data;
			day[pr2->LastResult->datalength]=0;

			t.tm_year = atoi(year)-1900;
			t.tm_mon = atoi(month)-1;
			t.tm_mday = atoi(day);

			ILibDestructParserResults(pr2);
		}
	}
	pr2 = ILibParseString(pr->LastResult->data, 0, pr->LastResult->datalength, ":", 1);
	switch(pr2->NumResults)
	{
	case 4:
		//	yyyy-mm-ddThh:mm:ss+hh:mm
		//	yyyy-mm-ddThh:mm:ss-hh:mm
		hour = pr2->FirstResult->data;
		hour[pr2->FirstResult->datalength]=0;
		minute = pr2->FirstResult->NextResult->data; // Klockwork reports this could be NULL, but that's not possible because NumResults is 4
		minute[pr2->FirstResult->NextResult->datalength]=0;
		second = pr2->FirstResult->NextResult->NextResult->data;
		second[2]=0;
		break;
	case 3:
		//	yyyy-mm-ddThh:mm:ssZ
		hour = pr2->FirstResult->data;
		hour[pr2->FirstResult->datalength]=0;
		minute = pr2->FirstResult->NextResult->data; // Klocwork reports this could be NULL, but that's not possible becuase NumResults is 3
		minute[pr2->FirstResult->NextResult->datalength]=0;
		second = pr2->FirstResult->NextResult->NextResult->data;
		second[2]=0;
		break;
	}
	if (hour!=NULL && minute!=NULL && second!=NULL)
	{
		t.tm_hour = atoi(hour);
		t.tm_min = atoi(minute);
		t.tm_sec = atoi(second);

		RetVal = mktime(&t);
	}

	ILibDestructParserResults(pr2);
	ILibDestructParserResults(pr);
	free(startTime);
	*val = RetVal;
	return(errCode);
}
time_t ILibTime_Parse(char *timeString)
{
	time_t retval;
	ILibTime_ParseEx(timeString, &retval);
	return(retval);
}


//! Copy an IPv4 address into an IPv6 mapped address
/*!
	\param[in] addr4 IPv4 Address
	\param[in,out] addr6 IPv6 Address
*/
void ILibMakeIPv6Addr(struct sockaddr *addr4, struct sockaddr_in6 *addr6)
{
	if (addr4->sa_family == AF_INET6 || !g_ILibDetectIPv6Support)
	{
		// Direct copy
		memcpy_s(addr6, sizeof(struct sockaddr_in6), addr4, sizeof(struct sockaddr_in6));
	}
	else
	{
		// Conversion
		memset(addr6, 0, sizeof(struct sockaddr_in6));
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = ((struct sockaddr_in*)addr4)->sin_port;
		(((int*)&(addr6->sin6_addr)))[2] = 0xFFFF0000;
		(((int*)&(addr6->sin6_addr)))[3] = ((int*)addr4)[1];
	}
}

char IPv4MappedPrefix[12] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF};
char IPv4MappedLoopback[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x00,0x00,0x01};
char IPv4Loopback[4] = {0x7F,0x00,0x00,0x01};

//! Copy the IP Address into an HTTP Header style string
/*!
	\param[in] addr IP Address
	\param[out] str HTTP Header Style String
	\return Length of str
*/
int ILibMakeHttpHeaderAddr(struct sockaddr *addr, char** str)
{
	int len;
	if ((*str = (char *)malloc(256)) == NULL) ILIBCRITICALEXIT(254);

	if (addr->sa_family == AF_INET6)
	{
		// Check if this IPv6 addresses starts with the IPv4 mapped prefix
		if (memcmp(&((struct sockaddr_in6*)addr)->sin6_addr, IPv4MappedPrefix, 12) == 0)
		{
			// IPv4 translation conversion
			ILibInet_ntop(AF_INET, &(((int*)&((struct sockaddr_in6*)addr)->sin6_addr)[3]), *str, 256);
			len = (int)strnlen_s(*str, 256);
		}
		else
		{
			// IPv6 conversion
			*str[0] = '[';
			ILibInet_ntop(AF_INET6, &((struct sockaddr_in6*)addr)->sin6_addr, *str + 1, 254);
			len = (int)strnlen_s(*str, 256);
			(*str)[len++] = ']';
			(*str)[len++] = 0;
		}
	}
	else
	{
		// IPv4 conversion
		ILibInet_ntop(AF_INET, &(((struct sockaddr_in*)addr)->sin_addr), *str, 256);
		len = (int)strnlen_s(*str, 256);
	}
	return len;
}

//! Return true if the given address is an IPv4 mapped address in an IPv6 container
/*!
	\param addr IP Address
	\return [ZERO = FALSE, NON-ZERO = TRUE]
*/
int ILibIsIPv4MappedAddr(struct sockaddr *addr)
{
	return (addr->sa_family == AF_INET6 && memcmp(&((struct sockaddr_in6*)addr)->sin6_addr, IPv4MappedPrefix, 12) == 0);
}

//! Checks if the given address is a loopback address (IPv4 or IPv6)
/*!
	\param addr
	\return [0 = NOT-LOOPBACK, 1 = LOOPBACK]
*/
int ILibIsLoopback(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET)
	{
		if (memcmp(&((struct sockaddr_in*)addr)->sin_addr, IPv4Loopback, 4) == 0) return 1; else return 0;
	}
	else if (addr->sa_family == AF_INET6)
	{
		if (memcmp(&((struct sockaddr_in6*)addr)->sin6_addr, &in6addr_loopback, 16) == 0) return 1;
		if (memcmp(&((struct sockaddr_in6*)addr)->sin6_addr, IPv4MappedLoopback, 16) == 0) return 1;
	}
	return 0;
}

int ILibGetAddrBlob(struct sockaddr *addr, char** ptr)
{
	if (addr->sa_family == AF_INET)
	{
		// IPv4
		*ptr = (char*)&(((struct sockaddr_in*)addr)->sin_addr);
		return 4;
	}
	if (addr->sa_family == AF_INET6)
	{
		// IPv6, check if this is a IPv4 mapped address
		if (ILibIsIPv4MappedAddr(addr))
		{
			// This is a IPv4 mapped address, only take 4 last bytes
			*ptr = (char*)&(((struct sockaddr_in6*)addr)->sin6_addr) + 12;
			return 4;
		}
		else
		{
			// This is a real IPv6 address
			*ptr = (char*)&(((struct sockaddr_in6*)addr)->sin6_addr);

			// If the scope is not null, we need to store the address & scope (4 bytes more)
			return (((struct sockaddr_in6*)addr)->sin6_scope_id == 0)?16:20;
		}
	}
	return 0;
}

void ILibGetAddrFromBlob(char* ptr, int len, unsigned short port, struct sockaddr_in6 *addr)
{
	// Fetch the IP address
	memset(addr, 0, sizeof(struct sockaddr_in6));
	if (len == 4)
	{
		// IPv4 address
		addr->sin6_family = AF_INET;
		((struct sockaddr_in*)addr)->sin_port = port;
		memcpy_s(&(((struct sockaddr_in*)addr)->sin_addr), sizeof(struct in_addr), ptr, 4);
	}
	else if (len == 16 || len == 20)
	{
		// IPv6 address, or IPv6 + Scope
		addr->sin6_family = AF_INET6;
		addr->sin6_port = port;
		// memcpy(&(addr->sin6_addr), ptr, len); // Fast version (not klocworks friendly);
		memcpy_s(&(addr->sin6_addr), sizeof(struct in6_addr), ptr, 16);
		if (len == 20) memcpy_s(&(addr->sin6_scope_id), 4, ptr + 16, 4);
	}
	else ILIBCRITICALEXIT(253)
}

int g_ILibDetectIPv6Support = -1;

//! Determine if IPv6 is supported
/*!
	\return [ZERO = NOT SUPPORTED, NON-ZERO = SUPPORTED]
*/
int ILibDetectIPv6Support()
{
	SOCKET sock;
	struct sockaddr_in6 addr;

	if (g_ILibDetectIPv6Support >= 0) return g_ILibDetectIPv6Support;

	memset(&addr, 0, sizeof(struct sockaddr_in6));
	addr.sin6_family = AF_INET6;

	// Get our listening socket
	sock = ILibGetSocket((struct sockaddr*)&addr, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == 0)
	{
		g_ILibDetectIPv6Support = 0;
	}
	else
	{
		g_ILibDetectIPv6Support = 1;
#if defined(WIN32)
		closesocket(sock);
#else
		close(sock);
#endif
	}

	return g_ILibDetectIPv6Support; // Klocwork is being dumb. This can't be lost, because sock is getting closed if it's valid... So Klocwork is saying an invalid resource is getting lost. But we can't close an invalid handle
}

char* ILibInet_ntop2(struct sockaddr* addr, char *dst, size_t dstsize)
{
	if (addr != NULL && addr->sa_family == AF_INET) { ILibInet_ntop(AF_INET, &(((int*)&((struct sockaddr_in*)addr)->sin_addr)[0]), dst, dstsize); return dst; }
	if (addr != NULL && addr->sa_family == AF_INET6) { ILibInet_ntop(AF_INET6, &((struct sockaddr_in6*)addr)->sin6_addr, dst, dstsize); return dst; }
	dst[0] = 0;
	return NULL;
}

#if defined(WIN32)

typedef PCTSTR (WSAAPI *inet_ntop_type)(__in INT Family, __in PVOID pAddr, __out PTSTR pStringBuf, __in size_t StringBufSize);
typedef INT (WSAAPI *inet_pton_type)(__in INT Family, __in PCTSTR pszAddrString, __out PVOID pAddrBuf);
inet_ntop_type inet_ntop_ptr = NULL;
inet_pton_type inet_pton_ptr = NULL;
int inet_pton_setup_done = 0;

void inet_pton_setup()
{
	HMODULE h = NULL;
	if (inet_pton_setup_done == 1) return;
	inet_pton_setup_done = 1;
	h = GetModuleHandle(TEXT("Ws2_32.dll"));
	if (h == NULL) return;
	inet_ntop_ptr = (inet_ntop_type)GetProcAddress(h, "inet_ntop");
	inet_pton_ptr = (inet_pton_type)GetProcAddress(h, "inet_pton");
	if (inet_ntop_ptr == NULL || inet_pton_ptr == NULL) inet_pton_setup_done = 2;
}

char* ILibInet_ntop(int af, const char *src, char *dst, size_t dstsize)
{
	if (inet_pton_setup_done == 0) inet_pton_setup();

	if (inet_pton_setup_done == 1)
	{
		// Use the real IPv6 version
		return (char*)((inet_ntop_ptr)(af, (char*)src, (PTSTR)dst, dstsize));
	}
	else
	{
		// Use the alternate IPv4 only version
		if (af != AF_INET) return NULL;
		_snprintf_s(dst, dstsize, dstsize, "%u.%u.%u.%u", (unsigned char)src[0], (unsigned char)src[1], (unsigned char)src[2], (unsigned char)src[3]);
		return dst;
	}
}

int ILibInet_pton(int af, const char *src, char *dst)
{
	if (inet_pton_setup_done == 0) inet_pton_setup();

	if (inet_pton_setup_done == 1)
	{
		// Use the real IPv6 version
		return (int)(inet_pton_ptr)(af, (PCTSTR)src, dst);
	}
	else
	{
		// Use the alternate IPv4 only version
		char dst2[8];
		char dst3[4];
		int i;

		if (af != AF_INET) return 0;
		sscanf_s(src, "%hu.%hu.%hu.%hu", (unsigned short*)(dst2), (unsigned short*)(dst2 + 2), (unsigned short*)(dst2 + 4), (unsigned short*)(dst2 + 6));
		
		for (i = 0; i < 4; ++i) { ((unsigned char*)dst3)[i] = (unsigned char)((unsigned short*)dst2)[i]; }

		((int*)dst)[0] = ((int*)dst3)[0];
		return 1;
	}
}

#else

char* ILibInet_ntop(int af, const void *src, char *dst, size_t dstsize)
{
	return (char*)inet_ntop(af, src, dst, (socklen_t)dstsize);
}

int ILibInet_pton(int af, const char *src, void *dst)
{
	return inet_pton(af, src, dst);
}

#endif

// Compare two IP addresses. Compare: 0 already true, 1 family only, 2 family and address, 3 familly, address and port. Return 0 if different, 1 if same.
int ILibInetCompare(struct sockaddr* addr1, struct sockaddr* addr2, int compare)
{
	if (addr1 == NULL || addr2 == NULL) return 0;
	if (addr1 == addr2 || compare == 0) return 1;
	if (addr1->sa_family != addr2->sa_family) return 0;
	if (compare == 1) return 1;
	if (addr1->sa_family != AF_INET && addr1->sa_family != AF_INET6) return 0;
	if (addr1->sa_family == AF_INET)
	{ 
#ifdef WIN32
		if (((struct sockaddr_in*)addr1)->sin_addr.S_un.S_addr != ((struct sockaddr_in*)addr2)->sin_addr.S_un.S_addr) return 0;
#else
		if (((struct sockaddr_in*)addr1)->sin_addr.s_addr != ((struct sockaddr_in*)addr2)->sin_addr.s_addr) return 0;
#endif
	}
	else
	{
		if (memcmp(&((struct sockaddr_in6*)addr1)->sin6_addr, &((struct sockaddr_in6*)addr2)->sin6_addr, 16) != 0) return 0;
	}
	if (compare == 2) return 1;
	if (addr1->sa_family == AF_INET)
	{
		if (((struct sockaddr_in*)addr1)->sin_port != ((struct sockaddr_in*)addr2)->sin_port) return 0;
	}
	else
	{
		if (((struct sockaddr_in6*)addr1)->sin6_port != ((struct sockaddr_in6*)addr2)->sin6_port) return 0;
	}
	return 1;
}

int ILibResolve(char* hostname, char* service, struct sockaddr_in6* addr6)
{
	int r;
    struct addrinfo *result = NULL;
    struct addrinfo hints;
	
	memset(addr6, 0, sizeof(struct sockaddr_in6));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

	r = getaddrinfo(hostname, service, &hints, &result);
	if (r != 0) { if (result != NULL) { freeaddrinfo(result); } return r; }
	if (result == NULL) return -1;
	if (result->ai_family == AF_INET) { memcpy_s(addr6, sizeof(struct sockaddr_in6), result->ai_addr, sizeof(struct sockaddr_in)); }
	if (result->ai_family == AF_INET6) { memcpy_s(addr6, sizeof(struct sockaddr_in6), result->ai_addr, sizeof(struct sockaddr_in6)); }
	freeaddrinfo(result);
	return 0;
}
int ILibResolveEx(char* hostname, unsigned short port, struct sockaddr_in6* addr6)
{
	char service[16];
	if (sprintf_s(service, sizeof(service), "%u", port) > 0)
	{
		return(ILibResolve(hostname, service, addr6));
	}
	else
	{
		return(1);
	}
}
unsigned char ILib6to4Header[12]= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
void ILib6to4(struct sockaddr* addr)
{
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addr;
	struct sockaddr_in6* addr6 = (struct sockaddr_in6*)addr;
	
#ifdef WIN32
	if (addr->sa_family != AF_INET6 || memcmp(addr6->sin6_addr.u.Byte, ILib6to4Header, 12)) return;
	addr4->sin_addr.s_addr = ntohl(((int*)(addr6->sin6_addr.u.Byte + 12))[0]);
	addr4->sin_family = AF_INET;
#else
	if (addr->sa_family != AF_INET6 || memcmp(&(addr6->sin6_addr), ILib6to4Header, 12)) return;
	addr4->sin_addr.s_addr = ntohl(((int*)(&(addr6->sin6_addr) + 12))[0]);
	addr4->sin_family = AF_INET;
#endif
}

// Log a critical error to file
char* ILibCriticalLog (const char* msg, const char* file, int line, int user1, int user2)
{
	char timeStamp[32];
	int len = ILibGetLocalTime((char*)timeStamp, (int)sizeof(timeStamp));
	if (file != NULL)
	{
		len = sprintf_s(ILibScratchPad, sizeof(ILibScratchPad), "\r\n[%s] %s:%d (%d,%d) %s", timeStamp, file, line, user1, user2, msg);
	}
	else
	{
		len = sprintf_s(ILibScratchPad, sizeof(ILibScratchPad), "\r\n[%s] %s", timeStamp, msg);
	}
	if (len > 0 && len < (int)sizeof(ILibScratchPad) && ILibCriticalLogFilename != NULL) ILibAppendStringToDiskEx(ILibCriticalLogFilename, ILibScratchPad, len);
	if (file != NULL)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s:%d (%d,%d) %s", file, line, user1, user2, msg);
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(gILibChain), ILibRemoteLogging_Modules_Microstack_Generic, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s", msg);
	}
	return(ILibScratchPad);
}
//! Platform Agnostic method to Spawn a detached worker thread with normal priority/affinity
/*!
	\param method Handler to dispatch on the new thread
	\param arg Optional Parameter to dispatch [Can be NULL]
	\return Thread Handle
*/
void* ILibSpawnNormalThread(voidfp1 method, void* arg)
{
#if defined (_POSIX) || defined (__APPLE__)
	intptr_t result;
	void* (*fptr) (void* a);
	pthread_t newThread;
	fptr = (void*(*)(void*))method;
	result = (intptr_t)pthread_create(&newThread, NULL, fptr, arg);
	pthread_detach(newThread);
	return (void*)result;
#endif

#ifdef WIN32
	return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)method, arg, 0, NULL );
#endif

#ifdef UNDER_CE
	return CreateThread(NULL, 0, method, arg, 0, NULL );
#endif
}
//! Platform Agnostic to end the currently executing thread
void ILibEndThisThread()
{
#if defined (_POSIX) || defined (__APPLE__)
	pthread_exit(NULL);
#endif

#ifdef WIN32
	ExitThread(0);
#endif

#ifdef UNDER_CE
	ExitThread(0);
#endif
}
#ifdef WIN32
void ILibHandle_DisableInherit(HANDLE * h)
{
	HANDLE tmpRead = *h;
	if (tmpRead != NULL)
	{
		DuplicateHandle(GetCurrentProcess(), tmpRead, GetCurrentProcess(), h, 0, FALSE, DUPLICATE_SAME_ACCESS);
		CloseHandle(tmpRead);
	}
}
#endif
// Convert a block of data to HEX
// The "out" must have (len*2)+1 free space.
char ILibHexTable[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
//! Convert a block of data to HEX
/*!
	\param data Block of data to convert
	\param len Data Length
	\param[in,out] out Hex Result. Length must be at least (len*2) + 1
*/
char* ILibToHex(char* data, int len, char* out)
{
	int i;
	char *p = out;
	if (data == NULL || len == 0) { *p = 0; return NULL; }
	for(i = 0; i < len; i++)
	{
		*(p++) = ILibHexTable[((unsigned char)data[i]) >> 4];
		*(p++) = ILibHexTable[((unsigned char)data[i]) & 0x0F];
	}
	*p = 0;
	return out;
}
//! Determine if the current thread is the Microstack thread
/*!
	\param chain Microstack Thread to compare
	\return 0 = NO, 1 = YES
*/
int ILibIsRunningOnChainThread(void* chain)
{
	struct ILibBaseChain* c = (struct ILibBaseChain*)chain;

#if defined(WIN32)
	return(c->ChainThreadID == 0 || c->ChainThreadID == GetCurrentThreadId());
#else
	return(pthread_equal(c->ChainThreadID, pthread_self()));
#endif
}

#define ILibLinkedList_FileBacked_Root_Raw_HEAD_OFFSET(root) ((unsigned int)((char*)&(root->head) - (char*)root))
#define ILibLinkedList_FileBacked_Root_Raw_TAIL_OFFSET(root) ((unsigned int)((char*)&(root->tail) - (char*)root))

void ILibLinkedList_FileBacked_Close(ILibLinkedList_FileBacked_Root *root)
{
	FILE *source = ((FILE**)ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root)))[0];
	if (source != NULL) { fclose(source); }
	free(root);
}

int ILibLinkedList_FileBacked_IsEmpty(ILibLinkedList_FileBacked_Root *root)
{
	return(root->head == 0 ? 1 : 0);
}

void ILibLinkedList_FileBacked_Reset(ILibLinkedList_FileBacked_Root *root)
{
	void **ptr = ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root));
	FILE* source = (FILE*)ptr[0];
	fclose(source);
	source = NULL;
#ifdef WIN32
	fopen_s(&source, (char*)ptr[1], "wb+N");
#else
	source = fopen((char*)ptr[1], "wb+");
#endif
	ptr[0] = source;
	root->head = root->tail = 0;
	ILibLinkedList_FileBacked_SaveRoot(root);
}
int ILibLinkedList_FileBacked_ReloadRoot(ILibLinkedList_FileBacked_Root* root)
{
	void **ptr = ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root));
	FILE* source = (FILE*)ptr[0];

	fseek(source, 0, SEEK_SET);
	return(fread((void*)root, sizeof(char), sizeof(ILibLinkedList_FileBacked_Root), source) > 0 ? 0 : 1);
}
void ILibLinkedList_FileBacked_SaveRoot(ILibLinkedList_FileBacked_Root* root)
{
	void **ptr = ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root));
	FILE* source = (FILE*)ptr[0];

	fseek(source, 0, SEEK_SET);
	fwrite(root, sizeof(char), sizeof(ILibLinkedList_FileBacked_Root), source);
	fflush(source);
}
ILibLinkedList_FileBacked_Node* ILibLinkedList_FileBacked_ReadNext(ILibLinkedList_FileBacked_Root* root, ILibLinkedList_FileBacked_Node* current)
{
	void **ptr = ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root));
	FILE* source = (FILE*)ptr[0];
	char* buffer = (char*)&ptr[2];
	ILibLinkedList_FileBacked_Node *retVal = NULL;
	size_t br;

	if (current == NULL)
	{
		if (root->head > 0)
		{
			retVal = (ILibLinkedList_FileBacked_Node*)(buffer);
			fseek(source, root->head, SEEK_SET);
			br = fread(buffer, sizeof(char), 2 * sizeof(unsigned int), source);
			if (fread(buffer + br, sizeof(char), retVal->dataLen, source) != retVal->dataLen) { retVal = NULL; }
		}
	}
	else
	{
		if (current->next > 0)
		{
			retVal = (ILibLinkedList_FileBacked_Node*)(buffer);
			fseek(source, current->next, SEEK_SET);
			br = fread(buffer, sizeof(char), 2 * sizeof(unsigned int), source);
			if (fread(buffer + br, sizeof(char), retVal->dataLen, source) != retVal->dataLen) { retVal = NULL; }
		}
	}

	return(retVal);
}

void ILibLinkedList_FileBacked_CopyPath(ILibLinkedList_FileBacked_Root *root, char* path)
{
	char *extraMemory = (char*)ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root));
	int pathLen = (int)strnlen_s(path, _MAX_PATH);
	int offset = ILibMemory_GetExtraMemorySize(extraMemory) - pathLen - 1;

	memcpy_s(extraMemory + offset, pathLen, path, pathLen);
	(extraMemory + offset)[pathLen] = 0;

	((void**)extraMemory)[1] = &extraMemory[offset];
}
ILibLinkedList_FileBacked_Root* ILibLinkedList_FileBacked_Create(char* path, unsigned int maxFileSize, int maxRecordSize)
{
	FILE* source;
	ILibLinkedList_FileBacked_Root *retVal = NULL;

#ifdef WIN32
	if (fopen_s(&source, path, "rb+N") != 0)
	{
		fopen_s(&source, path, "wb+N");
	}
#else
	if ((source = fopen(path, "rb+")) == NULL)
	{
		source = fopen(path, "wb+");
	}
#endif
	
	if (source == NULL) { return(NULL); }

	fseek(source, 0, SEEK_END);
	if ((int)ftell(source) < (int)sizeof(ILibLinkedList_FileBacked_Root))
	{
		// New File
		retVal = (ILibLinkedList_FileBacked_Root*)ILibMemory_Allocate(sizeof(ILibLinkedList_FileBacked_Root), maxRecordSize + (int)strnlen_s(path, _MAX_PATH) + 1 + (int)(2*sizeof(void*)), NULL, NULL);
		retVal->maxSize = maxFileSize;
		((FILE**)ILibMemory_GetExtraMemory(retVal, sizeof(ILibLinkedList_FileBacked_Root)))[0] = source;
		ILibLinkedList_FileBacked_CopyPath(retVal, path);
		fwrite((void*)retVal, sizeof(char), sizeof(ILibLinkedList_FileBacked_Root), source);
		fflush(source);
	}
	else
	{
		// File Exists
		fseek(source, 0, SEEK_SET);
		retVal = (ILibLinkedList_FileBacked_Root*)ILibMemory_Allocate(sizeof(ILibLinkedList_FileBacked_Root), maxRecordSize + (int)strnlen_s(path, _MAX_PATH) + 1 + (int)(2 * sizeof(void*)), NULL, NULL);
		if (fread((void*)retVal, sizeof(char), sizeof(ILibLinkedList_FileBacked_Root), source) != sizeof(ILibLinkedList_FileBacked_Root))
		{
			free(retVal);
			retVal = NULL;
			fclose(source);
		}
		else
		{
			((FILE**)ILibMemory_GetExtraMemory(retVal, sizeof(ILibLinkedList_FileBacked_Root)))[0] = source;
			ILibLinkedList_FileBacked_CopyPath(retVal, path);
		}
	}

	return(retVal);
}

int ILibLinkedList_FileBacked_AddTail(ILibLinkedList_FileBacked_Root* root, char* data, unsigned int dataLen)
{
	unsigned int sizeNeeded = dataLen + 8;
	unsigned int i;
	unsigned int tmp;
	int headUpdated = 0;

	FILE* source = ((FILE**)ILibMemory_GetExtraMemory(root, sizeof(ILibLinkedList_FileBacked_Root)))[0];

	fseek(source, 0, SEEK_SET);
	if (fread((void*)root, sizeof(char), sizeof(ILibLinkedList_FileBacked_Root), source) != sizeof(ILibLinkedList_FileBacked_Root)) { return(1); }

	// Grab a memory pointer that we can write to
	if (root->tail == 0)
	{
		i = (unsigned int)sizeof(ILibLinkedList_FileBacked_Root);
	}
	else
	{
		// Need to calculate some things
		fseek(source, root->tail + (unsigned int)sizeof(unsigned int), SEEK_SET);
		if (fread(&i, sizeof(char), sizeof(unsigned int), source) != sizeof(unsigned int)) { return(1); }
		i += ((unsigned int)2*sizeof(unsigned int) + root->tail);

		if (i + sizeNeeded > root->maxSize) { i = root->head; }
	}
	
	// Check to see if we write to this memory location, do we need to move the head pointer
	while (root->head != 0 && i <= root->head && (i + sizeNeeded > root->head))
	{
		fseek(source, root->head, SEEK_SET);
		if (fread(&(root->head), sizeof(char), sizeof(unsigned int), source) != sizeof(unsigned int)) { return(1); }
		headUpdated = 1;
	}

	if (root->head == 0)
	{
		root->head = i;
		headUpdated = 1;
	}

	// Write the new Head Pointer
	if (headUpdated != 0)
	{
		fseek(source, ILibLinkedList_FileBacked_Root_Raw_HEAD_OFFSET(root), SEEK_SET);
		fwrite(&(root->head), sizeof(char), sizeof(unsigned int), source);
		fflush(source);
	}

	// Write Record
	fseek(source, i, SEEK_SET);
	tmp = 0;
	fwrite(&tmp, sizeof(char), sizeof(unsigned int), source);		// Next
	fwrite(&dataLen, sizeof(char), sizeof(unsigned int), source);	// Data Length
	if (dataLen > 0)
	{
		fwrite(data, sizeof(char), dataLen, source);				// Data
	}
	fflush(source);

	// Update 'Next' of old Tail
	if (root->tail != 0)
	{
		fseek(source, root->tail, SEEK_SET);
		fwrite(&i, sizeof(char), sizeof(unsigned int), source);			// New NEXT value
		fflush(source);
	}

	// Write new Tail Pointer
	fseek(source, ILibLinkedList_FileBacked_Root_Raw_TAIL_OFFSET(root), SEEK_SET);
	fwrite(&i, sizeof(char), sizeof(unsigned int), source);			// NEXT
	fflush(source);
	return(0);
}