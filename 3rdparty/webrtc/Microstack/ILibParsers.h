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


/*! \file ILibParsers.h 
\brief MicroStack APIs for various functions and tasks
*/

#ifndef __ILibParsers__
#define __ILibParsers__

#ifdef __cplusplus
extern "C" {
#endif

	/*! \defgroup ILibParsers ILibParser Modules
	* @{
	* @}
	*/

	/*! \def MAX_HEADER_LENGTH
	Specifies the maximum allowed length for an HTTP Header
	*/
#define MAX_HEADER_LENGTH 800

#ifdef MEMORY_CHECK
#include <assert.h>
#define MEMCHECK(x) x
#else
#define MEMCHECK(x)
#endif

	//extern struct sockaddr_in6;

#if !defined(WIN32) && !defined(_WIN32_WCE)
#define HANDLE int
#define SOCKET int
#endif

extern char* ILibCriticalLogFilename;

#ifdef _WIN32_WCE
#define REQUIRES_MEMORY_ALIGNMENT
#define errno 0
#ifndef ERANGE
#define ERANGE 1
#endif
#define time(x) GetTickCount()
#endif

#ifndef WIN32
#define REQUIRES_MEMORY_ALIGNMENT
#endif

#if defined(WIN32) || defined (_WIN32_WCE)
#ifndef MICROSTACK_NO_STDAFX
#include "stdafx.h"
#endif
struct sockaddr_in6;

#elif defined(_POSIX) || defined (__APPLE__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/utsname.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#if !defined(__APPLE__) && !defined(_VX_CPU)
#include <malloc.h>
#endif
#include <fcntl.h>
#include <signal.h>
#define UNREFERENCED_PARAMETER(P)
#endif

#ifdef __linux__
#include <linux/limits.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>


#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#include <winioctl.h>
#include <winbase.h>
#endif

#ifdef __APPLE__
#include <pthread.h>
#define sem_t pthread_mutex_t
#define sem_init(x,pShared,InitValue) pthread_mutex_init(x, NULL)
#define sem_destroy(x) pthread_mutex_destroy(x)
#define sem_wait(x) pthread_mutex_lock(x)
#define sem_trywait(x) pthread_mutex_trylock(x)
#define sem_post(x) pthread_mutex_unlock(x)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#include <time.h>
#include <sys/timeb.h>
#endif

#ifndef _MAX_PATH
#define _MAX_PATH PATH_MAX
#endif

#include<stdint.h>
static inline void ignore_result(uint64_t result) { (void)result; }

#if defined(_DEBUG)
#define PRINTERROR() printf("ERROR in %s, line %d\r\n", __FILE__, __LINE__);
#else
#define PRINTERROR()
#endif

int ILibGetLocalTime(char *dest, int destLen);
long ILibGetTimeStamp();

#ifdef ILibEXPORT
#ifdef WIN32
#define ILibExportMethod __declspec(dllexport)
#else
#define ILibExportMethod
#endif
#else
#define ILibExportMethod
#endif


#if defined(WIN32) || defined(_WIN32_WCE)
#ifndef FD_SETSIZE
#define SEM_MAX_COUNT 64
#else
#define SEM_MAX_COUNT FD_SETSIZE
#endif
	void ILibGetTimeOfDay(struct timeval *tp);

/*
// Implemented in Windows using events.
#define sem_t HANDLE
#define sem_init(x,pShared,InitValue) *x=CreateSemaphore(NULL,InitValue,SEM_MAX_COUNT,NULL)
#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
#define sem_post(x) ReleaseSemaphore(*x,1,NULL)
*/

// Implemented in Windows using critical section.
#define sem_t CRITICAL_SECTION
#define sem_init(x,pShared,InitValue) InitializeCriticalSection(x);
#define sem_destroy(x) (DeleteCriticalSection(x))
#define sem_wait(x) EnterCriticalSection(x)
#define sem_trywait(x) TryEnterCriticalSection(x)
#define sem_post(x) LeaveCriticalSection(x)

#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#define strcasecmp(x,y) _stricmp(x,y)
#define gettimeofday(tp,tzp) ILibGetTimeOfDay(tp)

#if !defined(_WIN32_WCE)
#define tzset() _tzset()
#define daylight _daylight
#define timezone _timezone
#endif

#ifndef stricmp
#define stricmp(x,y) _stricmp(x,y)
#endif

#ifndef strnicmp
#define strnicmp(x,y,z) _strnicmp(x,y,z)
#endif

#ifndef strcmpi
#define strcmpi(x,y) _stricmp(x,y)
#endif
#endif


#ifndef WIN32
	
// Polyfills
#ifndef memcpy_s
	int ILibMemory_Copy_s(void *destination, size_t destinationSize, void *source, size_t sourceLength);
	#define memcpy_s(dest, destSize, source, sourceLen) ILibMemory_Copy_s(dest, destSize, source, sourceLen)
#endif

#ifndef memmove_s
	int ILibMemory_Move_s(void *destination, size_t destinationSize, void *source, size_t sourceLength);
	#define memmove_s(dest, destSize, source, sourceLen) ILibMemory_Move_s(dest, destSize, source, sourceLen)
#endif

#ifndef strncpy_s 
#define strncpy_s(dest, destSize, source, count) ILibString_n_Copy_s(dest, destSize, source, count)
#endif

#ifndef strcpy_s
#define strcpy_s(dest, destSize, source) ILibString_Copy_s(dest, destSize, source)
#endif

#ifndef strnlen_s
#define strnlen_s(source, maxCount) (strlen(source) < (maxCount) ? strlen(source) : (maxCount))
#endif

#ifndef sprintf_s
#define ILib_need_sprintf_s
#include <stdarg.h>
	int sprintf_s(void *dest, size_t destSize, char *format, ...);
#endif

#ifndef strnlen_s
#define strnlen_s(source, maxLen) strnlen(source, maxLen)
#endif

#endif



#if !defined(WIN32) 
#define __fastcall
#endif

	typedef void (*voidfp)(void);		// Generic function pointer
	typedef void(*voidfp1)(void*);		// Generic function pointer
	extern char ILibScratchPad[4096];   // General buffer
	extern char ILibScratchPad2[65536]; // Often used for UDP packet processing
	extern void* gILibChain;			// Global Chain used for Remote Logging when a chain instance is not otherwise exposed

	/*! \def UPnPMIN(a,b)
	Returns the minimum of \a a and \a b.
	*/
#define UPnPMIN(a,b) (((a)<(b))?(a):(b))

/*!
\ingroup ChainGroup
*@{
*/
	extern const int ILibMemory_CHAIN_CONTAINERSIZE;

//! Determines if the specified chain is in the process of being disposed.
/*!
	\param Chain Microstack Chain to query
	\return 0 = NO, 1 = YES
*/
#define ILibIsChainBeingDestroyed(Chain) (*((int*)Chain))
//!	Determines if the specified chain is running
/*!
	\param Chain Microstack Chain to query
	\return 0 = NO, 1 = YES
*/
#define ILibIsChainRunning(Chain) (((int*)Chain)[1])
int ILibIsRunningOnChainThread(void* chain);
/*! @} */

	typedef enum ILibServerScope
	{
		ILibServerScope_All=0,
		ILibServerScope_LocalLoopback=1,
		ILibServerScope_LocalSegment=2
	}ILibServerScope;


	typedef	void(*ILibChain_PreSelect)(void* object, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	typedef	void(*ILibChain_PostSelect)(void* object, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	typedef	void(*ILibChain_Destroy)(void* object);
	typedef void(*ILibChain_DestroyEvent)(void *chain, void *user);
	typedef void(*ILibChain_StartEvent)(void *chain, void *user);
	typedef void*(*ILibChain_Link_GetUserMemory)(void *ChainLinkObject, int *len);
	typedef void* ILibChain_EventHookToken;
	typedef void(*ILibChain_EventHookHandler)(void *hookedObject, ILibChain_EventHookToken token);

	typedef struct ILibChain_Link
	{
		ILibChain_PreSelect PreSelectHandler;
		ILibChain_PostSelect PostSelectHandler;
		ILibChain_Destroy DestroyHandler;
		void* ParentChain;
		void* ExtraMemoryPtr;
	}ILibChain_Link;
	ILibChain_Link* ILibChain_Link_Allocate(int structSize, int extraMemorySize);
	int ILibChain_Link_GetExtraMemorySize(ILibChain_Link* link);

	void* ILibMemory_Allocate(int containerSize, int extraMemorySize, void** allocatedContainer, void **extraMemory);
	int ILibMemory_GetExtraMemorySize(void* extraMemory);
	ILibExportMethod void* ILibMemory_GetExtraMemory(void *container, int containerSize);
	ILibChain_EventHookToken ILibChain_SetEventHook(void* chainLinkObject, int maxTimeout, ILibChain_EventHookHandler handler);
	void ILibChain_UpdateEventHook(ILibChain_EventHookToken token, int maxTimeout);

	/*!
		\defgroup LoggingMethods Logging Methods
		\ingroup ILibParsers
		@{
	*/
#ifdef _REMOTELOGGING
	//! Associate an ILibRemoteLogging module with a microstack Chain
	/*!
		\param chain Microstack Chain to associate
		\param logger ILibRemoteLogging module to associate
	*/
	#define ILibChainSetLogger(chain, logger) ((void**)&((int*)chain)[2])[0] = logger
	//! Gets the associated ILibRemoteLogging module from a microstack chain
	/*!
		\paran chain Microstack chain to get the association from
		\return ILibRemoteLogging module associated (NULL if none associated)
	*/
	#define ILibChainGetLogger(chain) (chain!=NULL?(((void**)&((int*)chain)[2])[0]):NULL)
	#ifdef _REMOTELOGGINGSERVER
		ILibExportMethod unsigned short ILibStartDefaultLoggerEx(void* chain, unsigned short portNumber, char *path);
		#define ILibStartDefaultLogger(chain, portNumber) ILibStartDefaultLoggerEx(chain, portNumber, NULL)
	#else
		#define ILibStartDefaultLogger(chain, portNumber) 0
		#define ILibStartDefaultLoggerEx(chain, portNumber, path) 0
	#endif
#else
	#define ILibChainSetLogger(chain, logger) ;
	#define ILibChainGetLogger(chain) NULL
	#define ILibStartDefaultLogger(chain, portNumber) 0
	#define ILibStartDefaultLoggerEx(chain, portNumber, path) 0
#endif
	/*! @} */

	/*!
		\ingroup ChainGroup
	*@{
	*/
	void ILibChain_OnDestroyEvent_AddHandler(void *chain, ILibChain_DestroyEvent sink, void *user);
	void ILibChain_OnStartEvent_AddHandler(void *chain, ILibChain_StartEvent sink, void *user);
	/*! @} */

	int ILibIsLittleEndian();
	int ILibGetCurrentTimezoneOffset_Minutes();
	int ILibIsDaylightSavingsTime();
	int ILibTime_ParseEx(char *timeString, time_t *val);
	time_t ILibTime_Parse(char *timeString);
	char* ILibTime_Serialize(time_t timeVal);
	long long ILibGetUptime();

	unsigned long long ILibHTONLL(unsigned long long v);
	unsigned long long ILibNTOHLL(unsigned long long v);

	typedef void* ILibReaderWriterLock;
	ILibReaderWriterLock ILibReaderWriterLock_Create();
	ILibReaderWriterLock ILibReaderWriterLock_CreateEx(void *chain);
	int ILibReaderWriterLock_ReadLock(ILibReaderWriterLock rwLock);
	int ILibReaderWriterLock_ReadUnLock(ILibReaderWriterLock rwLock);
	int ILibReaderWriterLock_WriteLock(ILibReaderWriterLock rwLock);
	int ILibReaderWriterLock_WriteUnLock(ILibReaderWriterLock rwLock);
	void ILibReaderWriterLock_Destroy(ILibReaderWriterLock rwLock);

	/*! \defgroup ILibTransport ILibTransport Abstraction
		\ingroup ILibParsers
	* @{
	*/
	//! Fetch the underlying chain from an ILibTransport object
	/*!
		\param transport ILibTransport object to query
		\return Microstack Chain
	*/
	#define ILibTransportChain(transport) ((ILibTransport*)transport)->ChainLink.ParentChain
	//! Memory Ownership of ILibTransport Data
	typedef enum ILibTransport_MemoryOwnership
	{
		ILibTransport_MemoryOwnership_CHAIN = 0, /*!< The Microstack will own this memory, and free it when it is done with it */
		ILibTransport_MemoryOwnership_STATIC = 1, /*!< This memory is static, so the Microstack will not free it, and assume it will not go away, so it won't copy it either */
		ILibTransport_MemoryOwnership_USER = 2 /*!< The Microstack doesn't own this memory, so if necessary the memory will be copied */
	}ILibTransport_MemoryOwnership;
	//! Operation Status
	typedef enum ILibTransport_DoneState
	{
		ILibTransport_DoneState_INCOMPLETE = 0,	/*!< Operation not complete [PENDING] */
		ILibTransport_DoneState_COMPLETE = 1,	/*!< Operation complete */
		ILibTransport_DoneState_ERROR	= -4	/*!< Operation failed */
	}ILibTransport_DoneState;

	typedef ILibTransport_DoneState(*ILibTransport_SendPtr)(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done);
	typedef void(*ILibTransport_ClosePtr)(void *transport);
	typedef unsigned int(*ILibTransport_PendingBytesToSendPtr)(void *transport);

	ILibTransport_DoneState ILibTransport_Send(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done);
	void ILibTransport_Close(void *transport);
	unsigned int ILibTransport_PendingBytesToSend(void *transport);

	//! ILibTransport Abstraction [DO NOT MODIFY STRUCT DEFINITION]
	typedef	void(*ILibTransport_OnSendOK)(void* sender);
	typedef struct ILibTransport
	{
		ILibChain_Link ChainLink;								
		ILibTransport_SendPtr SendPtr;							/*!< [RESERVED: Encapsulated Send Handler] */
		ILibTransport_ClosePtr ClosePtr;						/*!< [RESERVED: Encapsulated Close Handler] */
		ILibTransport_PendingBytesToSendPtr PendingBytesPtr;	/*!< [RESERVED: Encapsulated Pending Handler] */
		ILibTransport_OnSendOK SendOkPtr;						/*!< [RESERVED: Encapsulated SendOK Callback] */
		unsigned int IdentifierFlags;						/*!< ILibTransport Type Identifier */
	}ILibTransport;
	/*! @} */


	/*! \defgroup DataStructures Data Structures
	\ingroup ILibParsers
	\{
	\}
	*/


	/*! \struct parser_result_field ILibParsers.h
	\brief Data Elements of \a parser_result
	\par
	This structure represents individual tokens, resulting from a call to
	\a ILibParseString and \a ILibParseStringAdv
	*/
	typedef struct parser_result_field
	{
		/*! \var data
		\brief Pointer to string
		*/
		char* data;

		/*! \var datalength
		\brief Length of \a data
		*/
		int datalength;

		/*! \var NextResult
		\brief Pointer to next token
		*/
		struct parser_result_field *NextResult;
	}parser_result_field;

	/*! \struct parser_result ILibParsers.h
	\brief String Parsing Result Index
	\par
	This is returned from a successfull call to either \a ILibParseString or
	\a ILibParseStringAdv.
	*/
	typedef struct parser_result
	{
		/*! \var FirstResult
		\brief Pointer to the first token
		*/
		struct parser_result_field *FirstResult;
		/*! \var LastResult
		\brief Pointer to the last token
		*/
		struct parser_result_field *LastResult;
		/*! \var NumResults
		\brief The total number of tokens
		*/
		int NumResults;
	}parser_result;

	/*! \struct packetheader_field_node ILibParsers.h
	\brief HTTP Headers
	\par
	This structure represents an individual header element. A list of these
	is referenced from a \a packetheader_field_node.
	*/
	typedef struct packetheader_field_node
	{
		/*! \var Field
		\brief Header Name
		*/
		char* Field;
		/*! \var FieldLength
		\brief Length of \a Field
		*/
		int FieldLength;
		/*! \var FieldData
		\brief Header Value
		*/
		char* FieldData;
		/*! \var FieldDataLength
		\brief Length of \a FieldData
		*/
		int FieldDataLength;
		/*! \var UserAllocStrings
		\brief Boolean indicating if the above strings are non-static memory
		*/
		int UserAllocStrings;
		/*! \var NextField
		\brief Pointer to the Next Header entry. NULL if this is the last one
		*/
		struct packetheader_field_node* NextField;
	}packetheader_field_node;

	/*! \struct packetheader ILibParsers.h
	\brief Structure representing a packet formatted according to HTTP encoding rules
	\par
	This can be created manually by calling \a ILibCreateEmptyPacket(), or automatically via a call to \a ILibParsePacketHeader(...)
	*/
	typedef struct packetheader
	{
		/*! \var Directive
		\brief HTTP Method
		\par
		eg: \b GET /index.html HTTP/1.1
		*/
		char* Directive;
		/*! \var DirectiveLength
		\brief Length of \a Directive
		*/
		int DirectiveLength;
		/*! \var DirectiveObj
		\brief HTTP Method Path
		\par
		eg: GET \b /index.html HTTP/1.1
		*/
		char* DirectiveObj;
		/*! \var DirectiveObjLength
		\brief Length of \a DirectiveObj
		*/

		void *Reserved;

		int DirectiveObjLength;
		/*! \var StatusCode
		\brief HTTP Response Code
		\par
		eg: HTTP/1.1 \b 200 OK
		*/
		int StatusCode;
		/* \var StatusData
		\brief Status Meta Data
		\par
		eg: HTTP/1.1 200 \b OK
		*/
		char* StatusData;
		/*! \var StatusDataLength
		\brief Length of \a StatusData
		*/
		int StatusDataLength;
		/*! \var Version
		\brief HTTP Version
		\par
		eg: 1.1
		*/
		char* Version;
		/*! \var VersionLength
		\brief Length of \a Version
		*/
		int VersionLength;
		/*! \var Body
		\brief Pointer to HTTP Body
		*/
		char* Body;
		/*! \var BodyLength
		\brief Length of \a Body
		*/
		int BodyLength;
		/*! \var UserAllocStrings
		\brief Boolean indicating if Directive/Obj are non-static
		\par
		This only needs to be set, if you manually populate \a Directive and \a DirectiveObj.<br>
		It is \b recommended that you use \a ILibSetDirective
		*/
		int UserAllocStrings;	// Set flag if you allocate memory pointed to by Directive/Obj
		/*! \var UserAllocVersion
		\brief Boolean indicating if Version string is non-static
		\par
		This only needs to be set, if you manually populate \a Version.<br>
		It is \b recommended that you use \a ILibSetVersion
		*/	
		int UserAllocVersion;	// Set flag if you allocate memory pointed to by Version
		int ClonedPacket;

		/*! \var FirstField
		\brief Pointer to the first Header field
		*/
		struct packetheader_field_node* FirstField;
		/*! \var LastField
		\brief Pointer to the last Header field
		*/
		struct packetheader_field_node* LastField;

		/*! \var Source
		\brief The origin of this packet
		\par
		This is only populated if you obtain this structure from either \a ILibWebServer or
		\a ILibWebClient.
		*/
		char Source[30];
		/*! \var ReceivingAddress
		\brief IP address that this packet was received on
		\par
		This is only populated if you obtain this structure from either \a ILibWebServer or
		\a ILibWebClient.
		*/
		char ReceivingAddress[30];

		void *HeaderTable;
	}ILibHTTPPacket;

	/*! \struct ILibXMLNode
	\brief An XML Tree
	\par
	This is obtained via a call to \a ILibParseXML. It is \b highly \b recommended
	that you call \a ILibProcessXMLNodeList, so that the node pointers at the end of this
	structure will be populated. That will greatly simplify node traversal.<br><br>
	In order for namespaces to be resolved, you must call \a ILibXML_BuildNamespaceLookupTable(...)
	with root-most node that you would like to resolve namespaces for. It is recommended that you always use
	the root node, obtained from the initial call to \a ILibParseXML.<br><br>
	For most intents and purposes, you only need to work with the "StartElements"
	*/
	struct ILibXMLNode
	{
		/*! \var Name
		\brief Local Name of the current element
		*/
		char* Name;			// Element Name
		/*! \var NameLength
		\brief Length of \a Name
		*/
		int NameLength;

		/*! \var NSTag
		\brief Namespace Prefix of the current element
		\par
		This can be resolved using a call to \a ILibXML_LookupNamespace(...)
		*/
		char* NSTag;		// Element Prefix
		/*! \var NSLength
		\brief Length of \a NSTag
		*/
		int NSLength;

		/*! \var StartTag
		\brief boolean indicating if the current element is a start element
		*/
		int StartTag;		// Non zero if this is a StartElement
		/*! \var EmptyTag
		\brief boolean indicating if this element is an empty element
		*/
		int EmptyTag;		// Non zero if this is an EmptyElement

		void *Reserved;		// DO NOT TOUCH
		void *Reserved2;	// DO NOT TOUCH

		/*! \var Next
		\brief Pointer to the child of the current node
		*/
		struct ILibXMLNode *Next;			// Next Node
		/*! \var Parent
		\brief Pointer to the Parent of the current node
		*/
		struct ILibXMLNode *Parent;			// Parent Node
		/*! \var Peer
		\brief Pointer to the sibling of the current node
		*/
		struct ILibXMLNode *Peer;			// Sibling Node
		struct ILibXMLNode *ClosingTag;		// Pointer to closing node of this element
		struct ILibXMLNode *StartingTag;	// Pointer to start node of this element
	};

	/*! \struct ILibXMLAttribute
	\brief A list of XML Attributes for a specified XML node
	\par
	This can be obtained via a call to \a ILibGetXMLAttributes(...)
	*/
	struct ILibXMLAttribute
	{
		/*! \var Name
		\brief Local name of Attribute
		*/
		char* Name;						// Attribute Name
		/*! \var NameLength
		\brief Length of \a Name
		*/
		int NameLength;

		/*! \var Prefix
		\brief Namespace Prefix of this attribute
		\par
		This can be resolved by calling \a ILibXML_LookupNamespace(...) and passing in \a Parent as the current node
		*/
		char* Prefix;					// Attribute Namespace Prefix
		/*! \var PrefixLength
		\brief Lenth of \a Prefix
		*/
		int PrefixLength;

		/*! \var Parent
		\brief Pointer to the XML Node that contains this attribute
		*/
		struct ILibXMLNode *Parent;		// The XML Node this attribute belongs to

		/*! \var Value
		\brief Attribute Value
		*/
		char* Value;					// Attribute Value	
		/*! \var ValueLength
		\brief Length of \a Value
		*/
		int ValueLength;
		/*! \var Next
		\brief Pointer to the next attribute
		*/
		struct ILibXMLAttribute *Next;	// Next Attribute
	};

	char *ILibReadFileFromDisk(char *FileName);
	int ILibReadFileFromDiskEx(char **Target, char *FileName);
	void ILibWriteStringToDisk(char *FileName, char *data);
	void ILibAppendStringToDiskEx(char *FileName, char *data, int dataLen);
	void ILibWriteStringToDiskEx(char *FileName, char *data, int dataLen);
	void ILibDeleteFileFromDisk(char *FileName);
	void ILibGetDiskFreeSpace(void *i64FreeBytesToCaller, void *i64TotalBytes);


	/*! \defgroup StackGroup Stack
	\ingroup DataStructures
	Stack Methods
	\{
	*/
	void ILibCreateStack(void **TheStack);
	void ILibPushStack(void **TheStack, void *data);
	void *ILibPopStack(void **TheStack);
	void *ILibPeekStack(void **TheStack);
	void ILibClearStack(void **TheStack);
	/*! \} */

	/*! \defgroup QueueGroup Queue
	\ingroup DataStructures
	Queue Methods
	\{
	*/
	typedef void* ILibQueue;
	ILibQueue ILibQueue_Create();
	void ILibQueue_Destroy(ILibQueue q);
	int ILibQueue_IsEmpty(ILibQueue q);
	void ILibQueue_EnQueue(ILibQueue q, void *data);
	void *ILibQueue_DeQueue(ILibQueue q);
	#define ILibQueue_PeekTail(q) ILibLinkedList_GetDataFromNode(ILibLinkedList_GetNode_Tail(q))
	void *ILibQueue_PeekQueue(ILibQueue q);
	void ILibQueue_Lock(ILibQueue q);
	void ILibQueue_UnLock(ILibQueue q);
	long ILibQueue_GetCount(ILibQueue q);
	/* \} */


	/*! \defgroup XML XML Parsing Methods
	\ingroup ILibParsers
	MicroStack supplied XML Parsing Methods
	\par
	\b Note: None of the XML Parsing Methods copy or allocate memory
	The lifetime of any/all strings is bound to the underlying string that was
	parsed using ILibParseXML. If you wish to keep any of these strings for longer
	then the lifetime of the underlying string, you must copy the string.
	\{
	*/


	//
	// Parses an XML string. Returns a tree of ILibXMLNode elements.
	//
	struct ILibXMLNode *ILibParseXML(char *buffer, int offset, int length);

	//
	// Preprocesses the tree of ILibXMLNode elements returned by ILibParseXML.
	// This method populates all the node pointers in each node for easy traversal.
	// In addition, this method will also determine if the given XML document was well formed.
	// Returns 0 if processing succeeded. Specific Error Codes are returned otherwise.
	//
	int ILibProcessXMLNodeList(struct ILibXMLNode *nodeList);

	//
	// Initalizes a namespace lookup table for a given parent node. 
	// If you want to resolve namespaces, you must call this method exactly once
	//
	void ILibXML_BuildNamespaceLookupTable(struct ILibXMLNode *node);

	//
	// Resolves a namespace prefix.
	//
	char* ILibXML_LookupNamespace(struct ILibXMLNode *currentLocation, char *prefix, int prefixLength);

	//
	// Fetches all the data for an element. Returns the length of the populated data
	//
	int ILibReadInnerXML(struct ILibXMLNode *node, char **RetVal);

	//
	// Returns the attributes of an XML element. Returned as a linked list of ILibXMLAttribute.
	//
	struct ILibXMLAttribute *ILibGetXMLAttributes(struct ILibXMLNode *node);

	void ILibDestructXMLNodeList(struct ILibXMLNode *node);
	void ILibDestructXMLAttributeList(struct ILibXMLAttribute *attribute);

	//
	// Escapes an XML string.
	// indata must be pre-allocated. 
	//
	int ILibXmlEscapeEx(char* outdata, const char* indata, size_t indataLen);
	#define ILibXmlEscape(outdata, indata) ILibXmlEscapeEx(outdata, indata, strnlen_s(indata, sizeof(ILibScratchPad2)))
	//
	// Returns the required size string necessary to escape this XML string
	//
	int ILibXmlEscapeLengthEx(const char* data, size_t dataLen);
	#define ILibXmlEscapeLength(data) ILibXmlEscapeLengthEx(data, strnlen_s(data, sizeof(ILibScratchPad2)))
	//
	// Unescapes an XML string.
	// Since Unescaped strings are always shorter than escaped strings,
	// the resultant string will overwrite the supplied string, to save memory
	//
	int ILibInPlaceXmlUnEscapeEx(char* data, size_t dataLen);
	#define ILibInPlaceXmlUnEscape(data) ILibInPlaceXmlUnEscapeEx(data, strnlen_s(data, sizeof(ILibScratchPad2)))
	/*! \} */

	/*! \defgroup ChainGroup Chain Methods
	\ingroup ILibParsers
	\brief Chaining Methods
	\{
	*/
	ILibExportMethod void *ILibCreateChain();
	void *ILibCreateChainEx(int extraMemorySize);
	void ILibAddToChain(void *chain, void *object);
	void *ILibGetBaseTimer(void *chain);
	void ILibChain_SafeAdd(void *chain, void *object);
	void ILibChain_SafeRemove(void *chain, void *object);
	void ILibChain_SafeRemoveEx(void *chain, void *object);
	void ILibChain_DestroyEx(void *chain);
	ILibExportMethod void ILibStartChain(void *chain);
	ILibExportMethod void ILibStopChain(void *chain);
	ILibExportMethod void ILibChain_Continue(void *chain, ILibChain_Link **modules, int moduleCount, int maxTimeout);
	ILibExportMethod void ILibChain_EndContinue(void *chain);

	void ILibForceUnBlockChain(void *Chain);
	void ILibChain_RunOnMicrostackThreadEx(void *chain, ILibChain_StartEvent handler, void *user);
	#define ILibChain_RunOnMicrostackThread(chain, handler, user) if(ILibIsRunningOnChainThread(chain)==0){ILibChain_RunOnMicrostackThreadEx(chain, handler, user);}else{handler(chain,user);}
	/* \} */

	/*! \defgroup SparseArrayGroup Sparse Array
	\ingroup DataStructures
	Sparse Array is a compact array where most of the elements will have the default value [NULL].
	*@{
	*/

	typedef void* ILibSparseArray;
	//! Event handler triggered when an index needs to be hashed into a bucket value
	/*!
		\param value Index value to be hashed
		\return Hashed Bucket value
	*/
	typedef int(*ILibSparseArray_Bucketizer)(int value);
	//! Generic Event handler
	/*!
		\param sender The Sparse Array that generated the event
		\param index Index Value
		\param value Data Value
		\param user Custom user state object
	*/
	typedef void(*ILibSparseArray_OnValue)(ILibSparseArray sender, int index, void *value, void *user);
	extern const int ILibMemory_SparseArray_CONTAINERSIZE;
	ILibSparseArray ILibSparseArray_CreateWithUserMemory(int numberOfBuckets, ILibSparseArray_Bucketizer bucketizer, int userMemorySize);
	#define ILibSparseArray_Create(numberOfBuckets, bucketizer) ILibSparseArray_CreateWithUserMemory(numberOfBuckets, bucketizer, 0)
	ILibSparseArray ILibSparseArray_CreateEx(ILibSparseArray source);
	void* ILibSparseArray_Add(ILibSparseArray sarray, int index, void *data);
	void* ILibSparseArray_Get(ILibSparseArray sarray, int index);
	void* ILibSparseArray_Remove(ILibSparseArray sarray, int index);
	void ILibSparseArray_Destroy(ILibSparseArray sarray);
	void ILibSparseArray_DestroyEx(ILibSparseArray sarray, ILibSparseArray_OnValue onDestroy, void *user);
	void ILibSparseArray_ClearEx2(ILibSparseArray sarray, ILibSparseArray_OnValue onClear, void *user, int nonZeroWillDelete);
	#define ILibSparseArray_ClearEx(sarray, onClear, user) ILibSparseArray_ClearEx2(sarray, onClear, user, 1)
	#define ILibSparseArray_Enumerate(sarray, onEnumerate, user) ILibSparseArray_ClearEx2(sarray, onEnumerate, user, 0)
	ILibSparseArray ILibSparseArray_Move(ILibSparseArray sarray);

	void ILibSparseArray_Lock(ILibSparseArray sarray);
	void ILibSparseArray_UnLock(ILibSparseArray sarray);

	/*!	
	*@}
	*/

	/*! \defgroup AdvHashtableGroup Advanced Hashtable
	\ingroup DataStructures
	Advanced Hashtable is a customizable Hashtable data structure, that can be optimized as necessary
	*@{
	*/

	typedef void* ILibHashtable;
	//! Event handler for hash requests [Key1 or Key2 or Both]
	/*!
		\param Key1 Address Key
		\param Key2 String Key
		\param Key2Len String Key Length
		\return Hash result
	*/
	typedef int(*ILibHashtable_Hash_Func)(void* Key1, char* Key2, int Key2Len);
	//! Event handler for when elements in the hashtable are freed
	/*!
		\param sender Hashtable that generated the request
		\param Key1 Address Key
		\param Key2 String Key
		\param Key2Len String Key Length
		\param Data Element Value
		\param user Custom User State data
	*/
	typedef void(*ILibHashtable_OnDestroy)(ILibHashtable sender, void *Key1, char* Key2, int Key2Len, void *Data, void *user);

	ILibHashtable ILibHashtable_Create();

	void ILibHashtable_DestroyEx(ILibHashtable table, ILibHashtable_OnDestroy onDestroy, void *user);
	//! Free resources associated with Advanced Hashtable
	/*!
		\param hashtable Hashtable
	*/
	#define ILibHashtable_Destroy(hashtable) ILibHashtable_DestroyEx(hashtable, NULL, NULL)
	void ILibHashtable_ChangeHashFunc(ILibHashtable table, ILibHashtable_Hash_Func hashFunc);
	void ILibHashtable_ChangeBucketizer(ILibHashtable table, int bucketCount, ILibSparseArray_Bucketizer bucketizer);
	void* ILibHashtable_Put(ILibHashtable table, void *Key1, char* Key2, int Key2Len, void* Data);
	void* ILibHashtable_Get(ILibHashtable table, void *Key1, char* Key2, int Key2Len);
	void* ILibHashtable_Remove(ILibHashtable table, void *Key1, char* Key2, int Key2Len);
	void ILibHashtable_Clear(ILibHashtable table);
	void ILibHashtable_ClearEx(ILibHashtable table, ILibHashtable_OnDestroy onClear, void *user);
	void ILibHashtable_Enumerate(ILibHashtable table, ILibHashtable_OnDestroy onEnumerate, void *user);
	void ILibHashtable_Lock(ILibHashtable table);
	void ILibHashtable_UnLock(ILibHashtable table);
	/*!
	*@}
	*/

	/*! \defgroup LinkedListGroup Linked List
	\ingroup DataStructures
	*@{
	*/

	typedef void* ILibLinkedList;

	/*!
		\ingroup ChainGroup
	*@{
	*/

	typedef struct ILibLinkedList_FileBacked_Root
	{
		unsigned int flags;
		unsigned int maxSize;
		unsigned int head;
		unsigned int tail;
	}ILibLinkedList_FileBacked_Root;
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4200 )
#endif
	typedef struct ILibLinkedList_FileBacked_Node
	{
		unsigned int next;
		unsigned int dataLen;
		char data[];
	}ILibLinkedList_FileBacked_Node;
#ifdef WIN32
#pragma warning( pop )
#endif
	void ILibLinkedList_FileBacked_Close(ILibLinkedList_FileBacked_Root *root);
	int ILibLinkedList_FileBacked_IsEmpty(ILibLinkedList_FileBacked_Root *root);
	ILibLinkedList_FileBacked_Node* ILibLinkedList_FileBacked_ReadNext(ILibLinkedList_FileBacked_Root* root, ILibLinkedList_FileBacked_Node* current);
	ILibLinkedList_FileBacked_Root* ILibLinkedList_FileBacked_Create(char* path, unsigned int maxFileSize, int maxRecordSize);
	void ILibLinkedList_FileBacked_Reset(ILibLinkedList_FileBacked_Root *root);
	int ILibLinkedList_FileBacked_AddTail(ILibLinkedList_FileBacked_Root* root, char* data, unsigned int dataLen);
	int ILibLinkedList_FileBacked_ReloadRoot(ILibLinkedList_FileBacked_Root* root);
	void ILibLinkedList_FileBacked_SaveRoot(ILibLinkedList_FileBacked_Root* root);


	ILibLinkedList ILibChain_GetLinks(void *chain);
	ILibHashtable ILibChain_GetBaseHashtable(void* chain);
	/*! @} */


	void* ILibLinkedList_Create();
	void* ILibLinkedList_CreateEx(int userMemorySize);
	void ILibLinkedList_SetTag(ILibLinkedList list, void *tag);
	void* ILibLinkedList_GetTag(ILibLinkedList list);
	int ILibLinkedList_GetIndex(void *node);

	//! Comparer delegate is called to compare two values. Mimics behavior of .NET IComparer..
	//! obj2 == obj1	: 0
	//! obj2 < obj1	: -1
	//! obj2 > obj1	: +1
	/*!
		\param obj1 Compare this object
		\param obj2 with this object
		\return Result [0: EQ, -1: LT, +1: GT]
	*/
	typedef int(*ILibLinkedList_Comparer)(void* obj1, void *obj2);

	//! Chooser handler is called when a new item is to be added to Linked List. The old value is passed in as well as the new value... The return value will be the designated added node
	/*!
		\param oldObject Current Element Value [NULL if it doesn't exist]
		\param newObject Proposed Element Value
		\param user Custom user state data
		\return Element Value to be kept
	*/
	typedef void*(*ILibLinkedList_Chooser)(void* oldObject, void *newObject, void *user);

	void* ILibLinkedList_SortedInsert(void* LinkedList, ILibLinkedList_Comparer comparer, void *data);
	void* ILibLinkedList_SortedInsertEx(void* LinkedList, ILibLinkedList_Comparer comparer, ILibLinkedList_Chooser chooser, void *data, void *user);
	void* ILibLinkedList_GetNode_Search(void* LinkedList, ILibLinkedList_Comparer comparer, void *matchWith);

	void* ILibLinkedList_GetNode_Head(void *LinkedList);		// Returns Node
	void* ILibLinkedList_GetNode_Tail(void *LinkedList);		// Returns Node
	void* ILibLinkedList_GetNextNode(void *LinkedList_Node);	// Returns Node
	void* ILibLinkedList_GetPreviousNode(void *LinkedList_Node);// Returns Node
	long ILibLinkedList_GetCount(void *LinkedList);
	void* ILibLinkedList_ShallowCopy(void *LinkedList);
	void *ILibLinkedList_GetDataFromNode(void *LinkedList_Node);
	void* ILibLinkedList_InsertBefore(void *LinkedList_Node, void *data);
	void* ILibLinkedList_InsertAfter(void *LinkedList_Node, void *data);
	void* ILibLinkedList_Remove(void *LinkedList_Node);
	int ILibLinkedList_Remove_ByData(void *LinkedList, void *data);
	void* ILibLinkedList_AddHead(void *LinkedList, void *data);
	void* ILibLinkedList_AddTail(void *LinkedList, void *data);
	#define ILibLinkedList_Clear(LinkedList) while(ILibLinkedList_GetNode_Head(LinkedList)!=NULL){ILibLinkedList_Remove(ILibLinkedList_GetNode_Head(LinkedList));}

	void ILibLinkedList_Lock(void *LinkedList);
	void ILibLinkedList_UnLock(void *LinkedList);
	void ILibLinkedList_Destroy(void *LinkedList);
	void* ILibLinkedList_GetExtendedMemory(void* LinkedList_Node);
	/*! 
	*@} 
	*/



	/*! \defgroup HashTreeGroup Hash Table
	\ingroup DataStructures
	\b Note: Duplicate key entries will be overwritten.
	*@{
	*/

	void* ILibInitHashTree();
	void* ILibInitHashTree_CaseInSensitive();
	void ILibDestroyHashTree(void *tree);
	int ILibHasEntry(void *hashtree, char* key, int keylength);
	void ILibAddEntry(void* hashtree, char* key, int keylength, void *value);
	void ILibAddEntryEx(void* hashtree, char* key, int keylength, void *value, int valueEx);
	void* ILibGetEntry(void *hashtree, char* key, int keylength);
	ILibExportMethod void ILibGetEntryEx(void *hashtree, char* key, int keylength, void **value, int *valueEx);
	void ILibDeleteEntry(void *hashtree, char* key, int keylength);

	void *ILibHashTree_GetEnumerator(void *tree);
	void ILibHashTree_DestroyEnumerator(void *tree_enumerator);
	int ILibHashTree_MoveNext(void *tree_enumerator);

	void ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data);
	void ILibHashTree_GetValueEx(void *tree_enumerator, char **key, int *keyLength, void **data, int *dataEx);
	void ILibHashTree_Lock(void *hashtree);
	void ILibHashTree_UnLock(void *hashtree);

	/*! 
	*@} 
	*/

	/*! \defgroup LifeTimeMonitor LifeTimeMonitor
	\ingroup ILibParsers
	\brief Timed Callback Service
	\par
	These callbacks will always be triggered on the thread that calls ILibStartChain().
	\{
	*/

	typedef void(*ILibLifeTime_OnCallback)(void *obj);

	//
	// Adds an event trigger to be called after the specified time elapses, with the
	// specified data object
	//
#define ILibLifeTime_Add(LifetimeMonitorObject, data, seconds, Callback, Destroy) ILibLifeTime_AddEx(LifetimeMonitorObject, data, seconds * 1000, Callback, Destroy)
	void ILibLifeTime_AddEx(void *LifetimeMonitorObject,void *data, int milliseconds, ILibLifeTime_OnCallback Callback, ILibLifeTime_OnCallback Destroy);

	//
	// Removes all event triggers that contain the specified data object.
	//
	void ILibLifeTime_Remove(void *LifeTimeToken, void *data);

	//
	// Return the expiration time for an event
	//
	long long ILibLifeTime_GetExpiration(void *LifetimeMonitorObject, void *data);

	//
	// Removes all events triggers
	//
	void ILibLifeTime_Flush(void *LifeTimeToken);
	void *ILibCreateLifeTime(void *Chain);
	long ILibLifeTime_Count(void* LifeTimeToken);

	/* \} */


	/*! \defgroup StringParsing String Parsing
	\ingroup ILibParsers
	\{
	*/

	int ILibFindEntryInTable(char *Entry, char **Table);


	int ILibTrimString(char **theString, int length);
	int ILibString_IndexOfFirstWhiteSpace(const char *inString, int inStringLength);
	char* ILibString_Cat(const char *inString1, int inString1Len, const char *inString2, int inString2Len);
	char* ILibString_Cat_s(char *destination, size_t destinationSize, char *source);
	char *ILibString_Copy(const char *inString, int length);
	int ILibString_Copy_s(char *destination, size_t destinationSize, char *source);
	int ILibString_n_Copy_s(char *destination, size_t destinationSize, char *source, size_t count);
	int ILibString_EndsWith(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength);
	int ILibString_EndsWithEx(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength, int caseSensitive);
	int ILibString_StartsWith(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength);
	int ILibString_StartsWithEx(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength, int caseSensitive);
	int ILibString_IndexOfEx(const char *inString, int inStringLength, const char *indexOf, int indexOfLength, int caseSensitive);
	int ILibString_IndexOf(const char *inString, int inStringLength, const char *indexOf, int indexOfLength);
	int ILibString_LastIndexOf(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength);
	int ILibString_LastIndexOfEx(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength, int caseSensitive);
	char *ILibString_Replace(const char *inString, int inStringLength, const char *replaceThis, int replaceThisLength, const char *replaceWithThis, int replaceWithThisLength);
	char *ILibString_ToUpper(const char *inString, int length);
	char *ILibString_ToLower(const char *inString, int length);
	void ILibToUpper(const char *in, int inLength, char *out);
	void ILibToLower(const char *in, int inLength, char *out);
#if !defined(WIN32)
	#ifndef strcat_s
		#define strcat_s(destination, destinationSize, source) ILibString_Cat_s(destination, destinationSize, source)
	#endif
	#ifndef strcpy_s
		#define strcpy_s(destination, destinationSize, source) ILibString_Copy_s(destination, destinationSize, source);
	#endif
#endif

	struct parser_result* ILibParseString (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength);
	struct parser_result* ILibParseStringAdv (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength);
	parser_result_field* ILibParseString_GetResultIndex(parser_result* r, int index);
	void ILibDestructParserResults(struct parser_result *result);

	typedef enum ILibParseUriResult
	{
		ILibParseUriResult_UNKNOWN_SCHEME = 0,
		ILibParseUriResult_NO_TLS = 1,
		ILibParseUriResult_TLS = 2
	}ILibParseUriResult;

	ILibParseUriResult ILibParseUriEx(const char* URI, size_t URILen, char** Addr, unsigned short* Port, char** Path, struct sockaddr_in6* AddrStruct);
	#define ILibParseUri(URI, pAddr, pPort, pPath, pAddrStruct) ILibParseUriEx(URI, strnlen_s(URI, sizeof(ILibScratchPad)), pAddr, pPort, pPath, pAddrStruct)

	int ILibGetLong(char *TestValue, int TestValueLength, long* NumericValue);
	int ILibGetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue);
	int ILibFragmentText(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength, char **RetVal);
	int ILibFragmentTextLength(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength);


	/* Base64 handling methods */
	int ILibBase64EncodeLength(const int inputLen);
	int ILibBase64DecodeLength(const int inputLen);
	int ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output);
	int ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output);

	/* Compression Handling Methods */
	char* ILibDecompressString(unsigned char* CurrentCompressed, const int bufferLength, const int DecompressedLength);

	/* \} */


	/*! \defgroup PacketParsing Packet Parsing
	\ingroup ILibParsers
	\{
	*/

	/* Packet Methods */

	struct packetheader *ILibCreateEmptyPacket();
	void ILibAddHeaderLine(struct packetheader *packet, const char* FieldName, int FieldNameLength, const char* FieldData, int FieldDataLength);
	void ILibDeleteHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);
	void ILibHTTPPacket_Stash_Put(ILibHTTPPacket *packet, char* key, int keyLen, void *data);
	int ILibHTTPPacket_Stash_HasKey(ILibHTTPPacket *packet, char* key, int keyLen);
	void* ILibHTTPPacket_Stash_Get(ILibHTTPPacket *packet, char* key, int keyLen);

	char* ILibUrl_GetHost(char *url, int urlLen);
	#define ILibGetHeaderLine(packet,  FieldName, FieldNameLength) ILibGetHeaderLineEx(packet, FieldName, FieldNameLength, NULL)
	char* ILibGetHeaderLineEx(struct packetheader *packet, char* FieldName, int FieldNameLength, int *len);
	char* ILibGetHeaderLineSP(struct packetheader *packet, char* FieldName, int FieldNameLength);
	char* ILibGetHeaderLineSP_Next(char* PreviousValue, char* FieldName, int FieldNameLength);
	void ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength);
	void ILibSetStatusCode(struct packetheader *packet, int StatusCode, char* StatusData, int StatusDataLength);
	void ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength);
	void ILibDestructPacket(struct packetheader *packet);
	struct packetheader* ILibParsePacketHeader(char* buffer, int offset, int length);
	int ILibGetRawPacket(struct packetheader *packet,char **buffer);
	struct packetheader* ILibClonePacket(struct packetheader *packet);

	int ILibHTTPEscapeEx(char* outdata, const char* indata, size_t indataLen);
	int ILibHTTPEscapeLengthEx(const char* data, size_t dataLen);
	#define ILibHTTPEscape(outdata, indata) ILibHTTPEscapeEx(outdata, indata, strnlen_s(indata, sizeof(ILibScratchPad)))
	#define ILibHTTPEscapeLength(data) ILibHTTPEscapeLengthEx(data, strnlen_s(data, sizeof(ILibScratchPad)))

	int ILibInPlaceHTTPUnEscapeEx(char* data, int length);
	#define ILibInPlaceHTTPUnEscape(data) ILibInPlaceHTTPUnEscapeEx(data, (int)strnlen_s(data, sizeof(ILibScratchPad)))
	/* \} */

	void* dbg_malloc(int sz);
	void dbg_free(void* ptr);
	int dbg_GetCount();

	/*! \defgroup NetworkHelper Network Helper
	\ingroup ILibParsers
	*@{
	*/

	int ILibGetLocalIPv6IndexList(int** indexlist);
	int ILibGetLocalIPv6List(struct sockaddr_in6** list);
	int ILibGetLocalIPAddressList(int** pp_int);
	int ILibGetLocalIPv4AddressList(struct sockaddr_in** addresslist, int includeloopback);
	//int ILibGetLocalIPv6AddressList(struct sockaddr_in6** addresslist);

