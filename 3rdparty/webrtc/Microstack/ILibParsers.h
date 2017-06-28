/*   
Copyright 2006 - 2015 Intel Corporation

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
	\{
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
#include <netinet/in.h>
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

#include "ILibRemoteLogging.h"

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

#if defined(_DEBUG)
#define PRINTERROR() printf("ERROR in %s, line %d\r\n", __FILE__, __LINE__);
#else
#define PRINTERROR()
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

#if !defined(WIN32) 
#define __fastcall
#endif

	typedef void (*voidfp)(void);		// Generic function pointer
	extern char ILibScratchPad[4096];   // General buffer
	extern char ILibScratchPad2[65536]; // Often used for UDP packet processing
	extern void* gILibChain;			// Global Chain used for Remote Logging when a chain instance is not otherwise exposed

	/*! \def UPnPMIN(a,b)
	Returns the minimum of \a a and \a b.
	*/
#define UPnPMIN(a,b) (((a)<(b))?(a):(b))
	/*! \def ILibIsChainBeingDestroyed(Chain)
	Determines if the specified chain is in the process of being disposed.
	*/
#define ILibIsChainBeingDestroyed(Chain) (*((int*)Chain))
#define ILibIsChainRunning(Chain) (((int*)Chain)[1])

	int ILibIsRunningOnChainThread(void* chain);

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

#ifdef _REMOTELOGGING
	#define ILibChainSetLogger(chain, logger) ((void**)&((int*)chain)[2])[0] = logger
	#define ILibChainGetLogger(chain) (chain!=NULL?(((void**)&((int*)chain)[2])[0]):NULL)
	#ifdef _REMOTELOGGINGSERVER
		unsigned short ILibStartDefaultLogger(void* chain, unsigned short portNumber);
	#else
		#define ILibStartDefaultLogger(chain, portNumber) 0
	#endif
#else
	#define ILibChainSetLogger(chain, logger) ;
	#define ILibChainGetLogger(chain) NULL
	#define ILibStartDefaultLogger(chain, portNumber) 0
#endif

#define ILibTransportChain(transport) ((ILibTransport*)transport)->Chain

	void ILibChain_OnDestroyEvent_AddHandler(void *chain, ILibChain_DestroyEvent sink, void *user);
	void ILibChain_OnStartEvent_AddHandler(void *chain, ILibChain_StartEvent sink, void *user);

#ifdef MICROSTACK_NOTLS
	#define NONCE_SIZE        32
	#define HALF_NONCE_SIZE   16

	char* __fastcall util_tohex(char* data, int len, char* out);
	int __fastcall util_hexToint(char *hexString, int hexStringLength);

	// Convert hex string to int 
	int __fastcall util_hexToBuf(char *hexString, int hexStringLength, char* output);
	// Perform a MD5 hash on the data
	void __fastcall util_md5(char* data, int datalen, char* result);

	// Perform a MD5 hash on the data and convert result to HEX and store in output
	// This is useful for HTTP Digest
	void __fastcall util_md5hex(char* data, int datalen, char *out);