#if defined(WINSOCK2)
	int ILibGetLocalIPAddressNetMask(unsigned int address);
#endif

	SOCKET ILibGetSocket(struct sockaddr *localif, int type, int protocol);

	//
	// IPv6 helper methods
	//
	void ILibMakeIPv6Addr(struct sockaddr *addr4, struct sockaddr_in6* addr6);
	int ILibMakeHttpHeaderAddr(struct sockaddr *addr, char** str);
	int ILibIsIPv4MappedAddr(struct sockaddr *addr);
	int ILibIsLoopback(struct sockaddr *addr);
	int ILibGetAddrBlob(struct sockaddr *addr, char** ptr);
	void ILibGetAddrFromBlob(char* ptr, int len, unsigned short port, struct sockaddr_in6 *addr);
	int ILibDetectIPv6Support();
	extern int g_ILibDetectIPv6Support;
	char* ILibInet_ntop2(struct sockaddr* addr, char *dst, size_t dstsize);
	char* ILibInet_ntop(int af, const void *src, char *dst, size_t dstsize);
	int ILibInet_pton(int af, const char *src, void *dst);
	int ILibInetCompare(struct sockaddr* addr1, struct sockaddr* addr2, int compare);	// Compare=1 family only, 2 family and address, 3 familly, address and port.
	int ILibResolve(char* hostname, char* service, struct sockaddr_in6* addr6);
	int ILibResolveEx(char* hostname, unsigned short port, struct sockaddr_in6* addr6);
	void ILib6to4(struct sockaddr* addr);
	#define ILibInet_StructSize(addr) ((((struct sockaddr*)(addr))->sa_family == AF_INET6)?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in))

	/*! @} */
	//
	// Used to log critical problems
	//
	char* ILibChain_Debug(void *chain, char* buffer, int bufferLen);
#if defined(WIN32)
	int ILib_WindowsExceptionFilter(int exceptionCode, void *exceptionInfo, CONTEXT *exceptionContext);
	void ILib_WindowsExceptionDebug(CONTEXT *exceptionContext);
#elif defined(_POSIX)
	char* ILib_POSIX_InstallCrashHandler(char *exename);
#endif

#define ILIBCRITICALEXITMSG(code, msg) {printf("%s", ILibCriticalLog(msg, NULL, 0, 0, 0)); exit(code);}
#if defined(WIN32)
#define ILIBCRITICALERREXIT(ex) { ILibCriticalLog(NULL, __FILE__, __LINE__, GetLastError(), 0); exit(ex); }
#define ILIBCRITICALEXIT(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, GetLastError());printf("CRITICALEXIT, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT2(ex,u) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT2, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT3(ex,m,u) {ILibCriticalLog(m, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT3, FILE: %s, LINE: %d\r\nMessage: %s\r\n", __FILE__, __LINE__, m); exit(ex);}
#define ILIBMARKPOSITION(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, GetLastError());}
#define ILIBMESSAGE(m) {ILibCriticalLog(m, __FILE__, __LINE__, 0, GetLastError());printf("ILIBMSG: %s (%d).\r\n", m, (int)GetLastError());}
#define ILIBMESSAGE2(m,u) {ILibCriticalLog(m, __FILE__, __LINE__, u, GetLastError());printf("ILIBMSG: %s (%d,%d).\r\n", m, (int)u, (int)GetLastError());}
#else
extern void ILib_POSIX_CrashHandler(int code);
#define ILIBCRITICALERREXIT(ex) { ILibCriticalLog(NULL, __FILE__, __LINE__, errno, 0); fflush(stdout); exit(ex); }
#ifdef _POSIX
#define ILIBCRITICALEXIT(ex) ILib_POSIX_CrashHandler(ex);
#else
#define ILIBCRITICALEXIT(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, errno); printf("CRITICALEXIT, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); fflush(stdout); exit(ex); }
#endif
#define ILIBCRITICALEXIT2(ex,u) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT2, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__);fflush(stdout); exit(ex);}
#define ILIBCRITICALEXIT3(ex,m,u) {ILibCriticalLog(m, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT3, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__);fflush(stdout); exit(ex);}
#define ILIBMARKPOSITION(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, errno);}
#define ILIBMESSAGE(m) {ILibCriticalLog(m, __FILE__, __LINE__, 0, errno);printf("ILIBMSG: %s\r\n", m);fflush(stdout);}
#define ILIBMESSAGE2(m,u) {ILibCriticalLog(m, __FILE__, __LINE__, u, errno);printf("ILIBMSG: %s (%d)\r\n", m, u);fflush(stdout);}
#endif

	char* ILibCriticalLog(const char* msg, const char* file, int line, int user1, int user2);

	/*! \defgroup ThreadingHelper Threading Helper
	\ingroup ILibParsers
	*@{
	*/
	void* ILibSpawnNormalThread(voidfp1 method, void* arg);
	void  ILibEndThisThread();
#ifdef WIN32
	void ILibHandle_DisableInherit(HANDLE *h);
#endif
	/*! @} */


	/*! \defgroup ConversionGroup Conversion Utilities
	\ingroup ILibParsers
	*@{
	*/
	char* ILibToHex(char* data, int len, char* out);
	int ILibWhichPowerOfTwo(int number);
#define ILibPowerOfTwo(exponent) (0x01 << exponent)
	/*! @} */
#ifdef __cplusplus
}
#endif

/* \} */   // End of ILibParser Group
#endif