#endif


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

	typedef enum ILibTransport_MemoryOwnership
	{
		ILibTransport_MemoryOwnership_CHAIN = 0, /*!< The Microstack will own this memory, and free it when it is done with it */
		ILibTransport_MemoryOwnership_STATIC = 1, /*!< This memory is static, so the Microstack will not free it, and assume it will not go away, so it won't copy it either */
		ILibTransport_MemoryOwnership_USER = 2 /*!< The Microstack doesn't own this memory, so if necessary the memory will be copied */
	}ILibTransport_MemoryOwnership;
	typedef enum ILibTransport_DoneState
	{
		ILibTransport_DoneState_INCOMPLETE = 0, 
		ILibTransport_DoneState_COMPLETE = 1,
		ILibTransport_DoneState_ERROR	= -4
	}ILibTransport_DoneState;

	typedef ILibTransport_DoneState(*ILibTransport_SendPtr)(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done);
	typedef void(*ILibTransport_ClosePtr)(void *transport);
	typedef unsigned int(*ILibTransport_PendingBytesToSendPtr)(void *transport);

	ILibTransport_DoneState ILibTransport_Send(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done);
	void ILibTransport_Close(void *transport);
	unsigned int ILibTransport_PendingBytesToSend(void *transport);

	typedef struct ILibTransport
	{
		void* Reserved[3];
		void* Chain;	
		ILibTransport_SendPtr Send;
		ILibTransport_ClosePtr Close;
		ILibTransport_PendingBytesToSendPtr PendingBytes;	
		unsigned int IdentifierFlags;
	}ILibTransport;



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
	struct parser_result_field
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
	};

	/*! \struct parser_result ILibParsers.h
	\brief String Parsing Result Index
	\par
	This is returned from a successfull call to either \a ILibParseString or
	\a ILibParseStringAdv.
	*/
	struct parser_result
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
	};

	/*! \struct packetheader_field_node ILibParsers.h
	\brief HTTP Headers
	\par
	This structure represents an individual header element. A list of these
	is referenced from a \a packetheader_field_node.
	*/
	struct packetheader_field_node
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
	};

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
	int ILibXmlEscape(char* outdata, const char* indata);

	//
	// Returns the required size string necessary to escape this XML string
	//
	int ILibXmlEscapeLength(const char* data);

	//
	// Unescapes an XML string.
	// Since Unescaped strings are always shorter than escaped strings,
	// the resultant string will overwrite the supplied string, to save memory
	//
	int ILibInPlaceXmlUnEscape(char* data);

	/*! \} */

	/*! \defgroup ChainGroup Chain Methods
	\ingroup ILibParsers
	\brief Chaining Methods
	\{
	*/
	void *ILibCreateChain();
	void ILibAddToChain(void *chain, void *object);
	void *ILibGetBaseTimer(void *chain);
	void ILibChain_SafeAdd(void *chain, void *object);
	void ILibChain_SafeRemove(void *chain, void *object);
	void ILibChain_DestroyEx(void *chain);
	void ILibStartChain(void *chain);
	void ILibStopChain(void *chain);

	void ILibForceUnBlockChain(void *Chain);
	/* \} */


	typedef void* ILibSparseArray;
	typedef int(*ILibSparseArray_Bucketizer)(int value);
	typedef void(*ILibSparseArray_OnValue)(ILibSparseArray sender, int index, void *value, void *user);

	// Allocates a new SparseArray using the specified number of buckets and the given bucketizer method, which maps indexes to buckets.
	ILibSparseArray ILibSparseArray_Create(int numberOfBuckets, ILibSparseArray_Bucketizer bucketizer);

	// Allocates a new SparseArray using the same parameters as an existing SparseArray
	ILibSparseArray ILibSparseArray_CreateEx(ILibSparseArray source);

	// Populates the "index" in the SparseArray. If that index is already defined, the new value is saved, and the old value is returned. 
	void* ILibSparseArray_Add(ILibSparseArray sarray, int index, void *data);
	
	// Fetches the value of the index. NULL if the index is not defined
	void* ILibSparseArray_Get(ILibSparseArray sarray, int index);

	// Removes an index from the SparseArray (if it exists), and returns the associated value if it does exist.
	void* ILibSparseArray_Remove(ILibSparseArray sarray, int index);

	// Destroys the SparseArray
	void ILibSparseArray_Destroy(ILibSparseArray sarray);

	// Destroys the SparseArray, and calls the OnDestroy delegate for each index that was defined, passing the index and associated value
	void ILibSparseArray_DestroyEx(ILibSparseArray sarray, ILibSparseArray_OnValue onDestroy, void *user);

	// Clears the SparseArray, and calls the OnClear delegate for each index that was defined, passing the index and associated value
	void ILibSparseArray_ClearEx(ILibSparseArray sarray, ILibSparseArray_OnValue onClear, void *user);

	// Moves the Buckets from one SparseArray to another SparseArray
	ILibSparseArray ILibSparseArray_Move(ILibSparseArray sarray);

	void ILibSparseArray_Lock(ILibSparseArray sarray);
	void ILibSparseArray_UnLock(ILibSparseArray sarray);

	typedef void* ILibHashtable;
	typedef int(*ILibHashtable_Hash_Func)(void* Key1, char* Key2, int Key2Len);
	typedef void(*ILibHashtable_OnDestroy)(ILibHashtable sender, void *Key1, char* Key2, int Key2Len, void *Data, void *user);

	ILibHashtable ILibHashtable_Create();

	void ILibHashtable_DestroyEx(ILibHashtable table, ILibHashtable_OnDestroy onDestroy, void *user);
	#define ILibHashtable_Destroy(hashtable) ILibHashtable_DestroyEx(hashtable, NULL, NULL)
	void ILibHashtable_ChangeHashFunc(ILibHashtable table, ILibHashtable_Hash_Func hashFunc);
	void ILibHashtable_ChangeBucketizer(ILibHashtable table, int bucketCount, ILibSparseArray_Bucketizer bucketizer);
	void* ILibHashtable_Put(ILibHashtable table, void *Key1, char* Key2, int Key2Len, void* Data);
	void* ILibHashtable_Get(ILibHashtable table, void *Key1, char* Key2, int Key2Len);
	void* ILibHashtable_Remove(ILibHashtable table, void *Key1, char* Key2, int Key2Len);
	void ILibHashtable_Lock(ILibHashtable table);
	void ILibHashtable_UnLock(ILibHashtable table);


	ILibHashtable ILibChain_GetBaseHashtable(void* chain);

	/*! \defgroup LinkedListGroup Linked List
	\ingroup DataStructures
	\{
	*/

	typedef void* ILibLinkedList;
	//
	// Initializes a new Linked List data structre
	//
	void* ILibLinkedList_Create();
	void ILibLinkedList_SetTag(ILibLinkedList list, void *tag);
	void* ILibLinkedList_GetTag(ILibLinkedList list);

	// Comparer delegate is called to compare two values. Mimics behavior of .NET IComparer..
	// obj2 == obj1	: 0
	// obj2 < obj1	: -1
	// obj2 > obj1	: +1
	typedef int(*ILibLinkedList_Comparer)(void* obj1, void *obj2);

	// Chooser delegate is called when a new item is to be added to Linked List. The old value is passed in as well as the new value... The return value is set to the added node
	typedef void*(*ILibLinkedList_Chooser)(void* oldObject, void *newObject, void *user);

	// Uses the given comparer to insert data into the list. A Duplicate (according to comparer) will result in the value being updated, and the old value being returned
	void* ILibLinkedList_SortedInsert(void* LinkedList, ILibLinkedList_Comparer comparer, void *data);
	void ILibLinkedList_SortedInsertEx(void* LinkedList, ILibLinkedList_Comparer comparer, ILibLinkedList_Chooser chooser, void *data, void *user);

	// Uses the given comparer to return the Linked List node that is reported as a match to matchWith. Returns NULL if not found.
	void* ILibLinkedList_GetNode_Search(void* LinkedList, ILibLinkedList_Comparer comparer, void *matchWith);

	//
	// Returns the Head node of a linked list data structure
	//
	void* ILibLinkedList_GetNode_Head(void *LinkedList);		// Returns Node

	//
	// Returns the Tail node of a linked list data structure
	//
	void* ILibLinkedList_GetNode_Tail(void *LinkedList);		// Returns Node

	//
	// Returns the Next node of a linked list data structure
	//
	void* ILibLinkedList_GetNextNode(void *LinkedList_Node);	// Returns Node

	//
	// Returns the Previous node of a linked list data structure
	//
	void* ILibLinkedList_GetPreviousNode(void *LinkedList_Node);// Returns Node

	//
	// Returns the number of nodes contained in a linked list data structure
	//
	long ILibLinkedList_GetCount(void *LinkedList);

	//
	// Returns a shallow copy of a linked list data structure. That is, the structure
	// is copied, but none of the data contents are copied. The pointer values are just copied.
	//
	void* ILibLinkedList_ShallowCopy(void *LinkedList);

	//
	// Returns the data pointer of a linked list element
	//
	void *ILibLinkedList_GetDataFromNode(void *LinkedList_Node);

	//
	// Creates a new element, and inserts it before the given node
	//
	void ILibLinkedList_InsertBefore(void *LinkedList_Node, void *data);

	//
	// Creates a new element, and inserts it after the given node
	//
	void ILibLinkedList_InsertAfter(void *LinkedList_Node, void *data);

	//
	// Removes the given node from a linked list data structure
	//
	void* ILibLinkedList_Remove(void *LinkedList_Node);

	//
	// Given a data pointer, will traverse the linked list data structure, deleting
	// elements that point to this data pointer.
	//
	int ILibLinkedList_Remove_ByData(void *LinkedList, void *data);

	//
	// Creates a new element, and inserts it at the top of the linked list.
	//
	void ILibLinkedList_AddHead(void *LinkedList, void *data);

	//
	// Creates a new element, and appends it to the end of the linked list
	//
	void ILibLinkedList_AddTail(void *LinkedList, void *data);

	void ILibLinkedList_Lock(void *LinkedList);
	void ILibLinkedList_UnLock(void *LinkedList);
	void ILibLinkedList_Destroy(void *LinkedList);
	/*! \} */



	/*! \defgroup HashTreeGroup Hash Table
	\ingroup DataStructures
	\b Note: Duplicate key entries will be overwritten.
	\{
	*/

	//
	// Initialises a new Hash Table (tree) data structure
	//
	void* ILibInitHashTree();
	void* ILibInitHashTree_CaseInSensitive();
	void ILibDestroyHashTree(void *tree);

	//
	// Returns non-zero if the key entry is found in the table
	//
	int ILibHasEntry(void *hashtree, char* key, int keylength);

	//
	// Add a new entry into the hashtable. If the key is already used, it will be overwriten.
	//
	void ILibAddEntry(void* hashtree, char* key, int keylength, void *value);
	void ILibAddEntryEx(void* hashtree, char* key, int keylength, void *value, int valueEx);
	void* ILibGetEntry(void *hashtree, char* key, int keylength);
	void ILibGetEntryEx(void *hashtree, char* key, int keylength, void **value, int *valueEx);
	void ILibDeleteEntry(void *hashtree, char* key, int keylength);

	//
	// Returns an Enumerator to browse all the entries of the Hashtable
	//
	void *ILibHashTree_GetEnumerator(void *tree);
	void ILibHashTree_DestroyEnumerator(void *tree_enumerator);

	//
	// Advance the Enumerator to the next element.
	// Returns non-zero if there are no more entries to enumerate
	//
	int ILibHashTree_MoveNext(void *tree_enumerator);

	//
	// Obtains the value of a Hashtable Entry.
	//
	void ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data);
	void ILibHashTree_GetValueEx(void *tree_enumerator, char **key, int *keyLength, void **data, int *dataEx);
	void ILibHashTree_Lock(void *hashtree);
	void ILibHashTree_UnLock(void *hashtree);

	/*! \} */

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

	//
	// Trims preceding and proceding whitespaces from a string
	//
	int ILibTrimString(char **theString, int length);

	int ILibString_IndexOfFirstWhiteSpace(const char *inString, int inStringLength);
	char* ILibString_Cat(const char *inString1, int inString1Len, const char *inString2, int inString2Len);
	char *ILibString_Copy(const char *inString, int length);
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
	//
	// Parses the given string using the specified multichar delimiter.
	// Returns a parser_result object, which points to a linked list
	// of parser_result_field objects.
	//
	struct parser_result* ILibParseString (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength);

	//
	// Same as ILibParseString, except this method ignore all delimiters that are contains within
	// quotation marks
	//
	struct parser_result* ILibParseStringAdv (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength);

	//
	// Releases resources used by string parser
	//
	void ILibDestructParserResults(struct parser_result *result);

	//
	// Parses a URI into Address, Port Number, and Path components
	// Note: IP and Path must be freed.
	//
	void ILibParseUri(const char* URI, char** Addr, unsigned short* Port, char** Path, struct sockaddr_in6* AddrStruct);

	//
	// Parses a string into a Long or unsigned Long. 
	// Returns non-zero on error condition
	//
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

	//
	// Allocates an empty HTTP Packet
	//
	struct packetheader *ILibCreateEmptyPacket();

	//
	// Add a header into the packet. (String is copied)
	//
	void ILibAddHeaderLine(struct packetheader *packet, const char* FieldName, int FieldNameLength, const char* FieldData, int FieldDataLength);
	void ILibDeleteHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);

	char* ILibUrl_GetHost(char *url, int urlLen);

	//
	// Fetches the header value from the packet. (String is NOT copied)
	// Returns NULL if the header field does not exist
	//
	char* ILibGetHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);

	//
	// Sets the HTTP version: 1.0, 1.1, etc. (string is copied)
	//
	void ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength);

	//
	// Set the status code and data line. The status data is copied.
	//
	void ILibSetStatusCode(struct packetheader *packet, int StatusCode, char* StatusData, int StatusDataLength);

	//
	// Sets the method and path. (strings are copied)
	//
	void ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength);

	//
	// Releases all resources consumed by this packet structure
	//
	void ILibDestructPacket(struct packetheader *packet);

	//
	// Parses a string into an packet structure.
	// None of the strings are copied, so the lifetime of all the values are bound
	// to the lifetime of the underlying string that is parsed.
	//
	struct packetheader* ILibParsePacketHeader(char* buffer, int offset, int length);

	//
	// Returns the packetized string and it's length. (must be freed)
	//
	int ILibGetRawPacket(struct packetheader *packet,char **buffer);

	//
	// Performs a deep copy of a packet structure
	//
	struct packetheader* ILibClonePacket(struct packetheader *packet);

	//
	// Escapes a string according to HTTP escaping rules.
	// indata must be pre-allocated
	//
	int ILibHTTPEscape(char* outdata, const char* indata);

	//
	// Returns the size of string required to escape this string,
	// according to HTTP escaping rules
	//
	int ILibHTTPEscapeLength(const char* data);

	//
	// Unescapes the escaped string sequence
	// Since escaped string sequences are always longer than unescaped
	// string sequences, the resultant string is overwritten onto the supplied string
	//
	int ILibInPlaceHTTPUnEscape(char* data);
	int ILibInPlaceHTTPUnEscapeEx(char* data, int length);
	/* \} */

	/*! \defgroup NetworkHelper Network Helper
	\ingroup ILibParsers
	\{
	*/

	//
	// Obtain an array of IP Addresses available on the local machine.
	//
	int ILibGetLocalIPv6IndexList(int** indexlist);
	int ILibGetLocalIPv6List(struct sockaddr_in6** list);
	int ILibGetLocalIPAddressList(int** pp_int);
	int ILibGetLocalIPv4AddressList(struct sockaddr_in** addresslist, int includeloopback);
	//int ILibGetLocalIPv6AddressList(struct sockaddr_in6** addresslist);

#if defined(WINSOCK2)
	int ILibGetLocalIPAddressNetMask(unsigned int address);
#endif

	SOCKET ILibGetSocket(struct sockaddr *localif, int type, int protocol);

	/* \} */

	void* dbg_malloc(int sz);
	void dbg_free(void* ptr);
	int dbg_GetCount();

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
	void ILib6to4(struct sockaddr* addr);
	
	//
	// Used to log critical problems
	//
#if defined(WIN32)
#define ILIBCRITICALERREXIT(ex) { ILibCriticalLog(NULL, __FILE__, __LINE__, GetLastError(), 0); exit(ex); }
#define ILIBCRITICALEXIT(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, GetLastError());printf("CRITICALEXIT, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT2(ex,u) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT2, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT3(ex,m,u) {ILibCriticalLog(m, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT3, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBMARKPOSITION(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, GetLastError());}
#define ILIBMESSAGE(m) {ILibCriticalLog(m, __FILE__, __LINE__, 0, GetLastError());printf("ILIBMSG: %s (%d).\r\n", m, (int)GetLastError());}
#define ILIBMESSAGE2(m,u) {ILibCriticalLog(m, __FILE__, __LINE__, u, GetLastError());printf("ILIBMSG: %s (%d,%d).\r\n", m, (int)u, (int)GetLastError());}
#else
#define ILIBCRITICALERREXIT(ex) { ILibCriticalLog(NULL, __FILE__, __LINE__, errno, 0); fflush(stdout); exit(ex); }
#define ILIBCRITICALEXIT(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, errno); printf("CRITICALEXIT, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__);fflush(stdout); exit(ex);}
#define ILIBCRITICALEXIT2(ex,u) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT2, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__);fflush(stdout); exit(ex);}
#define ILIBCRITICALEXIT3(ex,m,u) {ILibCriticalLog(m, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT3, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__);fflush(stdout); exit(ex);}
#define ILIBMARKPOSITION(ex) {ILibCriticalLog(NULL, __FILE__, __LINE__, ex, errno);}
#define ILIBMESSAGE(m) {ILibCriticalLog(m, __FILE__, __LINE__, 0, errno);printf("ILIBMSG: %s\r\n", m);fflush(stdout);}
#define ILIBMESSAGE2(m,u) {ILibCriticalLog(m, __FILE__, __LINE__, u, errno);printf("ILIBMSG: %s (%d)\r\n", m, u);fflush(stdout);}
#endif

	void ILibCriticalLog(const char* msg, const char* file, int line, int user1, int user2);


	void* ILibSpawnNormalThread(voidfp method, void* arg);
	void  ILibEndThisThread();

	char* ILibToHex(char* data, int len, char* out);
	int ILibWhichPowerOfTwo(int number);
#define ILibPowerOfTwo(exponent) (0x01 << exponent)

#ifdef __cplusplus
}
#endif

/* \} */   // End of ILibParser Group
#endif

