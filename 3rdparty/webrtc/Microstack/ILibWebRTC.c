/*
Copyright 2014 - 2015 Intel Corporation

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

// This is a version of the WebRTC stack with Initiator, TURN and proper retry logic.

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
#include "ILibAsyncSocket.h"
#include "ILibAsyncUDPSocket.h"
#include "ILibWebRTC.h"
#include "../core/utils.h"


#include "ILibRemoteLogging.h"
#ifdef _REMOTELOGGINGSERVER
	#include "ILibWebServer.h"
	#define ILibWebRTC_LoggingServerPort 0
#endif

#if defined( SSL_CTX_set_ecdh_auto )
// use it
#else
#define SSL_CTX_set_ecdh_auto( ctx, IGNORED ) \
SSL_CTX_set_tmp_ecdh(ctx, EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
#endif

#define ILibStunClient_TIMEOUT 2
#define ILibRUDP_WindowSize 32000
#define ILibRUDP_StartBufferSize 2048
#define ILibRUDP_StartMTU 1400
#define ILibRUDP_MaxMTU 2048

#define RTO_MIN 1000
#define RTO_MAX 6000
#define RTO_ALPHA 0.125
#define RTO_BETA 0.25

#define ILibSCTP_MaxReceiverCredits 100000
#define ILibSCTP_MaxSenderCredits 0				// When we do real-time traffic, reduce the buffering. In theory, this should never be used, leave to zero
#define ILibSCTP_Stream_SparseArraySize 16		// Must be a power of 2
#define ILibSCTP_Stream_MaximumCount 1024		// This is what Chrome/Firefox Support
#define ILibSTUN_MaxSlots 10					// MUST be less then 128, otherwise IceSlot and dtlsSlot will collide
#define ILibSTUN_MaxOfferAgeSeconds 60			// Offers are only valid for this amount of time
#define ILibSCTP_FastRetry_GAP 3


//
// NAT Keep Alive Interval. We'll use a random value between these two values
//
#define ILibSTUN_MaxNATKeepAliveIntervalSeconds 15
#define ILibSTUN_MinNATKeepAliveIntervalSeconds 1

//
// WebRTC Spec requires that we continue to send STUN packets on the DTLS candidate pair, even after DTLS is established, for a variety of reasons, according to the following
// link posted by Google: http://tools.ietf.org/html/draft-muthu-behave-consent-freshness-04
//
#define ILibStun_MaxConsentFreshnessTimeoutSeconds 15		// Must be 15 Seconds or less to be spec compliant

#define RCTPDEBUG(x)
#define RCTPRCVDEBUG(x)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define FOURBYTEBOUNDARY(a) ((a) + ((4 - ((a) % 4)) % 4))

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))
#define INET_SOCKADDR_PORT(x) (x->sa_family==AF_INET6?(unsigned short)(((struct sockaddr_in6*)x)->sin6_port):(unsigned short)(((struct sockaddr_in*)x)->sin_port))
#define UPDC32(octet, crc) (ILibStun_CRC32_table[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

#define IS_REQUEST(msg_type)       (((msg_type) & 0x0110) == 0x0000)
#define IS_INDICATION(msg_type)    (((msg_type) & 0x0110) == 0x0010)
#define IS_SUCCESS_RESP(msg_type)  (((msg_type) & 0x0110) == 0x0100)
#define IS_ERR_RESP(msg_type)      (((msg_type) & 0x0110) == 0x0110)

#define NAT_MAPPING_DETECTION(TransactionID) (TransactionID[11])
#define DTLS_PAUSE_FLAG 0x01
#define DTLS_RESUME_FLAG 0x02

#define ReceiveHoldBuffer_Increment(list, value) {union{int i;void*p;}u; u.p = ILibLinkedList_GetTag(list); u.i += value; ILibLinkedList_SetTag(list, u.p);}
#define ReceiveHoldBuffer_Decrement(list, value) {union{int i;void*p;}u; u.p = ILibLinkedList_GetTag(list); u.i -= value; ILibLinkedList_SetTag(list, u.p);}
int ReceiveHoldBuffer_Used(ILibLinkedList list)
{
	union{ int i; void *p; }u;
	u.p = ILibLinkedList_GetTag(list);
	return(u.i);
}

static unsigned int ILibStun_CRC32_table[] = { /* CRC polynomial 0xedb88320 */
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

typedef enum STUN_STATUS
{
	STUN_STATUS_CHECKING_UDP_CONNECTIVITY = 0,
	STUN_STATUS_CHECKING_FULL_CONE_NAT = 1,
	STUN_STATUS_CHECKING_SYMETRIC_NAT = 2,
	STUN_STATUS_CHECKING_RESTRICTED_NAT = 3,
	STUN_STATUS_COMPLETE = 5,
} STUN_STATUS;

typedef enum STUN_TYPE
{
	STUN_BINDING_REQUEST = 0x0001,
	STUN_BINDING_RESPONSE = 0x0101,
	STUN_BINDING_ERROR_RESPONSE = 0x0111,
	STUN_SHAREDSECRET_REQUEST = 0x0002,
	STUN_SHAREDSECRET_RESPONSE = 0x0102,
	STUN_SHAREDSECRET_ERROR_RESPONSE = 0x0112,

	TURN_ALLOCATE = 0x003,
	TURN_ALLOCATE_RESPONSE = 0x103,
	TURN_ALLOCATE_ERROR = 0x113,
	TURN_REFRESH = 0x004,
	TURN_REFRESH_RESPONSE = 0x104,
	TURN_REFRESH_ERROR = 0x114,
	TURN_SEND = 0x016,
	TURN_DATA = 0x017,
	TURN_CREATE_PERMISSION = 0x008,
	TURN_CREATE_PERMISSION_RESPONSE = 0x108,
	TURN_CREATE_PERMISSION_ERROR = 0x118,
	TURN_CHANNEL_BIND = 0x009,
	TURN_CHANNEL_BIND_RESPONSE = 0x109,
	TURN_CHANNEL_BIND_ERROR = 0x119,
	TURN_TCP_CONNECT = 0x00A,
	TURN_TCP_CONNECT_RESPONSE = 0x10A,
	TURN_TCP_CONNECT_ERROR = 0x11A,
	TURN_TCP_CONNECTION_BIND = 0x00B,
	TURN_TCP_CONNECTION_BIND_RESPONSE = 0x10B,
	TURN_TCP_CONNECTION_BIND_ERROR = 0x11B,
	TURN_TCP_CONNECTION_ATTEMPT = 0x01C
} STUN_TYPE;

typedef enum STUN_ATTRIBUTES
{
	STUN_ATTRIB_MAPPED_ADDRESS = 0x0001,
	STUN_ATTRIB_RESPONSE_ADDRESS = 0x0002,
	STUN_ATTRIB_CHANGE_REQUEST = 0x0003,
	STUN_ATTRIB_SOURCE_ADDRESS = 0x0004,
	STUN_ATTRIB_CHANGED_ADDRESS = 0x0005,
	STUN_ATTRIB_USERNAME = 0x0006,
	STUN_ATTRIB_PASSWORD = 0x0007,
	STUN_ATTRIB_MESSAGE_INTEGRITY = 0x0008,
	STUN_ATTRIB_ERROR_CODE = 0x0009,
	STUN_ATTRIB_UNKNOWN = 0x000A,
	STUN_ATTRIB_REFLECTED_FROM = 0x000B,
	STUN_ATTRIB_XOR_PEER_ADDRESS = 0x0012,
	STUN_ATTRIB_DATA = 0x0013,
	STUN_ATTRIB_REALM = 0x0014,
	STUN_ATTRIB_NONCE = 0x0015,
	STUN_ATTRIB_XOR_RELAY_ADDRESS = 0x0016,
	STUN_ATTRIB_REQUESTED_ADDRESS_FAMILLY = 0x0017,
	STUN_ATTRIB_EVENPORT = 0x0018,
	STUN_ATTRIB_REQUESTED_TRANSPORT = 0x0019,
	STUN_ATTRIB_XOR_MAPPED_ADDRESS = 0x0020,
	STUN_ATTRIB_RESERVATION_TOKEN = 0x0022,
	STUN_ATTRIB_PRIORITY = 0x0024,	// 36
	STUN_ATTRIB_USE_CANDIDATE = 0x0025,	// 37
	STUN_ATTRIB_PADDING = 0x0026,	// 38
	STUN_ATTRIB_RESPONSE_PORT = 0x0027,
	STUN_ATTRIB_SOFTWARE = 0x8022,
	STUN_ATTRIB_ALTERNATE_SERVER = 0x8023,
	STUN_ATTRIB_CACHE_TIMEOUT = 0x8027,
	STUN_ATTRIB_FINGERPRINT = 0x8028,
	STUN_ATTRIB_ICE_CONTROLLED = 0x8029,
	STUN_ATTRIB_ICE_CONTROLLING = 0x802a,
	STUN_ATTRIB_RESPONSE_ORIGIN = 0x802b,
	STUN_ATTRIB_OTHER_ADDRESS = 0x802c,
	STUN_ATTRIB_ECN_CHECK_STUN = 0x802d,

	TURN_CHANNEL_NUMBER = 0x000C,
	TURN_LIFETIME = 0x000D,
	TURN_RESERVED_WAS_BANDWIDTH = 0x0010,
	TURN_DONT_FRAGMENT = 0x001A,
	TURN_RESERVED_WAS_TIMER_VAL = 0x0021,
	TURN_TCP_CONNECTION_ID = 0x002A
} STUN_ATTRIBUTES;

typedef enum RCTP_CHUNK_TYPES
{
	RCTP_CHUNK_TYPE_DATA = 0x0000,
	RCTP_CHUNK_TYPE_INIT = 0x0001,
	RCTP_CHUNK_TYPE_INITACK = 0x0002,
	RCTP_CHUNK_TYPE_SACK = 0x0003,
	RCTP_CHUNK_TYPE_HEARTBEAT = 0x0004,
	RCTP_CHUNK_TYPE_HEARTBEATACK = 0x0005,
	RCTP_CHUNK_TYPE_ABORT = 0x0006,
	RCTP_CHUNK_TYPE_SHUTDOWN = 0x0007,
	RCTP_CHUNK_TYPE_SHUTDOWNACK = 0x0008,
	RCTP_CHUNK_TYPE_ERROR = 0x0009,
	RCTP_CHUNK_TYPE_COOKIEECHO = 0x000A,
	RCTP_CHUNK_TYPE_COOKIEACK = 0x000B,
	RCTP_CHUNK_TYPE_ECNE = 0x000C,
	RCTP_CHUNK_TYPE_RECONFIG = 0x0082,
	RCTP_CHUNK_TYPE_ASCONF = 0x00C1,
	RCTP_CHUNK_TYPE_ASCONFACK = 0x0080,
} RCTP_CHUNK_TYPES;

typedef enum SCTP_ERROR_CAUSE_CODES
{
	SCTP_ERROR_CAUSE_CODE_INVALID_STREAM					= 0x01,
	SCTP_ERROR_CAUSE_CODE_MISSING_MANDATORY_PARAMATER		= 0x02,
	SCTP_ERROR_CAUSE_CODE_STALE_COOKIE						= 0x03,
	SCTP_ERROR_CAUSE_CODE_OUT_OF_RESOURCES					= 0x04,
	SCTP_ERROR_CAUSE_CODE_UNRESOLVABLE_ADDRESS				= 0x05,
	SCTP_ERROR_CAUSE_CODE_UNRECOGNIZED_CHUNK_TYPE			= 0x06,
	SCTP_ERROR_CAUSE_CODE_INVALID_MANDATORY_PARAMETER		= 0x07,
	SCTP_ERROR_CAUSE_CODE_UNRECOGNIZED_PARAMETERS			= 0x08,
	SCTP_ERROR_CAUSE_CODE_NO_USER_DATA						= 0x09,
	SCTP_ERROR_CAUSE_CODE_COOKIE_RECEIVED_DURING_SHUTDOWN	= 0x0A,
	SCTP_ERROR_CAUSE_CODE_ASSOCIATION_RESTART_NEW_ADDRESS	= 0x0B,
	SCTP_ERROR_CAUSE_CODE_USER_ABORT						= 0x0C,
	SCTP_ERROR_CAUSE_CODE_PROTOCOL_VIOLATION				= 0x0D,
}SCTP_ERROR_CAUSE_CODES;

typedef enum SCTP_RECONFIG_TYPES
{
	SCTP_RECONFIG_TYPE_OUTGOING_SSN_RESET_REQUEST = 0x0D,
	SCTP_RECONFIG_TYPE_INCOMING_SSN_RESET_REQUEST = 0x0E,
	SCTP_RECONFIG_TYPE_SSN_TSN_RESET_REQUEST = 0x0F,
	SCTP_RECONFIG_TYPE_RECONFIGURATION_RESPONSE = 0x10,
	SCTP_RECONFIG_TYPE_ADD_OUTGOING_STREAMS_REQUEST = 0x11,
	SCTP_RECONFIG_TYPE_ADD_INCOMING_STREAMS_REQUEST = 0x12,
}SCTP_RECONFIG_TYPES;

typedef enum SCTP_INIT_PARAMS
{
	SCTP_INIT_PARAM_UNRELIABLE_STREAM = 0xC000,
	SCTP_INIT_PARAM_PARTIAL_CHECKSUM = 0xC004,
	SCTP_INIT_PARAM_RANDOM = 0x8002,
	SCTP_INIT_PARAM_CHUNK_LIST = 0x8003,
	SCTP_INIT_PARAM_REQUESTED_HMAC_ALGORITHM = 0x8004,
	SCTP_INIT_PARAM_SUPPORTED_EXTENSIONS = 0x8008,
}SCTP_INIT_PARAMS;

struct STUN_MAPPED_ADDRESS
{
	unsigned char unused;
	unsigned char family;
	unsigned short port;
	unsigned int address;
};

struct TLV
{
	unsigned short type;
	unsigned short length;
	char* data;
};

struct STUNHEADER
{
	unsigned short type;
	unsigned short length;
	char transactionID[16];
	void *Attributes;
};

#pragma pack(push,1)
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4200 )
#endif
struct ILibStun_IceStateCandidate
{
	unsigned int addr;
	unsigned short port;
};
typedef struct ILibSCTP_InitAckChunk
{
	unsigned int InitiateTag;
	unsigned int A_RWND;
	unsigned short NumberOfOutboundStreams;
	unsigned short NumberOfInboundStreams;
	unsigned int InitialTSN;
}ILibSCTP_InitAckChunk;
typedef struct ILibSCTP_ChunkHeader
{
	unsigned char chunkType;
	unsigned char chunkFlags;
	unsigned short chunkLength;
	char chunkData[];
}ILibSCTP_ChunkHeader;
typedef struct ILibSCTP_ErrorCause_Header
{
	unsigned short CauseCode;
	unsigned short CauseLength;
	char CauseInformation[];
}ILibSCTP_ErrorCause_Header;
typedef struct ILibSCTP_ReconfigChunk
{
	unsigned char type;
	unsigned char chunkFlags;
	unsigned short chunkLength;
	char* reconfigurationParameter;
}ILibSCTP_ReconfigChunk;
typedef struct ILibSCTP_Reconfig_Parameter_Header
{
	unsigned short parameterType;
	unsigned short parameterLength;
}ILibSCTP_Reconfig_Parameter_Header;
typedef struct ILibSCTP_Reconfig_OutgoingSSNResetRequest
{
	unsigned short parameterType;
	unsigned short parameterLength;
	unsigned int RReqSeqNum;
	unsigned int RResSeqNum;
	unsigned int LastTSN;
	unsigned short Streams[];
}ILibSCTP_Reconfig_OutgoingSSNResetRequest;
typedef struct ILibSCTP_Reconfig_IncomingSSNResetRequest
{
	unsigned short parameterType;
	unsigned short parameterLength;
	unsigned int RReqSeqNum;
	unsigned short Streams[];
}ILibSCTP_Reconfig_IncomingSSNResetRequest;
typedef struct ILibSCTP_StreamAttributesStruct
{
	unsigned short StatusFlags;
	unsigned short ReliabilityFlags;
}ILibSCTP_StreamAttributesStruct;
typedef struct ILibSCTP_StreamAttributesStruct_Data
{
	unsigned short NextSequenceNumber;
	unsigned short ReliabilityValue;
}ILibSCTP_StreamAttributesStruct_Data;
typedef struct ILibSCTP_Reconfig_Response
{
	unsigned short parameterType;
	unsigned short parameterLength;
	unsigned int RESSEQNum;
	unsigned int Result;
}ILibSCTP_Reconfig_Response;
typedef struct ILibSCTP_HoldQueuePacket
{
	char** NextPacket;
	unsigned short PacketLength;
	char Packet[];
}ILibSCTP_HoldQueuePacket;
typedef struct ILibSCTP_DataPayload
{
	unsigned char type;
	unsigned char flags;
	unsigned short length;
	unsigned int TSN;
	unsigned short StreamID;
	unsigned short StreamSequenceNumber;
	unsigned int ProtocolID;
	char UserData[];
}ILibSCTP_DataPayload;
typedef union ILibSCTP_PendingTSN_Data
{
	struct 
	{
		unsigned char Type;
		unsigned char Flags;
		unsigned short StreamIdOffset;
	}Data;
	void* Raw;
}ILibSCTP_PendingTSN_Data;
#ifdef WIN32
#pragma warning( pop )
#endif
#pragma pack(pop)

typedef struct ILibSCTP_Accumulator
{
	char *buffer;
	int bufferPtr;
	int bufferLen;
}ILibSCTP_Accumulator;

ILibSCTP_Accumulator* ILibSCTP_CreateAccumulator()
{
	ILibSCTP_Accumulator* retVal = (ILibSCTP_Accumulator*)malloc(sizeof(ILibSCTP_Accumulator));
	if(retVal == NULL) {ILIBCRITICALEXIT(254);}
	memset(retVal, 0, sizeof(ILibSCTP_Accumulator));
	return(retVal);
}

typedef union ILibSCTP_StreamAttributes
{
	ILibSCTP_StreamAttributesStruct Data;
	void* Raw;
}ILibSCTP_StreamAttributes;
typedef union ILibSCTP_StreamAttributes_Data
{
	ILibSCTP_StreamAttributesStruct_Data Data;
	unsigned int TSN;
	void* Raw;
}ILibSCTP_StreamAttributes_Data;

typedef enum ILibSCTP_Reconfig_Result
{
	ILibSCTP_Reconfig_Result_Success_NOP = 0,
	ILibSCTP_Reconfig_Result_Success_Performed = 1,
	ILibSCTP_Reconfig_Result_Denied = 2,
	ILibSCTP_Reconfig_Result_Error_Wrong_SSN = 3,
	ILibSCTP_Reconfig_Result_Error_Already_In_Progress = 4,
	ILibSCTP_Reconfig_Result_Error_Bad_Sequence_Number = 5,
	ILibSCTP_Reconfig_Result_In_Progress = 6,
}ILibSCTP_Reconfig_Result;

typedef enum ILibSCTP_SackStatus
{
	ILibSCTP_SackStatus_NotSent = 0,
	ILibSCTP_SackStatus_Sent = 1,
	ILibSCTP_SackStatus_Skip = 2
}ILibSCTP_SackStatus;

#define	ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED 0x8000
#define	ILibSCTP_StreamAttributesData_Assigned_Status_WAITING_FOR_ACK 0x4000
#define ILibSCTP_StreamAttributesData_Assigned_Status_WAITING_FOR_TSN_BEFORE_CLOSE 0x2000
#define ILibSCTP_StreamAttributesData_Assigned_Status_PENDING_CLOSE 0x1000
#define ILibSCTP_StreamAttributesData_Assigned_Status_FRAGMENTED 0x100
#define	ILibSCTP_StreamAttributesData_Assigned_Status_UNORDERED 0x0080
#define	ILibSCTP_StreamAttributesData_Assigned_Status_TIMED 0x0002
#define	ILibSCTP_StreamAttributesData_Assigned_Status_REXMIT 0x0001

#if defined(_REMOTELOGGINGSERVER)
unsigned char ILibWebRTC_LoggingServer_HTML[3368] = 
{
   0x1F,0x8B,0x08,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0xB5,0x5A,0xFD,0x72,0xDB,0xB8,0x11,0x7F,0x15,0x84,0x99,0xC4,0x64,0x24,0x51,0xA2,0xBE,0x6C,0x7D,0x50,0x19,0x5B,
   0xD2,0x25,0x9E,0x3A,0x4E,0x1A,0xDB,0x49,0x6E,0x32,0x69,0x06,0x24,0x21,0x89,0x0D,0x45,0xAA,0x24,0x64,0xD9,0xA7,0xFA,0xC9,0xFA,0x47,0x1F,0xA9,0xAF,0xD0,0x5D,0x80,
   0xA4,0x40,0x4A,0xF6,0x5D,0xA6,0xD7,0x51,0xC6,0x22,0x81,0xC5,0x7E,0x61,0xF7,0xB7,0x0B,0x28,0xFF,0xF9,0xD7,0xBF,0x87,0xCF,0x26,0xEF,0xC7,0xD7,0xBF,0x7E,0x98,0x92,
   0x05,0x5F,0x06,0xE4,0xC3,0xCD,0xD9,0xC5,0xF9,0x98,0x68,0xB5,0x7A,0xFD,0x73,0x6B,0x5C,0xAF,0x4F,0xAE,0x27,0xE4,0xCB,0xDB,0xEB,0x77,0x17,0xC4,0x32,0x1B,0xE4,0x3A,
   0xA6,0x61,0xE2,0x73,0x3F,0x0A,0x69,0x50,0xAF,0x4F,0x2F,0x35,0xA2,0x2D,0x38,0x5F,0xF5,0xEB,0xF5,0xCD,0x66,0x63,0x6E,0x5A,0x66,0x14,0xCF,0xEB,0xD7,0x1F,0xEB,0x77,
   0xC8,0xCB,0xC2,0xC5,0xE9,0x63,0x8D,0x2B,0x2B,0x4D,0x8F,0x7B,0xDA,0x88,0x0C,0x71,0x06,0xBF,0x18,0xF5,0xE0,0x6B,0xC9,0x38,0x25,0x6E,0x14,0x72,0x16,0x72,0x5B,0xE3,
   0xEC,0x8E,0xD7,0x91,0x60,0x40,0xDC,0x05,0x8D,0x13,0xC6,0xED,0x35,0x9F,0xD5,0x4E,0x34,0x82,0x02,0x6B,0xEC,0x1F,0x6B,0xFF,0xD6,0xD6,0xC6,0x92,0xBC,0x76,0x7D,0xBF,
   0x62,0x5A,0xC6,0x43,0x25,0xF8,0x52,0xBB,0x39,0xAD,0x8D,0xA3,0xE5,0x8A,0x72,0xDF,0x09,0x98,0xB6,0x13,0x70,0x3E,0xB5,0xA7,0x93,0x37,0xD3,0x7C,0x55,0x48,0x97,0xCC,
   0xD6,0x66,0x51,0xBC,0xA4,0xBC,0xE6,0x31,0xCE,0x5C,0x54,0x56,0x53,0x35,0x0A,0xD8,0x6A,0x11,0x85,0xCC,0x0E,0x23,0x5C,0xC5,0x7D,0x1E,0xB0,0xD1,0x67,0xE6,0x7C,0xBC,
   0x1E,0x93,0x09,0x73,0xD6,0xF3,0x61,0x5D,0x8E,0x91,0x61,0xC2,0xEF,0xF1,0xDB,0x89,0xBC,0xFB,0xED,0x92,0xC6,0x73,0x3F,0xEC,0x37,0x06,0x2B,0xEA,0x79,0x7E,0x38,0x87,
   0x27,0x27,0x8A,0x3D,0x16,0xC3,0x83,0x1B,0x05,0x51,0xDC,0x77,0x02,0xEA,0xFE,0x18,0xCC,0x40,0x50,0x2D,0xF1,0x7F,0x63,0x7D,0xAB,0xB5,0xBA,0x93,0xAF,0x33,0xBA,0xF4,
   0x83,0xFB,0xBE,0x76,0x1D,0x03,0x7F,0x77,0xC1,0x38,0x79,0x77,0xA5,0x55,0xC9,0x69,0xEC,0xD3,0xA0,0x4A,0xDE,0xB2,0xE0,0x96,0x71,0xDF,0xA5,0x55,0x92,0x80,0x77,0x6B,
   0x09,0x8B,0xFD,0xD9,0xC0,0x01,0x66,0xF3,0x38,0x5A,0x87,0x5E,0x4D,0xB2,0x7F,0xEE,0xB5,0xBC,0x9E,0xD7,0x1D,0x3C,0x3C,0x47,0x5B,0xA8,0x1F,0xB2,0x78,0xBB,0x4F,0x34,
   0x9B,0xCD,0x06,0x1B,0xDF,0xE3,0x8B,0x7E,0xAF,0xDB,0x00,0xF9,0x99,0xDA,0x84,0xAE,0x79,0x94,0x6A,0x5C,0xE3,0xD1,0x2A,0x57,0xBF,0x16,0xFB,0xF3,0x05,0xEF,0x5B,0xAB,
   0x3B,0x92,0x44,0x81,0xEF,0x91,0xE7,0xCE,0x31,0x7E,0xB2,0x69,0x27,0xE2,0x3C,0x5A,0xEE,0xC8,0x03,0x36,0x3B,0x44,0xBD,0xF3,0xCA,0xC3,0xF3,0x25,0x4D,0x38,0x46,0xC3,
   0x56,0x2A,0x22,0x24,0x1F,0x70,0x5F,0x74,0xCB,0xE2,0x59,0x10,0x6D,0x24,0x01,0x46,0x4A,0x8D,0x06,0xFE,0x3C,0xEC,0x0B,0x8D,0x0E,0x38,0xA0,0xD1,0xEA,0x16,0x6C,0xDB,
   0x51,0xF4,0xE3,0xB9,0xA3,0xB7,0x3B,0xD5,0x93,0x6E,0xD5,0x6A,0x1D,0x1B,0x03,0xA2,0x4C,0xD5,0x96,0xD1,0x6F,0xB5,0x00,0xDC,0x45,0xE3,0xDA,0x3C,0xA6,0x9E,0x0F,0x61,
   0xA0,0xA3,0x15,0x55,0x02,0xAB,0xE8,0x6E,0x59,0xD5,0x32,0x48,0xE3,0x45,0x3A,0xDA,0xA8,0x76,0xAC,0xAA,0xD5,0x68,0xE2,0x60,0xB3,0xF7,0xA2,0xC4,0x72,0xC3,0x9C,0x1F,
   0x3E,0x57,0xD8,0x09,0xF6,0x55,0x82,0x6C,0x09,0x78,0x17,0x98,0xA0,0x0D,0xF2,0x51,0x68,0x5F,0x4B,0xE0,0x59,0x07,0xF6,0x7B,0x32,0x8D,0x02,0x05,0xC8,0xAA,0x96,0x15,
   0x30,0x0E,0x4B,0xFF,0x09,0x9B,0xFE,0x80,0x49,0xD1,0x9F,0xCC,0x6F,0x99,0xFC,0xB9,0x0C,0xCB,0xCC,0x78,0x24,0x7D,0xFC,0x73,0x0C,0x67,0x7E,0xC0,0x21,0x65,0x57,0x71,
   0x34,0xF7,0xBD,0xFE,0xE4,0xCB,0xF9,0x92,0xCE,0x99,0x80,0x43,0x84,0x0C,0xF3,0x9D,0xEF,0xC6,0x51,0x12,0xCD,0xB8,0x99,0xCB,0x21,0x09,0xA7,0x31,0x1F,0xE3,0x0E,0x25,
   0x3C,0xB6,0x8F,0x9E,0x37,0xBD,0x4E,0xF7,0xA4,0x77,0x54,0x25,0x2C,0xF4,0x94,0xE1,0x46,0xA3,0xD5,0xEA,0x76,0x8F,0xAA,0x6F,0xD2,0x85,0x88,0x65,0xB6,0x45,0x40,0x26,
   0x26,0x6C,0xB0,0x5E,0x86,0xDF,0x83,0xED,0x2A,0x92,0xE0,0xD9,0x8F,0x59,0x00,0x50,0x76,0xCB,0x06,0x90,0x00,0x94,0xF7,0xD1,0x33,0x59,0x68,0xB7,0xD4,0xB4,0xDD,0xA5,
   0x0B,0xB1,0x3A,0x85,0x90,0x57,0x13,0xFE,0xE1,0xF9,0x2C,0x8A,0xC0,0xAE,0xAD,0x1B,0x80,0x8F,0xFA,0x90,0xB2,0x8B,0x52,0x76,0xA9,0x69,0xA3,0x64,0x9A,0x0B,0x8A,0xB2,
   0xF8,0x00,0x57,0xCB,0x6A,0xF5,0xBA,0xCD,0x4C,0xBA,0x00,0x0C,0x14,0x9F,0xBD,0xA7,0xA0,0x80,0x43,0x99,0x6C,0x42,0xB7,0x8A,0x46,0x42,0x86,0xC7,0xDC,0x28,0xA6,0xC2,
   0x5E,0x60,0xCD,0x62,0xDC,0x42,0x85,0xBE,0xBF,0x40,0x1D,0x9F,0x5A,0x15,0x46,0xB8,0x20,0x67,0x8C,0xF9,0xFF,0x04,0x63,0x53,0xC0,0x75,0x6B,0xBB,0x6F,0x9F,0x5C,0xBF,
   0x59,0xF8,0x9C,0x1D,0xB0,0xF5,0xA4,0x81,0x1F,0x89,0xD4,0x1B,0x26,0xD0,0xD0,0x89,0x02,0x2F,0x63,0xD8,0xDD,0xAA,0x5E,0x68,0xEE,0x7B,0xA1,0x79,0x70,0x5F,0xC6,0x0D,
   0xFC,0x00,0x93,0xBB,0x44,0xD6,0xA0,0x03,0x60,0x3D,0x6D,0xE0,0x27,0xDF,0x63,0x70,0x27,0x69,0x36,0xF0,0x4F,0xF6,0x94,0x63,0x34,0xC4,0xD4,0x3A,0xE9,0x5B,0x38,0xF4,
   0x60,0xCE,0xFC,0x3B,0xE6,0xA1,0xBA,0x5B,0xB5,0xBA,0xB8,0xD1,0x3A,0xF6,0x19,0x80,0x90,0x96,0x3E,0x91,0x90,0x6D,0xA0,0xCA,0x2C,0xA3,0x30,0x4A,0x56,0xD4,0x45,0x0F,
   0xF9,0xE1,0x2C,0x02,0x97,0xC4,0xF7,0x05,0x9B,0x2C,0xC1,0xF5,0xCD,0xF4,0x72,0xFA,0xF1,0x7C,0xFC,0xFD,0xDD,0xFB,0xC9,0xCD,0xC5,0x74,0x4B,0xD4,0xA2,0xF6,0x60,0x5E,
   0x5D,0xDF,0x5C,0x7E,0x3F,0x1F,0x4F,0xF7,0xA6,0xD7,0xC8,0x77,0x72,0x7D,0x71,0x55,0x9A,0xF1,0x68,0x0C,0xD6,0x32,0x16,0xE2,0xE2,0xF1,0xF5,0x87,0x03,0xD3,0xB7,0x7E,
   0x14,0x40,0x2D,0x7C,0x30,0x6F,0x2E,0xFF,0x72,0xF9,0xFE,0xF3,0x65,0x89,0x24,0x66,0xB8,0x07,0x17,0xEF,0xDF,0x7C,0xBF,0x98,0x7E,0x9A,0x5E,0x7C,0xB7,0xB6,0x44,0x7D,
   0x6D,0x16,0x5F,0x5B,0xC5,0xD7,0x76,0xF1,0xB5,0x53,0x7C,0xED,0x66,0xAF,0x9F,0xCE,0xAF,0xCE,0xCF,0x50,0xA4,0xE7,0x27,0xAB,0x80,0xDE,0xF7,0xFD,0x30,0x0D,0x26,0x9C,
   0x7D,0x7B,0x3E,0x99,0x4C,0x2F,0x77,0x93,0x32,0x1E,0x4D,0xD1,0x4B,0xDC,0xF1,0x77,0x2C,0x5C,0x2B,0x9B,0xDA,0x7F,0xFE,0x4B,0x0F,0x3F,0xB0,0x67,0x77,0xB5,0x64,0x41,
   0x3D,0xC8,0xBD,0x06,0x81,0xBC,0x85,0xF8,0x90,0x18,0x45,0x1A,0xD5,0xF4,0x9F,0xD9,0x02,0x60,0x48,0xDB,0x07,0xA5,0x96,0xBA,0xAE,0x3B,0x28,0x0A,0xCB,0x01,0x83,0x3A,
   0x40,0xB3,0x86,0xF8,0x95,0xB5,0x5B,0x54,0x61,0xF8,0xF2,0x13,0x68,0x36,0x30,0x4A,0x25,0xF9,0x1E,0x6C,0x60,0x86,0x2E,0xFD,0xB0,0x26,0xF3,0xBF,0x23,0xA1,0xE5,0x2E,
   0x7D,0xB5,0x1A,0xF8,0xFE,0x5B,0xCD,0x87,0x1C,0xBA,0x83,0x49,0x8C,0x56,0x77,0x89,0xA6,0x65,0x19,0xD7,0x6E,0xB7,0x07,0x45,0xCF,0xD4,0x9C,0x20,0x82,0x80,0xC8,0xC2,
   0x47,0xE8,0x71,0xA2,0xE4,0x84,0xEC,0x26,0xD4,0x91,0x27,0xB0,0xE3,0x60,0xBE,0x4B,0xDD,0x4E,0x3A,0x2F,0x06,0xEE,0x3A,0x4E,0x30,0x54,0xD8,0x8C,0xAE,0x03,0xBE,0x43,
   0xB4,0x85,0xEF,0x79,0x10,0x57,0xFB,0x60,0x9A,0xA9,0x5F,0x42,0x96,0x1E,0x7E,0xD4,0x6E,0x41,0xD8,0xF5,0x30,0xAC,0xA7,0xFD,0xDD,0xB0,0x9E,0xF6,0xAE,0xD8,0xE8,0x91,
   0x28,0x04,0x48,0xF6,0x6C,0xCD,0x9F,0x11,0x9D,0x03,0x88,0x47,0x33,0x5D,0x94,0x80,0xF5,0xCA,0x20,0xCF,0x6C,0x9B,0x1C,0x21,0xE4,0xCC,0xC0,0x15,0xDE,0x91,0x41,0xD2,
   0x19,0xDD,0x18,0x68,0xB0,0x30,0x0D,0x8C,0x25,0x04,0x86,0xAD,0x2D,0x68,0xE8,0x05,0x6C,0xBC,0x8B,0x15,0x9D,0xDD,0x42,0xEE,0x19,0xD8,0x74,0x7A,0xFE,0x2D,0xF1,0x41,
   0x46,0xDE,0xC9,0xA9,0x83,0x59,0xFB,0xA4,0x11,0xA1,0x1F,0x70,0x92,0xA0,0xD4,0xED,0x82,0xCB,0xF2,0x9D,0x7B,0x51,0xF6,0x47,0xC6,0x22,0x5D,0xA4,0x14,0x16,0x75,0x7D,
   0xEA,0x13,0xF7,0x04,0x3F,0xC5,0x6D,0x14,0x80,0xA3,0xEE,0x1A,0xEC,0xA2,0x36,0x82,0x26,0x38,0x8E,0xC2,0xF9,0x68,0x88,0x58,0x93,0x33,0xCF,0x9B,0xDC,0x76,0xB7,0xD4,
   0xE4,0x3E,0xD9,0xD4,0x6A,0xD8,0x68,0xD7,0xB0,0xD3,0xFE,0xC8,0x68,0xC0,0xFD,0x25,0x23,0x17,0xD1,0x1C,0x42,0x16,0x9A,0x6E,0x64,0x32,0xC2,0x3D,0x91,0xE2,0xEA,0x60,
   0xCB,0xFF,0x6A,0x51,0xA7,0x64,0x90,0xD5,0xFE,0x03,0x16,0x21,0xD1,0xCF,0x58,0x34,0x04,0x70,0x0D,0xC5,0xC6,0x89,0x63,0xC3,0x22,0x4A,0xB8,0x86,0x76,0xC0,0xE8,0xE8,
   0x31,0xAB,0x14,0xE3,0xC4,0xC2,0x68,0xE5,0x50,0x11,0x03,0x9C,0xC2,0xF1,0x26,0xD3,0x49,0xD9,0xEB,0xD4,0xE0,0x26,0xD6,0x1A,0x38,0xCC,0xB0,0x20,0x48,0xED,0xB2,0xB5,
   0x86,0x7C,0x47,0x88,0xCF,0xDF,0x03,0x9A,0x24,0xB6,0x26,0xD8,0x58,0x82,0x6D,0x8C,0x7F,0x3C,0x21,0xEC,0x8A,0xAF,0xC3,0x73,0x97,0x9D,0xAD,0x21,0x0D,0x43,0x6D,0x4F,
   0x16,0x08,0xC8,0x84,0xA1,0x23,0xD2,0x1C,0x5C,0x45,0xBE,0xAC,0xA4,0xF2,0x15,0x63,0x5B,0x04,0x7C,0xE0,0xBB,0x3F,0x50,0xFF,0xF9,0x3C,0x60,0xC8,0x58,0x37,0x8A,0xD2,
   0x5B,0xDA,0x08,0x2B,0x47,0x1D,0x2A,0x07,0x1C,0xAB,0xBC,0x9D,0x16,0x13,0x1E,0x24,0x7F,0xBE,0x0A,0xC8,0xF5,0x80,0x0A,0x58,0x9E,0x8A,0xE2,0xAF,0x5C,0xBE,0xFA,0x3F,
   0x78,0x00,0xB8,0x1E,0xF2,0x00,0x94,0xBF,0x54,0x7C,0x5D,0xEC,0x44,0x5D,0x6C,0xF3,0x7E,0x1C,0xAC,0xA0,0x19,0xFD,0x9E,0x9E,0x54,0x8B,0x30,0x21,0xFB,0x47,0x65,0xEC,
   0x28,0x66,0xB3,0x98,0x25,0x0B,0x69,0xC4,0xC4,0xBF,0x3D,0x2A,0xA6,0x89,0x3C,0x43,0xC9,0x7A,0x20,0x1B,0x17,0xB4,0x4B,0xAD,0x2D,0x10,0xA4,0x7E,0xB8,0x5A,0xC3,0x21,
   0x05,0x3B,0x55,0xCD,0x49,0xBD,0x71,0x4B,0xA1,0xA4,0xDB,0xDA,0x47,0xC9,0x5E,0x31,0x31,0x15,0x88,0x50,0xA7,0x66,0xE7,0x68,0xB8,0xB0,0x76,0xD1,0x8F,0x53,0x0B,0x2B,
   0x9F,0x5F,0xA5,0x80,0xE6,0x87,0xAA,0x39,0x00,0x9A,0x7C,0x9D,0xC0,0x5B,0x19,0xE1,0xDA,0x8D,0x7D,0x25,0xE5,0x32,0xD1,0xCE,0x81,0x23,0x44,0x3F,0x97,0x2F,0xCB,0xCA,
   0x5D,0x39,0xCD,0x45,0x9B,0xBB,0xCB,0x68,0x34,0x1D,0x19,0xE5,0x89,0x2A,0x15,0x40,0x5C,0xCE,0x33,0x95,0x94,0x52,0xF3,0x70,0xA2,0x02,0x58,0x7D,0x86,0x6A,0x19,0x6D,
   0xF6,0xA3,0x26,0xCF,0xD1,0xAE,0x30,0x22,0x03,0xE7,0xDA,0x7D,0x3F,0x81,0x33,0x45,0x20,0xF6,0xAE,0xC4,0x4D,0x36,0xC2,0xBB,0xB4,0x7F,0x32,0xAD,0xAD,0xC6,0x01,0x99,
   0xA5,0xDC,0x3E,0xBA,0x93,0x2C,0xF3,0x58,0x50,0x5A,0x61,0x81,0x9B,0xA5,0xFD,0xCF,0x53,0x62,0x9F,0x5C,0xC6,0x8F,0xC4,0xD6,0x00,0x9F,0xE1,0x40,0x74,0xAF,0x8D,0xCE,
   0x43,0x79,0xB5,0x02,0x65,0x97,0x50,0x4E,0x86,0x34,0x5B,0x5A,0xA6,0x24,0x0B,0x88,0x17,0x5B,0x5C,0x2B,0x25,0xFD,0x7A,0x1D,0x1B,0x4F,0x73,0x09,0xE1,0x83,0x3B,0x18,
   0xD3,0x00,0x7A,0xA8,0xA5,0x36,0x3A,0x34,0x3A,0xAC,0xD3,0xD1,0x93,0xC9,0x52,0x72,0xA2,0xD2,0x8C,0xE5,0x89,0x57,0x18,0x4B,0x15,0x3C,0x14,0x55,0x19,0xB9,0x68,0x1A,
   0x94,0x50,0x87,0x6D,0xBE,0x80,0x62,0x1D,0xE8,0x96,0xA8,0xD6,0xCE,0xE8,0x13,0x8B,0x1D,0xEC,0x36,0xEE,0xFB,0xC4,0x1A,0xD6,0x9D,0xA2,0x16,0xBF,0xCB,0xA5,0xB9,0xCF,
   0xA5,0xF9,0xF3,0x5C,0x5A,0xFB,0x5C,0x5A,0x3F,0xCF,0xA5,0xBD,0xCF,0xA5,0xFD,0xF3,0x5C,0x3A,0xFB,0x5C,0x3A,0x05,0x2E,0x8B,0xF8,0x77,0x58,0x8D,0xF1,0xA4,0x0A,0xFC,
   0x10,0xAE,0x05,0x2B,0x31,0x80,0xDD,0x40,0x72,0x98,0x91,0xB2,0xD7,0x1F,0xE8,0x3A,0x61,0x1F,0x59,0xB2,0x5E,0x32,0xED,0x51,0x01,0xD7,0x02,0x91,0x15,0xD2,0x4C,0xD0,
   0x87,0xD3,0x9B,0xAB,0x69,0x41,0x46,0xFA,0x05,0x79,0xEA,0xAF,0x32,0x38,0x14,0x17,0x97,0x7F,0xA7,0xB7,0x54,0x8E,0x6A,0xA3,0x5B,0xD0,0x4E,0x2C,0x9D,0xD8,0x33,0x1A,
   0x24,0x6C,0x80,0x03,0x58,0xDB,0x94,0x57,0xAC,0x33,0xEA,0x2C,0xE0,0xBE,0xF2,0xBA,0x49,0xA0,0x73,0x66,0x5C,0x3C,0x83,0x25,0x21,0x1C,0x0B,0x99,0xA7,0xCC,0x8F,0xDF,
   0x4D,0xEC,0xED,0xE5,0xFB,0xCB,0x69,0xBF,0x51,0xCD,0x8E,0x5B,0xFD,0x66,0x15,0x99,0xF6,0xDB,0x55,0x64,0xD6,0x3F,0x79,0x10,0x94,0xB9,0xDB,0x6D,0x6B,0x30,0x5B,0x87,
   0xE2,0x78,0x49,0xFE,0xAA,0x53,0x63,0x1B,0x33,0xBE,0x8E,0x43,0xE2,0x45,0x2E,0x58,0x1C,0x72,0x73,0xCE,0xF8,0x34,0x60,0xF8,0x78,0x76,0x7F,0xEE,0x01,0xC5,0xC3,0x8E,
   0xFE,0x4A,0x59,0x80,0x8B,0xE5,0x71,0x57,0x21,0x98,0xEA,0xB4,0xEA,0x18,0x5B,0x31,0x07,0xE9,0x83,0x69,0xE8,0xD9,0xCF,0x1C,0x85,0xE2,0x53,0x4A,0x71,0x95,0x92,0x60,
   0x86,0xD9,0xBA,0xF3,0x5A,0xD3,0xFA,0x9A,0x48,0x34,0x55,0xDE,0xA9,0xC2,0xCE,0x07,0xF3,0x63,0xBC,0x8F,0xAE,0xD8,0x2A,0xBF,0xB7,0x87,0x48,0x0A,0x14,0xE3,0xA7,0xAD,
   0x4C,0xCE,0xEE,0xC7,0x18,0x10,0x97,0x14,0x36,0xBC,0x60,0xED,0xF8,0x93,0xEE,0x56,0x3D,0x63,0x8B,0xFE,0xA3,0x36,0xF0,0x71,0x0D,0x28,0x11,0xB1,0x8E,0xEF,0x8E,0xDD,
   0x18,0x38,0x43,0x6A,0x06,0x2C,0x9C,0xF3,0xC5,0xC0,0xA9,0x54,0x8C,0x2D,0xFD,0xEA,0x7C,0x93,0x1E,0xC9,0x0D,0xF3,0x1E,0x76,0xFC,0xCE,0x43,0xDE,0x6A,0x5E,0x47,0x57,
   0x3C,0x56,0x14,0x82,0x37,0xC0,0x6A,0x73,0x16,0x47,0xCB,0xF1,0x82,0xC6,0xE3,0xC8,0x63,0xBA,0x4E,0x47,0xA3,0x66,0xDB,0x78,0xD9,0xEC,0x74,0xAA,0xF8,0x6C,0x75,0x77,
   0xCF,0x27,0xF2,0x91,0xE2,0x5F,0xA3,0xC0,0xDB,0xEA,0xFE,0x51,0xDE,0x8F,0xF0,0x80,0xFE,0xDA,0xBB,0x5A,0x44,0x31,0x97,0x1E,0x95,0x4C,0x74,0x6A,0xBA,0xE9,0xDA,0x53,
   0xAE,0x3B,0xC6,0x70,0x78,0x62,0x54,0x8A,0x63,0x15,0xAB,0xC4,0x04,0x94,0x79,0x8A,0xC5,0x2B,0xAB,0x7B,0x7C,0x7C,0xDC,0x04,0xAB,0x2A,0xFA,0x1E,0xA7,0x57,0xDD,0x4E,
   0xA7,0x75,0x60,0xA6,0x69,0xBC,0x6A,0x76,0xBA,0x7B,0xA2,0x5B,0x25,0xD1,0x17,0xD0,0x32,0x17,0x64,0xEF,0x09,0x3F,0x36,0x9B,0x8D,0xCE,0x71,0xA7,0xD7,0x6E,0xB4,0x8E,
   0x7B,0xCD,0xE3,0x5E,0x9B,0x55,0x1E,0xD1,0xA4,0x79,0x62,0xB5,0x8F,0xDB,0xBD,0xE3,0xEE,0xB1,0xD5,0xE8,0x76,0x0E,0xEB,0x64,0x35,0x7A,0xBD,0x8E,0x65,0x75,0x9B,0x60,
   0xD1,0x01,0x8A,0x96,0xF1,0xAA,0xDD,0xEC,0xB5,0x7B,0xDD,0xE3,0x66,0x6F,0x5F,0xF9,0xF6,0x53,0xAE,0xE8,0x3C,0xEA,0x8A,0xEE,0x61,0x57,0x1C,0x2B,0xAE,0xB8,0x12,0x17,
   0x8D,0xCB,0x25,0x74,0x9C,0xD2,0x1B,0x29,0x90,0x98,0x09,0x4C,0xE8,0x85,0x60,0xA9,0x38,0xCA,0x3A,0x05,0x62,0xB7,0x90,0x59,0x4A,0xF3,0x52,0xD5,0xD4,0xC4,0x7C,0xEB,
   0x7B,0xEC,0x26,0x94,0x1D,0xAC,0x07,0xB4,0xFE,0x4C,0x7F,0x86,0xF8,0x03,0xAB,0x20,0x69,0xB4,0xD2,0xCD,0x0F,0x2C,0x4E,0x33,0x1B,0xE9,0x10,0x9C,0x52,0x3A,0xE5,0x0E,
   0xA8,0x48,0x83,0xC8,0x95,0xF1,0xDA,0x5D,0x04,0xED,0x68,0x76,0x9A,0x1C,0xC0,0x6C,0xA1,0x8D,0xC4,0x5D,0x63,0x7B,0xEA,0x41,0x4C,0xCC,0x75,0xED,0xEB,0xCD,0x39,0x11,
   0x54,0xDE,0x37,0x58,0x9F,0xA2,0x72,0x4A,0x35,0x40,0x53,0x0F,0x14,0x8A,0xAA,0x06,0xD8,0xAF,0x55,0x74,0x49,0xF5,0x5A,0xFB,0x38,0xBD,0xBA,0x79,0x37,0x05,0x9C,0x12,
   0x03,0x9A,0x51,0xD1,0xB0,0x26,0x68,0xC6,0xE0,0x31,0x81,0x92,0x8F,0x90,0xB8,0xD3,0x38,0xAF,0x88,0xB0,0x2D,0x3B,0x54,0x76,0x06,0xD9,0xC2,0x7C,0x8C,0x08,0x32,0xFC,
   0x11,0x2C,0x9C,0x33,0x8F,0xF0,0xA8,0x4F,0x34,0xD8,0xAD,0x81,0x84,0xA3,0x86,0x80,0x22,0x0A,0x70,0x4E,0x87,0x76,0x77,0x50,0xA9,0xD0,0xD4,0x61,0xBB,0x2B,0x2B,0xAD,
   0x42,0xAB,0x30,0x09,0xD8,0x2A,0x2F,0x62,0x76,0x08,0x5B,0xDA,0xBF,0x81,0x1A,0x2F,0x50,0x53,0x4C,0x2C,0x29,0x55,0x25,0x4C,0xAC,0xE1,0x30,0x57,0xCB,0x50,0xE2,0x20,
   0xD5,0x99,0x56,0x67,0x55,0x57,0xE2,0xA4,0x63,0x6B,0x1A,0xFA,0x63,0x66,0xDB,0xF9,0xA5,0x07,0x28,0x76,0x5A,0x8C,0x25,0xA5,0xCE,0x1F,0x15,0x2F,0x11,0x8F,0xC8,0x08,
   0xAC,0xA4,0xC2,0xB5,0x71,0x7A,0x28,0x00,0x0F,0xCB,0x6C,0xC6,0xD8,0xC8,0xFC,0x9C,0x8E,0xA0,0x4C,0xA8,0xBD,0x44,0xB9,0xF3,0xD3,0x84,0x87,0x3C,0x5B,0x77,0x87,0x76,
   0x51,0xF3,0xD7,0x92,0x4E,0xDC,0xE0,0x9D,0x5F,0xE0,0x4E,0x92,0xDD,0x9D,0x1D,0x48,0x49,0x36,0x3E,0x77,0x17,0x00,0xF3,0x5B,0x97,0x26,0x8C,0x34,0xFB,0x7B,0x9C,0x9D,
   0x98,0xD1,0x1F,0x03,0x31,0xDB,0x2E,0xCD,0x36,0x0B,0xB3,0x27,0xA5,0xD9,0x56,0x61,0xD6,0xEA,0x96,0xA6,0xDB,0x85,0xE9,0x56,0x59,0x72,0x27,0x9B,0x4E,0x2F,0xB6,0x1E,
   0x51,0xEC,0x21,0x35,0x80,0xFE,0xAD,0xD5,0x3C,0xEE,0x9E,0xA4,0x66,0xE0,0x86,0xE6,0xCD,0x01,0xEC,0x4F,0x39,0x3B,0x55,0xC9,0x48,0x2B,0xBA,0x07,0xA0,0x53,0xB3,0xB3,
   0x4C,0x23,0x5A,0x0B,0xE4,0xA5,0x64,0x67,0x49,0x45,0x98,0x2D,0x5E,0xD2,0x66,0x4A,0x3E,0x15,0x0C,0x10,0xE0,0x15,0x56,0xF1,0x2A,0x9A,0x8C,0x03,0x3D,0x64,0x1B,0x32,
   0xA1,0x1C,0xB2,0xDA,0xE4,0xD1,0x45,0xE4,0xD2,0x80,0x5D,0xFB,0x4B,0x26,0x2B,0x9B,0x6E,0x40,0x0A,0x62,0x52,0xCC,0x4A,0xE1,0xB2,0x0B,0xD0,0xFC,0xB2,0x6D,0x9B,0x57,
   0xFE,0xAC,0xDD,0xCB,0x68,0xF4,0x2C,0x71,0xD4,0x23,0x40,0x8E,0x34,0x79,0x5A,0x4E,0x13,0x3C,0x54,0xF8,0xC9,0x02,0x04,0x67,0xBD,0x18,0xAC,0x36,0x4D,0x13,0x02,0x27,
   0x85,0x57,0x1B,0xD5,0xFD,0xCC,0x9C,0x2B,0xF1,0xA6,0x6B,0x1B,0x3C,0xC7,0x68,0x95,0x8D,0xB0,0xD4,0x0C,0x40,0x7D,0xB1,0x04,0xEF,0x7B,0x2A,0x5A,0x7D,0xB7,0xCE,0x74,
   0xFC,0x90,0xC6,0xF7,0xE2,0xF7,0x1F,0x8D,0xC6,0x70,0x18,0x72,0xD6,0xB3,0x19,0x9C,0xF3,0x72,0x82,0x28,0x8C,0x56,0x2C,0x2C,0xE8,0xBC,0xEB,0x07,0x79,0xBC,0x66,0xB9,
   0x9E,0xF9,0xB0,0xD0,0xEC,0x41,0xE1,0xE0,0x06,0x51,0xC2,0x1E,0x61,0x21,0x5B,0xCA,0x8C,0xC7,0x38,0x37,0x8F,0xC0,0x1A,0xBE,0xC7,0x09,0xCE,0x5F,0x09,0x9D,0x17,0x79,
   0xC9,0xE4,0x47,0x07,0xFC,0xE2,0x07,0x0C,0x6B,0x31,0x8B,0x01,0x59,0x1C,0x33,0xBD,0x1C,0xCD,0x69,0xD3,0x86,0x6A,0x6E,0x7B,0x26,0x6C,0x0E,0xF4,0x61,0x66,0x0C,0x30,
   0x19,0xA4,0x4D,0xAE,0xBD,0xEB,0x42,0xE6,0xD5,0x86,0x04,0xBB,0x59,0x61,0xB0,0x29,0xF0,0x56,0x77,0x5F,0xCA,0x18,0xB7,0xED,0x34,0xD6,0x53,0xE5,0xDD,0xEA,0xDC,0x4C,
   0xD6,0x4E,0x22,0x23,0xA4,0x6D,0x54,0x67,0x00,0xBE,0xA0,0x06,0x84,0x9E,0x77,0x9A,0x9C,0x09,0x47,0xA7,0xE1,0x83,0xCA,0x9E,0x05,0x91,0xA3,0x7F,0xA5,0xA6,0x47,0x39,
   0xFD,0x66,0xA8,0x38,0xBD,0x7F,0x19,0xEB,0x3E,0x11,0x28,0x03,0x69,0x7F,0x1E,0x65,0x4C,0x36,0x97,0xBF,0x40,0xEF,0xF5,0x01,0xAF,0x81,0x74,0xD7,0xC4,0xAB,0x9A,0x2F,
   0xB5,0x9C,0x02,0x6F,0x8E,0x4D,0x79,0xD0,0xBF,0xC0,0x5F,0x39,0x25,0xC1,0xAF,0x07,0x09,0xAE,0xA3,0x95,0xB0,0xDA,0x79,0xF9,0xD2,0x79,0x66,0x87,0xEB,0x20,0x78,0xF9,
   0x52,0x77,0x4C,0x38,0xE0,0x94,0x2F,0xE7,0xFE,0xF9,0xCF,0x74,0x58,0xB9,0x2D,0xCB,0xC7,0x94,0x2B,0x2C,0x23,0x6F,0x6B,0x8B,0xF6,0x18,0x03,0x9A,0xB6,0xB0,0x78,0xFA,
   0xB7,0x53,0xAD,0x2B,0xDA,0xEA,0x4E,0xCB,0x67,0x78,0xB4,0x4A,0x27,0x7E,0x2D,0x4E,0x64,0x5D,0xAF,0x26,0x6E,0xFC,0xB5,0x07,0x20,0x8A,0xC5,0x0D,0xF6,0x44,0x62,0x82,
   0x9E,0x01,0x39,0x11,0xF1,0xB6,0xF3,0xB5,0xAC,0x44,0xE2,0xA7,0x51,0xD9,0xAF,0xD0,0xAF,0x5A,0xF9,0xB7,0x2F,0xED,0x1B,0x56,0x32,0xFC,0xF5,0x08,0x70,0x7B,0x1E,0xB3,
   0x7B,0xAD,0xBC,0x5E,0xDE,0x26,0x6E,0xC5,0x09,0x4B,0xB4,0x23,0x03,0x95,0x2F,0x9C,0x38,0x4A,0xBE,0x32,0xC4,0x99,0x69,0xBF,0xFA,0x65,0x00,0xA9,0x56,0x40,0x1C,0x7B,
   0xDD,0xEC,0x5B,0x86,0x31,0x38,0xDC,0xE2,0x08,0x82,0xBD,0x42,0x5B,0xD2,0x50,0x5E,0x36,0x6E,0xC5,0xA1,0x4F,0x34,0x42,0x7B,0x1A,0x2A,0xDB,0x66,0x88,0x73,0xDC,0xBE,
   0x7A,0x38,0xAA,0xAA,0x86,0xEF,0xAA,0x6A,0x85,0xAE,0x4A,0x4C,0xFE,0x9E,0x5A,0xF2,0x12,0x72,0x2B,0x0E,0x9F,0xA2,0xF7,0xDA,0x77,0x9C,0x12,0x39,0xE2,0x5C,0x79,0xC0,
   0x6B,0x30,0x5A,0xF0,0x18,0xBC,0x17,0x3C,0xA6,0x36,0x72,0x62,0x72,0x4F,0xAD,0xC1,0xB0,0x2E,0xCF,0xCE,0x78,0xC2,0xC6,0xF0,0x17,0x3F,0xB4,0xE0,0xFF,0x15,0xFA,0x2F,
   0x6E,0x8F,0x04,0x18,0xAF,0x24,0x00,0x00
};
#endif

struct ILibStun_IceState
{
	// These 2 fields must be the first 2 fields of this structure
	char* rusername;
	char* rkey;
	
	// 
	int rusernamelen;
	int rkeylen;
	int isDoingConnectivityChecks;
	char* offerblock;
	unsigned short blockversion;
	unsigned int blockflags;
	char userAndKey[43];
	char tieBreaker[8];
	char* dtlscerthash;
	int dtlscerthashlen;
	struct ILibStun_IceStateCandidate* hostcandidates;
	char* hostcandidateResponseFlag;
	int hostcandidatecount;
	int requerycount;
	int peerHasActiveOffer;
	int dtlsInitiator;
	int dtlsSession;
	struct ILibStun_Module* parentStunModule;
	long long creationTime;
	int useTurn;
	void *userObject;
};

struct ILibStun_dTlsSession
{
	struct ILibStun_Module* parent;
	SSL* ssl;
	void* User;
	void* Chain;	
	ILibTransport_SendPtr sendPtr;
	ILibTransport_ClosePtr closePtr;
	ILibTransport_PendingBytesToSendPtr pendingPtr;
	unsigned int IdentifierFlags;
	// DO NOT MODIFY ABOVE FIELDS (ILibTransport)

	int sessionId;
	int iceStateSlot;
	struct sockaddr_in6 remoteInterface;
	
	unsigned short inport;
	unsigned short outport;
	unsigned int tag;
	int receiverCredits;
	int senderCredits;
	int congestionWindowSize;
	unsigned int outtsn;
	unsigned int intsn;
	unsigned int userTSN;
	long long lastResent;
	int state; // 0 = Free, 1 = Setup, 2 = Connecting, 3 = Disconnecting, 4 = Handshake
	sem_t Lock;

	ILibSparseArray DataChannelMetaDeta;
	ILibSparseArray DataChannelMetaDetaValues;
	ILibSparseArray PeerFeatureSet;
	ILibSparseArray DataAccumulator;

	unsigned short maxInStreams;
	unsigned short maxOutStreams;

	char* pendingReconfigPacket;
	int reconfigFailures;
	int flags;

	unsigned int RREQSEQ,RRESSEQ;

	unsigned int FastRetransmitExitPoint;
	unsigned int zeroWindowProbeTime;
	unsigned int lastSackTime;
	unsigned int lastRetransmitTime;

	int timervalue;
	unsigned short pendingCount;
	unsigned int pendingByteCount;
	char* pendingQueueHead;
	char* pendingQueueTail;
	unsigned short holdingCount;
	unsigned int holdingByteCount;
	char* holdingQueueHead;
	char* holdingQueueTail;

	ILibLinkedList receiveHoldBuffer;

	char* rpacket;
	int rpacketptr;
	int rpacketsize;

	long freshnessTimestampStart;
	
	void* User2;
	int User3;
	int User4;

#ifdef _WEBRTCDEBUG
	ILibSCTP_OnTSNChanged onTSNChanged;				// TODO: This is not used

	ILibSCTP_OnSCTPDebug onHold;
	ILibSCTP_OnSCTPDebug onReceiverCredits;
	ILibSCTP_OnSCTPDebug onSenderCredits;
	ILibSCTP_OnSCTPDebug onSendFastRetry;
	ILibSCTP_OnSCTPDebug onSendRetry;
	ILibSCTP_OnSCTPDebug onCongestionWindowSizeChanged;
	ILibSCTP_OnSCTPDebug onSACKreceived;

	ILibSCTP_OnSCTPDebug onFastRecovery;
	ILibSCTP_OnSCTPDebug onT3RTX;
	ILibSCTP_OnSCTPDebug onRTTCalculated;

	ILibSCTP_OnSCTPDebug onTSNFloorNotRaised;
#endif

	unsigned int PARTIAL_BYTES_ACKED;
	int SSTHRESH;
	int SRTT;
	int RTTVAR;
	int RTO;
	unsigned int T3RTXTIME;
};


#define ILibWebRTC_DTLS_TO_TIMER_OBJECT(d) ((char*)d+16)
#define ILibWebRTC_DTLS_FROM_TIMER_OBJECT(d) ((struct ILibStun_dTlsSession*)((char*)d-16))

struct ILibStun_Module
{
	ILibChain_PreSelect PreSelect;
	ILibChain_PostSelect PostSelect;
	ILibChain_Destroy Destroy;

	void *UDP;
	void *UDP6;
	void *Timer;
	void *Chain;
	void *user;
	ILibStunClient_OnResult OnResult;

	// STUN State
	struct sockaddr_in LocalIf;
	struct sockaddr_in6 LocalIf6;
	struct sockaddr_in6 Public;
	struct sockaddr_in6 Public2;
	struct sockaddr_in6 Public3;
	struct sockaddr_in6 StunServer;

	struct sockaddr_in6 StunServer2_PrimaryPort;
	struct sockaddr_in6 StunServer2_AlternatePort;


	STUN_STATUS State;
	char Secret[32];
	char TransactionId[12];

	// ICE State
	ILibSCTP_OnConnect OnConnect;
	ILibSCTP_OnData OnData;
	ILibSCTP_OnSendOK OnSendOK;

	int IceStatesNextSlot;
	struct ILibStun_IceState* IceStates[ILibSTUN_MaxSlots];
	int dTlsAllowedNextSlot;
	struct sockaddr_in6 dTlsAllowed[ILibSTUN_MaxSlots];
	SSL_CTX* SecurityContext;
	struct ILibStun_dTlsSession* dTlsSessions[ILibSTUN_MaxSlots];
	char* CertThumbprint;
	int CertThumbprintLength;

	ILibWebRTC_OnOfferUpdated OnOfferUpdated;

	// WebRTC Data Channel
	ILibWebRTC_OnDataChannel OnWebRTCDataChannel;
	ILibWebRTC_OnDataChannelClosed OnWebRTCDataChannelClosed;
	ILibWebRTC_OnDataChannelAck OnWebRTCDataChannelAck;

	// TURN Client
	ILibTURN_ClientModule mTurnClientModule;
	struct sockaddr_in6 mTurnServerAddress;
	struct sockaddr_in6 mRelayedTransportAddress;
	char* turnUsername;
	char* turnPassword;
	int turnUsernameLength;
	int turnPasswordLength;
	int alwaysUseTurn;
	int alwaysConnectTurn;
	int consentFreshnessDisabled;

#ifdef _WEBRTCDEBUG
	int lossPercentage;
	int inboundDropPackets;
	int outboundDropPackets;
#endif
};

unsigned int ILibStun_CRC32(char *buf, int len)
{
	unsigned int c = 0xFFFFFFFF;
	for (; len; --len, ++buf) { c = UPDC32(*buf, c); }
	return ~c;
}

struct ILibStun_Module *g_stunModule = NULL;

// Function prototypes
ILibTransport_DoneState ILibStun_SendSctpPacket(struct ILibStun_Module *obj, int session, char* buffer, int bufferLength);
void ILibStun_OnTimeout(void *object);
void ILibStun_ProcessSctpPacket(struct ILibStun_Module *obj, int session, char* buffer, int bufferLength);
void ILibStun_SctpDisconnect(struct ILibStun_Module *obj, int session);
void ILibStun_SendIceRequest(struct ILibStun_IceState *IceState, int SlotNumber, int useCandidate, struct sockaddr_in6* remoteInterface);
void ILibStun_SendIceRequestEx(struct ILibStun_IceState *IceState, char* TransactionID, int useCandidate, struct sockaddr_in6* remoteInterface);
void ILibStun_ICE_Start(struct ILibStun_IceState *state, int SelectedSlot);
unsigned int crc32c(unsigned int crci, const void *buf, unsigned int len);
int ILibStun_GetDtlsSessionSlotForIceState(struct ILibStun_Module *obj, struct ILibStun_IceState* ice);
void ILibStun_InitiateDTLS(struct ILibStun_IceState *IceState, int IceSlot, struct sockaddr_in6* remoteInterface);
void ILibStun_PeriodicStunCheck(struct ILibStun_Module* obj);
int ILibTURN_GenerateStunFormattedPacketHeader(char* rbuffer, STUN_TYPE packetType, char* transactionID);
int ILibTURN_AddAttributeToStunFormattedPacketHeader(char* rbuffer, int rptr, STUN_ATTRIBUTES attrType, char* data, int dataLen);
void ILibTURN_DisconnectFromServer(ILibTURN_ClientModule turnModule);
ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannelEx2(void *WebRTCModule, unsigned short *streamIds, int streamIdLength);
void ILibWebRTC_PropagateChannelCloseEx(ILibSparseArray sender, struct ILibStun_dTlsSession* obj);
ILibSparseArray ILibWebRTC_PropagateChannelClose(struct ILibStun_dTlsSession* obj, char* packet);

typedef enum ILibWebRTC_DTLS_ContentTypes_Def
{
	ILibAsyncSocket_TLSPlainText_ContentType_ChangeCipherSpec = 20,
	ILibAsyncSocket_TLSPlainText_ContentType_Alert = 21,
	ILibAsyncSocket_TLSPlainText_ContentType_Handshake = 22,
	ILibAsyncSocket_TLSPlainText_ContentType_ApplicationData = 23
}ILibWebRTC_DTLS_ContentTypes;
typedef enum ILibWebRTC_DTLS_HandshakeTypes_Def
{
	ILibWebRTC_DTLSHandshakeType_hello = 0,
	ILibWebRTC_DTLSHandshakeType_clienthello = 1,
	ILibWebRTC_DTLSHandshakeType_serverhello = 2,
	ILibWebRTC_DTLSHandshakeType_certificate = 11,
	ILibWebRTC_DTLSHandshakeType_serverkeyexchange = 12,
	ILibWebRTC_DTLSHandshakeType_certificaterequest = 13,
	ILibWebRTC_DTLSHandshakeType_serverhellodone = 14,
	ILibWebRTC_DTLSHandshakeType_certificateverify = 15,
	ILibWebRTC_DTLSHandshakeType_clientkeyexchange = 16,
	ILibWebRTC_DTLSHandshakeType_finished = 20
}ILibWebRTC_DTLS_HandshakeTypes;

void ILibWebRTC_DTLS_HandshakeDetect(struct ILibStun_Module* obj, char* directionPrefix, char* buffer, int offset, int length)
{
	ILibWebRTC_DTLS_ContentTypes contentType = (ILibWebRTC_DTLS_ContentTypes)buffer[offset+0];
#ifdef _REMOTELOGGING
	unsigned char versionMajor = 255 - buffer[offset+1];
	unsigned char versionMinor = 255 - buffer[offset+2];
#endif
	ILibWebRTC_DTLS_HandshakeTypes tlsHandshakeType = (ILibWebRTC_DTLS_HandshakeTypes)(buffer+offset+13)[0];

	UNREFERENCED_PARAMETER(obj);
	UNREFERENCED_PARAMETER(directionPrefix);
	UNREFERENCED_PARAMETER(length);

	switch(contentType)
	{
		case ILibAsyncSocket_TLSPlainText_ContentType_ChangeCipherSpec:
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [CHANGE-CIPHER-SPEC]", directionPrefix, versionMajor, versionMinor);
			break;
		case ILibAsyncSocket_TLSPlainText_ContentType_Alert:
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [ALERT]", directionPrefix, versionMajor, versionMinor);
			break;
		case ILibAsyncSocket_TLSPlainText_ContentType_ApplicationData:
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [APP/DATA]", directionPrefix, versionMajor, versionMinor);
			break;
		case ILibAsyncSocket_TLSPlainText_ContentType_Handshake:
			switch(tlsHandshakeType)
			{
				case ILibWebRTC_DTLSHandshakeType_hello:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [HELLO]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_clienthello:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [CLIENT-HELLO]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_serverhello:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [SERVER-HELLO]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_certificate:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [CERTIFICATE]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_serverkeyexchange:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [SERVER-KEY-EXCHANGE]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_certificaterequest:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [CERTIFICATE-REQUEST]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_serverhellodone:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [SERVER-HELLO-DONE]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_certificateverify:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [CERTIFICATE-VERIFY]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_clientkeyexchange:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [CLIENT-KEY-EXCHANGE]", directionPrefix, versionMajor, versionMinor);
					break;
				case ILibWebRTC_DTLSHandshakeType_finished:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [FINISHED]", directionPrefix, versionMajor, versionMinor);
					break;
				default:
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "%s DTLS v%d.%d [UNKNOWN Handshake Type]", directionPrefix, versionMajor, versionMinor);
					break;
		
			}
			break;
	}
}

int ILibAlignOnFourByteBoundary(char *data, int dataLen)
{
	int retVal = FOURBYTEBOUNDARY(dataLen);
	if(retVal - dataLen > 0) {memset(data + dataLen, 0, retVal - dataLen);}
	return(retVal);
}

int ILibWebRTC_DataChannelBucketizer(int index)
{
	return(index & (ILibSCTP_Stream_SparseArraySize - 1));
}

void ILibWebRTC_DestroySparseArrayTables_Accumulator(ILibSparseArray sender, int index, void *value, void *user)
{
	ILibSCTP_Accumulator *acc = (ILibSCTP_Accumulator*)value;

	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(index);
	UNREFERENCED_PARAMETER(user);

	if(acc != NULL)
	{
		free(acc->buffer);
		free(acc);
	}
}
void ILibWebRTC_DestroySparseArrayTables(struct ILibStun_dTlsSession *obj)
{
	ILibSparseArray_Destroy(obj->DataChannelMetaDeta);
	ILibSparseArray_Destroy(obj->PeerFeatureSet);
	ILibSparseArray_Destroy(obj->DataChannelMetaDetaValues);
	ILibSparseArray_DestroyEx(obj->DataAccumulator, &ILibWebRTC_DestroySparseArrayTables_Accumulator, NULL);
}
void ILibWebRTC_CreateSparseArrayTables(struct ILibStun_dTlsSession *obj)
{
	obj->DataChannelMetaDeta = ILibSparseArray_Create(ILibSCTP_Stream_SparseArraySize,&ILibWebRTC_DataChannelBucketizer);
	obj->PeerFeatureSet = ILibSparseArray_Create(ILibSCTP_Stream_SparseArraySize, &ILibWebRTC_DataChannelBucketizer);
	obj->DataChannelMetaDetaValues = ILibSparseArray_Create(ILibSCTP_Stream_SparseArraySize, &ILibWebRTC_DataChannelBucketizer);
	obj->DataAccumulator = ILibSparseArray_Create(ILibSCTP_Stream_SparseArraySize, &ILibWebRTC_DataChannelBucketizer);
}

char* SCTP_ERROR_CAUSE_TO_STRING(ILibSCTP_ErrorCause_Header *cause)
{
	
	switch((SCTP_ERROR_CAUSE_CODES)ntohs(cause->CauseCode))
	{
		case SCTP_ERROR_CAUSE_CODE_ASSOCIATION_RESTART_NEW_ADDRESS:
			return("SCTP_ERROR_CAUSE_CODE_ASSOCIATION_RESTART_NEW_ADDRESS");
		case SCTP_ERROR_CAUSE_CODE_COOKIE_RECEIVED_DURING_SHUTDOWN:
			return("SCTP_ERROR_CAUSE_CODE_COOKIE_RECEIVED_DURING_SHUTDOWN");
		case SCTP_ERROR_CAUSE_CODE_INVALID_MANDATORY_PARAMETER:
			return("SCTP_ERROR_CAUSE_CODE_INVALID_MANDATORY_PARAMETER");
		case SCTP_ERROR_CAUSE_CODE_INVALID_STREAM:
			return("SCTP_ERROR_CAUSE_CODE_INVALID_STREAM");
		case SCTP_ERROR_CAUSE_CODE_MISSING_MANDATORY_PARAMATER:
			return("SCTP_ERROR_CAUSE_CODE_MISSING_MANDATORY_PARAMATER");
		case SCTP_ERROR_CAUSE_CODE_NO_USER_DATA:
			return("SCTP_ERROR_CAUSE_CODE_NO_USER_DATA");
		case SCTP_ERROR_CAUSE_CODE_OUT_OF_RESOURCES:
			return("SCTP_ERROR_CAUSE_CODE_OUT_OF_RESOURCES");
		case SCTP_ERROR_CAUSE_CODE_PROTOCOL_VIOLATION:
			return("SCTP_ERROR_CAUSE_CODE_PROTOCOL_VIOLATION");
		case SCTP_ERROR_CAUSE_CODE_STALE_COOKIE:
			return("SCTP_ERROR_CAUSE_CODE_STALE_COOKIE");
		case SCTP_ERROR_CAUSE_CODE_UNRECOGNIZED_CHUNK_TYPE:
			return("SCTP_ERROR_CAUSE_CODE_UNRECOGNIZED_CHUNK_TYPE");
		case SCTP_ERROR_CAUSE_CODE_UNRECOGNIZED_PARAMETERS:
			return("SCTP_ERROR_CAUSE_CODE_UNRECOGNIZED_PARAMETERS");
		case SCTP_ERROR_CAUSE_CODE_UNRESOLVABLE_ADDRESS:
			return("SCTP_ERROR_CAUSE_CODE_UNRESOLVABLE_ADDRESS");
		case SCTP_ERROR_CAUSE_CODE_USER_ABORT:
			return("SCTP_ERROR_CAUSE_CODE_USER_ABORT");
		default:
			return("---");
	}
}

void ILibWebRTC_SetUserObject(void *stunModule, char* localUsername, void *userObject)
{
	struct ILibStun_Module* obj = (struct ILibStun_Module*) stunModule;
	int SlotNumber = ILibStun_CharToSlot(localUsername[0]);
	if (SlotNumber >= 0 && SlotNumber < ILibSTUN_MaxSlots && obj->IceStates[SlotNumber] != NULL) { obj->IceStates[SlotNumber]->userObject = userObject; }
}

void* ILibWebRTC_GetUserObject(void *stunModule, char* localUsername)
{
	struct ILibStun_Module* obj = (struct ILibStun_Module*) stunModule;
	int SlotNumber = ILibStun_CharToSlot(localUsername[0]);
	if (SlotNumber >= 0 && SlotNumber < ILibSTUN_MaxSlots && obj->IceStates[SlotNumber] != NULL) { return(obj->IceStates[SlotNumber]->userObject); }
	return NULL;
}

void* ILibWebRTC_GetUserObjectFromDtlsSession(void *webRTCSession)
{
	struct ILibStun_dTlsSession* session = (struct ILibStun_dTlsSession*)webRTCSession;
	return (session->parent->IceStates[session->iceStateSlot] != NULL ? session->parent->IceStates[session->iceStateSlot]->userObject : NULL);
}

int ILibWebRTC_IsDtlsInitiator(void* dtlsSession)
{
	struct ILibStun_dTlsSession* session = (struct ILibStun_dTlsSession*)dtlsSession;
	return (session->parent->IceStates[session->iceStateSlot]->dtlsInitiator);
}

void ILibStun_OnCompleted(void *object)
{
	struct ILibStun_Module* obj = (struct ILibStun_Module*) object;
	obj->State = STUN_STATUS_COMPLETE;
}

void ILibStun_OnDestroy(void *object)
{
	int i, extraClean = 0;
	struct ILibStun_Module *obj = (struct ILibStun_Module*)object;

	// Clean up all reliable UDP state && all ICE offers (Use the same loop since both tables are ILibSTUN_MaxSlots long)
	for (i = 0; i < ILibSTUN_MaxSlots; i++)
	{
		// Clean up OpenSSL dTLS session
		if (obj->dTlsSessions[i] != NULL)
		{
			if (obj->dTlsSessions[i]->state != 0) extraClean = 1;
			ILibStun_SctpDisconnect(obj, i);
		}

		if (obj->IceStates[i] != NULL)
		{
			// Clean up ICE state
			free(obj->IceStates[i]->offerblock);
			free(obj->IceStates[i]);
			obj->IceStates[i] = NULL;
		}
	}

	if (extraClean == 0) return;

#ifdef WIN32
	Sleep(500);
#else
	sleep(1);
#endif

	for (i = 0; i < ILibSTUN_MaxSlots; i++)
	{
		// Clean up OpenSSL dTLS session
		if (obj->dTlsSessions[i] != NULL)
		{
			free(obj->dTlsSessions[i]);
			obj->dTlsSessions[i] = NULL;
		}
	}
}

int ILibStun_GetFreeSessionSlot(void *StunModule)
{
	int i, j = -1;
	struct ILibStun_Module* obj = (struct ILibStun_Module*)StunModule;

	// Find a free dTLS session slot
	for (i = 0; i < ILibSTUN_MaxSlots; i++) { if (obj->dTlsSessions[i] == NULL || obj->dTlsSessions[i]->state == 0) { j = i; break; } }
	return j;
}

// Returns slot number that was used... -1 on Error
int ILibStun_GetFreeIceStateSlot(struct ILibStun_Module *stunModule, struct ILibStun_IceState* newIceState, struct ILibStun_IceState** oldIceState, int checkUpdateFirst)
{
	int i, slot;

	if (newIceState->userAndKey[0] != 0)
	{
		// If this is nonzero, it's because we generated the original offer. Thus, the slotnumber is encoded in the username
		slot = ILibStun_CharToSlot(newIceState->userAndKey[1]);
		if (slot < ILibSTUN_MaxSlots)
		{
			if (oldIceState != NULL) { *oldIceState = stunModule->IceStates[slot]; }
			stunModule->IceStates[slot] = newIceState;
			ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibStun_GetFreeIceStateSlot: Encoded Slot = %d", slot);
			return slot;
		}
	}

	if (checkUpdateFirst != 0)
	{
		// Check to see if this iceState is an update to an existing one... Keying by remote username/password
		for (i = 0; i < ILibSTUN_MaxSlots; ++i)
		{
			if (stunModule->IceStates[i] != NULL)
			{
				if (stunModule->IceStates[i]->rusernamelen == newIceState->rusernamelen && stunModule->IceStates[i]->rkeylen == newIceState->rkeylen && memcmp(stunModule->IceStates[i]->rusername, newIceState->rusername, newIceState->rusernamelen) == 0 && memcmp(stunModule->IceStates[i]->rkey, newIceState->rkey, newIceState->rkeylen) == 0)
				{
					// These offers are for the same session
					// Check if there is a DTLS session already
					if (stunModule->IceStates[i]->dtlsSession < 0)
					{
						if (oldIceState != NULL) { *oldIceState = stunModule->IceStates[i]; }
						stunModule->IceStates[i] = newIceState;
						ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibStun_GetFreeIceStateSlot: Update Slot = %d", i);
						return i;
					}
					else
					{
						// Return Error, becuase we won't update if DTLS is already active
						return -1;
					}
				}
			}
		}
	}

	for (i = 0; i < ILibSTUN_MaxSlots; ++i)
	{
		slot = (stunModule->IceStatesNextSlot + i) % ILibSTUN_MaxSlots;
		ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibStun_GetFreeIceStateSlot: NextSlot: %d, i: %d, Slot: %d ", stunModule->IceStatesNextSlot, i, slot);
		if (stunModule->IceStates[slot] == NULL || (stunModule->IceStates[slot] != NULL && stunModule->IceStates[slot]->dtlsSession < 0 && ((ILibGetUptime() - stunModule->IceStates[slot]->creationTime) > (ILibSTUN_MaxOfferAgeSeconds * 1000))))
		{
			// This slot is either empty, or contains an offer with no DTLS session, and is older than what is allowed
			if (oldIceState != NULL) { *oldIceState = stunModule->IceStates[slot]; }
			stunModule->IceStates[slot] = newIceState;
			stunModule->IceStatesNextSlot = slot + 1;
			ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibStun_GetFreeIceStateSlot: Free Slot = %d", slot);
			return slot;
		}
		else
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Slot Busy[%d]: Dtls: %d, Age: %d", slot, stunModule->IceStates[slot]->dtlsSession, ILibGetUptime() - stunModule->IceStates[slot]->creationTime);
		}
	}
	return -1;
}

void ILibStun_ClearIceState(void *stunModule, int iceSlot)
{
	struct ILibStun_IceState* ice;
	struct ILibStun_Module* module = (struct ILibStun_Module*)stunModule;

	// Start by removing the IceState Object
	ice = module->IceStates[iceSlot];
	module->IceStates[iceSlot] = NULL;

	if (ice != NULL)
	{
		if (ice->offerblock != NULL) { free(ice->offerblock); }
		free(ice);
	}
}

int ILibStun_GetNextPeriodicInterval(int minVal, int maxVal)
{
	char i;
	float v;

	util_random(1, (char*)&i);
	v = (float)i / (float)255;
	v = (v*(float)maxVal) + (minVal);
	return (int)v;
}


// Get and set the user for a RUDP connection
void ILibSCTP_SetUser(void* module, void* user) { ((struct ILibStun_dTlsSession*)module)->User = user; }
void* ILibSCTP_GetUser(void* module) { return ((struct ILibStun_dTlsSession*)module)->User; }
void ILibSCTP_SetUser2(void* module, void* user) { ((struct ILibStun_dTlsSession*)module)->User2 = user; }
void* ILibSCTP_GetUser2(void* module) { return ((struct ILibStun_dTlsSession*)module)->User2; }
void ILibSCTP_SetUser3(void* module, int user) { ((struct ILibStun_dTlsSession*)module)->User3 = user; }
int ILibSCTP_GetUser3(void* module) { return ((struct ILibStun_dTlsSession*)module)->User3; }
void ILibSCTP_SetUser4(void* module, int user) { ((struct ILibStun_dTlsSession*)module)->User4 = user; }
int ILibSCTP_GetUser4(void* module) { return ((struct ILibStun_dTlsSession*)module)->User4; }

int ILibSCTP_GetPendingBytesToSend(void* module)
{
	int r;
	sem_wait(&(((struct ILibStun_dTlsSession*)module)->Lock));
	r = ((struct ILibStun_dTlsSession*)module)->holdingByteCount;
	sem_post(&(((struct ILibStun_dTlsSession*)module)->Lock));
	if (r < 0) return 0 - r;
	return 0;
}

void ILibSCTP_SetCallbacks(void* StunModule, ILibSCTP_OnConnect onconnect, ILibSCTP_OnData ondata, ILibSCTP_OnSendOK onsendok)
{
	struct ILibStun_Module* stunModule = (struct ILibStun_Module*)StunModule;
	if (onconnect != NULL) { stunModule->OnConnect = onconnect; }
	if (ondata != NULL) { stunModule->OnData = ondata; }
	if (onsendok != NULL) { stunModule->OnSendOK = onsendok; }
	if (onconnect == NULL && ondata == NULL && onsendok == NULL)
	{
		stunModule->OnConnect = NULL;
		stunModule->OnData = NULL;
		stunModule->OnSendOK = NULL;
	}
}

void ILibWebRTC_SetCallbacks(void *StunModule, ILibWebRTC_OnDataChannel OnDataChannel, ILibWebRTC_OnDataChannelClosed OnDataChannelClosed, ILibWebRTC_OnDataChannelAck OnDataChannelAck, ILibWebRTC_OnOfferUpdated OnOfferUpdated)
{
	struct ILibStun_Module* stunModule = (struct ILibStun_Module*)StunModule;
	stunModule->OnWebRTCDataChannel = OnDataChannel;
	stunModule->OnWebRTCDataChannelClosed = OnDataChannelClosed;
	stunModule->OnWebRTCDataChannelAck = OnDataChannelAck;
	stunModule->OnOfferUpdated = OnOfferUpdated;
}

// user must be 8 byte buffer, secret must be 32 byte buffer, key must be 32 byte buffer
void ILibStun_ComputeIntegrityKey(char* user, char* secret, char* key)
{
	char buf[40];
	char result[32];
	memcpy(buf, user, 8);
	memcpy(buf + 8, secret, 32);
	util_sha256(buf, 40, result);
	util_tohex(result, 16, key);
}

// result must be 42 byte buffer. Result will contrain user length (8), 8 byte username, password length (32), 32 byte password
void ILibStun_GenerateUserAndKey(int iceSlot, char* secret, char* result)
{
	char rand[4];
	util_random(4, rand);
	result[0] = 8;
	util_tohex(rand, 4, result + 1);
	// 1st byte of username will be encoded with IceState slot number
	// So when we receive an ICE request, we'll know which offer the request is for, so if the peer
	// elects a candidate we can mark it in the IceState object.
	result[1] = (char)ILibStun_SlotToChar(iceSlot);
	result[9] = 32;
	ILibStun_ComputeIntegrityKey(result + 1, secret, result + 10);
}

int ILibStun_AddMessageIntegrityAttr(char* rbuffer, int ptr, char* integritykey, int integritykeylen)
{
	HMAC_CTX hmac;
	unsigned int hmaclen = 20;

	((unsigned short*)(rbuffer))[1] = htons((unsigned short)(ptr + 24 - 20));						// Set the length
	((unsigned short*)(rbuffer + ptr))[0] = htons((unsigned short)STUN_ATTRIB_MESSAGE_INTEGRITY);	// Attribute header
	((unsigned short*)(rbuffer + ptr))[1] = htons(20);												// Attribute length

	// Setup and perform HMAC-SHA1
	HMAC_CTX_init(&hmac);
	HMAC_Init_ex(&hmac, integritykey, integritykeylen, EVP_sha1(), NULL);
	HMAC_Update(&hmac, (unsigned char*)rbuffer, ptr);
	HMAC_Final(&hmac, (unsigned char*)(rbuffer + ptr + 4), &hmaclen); // Put the HMAC in the outgoing result location
	HMAC_CTX_cleanup(&hmac);
	return 24;
}

int ILibStun_AddFingerprint(char* rbuffer, int ptr)
{
	((unsigned short*)rbuffer)[1] = htons((unsigned short)(ptr + 8 - 20));						// Set the length
	((unsigned short*)(rbuffer + ptr))[0] = htons((unsigned short)STUN_ATTRIB_FINGERPRINT);		// Attribute header
	((unsigned short*)(rbuffer + ptr))[1] = htons(4);											// Attribute length
	((unsigned int*)(rbuffer + ptr))[1] = htonl(ILibStun_CRC32(rbuffer, ptr) ^ 0x5354554e);		// Compute and set the CRC32
	return 8;
}

int ILib_Stun_GetAttributeChangeRequestPacket(int flags, char* TransactionId, char* rbuffer)
{
	int rptr = 20;

	// Should generate a 36 byte packet
	((unsigned short*)rbuffer)[0] = htons(STUN_BINDING_REQUEST);								// Response type, skip setting length for now
	((unsigned int*)rbuffer)[1] = htonl(0x2112A442);											// Set the magic string
	util_random(12, TransactionId);																// Random used for transaction id

	TransactionId[0] = 255;																		// Set the first byte to 255, so it doesn't collide with IceStateSlot
	memcpy(rbuffer + 8, TransactionId, 12);

	((unsigned short*)(rbuffer + rptr))[0] = htons(STUN_ATTRIB_CHANGE_REQUEST);					// Attribute header
	((unsigned short*)(rbuffer + rptr))[1] = htons(4);											// Attribute length
	((unsigned int*)(rbuffer + rptr))[1] = htonl(flags);										// Attribute data
	rptr += 8;

	rptr += ILibStun_AddFingerprint(rbuffer, rptr);												// Set the length in this function
	return rptr;
}

void ILib_Stun_SendAttributeChangeRequest(void* module, struct sockaddr* StunServer, int flags)
{
	struct ILibStun_Module *StunModule = (struct ILibStun_Module*)module;
	int rptr = 20;
	char rbuffer[64];																			// Should generate a 36 byte packet

	((unsigned short*)rbuffer)[0] = htons(STUN_BINDING_REQUEST);								// Response type, skip setting length for now
	((unsigned int*)rbuffer)[1] = htonl(0x2112A442);											// Set the magic string
	util_random(12, StunModule->TransactionId);													// Random used for transaction id

	StunModule->TransactionId[0] = 255;															// Set the first byte to 255, so it doesn't collide with IceStateSlot
	NAT_MAPPING_DETECTION(StunModule->TransactionId) = ((flags & 0x8000)==0x8000)?255:0;		// Mapping Detection vs Public Interface Only Detection
	memcpy(rbuffer + 8, StunModule->TransactionId, 12);

	((unsigned short*)(rbuffer + rptr))[0] = htons(STUN_ATTRIB_CHANGE_REQUEST);					// Attribute header
	((unsigned short*)(rbuffer + rptr))[1] = htons(4);											// Attribute length
	((unsigned int*)(rbuffer + rptr))[1] = htonl(flags & 0xFFFF7FFF);							// Attribute data
	rptr += 8;

	rptr += ILibStun_AddFingerprint(rbuffer, rptr);												// Set the length in this function
	ILibAsyncUDPSocket_SendTo(StunModule->UDP, (struct sockaddr*)StunServer, rbuffer, rptr, ILibAsyncSocket_MemoryOwnership_USER);
}

int ILibStun_WebRTC_UpdateOfferResponse(struct ILibStun_IceState *iceState, char **answer)
{
	int rlen, i;
	int LocalInterfaceListV4Len;
	struct sockaddr_in *LocalInterfaceListV4;
	int BlockFlags = 0;

	int turnRecordSize = 0;

	if (iceState->useTurn != 0) { turnRecordSize = 1 + sizeof(struct sockaddr_in6); }
	if (iceState->dtlsInitiator == 0) { BlockFlags |= ILibWebRTC_SDP_Flags_DTLS_SERVER; }

	// Generate an return answer
	LocalInterfaceListV4Len = ILibGetLocalIPv4AddressList(&LocalInterfaceListV4, 0);
	if (LocalInterfaceListV4Len > 8) LocalInterfaceListV4Len = 8; // Don't send more than 8 candidates

	if (iceState->parentStunModule->alwaysUseTurn != 0) { LocalInterfaceListV4Len = 0; }

	rlen = (6 + 42 + 1 + 32 + 1 + (LocalInterfaceListV4Len * 6));

	if ((*answer = (char*)malloc(rlen + turnRecordSize)) == NULL) ILIBCRITICALEXIT(254);
	((unsigned short*)(*answer))[0] = 1; // Block version number
	((unsigned int*)(*answer + 2))[0] = htonl(BlockFlags); // Block flags
	memcpy(*answer + 6, iceState->userAndKey, 42);

	(*answer)[48] = (char)(iceState->parentStunModule->CertThumbprintLength);
	memcpy(*answer + 49, iceState->parentStunModule->CertThumbprint, iceState->parentStunModule->CertThumbprintLength); // Set DTLS fingerprint here 
	(*answer)[49 + 32] = (char)LocalInterfaceListV4Len;
	for (i = 0; i < LocalInterfaceListV4Len; i++)
	{
		((int*)(*answer + 49 + 33 + (i * 6)))[0] = LocalInterfaceListV4[i].sin_addr.s_addr;
		((short*)(*answer + 49 + 33 + (i * 6) + 4))[0] = ((struct sockaddr_in*)(&(iceState->parentStunModule->LocalIf)))->sin_port;
	}
	if (LocalInterfaceListV4 != NULL) free(LocalInterfaceListV4);
	if (turnRecordSize > 0)
	{
		(*answer + rlen)[0] = INET_SOCKADDR_LENGTH(iceState->parentStunModule->mRelayedTransportAddress.sin6_family);
		memcpy(*answer + rlen + 1, &(iceState->parentStunModule->mRelayedTransportAddress), INET_SOCKADDR_LENGTH(iceState->parentStunModule->mRelayedTransportAddress.sin6_family));
	}
	return (rlen + turnRecordSize);
}

void ILibStun_WebRTC_ConsentFreshness_Continue(void *object)
{
	struct ILibStun_dTlsSession *session = (struct ILibStun_dTlsSession*)object - 1;
	struct timeval tv;
	char TransactionID[12];
	int SessionSlot;

	gettimeofday(&tv, NULL);

	// If this was triggered, it means we haven't received a STUN/Response yet
	// Based on http://tools.ietf.org/html/draft-muthu-behave-consent-freshness-04
	if (tv.tv_sec - session->freshnessTimestampStart >= ILibStun_MaxConsentFreshnessTimeoutSeconds)
	{
		// We need to disconnect DTLS
		ILibRemoteLogging_printf(ILibChainGetLogger(session->parent->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_2, "Consent Freshness Timeout, Closing Session: %d", session->sessionId);
		ILibStun_SctpDisconnect(session->parent, session->sessionId);
	}
	else
	{
		// Keep sending a Probe and wait 500ms for a response, using the same TransactionID
		SessionSlot = session->sessionId;
		memset(TransactionID, 0, 12);
		TransactionID[0] = (char)(SessionSlot ^ 0x80);
		memcpy(TransactionID + 1, &(session->freshnessTimestampStart), sizeof(long) < 11 ? sizeof(long) : 11);

		ILibRemoteLogging_printf(ILibChainGetLogger(session->parent->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_2, "Probing Consent Freshness for Session: %d with %s:%u", session->sessionId, ILibRemoteLogging_ConvertAddress((struct sockaddr*)&(session->remoteInterface)), htons(session->remoteInterface.sin6_port));

		ILibLifeTime_AddEx(session->parent->Timer, session + 1, 500, ILibStun_WebRTC_ConsentFreshness_Continue, NULL);
		ILibStun_SendIceRequestEx(session->parent->IceStates[session->iceStateSlot], TransactionID, 0, &(session->remoteInterface));
	}
}
void ILibStun_WebRTC_ConsentFreshness_Start(void *object)
{
	struct ILibStun_dTlsSession *session = (struct ILibStun_dTlsSession*)object - 1;
	struct timeval tv;
	char TransactionID[12];
	int SessionSlot = session->sessionId;

	if (session->parent->IceStates[session->iceStateSlot] == NULL) { return; }

	memset(TransactionID, 0, 12);
	gettimeofday(&tv, NULL);
	session->freshnessTimestampStart = tv.tv_sec;

	TransactionID[0] = ((unsigned char)SessionSlot ^ 0x80);
	memcpy(TransactionID + 1, &(session->freshnessTimestampStart), sizeof(long) < 11 ? sizeof(long) : 11);

	ILibRemoteLogging_printf(ILibChainGetLogger(session->parent->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_2, "Probing Consent Freshness for Session: %d with %s:%u", session->sessionId, ILibRemoteLogging_ConvertAddress((struct sockaddr*)&(session->remoteInterface)), htons(session->remoteInterface.sin6_port));

	ILibLifeTime_AddEx(session->parent->Timer, session + 1, 500, ILibStun_WebRTC_ConsentFreshness_Continue, NULL);	// Wait 500ms for a response
	ILibStun_SendIceRequestEx(session->parent->IceStates[session->iceStateSlot], TransactionID, 0, &(session->remoteInterface));
}

enum ILibAsyncSocket_SendStatus ILibStun_SendPacketEx(struct ILibStun_Module *stunModule, int useTurn, char* buffer, int offset, int length, struct sockaddr_in6* remoteInterface, enum ILibAsyncSocket_MemoryOwnership memoryOwnership)
{
	if (useTurn != 0)
	{
		if (remoteInterface->sin6_family == 0)
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_5, "[Sending on TURN Channel Binding: %u]", remoteInterface->sin6_port);
			return (ILibTURN_SendChannelData(stunModule->mTurnClientModule, remoteInterface->sin6_port, buffer, offset, length));
		}
		else
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_5, "[Sending TURN Indication to: %s:%u]", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), ntohs(remoteInterface->sin6_port));
			return (ILibTURN_SendIndication(stunModule->mTurnClientModule, remoteInterface, buffer, offset, length));
		}
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_5, "[Sending UDP to: %s:%u]", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), ntohs(remoteInterface->sin6_port));
		return (ILibAsyncSocket_SendTo(stunModule->UDP, buffer + offset, length, (struct sockaddr*)remoteInterface, memoryOwnership));
	}
}

enum ILibAsyncSocket_SendStatus ILibStun_SendPacket(struct ILibStun_IceState *iceState, char* buffer, int offset, int length, struct sockaddr_in6* remoteInterface, enum ILibAsyncSocket_MemoryOwnership memoryOwnership)
{
	return (ILibStun_SendPacketEx(iceState->parentStunModule, iceState->useTurn, buffer, offset, length, remoteInterface, memoryOwnership));
}

void ILibWebRTC_CloseDataChannel_Timeout(void *obj)
{
	struct ILibStun_dTlsSession *o = ILibWebRTC_DTLS_FROM_TIMER_OBJECT(obj);
	int newTimeout;

	sem_wait(&o->Lock);
	++o->reconfigFailures;
	
	newTimeout = RTO_MIN * (0x01 << o->reconfigFailures); // Exponential Backoff
	if(newTimeout > RTO_MAX)
	{
		// Too many failures
		ILibSparseArray dup = ILibWebRTC_PropagateChannelClose(o, o->pendingReconfigPacket);
		ILibRemoteLogging_printf(ILibChainGetLogger(o->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Exceeded maximum retries for RECONFIG CHUNK on Dtls Session: %d", o->sessionId);

		o->pendingReconfigPacket = NULL;
		o->reconfigFailures = 0;
		sem_post(&(o->Lock));

		ILibWebRTC_PropagateChannelCloseEx(dup, o);
	}
	else
	{
		// Retransmit the Reconfig Chunk, and set a new timeout
		ILibRemoteLogging_printf(ILibChainGetLogger(o->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "RECONFIG CHUNK retry attempt %d on Dtls Session: %d", o->reconfigFailures, o->sessionId);
		ILibLifeTime_AddEx(o->parent->Timer, ILibWebRTC_DTLS_TO_TIMER_OBJECT(o), newTimeout, &ILibWebRTC_CloseDataChannel_Timeout, NULL);
		ILibStun_SendSctpPacket(o->parent, o->sessionId, o->pendingReconfigPacket, o->rpacket + o->rpacketsize - o->pendingReconfigPacket);
		sem_post(&o->Lock);
	}
}

int ILibStun_ProcessStunPacket(void *j, char* buffer, int bufferLength, struct sockaddr_in6 *remoteInterface)
{
	struct ILibStun_Module* obj = (struct ILibStun_Module*)j;
	int ptr = 20;
	char* username = NULL;
	unsigned short messageType;
	unsigned short messageLength;
	unsigned short attrType;
	unsigned short attrLength;
	struct sockaddr_in mappedAddress;
	struct sockaddr_in changedAddress;
	char integritykey[33]; // Key is 32, but need 33 to put the terminating null char.
	int integritykeySet = 0;
	int processed = 0;
	int isControlled = 0;
	int isControlling = 0;
	unsigned long long tiebreakValue = 0;

	// Check the length of the packet & IPv4
	if (buffer == NULL || bufferLength < 20 || bufferLength > 10000 || remoteInterface->sin6_family != AF_INET) { ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "Skipping: Not a STUN Packet (Packet Length/Address Family)"); return 0; }

	messageType = ntohs(((unsigned short*)buffer)[0]);											// Get the message type
	messageLength = ntohs(((unsigned short*)buffer)[1]) + 20;									// Get the message length

	// Check the length and magic string
	if (messageLength != bufferLength || ntohl(((int*)buffer)[1]) != 0x2112A442) { ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "Skipping: Not a STUN Packet (Magic String)"); return 0; }

	memset(&mappedAddress, 0, sizeof(mappedAddress));
	memset(&changedAddress, 0, sizeof(changedAddress));

	switch(messageType)
	{
	case STUN_BINDING_REQUEST:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: STUN_BINDING_REQUEST");
		break;
	case STUN_BINDING_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: STUN_BINDING_RESPONSE");
		break;
	case STUN_BINDING_ERROR_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: STUN_BINDING_ERROR_RESPONSE");
		break;
	case STUN_SHAREDSECRET_REQUEST:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: STUN_SHARED_SECRET_REQUEST");
		break;
	case STUN_SHAREDSECRET_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: STUN_SHARED_SECRET_RESPONSE");
		break;
	case STUN_SHAREDSECRET_ERROR_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: STUN_SHARED_SECRET_ERROR_RESPONSE");
		break;
	case TURN_ALLOCATE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_ALLOCATE");
		break;
	case TURN_ALLOCATE_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_ALLOCATE_RESPONSE");
		break;
	case TURN_ALLOCATE_ERROR:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_ALLOCATE_ERROR");
		break;
	case TURN_REFRESH:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_REFRESH");
		break;
	case TURN_REFRESH_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_REFRESH_RESPONSE");
		break;
	case TURN_REFRESH_ERROR:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_REFRESH_ERROR");
		break;
	case TURN_SEND:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_SEND");
		break;
	case TURN_DATA:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_DATA");
		break;
	case TURN_CREATE_PERMISSION:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "R: TURN_CREATE_PERMISSION");
		break;
	case TURN_CREATE_PERMISSION_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "R: TURN_CREATE_PERMISSION_RESPONSE");
		break;
	case TURN_CREATE_PERMISSION_ERROR:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "R: TURN_CREATE_PERMISSION_ERROR");
		break;
	case TURN_CHANNEL_BIND:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "R: TURN_CHANNEL_BIND");
		break;
	case TURN_CHANNEL_BIND_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "R: TURN_CHANNEL_BIND_RESPONSE");
		break;
	case TURN_CHANNEL_BIND_ERROR:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "R: TURN_CHANNEL_BIND_ERROR");
		break;
	case TURN_TCP_CONNECT:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECT");
		break;
	case TURN_TCP_CONNECT_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECT_RESPONSE");
		break;
	case TURN_TCP_CONNECT_ERROR:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECT_ERROR");
		break;
	case TURN_TCP_CONNECTION_BIND:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECTION_BIND");
		break;
	case TURN_TCP_CONNECTION_BIND_RESPONSE:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECTION_BIND_RESPONSE");
		break;
	case TURN_TCP_CONNECTION_BIND_ERROR:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECTION_BIND_ERROR");
		break;
	case TURN_TCP_CONNECTION_ATTEMPT:
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "R: TURN_TCP_CONNECTION_ATTEMPT");
		break;
	}

	{
		char tid[25];
		util_tohex(buffer+8, 12, tid);
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...TransactionID: %s", tid);
	}

	while (ptr + 4 <= messageLength)															// Decode each attribute one at a time
	{
		attrType = ntohs(((unsigned short*)(buffer + ptr))[0]);
		attrLength = ntohs(((unsigned short*)(buffer + ptr))[1]);
		if (ptr + 4 + attrLength > messageLength) { ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ABORT: Message Length Error");return 0; }

		switch (attrType)
		{
			case STUN_ATTRIB_ERROR_CODE:
			{
#ifdef _REMOTELOGGING
				int ErrorCode = (buffer[ptr+6] * 100) + (buffer[ptr+7] % 100);
#endif
				char* ErrorReason = buffer+ptr+8;
				char tid[25];
				
				util_tohex(buffer+8, 12, tid);
				ErrorReason[attrLength] = 0;
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ERROR Received: [%s] Code(%d) Reason(%s) from %s:%u", tid, ErrorCode, ErrorReason, ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), htons(remoteInterface->sin6_port));
				break;
			}

			case STUN_ATTRIB_USE_CANDIDATE:
			{
				break;
			}
			case STUN_ATTRIB_ICE_CONTROLLED:
			{
				isControlled = 1;
				tiebreakValue = ILibNTOHLL(((unsigned long long*)(buffer + ptr + 4))[0]);
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...ICE-CONTROLLED");
				break;
			}
			case STUN_ATTRIB_ICE_CONTROLLING:
			{
				isControlling = 1;
				tiebreakValue = ILibNTOHLL(((unsigned long long*)(buffer + ptr + 4))[0]);
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...ICE-CONTROLLING");
				break;
			}
			case STUN_ATTRIB_MAPPED_ADDRESS:
			case STUN_ATTRIB_XOR_MAPPED_ADDRESS:
			{
				if (buffer[ptr + 5] != 1) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_XOR_MAPPED_ADDRESS/ERROR"); return 0; }
				mappedAddress.sin_family = AF_INET;
				mappedAddress.sin_port = ((unsigned short*)(buffer + ptr + 6))[0];
				mappedAddress.sin_addr.s_addr = ((unsigned int*)(buffer + ptr + 8))[0];
				if (attrType == STUN_ATTRIB_XOR_MAPPED_ADDRESS) { mappedAddress.sin_port ^= 0x1221; mappedAddress.sin_addr.s_addr ^= 0x42A41221; }
				break;
			}
			case STUN_ATTRIB_OTHER_ADDRESS:
			case STUN_ATTRIB_CHANGED_ADDRESS:
			{
				if (buffer[ptr + 5] != 1) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_CHANGED_ADDRESS/ERROR"); return 0; }
				changedAddress.sin_family = AF_INET;
				changedAddress.sin_port = ((unsigned short*)(buffer + ptr + 6))[0];
				changedAddress.sin_addr.s_addr = ((unsigned int*)(buffer + ptr + 8))[0];
				break;
			}
			case STUN_ATTRIB_USERNAME:
			{
				// The username must be at least "xxxxxxxx:x", and our username is always 8 bytes long
				if (attrLength < 9 || buffer[ptr + 4 + 8] != ':') {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_USERNAME/ERROR"); return 0; }
				username = buffer + ptr + 4;
				break;
			}
			case STUN_ATTRIB_FINGERPRINT:
			{
				// Fingerprint must be 4 bytes, must be last attribute.
				if (attrLength != 4 || (ptr + 8) != messageLength) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_FINGERPRINT/ERROR"); return 0; }
				
				// Check the fingerprint
				if ((ILibStun_CRC32(buffer, ptr) ^ 0x5354554e) != ntohl(((int*)(buffer + ptr + 4))[0])) 	 {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_FINGERPRINT/MISMATCH"); return 0; }

				break;
			}
			case STUN_ATTRIB_MESSAGE_INTEGRITY:
			{
				HMAC_CTX hmac;
				unsigned int hmaclen = 20;
				unsigned char hmacresult[20];
				char* key = NULL;
				int keylen = 0;

				if (attrLength != 20) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_MESSAGE_INTEGRITY/LENGTH-ERROR"); return 0; }

				if (username != NULL)
				{
					// Compute the secret key
					ILibStun_ComputeIntegrityKey(username, obj->Secret, integritykey);
					key = integritykey;
					keylen = 32;
					integritykeySet = 1;
				}
				else
				{
					// Grab the key from stored state
					unsigned char slot = buffer[8];

					// Check to see if this is an IceSlot
					if (slot < ILibSTUN_MaxSlots && obj->IceStates[slot] != NULL)
					{
						key = obj->IceStates[slot]->rkey;
						keylen = obj->IceStates[slot]->rkeylen;
					}
					// Check to see if it's a DTLS Session Slot
					else if ((slot ^ 0x80) < ILibSTUN_MaxSlots && obj->dTlsSessions[slot ^ 0x80] != NULL)
					{
						key = obj->dTlsSessions[slot ^ 0x80]->parent->IceStates[obj->dTlsSessions[slot ^ 0x80]->iceStateSlot]->rkey;
						keylen = obj->dTlsSessions[slot ^ 0x80]->parent->IceStates[obj->dTlsSessions[slot ^ 0x80]->iceStateSlot]->rkeylen;
					}
					else
					{
						ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_MESSAGE_INTEGRITY/FAILED"); 
						return 0; // FAILED (or don't care)
						// TODO: Bryan: If we really don't care, we may need to return 1, otherwise this packet will get fed into OpenSSL.
					}
				}

				// Fix the length: -20 + 24 = 4, but we only need to do this if fingerprint is present also
				if (ntohs(((unsigned short*)(buffer + ptr + (4 + FOURBYTEBOUNDARY(attrLength))))[0]) == STUN_ATTRIB_FINGERPRINT)
				{
					((unsigned short*)buffer)[1] = htons((unsigned short)(ptr + 4));
				}

				// Setup and perform HMAC-SHA1
				HMAC_CTX_init(&hmac);
				HMAC_Init_ex(&hmac, key, keylen, EVP_sha1(), NULL);
				HMAC_Update(&hmac, (unsigned char*)buffer, ptr);
				HMAC_Final(&hmac, hmacresult, &hmaclen); // Put the HMAC in the outgoing result location
				HMAC_CTX_cleanup(&hmac);

				// Put the length back, if fingerprint was present
				if (ntohs(((unsigned short*)(buffer + ptr + (4 + FOURBYTEBOUNDARY(attrLength))))[0]) == STUN_ATTRIB_FINGERPRINT)
				{
					((unsigned short*)buffer)[1] = htons(messageLength - 20);
				}

				// Compare the HMAC result
				if (hmaclen != 20 || memcmp(hmacresult, buffer + ptr + 4, 20) != 0) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...STUN_ATTRIB_MESSAGE_INTEGRITY/FAILED(2)"); return 0; }
				break;
			}
		}

		ptr += (4 + FOURBYTEBOUNDARY(attrLength));	// Move the ptr forward by the attribute length plus padding.
	}

	// Message length must be exact
	if (ptr != messageLength) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...PTR != messageLength"); return 0; }

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...Successfully Passed Fingerprint/Message-Integrity");

	if (IS_REQUEST(messageType) && integritykeySet == 1)
	{
		// This is a STUN/ICE request, reply to it here.
		int rptr = 20 + 12;
		char rbuffer[110];																			// Length of this response should be exactly 64 bytes
		int EncodedSlot;

		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "[STUN/ICE REQUEST]");
		processed = 1;

		if (username != NULL)
		{
			EncodedSlot = ILibStun_CharToSlot(username[0]);
			if (EncodedSlot < ILibSTUN_MaxSlots && obj->IceStates[EncodedSlot] != NULL)
			{
				//
				// This will be true, if we are CONTROLLED
				//
				int i = 0;
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...EncodedSlot: %d", EncodedSlot);
				if (remoteInterface->sin6_family == AF_INET) // We currently only support IPv4 candidates
				{
					if (obj->IceStates[EncodedSlot]->hostcandidatecount == 0 && obj->IceStates[EncodedSlot]->hostcandidates == NULL)
					{
						//
						// We generated the offer, which is sitting in this SLOT... We haven't received the response offer yet
						// So we need to just save the interfaces we are receiving authenticated ICE requests on
						//
						for (i = 0; i < 8; ++i)
						{
							if (memcmp(&(((struct sockaddr_in6*)obj->IceStates[EncodedSlot]->offerblock)[i]), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family)) == 0)
							{
								// Don't need to do anything here
								break;
							}
							else if (((struct sockaddr_in6*)obj->IceStates[EncodedSlot]->offerblock)[i].sin6_family == 0)
							{
								// Save the interface we are receiving authentiated ICE requests on
								memcpy(&(((struct sockaddr_in6*)obj->IceStates[EncodedSlot]->offerblock)[i]), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Received ICE Request from: %s:%u , caching until we receive offer", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), htons(remoteInterface->sin6_port));
								break;
							}
						}
					}
					else
					{
						for (i = 0; i < obj->IceStates[EncodedSlot]->hostcandidatecount; ++i)
						{
							if (obj->IceStates[EncodedSlot]->hostcandidates[i].port == remoteInterface->sin6_port)
							{
								if (obj->IceStates[EncodedSlot]->hostcandidates[i].addr == ((struct sockaddr_in*)remoteInterface)->sin_addr.s_addr)
								{
									// Matching Candidate
									if(obj->IceStates[EncodedSlot]->hostcandidateResponseFlag[i] != 1)
									{
										ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Ice Slot: %d  Candidate Match [%s:%u]", EncodedSlot, ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), htons(remoteInterface->sin6_port));
									}
									obj->IceStates[EncodedSlot]->hostcandidateResponseFlag[i] = 1;

									break;
								}
							}
						}
					}
				}

				// The response is going here, becuase we'll always know what the associated iceState is, because we encoded the ice state in our user name, and we
				// always check MessageIntegrity of inbound messages

				if (obj->IceStates[EncodedSlot]->rkey == NULL)
				{
					// Remote Key is unknown... This means the remote offer was not set yet.
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...Remote Offer wasn't set yet, so we won't respond to ICE-REQUESTs");
					return 1;
				}

				if ((obj->IceStates[EncodedSlot]->peerHasActiveOffer == 1 && isControlling != 0) || (obj->IceStates[EncodedSlot]->peerHasActiveOffer == 0 && isControlled != 0))
				{
					// Build the reply packet
					((unsigned short*)rbuffer)[0] = htons(0x0101);												// Response type, skip setting length for now
					((unsigned int*)rbuffer)[1] = htonl(0x2112A442);											// Set the magic string
					memcpy(rbuffer + 8, buffer + 8, 12);														// Copy the transaction id
					((unsigned short*)(rbuffer + 20))[0] = htons(STUN_ATTRIB_XOR_MAPPED_ADDRESS);				// Attribute header
					((unsigned short*)(rbuffer + 20))[1] = htons(8);											// Attribute length
					((unsigned short*)(rbuffer + 20))[2] = htons(1);											// reserved + familly (IPv4)
					((unsigned short*)(rbuffer + 20))[3] = ((struct sockaddr_in*)remoteInterface)->sin_port ^ 0x1221;					// IPv4 port
					((unsigned int*)(rbuffer + 20))[2] = ((struct sockaddr_in*)remoteInterface)->sin_addr.s_addr ^ 0x42A41221;			// IPv4 address														

					rptr += ILibStun_AddMessageIntegrityAttr(rbuffer, rptr, integritykey, 32);
					rptr += ILibStun_AddFingerprint(rbuffer, rptr);												// Set the length in this function

					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...Sending Response to %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), ntohs(remoteInterface->sin6_port));
				}
				else if ((obj->IceStates[EncodedSlot]->peerHasActiveOffer == 1 && isControlled != 0 &&
					ILibNTOHLL(((unsigned long long*)obj->IceStates[EncodedSlot]->tieBreaker)[0]) < tiebreakValue) ||
					(obj->IceStates[EncodedSlot]->peerHasActiveOffer == 0 && isControlling != 0 &&
					ILibNTOHLL(((unsigned long long*)obj->IceStates[EncodedSlot]->tieBreaker)[0]) >= tiebreakValue))
				{
					// Respond with Role Conflict
					char errorCode[] = { 0, 0, 4, 87, 'R', 'o', 'l', 'e', ' ', 'C', 'o', 'n', 'f', 'l', 'i', 'c', 't' };
					char address[20];
					int addressLen = ILibTURN_CreateXORMappedAddress(remoteInterface, address, buffer + 8);
					rptr = ILibTURN_GenerateStunFormattedPacketHeader(rbuffer, STUN_BINDING_ERROR_RESPONSE, buffer + 8);
					rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_XOR_MAPPED_ADDRESS, address, addressLen);
					rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_ERROR_CODE, errorCode, 17);
					rptr += ILibStun_AddMessageIntegrityAttr(rbuffer, rptr, integritykey, 32);
					rptr += ILibStun_AddFingerprint(rbuffer, rptr);												// Set the length in this function

					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Responding with Role Conflict");
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...WE had a %s tiebreak", (obj->IceStates[EncodedSlot]->peerHasActiveOffer == 1)?"SMALLER":"LARGER");
				}
				else
				{
					// We need to switch roles
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Role Conflict, we're switching roles");
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...WE had a %s tiebreak", (obj->IceStates[EncodedSlot]->peerHasActiveOffer == 1)?"LARGER":"SMALLER");

					obj->IceStates[EncodedSlot]->peerHasActiveOffer = obj->IceStates[EncodedSlot]->peerHasActiveOffer == 0 ? 1 : 0;
					ILibStun_ICE_Start(obj->IceStates[EncodedSlot], EncodedSlot);
					return 1;
				}


				// Send the data in plain text
				ILibStun_SendPacket(obj->IceStates[EncodedSlot], rbuffer, 0, rptr, remoteInterface, ILibAsyncSocket_MemoryOwnership_USER);
				//ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)obj)->UDP, (struct sockaddr*)remoteInterface, rbuffer, rptr, ILibAsyncSocket_MemoryOwnership_USER);

				// Send an ICE request back. This is needed to unlock Chrome/Opera inbound port for TLS. Don't do more than ILibSTUN_MaxSlots of these.
				if (obj->IceStates[EncodedSlot]->hostcandidates != NULL && obj->IceStates[EncodedSlot]->hostcandidatecount > 0)
				{
					if (obj->IceStates[EncodedSlot] != NULL && obj->IceStates[EncodedSlot]->requerycount < ILibSTUN_MaxSlots)
					{
						char TransactionID[12];
						util_random(12, TransactionID);
						TransactionID[0] = (char)EncodedSlot;
						obj->IceStates[EncodedSlot]->requerycount++;
						ILibStun_SendIceRequest(obj->IceStates[EncodedSlot], EncodedSlot, 0, (struct sockaddr_in6*)remoteInterface);
					}
				}
			}
		}

		return 1;
	}

	if (IS_SUCCESS_RESP(messageType) && (unsigned char)buffer[8] <= ILibSTUN_MaxSlots)
	{
		int hx;
		// This is a STUN response for a STUN packet we sent as a result of an Offer

		if(obj->IceStates[(int)buffer[8]]->dtlsSession < 0 || (obj->IceStates[(int)buffer[8]]->dtlsSession >= 0 && obj->dTlsSessions[obj->IceStates[(int)buffer[8]]->dtlsSession]->state != 1))
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Received Response to ICE-REQUEST, IceSlot: %d from %s:%u", (int)buffer[8], ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), htons(remoteInterface->sin6_port));
		}

		processed = 1;
		if (obj->IceStates[(int)buffer[8]]->peerHasActiveOffer == 0 && obj->IceStates[(int)buffer[8]]->dtlsSession < 0)		// SlotNumer is encoded as first byte of TransactionID
		{
			// We'll only enter this section if we are CONTROLLING, and there is no DTLS session established yet
			for (hx = 0; hx < obj->IceStates[(int)buffer[8]]->hostcandidatecount; hx++)
			{
				struct sockaddr_in candidateInterface;
				memset(&candidateInterface, 0, sizeof(struct sockaddr_in));
				candidateInterface.sin_family = AF_INET;
				candidateInterface.sin_addr.s_addr = obj->IceStates[(int)buffer[8]]->hostcandidates[hx].addr;
				candidateInterface.sin_port = obj->IceStates[(int)buffer[8]]->hostcandidates[hx].port;

				// Enumerate the Candidates and make sure this STUN Response came from one of them
				if (candidateInterface.sin_family == remoteInterface->sin6_family && memcmp(remoteInterface, &candidateInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family)) == 0)
				{
					// We have a matching ICE Candidate and STUN Response
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Candidate Match [%s:%u]", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), htons(remoteInterface->sin6_port));
					obj->IceStates[(int)buffer[8]]->hostcandidateResponseFlag[hx] = 1;
					break;
				}
			}
		}

		//// TODO: Bryan: I commented this out, because this logic will now take place during Candidate Nomination.
		////
		//if(obj->IceStates[buffer[8]]->peerHasActiveOffer==0 && obj->IceStates[buffer[8]]->dtlsSession<0)		// SlotNumer is encoded as first byte of TransactionID
		//{ 
		//	// We'll only enter this section if we are CONTROLLING, and no DTLS session is established yet
		//	for (hx = 0; hx < obj->IceStates[buffer[8]]->hostcandidatecount; hx++)
		//	{
		//		struct sockaddr_in candidateInterface;
		//		memset(&candidateInterface, 0, sizeof(struct sockaddr_in));
		//		candidateInterface.sin_family = AF_INET;
		//		candidateInterface.sin_addr.s_addr = obj->IceStates[buffer[8]]->hostcandidates[hx].addr;
		//		candidateInterface.sin_port = obj->IceStates[buffer[8]]->hostcandidates[hx].port;

		//		//
		//		// Enumerate the Candidates and make sure this STUN Response came from one of them
		//		//
		//		if(INET_SOCKADDR_LENGTH(remoteInterface->sin6_family) == INET_SOCKADDR_LENGTH(candidateInterface.sin_family))
		//		{
		//			if(remoteInterface->sin6_family == AF_INET)
		//			{
		//				// For now, only IPv4 is supported for DTLS
		//				if(((struct sockaddr_in*)(remoteInterface))->sin_addr.s_addr == candidateInterface.sin_addr.s_addr)
		//				{
		//					if(remoteInterface->sin6_port == candidateInterface.sin_port)
		//					{
		//						//
		//						// We have a matching ICE Candidate and STUN Response
		//						//
		//						ILibStun_InitiateDTLS(obj->IceStates[buffer[8]], buffer[8], remoteInterface);
		//					}
		//				}
		//			}
		//		}
		//	}
		//}
	}
	if (IS_SUCCESS_RESP(messageType) && (((unsigned char)buffer[8] ^ 0x80) < ILibSTUN_MaxSlots))
	{
		// This is an encoded DTLS Session Slot #
		int SessionSlot = (unsigned char)buffer[8] ^ 0x80;

		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_2, "Consent Freshness Updated on IceSlot: %d", SessionSlot);

		processed = 1;
		// We got a response, so we can reset the timer for Freshness
		ILibLifeTime_Remove(obj->Timer, obj->dTlsSessions[SessionSlot] + 1);
		ILibLifeTime_Add(obj->Timer, obj->dTlsSessions[SessionSlot] + 1, ILibStun_MaxConsentFreshnessTimeoutSeconds, ILibStun_WebRTC_ConsentFreshness_Start, NULL);
	}

	if (IS_SUCCESS_RESP(messageType) && memcmp(buffer + 8, obj->TransactionId, 12) == 0) // NAT Detection Response
	{
		int i, result;

		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "[STUN/RESPONSE]");
		processed = 1;

		// This is a STUN server response
		ILibLifeTime_Remove(obj->Timer, obj);

		// Process STUN Results, if we are currently evaluating the NAT
		switch (obj->State)
		{
		case STUN_STATUS_CHECKING_UDP_CONNECTIVITY:
			if (mappedAddress.sin_family == AF_INET)
			{
				if(changedAddress.sin_family == AF_INET)
				{			
					memcpy(&(obj->StunServer2_AlternatePort), &(changedAddress), sizeof(struct sockaddr_in));
					memcpy(&(obj->StunServer2_PrimaryPort), &(changedAddress), sizeof(struct sockaddr_in));
					((struct sockaddr_in*)(&obj->StunServer2_PrimaryPort))->sin_port = ((struct sockaddr_in*)(&obj->StunServer))->sin_port;
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "RFC-5780 Support Detected: %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)&changedAddress), ntohs(changedAddress.sin_port));
				}
				else
				{
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "RFC-5780 Support was NOT detected");
				}
				memcpy(&(obj->Public), &(mappedAddress), sizeof(struct sockaddr_in));
			}
			else
			{
				// Problem decoding the packet.
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Unknown, NULL, obj->user); }
				ILibStun_OnCompleted(obj);
				return 1;
			}

			result = 0;
			if (((struct sockaddr_in*)&(obj->LocalIf))->sin_addr.s_addr == 0)
			{
				// Check if the public IP address is one of our own.
				struct sockaddr_in* LocalInterfaceListV4 = NULL;
				int LocalInterfaceListV4Len = ILibGetLocalIPv4AddressList(&LocalInterfaceListV4, 0);
				if (LocalInterfaceListV4 != NULL)
				{
					for (i = 0; i < LocalInterfaceListV4Len; ++i)
					{
						if (((struct sockaddr_in*)&(obj->Public))->sin_addr.s_addr == ((struct sockaddr_in*)&(LocalInterfaceListV4[i]))->sin_addr.s_addr) result = 1;
					}
					free(LocalInterfaceListV4);
				}
			}
			else
			{
				if (((struct sockaddr_in*)&(obj->Public))->sin_addr.s_addr == ((struct sockaddr_in*)&(obj->LocalIf))->sin_addr.s_addr) result = 1;
			}

			if (result == 1)
			{
				// No Nat
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_No_NAT, (struct sockaddr_in*)&(obj->Public), obj->user); }
				ILibStun_OnCompleted(obj);
				break;
			}

			if(NAT_MAPPING_DETECTION(obj->TransactionId)==0)
			{
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Public_Interface, (struct sockaddr_in*)&(obj->Public), obj->user); }
				ILibStun_OnCompleted(obj);
			}
			else if(changedAddress.sin_family == AF_INET)
			{
				obj->State = STUN_STATUS_CHECKING_FULL_CONE_NAT;
				ILibLifeTime_Add(obj->Timer, obj, ILibStunClient_TIMEOUT, &ILibStun_OnTimeout, NULL); // This timeout should *NEVER* happen unless RFC5780 is not implemented
				ILib_Stun_SendAttributeChangeRequest(obj, (struct sockaddr*)&(obj->StunServer2_PrimaryPort), 0x00);
			}
			else
			{
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_RFC5780_NOT_IMPLEMENTED, (struct sockaddr_in*)&(obj->Public), obj->user); }
				ILibStun_OnCompleted(obj);
			}
			break;
		case STUN_STATUS_CHECKING_FULL_CONE_NAT:
			memcpy(&(obj->Public2), &(mappedAddress), sizeof(struct sockaddr_in));
			if (((struct sockaddr_in*)(&obj->Public))->sin_port == ((struct sockaddr_in*)(&obj->Public2))->sin_port)
			{
				// Endpoint-Independent Mapping (Full-Cone)
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Full_Cone_NAT, (struct sockaddr_in*)&(obj->Public), obj->user); }
				ILibStun_OnCompleted(obj);
			}
			else
			{
				// Run Phase-III test
				obj->State = STUN_STATUS_CHECKING_SYMETRIC_NAT;
				ILib_Stun_SendAttributeChangeRequest(obj, (struct sockaddr*)&(obj->StunServer2_AlternatePort), 0x00);
				ILibLifeTime_Add(obj->Timer, obj, ILibStunClient_TIMEOUT, &ILibStun_OnTimeout, NULL); // This timeout should *NEVER* happen for this case
			}

			break;
		case STUN_STATUS_CHECKING_SYMETRIC_NAT:
			memcpy(&(obj->Public3), &(mappedAddress), sizeof(struct sockaddr_in));
			if (((struct sockaddr_in*)(&obj->Public2))->sin_port == ((struct sockaddr_in*)(&obj->Public3))->sin_port)
			{
				// Address Dependent Mapping
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Restricted_NAT, (struct sockaddr_in*)&(obj->Public3), obj->user); }
				ILibStun_OnCompleted(obj);
			}
			else
			{
				// Address and Port Dependent Mapping (Symmetric in this case)
				if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Symetric_NAT, (struct sockaddr_in*)&(obj->Public3), obj->user); }
				ILibStun_OnCompleted(obj);
			}
			break;
		case STUN_STATUS_CHECKING_RESTRICTED_NAT:
			// Restricted NAT
			if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Restricted_NAT, (struct sockaddr_in*)&(obj->Public), obj->user); }
			ILibStun_OnCompleted(obj);
			break;
		default:
			break;
		}

	}

	if (processed == 0) { ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "DID NOT Process Packet of type: %u", messageType);}

	return 1;
}

void ILibStun_DelaySendIceRequest_OnLifeTimeDestroy(void *object)
{
	free(object);
}

void ILibStun_DelaySendIceRequest_OnLifeTime(void *object)
{
	struct sockaddr_in dest;
	int Ptr;
	char *Packet = (char*)object;
	struct ILibStun_Module *stun;
	char *Data;

	Ptr = ntohs(((unsigned short*)Packet)[1]) + 20;
	Data = Packet + Ptr;

	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = ((unsigned int*)Data)[0];
	dest.sin_port = ((unsigned short*)Data)[2];
	stun = (struct ILibStun_Module*)((void**)(Data + 7))[0];

	ILibStun_SendPacketEx(stun, (int)Data[6], Packet, 0, Ptr, (struct sockaddr_in6*)&dest, ILibAsyncSocket_MemoryOwnership_CHAIN);
	//ILibAsyncSocket_SendTo(stun->UDP, Packet, Ptr, (struct sockaddr*)&dest, ILibAsyncSocket_MemoryOwnership_CHAIN);
}

int ILibStun_GenerateIceRequestPacket(struct ILibStun_IceState* module, char* Packet, char* TransactionID, int useCandidate, struct sockaddr_in6* remote)
{
	char remKey[64];
	char *stunUsername;
	char Address[20];
	int AddressLen;
	int Ptr = 0;
	
	AddressLen = ILibTURN_CreateXORMappedAddress(remote, Address, TransactionID);

	if ((stunUsername = (char*)malloc(10 + module->rusernamelen)) == NULL){ ILIBCRITICALEXIT(254); }
	memcpy(stunUsername, module->rusername, module->rusernamelen);
	memcpy(stunUsername + module->rusernamelen, ":", 1);
	memcpy(stunUsername + module->rusernamelen + 1, module->userAndKey + 1, 8);
	stunUsername[9 + module->rusernamelen] = 0;

	ILibRemoteLogging_printf(ILibChainGetLogger(module->parentStunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_2, "Generating ICE Request [%s] Credentials[%s]", ILibRemoteLogging_ConvertToHex(TransactionID, 12), stunUsername);

	Ptr = ILibTURN_GenerateStunFormattedPacketHeader(Packet, STUN_BINDING_REQUEST, TransactionID);
	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_USERNAME, stunUsername, 9 + module->rusernamelen);

	if (useCandidate != 0)
	{
		Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_USE_CANDIDATE, NULL, 0);
	}

	if (module->peerHasActiveOffer == 0)
	{
		// We are CONTROLLING
		Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_ICE_CONTROLLING, module->tieBreaker, 8);
	}
	else
	{
		// We are CONTROLLED
		Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_ICE_CONTROLLED, module->tieBreaker, 8);
	}


	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_XOR_MAPPED_ADDRESS, Address, AddressLen);
	Ptr += ILibStun_AddMessageIntegrityAttr(Packet, Ptr, module->rkey, module->rkeylen);
	Ptr += ILibStun_AddFingerprint(Packet, Ptr);
	free(stunUsername);

	memcpy(remKey, module->rkey, module->rkeylen);
	remKey[module->rkeylen] = 0;
	ILibRemoteLogging_printf(ILibChainGetLogger(module->parentStunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_3, "...Using Integrity Key: %s", remKey);
	return Ptr;
}

void ILibStun_SendIceRequestEx(struct ILibStun_IceState *IceState, char* TransactionID, int useCandidate, struct sockaddr_in6* remoteInterface)
{
	int Ptr;
	char Packet[512];

	Ptr = ILibStun_GenerateIceRequestPacket(IceState, Packet, TransactionID, useCandidate, remoteInterface);
	ILibStun_SendPacket(IceState, Packet, 0, Ptr, remoteInterface, ILibAsyncSocket_MemoryOwnership_USER);
	//ILibAsyncSocket_SendTo(IceState->parentStunModule->UDP, Packet, Ptr, (struct sockaddr*)remoteInterface, ILibAsyncSocket_MemoryOwnership_USER);
}

void ILibStun_SendIceRequest(struct ILibStun_IceState *IceState, int SlotNumber, int useCandidate, struct sockaddr_in6* remoteInterface)
{
	char TransactionID[12];
	TransactionID[0] = (char)SlotNumber;
	util_random(11, TransactionID + 1);
	ILibStun_SendIceRequestEx(IceState, TransactionID, useCandidate, remoteInterface);
}

void ILibStun_DelaySendIceRequest(struct ILibStun_IceState* module, int selectedSlot, int useCandidate)
{
	char TransactionID[12];
	char *Packet, *Data;
	int Ptr;

	int i, ii;
	struct sockaddr_in remote;
	int delay;

	for (i = 0; i < module->hostcandidatecount; ++i) // Queue an ICE Request for each candidate
	{
		for (ii = 0; ii < 2; ++ii) // Queue TWO requests for each candidate
		{
			char dest[255];

			memset(&remote, 0, sizeof(struct sockaddr_in));
			remote.sin_family = AF_INET;
			remote.sin_port = module->hostcandidates[i].port;
			remote.sin_addr.s_addr = module->hostcandidates[i].addr;

			TransactionID[0] = (char)selectedSlot;  // We're going to encode the IceState slot in the Transaction ID, so we can refer to it in the response
			util_random(11, TransactionID + 1);

			if ((Packet = (char*)malloc(512)) == NULL){ ILIBCRITICALEXIT(254); }
			Ptr = ILibStun_GenerateIceRequestPacket(module, Packet, TransactionID, useCandidate, (struct sockaddr_in6*)&remote);

			ILibInet_ntop2((struct sockaddr*)&remote, dest, 255);
			ILibRemoteLogging_printf(ILibChainGetLogger(module->parentStunModule->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "IceSlot: %d Sending ICE Request to: %s:%u [Controlling: %s]", selectedSlot, dest, ntohs(remote.sin_port), module->peerHasActiveOffer == 0 ? "YES" : "NO");

			Data = Packet + Ptr;
			((unsigned int*)Data)[0] = module->hostcandidates[i].addr;
			((unsigned short*)Data)[2] = module->hostcandidates[i].port;
			Data[6] = (char)(module->useTurn);
			((void**)(Data + 7))[0] = module->parentStunModule;


			delay = ILibStun_GetNextPeriodicInterval(100, 500);
			ILibLifeTime_AddEx(module->parentStunModule->Timer, Packet, delay, ILibStun_DelaySendIceRequest_OnLifeTime, ILibStun_DelaySendIceRequest_OnLifeTimeDestroy);
		}
	}
}

int ILibStun_GenerateIceOffer(void* StunModule, char** offer, char* userName, char* password)
{
	int slot;
	char userAndKey[43];
	struct ILibStun_IceState *ice;
	struct ILibStun_Module* obj = (struct ILibStun_Module*)StunModule;

	if ((ice = (struct ILibStun_IceState*)malloc(sizeof(struct ILibStun_IceState))) == NULL){ ILIBCRITICALEXIT(254); }
	memset(ice, 0, sizeof(struct ILibStun_IceState));
	util_random(8, ice->tieBreaker);
	ice->useTurn = obj->alwaysUseTurn;
	ice->parentStunModule = obj;
	ice->creationTime = ILibGetUptime();
	if ((ice->offerblock = (char*)malloc(8 * sizeof(struct sockaddr_in6))) == NULL){ ILIBCRITICALEXIT(254); }
	ice->peerHasActiveOffer = 0;
	ice->dtlsInitiator = 1;
	memset(ice->offerblock, 0, 8 * sizeof(struct sockaddr_in6));
	slot = ILibStun_GetFreeIceStateSlot(obj, ice, NULL, 0);

	if (slot < 0)
	{
		// No free slots
		free(ice->offerblock);
		free(ice);
		return 0;
	}

	// Encoding the slot number into the user name, so we can do some processing when we receive ICE requests
	ILibStun_GenerateUserAndKey(slot, obj->Secret, userAndKey);
	memcpy(ice->userAndKey, userAndKey, 43);
	memcpy(userName, userAndKey + 1, userAndKey[0]);
	memcpy(password, userAndKey + userAndKey[0] + 2, userAndKey[userAndKey[0] + 1]);
	userName[(int)userAndKey[0]] = 0;
	password[(int)userAndKey[(int)userAndKey[0] + 1]] = 0;

	return ILibStun_WebRTC_UpdateOfferResponse(ice, offer);
}

void ILibStun_ICE_FinalizeConnectivityChecks(void *object)
{
	struct ILibStun_Module* obj = (struct ILibStun_Module*)object - 1;
	int i;
	int x;
	
	// Go through each ICE offer that is saved, and find the ones that were doing ICE Connectivity Checks, and finalize all of them
	for (i = 0; i < ILibSTUN_MaxSlots; ++i)
	{
		if (obj->IceStates[i] != NULL && obj->IceStates[i]->isDoingConnectivityChecks != 0)
		{
			obj->IceStates[i]->isDoingConnectivityChecks = 0;
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "FINALIZING Connectivity checks on IceSlot: %d", i);

			for (x = 0; x < obj->IceStates[i]->hostcandidatecount; ++x)
			{
				if (obj->IceStates[i]->hostcandidateResponseFlag[x] != 0)
				{
					if (obj->IceStates[i]->peerHasActiveOffer == 0)
					{
						// Since this list is in priority order, we'll nominate the highest priority candidate that received a response
						struct sockaddr_in dest;

						memset(&dest, 0, sizeof(struct sockaddr_in));
						dest.sin_family = AF_INET;
						dest.sin_port = obj->IceStates[i]->hostcandidates[x].port;
						dest.sin_addr.s_addr = obj->IceStates[i]->hostcandidates[x].addr;
						ILibStun_SendIceRequest(obj->IceStates[i], i, 1, (struct sockaddr_in6*)&dest);

						if (obj->IceStates[i]->dtlsInitiator != 0)
						{
							// Simultaneously initiate DTLS
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Initiating DTLS to: %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)&dest), ntohs(dest.sin_port));
							ILibStun_InitiateDTLS(obj->IceStates[i], i, (struct sockaddr_in6*)&dest);
						}
						else
						{
							// We are DTLS Server, not Client
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Waiting for DTLS from: %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)&dest), ntohs(dest.sin_port));
						}
						break;
					}
				}
			}
		}
	}
}

void ILibStun_PeriodicStunCheckEx(void *obj)
{
	ILibStun_PeriodicStunCheck((struct ILibStun_Module*)obj - 3);
}

void ILibStun_PeriodicStunCheck(struct ILibStun_Module* obj)
{
	int i;
	int needRecheck = 0;

	ILibLifeTime_Remove(obj->Timer, obj + 3);

	for (i = 0; i < ILibSTUN_MaxSlots; ++i)
	{
		if (obj->IceStates[i] != NULL && obj->IceStates[i]->hostcandidates != NULL && obj->IceStates[i]->dtlsSession < 0 && ((ILibGetUptime() - obj->IceStates[i]->creationTime) < ILibSTUN_MaxOfferAgeSeconds * 1000))
		{
			// We will only do periodic stuns for IceOffers that don't have DTLS sessions associated, and are not expired
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_4, "PeriodicStun for IceStateSlot[%d]", i);
			ILibStun_DelaySendIceRequest(obj->IceStates[i], i, 0);
			needRecheck = 1;
		}
	}

	if (needRecheck != 0)
	{
		ILibLifeTime_Add(obj->Timer,
			obj + 3,
			ILibStun_GetNextPeriodicInterval(ILibSTUN_MinNATKeepAliveIntervalSeconds, ILibSTUN_MaxNATKeepAliveIntervalSeconds),
			ILibStun_PeriodicStunCheckEx,
			NULL);
	}
}

void ILibStun_ICE_Start(struct ILibStun_IceState *state, int SelectedSlot)
{
	struct ILibStun_Module *obj = state->parentStunModule;
	if (state->peerHasActiveOffer == 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Starting Connectivity Checks...");
		// Since we're not a STUN-LITE implementation, we need to perform connectivity checks
		// We'll only do this, if we're the initiator and the peer is passive
		state->isDoingConnectivityChecks = 1;
		ILibStun_DelaySendIceRequest(state, SelectedSlot, 0);

		// All of these packets are distributed within a 500ms window. 
		// We'll set a timer for 3 seconds, to finalize the connectivity check
		// The timer is set on the stunModule, so we don't have a race condition if the ice offer is updated/deleted during
		// these 3 seconds.
		ILibLifeTime_Add(obj->Timer, obj + 1, 3, ILibStun_ICE_FinalizeConnectivityChecks, NULL);
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Skipping Connectivity Checks...");
		// We are CONTROLLED, so we just need to start the Periodic ICE Request Packets
		ILibStun_PeriodicStunCheck(obj);
	}
}

void ILibStun_SetIceOffer2_TURN_CreatePermissionResponse(ILibTURN_ClientModule turnModule, int success, void *user)
{
	int SelectedSlot = (int)(uintptr_t)user; // We're casting it this way, becuase SelectedSlot can never be > 255
	struct ILibStun_Module* obj = (struct ILibStun_Module*)ILibTURN_GetTag(turnModule);
	struct ILibStun_IceState* state = obj->IceStates[SelectedSlot];

	if (success != 0)
	{
		if (state->peerHasActiveOffer == 0)
		{
			// Since we're not a STUN-LITE implementation, we need to perform connectivity checks
			// We'll only do this, if we're the initiator and the peer is passive
			state->isDoingConnectivityChecks = 1;
			ILibStun_DelaySendIceRequest(state, SelectedSlot, 0);

			// All of these packets are distributed within a 500ms window. 
			// We'll set a timer for 3 seconds, to finalize the connectivity check
			// The timer is set on the stunModule, so we don't have a race condition if the ice offer is updated/deleted during
			// these 3 seconds.
			ILibLifeTime_Add(obj->Timer, obj + 1, 3, ILibStun_ICE_FinalizeConnectivityChecks, NULL);
		}
		else
		{
			// We are CONTROLLED, so we just need to start the Periodic ICE Request Packets
			ILibStun_PeriodicStunCheck(obj);
		}
	}
}

int ILibStun_SetIceOffer2(void *StunModule, char* iceOffer, int iceOfferLen, char *username, int usernameLength, char* password, int passwordLength, char** answer)
{
	int generateUserAndKey = 1;
	struct ILibStun_Module* obj = (struct ILibStun_Module*)StunModule;
	struct ILibStun_IceState *state, *oldState = NULL;
	int i, j, rlen;
	int SelectedSlot = -1;
	int candidateCount;

	if (iceOffer == NULL || iceOfferLen == 0) return 0;

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Setting Ice Offer:");
#ifdef _REMOTELOGGING
	if(username!=NULL && usernameLength>0)
	{
		char tmp = username[usernameLength];
		username[usernameLength] = 0;
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...username: %s", username);
		username[usernameLength] = tmp;
	}
#endif

	// Decode the offer
	if ((state = (struct ILibStun_IceState*)malloc(sizeof(struct ILibStun_IceState))) == NULL) ILIBCRITICALEXIT(254);
	memset(state, 0, sizeof(struct ILibStun_IceState));

	util_random(8, state->tieBreaker);
	state->useTurn = obj->alwaysUseTurn;
	candidateCount = iceOffer[7 + iceOffer[6] + 1 + iceOffer[7 + iceOffer[6]] + 1 + iceOffer[7 + iceOffer[6] + 1 + iceOffer[7+iceOffer[6]]]];

	// We're mallocing this so we can encapsulate the ConnectivityCheck + Turn Candidate
	if ((state->offerblock = (char*)malloc(iceOfferLen + candidateCount + 1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy(state->offerblock, iceOffer, iceOfferLen);
	memset(state->offerblock + iceOfferLen, 0, candidateCount + 1);
	state->blockversion = ntohs(((unsigned short*)(state->offerblock))[0]);
	state->blockflags = ntohl(((unsigned int*)(state->offerblock + 2))[0]);
	state->rusernamelen = state->offerblock[6];
	state->rusername = state->offerblock + 7;
	state->rkeylen = state->offerblock[7 + state->rusernamelen];
	state->rkey = state->offerblock + 7 + state->rusernamelen + 1;
	state->dtlscerthashlen = state->offerblock[7 + state->rusernamelen + 1 + state->rkeylen];
	state->dtlscerthash = state->offerblock + 7 + state->rusernamelen + 1 + state->rkeylen + 1;
	state->hostcandidatecount = state->offerblock[7 + state->rusernamelen + 1 + state->rkeylen + 1 + state->dtlscerthashlen];
	state->hostcandidates = (struct ILibStun_IceStateCandidate*)(state->offerblock + 7 + state->rusernamelen + 1 + state->rkeylen + 1 + state->dtlscerthashlen + 1);
	state->hostcandidateResponseFlag = state->offerblock + iceOfferLen;
	state->requerycount = 0;
	state->peerHasActiveOffer = ((state->blockflags & ILibWebRTC_SDP_Flags_DTLS_SERVER) == ILibWebRTC_SDP_Flags_DTLS_SERVER ? 0 : 1);
	state->dtlsInitiator = ((state->blockflags & ILibWebRTC_SDP_Flags_DTLS_SERVER) == ILibWebRTC_SDP_Flags_DTLS_SERVER ? 1 : 0);
	state->parentStunModule = obj;
	state->dtlsSession = -1;
	state->creationTime = ILibGetUptime();

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Remote is: [%d] %s", state->blockflags, state->peerHasActiveOffer != 0 ? "Initiator" : "Receiver");
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Certificate: %s", ILibRemoteLogging_ConvertToHex(state->dtlscerthash, state->dtlscerthashlen));


	//
	// If username/pasword was passed in, use that because we generated the original offer, 
	// so we have to keep the credentials the same, otherwise the peer will not be able to connect.
	//
	if (username != NULL && password != NULL && usernameLength + passwordLength == 40)
	{
		state->userAndKey[0] = (char)usernameLength;
		memcpy(state->userAndKey + 1, username, usernameLength);
		state->userAndKey[1 + usernameLength] = (char)passwordLength;
		memcpy(state->userAndKey + 2 + usernameLength, password, passwordLength);
	}

	SelectedSlot = ILibStun_GetFreeIceStateSlot(obj, state, &oldState, 1);
	if (SelectedSlot < 0)
	{
		// We couldn't update/add the new offer
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Unable to update/add offer");
		free(state->offerblock);
		free(state);
		return 0;
	}
	else
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Offer Added/Updated into Ice Slot: %d", SelectedSlot);
		if (oldState != NULL)
		{		
			// The old object that was in this slot, was just a placeholder that was storing DTLS Allowed Candidates
			// for the actual offer that we are now putting in it.
			if (oldState->hostcandidatecount == 0 && oldState->hostcandidates == NULL)
			{
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "......No previous offer was set, this must be the peer's answer to our original offer");
				for (i = 0; i < 8; ++i)
				{
					if (((struct sockaddr_in6*)oldState->offerblock)[i].sin6_family != 0)
					{
						for (j = 0; j < state->hostcandidatecount; ++j)
						{
							struct sockaddr_in candidate;
							memset(&candidate, 0, sizeof(struct sockaddr_in));

							candidate.sin_family = AF_INET;
							candidate.sin_port = state->hostcandidates[j].port;
							candidate.sin_addr.s_addr = state->hostcandidates[j].addr;

							if (memcmp(&candidate, &(((struct sockaddr_in6*)oldState->offerblock)[i]), INET_SOCKADDR_LENGTH(candidate.sin_family)) == 0)
							{
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Candidate: %s:%u was already unlocked", ILibRemoteLogging_ConvertAddress((struct sockaddr*)&candidate), ntohs(candidate.sin_port));
								state->hostcandidateResponseFlag[j] = 1;
								break;
							}
						}
					}
				}
			}
			// We need to copy UserAndKey, because we will be using the same username and password
			memcpy(state->userAndKey, oldState->userAndKey, sizeof(state->userAndKey));
			generateUserAndKey = 0;
			free(oldState->offerblock);
			free(oldState);
			oldState = NULL;
		}
	}

	if (generateUserAndKey != 0)
	{
		ILibStun_GenerateUserAndKey(SelectedSlot, obj->Secret, state->userAndKey); // We are going to encode the slot number in the username
	}

	// Generate an return answer
	rlen = ILibStun_WebRTC_UpdateOfferResponse(state, answer);

	// If we are using a TURN server, we need to Create Permissions
	if (state->useTurn != 0)
	{
		struct sockaddr_in6* permission = (struct sockaddr_in6*)malloc(state->hostcandidatecount * sizeof(struct sockaddr_in6));
		if (permission == NULL){ ILIBCRITICALEXIT(254); }
		memset(permission, 0, state->hostcandidatecount * sizeof(struct sockaddr_in6));
		for (i = 0; i < state->hostcandidatecount; ++i)
		{
			((struct sockaddr_in*)&(permission[i]))->sin_family = AF_INET;
			((struct sockaddr_in*)&(permission[i]))->sin_port = state->hostcandidates[i].port;
			((struct sockaddr_in*)&(permission[i]))->sin_addr.s_addr = state->hostcandidates[i].addr;
		}
		ILibTURN_CreatePermission(state->parentStunModule->mTurnClientModule, permission, state->hostcandidatecount, ILibStun_SetIceOffer2_TURN_CreatePermissionResponse, (void*)(uintptr_t)SelectedSlot); // We are casting SelectedSlot this way, becuase it will always be < 255
		free(permission);
		return rlen;
		// We are returning here, because if we need to create permissions on the TURN server, we need to wait for it to complete, 
		// otherwise if we try to send the packets below, they'll get dropped... So we'll send those packets in the callback
	}


	ILibStun_ICE_Start(state, SelectedSlot);


	return rlen;
}

// Decode and save an ICE offer block, then return an answer block
int ILibStun_SetIceOffer(void* StunModule, char* iceOffer, int iceOfferLen, char** answer)
{
	// Initial offers are required to be Active or ActPass, so peerHasActiveOffer is set to 1
	// We need to save the username/password from our generated offer, when we pass in an offer. If we didn't generate the initial offer, pass in NULL
	return ILibStun_SetIceOffer2(StunModule, iceOffer, iceOfferLen, NULL, 0, NULL, 0, answer);
}

void ILibORTC_SetRemoteParameters(void* stunModule, char *username, int usernameLen, char *password, int passwordLen, char *certHash, int certHashLen, char *localUserName)
{
	// Generate a pseudo WebRTC Offer
	char *answer;
	char offer[255];
	int offerLen = 7 + usernameLen + 1 + passwordLen + 1 + certHashLen + 1;
	struct ILibStun_Module* obj = (struct ILibStun_Module*)stunModule;
	int localUserNameLen;
	char* localPassword;
	int localPasswordLen;
	int slot;

	if(offerLen > sizeof(offer)) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibORTC_SetRemoteParameters called, but passed data is > %d bytes", (int)sizeof(offer)); return;}
	slot = ILibStun_CharToSlot(localUserName[0]);
	if(slot < 0 || slot >= ILibSTUN_MaxSlots) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibORTC_SetRemoteParameters called with invalid local username"); return;}

	localUserNameLen = obj->IceStates[slot]->userAndKey[0];
	localPasswordLen = obj->IceStates[slot]->userAndKey[localUserNameLen+1];
	localPassword = obj->IceStates[slot]->userAndKey + 1 + localUserNameLen + 1;

	((unsigned short*)offer)[0] = htons(1); // Block Version
	((unsigned int*)offer+2)[0] = htons(0); // Block Version
	offer[6] = usernameLen;					// Username Length
	memcpy(offer+7, username, usernameLen);	// Username
	offer[7+usernameLen] = passwordLen;		// Password Length
	memcpy(offer+7+usernameLen+1, password, passwordLen);	// Password
	offer[7 + usernameLen + 1 + passwordLen] = certHashLen;	// Cert Fingerprint Length
	memcpy(offer + 7 + usernameLen + 1 + passwordLen + 1, certHash, certHashLen);	// Cert Fingerprint
	offer[7 + usernameLen + 1 + passwordLen + 1 + certHashLen] = 0;					// No Candidates yet

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibORTC_SetRemoteParameters -> ILibStun_SetIceOffer2");
	ILibStun_SetIceOffer2(stunModule, offer, offerLen, localUserName, localUserNameLen, localPassword, localPasswordLen, &answer);
	free(answer);
}

void ILibORTC_AddCandidate(void *stunModule, char* localUsername, struct sockaddr_in6 *candidate)
{
	int slot = ILibStun_CharToSlot(localUsername[0]);

	if (slot < 0 || slot > ILibSTUN_MaxSlots) { ILibRemoteLogging_printf(ILibChainGetLogger(((struct ILibStun_Module*)stunModule)->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibORTC_AddCandidate called with invalid local username"); return; }
	ILibRemoteLogging_printf(ILibChainGetLogger(((struct ILibStun_Module*)stunModule)->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "ILibORTC_AddCandidate: %s", ILibRemoteLogging_ConvertAddress((struct sockaddr*)candidate));
	

}

ILibTransport_DoneState ILibStun_SendDtls(struct ILibStun_Module *obj, int session, char* buffer, int bufferLength)
{
	ILibTransport_DoneState r = ILibTransport_DoneState_ERROR;
	char exBuffer[4096];
	int j;
	if (obj == NULL || session < 0 || session > (ILibSTUN_MaxSlots-1) || obj->dTlsSessions[session] == NULL || SSL_state(obj->dTlsSessions[session]->ssl) != 3) return ILibTransport_DoneState_ERROR;

#ifdef _WEBRTCDEBUG
	// Simulated Inbound Packet Loss
	if ((obj->lossPercentage) > 0 && ((rand() % 100) >= (100 - obj->lossPercentage))) { return; }
#endif

	SSL_write(obj->dTlsSessions[session]->ssl, buffer, bufferLength);
	while (BIO_ctrl_pending(SSL_get_wbio(obj->dTlsSessions[session]->ssl)) > 0)
	{
		// Data is pending in the write buffer, send it out
		j = BIO_read(SSL_get_wbio(obj->dTlsSessions[session]->ssl), exBuffer, 4096);
		if (obj->IceStates[obj->dTlsSessions[session]->iceStateSlot]->useTurn != 0)
		{
			if (ILibTURN_GetPendingBytesToSend(obj->mTurnClientModule) == 0)
			{
				r = (ILibTransport_DoneState)ILibTURN_SendChannelData(obj->mTurnClientModule, (unsigned short)session, exBuffer, 0, j);
			}
			else
			{
				// If the TCP socket is overflowing, just drop the packet... SCTP will take care of retrying this later
				r = ILibTransport_DoneState_INCOMPLETE; // ToDo: Make Sure SendOK is getting called
			}
		}
		else
		{
			r = (ILibTransport_DoneState)ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)obj)->UDP, (struct sockaddr*)&(obj->dTlsSessions[session]->remoteInterface), exBuffer, j, ILibAsyncSocket_MemoryOwnership_USER);
		}
	}
	return r;
}

// This method assumes the buffer has 12 byte available for the header.
ILibTransport_DoneState ILibStun_SendSctpPacket(struct ILibStun_Module *obj, int session, char* buffer, int bufferLength)
{
	if (bufferLength < 12) return ILibTransport_DoneState_ERROR;

	// Setup the header
	((unsigned short*)buffer)[0] = htons(obj->dTlsSessions[session]->inport);		// Source port
	((unsigned short*)buffer)[1] = htons(obj->dTlsSessions[session]->outport);		// Destination port
	((unsigned int*)buffer)[1] = obj->dTlsSessions[session]->tag;					// Tag

	// Compute and put the CRC at the right place
	((unsigned int*)buffer)[2] = 0;
	((unsigned int*)buffer)[2] = crc32c(0, buffer, (unsigned int)bufferLength);

	return(ILibStun_SendDtls(obj, session, buffer, bufferLength));
	// ILibStun_ProcessSctpPacket(obj, -1, buffer, bufferLength); // DEBUG
}

int ILibStun_AddSctpChunkHeader(char* buffer, int ptr, unsigned char chunktype, unsigned char chunkflags, unsigned short chunklength)
{
	buffer[ptr] = chunktype;
	buffer[ptr + 1] = chunkflags;
	((unsigned short*)(buffer + ptr))[1] = htons(chunklength);
	return ptr + 4;
}

int ILibStun_AddSctpChunkArg(char* buffer, int ptr, unsigned short otype, char* data, int datalen)
{
	ptr = ILibAlignOnFourByteBoundary(buffer, ptr);
	((unsigned short*)(buffer + ptr))[0] = ntohs(otype);
	((unsigned short*)(buffer + ptr))[1] = ntohs((unsigned short)(4 + datalen));
	if (datalen != 0) memcpy(buffer + ptr + 4, data, datalen);
	return ptr + 4 + datalen;
}

int ILibStun_SctpAddSackChunk(struct ILibStun_Module *obj, int session, char* packet, int ptr)
{
	int clen = 16;
	unsigned int tsn = obj->dTlsSessions[session]->intsn;
	unsigned int bytecount = 0;

	// Skip all packets with a low TSN. We do ths bacause we have not processed them yet, but will after adding this SACK
	void *node = ILibLinkedList_GetNode_Head(obj->dTlsSessions[session]->receiveHoldBuffer);
	while(node!=NULL && ntohl(((ILibSCTP_DataPayload*)ILibLinkedList_GetDataFromNode(node))->TSN) <= obj->dTlsSessions[session]->userTSN)
	{
		node = ILibLinkedList_GetNextNode(node);
	}

	// Compute all selective ACK's
	while (node != NULL)
	{
		ILibSCTP_DataPayload *p = (ILibSCTP_DataPayload*)ILibLinkedList_GetDataFromNode(node);
		unsigned int gstart = ntohl(p->TSN);
		unsigned int gend = gstart;
		bytecount += ntohs(p->length);

		node = ILibLinkedList_GetNextNode(node);
		while (node != NULL && (p = (ILibSCTP_DataPayload*)ILibLinkedList_GetDataFromNode(node))!=NULL)
		{
			unsigned int x = ntohl(p->TSN);
			bytecount += ntohs(p->length);
			if (x == gend + 1) { gend = x; }
			else break;
			node = ILibLinkedList_GetNextNode(node);
		}

		if (clen < 500) // Cap the number of encoded gaps.
		{
			((unsigned short*)(packet + ptr + clen))[0] = htons((unsigned short)(gstart - tsn));			// Start	
			((unsigned short*)(packet + ptr + clen))[1] = htons((unsigned short)(gend - tsn));				// End
			RCTPRCVDEBUG(printf("SACK %u + %u to %u\r\n", tsn, gstart, gend);)
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP: %d SENT [SACK] %u + %u to %u", session, tsn, gstart, gend);
				clen += 4;
		}
	}

	// Create response
	if (bytecount > ILibSCTP_MaxReceiverCredits) bytecount = ILibSCTP_MaxReceiverCredits;
	ILibStun_AddSctpChunkHeader(packet, ptr, RCTP_CHUNK_TYPE_SACK, 0, (unsigned short)clen);
	((unsigned int*)(packet + ptr + 4))[0] = htonl(obj->dTlsSessions[session]->intsn);		// Cumulative TSN Ack
	((unsigned int*)(packet + ptr + 8))[0] = htonl(ILibSCTP_MaxReceiverCredits - bytecount);// Advertised Receiver Window Credit (a_rwnd) 
	((unsigned short*)(packet + ptr + 12))[0] = htons(((unsigned short)clen - 16) / 4);		// Number of Gap Ack Blocks
	((unsigned short*)(packet + ptr + 14))[0] = htons(0);									// Number of Duplicate TSNs

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP: %d SENT [SACK] a_rwnd: %u Cumalative TSN: %u", session, ILibSCTP_MaxReceiverCredits - bytecount, obj->dTlsSessions[session]->intsn);

	return (ptr + clen);
}

ILibTransport_DoneState ILibStun_SctpSendDataEx(struct ILibStun_Module *obj, int session, unsigned char flags, unsigned short streamid, unsigned short streamnum, int pid, char* data, int datalen)
{
	int len;
	int rptr = sizeof(char*) + 8 + 12;
	char* rpacket;
	unsigned int tsn = obj->dTlsSessions[session]->outtsn;
	obj->dTlsSessions[session]->outtsn++;

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d] ILibStun_SctpSendDataEx -> outtsn = %u", session, tsn + 1);

	// Create data packet, allow for a header in front.
	len = sizeof(char*) + 8 + 12 + 16 + datalen;
	if ((rpacket = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	((char**)(rpacket))[0] = NULL;																				// Pointer to the next packet (Used for queuing)
	((unsigned short*)(rpacket + sizeof(char*)))[0] = (unsigned short)(12 + 16 + datalen);						// Size of the packet (Used for queuing)
	((unsigned short*)(rpacket + sizeof(char*)))[1] = 0;														// Number of times the packet was resent (Used for retry)
	((unsigned int*)(rpacket + sizeof(char*)))[1] = 0;															// Last time the packet was sent (Used for retry)
	
	// There is a 12 byte GAP here to accomodate SCTP Common Header, if necessary

	ILibStun_AddSctpChunkHeader(rpacket, rptr, RCTP_CHUNK_TYPE_DATA, flags, (unsigned short)(16 + datalen));	// Setup the data chunk header
	((unsigned int*)(rpacket + rptr + 4))[0] = htonl(tsn);														// Cumulative TSN Ack
	((unsigned short*)(rpacket + rptr + 8))[0] = htons(streamid);												// Stream Identifier
	((unsigned short*)(rpacket + rptr + 10))[0] = htons(streamnum);												// Stream Sequence Number
	((unsigned int*)(rpacket + rptr + 12))[0] = htonl(pid);														// Payload Protocol Identifier
	memcpy(rpacket + rptr + 16, data, datalen);																	// Copy the user data
	rptr += (16 + datalen);

	RCTPDEBUG(printf("OUT DATA_CHUNK FLAGS: %d, TSN: %u, ID: %d, SEQ: %d, PID: %u, SIZE: %d\r\n", flags, tsn, streamid, streamnum, pid, datalen);)

	// Check the credits
	if ((obj->dTlsSessions[session]->receiverCredits < datalen) || datalen > obj->dTlsSessions[session]->senderCredits || (obj->dTlsSessions[session]->holdingCount != 0))
	{
		// Add this packet to the holding queue
		if (obj->dTlsSessions[session]->holdingQueueTail == NULL) { obj->dTlsSessions[session]->holdingQueueHead = rpacket; }
		else { ((char**)(obj->dTlsSessions[session]->holdingQueueTail))[0] = rpacket; }
		obj->dTlsSessions[session]->holdingQueueTail = rpacket;
		obj->dTlsSessions[session]->holdingCount++;
		obj->dTlsSessions[session]->holdingByteCount += datalen;
		// if (obj->dTlsSessions[session]->holdingCount == 1) printf("HOLD\r\n");
#ifdef _WEBRTCDEBUG
		if (obj->dTlsSessions[session]->onHold != NULL) { obj->dTlsSessions[session]->onHold(obj->dTlsSessions[session], "OnHold", obj->dTlsSessions[session]->holdingCount); }
#endif
		return ILibTransport_DoneState_INCOMPLETE; // Hold, we don't have anymore credits
	}

	// Update the packet retry data
	((unsigned int*)(rpacket + sizeof(char*)))[1] = (unsigned int)ILibGetUptime();								// Last time the packet was sent (Used for retry)
	if (obj->dTlsSessions[session]->T3RTXTIME == 0)
	{
		// Only set the T3RTX timer if it is not already running
		obj->dTlsSessions[session]->T3RTXTIME = ((unsigned int*)(rpacket + sizeof(char*)))[1];
#ifdef _WEBRTCDEBUG
		if (obj->dTlsSessions[session]->onT3RTX != NULL){ obj->dTlsSessions[session]->onT3RTX(obj, "OnT3RTX", obj->dTlsSessions[session]->RTO); }
#endif
	}

	// Add this packet to the pending ack queue
	obj->dTlsSessions[session]->receiverCredits -= datalen; // Receiver Window
	obj->dTlsSessions[session]->senderCredits -= datalen;   // Congestion Window

	if (obj->dTlsSessions[session]->pendingQueueTail == NULL) { obj->dTlsSessions[session]->pendingQueueHead = rpacket; }
	else { ((char**)(obj->dTlsSessions[session]->pendingQueueTail))[0] = rpacket; }
	obj->dTlsSessions[session]->pendingQueueTail = rpacket;
	obj->dTlsSessions[session]->pendingCount++;
	obj->dTlsSessions[session]->pendingByteCount += (((unsigned short*)(rpacket + sizeof(char*)))[0] - (12 + 16));

#ifdef _WEBRTCDEBUG
	// Debug Event
	if (obj->dTlsSessions[session]->onReceiverCredits != NULL) { obj->dTlsSessions[session]->onReceiverCredits(obj->dTlsSessions[session], "OnReceiverCredits", obj->dTlsSessions[session]->receiverCredits); }
#endif

	// Send the packet now
	if ((flags & 0x03) == 0x03 && obj->dTlsSessions[session]->rpacketptr > 0 && obj->dTlsSessions[session]->rpacketsize > (obj->dTlsSessions[session]->rpacketptr + 16 + datalen + 4))
	{
		// Merge this data chunk in packet that is going to be sent

		// Align/Pad on 32bit aligned pointer
		obj->dTlsSessions[session]->rpacketptr = ILibAlignOnFourByteBoundary(obj->dTlsSessions[session]->rpacket, obj->dTlsSessions[session]->rpacketptr);

		memcpy(obj->dTlsSessions[session]->rpacket + obj->dTlsSessions[session]->rpacketptr, rpacket + sizeof(char*) + 8 + 12, 16 + datalen);
		obj->dTlsSessions[session]->rpacketptr += (16 + datalen);
	}
	else
	{
		// Send in a seperate packet
		return ILibStun_SendSctpPacket(obj, session, rpacket + sizeof(char*) + 8, rptr - sizeof(char*) - 8); // Problem sending
	}

	return ILibTransport_DoneState_COMPLETE; // Everything is ok
}


ILibTransport_DoneState ILibStun_SctpSendData(struct ILibStun_Module *obj, int session, unsigned short streamid, int pid, char* data, int datalen)
{
	ILibTransport_DoneState r = ILibTransport_DoneState_ERROR;
	int len, ptr = 0;
	unsigned char flags = 0; // 2 = Start, 0 = Middle, 1 = End, 3 = Start & End
	ILibSCTP_StreamAttributes attr;
	ILibSCTP_StreamAttributes_Data attrData;
	unsigned short seq;

	attr.Raw = ILibSparseArray_Get(obj->dTlsSessions[session]->DataChannelMetaDeta, streamid);
	attrData.Raw = ILibSparseArray_Get(obj->dTlsSessions[session]->DataChannelMetaDetaValues, streamid);

	if(pid != 50 && ((attr.Data.StatusFlags & ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED) != ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED || 
		data == NULL || datalen == 0))
	{
		return ILibTransport_DoneState_ERROR; // Error
	}

	seq = attrData.Data.NextSequenceNumber++;
	ILibSparseArray_Add(obj->dTlsSessions[session]->DataChannelMetaDetaValues, streamid, attrData.Raw);


	// Send the data in one block
	if (datalen <= 1232) return ILibStun_SctpSendDataEx(obj, session, 3, streamid, seq, pid, data, datalen);

	// Break the data into parts
	while (ptr < datalen)
	{
		// Compute the length of this block
		len = datalen - ptr;
		if (len > 1232) len = 1232;

		// Compute the flags
		flags = 0;
		if (ptr == 0) flags |= 0x02;
		if (ptr + len == datalen) flags |= 0x01;

		// Send the block
		r = ILibStun_SctpSendDataEx(obj, session, flags, streamid, seq, pid, data + ptr, len);
		ptr += len;
	}

	return r;
}

// Public method used to send data on RCTP stream
ILibTransport_DoneState ILibSCTP_Send(void* module, unsigned short streamId, char* data, int datalen)
{
	return ILibSCTP_SendEx(module, streamId, data, datalen, 53);
}

ILibTransport_DoneState ILibSCTP_SendEx(void* module, unsigned short streamId, char* data, int datalen, int dataType)
{
	ILibTransport_DoneState r;
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)module;
	if (obj->state != 2) return ILibTransport_DoneState_ERROR; // Error
	sem_wait(&(obj->Lock));
	r = ILibStun_SctpSendData(obj->parent, obj->sessionId, streamId, dataType, data, datalen);
	sem_post(&(obj->Lock));
	RCTPDEBUG(printf("ILibSCTP_Send() len=%d, r=%d\r\n", datalen, r);)
	return r;
}

void ILibStun_SctpProcessStreamData(struct ILibStun_Module *obj, int session, unsigned short streamId, unsigned short steamSeq, unsigned char chunkflags, int pid, char* data, int datalen)
{
	struct ILibStun_dTlsSession *o = (struct ILibStun_dTlsSession*)obj->dTlsSessions[session];
	UNREFERENCED_PARAMETER(steamSeq); // We expect all packets to be in order now.

	// TODO: Add error case for when invalid streamId is specified. Bryan

	sem_wait(&(obj->dTlsSessions[session]->Lock));
	if(pid == 50)
	{
		// WebRTC Control
		ILibSCTP_StreamAttributes attributes;
		attributes.Raw = ILibSparseArray_Get(obj->dTlsSessions[session]->DataChannelMetaDeta, streamId);

		switch(data[0])
		{
			case 0x02: // WebRTC Data Channel Protocol: DATA_CHANNEL_ACK				
				if((attributes.Data.StatusFlags & ILibSCTP_StreamAttributesData_Assigned_Status_WAITING_FOR_ACK) == ILibSCTP_StreamAttributesData_Assigned_Status_WAITING_FOR_ACK)
				{
					attributes.Data.StatusFlags ^= ILibSCTP_StreamAttributesData_Assigned_Status_WAITING_FOR_ACK;
					attributes.Data.StatusFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED;
					ILibSparseArray_Add(obj->dTlsSessions[session]->DataChannelMetaDeta, streamId, attributes.Raw);		

					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Data Channel ACK on session: %u for StreamId: %u", session, streamId);

					sem_post(&(obj->dTlsSessions[session]->Lock));
					if (obj->OnWebRTCDataChannelAck != NULL) { obj->OnWebRTCDataChannelAck(obj, obj->dTlsSessions[session], streamId); }
					sem_wait(&(obj->dTlsSessions[session]->Lock));
				}
				break;
			case 0x03: // WebRTC Data Channel Protocol: DATA_CHANNEL_OPEN
				{
					// Respond with DATA_CHANNEL_ACK
					char DATA_CHANNEL_ACK = 0x02;
					char *channelName = data + 12;
					int channelNameLength = (int)ntohs(((unsigned short*)data)[4]);
					int sendAck = 1;
					ILibSCTP_StreamAttributes_Data attributesData;		

					channelName[channelNameLength] = 0;
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Data Channel Open request on session: %u for StreamId: %u [%s]", session, streamId, channelName);

					if((attributes.Data.StatusFlags & ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED) == ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED)
					{
						// This data channel already exists
						ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...This stream ID already exists!");
						break;
					}
					attributes.Raw = 0x00;
					attributesData.Raw = 0x00;

					attributes.Data.StatusFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED;

					switch((ILibWebRTC_DataChannel_ReliabilityModes)((unsigned char)data[1]))
					{
						case ILibWebRTC_DataChannel_ReliabilityMode_RELIABLE:	
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Reliable");
							break;
						case ILibWebRTC_DataChannel_ReliabilityMode_RELIABLE_UNORDERED:
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Reliable / UNORDERED");
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_UNORDERED;
							break;
						case ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_REXMIT:
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Partial-Reliable / REXMIT");
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_REXMIT;
							attributesData.Data.ReliabilityValue = (unsigned short)(ntohl(((unsigned int*)(data+4))[0]));
							break;
						case ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_REXMIT_UNORDERED:
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Partial-Reliable / UNORDERED + REXMIT");
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_UNORDERED;
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_REXMIT;
							attributesData.Data.ReliabilityValue = (unsigned short)(ntohl(((unsigned int*)(data+4))[0]));
							break;
						case ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_TIMED:
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Partial-Reliable / TIMED");
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_TIMED;
							attributesData.Data.ReliabilityValue = (unsigned short)(ntohl(((unsigned int*)(data+4))[0]));
							break;
						case ILibWebRTC_DataChannel_ReliabilityMode_PARTIAL_RELIABLE_TIMED_UNORDERED:
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Partial-Reliable / TIMED + UNORDERED");
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_TIMED;
							attributes.Data.ReliabilityFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_UNORDERED;
							attributesData.Data.ReliabilityValue = (unsigned short)(ntohl(((unsigned int*)(data+4))[0]));
							break;
					}
					ILibSparseArray_Add(obj->dTlsSessions[session]->DataChannelMetaDeta, streamId, attributes.Raw);
					ILibSparseArray_Add(obj->dTlsSessions[session]->DataChannelMetaDetaValues, streamId, attributesData.Raw);

					sem_post(&(obj->dTlsSessions[session]->Lock));
					if (obj->OnWebRTCDataChannel != NULL) { sendAck = obj->OnWebRTCDataChannel(obj, obj->dTlsSessions[session], streamId, data + 12, channelNameLength); }
					sem_wait(&(obj->dTlsSessions[session]->Lock));

					if (sendAck == 0)
					{
						// Respond with DATA_CHANNEL_ACK
						ILibStun_SctpSendData(obj, session, streamId, 50, &DATA_CHANNEL_ACK, 1);
					}
				}
				break;
		}
	}
	else
	{
		// Other
		if ((chunkflags & 0x03) == 0x03)
		{
			// This isn't needed, becuase ConsentFreshness algorithm will shut down the connection if the other side disappears. 
			//
			//// This is a full data block, check if this is a close signal
			//if (obj->dTlsSessions[session]->state == 2 && pid == 51 && datalen == 5 && memcmp(data, "close", 5) == 0)
			//{
			//	sem_post(&(obj->dTlsSessions[session]->Lock));
			//	ILibStun_SctpDisconnect(obj, session);
			//	return;
			//}

			// This is a full data block
			if (obj->OnData != NULL && obj->dTlsSessions[session]->state == 2)
			{
				sem_post(&(obj->dTlsSessions[session]->Lock));
				obj->OnData(obj, obj->dTlsSessions[session], streamId, pid, data, datalen, &(obj->dTlsSessions[session]->User));
				if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state != 2) return;
				sem_wait(&(obj->dTlsSessions[session]->Lock));
			}
			// ILibStun_SctpSendData(obj, session, 0, pid, data, datalen); // ECHO
		}
		else
		{
			// Start of a data accumulation
			ILibSCTP_Accumulator *acc = (ILibSCTP_Accumulator*)ILibSparseArray_Get(obj->dTlsSessions[session]->DataAccumulator, streamId);
			if(acc == NULL) {acc = ILibSCTP_CreateAccumulator();}

			if (chunkflags & 0x02) { acc->bufferPtr = 0; }

			// Accumulate data
			if (acc->bufferPtr + datalen > acc->bufferLen)
			{
				// Realloc the accumulation buffer if needed
				if ((acc->buffer = (char*)realloc(acc->buffer, acc->bufferPtr + datalen)) == NULL) ILIBCRITICALEXIT(254);
				acc->bufferLen = acc->bufferPtr + datalen;
			}
			memcpy(acc->buffer + acc->bufferPtr, data, datalen);
			acc->bufferPtr += datalen;

			ILibSparseArray_Add(obj->dTlsSessions[session]->DataAccumulator, streamId, acc);

			// End of data accumulation
			if (chunkflags & 0x01)
			{
				if (obj->OnData != NULL && obj->dTlsSessions[session]->state == 2)
				{
					sem_post(&(obj->dTlsSessions[session]->Lock));
					obj->OnData(obj, obj->dTlsSessions[session], streamId, pid, acc->buffer, acc->bufferPtr, &(obj->dTlsSessions[session]->User));
					if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state != 2) return;
					sem_wait(&(obj->dTlsSessions[session]->Lock));
				}
				//ILibStun_SctpSendData(obj, session, streamId, pid, obj->dTlsSessions[session]->dataAssembly, obj->dTlsSessions[session]->dataAssemblyPtr); // ECHO
			}
		}
	}
	
	// Check to see if there are any Channel Close operations that are pending, waiting for a specific TSN
	if(o->pendingReconfigPacket != NULL && ((ILibSCTP_PendingTSN_Data*)((char*)&o->pendingReconfigPacket))->Data.Type == 0xFF)
	{
		unsigned short offset = ((ILibSCTP_PendingTSN_Data*)((char*)&o->pendingReconfigPacket))->Data.StreamIdOffset;
		int streamIdCount = (offset / 2) - 2;

		if(((unsigned int*)(o->rpacket + o->rpacketsize - offset))[0] <= o->userTSN)
		{
			// The LastTSN specified by the peer has passed, so we can continue with the ChannelClose operation
			// Let's locally initiate a ChannelClose... We'll propagate the close in the response handler
			char tmp[4096];
			if(streamIdCount * 2 * sizeof(unsigned short) <= 4096) // Only do this if we have enough memory to copy the streamId values
			{
				if(streamIdCount > 0) {memcpy(tmp, o->rpacket + o->rpacketsize - offset + 4, streamIdCount * sizeof(unsigned short));}
				o->pendingReconfigPacket = NULL;
				ILibWebRTC_CloseDataChannelEx2(o, (unsigned short*)tmp, streamIdCount);
			}
			else
			{
				// Not enough memory... Just dump all the streams, as the Peer endpoint is being retarded by specifying so many streams individually
				o->pendingReconfigPacket = NULL;
				ILibWebRTC_CloseDataChannelEx2(o, (unsigned short*)tmp, 0); // Ignore Klocwork Error, it's not a problem in this case
			}											
		}
	}

	sem_post(&(obj->dTlsSessions[session]->Lock));
}

void ILibStun_SctpDisconnect_Final(void *obj)
{
	struct ILibStun_dTlsSession* o = (struct ILibStun_dTlsSession*)obj;
	char* packet;
	void *node;

	// Free the SSL state
	if (o->ssl != NULL) { SSL_free(o->ssl); o->ssl = NULL; }

	if (o->holdingByteCount > 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(o->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP(%d) Was closed while %d bytes are pending in HoldingQueue", o->sessionId, o->holdingByteCount);
	}

	// Free the rpacket buffer
	if (o->rpacket != NULL) { free(o->rpacket); o->rpacket = NULL; }

	// Free all packets pending ACK queue
	while (o->pendingQueueHead != NULL)
	{
		packet = ((char**)(o->pendingQueueHead))[0];
		free(o->pendingQueueHead);
		o->pendingQueueHead = packet;
	}

	// Free all packets in holding queue
	while (o->holdingQueueHead != NULL)
	{
		packet = ((char**)(o->holdingQueueHead))[0];
		free(o->holdingQueueHead);
		o->holdingQueueHead = packet;
	}

	// Free all packets in receive holding queue
	node = ILibLinkedList_GetNode_Head(o->receiveHoldBuffer);
	while(node != NULL)
	{
		free(ILibLinkedList_GetDataFromNode(node));
		node = ILibLinkedList_GetNextNode(node);
	}
	ILibLinkedList_Destroy(o->receiveHoldBuffer);

	ILibWebRTC_DestroySparseArrayTables(o);

	// Free the session
	o->state = 0;
	sem_post(&(o->Lock));
	sem_destroy(&(o->Lock));

	ILibRemoteLogging_printf(ILibChainGetLogger(o->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Cleaning up Dtls Session Object for session: %d", o->sessionId);
	ILibLifeTime_Remove(o->parent->Timer, ILibWebRTC_DTLS_TO_TIMER_OBJECT(o));
}

void ILibStun_SctpDisconnect_Continue(void *j)
{
	char exBuffer[4096];
	int i;
	struct ILibStun_dTlsSession* o = (struct ILibStun_dTlsSession*)j - 5;
	struct ILibStun_Module* obj = o != NULL ? o->parent : NULL;

	// Now that we are on the microstack thread, we can continue the disconnet
	RCTPDEBUG(if (o != NULL) printf("ILibStun_SctpDisconnect state=%d\r\n", o->state);)

	if (o == NULL || o->state == 0) return;

	ILibWebRTC_CloseDataChannel_ALL(o);

	sem_wait(&(o->Lock));
	if (o->state < 1 || o->state > 2) { sem_post(&(o->Lock)); return; }

	// Send the event
	if (o->state == 2 && obj->OnConnect != NULL)
	{
		o->state = 3;
		sem_post(&(o->Lock));
		obj->OnConnect(obj, o, 0);
		sem_wait(&(o->Lock));
	}

	// Rather then sending an Abort/Shutdown, I changed this to just send an SSL/Close, which is what Chrome/Firefox do

	// Send an abort & shutdown
	//ILibStun_AddSctpChunkHeader(rpacket, 12, RCTP_CHUNK_TYPE_ABORT, 0, 4);
	//ILibStun_AddSctpChunkHeader(rpacket, 16, RCTP_CHUNK_TYPE_SHUTDOWN, 0, 8);
	//((unsigned int*)(rpacket + 20))[0] = htonl(obj->dTlsSessions[session]->intsn);
	//ILibStun_SendSctpPacket(obj, session, rpacket, 24);
	//ILibStun_SendSctpPacket(obj, session, rpacket, 24);
	//ILibStun_SendSctpPacket(obj, session, rpacket, 24);

	if (o->rpacketptr > 0)
	{
		ILibRemoteLogging_printf(ILibChainGetLogger(o->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP(%d) trying to close, flushing %d bytes from RPACKET", o->sessionId, o->rpacketptr);
		ILibStun_SendSctpPacket(obj, o->sessionId, o->rpacket, o->rpacketptr);
	}

	if (obj->IceStates[o->iceStateSlot] != NULL)
	{
		SSL_shutdown(o->ssl);

		while (BIO_ctrl_pending(SSL_get_wbio(o->ssl)) > 0)
		{
			// Data is pending in the write buffer, send it out
			i = BIO_read(SSL_get_wbio(o->ssl), exBuffer, 4096);
			if (obj->IceStates[o->iceStateSlot]->useTurn != 0)
			{
				ILibTURN_SendChannelData(obj->mTurnClientModule, (unsigned short)(o->sessionId), exBuffer, 0, i);
			}
			else
			{
				ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)obj)->UDP, (struct sockaddr*)&(o->remoteInterface), exBuffer, i, ILibAsyncSocket_MemoryOwnership_USER);
			}
		}
	}

	// Lets abort Consent-Freshness Checks
	ILibLifeTime_Remove(obj->Timer, o + 1);

	// Remove the SCTP Heartbeat timer
	ILibLifeTime_Remove(o->parent->Timer, o);

	// Start by clearing the IceState Object
	ILibStun_ClearIceState(obj, o->iceStateSlot);

	// Follow by clearing the DTLS object
	obj->dTlsSessions[o->sessionId] = NULL;

	ILibStun_SctpDisconnect_Final(o); // sem_post(&(o->Lock)) done inside this method.

	free(o);

	//
	// Note:
	//
	// Klocwork will report a spurious error here that o->Lock has been locked in line xxx was not unlocked
	// This is not an issue, because it is actually unlocked and destroyed within ILibStun_SctpDisconnect_Final(), which
	// will get called above...
	//
	// If we try to unlock here to satisfy Klocwork, you'll just end up corrupting the heap, because 
	// the lock will already have been destroyed before the end of this method.
}

void ILibStun_SctpDisconnect(struct ILibStun_Module *obj, int session)
{
	struct ILibStun_dTlsSession* o = obj->dTlsSessions[session];

	ILibLifeTime_Remove(o->parent->Timer, o); // Stop SCTP Heartbeats

	ILibRemoteLogging_printf(ILibChainGetLogger(o->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Disconnect Requested on Session: %d", session);

	if (ILibIsRunningOnChainThread(obj->Chain) == 0)
	{
		// Need to context switch to MicrostackChain thread
		ILibLifeTime_Add(obj->Timer, o + 5, 0, &ILibStun_SctpDisconnect_Continue, NULL);
	}
	else
	{
		// We are already running on the MicrostackChain thread
		ILibStun_SctpDisconnect_Continue(o + 5);
	}
}

void ILibSCTP_Close(void* module)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)module;
	if (obj != NULL) { ILibStun_SctpDisconnect(obj->parent, obj->sessionId); }
}

void ILibStun_SctpResent(struct ILibStun_dTlsSession *obj)
{
	unsigned int time = (unsigned int)ILibGetUptime();
	char* packet = obj->pendingQueueHead;

	// If T3RTXTIME == 0, it is disabled, otherwise it is the timestamp of when it was enabled
	if (obj->T3RTXTIME > 0 && time >= (obj->T3RTXTIME + obj->RTO) && packet != NULL)
	{
		obj->T3RTXTIME = 0;
#ifdef _WEBRTCDEBUG
		if (obj->onT3RTX != NULL) { obj->onT3RTX(obj, "OnT3RTX", -1); }	// Debug event informing of the T3RTX timer expiration
#endif

		obj->senderCredits = ILibRUDP_StartMTU;												// Set CWND to 1 MTU
		obj->RTO = obj->RTO * 2;															// Double the RT Timer
		if (obj->RTO > RTO_MAX) { obj->RTO = RTO_MAX; }										// Enforce a cap on the max timeout
		obj->SSTHRESH = MAX(obj->congestionWindowSize / 2, 4 * ILibRUDP_StartMTU);			// Update Slow Start Threshold
		obj->congestionWindowSize = ILibRUDP_StartMTU;										// Reset the size of the Congestion Window, so that we'll initialy only have one SCTP packet in flight
#ifdef _WEBRTCDEBUG
		if (obj->onCongestionWindowSizeChanged != NULL) { obj->onCongestionWindowSizeChanged(obj, "OnCongestionWindowSizeChanged", obj->congestionWindowSize); }
#endif

		while (packet != NULL)
		{
			if (((unsigned char*)(packet + sizeof(char*) + 2))[1] != 0xFE)					// 0xFE means this packet was already ACK'ed with a GAP Ack Block, so we don't need to retransmit it
			{
				if (obj->senderCredits >= ((unsigned short*)(packet + sizeof(char*)))[0])	// If we have available CWND, then retransmit packets
				{
					((unsigned char*)(packet + sizeof(char*) + 2))[0]++;					// Update retry counter
					((unsigned char*)(packet + sizeof(char*) + 2))[1] = 0;					// Update gap counter, so that it will not FastRetry
					((unsigned int*)(packet + sizeof(char*)))[1] = time;					// Update last send time
					obj->lastRetransmitTime = time;											// Anytime we retransmit a packet, we set a timestamp, for RTT calculation purposes

					obj->senderCredits -= ((unsigned short*)(packet + sizeof(char*)))[0];	// Deduct CWND
					if (obj->T3RTXTIME == 0)
					{
						// Only set the T3-RTX timer if it's not already running
						obj->T3RTXTIME = time;
#ifdef _WEBRTCDEBUG
						if (obj->onT3RTX != NULL){ obj->onT3RTX(obj, "OnT3RTX", obj->RTO); }
#endif
					}

					ILibStun_SendSctpPacket(obj->parent, obj->sessionId, packet + sizeof(char*) + 8, ((unsigned short*)(packet + sizeof(char*)))[0]);
#ifdef _WEBRTCDEBUG
					if (obj->onSendRetry != NULL) { obj->onSendRetry(obj, "OnSendRetry", ((unsigned short*)(packet + sizeof(char*)))[0]); }
#endif
				}
				else
				{
					((unsigned char*)(packet + sizeof(char*) + 2))[1] = 0xFF;			// Mark for retransmit later (when CWND allows)
				}
			}
			packet = ((char**)packet)[0];												// Move to the next packet
		}
	}
}

void ILibStun_SctpOnTimeout(void *object)
{
	struct ILibStun_dTlsSession *obj = (struct ILibStun_dTlsSession*)object;
	sem_wait(&(obj->Lock));
	// printf("Timer state=%d, value=%d\r\n", obj->state, obj->timervalue);
	if (obj->state < 1 || obj->state > 2) { sem_post(&(obj->Lock)); return; } // Check if we are still needed, connecting or connected state only.
	obj->timervalue++;
	if (obj->timervalue < 80)
	{
		// Resent packets
		if (obj->state == 2) ILibStun_SctpResent(obj);
	}
	else
	{
		// Close the connection
		sem_post(&(obj->Lock));
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP Timeout on Session: %d", obj->sessionId);
		ILibStun_SctpDisconnect(obj->parent, obj->sessionId);
		return;
	}

	if (obj->timervalue >= 40 && obj->state == 2)
	{
		// Send Heartbeat
		char hb[16];
		ILibStun_AddSctpChunkHeader(hb, 12, RCTP_CHUNK_TYPE_HEARTBEAT, 0, 4);
		ILibStun_SendSctpPacket(obj->parent, obj->sessionId, hb, 16);
	}

	ILibLifeTime_AddEx(obj->parent->Timer, obj, 100 + (200 * obj->timervalue), &ILibStun_SctpOnTimeout, NULL);
	sem_post(&(obj->Lock));
}

int ILibSCTP_AddOptionalVariableParameter(char* insertionPoint, unsigned short parameterType, void *parameterData, int parameterDataLen)
{
	int retValue;
	((unsigned short*)insertionPoint)[0] = parameterType;
	((unsigned short*)insertionPoint)[1] = htons((unsigned short)(4 + parameterDataLen));
	if(parameterDataLen>0)
	{
		memcpy(insertionPoint + 4, parameterData, parameterDataLen);
	}
	parameterDataLen += 4;
	retValue = ILibAlignOnFourByteBoundary(insertionPoint, parameterDataLen);
	return(retValue);
}

void ILibWebRTC_PropagateChannelCloseEx2(ILibSparseArray sender, int index, void *value, void *user)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)user;

	UNREFERENCED_PARAMETER(sender);

	if ((((ILibSCTP_StreamAttributes*)(char*)&value)->Data.StatusFlags & ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED) == ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED)
	{
		if (obj->parent->OnWebRTCDataChannelClosed != NULL) { obj->parent->OnWebRTCDataChannelClosed(obj->parent, obj, (unsigned short)index); }
	}
}
void ILibWebRTC_PropagateChannelCloseEx(ILibSparseArray sender, struct ILibStun_dTlsSession* obj)
{
	ILibSparseArray_DestroyEx(sender, &ILibWebRTC_PropagateChannelCloseEx2, obj);
}
ILibSparseArray ILibWebRTC_PropagateChannelClose(struct ILibStun_dTlsSession* obj, char* packet)
{
	ILibSCTP_ReconfigChunk* outChunk = (ILibSCTP_ReconfigChunk*)(packet + 12);
	ILibSCTP_Reconfig_Parameter_Header *hdr;
	ILibSparseArray retVal = NULL;
	unsigned short len = ntohs(outChunk->chunkLength);
	int bytesProcessed = 0;

	if (packet == NULL || ((ILibSCTP_PendingTSN_Data*)((char*)&packet))->Data.Flags == 0xFF) { return NULL; }
	

	while((bytesProcessed + 4) < len)
	{
		hdr = (ILibSCTP_Reconfig_Parameter_Header*)((char*)&outChunk->reconfigurationParameter + bytesProcessed);
		if(hdr->parameterType == ntohs(SCTP_RECONFIG_TYPE_OUTGOING_SSN_RESET_REQUEST))
		{
			ILibSCTP_Reconfig_OutgoingSSNResetRequest *req = (ILibSCTP_Reconfig_OutgoingSSNResetRequest*)hdr;
			unsigned short count = (ntohs(req->parameterLength) - 16) / 2;
			unsigned short sid;

			if(count==0)
			{
				ILibSparseArray_ClearEx(obj->DataChannelMetaDetaValues, NULL, NULL);
				retVal = ILibSparseArray_Move(obj->DataChannelMetaDeta);
				break;
			}
			else
			{
				retVal = ILibSparseArray_CreateEx(obj->DataChannelMetaDeta);
				while(count > 0)
				{
					sid = ntohs(req->Streams[count-1]);
					ILibSparseArray_Add(retVal, sid, ILibSparseArray_Remove(obj->DataChannelMetaDeta, sid)); 
					ILibSparseArray_Remove(obj->DataChannelMetaDetaValues, sid);
					--count;
				}
				break;
			}
		}
		bytesProcessed += FOURBYTEBOUNDARY(ntohs(hdr->parameterLength));
	}

	return(retVal);
}

int ILibSCTP_AddPacketToHoldingQueue_Comparer(void *obj1, void *obj2)
{
	unsigned int v2 = ntohl(((ILibSCTP_DataPayload*)obj2)->TSN);
	unsigned int v1 = ntohl(((ILibSCTP_DataPayload*)obj1)->TSN);

	return(v2 == v1 ? 0 : ((v2 < v1)?-1:1));
}
void* ILibSCTP_AddPacketToHoldingQueue_Chooser(void *oldValue, void *newValue, void *user)
{
	if(oldValue!=NULL)
	{
		return(oldValue);
	}
	else
	{
		unsigned short len = ntohs(((ILibSCTP_DataPayload*)newValue)->length);
		char* retVal = (char*)malloc(len+1);
		memcpy(retVal, (char*)newValue, len);
		ReceiveHoldBuffer_Increment((ILibLinkedList)user, ((ILibSCTP_DataPayload*)newValue)->length);
		return(retVal);
	}
}
ILibSCTP_SackStatus ILibSCTP_AddPacketToHoldingQueue(struct ILibStun_dTlsSession* o, ILibSCTP_DataPayload *payload, int sentsack)
{
	// Out of sequence packet, find a spot in the receive queue.

	RCTPRCVDEBUG(printf("STORING %u, size = %d\r\n", tsn, chunksize);)
	if (ReceiveHoldBuffer_Used(o->receiveHoldBuffer) + payload->length > ILibSCTP_MaxReceiverCredits) {return(ILibSCTP_SackStatus_Skip);}

	ILibLinkedList_SortedInsertEx(o->receiveHoldBuffer, &ILibSCTP_AddPacketToHoldingQueue_Comparer, &ILibSCTP_AddPacketToHoldingQueue_Chooser, payload, o->receiveHoldBuffer);
	
	// Send ACK now
	if (sentsack == ILibSCTP_SackStatus_NotSent)
	{
		sentsack = ILibSCTP_SackStatus_Sent;
		o->rpacketptr = ILibStun_SctpAddSackChunk(o->parent, o->sessionId, o->rpacket, o->rpacketptr); // Send the ACK with the TSN as far forward as we can
	}
	return(sentsack);
}

// Main RFC specification: http://tools.ietf.org/html/rfc4960
void ILibStun_ProcessSctpPacket(struct ILibStun_Module *obj, int session, char* buffer, int bufferLength)
{
	ILibSCTP_ChunkHeader *chunkHdr;
	char* rpacket = NULL;
	unsigned int crc;
	int ptr = 12, stop = 0;
	int* rptr = NULL;
	struct ILibStun_dTlsSession* o = obj->dTlsSessions[session];
	int rttCalculated = 0;
	ILibSCTP_SackStatus sentsack = ILibSCTP_SackStatus_NotSent;

	// Setup the session
	if (bufferLength < 12 || o == NULL) return;

#ifdef _WEBRTCDEBUG
	// Simulated Inbound Packet Loss
	if ((obj->lossPercentage) > 0 && ((rand() % 100) >= (100 - obj->lossPercentage))) { return; }
#endif

	// Check size and the RCTP (RFC4960) checksum using CRC32c
	crc = ((unsigned int*)buffer)[2];
	((unsigned int*)buffer)[2] = 0;
	if (crc != crc32c(0, buffer, (unsigned int)bufferLength)) return;
	sem_wait(&(o->Lock));

	// Decode the rest of the header
	if (o->tag != ((unsigned int*)buffer)[1] && ((unsigned int*)buffer)[1] != 0) { sem_post(&(o->Lock)); return; } // Check the verification tag

	// Setup the response packet
	rptr = &(o->rpacketptr);
	rpacket = o->rpacket;
	*rptr = 12;

	// printf("SCTP size: %d\r\n", bufferLength);

	// Decode each chunk
	
	while ((chunkHdr = (ILibSCTP_ChunkHeader*)(buffer + ptr)) != NULL && (ptr + 4 <= bufferLength) && stop == 0)
	{
		unsigned char chunktype = chunkHdr->chunkType;
		unsigned char chunkflags = chunkHdr->chunkFlags;
		unsigned short chunksize = ntohs(chunkHdr->chunkLength);
		if (chunksize < 4 || ptr + chunksize > bufferLength) break;

		switch (chunktype) {
		case RCTP_CHUNK_TYPE_RECONFIG:
			{
				ILibSCTP_StreamAttributes attr;
				ILibSCTP_ReconfigChunk *recon = (ILibSCTP_ReconfigChunk*)(buffer + ptr);
				ILibSCTP_Reconfig_Parameter_Header *hdr;
				int bytesProcessed = 0;
				unsigned short chunkLen = ntohs(recon->chunkLength);
				unsigned short streamId;
				ILibSCTP_ReconfigChunk *outChunk = (ILibSCTP_ReconfigChunk*)(rpacket + *rptr);								
				unsigned short outOffset = 0;
				unsigned short hdrLen;
				ILibSCTP_Reconfig_OutgoingSSNResetRequest *outRequest = NULL;

				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d received [RECONFIG]", session);

				while((bytesProcessed + 4 )< chunkLen)
				{
					hdr = (ILibSCTP_Reconfig_Parameter_Header*)((char*)&recon->reconfigurationParameter + bytesProcessed);
					hdrLen = ntohs(hdr->parameterLength);

					switch(ntohs(hdr->parameterType))
					{
						case SCTP_RECONFIG_TYPE_OUTGOING_SSN_RESET_REQUEST: // Outgoing SSN Reset
							{
								ILibSCTP_Reconfig_Response *OutboundResponse = (ILibSCTP_Reconfig_Response*)((char*)(&outChunk->reconfigurationParameter) + outOffset); // Ignore Klocwork Error, it's not a problem in this case
								ILibSCTP_Reconfig_OutgoingSSNResetRequest *req = (ILibSCTP_Reconfig_OutgoingSSNResetRequest*)hdr;
								unsigned short streamCount = (ntohs(req->parameterLength) - 16) / 2;								
								unsigned int lastTSN = ntohl(req->LastTSN);

								OutboundResponse->parameterType = htons(SCTP_RECONFIG_TYPE_RECONFIGURATION_RESPONSE);
								OutboundResponse->parameterLength = htons(sizeof(ILibSCTP_Reconfig_Response));
								OutboundResponse->RESSEQNum = req->RReqSeqNum;								
								outOffset += sizeof(ILibSCTP_Reconfig_Response);

								if(outRequest!=NULL)
								{
									// ERROR: This should not happen unless the RECONFIG chunk was malformed
									OutboundResponse->Result = htonl((unsigned int)ILibSCTP_Reconfig_Result_Denied); 
									break;
								}
								
								outRequest = (ILibSCTP_Reconfig_OutgoingSSNResetRequest*)((char*)(&outChunk->reconfigurationParameter) + outOffset);
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Outgoing Reset Request: Seq/%u", (unsigned int)ntohl(req->RReqSeqNum));

								if(lastTSN <= o->userTSN)
								{
									// Response to the OUTGOING_RESET_REQUEST with a SUCCESS/RESPONSE and an OUTBOUND_RESET_REQUEST
									OutboundResponse->Result = htonl((unsigned int)ILibSCTP_Reconfig_Result_Success_Performed); 

									outRequest->parameterType = htons(SCTP_RECONFIG_TYPE_OUTGOING_SSN_RESET_REQUEST);
									outRequest->parameterLength = htons(16 + 2*streamCount);
									outRequest->LastTSN = htonl(o->outtsn - 1);
									outRequest->RReqSeqNum = htonl(o->RREQSEQ++);
									outRequest->RResSeqNum = htonl(o->RRESSEQ++);	
									outOffset += (16 + 2*streamCount);

									if(streamCount == 0)
									{
										// Reset All Streams
										// We're going to move the contents to a new SparseArray, so we can post the events without a lock

										ILibSparseArray dup = ILibSparseArray_Move(o->DataChannelMetaDeta);
										ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Resetting all streams");

										sem_post(&(o->Lock));
											ILibWebRTC_PropagateChannelCloseEx(dup, o);
										sem_wait(&(o->Lock));
									}
									else
									{
										ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Number of streams to be reset: %u", streamCount);
										while(streamCount > 0)
										{
											streamId = ntohs(req->Streams[streamCount-1]);
											attr.Raw = ILibSparseArray_Remove(o->DataChannelMetaDeta, streamId);

											ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, ".........Resetting stream: %u", streamId);

											if((attr.Data.StatusFlags & ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED) == ILibSCTP_StreamAttributesData_Assigned_Status_ASSIGNED)
											{
												OutboundResponse->Result = htonl(ILibSCTP_Reconfig_Result_Success_Performed);	
												ILibSparseArray_Remove(o->DataChannelMetaDetaValues, streamId); // Clear associated data with this stream ID
												if(obj->OnWebRTCDataChannelClosed != NULL)
												{
													sem_post(&(o->Lock));
													obj->OnWebRTCDataChannelClosed(obj, o, streamId);
													sem_wait(&(o->Lock));
												}
											}
											else
											{
												OutboundResponse->Result = htonl(ILibSCTP_Reconfig_Result_Success_NOP);	
											}
																					
											outRequest->Streams[streamCount-1] = htons(streamId);
											--streamCount;
										}
									}
								}
								else
								{
									// We have not received all data for the stream yet, so we need to defer the reset
									ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......LastTSN/%u has not elapsed, DEFERRING", lastTSN);
									OutboundResponse->Result = htonl((unsigned int)ILibSCTP_Reconfig_Result_In_Progress); 

									if(o->pendingReconfigPacket !=NULL && o->pendingReconfigPacket >= o->rpacket && o->pendingReconfigPacket <= (o->rpacket + o->rpacketsize))
									{
										// Locally initiated Channel Close is pending. Deny request, becuase we don't have resources to process this yet
										OutboundResponse->Result = htonl(ILibSCTP_Reconfig_Result_Denied);
										ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Insufficient resources to process Close, due to locally initiated close in progress");									
									}
									else 
									{
										ILibSCTP_PendingTSN_Data pending;
										int i;

										// We're going to use this field, which already contains malloc'ed memory, to store some data at the tail end									
										if(o->pendingReconfigPacket != NULL)
										{
											// ALREADY IN PROGRESS
											OutboundResponse->Result = htonl((unsigned int)ILibSCTP_Reconfig_Result_In_Progress); 
										}
										else
										{
											// Continue with deferring the request
											pending.Raw = o->pendingReconfigPacket;
											pending.Data.Type = 0xFF;
											pending.Data.Flags = 0x00;
											pending.Data.StreamIdOffset = (4 + (2*streamCount));

											// We are going to store the TSN followed by the StreamIds 
											((int*)(o->rpacket + o->rpacketsize - pending.Data.StreamIdOffset))[0] = lastTSN;
											for(i = 0; i < streamCount; ++i)
											{
												((unsigned short*)(o->rpacket + o->rpacketsize - pending.Data.StreamIdOffset))[i+2] = ntohs(req->Streams[i]);
											}
											o->pendingReconfigPacket = (char*)pending.Raw;		
										}
									}	
								}								
							}							
							break;
						case SCTP_RECONFIG_TYPE_INCOMING_SSN_RESET_REQUEST: // Incoming Reset Request
							{
								ILibSCTP_Reconfig_IncomingSSNResetRequest *req = (ILibSCTP_Reconfig_IncomingSSNResetRequest*)hdr;
								unsigned short streamCount = (ntohs(req->parameterLength) - 8) / 2;
								ILibSCTP_PendingTSN_Data pending;

								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Incoming Reset Request: Seq/%u", (unsigned int)ntohl(req->RReqSeqNum));
								pending.Raw = o->pendingReconfigPacket;

								if(outRequest != NULL)
								{
									// We already received an Outbound Reset Request, which would have triggered us to generate our own Outbound Request.
									// That means we can just reply with an Response Parameter, and call it a day
									ILibSCTP_Reconfig_Response *OutboundResponse = (ILibSCTP_Reconfig_Response*)((char*)(&outChunk->reconfigurationParameter) + outOffset); // Ignore Klocwork Error, it's not a problem in this case
									OutboundResponse->parameterType = htons(SCTP_RECONFIG_TYPE_RECONFIGURATION_RESPONSE);
									OutboundResponse->parameterLength = htons(sizeof(ILibSCTP_Reconfig_Response));
									OutboundResponse->RESSEQNum = req->RReqSeqNum;	
									OutboundResponse->Result = htonl((unsigned int)(pending.Data.Type == 0xFF && pending.Data.Flags == 0x00)?ILibSCTP_Reconfig_Result_In_Progress:ILibSCTP_Reconfig_Result_Success_NOP);									
									outOffset += sizeof(ILibSCTP_Reconfig_Response);
									ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Responded with Response Parameter");
									break;
								}
	
								// Lone INBOUND RESET REQUEST
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Responding with Outbound Reset Request");
								outRequest = (ILibSCTP_Reconfig_OutgoingSSNResetRequest*)((char*)(&outChunk->reconfigurationParameter) + outOffset); // Ignore Klocwork Error, it's not a problem in this case												
								outRequest->parameterType = htons(SCTP_RECONFIG_TYPE_OUTGOING_SSN_RESET_REQUEST);
								outRequest->parameterLength = 16 + 2*streamCount;
								outRequest->LastTSN = htonl(o->outtsn - 1);
								outRequest->RReqSeqNum = htonl(o->RREQSEQ++);
								outRequest->RResSeqNum = req->RReqSeqNum;

								outOffset += FOURBYTEBOUNDARY(outRequest->parameterLength);			 // Add parameter size and padding
								outRequest->parameterLength = htons(outRequest->parameterLength);
								
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, ".........Number of streams requested to be reset: %u", streamCount);								

								while(streamCount > 0)
								{
									streamId = ntohs(req->Streams[streamCount-1]);
									outRequest->Streams[streamCount-1] = htons(streamId);
									--streamCount;
									ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "............Resetting Stream id: %u", streamId);
								}	
							}
							break;
						case SCTP_RECONFIG_TYPE_RECONFIGURATION_RESPONSE: // Response
							{
								ILibSCTP_Reconfig_Response *res = (ILibSCTP_Reconfig_Response*)hdr;
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Received Response [%u/%s]: Seq/%u",(unsigned int)ntohl(res->Result), ((unsigned int)ntohl(res->Result)==0 || (unsigned int)ntohl(res->Result)==1 || (unsigned int)ntohl(res->Result)==4 || (unsigned int)ntohl(res->Result)==6)?"SUCCESS":"ERROR", (unsigned int)ntohl(res->RESSEQNum));
								
								if(o->pendingReconfigPacket!=NULL && ((ILibSCTP_PendingTSN_Data*)((char*)&o->pendingReconfigPacket))->Data.Type != 0xFF)
								{ 
									if((o->RREQSEQ -1 ) == ntohl(res->RESSEQNum))
									{
										// Both sides of the connection are aware of the ChannelClose... We can propagate this up.										
										ILibSparseArray arr = ILibWebRTC_PropagateChannelClose(o, o->pendingReconfigPacket);
										
										ILibLifeTime_Remove(o->parent->Timer, ILibWebRTC_DTLS_TO_TIMER_OBJECT(o));
										ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "......[Response for a pending Close]");
										o->pendingReconfigPacket = NULL;
										
										// We need to shed the Lock before we call up the stack
										sem_post(&(o->Lock));
											ILibWebRTC_PropagateChannelCloseEx(arr, o);
										sem_wait(&(o->Lock));									
									}
								}
							}
							break;
						default:
							break;
					}
					bytesProcessed += FOURBYTEBOUNDARY(hdrLen);		// Add parameter size and padding
				}

				if(outOffset != 0)
				{
					outChunk->type = RCTP_CHUNK_TYPE_RECONFIG;
					outChunk->chunkFlags = 0x00;
					outChunk->chunkLength = htons(4 + outOffset);
					*rptr += (4 + outOffset);
				}
			}			
			break;
		case RCTP_CHUNK_TYPE_COOKIEACK:
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d received [COOKIEACK]", session);
			if (obj->OnConnect != NULL && o->state == 1)
			{
				o->state = 2;
				sem_post(&(o->Lock));
				obj->OnConnect(obj, o, 1);
				if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state == 0) return;
				sem_wait(&(o->Lock));
			}
		}
		case RCTP_CHUNK_TYPE_INITACK:
		{
			int i;
			if (chunksize < 20 || session == -1 || o->state != 1) break;

			// Set TSN
			o->RRESSEQ = o->userTSN = o->intsn = ntohl(((unsigned int*)(buffer + ptr + 16))[0]) - 1;

			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d received [INIT-ACK]", session);

			// We need to send a RCTP_CHUNK_TYPE_COOKIEECHO response
			i = 20;
			while (i < chunksize)
			{
				if (ntohs(((unsigned short*)(buffer + ptr + i))[0]) == 7)
				{
					// Cookie
					*rptr = ILibStun_AddSctpChunkHeader(rpacket, *rptr, RCTP_CHUNK_TYPE_COOKIEECHO, 0, ntohs(((unsigned short*)(buffer + ptr + i))[1]));
					memcpy(rpacket + *rptr, buffer + ptr + i + 4, ntohs(((unsigned short*)(buffer + ptr + i))[1]) - 4);
					*rptr += (ntohs(((unsigned short*)(buffer + ptr + i))[1]) - 4);
					break;
				}
				else
				{
					i += (int)ntohs(((unsigned short*)(buffer + ptr + i))[1]);
				}
			}
		}
			break;
		case RCTP_CHUNK_TYPE_INIT:
			if (chunksize < 20 || session == -1 || o->state != 1) break;
			o->sessionId = session;
			o->tag = ((unsigned int*)(buffer + ptr + 4))[0];
			if (o->tag == 0) { sem_post(&(o->Lock)); return; } // This tag can't be zeroes
			o->receiverCredits = ntohl(((unsigned int*)(buffer + ptr + 8))[0]);
#if ILibSCTP_MaxSenderCredits > 0
			if (o->receiverCredits > ILibSCTP_MaxSenderCredits) o->receiverCredits = ILibSCTP_MaxSenderCredits; // Since we do real-time KVM, reduce the buffering.
#endif
			o->userTSN = o->intsn = ntohl(((unsigned int*)(buffer + ptr + 16))[0]) - 1;
			util_random(4, (char*)&(o->outtsn));
			o->RREQSEQ = o->outtsn;
			o->RRESSEQ = o->intsn;
			o->inport = ntohs(((unsigned short*)buffer)[1]);
			o->outport = ntohs(((unsigned short*)buffer)[0]);
			o->maxOutStreams = MIN(ntohs(((unsigned short*)(buffer + ptr + 12))[0]), ILibSCTP_Stream_MaximumCount);
			o->maxInStreams = MIN(ntohs(((unsigned short*)(buffer + ptr + 12))[1]), ILibSCTP_Stream_MaximumCount);
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d received [INIT]", session);

			// Optional/Variable Fields
			{
				int varLen = 0;
				unsigned short tLen = 0;
				int chunkIndex;

				while(chunksize - varLen > 20)
				{				
					SCTP_INIT_PARAMS optionalParam = (SCTP_INIT_PARAMS)ntohs(((unsigned short*)(buffer+ptr+20 + varLen))[0]);
					tLen = ntohs(((unsigned short*)(buffer+ptr+20+varLen))[1]);					
					switch(optionalParam)
					{
						case SCTP_INIT_PARAM_UNRELIABLE_STREAM:
							ILibSparseArray_Add(o->PeerFeatureSet, SCTP_INIT_PARAM_UNRELIABLE_STREAM, (void*)0x01);
							break;
						case SCTP_INIT_PARAM_SUPPORTED_EXTENSIONS:
							for(chunkIndex = 0 ; chunkIndex < tLen - 4; ++chunkIndex)
							{
								ILibSparseArray_Add(o->PeerFeatureSet, (int)((unsigned char*)(buffer + ptr + 20 + varLen + 4))[chunkIndex], (void*)0x01);
							}
							break;
						default:
							break;
					}
					varLen += FOURBYTEBOUNDARY(tLen);	// Add parameter size and padding
				}
			}

#ifdef _WEBRTCDEBUG
			// Debug Events
			if (o->onReceiverCredits != NULL) { o->onReceiverCredits(o, "OnReceiverCredits", o->receiverCredits); }
#endif

			RCTPDEBUG(printf("RCTP_CHUNK_TYPE_INIT, Flags=%d, outTSN=%u, inTSN=%u\r\n", chunkflags, obj->dTlsSessions[session]->outtsn, obj->dTlsSessions[session]->intsn);)

			// Create response
			{
				long long uptime = ILibGetUptime();
				char chunks[1] = {130};
				ILibStun_AddSctpChunkHeader(rpacket, *rptr, RCTP_CHUNK_TYPE_INITACK, 0, 37);
				*rptr += 4;
				((ILibSCTP_InitAckChunk*)(rpacket + *rptr))->InitiateTag = o->tag;									// Initiate Tag
				((ILibSCTP_InitAckChunk*)(rpacket + *rptr))->A_RWND = htonl(ILibSCTP_MaxReceiverCredits);			// Advertised Receiver Window Credit (a_rwnd)	
				((ILibSCTP_InitAckChunk*)(rpacket + *rptr))->NumberOfOutboundStreams = htons(o->maxOutStreams);		// Number of Outbound Streams
				((ILibSCTP_InitAckChunk*)(rpacket + *rptr))->NumberOfInboundStreams = htons(o->maxInStreams);		// Number of Inbound Streams
				((ILibSCTP_InitAckChunk*)(rpacket + *rptr))->InitialTSN = htonl(o->outtsn);							// Initial TSN
				*rptr += sizeof(ILibSCTP_InitAckChunk);
				*rptr += ILibSCTP_AddOptionalVariableParameter(rpacket + *rptr, htons(7), (void*)&uptime, sizeof(uptime)); // Stick uptime as cookie, so we can calculate initial RTT
				*rptr += ILibSCTP_AddOptionalVariableParameter(rpacket + *rptr, htons(SCTP_INIT_PARAM_SUPPORTED_EXTENSIONS), chunks, 1); // Supports RE-CONFIG
			}
			break;
		case RCTP_CHUNK_TYPE_SACK:
		{
			int FastRetryDisabled = 0;
			int GapAckPtr = 0, DuplicatePtr = 0, oldHoldCount;
			unsigned int gstart, gend = 0;
			char* packet = NULL;
			unsigned int tsn = ntohl(((unsigned int*)(buffer + ptr + 4))[0]);
#ifdef _REMOTELOGGING
			unsigned int arwnd = ntohl(((unsigned int*)(buffer + ptr + 8))[0]);
#endif
			unsigned short GapAckCount = ntohs(((unsigned short*)(buffer + ptr + 12))[0]);
			unsigned short DuplicateCount = ntohs(((unsigned short*)(buffer + ptr + 12))[1]);
			unsigned int tsnx = 0;
			unsigned int lastTSNX = 0;
			int windowReset = 0;
			int cumulativeTSNAdvanced = 0;
			int pbc = o->pendingByteCount;

			o->zeroWindowProbeTime = 0;
			o->lastSackTime = (unsigned int)ILibGetUptime();

			if (o->FastRetransmitExitPoint != 0 && tsn >= o->FastRetransmitExitPoint)
			{
				// We have exited Fast Recovery Mode
				o->FastRetransmitExitPoint = 0;
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d] has exited Fast-Recovery Mode (Sender Credits: %d / Receiver Credits: %d)", o->sessionId, o->senderCredits, o->receiverCredits);
#ifdef _WEBRTCDEBUG
				if (o->onFastRecovery != NULL){ o->onFastRecovery(o, "OnFastRecovery", 0); }
#endif
			} 
#ifdef _WEBRTCDEBUG
			if (o->onSACKreceived != NULL) { o->onSACKreceived(o, "OnSACKReceived", tsn); }
#endif

			while (DuplicatePtr < DuplicateCount) { tsnx = ntohl(((unsigned int*)(buffer + ptr + 16 + (GapAckCount * 4) + (DuplicatePtr * 4)))[0]); ++DuplicatePtr; }

			// Clear all packets that are fully acknowledged
			if (o->pendingQueueHead != NULL) { tsnx = ntohl(((unsigned int*)(o->pendingQueueHead + sizeof(char*) + 8 + 12 + 4))[0]); }
			while (o->pendingQueueHead != NULL && tsnx <= tsn)
			{
				// If we haven't made an RTT calculation, let's try to
				if (((unsigned char*)(o->pendingQueueHead + sizeof(char*) + 2))[0] == 0 && rttCalculated == 0)
				{
					// But only if no packets were re-transmitted since the last time we sent a packet
					if (o->lastRetransmitTime < ((unsigned int*)(o->pendingQueueHead + sizeof(char*)))[1])
					{
						// This packet that was ACK'ed and was NOT retransmitted, and NO OTHER packets
						// were retransmitted since this packet was originally sent
						int r = (int)(o->lastSackTime - ((unsigned int*)(o->pendingQueueHead + sizeof(char*)))[1]);
						o->RTTVAR = (int)((double)(1 - RTO_BETA) * (double)o->RTTVAR + RTO_BETA * (double)abs(o->SRTT - r));
						o->SRTT = (int)((double)(1 - RTO_ALPHA) * (double)o->SRTT + RTO_ALPHA * (double)r);
						o->RTO = o->SRTT + 4 * o->RTTVAR;
						if (o->RTO < RTO_MIN) { o->RTO = RTO_MIN; }
						if (o->RTO > RTO_MAX) { o->RTO = RTO_MAX; }
						rttCalculated = 1; // We only need to calculate this once for each packet received

#ifdef _WEBRTCDEBUG
						if (o->onRTTCalculated != NULL) { o->onRTTCalculated(o, "OnRTTCalculated", o->SRTT); }
#endif
					}
				}
				cumulativeTSNAdvanced += ((((unsigned short*)(o->pendingQueueHead + sizeof(char*)))[0]) - (12 + 16));
				o->pendingByteCount -= ((((unsigned short*)(o->pendingQueueHead + sizeof(char*)))[0]) - (12 + 16));
				o->pendingCount--;
				packet = o->pendingQueueHead;
				o->pendingQueueHead = ((char**)(packet))[0];
				if (o->pendingQueueHead == NULL) { o->pendingQueueTail = NULL; }
				else { tsnx = ntohl(((unsigned int*)(o->pendingQueueHead + sizeof(char*) + 8 + 12 + 4))[0]); }

				free(packet);
				o->timervalue = 0; // This is a valid SACK, reset the timeout for packet resent				
			}

			if (cumulativeTSNAdvanced == 0)
			{
#ifdef _WEBRTCDEBUG
				if (o->onTSNFloorNotRaised != NULL) { o->onTSNFloorNotRaised(o, "OnTSNFloorNotRaised", o->pendingQueueHead != NULL ? (((unsigned char*)(o->pendingQueueHead + sizeof(char*) + 2))[0]) : -1); }
#endif
			}
			else
			{
				o->senderCredits += cumulativeTSNAdvanced; // These bytes are no longer in-flight
				o->receiverCredits += cumulativeTSNAdvanced;

				if (o->senderCredits > o->congestionWindowSize) { o->senderCredits = o->congestionWindowSize; }
#ifdef _WEBRTCDEBUG
				if (o->onReceiverCredits != NULL) { o->onReceiverCredits(o, "OnReceiverCredits", o->receiverCredits); }
#endif
			}

			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "...A_RWND: %u", arwnd);
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "...Sender Credits: %u", o->senderCredits);

			if (o->pendingQueueHead == NULL)
			{
				o->senderCredits = o->congestionWindowSize;
				o->PARTIAL_BYTES_ACKED = 0;
				if(o->T3RTXTIME!=0) {ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d]: T3TX Timer OFF", o->sessionId);}
				o->T3RTXTIME = 0;
#ifdef _WEBRTCDEBUG
				if (o->onT3RTX != NULL){ o->onT3RTX(o, "OnT3RTX", 0); } // All data has been ack'ed, so we can turn off the T3RTX timer
#endif
			}
			else
			{
				o->PARTIAL_BYTES_ACKED += cumulativeTSNAdvanced;
				if (cumulativeTSNAdvanced > 0)
				{
					o->T3RTXTIME = o->lastSackTime;
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d]: T3TX Timer Restarted", o->sessionId);
#ifdef _WEBRTCDEBUG
					if (o->onT3RTX != NULL){ o->onT3RTX(o, "OnT3RTX", o->RTO); } // The lowest TSN has been ACK'ed, and there is still data pending, so restart the timer
#endif
				}
			}
			if (o->congestionWindowSize <= o->SSTHRESH && o->FastRetransmitExitPoint == 0 && cumulativeTSNAdvanced != 0 && o->congestionWindowSize < ILibSCTP_MaxReceiverCredits)
			{
				// When our window is smaller than the Slow Start Threshold, we can only grow our Window by the MIN of the total bytes ACK'ed, or one MTU
				o->congestionWindowSize += MIN(cumulativeTSNAdvanced, ILibRUDP_StartMTU);
#ifdef _WEBRTCDEBUG
				if (o->onCongestionWindowSizeChanged != NULL) { o->onCongestionWindowSizeChanged(o, "OnCongestionWindowSizeChanged", o->congestionWindowSize); }
#endif
			}
			else if (o->congestionWindowSize > o->SSTHRESH && (int)(o->PARTIAL_BYTES_ACKED) >= o->congestionWindowSize && pbc >= o->congestionWindowSize)
			{
				// When our Congestion Window is greater than the Slow Start Threshold, we only grow our Window if we are fully utilizing our congestion window
				o->congestionWindowSize += ILibRUDP_StartMTU;
				o->PARTIAL_BYTES_ACKED = o->PARTIAL_BYTES_ACKED - o->congestionWindowSize;
			}

			//printf("ARK-STR %d GAPS, %d SENDS\r\n", GapAckCount, SendCount);

			// If there are any GAPS in the ACK, add to the FAST ACK counter.
			packet = o->pendingQueueHead;
			while (packet != NULL && GapAckPtr < GapAckCount)
			{
				gstart = tsn + ntohs(((unsigned short*)(buffer + ptr + 16 + (GapAckPtr * 4)))[0]);	// Start of GAP Block
				gend = tsn + ntohs(((unsigned short*)(buffer + ptr + 18 + (GapAckPtr * 4)))[0]);	// End of GAP Block
				tsnx = ntohl(((unsigned int*)(packet + sizeof(char*) + 8 + 12 + 4))[0]);			// TSN of Pending Packet
				while (packet != NULL && tsnx <= gend)
				{
					// Packet TSN could potentially be within the GAP Block
					unsigned char frt = ((unsigned char*)(packet + sizeof(char*) + 2))[1]; // Number of times the packet was indicated to be in a gap
					if (tsnx < gstart)
					{
						// This packet was not received by the peer
						if (frt < 0xFE)
						{
							++frt; // Increment the GAP Counter if this packet isn't already marked for re-transmit
							if (frt >= 0xFE) { frt = 0xFD; } // Cap the counter, because OxFE and 0xFF have special meaning, and won't retransmit
						}
						else if (frt == 0xFE)
						{
							// This packet was forward ACKed before, but now is not... Peer must have dropped it out of it's buffer... 
							frt = 1;
						}
					}
					else
					{
						// This packet was received by the peer
						if (frt != 0xFE)
						{
							// This is the first time this packet was GAP ACK'ed
							frt = 0xFE; // This packet was forward ACKed by the peer. Ban this packet from being retransmitted for now
							o->senderCredits += (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));
							o->PARTIAL_BYTES_ACKED += (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));
						}
					}

					if (frt >= ILibSCTP_FastRetry_GAP && frt < 0xFE)
					{
						// Perform fast retry. This only happens if this packet was in the ACK gap 'ILibSCTP_FastRetry_GAP' times.
						if (windowReset == 0 && o->FastRetransmitExitPoint == 0)
						{
							// This is the first time entering Fast Recovery Mode, so we need to shrink our congestion window
#ifdef _WEBRTCDEBUG
							if (o->onFastRecovery != NULL){ o->onFastRecovery(o, "OnFastRecovery", 1); }
#endif
							o->SSTHRESH = MAX((o->congestionWindowSize / 2), (4 * ILibRUDP_StartMTU));;
							o->congestionWindowSize = o->SSTHRESH;
							o->senderCredits = MIN(o->senderCredits, o->congestionWindowSize);
							o->PARTIAL_BYTES_ACKED = 0;
							windowReset = 1;
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d]: Entering Fast-Retry Mode (Sender Credits: %d)", o->sessionId, o->senderCredits);
#ifdef _WEBRTCDEBUG
							if (o->onCongestionWindowSizeChanged != NULL) { o->onCongestionWindowSizeChanged(o, "OnCongestionWindowSizeChanged", o->congestionWindowSize); }
#endif
						}


						frt = 0xFF; // Mark these packets for retransmit						
						lastTSNX = tsnx;

						if (FastRetryDisabled == 0)
						{
							if (packet == o->pendingQueueHead)
							{
								// We are re-transmitting the lowest outstanding TSN, restart the T3-RTX timer
								o->T3RTXTIME = o->lastSackTime;
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d]: Restarting T3RTX Timer (Retransmitting lowest TSN)", o->sessionId);
#ifdef _WEBRTCDEBUG
								if (o->onT3RTX != NULL){ o->onT3RTX(o, "OnT3RTX", o->RTO); }
#endif
							}
							else if (o->T3RTXTIME == 0)
							{
								// Since we are not re-transmitting the lowest outstanding TSN, only start the T3-RTX timer, if it's not running
								o->T3RTXTIME = o->lastSackTime;
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d]: Restarting T3RTX Timer (Not retransmitting lowest TSN)", o->sessionId);
#ifdef _WEBRTCDEBUG
								if (o->onT3RTX != NULL){ o->onT3RTX(o, "OnT3RTX", o->RTO); }
#endif
							}

							// Send the first FastRetransmit candidate right now, ignoring sender credits
							o->lastRetransmitTime = o->lastSackTime; // Every time we retransmit, we need to take note of it, for RTT purposes
							((unsigned int*)(packet + sizeof(char*)))[1] = o->lastSackTime;					// Update Send Time, used for retry
							ILibStun_SendSctpPacket(obj, session, packet + sizeof(char*) + 8, ((unsigned short*)(packet + sizeof(char*)))[0]);
							((unsigned char*)(packet + sizeof(char*) + 2))[0]++;							// Add to the packet resent counter
							//printf("RESEND COUNT %d, TSN=%u\r\n", ((unsigned char*)(packet + sizeof(char*) + 2))[0], tsnx);

							FastRetryDisabled = 1;
#ifdef _WEBRTCDEBUG
							if (o->onSendFastRetry != NULL) { o->onSendFastRetry(o, "OnSendFastRetry", ((unsigned short*)(packet + sizeof(char*)))[0]); }
#endif
						}
						else
						{
							// We already sent the first retransmit, 
							// But we can at least try to retransmit the rest when sender credits permit
							if (o->senderCredits >= ((unsigned short*)(packet + sizeof(char*)))[0])
							{
								// We have enough sender credits to retransmit these
								((unsigned int*)(packet + sizeof(char*)))[1] = o->lastSackTime;							// Update Send Time, used for retry
								if (o->T3RTXTIME == 0)
								{
									o->T3RTXTIME = o->lastSackTime;
#ifdef _WEBRTCDEBUG
									if (o->onT3RTX != NULL) { o->onT3RTX(o, "OnT3RTX", o->RTO); }
#endif
								}
								((unsigned char*)(packet + sizeof(char*) + 2))[0]++;									// Add to the packet resent counter
								o->senderCredits -= (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));		// Update sender credits
								o->lastRetransmitTime = o->lastSackTime; // Every time we retransmit, we need to take note of it, for RTT purposes

								ILibStun_SendSctpPacket(obj, session, packet + sizeof(char*) + 8, ((unsigned short*)(packet + sizeof(char*)))[0]);
#ifdef _WEBRTCDEBUG
								if (o->onSendRetry != NULL) { o->onSendRetry(obj, "OnSendRetry", ((unsigned short*)(packet + sizeof(char*)))[0]); }
#endif
							}
							else
							{
								// No Sender Credits available, so we're just going to mark these packets for retransmit later (frt = 0xFF)
								// (This block left blank on purpose, so we can include these comments)
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP[%d]: No sender credits available for retransmit", o->sessionId);
							}
						}
					}
					else if (frt == 0xFF && o->senderCredits >= ((unsigned short*)(packet + sizeof(char*)))[0])
					{
						// We have sufficient sender credits to send packets marked for retransmit
						((unsigned int*)(packet + sizeof(char*)))[1] = o->lastSackTime;					// Update Send Time, used for retry
						((unsigned char*)(packet + sizeof(char*) + 2))[0]++;							// Add to the packet resent counter
						o->senderCredits -= ((unsigned short*)(packet + sizeof(char*)))[0];				// Update sender credits
						o->lastRetransmitTime = o->lastSackTime; // Every time we retransmit, we need to take note of it, for RTT purposes

						ILibStun_SendSctpPacket(obj, session, packet + sizeof(char*) + 8, ((unsigned short*)(packet + sizeof(char*)))[0]);
#ifdef _WEBRTCDEBUG
						if (o->onSendRetry != NULL) { o->onSendRetry(obj, "OnSendRetry", ((unsigned short*)(packet + sizeof(char*)))[0]); }
#endif
					}
					((unsigned char*)(packet + sizeof(char*) + 2))[1] = frt; // Possibly add to the packet gap counter
					packet = ((char**)(packet))[0];
					if (packet != NULL) tsnx = ntohl(((unsigned int*)(packet + sizeof(char*) + 8 + 12 + 4))[0]);
				}
				GapAckPtr++;
			}

			if (lastTSNX != 0 && o->FastRetransmitExitPoint == 0) { o->FastRetransmitExitPoint = lastTSNX; }  // Set the Fast Recovery Exit Point

			// printf("ARK-END %d GAPS, %d SENDS\r\n", GapAckCount, SendCount);

			// RCTPDEBUG(printf("RCTP_CHUNK_TYPE_SACK, Size=%d, TSN=%u, RC1=%u, PQ=%d, HQ=%d\r\n", chunksize, tsn, obj->dTlsSessions[session]->receiverCredits, obj->dTlsSessions[session]->pendingCount, obj->dTlsSessions[session]->holdingCount);)

			// Send any packets in the holding queue that we can, we do this because we may now have more credits
			oldHoldCount = o->holdingCount;
			while (o->receiverCredits > 0 && o->holdingQueueHead != NULL)
			{
				packet = o->holdingQueueHead;

				// Check if we have sufficient credits to send the next packet
				if (o->receiverCredits < (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16))) break;
				if (o->senderCredits < (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16))) break;

				// Remove the packet from the holding queue
				o->holdingQueueHead = ((char**)(o->holdingQueueHead))[0];	// Move to the next packet
				if (o->holdingQueueHead == NULL) o->holdingQueueTail = NULL;
				o->holdingCount--;

#ifdef _WEBRTCDEBUG
				// Debug Events
				if (o->onHold != NULL) { o->onHold(o, "OnHold", o->holdingCount); }
#endif

				// Add the packet to the end of the pending queue
				if (o->pendingQueueTail == NULL) { o->pendingQueueHead = packet; }
				else { ((char**)(o->pendingQueueTail))[0] = packet; }
				((char**)packet)[0] = NULL;
				o->pendingQueueTail = packet;
				o->pendingCount++;
				o->pendingByteCount += (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));

				// Remove the credits
				o->senderCredits -= (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));
				o->receiverCredits -= (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));
				o->holdingByteCount -= (((unsigned short*)(packet + sizeof(char*)))[0] - (12 + 16));

#ifdef _WEBRTCDEBUG
				// Debug Events
				if (o->onReceiverCredits != NULL) { o->onReceiverCredits(o, "OnReceiverCredits", o->receiverCredits); }
#endif

				if (o->T3RTXTIME == 0)
				{
					o->T3RTXTIME = o->lastSackTime;
#ifdef _WEBRTCDEBUG
					if (o->onT3RTX != NULL){ o->onT3RTX(o, "OnT3RTX", o->RTO); } // Restart the timer, because the timer isn't currently set
#endif
				}

				// Update the packet retry data
				((unsigned int*)(packet + sizeof(char*)))[1] = o->lastSackTime;								// Last time the packet was sent (Used for retry)


				// Send the packet
				ILibStun_SendSctpPacket(obj, session, packet + sizeof(char*) + 8, ((unsigned short*)(packet + sizeof(char*)))[0]);	// Send the packet
			}

			// If we can now send more packets, notify the application
			if (obj->OnSendOK != NULL && o->holdingCount == 0 && oldHoldCount > 0)
			{
				sem_post(&(o->Lock));
				// printf("SendOK\r\n");
				obj->OnSendOK(obj, o, o->User);
				if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state != 2) return;
				sem_wait(&(o->Lock));
			}
		}
			break;
		case RCTP_CHUNK_TYPE_HEARTBEAT:
			// Echo back the heartbeat
			RCTPDEBUG(printf("RCTP_CHUNK_TYPE_HEARTBEAT, Size=%d\r\n", chunksize);)
			o->timervalue = 0;
			ILibStun_AddSctpChunkHeader(rpacket, *rptr, RCTP_CHUNK_TYPE_HEARTBEATACK, 0, chunksize);
			memcpy(rpacket + *rptr + 4, buffer + ptr + 4, chunksize - 4);
			*rptr += chunksize;
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP: %d Received [HEARTBEAT/%u]", session, ntohs(chunkHdr->chunkLength));
			break;
		case RCTP_CHUNK_TYPE_HEARTBEATACK:
			RCTPDEBUG(printf("RCTP_CHUNK_TYPE_HEARTBEATACK, Size=%d\r\n", chunksize);)
			o->timervalue = 0;
			break;
		case RCTP_CHUNK_TYPE_ABORT:
			{
				unsigned short len = ntohs(chunkHdr->chunkLength) - 4;
				unsigned short start = 0;
				sem_post(&(o->Lock));

				while(start < len)
				{
					ILibSCTP_ErrorCause_Header *cause = (ILibSCTP_ErrorCause_Header*)(chunkHdr->chunkData + start);
					unsigned short causeLen = FOURBYTEBOUNDARY(ntohs(cause->CauseLength));
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d Received [ABORT:%u/%s]", session, ntohs(cause->CauseCode), SCTP_ERROR_CAUSE_TO_STRING(cause));
					if(ntohs(cause->CauseCode)==SCTP_ERROR_CAUSE_CODE_PROTOCOL_VIOLATION)
					{
						ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...%s", cause->CauseInformation);
					}
					start += causeLen;
				}				
				ILibStun_SctpDisconnect(obj, session);
				return;
			}
		case RCTP_CHUNK_TYPE_SHUTDOWN:
		case RCTP_CHUNK_TYPE_SHUTDOWNACK:
		case RCTP_CHUNK_TYPE_ERROR:
			sem_post(&(o->Lock));
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d received [SHUTDOWN/ERROR] (%u)", session, chunktype);
			ILibStun_SctpDisconnect(obj, session);
			return;
		case RCTP_CHUNK_TYPE_COOKIEECHO:
			o->SRTT = (int)(ILibGetUptime() - *((long long*)(buffer + ptr + 4)));
			o->RTTVAR = o->SRTT / 2;
			o->RTO = o->SRTT + 4 * o->RTTVAR;
			o->SSTHRESH = 4 * ILibRUDP_StartMTU;
			if (o->RTO < RTO_MIN) { o->RTO = RTO_MIN; }
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d received [COOKIE-ECHO]", session);

			*rptr = ILibStun_AddSctpChunkHeader(rpacket, *rptr, RCTP_CHUNK_TYPE_COOKIEACK, 0, 4);
			if (obj->OnConnect != NULL && o->state == 1)
			{
				o->state = 2;
				sem_post(&(o->Lock));
				obj->OnConnect(obj, o, 1);
				if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state == 0) return;
				sem_wait(&(o->Lock));
			}
			break;
		case RCTP_CHUNK_TYPE_DATA:
		{
			unsigned int tsn;
			unsigned short streamId;
			unsigned short streamSeq;
			unsigned int pid;
			ILibSCTP_DataPayload *data;
			int pulled = 0;

			RCTPDEBUG(printf("RCTP_CHUNK_TYPE_DATA, Flags=%d, Size=%d\r\n", chunkflags, chunksize);)
			data = (ILibSCTP_DataPayload*)(buffer + ptr);
			tsn = ntohl(data->TSN);

			if (tsn == o->intsn + 1)
			{
				void* rqptr;
				o->intsn = tsn;
				streamId = ntohs(data->StreamID);
				streamSeq = ntohs(data->StreamSequenceNumber);
				pid = ntohl(data->ProtocolID);

				RCTPRCVDEBUG(printf("GOT %u, size = %d\r\n", tsn, chunksize);)
				RCTPDEBUG(printf("IN DATA_CHUNK FLAGS: %d, TSN: %u, ID: %d, SEQ: %d, PID: %u, SIZE: %d\r\n", chunkflags, tsn, streamId, streamSeq, pid, chunksize - 16);)

				// Move the TSN as forward as we can
				rqptr = ILibLinkedList_GetNode_Head(o->receiveHoldBuffer);
				while (rqptr != NULL)
				{					
					unsigned int tsnx = ntohl(((ILibSCTP_DataPayload*)ILibLinkedList_GetDataFromNode(rqptr))->TSN);
					if (tsnx != o->intsn + 1) break;
					o->intsn = tsnx;
					RCTPRCVDEBUG(printf("MOVED TSN to %u\r\n", o->intsn);)
					rqptr = ILibLinkedList_GetNextNode(rqptr);
				}

				if((o->flags & DTLS_PAUSE_FLAG)==DTLS_PAUSE_FLAG || (o->userTSN < o->intsn - 1))
				{
					// We are paused, so we need to buffer this packet for later. (Or, the userTSN isn't caught up yet)
					sentsack = ILibSCTP_AddPacketToHoldingQueue(o, data, sentsack);
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP: %d Packet: %u Buffered, due to PAUSE", o->sessionId, ntohl(data->TSN));
					break;
				}
				else
				{
					o->userTSN = o->intsn;
				}

				// TODO: Optimize this, send SACK only after a little bit of time (or every 3 packets or so). Or only in some cases.
				if (sentsack == ILibSCTP_SackStatus_NotSent)
				{
					sentsack = ILibSCTP_SackStatus_Sent;
					*rptr = ILibStun_SctpAddSackChunk(obj, session, rpacket, *rptr); // Send the ACK with the TSN as far forward as we can
				}

				sem_post(&(o->Lock));
				ILibStun_SctpProcessStreamData(obj, session, streamId, streamSeq, chunkflags, pid, data->UserData, chunksize - 16);
				if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state == 0) return;
				sem_wait(&(o->Lock));

				if((o->flags & DTLS_PAUSE_FLAG)==DTLS_PAUSE_FLAG) {break;}

				// Pull as many packets as we can out of the receive hold queue
				pulled = 0;
				while (ILibLinkedList_GetCount(o->receiveHoldBuffer) > 0)
				{
					ILibSCTP_DataPayload *payload = (ILibSCTP_DataPayload*)ILibLinkedList_GetDataFromNode(ILibLinkedList_GetNode_Head(o->receiveHoldBuffer));
					unsigned char chunkflagsx = payload->flags;
					unsigned int tsnx = ntohl(payload->TSN);
					unsigned short chunksizex = ntohs(payload->length);
					if ((tsnx > o->intsn + 1) || (tsnx > o->userTSN + 1)) break; // This is not the next expected packet
					
					pulled += payload->length;
					RCTPRCVDEBUG(printf("UNSTORING %u, size = %d\r\n", tsnx, chunksizex);)

					streamId = ntohs(payload->StreamID);
					streamSeq = ntohs(payload->StreamSequenceNumber);
					pid = ntohl(payload->ProtocolID);
					o->userTSN = o->intsn = tsnx;

					sem_post(&(o->Lock));
					ILibStun_SctpProcessStreamData(obj, session, streamId, streamSeq, chunkflagsx, pid, payload->UserData, chunksizex - 16);
					if (obj->dTlsSessions[session] == NULL || obj->dTlsSessions[session]->state == 0) return;
					sem_wait(&(o->Lock));

					free(payload);
					ILibLinkedList_Remove(ILibLinkedList_GetNode_Head(o->receiveHoldBuffer));

					if((o->flags & DTLS_PAUSE_FLAG)==DTLS_PAUSE_FLAG) {break;}
				}
				if (pulled > 0) ReceiveHoldBuffer_Decrement(o->receiveHoldBuffer, pulled);
			}
			else if (tsn > o->intsn + 1)
			{
				// This is an out of order packet, so lets buffer it for later
				sentsack = ILibSCTP_AddPacketToHoldingQueue(o, data, sentsack);
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP: %d Packet: %u Buffered, out of order", o->sessionId, ntohl(data->TSN));
			}
			else
			{
				// Already received packet.
				RCTPRCVDEBUG(printf("TOSSING %u, size = %d\r\n", tsn, chunksize);)

				// Send ACK now
				if (sentsack == 0)
				{
					sentsack = 1;
					*rptr = ILibStun_SctpAddSackChunk(obj, session, rpacket, *rptr); // Send the ACK with the TSN as far forward as we can
				}
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_3, "SCTP: %d Packet: %u dropped, duplicate", o->sessionId, ntohl(data->TSN));
			}
		}
			break;
		default:
			if (buffer[ptr] & 0x40) { stop = 1; } // Unknown chunk, if this bit is set, stop processing now.
			break;
		}

		ptr += FOURBYTEBOUNDARY(chunksize); // Add chunk size and padding
	}

	if (session == -1) { sem_post(&(o->Lock)); return; }
	if (*rptr > 12) { ILibStun_SendSctpPacket(obj, session, rpacket, *rptr); }
	*rptr = 0;
	sem_post(&(o->Lock));
}

void ILibSCTP_Pause(void* module)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)module;
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d was PAUSED by the USER", obj->sessionId);

	sem_wait(&(obj->Lock));
	obj->flags |= DTLS_PAUSE_FLAG;
	sem_post(&(obj->Lock));
}
void ILibSCTP_Resume(void* module)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)module;
	struct ILibStun_Module *sobj = obj->parent;
	void *node;
	ILibSCTP_DataPayload *payload;
	int sessionID = obj->sessionId;
	int pulled = 0;

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d RESUME operation begin", obj->sessionId);

	sem_wait(&(obj->Lock));
	if((obj->flags & DTLS_PAUSE_FLAG) == DTLS_PAUSE_FLAG)
	{
		obj->flags ^= DTLS_PAUSE_FLAG;
	}

	// Check the receive hold buffer to see if there are any packets we need to propagate
	while((node = ILibLinkedList_GetNode_Head(obj->receiveHoldBuffer)) != NULL && (payload = (ILibSCTP_DataPayload*)ILibLinkedList_GetDataFromNode(node)) != NULL)
	{
		if(ntohl(payload->TSN) == obj->userTSN + 1)
		{
			// This packet is the next expected packet for the user
			obj->userTSN = ntohl(payload->TSN);
			pulled += payload->length;
			sem_post(&(obj->Lock));
			ILibStun_SctpProcessStreamData(obj->parent, obj->sessionId, ntohs(payload->StreamID), ntohs(payload->StreamSequenceNumber), payload->flags, ntohl(payload->ProtocolID), payload->UserData, ntohs(payload->length) - 16);
			if (sobj->dTlsSessions[sessionID] == NULL || sobj->dTlsSessions[sessionID]->state == 0) return; // Referencing Dtls object this way, in case it was closed/freed by the user in the last call
			sem_wait(&(obj->Lock));
			free(payload);
			ILibLinkedList_Remove(node);
		}
		else
		{
			break;
		}
	}
	if (pulled > 0) ReceiveHoldBuffer_Decrement(obj->receiveHoldBuffer, pulled);
	sem_post(&(obj->Lock));
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP: %d RESUME operation complete", sessionID);
}


void ILibStun_InitiateSCTP(struct ILibStun_Module *obj, int session, unsigned short sourcePort, unsigned short destinationPort)
{
	char* buffer;
	int ptr;
	unsigned int initiateTag;

	obj->dTlsSessions[session]->inport = sourcePort;
	obj->dTlsSessions[session]->outport = destinationPort;
	obj->dTlsSessions[session]->receiverCredits = ILibSCTP_MaxReceiverCredits;
	if ((buffer = (char*)malloc(20 + 12)) == NULL){ ILIBCRITICALEXIT(254); }
	ptr = 12;

#ifdef _WEBRTCDEBUG
	// Debug Events
	if (obj->dTlsSessions[session]->onReceiverCredits != NULL) { obj->dTlsSessions[session]->onReceiverCredits(obj->dTlsSessions[session], "OnReceiverCredits", obj->dTlsSessions[session]->receiverCredits); }
#endif

	ptr = ILibStun_AddSctpChunkHeader(buffer, ptr, RCTP_CHUNK_TYPE_INIT, 0, 20);
	util_random(4, buffer + ptr);							// Initiate Tag
	initiateTag = ((unsigned int*)(buffer + ptr))[0];
	ptr += 4;

	((unsigned int*)(buffer + ptr))[0] = htonl(ILibSCTP_MaxReceiverCredits);	// Receiver Credits
	ptr += 4;

	((unsigned short*)(buffer + ptr))[0] = htons(ILibSCTP_Stream_MaximumCount);		// Num Out-Streams
	((unsigned short*)(buffer + ptr))[1] = htons(ILibSCTP_Stream_MaximumCount);		// Num In-Streams
	ptr += 4;

	util_random(4, buffer + ptr);							// Initial TSN
	obj->dTlsSessions[session]->RREQSEQ = obj->dTlsSessions[session]->outtsn = ntohl(((unsigned int*)(buffer + ptr))[0]);
	ptr += 4;

	ILibStun_SendSctpPacket(obj, session, buffer, ptr);

	obj->dTlsSessions[session]->tag = initiateTag;
}

void ILibWebRTC_OpenDataChannel(void *WebRTCModule, unsigned short streamId, char* channelName, int channelNameLength)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)WebRTCModule;
	ILibSCTP_StreamAttributes attributes;
	char* buffer;
	if ((buffer = (char*)malloc(12 + channelNameLength)) == NULL){ ILIBCRITICALEXIT(254); }

	buffer[0] = 0x03;	// DATA_CHANNEL_OPEN
	buffer[1] = 0x00;	// Reliable Channel Type
	((unsigned short*)buffer)[1] = 0x00;	// Priority
	((unsigned int*)buffer)[1] = 0x00;		// Reliability (Ignored for Reliable Channels)

	((unsigned short*)buffer)[4] = htons((unsigned short)channelNameLength);	// Label Name Length
	((unsigned short*)buffer)[5] = 0x00;						// Protocol Length

	memcpy(buffer + 12, channelName, channelNameLength);

	attributes.Raw = 0x00;
	attributes.Data.StatusFlags |= ILibSCTP_StreamAttributesData_Assigned_Status_WAITING_FOR_ACK;
	
	ILibSparseArray_Add(obj->DataChannelMetaDeta, streamId, attributes.Raw);


	ILibSCTP_SendEx(WebRTCModule, streamId, buffer, 12 + channelNameLength, 50);
	free(buffer);
}

void ILibWebRTC_CloseDataChannel_ALL_Ex(ILibSparseArray sender, int index, void *data, void* user)
{
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(data);

	((unsigned short*)user)[++((unsigned short*)user)[0]] = (unsigned short)index;
}
void ILibWebRTC_CloseDataChannel_ALL(void *WebRTCModule)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)WebRTCModule;
	unsigned short ids[1024];

	//ILibWebRTC_CloseDataChannelEx(obj, NULL, 0); //This is temporarily commented out, as I filed a bug against Chrome and Firefox to properly support this required behavior
	
	ILibSparseArray channels = ILibSparseArray_Move(obj->DataChannelMetaDeta);
	ids[0] = 0;

	ILibSparseArray_DestroyEx(channels, &ILibWebRTC_CloseDataChannel_ALL_Ex, (void*)ids);	
	ILibWebRTC_CloseDataChannelEx(obj, ids+1, ids[0]);
}

ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannelEx2(void *WebRTCModule, unsigned short *streamIds, int streamIdLength)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)WebRTCModule;
	int len = FOURBYTEBOUNDARY(16 + 2*streamIdLength) + FOURBYTEBOUNDARY((8+2*streamIdLength)) + 16;
	int i;
	ILibWebRTC_DataChannel_CloseStatus retVal = ILibWebRTC_DataChannel_CloseStatus_OK;

	ILibSCTP_ReconfigChunk *chunk;
	ILibSCTP_Reconfig_OutgoingSSNResetRequest *req;
	ILibSCTP_Reconfig_IncomingSSNResetRequest *req2;

	if(len > (obj->rpacketsize / 2) )
	{
		// We don't want to consume too much of the rpacket buffer
		ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Cannot process close, rpacket buffer is too small. Dtls Session: %d", obj->sessionId);
		return(ILibWebRTC_DataChannel_CloseStatus_ERROR);
	}

	if(obj->pendingReconfigPacket == NULL)
	{
		obj->pendingReconfigPacket = obj->rpacket + (obj->rpacketsize - len);
		obj->reconfigFailures = 0;

		chunk  = (ILibSCTP_ReconfigChunk*)(obj->pendingReconfigPacket + 12);
		req = (ILibSCTP_Reconfig_OutgoingSSNResetRequest*)&(chunk->reconfigurationParameter);
		req2 = (ILibSCTP_Reconfig_IncomingSSNResetRequest*)((char*)&chunk->reconfigurationParameter + FOURBYTEBOUNDARY(16 + 2*streamIdLength));

		if(ILibSCTP_DoesPeerSupportFeature(obj, RCTP_CHUNK_TYPE_RECONFIG)!=0)
		{
			chunk->type = RCTP_CHUNK_TYPE_RECONFIG;
			chunk->chunkFlags = 0;
			chunk->chunkLength = htons((unsigned short)(4 + FOURBYTEBOUNDARY(16 + 2*streamIdLength) + FOURBYTEBOUNDARY(8+2*streamIdLength)));

			req->parameterType = htons(SCTP_RECONFIG_TYPE_OUTGOING_SSN_RESET_REQUEST);
			req->parameterLength = htons((unsigned short)(16 + 2*streamIdLength));
			req->LastTSN = htonl(obj->outtsn-1);

			req->RReqSeqNum = htonl(obj->RREQSEQ++);
			req->RResSeqNum = htonl(obj->RRESSEQ++);

			req2->parameterType = htons(SCTP_RECONFIG_TYPE_INCOMING_SSN_RESET_REQUEST);
			req2->parameterLength = htons((unsigned short)(8 + (2 * streamIdLength))); // One Stream
			req2->RReqSeqNum = htonl(obj->RREQSEQ++);

			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP/RECONFIG on Session(%d) LastTSN: %u", obj->sessionId, obj->outtsn - 1);
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Sending Outgoing Reset Request (Req/Res): %u/%u", (unsigned int)ntohl(req->RReqSeqNum), (unsigned int)ntohl(req->RResSeqNum));			
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "Sending Incoming Reset Request (Req): %u", (unsigned int)ntohl(req2->RReqSeqNum));
			for (i = 0; i<streamIdLength; ++i)
			{
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "...StreamID: %u", streamIds[i]);
				req->Streams[i] = htons(streamIds[i]);
				req2->Streams[i] = htons(streamIds[i]);
			}

			ILibLifeTime_AddEx(obj->parent->Timer, ILibWebRTC_DTLS_TO_TIMER_OBJECT(obj), RTO_MIN * (0x01 << obj->reconfigFailures), &ILibWebRTC_CloseDataChannel_Timeout, NULL);
			ILibStun_SendSctpPacket(obj->parent, obj->sessionId, obj->pendingReconfigPacket, len);
		}
		else
		{
			// Peer does not support this operation... 
			retVal = ILibWebRTC_DataChannel_CloseStatus_NOT_SUPPORTED;
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_SCTP, ILibRemoteLogging_Flags_VerbosityLevel_1, "SCTP/RECONFIG on PEER [NOT-SUPPORTED]");
		}
	}
	else
	{
		retVal = ILibWebRTC_DataChannel_CloseStatus_ALREADY_PENDING; // There is already a pending Close Operation
	}
	return(retVal);
}
ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannelEx(void *WebRTCModule, unsigned short *streamIds, int streamIdLength)
{
	struct ILibStun_dTlsSession* obj = (struct ILibStun_dTlsSession*)WebRTCModule;
	ILibWebRTC_DataChannel_CloseStatus retVal;

	sem_wait(&obj->Lock);
	retVal = ILibWebRTC_CloseDataChannelEx2(WebRTCModule, streamIds, streamIdLength);
	sem_post(&obj->Lock);
	return(retVal);
}

ILibWebRTC_DataChannel_CloseStatus ILibWebRTC_CloseDataChannel(void *WebRTCModule, unsigned short streamId)
{
	return(ILibWebRTC_CloseDataChannelEx(WebRTCModule, &streamId, 1)); // Ignore Klocwork Error, it's not a problem in this case
	//Note: Klocwork spuriously says that array '&chunk->reconfigurationParamter' of size 4, may use index value 20... This is false, as the array is not accessed like that
}


int ILibSCTP_DoesPeerSupportFeature(void* module, int feature)
{
	struct ILibStun_dTlsSession *obj = (struct ILibStun_dTlsSession*)module;
	return(ILibSparseArray_Get(obj->PeerFeatureSet, feature)==NULL?0:1);
}

ILibTransport_DoneState ILibWebRTC_TransportSend(void *transport, char* buffer, int bufferLength, ILibTransport_MemoryOwnership ownership, ILibTransport_DoneState done)
{
	ILibTransport_DoneState retVal = (ILibTransport_DoneState)ILibSCTP_Send(transport, 0, buffer, bufferLength);

	UNREFERENCED_PARAMETER(done);

	if(ownership == ILibTransport_MemoryOwnership_CHAIN) {free(buffer);}
	return(retVal);
}

void ILibStun_CreateDtlsSession(struct ILibStun_Module *obj, int sessionId, int iceSlot, struct sockaddr_in6* remoteInterface)
{
	if (obj->dTlsSessions[sessionId] == NULL) 
	{ 
		if ((obj->dTlsSessions[sessionId] = (struct ILibStun_dTlsSession*)malloc(sizeof(struct ILibStun_dTlsSession))) == NULL) ILIBCRITICALEXIT(254); 
	}
	else
	{
		sem_destroy(&(obj->dTlsSessions[sessionId]->Lock));
		ILibWebRTC_DestroySparseArrayTables(obj->dTlsSessions[sessionId]);
	}

	memset(obj->dTlsSessions[sessionId], 0, sizeof(struct ILibStun_dTlsSession));
	obj->dTlsSessions[sessionId]->IdentifierFlags = (unsigned short)ILibTransports_Raw_WebRTC;
	obj->dTlsSessions[sessionId]->Chain = obj->Chain;
	obj->dTlsSessions[sessionId]->sendPtr = &ILibWebRTC_TransportSend;
	obj->dTlsSessions[sessionId]->closePtr = &ILibSCTP_Close;
	//obj->dTlsSessions[sessionId]->closePtr = &ILibWebRTC_CloseDataChannel_ALL;
	obj->dTlsSessions[sessionId]->pendingPtr = (ILibTransport_PendingBytesToSendPtr)&ILibSCTP_GetPendingBytesToSend;

	obj->dTlsSessions[sessionId]->iceStateSlot = iceSlot;
	obj->dTlsSessions[sessionId]->state = 4; //Bryan: Changed this to 4 from 1, becuase we need to call SSL_do_handshake to determine when DTLS was successful
	obj->dTlsSessions[sessionId]->sessionId = sessionId;
	sem_init(&(obj->dTlsSessions[sessionId]->Lock), 0, 1);
	obj->dTlsSessions[sessionId]->parent = obj;
	memcpy(&(obj->dTlsSessions[sessionId]->remoteInterface), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
	obj->dTlsSessions[sessionId]->senderCredits = 4 * ILibRUDP_StartMTU;
	obj->dTlsSessions[sessionId]->congestionWindowSize = 4 * ILibRUDP_StartMTU;
	obj->dTlsSessions[sessionId]->ssl = SSL_new(obj->SecurityContext);
	if ((obj->dTlsSessions[sessionId]->rpacket = (char*)malloc(4096)) == NULL) ILIBCRITICALEXIT(254);
	obj->dTlsSessions[sessionId]->rpacketsize = 4096;

	ILibWebRTC_CreateSparseArrayTables(obj->dTlsSessions[sessionId]);
	obj->dTlsSessions[sessionId]->receiveHoldBuffer = ILibLinkedList_Create();

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "New DTLS Session: %d linked to IceStateSlot: %d using %s:%u", sessionId, iceSlot, ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), htons(remoteInterface->sin6_port));
}

void ILibStun_InitiateDTLS(struct ILibStun_IceState *IceState, int IceSlot, struct sockaddr_in6* remoteInterface)
{
	long l;
	BIO* read;
	BIO* write;
	int i, j = -1;
	struct ILibStun_Module *obj = IceState->parentStunModule;
	char tbuffer[4096];

	// Find a free dTLS session slot
	for (i = 0; i < ILibSTUN_MaxSlots; i++) { if (obj->dTlsSessions[i] == NULL || obj->dTlsSessions[i]->state == 0) { j = i; break; } }
	if (j == -1) return; // No free slots

	IceState->dtlsSession = j;  // Set the DTLS Session ID in the IceState object this is associated with

	// Start a new dTLS session
	ILibStun_CreateDtlsSession(obj, j, IceSlot, remoteInterface);

	// Set up the memory-buffer BIOs
	read = BIO_new(BIO_s_mem());
	write = BIO_new(BIO_s_mem());
	BIO_set_mem_eof_return(read, -1);
	BIO_set_mem_eof_return(write, -1);

	// Bind everything
	SSL_set_bio(obj->dTlsSessions[j]->ssl, read, write);
	SSL_set_connect_state(obj->dTlsSessions[j]->ssl);
	l = SSL_do_handshake(obj->dTlsSessions[j]->ssl);
	if (l <= 0) { l = SSL_get_error(obj->dTlsSessions[j]->ssl, l); }

	if (l == SSL_ERROR_WANT_READ)
	{
		while (BIO_ctrl_pending(write) > 0)
		{
			i = BIO_read(write, tbuffer, 4096);
			if (IceState->useTurn == 0)
			{
				ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)obj)->UDP, (struct sockaddr*)remoteInterface, tbuffer, i, ILibAsyncSocket_MemoryOwnership_USER);
			}
			else
			{
				ILibTURN_SendIndication(obj->mTurnClientModule, remoteInterface, tbuffer, 0, i);
			}
		}
		// We're going to drop out now, becuase we need to check for received UDP data
	}
}

int ILibStun_GetDtlsSessionSlotForIceState(struct ILibStun_Module *obj, struct ILibStun_IceState* ice)
{
	int x, i;
	struct ILibStun_dTlsSession *session;

	for (x = 0; x < ILibSTUN_MaxSlots; ++x)
	{
		session = obj->dTlsSessions[x];
		if (session != NULL)
		{
			for (i = 0; i < ice->hostcandidatecount; ++i)
			{
				if (ice->hostcandidates[i].addr == ((struct sockaddr_in*)&session->remoteInterface)->sin_addr.s_addr)
				{
					if (ice->hostcandidates[i].port == session->remoteInterface.sin6_port) { return x; }
				}
			}
		}
	}
	return -1;
}

void ILibStun_DTLS_GetIceUserName(void* WebRTCModule, char* username)
{
	struct ILibStun_dTlsSession* session = (struct ILibStun_dTlsSession*)WebRTCModule;
	memcpy(username, session->parent->IceStates[session->iceStateSlot]->userAndKey + 1, 8);
}

void ILibStun_DTLS_Success_OnCreateTURNChannelBinding(ILibTURN_ClientModule turnModule, unsigned short channelNumber, int success, void* user)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)user;

	UNREFERENCED_PARAMETER(turnModule);

	if (success != 0)
	{
		// ChannelBinding Creation was successful
		if (obj->IceStates[obj->dTlsSessions[(int)channelNumber]->iceStateSlot]->dtlsInitiator != 0)
		{
			// We must initiate SCTP
			ILibStun_InitiateSCTP(obj, (int)channelNumber, 5000, 5000);
		}
		else
		{
			// Start the timer, for SCTP Heartbeats
			ILibLifeTime_AddEx(obj->Timer, obj->dTlsSessions[(int)channelNumber], 100, &ILibStun_SctpOnTimeout, NULL);
		}
	}
}

void ILibStun_DTLS_Success(struct ILibStun_Module *obj, int session, struct sockaddr_in6 *remoteInterface)
{
	int IceSlot;

	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "DTLS Session: %d [Handshake SUCCESS]", session);

	obj->dTlsSessions[session]->SSTHRESH = 4 * ILibRUDP_StartMTU;

	IceSlot = obj->dTlsSessions[session]->iceStateSlot;
	if (IceSlot >= 0)
	{
		if (obj->IceStates[IceSlot]->useTurn != 0)
		{
			// We need to create a ChannelBinding for this DTLS Session
			ILibTURN_CreateChannelBinding(obj->mTurnClientModule, (unsigned short)session, remoteInterface, ILibStun_DTLS_Success_OnCreateTURNChannelBinding, obj);
		}
		else if (obj->IceStates[IceSlot]->dtlsInitiator != 0) // This is in an else clause, becuase if we create a channel binding, we'll check this condition later
		{
			// Since the remote side didn't initiate the offer, we must initiate SCTP
			ILibStun_InitiateSCTP(obj, session, 5000, 5000);
		}
		else
		{
			// Start the timer, for SCTP Heartbeats
			ILibLifeTime_AddEx(obj->Timer, obj->dTlsSessions[session], 100, &ILibStun_SctpOnTimeout, NULL);
		}

		// Since DTLS is established, we can stop sending periodic STUNS on the ICE Offer Candidates
		// TODO: Bryan: Fix this, Periodic stuns don't work like this anymore
		ILibLifeTime_Remove(obj->Timer, obj->IceStates[IceSlot]);

		if (obj->consentFreshnessDisabled == 0) // TODO: Bryan: We should really put this after SCTP has been established...
		{
			// Start Consent Freshness Algorithm. Wait for the Timeout, then send first packet
			ILibLifeTime_Add(obj->Timer, obj->dTlsSessions[session] + 1, ILibStun_MaxConsentFreshnessTimeoutSeconds, ILibStun_WebRTC_ConsentFreshness_Start, NULL);
		}
	}
}

void ILibStun_OnUDP(ILibAsyncUDPSocket_SocketModule socketModule, char* buffer, int bufferLength, struct sockaddr_in6 *remoteInterface, void *user, void *user2, int *PAUSE)
{
	BIO* read;
	BIO* write;
	int i, cx, j = -1;
	int existingSession = -1;
	struct ILibStun_Module *obj = (struct ILibStun_Module*)user;
	char tbuffer[4096];
	u_long err;
	int dtlsSessionId = -1;
	int iceSlotId = -1;

	UNREFERENCED_PARAMETER(user2);
	UNREFERENCED_PARAMETER(PAUSE);
	UNREFERENCED_PARAMETER(socketModule);

	if (socketModule == NULL && remoteInterface->sin6_family == 0)
	{
		existingSession = (int)remoteInterface->sin6_port;
		remoteInterface = &(obj->dTlsSessions[existingSession]->remoteInterface);
	}

	// Process STUN Packet
	if (ILibStun_ProcessStunPacket(obj, buffer, bufferLength, remoteInterface) == 1) return;

	if (obj->SecurityContext == NULL || obj->CertThumbprint == NULL) return; // No dTLS support
	if (existingSession < 0)
	{
		//
		// Enumerate ICE Offers, and see if any of the associated DTLS sessions is this one
		//
		for (i = 0; i < ILibSTUN_MaxSlots; ++i)
		{
			if (obj->IceStates[i] != NULL && obj->IceStates[i]->dtlsSession >= 0)
			{
				if (obj->dTlsSessions[obj->IceStates[i]->dtlsSession] != NULL)
				{
					if (memcmp(&(obj->dTlsSessions[obj->IceStates[i]->dtlsSession]->remoteInterface), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family)) == 0)
					{
						existingSession = obj->IceStates[i]->dtlsSession;
						break;
					}
				}
			}
		}

		if (existingSession < 0)
		{
			//
			// Check the existing sessions one more time, to see if the remote side switched interfaces on us
			//
			for (i = 0; i < ILibSTUN_MaxSlots; ++i)
			{
				if (obj->IceStates[i] != NULL && obj->IceStates[i]->dtlsSession >= 0)
				{
					if (obj->dTlsSessions[obj->IceStates[i]->dtlsSession] != NULL)
					{
						for (cx = 0; cx < obj->IceStates[i]->hostcandidatecount; ++cx)
						{
							if (obj->IceStates[i]->hostcandidates[cx].port == remoteInterface->sin6_port && obj->IceStates[i]->hostcandidates[cx].addr == ((struct sockaddr_in*)remoteInterface)->sin_addr.s_addr)
							{
								if (obj->IceStates[i]->hostcandidateResponseFlag[cx] == 1)
								{
									ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "DTLS Session: %d switching candidates from: %s:u", obj->IceStates[i]->dtlsSession, ILibRemoteLogging_ConvertAddress((struct sockaddr*)(&obj->dTlsSessions[obj->IceStates[i]->dtlsSession]->remoteInterface)), ntohs(obj->dTlsSessions[obj->IceStates[i]->dtlsSession]->remoteInterface.sin6_port));
									ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...TO: %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), ntohs(remoteInterface->sin6_port));
									
									// This Candidate was Allowed, let's switch to this candidate
									memcpy(&(obj->dTlsSessions[obj->IceStates[i]->dtlsSession]->remoteInterface), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
									existingSession = obj->IceStates[i]->dtlsSession;
									break;
								}
							}
						}
						if (existingSession >= 0) { break; }
					}
				}
			}
		}
	}
	// Check if this is for a new dTLS session
	// Modified to remove the dTLS Hello detection, because if this isn't a STUN packet, it has to be dTLS. OpenSSL will just fail the handshake if it isn't, which is fine.
	if (existingSession == -1) 
	{
		// We don't have a session established yet, so just check to see if the candidate is allowed
		for (i = 0; i < ILibSTUN_MaxSlots; ++i)
		{
			if (obj->IceStates[i] != NULL && obj->IceStates[i]->dtlsSession < 0)
			{
				for (cx = 0; cx < obj->IceStates[i]->hostcandidatecount; ++cx)
				{
					if (obj->IceStates[i]->hostcandidates[cx].port == remoteInterface->sin6_port && obj->IceStates[i]->hostcandidates[cx].addr == ((struct sockaddr_in*)remoteInterface)->sin_addr.s_addr)
					{
						if (obj->IceStates[i]->hostcandidateResponseFlag[cx] == 1)
						{
							// This Candidate was Allowed, find a free dTLS session slot
							for (dtlsSessionId = 0; dtlsSessionId < ILibSTUN_MaxSlots; dtlsSessionId++)
							{
								if (obj->dTlsSessions[dtlsSessionId] == NULL || obj->dTlsSessions[dtlsSessionId]->state == 0)
								{
									obj->IceStates[i]->dtlsSession = dtlsSessionId;
									iceSlotId = i;									
									break;
								}
							}
							if (dtlsSessionId == ILibSTUN_MaxSlots) 
							{
								ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, " ABORTING: No free dTLS Session Slots");
								return; // No free slots
							}
						}
						else
						{
							ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...DTLS Packet from: %s:%u was using a disallowed candidate", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), ntohs(remoteInterface->sin6_port));
							return; // This candidate was not allowed
						}
						i = ILibSTUN_MaxSlots;
						break;
					}
				}
			}
		}

		if (dtlsSessionId < 0)
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...DTLS Packet from: %s:%u has no valid candidates", ILibRemoteLogging_ConvertAddress((struct sockaddr*)remoteInterface), ntohs(remoteInterface->sin6_port));
			return; // No valid candidates were found
		}

		j = dtlsSessionId;

		// Start a new dTLS session
		ILibStun_CreateDtlsSession(obj, j, iceSlotId, remoteInterface);	

		// Set up the memory-buffer BIOs
		read = BIO_new(BIO_s_mem());
		write = BIO_new(BIO_s_mem());
		BIO_set_mem_eof_return(read, -1);
		BIO_set_mem_eof_return(write, -1);

		// Bind everything
		SSL_set_bio(obj->dTlsSessions[j]->ssl, read, write);
		SSL_set_accept_state(obj->dTlsSessions[j]->ssl);

		// Start the timer
		//ILibLifeTime_AddEx(obj->Timer, obj->dTlsSessions[j], 100, &ILibStun_SctpOnTimeout, NULL); //Moved this to after DTLS is setup, becuase SCTP might not be setup yet

		switch (SSL_do_handshake(obj->dTlsSessions[j]->ssl))
		{
		case 0:
			//
			// Handshake Failed!
			//			
			ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Handshake FAILED");
			while ((err = ERR_get_error()) != 0) 
			{
				ERR_error_string_n(err, ILibScratchPad, sizeof(ILibScratchPad));
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Reason: %s", ILibScratchPad);
			}				   
			existingSession = j; //ToDo: Bryan. Figure out what to do in this case
			break;
		case 1:
			obj->dTlsSessions[j]->state = 1;
			ILibStun_DTLS_Success(obj, j, remoteInterface);			 // Successful DTLS Handshake	
			break;
		default:
			//
			// SSL_WANT_READ most likely, so do nothing for now
			//
			break;
		}

		while (BIO_ctrl_pending(write) > 0)
		{
			i = BIO_read(write, tbuffer, 4096);
			if (socketModule == NULL)
			{
				// Response was from TURN
				if (remoteInterface->sin6_family == 0)
				{
					ILibTURN_SendChannelData(obj->mTurnClientModule, remoteInterface->sin6_port, tbuffer, 0, i);
				}
				else
				{
					ILibTURN_SendIndication(obj->mTurnClientModule, remoteInterface, tbuffer, 0, i);
				}
			}
			else
			{
				// Response was from a local socket
				ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)obj)->UDP, (struct sockaddr*)remoteInterface, tbuffer, i, ILibAsyncSocket_MemoryOwnership_USER);
			}
		}

		existingSession = j;
	}

	// If we have an existing dTLS session, process the data. This can also happen right after we create the session above.
	if (existingSession != -1 && (obj->dTlsSessions[existingSession]->state == 1 || obj->dTlsSessions[existingSession]->state == 2 || obj->dTlsSessions[existingSession]->state == 4))
	{
		sem_wait(&(obj->dTlsSessions[existingSession]->Lock));
		BIO_write(SSL_get_rbio(obj->dTlsSessions[existingSession]->ssl), buffer, bufferLength);
		if (obj->dTlsSessions[existingSession]->state == 4)
		{
			ILibWebRTC_DTLS_HandshakeDetect(obj, "R ", buffer, 0, bufferLength);

			// Connecting... Handshake isn't done yet
			switch (SSL_do_handshake(obj->dTlsSessions[existingSession]->ssl))
			{
			case 0:
				// Handshake Failed!
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Handshake FAILED");
				while ((err = ERR_get_error()) != 0) 
				{
					ERR_error_string_n(err, ILibScratchPad, sizeof(ILibScratchPad));
					ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "......Reason: %s", ILibScratchPad);
				}				   
				// TODO: We should probably do something
				break;
			case 1:
				obj->dTlsSessions[existingSession]->state = 1;
				ILibStun_DTLS_Success(obj, existingSession, remoteInterface);			 // Successful DTLS Handshake			
				break;
			default:
				// SSL_WANT_READ most likely, so do nothing for now
				break;
			}
			sem_post(&(obj->dTlsSessions[existingSession]->Lock)); //ToDo: Bryan/Is this right?
		}
		else
		{
			i = SSL_read(obj->dTlsSessions[existingSession]->ssl, tbuffer, 4096);
			sem_post(&(obj->dTlsSessions[existingSession]->Lock));
			if (i > 0)
			{
				// We got new dTLS data
				ILibStun_ProcessSctpPacket(obj, existingSession, tbuffer, i);
			}
			else if (i == 0)
			{
				// Session closed, perform cleanup
				ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "DTLS Session: %d was closed", existingSession);
				ILibStun_SctpDisconnect(obj, existingSession);
				return;
			}
			else
			{
				i = SSL_get_error(obj->dTlsSessions[existingSession]->ssl, i);
			}
		}

		if (obj->dTlsSessions[existingSession] != NULL && obj->dTlsSessions[existingSession]->state != 0)
		{
			sem_wait(&(obj->dTlsSessions[existingSession]->Lock));
			while (BIO_ctrl_pending(SSL_get_wbio(obj->dTlsSessions[existingSession]->ssl)) > 0)
			{
				// Data is pending in the write buffer, send it out
				j = BIO_read(SSL_get_wbio(obj->dTlsSessions[existingSession]->ssl), tbuffer, 4096);
				if (socketModule == NULL)
				{
					// Response was from TURN
					if (remoteInterface->sin6_family == 0)
					{
						ILibTURN_SendChannelData(obj->mTurnClientModule, remoteInterface->sin6_port, tbuffer, 0, j);
					}
					else
					{
						ILibTURN_SendIndication(obj->mTurnClientModule, remoteInterface, tbuffer, 0, j);
					}
				}
				else
				{
					// Response was from a local Socket
					ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)obj)->UDP, (struct sockaddr*)remoteInterface, tbuffer, j, ILibAsyncSocket_MemoryOwnership_USER);
				}
				if(obj->dTlsSessions[existingSession]->state == 4) {ILibWebRTC_DTLS_HandshakeDetect(obj, "S ", tbuffer, 0, j);}
			}
			sem_post(&(obj->dTlsSessions[existingSession]->Lock));
		}
	}
}

int ILibStunClient_dTLS_verify_callback(int ok, X509_STORE_CTX *ctx)
{
	int i, l = 32;
	char thumbprint[32];

	// Validate the incoming certificate against known allowed fingerprints.
	UNREFERENCED_PARAMETER(ok);

	X509_digest(ctx->current_cert, EVP_get_digestbyname("sha256"), (unsigned char*)thumbprint, (unsigned int*)&l);
	if (l != 32 || g_stunModule == NULL) return 0;
	ILibRemoteLogging_printf(ILibChainGetLogger(g_stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "Verifying Inbound Cert: %s", ILibRemoteLogging_ConvertToHex(thumbprint, 32));
	
	for (i = 0; i < ILibSTUN_MaxSlots; i++)
	{
		if (g_stunModule->IceStates[i] != NULL && g_stunModule->IceStates[i]->dtlscerthashlen == 32 && memcmp(g_stunModule->IceStates[i]->dtlscerthash, thumbprint, 32) == 0)
		{
			ILibRemoteLogging_printf(ILibChainGetLogger(g_stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...Matches Slot[%d]", i);
			return 1;
		}
	}
	ILibRemoteLogging_printf(ILibChainGetLogger(g_stunModule->Chain), ILibRemoteLogging_Modules_WebRTC_DTLS, ILibRemoteLogging_Flags_VerbosityLevel_1, "...FAILED (No Matches)");
	return 0;
}

//
// TURN Client Support for WebRTC
//
void ILibWebRTC_OnTurnConnect(ILibTURN_ClientModule turnModule, int success)
{
	if (success != 0) { ILibTURN_Allocate(turnModule, ILibTURN_TransportTypes_UDP); }
}

void ILibWebRTC_OnTurnAllocate(ILibTURN_ClientModule turnModule, int Lifetime, struct sockaddr_in6* RelayedTransportAddress)
{
	struct ILibStun_Module* stun = (struct ILibStun_Module*)ILibTURN_GetTag(turnModule);
	if (Lifetime > 0) { memcpy(&(stun->mRelayedTransportAddress), RelayedTransportAddress, sizeof(struct sockaddr_in6)); }
}

void ILibWebRTC_OnTurnDataIndication(ILibTURN_ClientModule turnModule, struct sockaddr_in6* remotePeer, char* buffer, int offset, int length)
{
	void* stunModule = ILibTURN_GetTag(turnModule);
	ILibStun_OnUDP(NULL, buffer + offset, length, remotePeer, stunModule, NULL, NULL);
}

void ILibWebRTC_OnTurnChannelData(ILibTURN_ClientModule turnModule, unsigned short channelNumber, char* buffer, int offset, int length)
{
	void* stunModule = ILibTURN_GetTag(turnModule);
	struct sockaddr_in6 fakeAddress;

	fakeAddress.sin6_family = 0;
	fakeAddress.sin6_port = channelNumber;

	ILibStun_OnUDP(NULL, buffer + offset, length, &fakeAddress, stunModule, NULL, NULL);
}

void ILibWebRTC_SetTurnServer(void* stunModule, struct sockaddr_in6* turnServer, char* username, int usernameLength, char* password, int passwordLength, ILibWebRTC_TURN_ConnectFlags turnFlags)
{
	struct ILibStun_Module* stun = (struct ILibStun_Module*)stunModule;

	// We will only support setting/changing these values if we aren't already connected to a TURN server
	if (turnServer != NULL)
	{
		switch (turnServer->sin6_family)
		{
		case AF_INET:
			memcpy(&(stun->mTurnServerAddress), turnServer, sizeof(struct sockaddr_in));
			break;
		case AF_INET6:
			memcpy(&(stun->mTurnServerAddress), turnServer, sizeof(struct sockaddr_in6));
			break;
		}
	}
	if (stun->turnUsername != NULL)
	{
		free(stun->turnUsername);
		stun->turnUsername = NULL;
	}
	if (usernameLength > 0 && username != NULL)
	{
		if ((stun->turnUsername = (char*)malloc(usernameLength + 1)) == NULL){ ILIBCRITICALEXIT(254); }
		memcpy(stun->turnUsername, username, usernameLength);
		stun->turnUsername[usernameLength] = 0;
		stun->turnUsernameLength = usernameLength;
	}
	if (stun->turnPassword != NULL)
	{
		free(stun->turnPassword);
		stun->turnPassword = NULL;
	}
	if (passwordLength > 0 && password != NULL)
	{
		if ((stun->turnPassword = (char*)malloc(passwordLength + 1)) == NULL){ ILIBCRITICALEXIT(254); }
		memcpy(stun->turnPassword, password, passwordLength);
		stun->turnPassword[passwordLength] = 0;
		stun->turnPasswordLength = passwordLength;
	}

	switch (turnFlags)
	{
		case ILibWebRTC_TURN_DISABLED:
		{
			stun->alwaysConnectTurn = 0;
			stun->alwaysUseTurn = 0;
			if (ILibTURN_IsConnectedToServer(stun->mTurnClientModule) == 0) { ILibTURN_DisconnectFromServer(stun->mTurnClientModule); }
			break;
		}
		case ILibWebRTC_TURN_ENABLED:
		{
			stun->alwaysConnectTurn = 1;
			stun->alwaysUseTurn = 0;
			break;
		}
		case ILibWebRTC_TURN_ALWAYS_RELAY:
		{
			stun->alwaysConnectTurn = 1;
			stun->alwaysUseTurn = 1;
			break;
		}
	}

	if (stun->alwaysConnectTurn != 0 && stun->turnUsername != NULL && stun->turnPassword != NULL)
	{
		ILibTURN_ConnectTurnServer(stun->mTurnClientModule, (struct sockaddr_in*)&(stun->mTurnServerAddress), stun->turnUsername, stun->turnUsernameLength, stun->turnPassword, stun->turnPasswordLength, NULL);
	}
}

void ILibWebRTC_DisableConsentFreshness(void *stunModule)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)stunModule;
	obj->consentFreshnessDisabled = 1;
}

void ILibStunClient_SetOptions(void* StunModule, SSL_CTX* securityContext, char* certThumbprintSha256)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)StunModule;
	obj->SecurityContext = securityContext;
	obj->CertThumbprint = certThumbprintSha256;
	obj->CertThumbprintLength = 32; // SHA256

	if (obj->SecurityContext != NULL)
	{
		SSL_CTX_set_ecdh_auto(obj->SecurityContext, 1);
		SSL_CTX_set_session_cache_mode(obj->SecurityContext, SSL_SESS_CACHE_OFF);
		SSL_CTX_set_read_ahead(obj->SecurityContext, 1);
		SSL_CTX_set_verify(obj->SecurityContext, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, ILibStunClient_dTLS_verify_callback);
	}
}

void* ILibStunClient_Start(void *Chain, unsigned short LocalPort, ILibStunClient_OnResult OnResult)
{
	struct ILibStun_Module *obj;

	if ((obj = (struct ILibStun_Module*)malloc(sizeof(struct ILibStun_Module))) == NULL) ILIBCRITICALEXIT(254);
	memset(obj, 0, sizeof(struct ILibStun_Module));

	ILibAddToChain(Chain, obj);

	obj->OnResult = OnResult;
	obj->LocalIf.sin_family = AF_INET;
	obj->LocalIf.sin_port = htons(LocalPort);
	obj->LocalIf6.sin6_family = AF_INET6;
	obj->LocalIf6.sin6_port = htons(LocalPort);
	obj->Chain = Chain;
	obj->Destroy = &ILibStun_OnDestroy;
	obj->UDP = ILibAsyncUDPSocket_CreateEx(Chain, ILibRUDP_MaxMTU, (struct sockaddr*)&(obj->LocalIf), ILibAsyncUDPSocket_Reuse_EXCLUSIVE, &ILibStun_OnUDP, NULL, obj);
	if (obj->UDP == NULL) { free(obj); return NULL; }
#ifdef WIN32
	obj->UDP6 = ILibAsyncUDPSocket_CreateEx(Chain, ILibRUDP_MaxMTU, (struct sockaddr*)&(obj->LocalIf6), ILibAsyncUDPSocket_Reuse_EXCLUSIVE, &ILibStun_OnUDP, NULL, obj);
#endif

	if (LocalPort == 0)
	{
		obj->LocalIf.sin_port = htons(ILibAsyncUDPSocket_GetLocalPort(obj->UDP));
#ifdef WIN32
		obj->LocalIf6.sin6_port = htons(ILibAsyncUDPSocket_GetLocalPort(obj->UDP6));
#endif
	}

	obj->Timer = ILibGetBaseTimer(Chain);
	obj->State = STUN_STATUS_CHECKING_UDP_CONNECTIVITY;
	util_random(32, obj->Secret); // Random used to generate integrity keys

	// Init TURN Client
	obj->mTurnClientModule = ILibTURN_CreateTurnClient(Chain, ILibWebRTC_OnTurnConnect, ILibWebRTC_OnTurnAllocate, ILibWebRTC_OnTurnDataIndication, ILibWebRTC_OnTurnChannelData);
	ILibTURN_SetTag(obj->mTurnClientModule, obj);

	g_stunModule = obj;
	return obj;
}

void ILibStun_OnTimeout(void *object)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)object;

	switch (obj->State)
	{
	default:
		break;
	case STUN_STATUS_CHECKING_UDP_CONNECTIVITY:
		// No UDP connectivity
		if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Unknown, NULL, obj->user); }
		ILibStun_OnCompleted(obj);
		break;
	case STUN_STATUS_CHECKING_FULL_CONE_NAT:
	{
		// We're not supposed to trigger this case. If we did, it means the StunSever we are using does not implement RFC5389/5780
		if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_RFC5780_NOT_IMPLEMENTED, (struct sockaddr_in*)&(obj->Public), obj->user); }
		ILibStun_OnCompleted(obj);
	}
		break;
	case STUN_STATUS_CHECKING_RESTRICTED_NAT:
		// Port Restricted NAT
		if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Port_Restricted_NAT, (struct sockaddr_in*)&(obj->Public), obj->user); }
		ILibStun_OnCompleted(obj);
		break;
	case STUN_STATUS_CHECKING_SYMETRIC_NAT:
		// Symetric NAT
		if (obj->OnResult != NULL) { obj->OnResult(obj, ILibStun_Results_Symetric_NAT, (struct sockaddr_in*)&(obj->Public), obj->user); }		// NOTE: Changed "ILibStun_Results_Unknown" that was here, is that ok??
		ILibStun_OnCompleted(obj);
		break;
	}
}

void ILibStunClient_PerformNATBehaviorDiscovery(void* StunModule, struct sockaddr_in* StunServer, void *user)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)StunModule;
	if (StunServer->sin_family != AF_INET) return;
	memcpy(&(obj->StunServer), StunServer, INET_SOCKADDR_LENGTH(StunServer->sin_family));
	obj->user = user;
	obj->State = STUN_STATUS_CHECKING_UDP_CONNECTIVITY;
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Performing NAT Behavior Discovery with: %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)StunServer), ntohs(StunServer->sin_port));
	ILib_Stun_SendAttributeChangeRequest(obj, (struct sockaddr*)&(obj->StunServer), 0x8000);
	ILibLifeTime_Add(obj->Timer, obj, ILibStunClient_TIMEOUT, &ILibStun_OnTimeout, NULL);
}
void ILibStunClient_PerformStun(void* StunModule, struct sockaddr_in* StunServer, void *user)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)StunModule;
	if (StunServer->sin_family != AF_INET) return;
	memcpy(&(obj->StunServer), StunServer, INET_SOCKADDR_LENGTH(StunServer->sin_family));
	obj->user = user;
	obj->State = STUN_STATUS_CHECKING_UDP_CONNECTIVITY;
	ILibRemoteLogging_printf(ILibChainGetLogger(obj->Chain), ILibRemoteLogging_Modules_WebRTC_STUN_ICE, ILibRemoteLogging_Flags_VerbosityLevel_1, "Performing STUN with: %s:%u", ILibRemoteLogging_ConvertAddress((struct sockaddr*)StunServer), ntohs(StunServer->sin_port));
	ILib_Stun_SendAttributeChangeRequest(obj, (struct sockaddr*)&(obj->StunServer), 0x00);
	ILibLifeTime_Add(obj->Timer, obj, ILibStunClient_TIMEOUT, &ILibStun_OnTimeout, NULL);
}

void ILibStunClient_SendData(void* StunModule, struct sockaddr* target, char* data, int datalen, enum ILibAsyncSocket_MemoryOwnership UserFree)
{
	if (target->sa_family == AF_INET) ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)StunModule)->UDP, (struct sockaddr*)target, data, datalen, UserFree);
	else if (target->sa_family == AF_INET6) ILibAsyncUDPSocket_SendTo(((struct ILibStun_Module*)StunModule)->UDP6, (struct sockaddr*)target, data, datalen, UserFree);
}

struct ILibTURN_TurnClientObject
{
	ILibChain_PreSelect PreSelect;
	ILibChain_PostSelect PostSelect;
	ILibChain_Destroy Destroy;
	ILibTURN_OnConnectTurnHandler OnConnectTurnCallback;
	ILibTURN_OnAllocateHandler OnAllocateCallback;
	ILibTURN_OnDataIndicationHandler OnDataIndicationCallback;
	ILibTURN_OnChannelDataHandler OnChannelDataCallback;

	void* TAG;
	void* tcpClient;
	char* username;
	int usernameLen;
	char* password;
	int passwordLen;
	void* transactionData;
	void* sendOkStack;

	char* currentNonce;
	int currentNonceLen;
	char* currentRealm;
	int currentRealmLen;
};

void ILibTURN_OnDestroy(void* object)
{
	struct ILibTURN_TurnClientObject* turn = (struct ILibTURN_TurnClientObject*)object;
	ILibDestroyHashTree(turn->transactionData);
}

void ILibTURN_SetTag(ILibTURN_ClientModule clientModule, void *tag)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)clientModule;
	turn->TAG = tag;
}

void* ILibTURN_GetTag(ILibTURN_ClientModule clientModule)
{
	return (((struct ILibTURN_TurnClientObject*)clientModule)->TAG);
}

int ILibTURN_IsConnectedToServer(ILibTURN_ClientModule clientModule)
{
	return (ILibAsyncSocket_IsConnected(((struct ILibTURN_TurnClientObject*)clientModule)->tcpClient));
}

void ILibTURN_DisconnectFromServer(ILibTURN_ClientModule clientModule)
{
	ILibAsyncSocket_Disconnect(((struct ILibTURN_TurnClientObject*)clientModule)->tcpClient);
}

void ILibTURN_ProcessChannelData(struct ILibTURN_TurnClientObject *turn, unsigned short ChannelNumber, char* buffer, int offset, int length)
{
	if (turn->OnChannelDataCallback != NULL) { turn->OnChannelDataCallback(turn, ChannelNumber, buffer, offset, length); }
}

STUN_TYPE ILibTURN_GetMethodType(char* buffer, int offset, int length)
{
	UNREFERENCED_PARAMETER(length);
	return ((STUN_TYPE)ntohs(((unsigned short*)(buffer + offset))[0]));
}

char* ILibTURN_GetTransactionID(char* buffer, int offset, int length)
{
	UNREFERENCED_PARAMETER(length);
	return (buffer + offset + 8);
}

int ILibTURN_GetAttributeValue(char* buffer, int offset, int length, STUN_ATTRIBUTES attribute, int index, char** value)
{
	unsigned short messageLength = 20 + ntohs(((unsigned short*)(buffer + offset))[1]);
	int i = 0;

	UNREFERENCED_PARAMETER(length);

	offset += 20;
	while (offset + 4 <= messageLength)												// Decode each attribute one at a time
	{
		STUN_ATTRIBUTES current = (STUN_ATTRIBUTES)ntohs(((unsigned short*)(buffer + offset))[0]);
		int attrLength = ntohs(((unsigned short*)(buffer + offset))[1]);
		if (offset + 4 + attrLength > messageLength) return 0;

		if (attribute == current)
		{
			if (i == index)
			{
				if (value != NULL) { *value = buffer + offset + 4; return attrLength; } else { return offset; }
			}
			else
			{
				++i;
			}
		}
		offset += (4 + FOURBYTEBOUNDARY(attrLength));					// Move the ptr forward by the attribute length plus padding.
	}
	return 0;
}

int ILibTURN_GetAttributeStartLocation(char *buffer, int offset, int length, STUN_ATTRIBUTES attribute, int index)
{
	return(ILibTURN_GetAttributeValue(buffer, offset, length, attribute, index, NULL));
}

int ILibTURN_GetAttributeCount(char* buffer, int offset, int length, STUN_ATTRIBUTES attribute)
{
	unsigned short messageLength = 20 + ntohs(((unsigned short*)(buffer + offset))[1]);
	int retVal = 0;

	UNREFERENCED_PARAMETER(length);

	offset += 20;
	while (offset + 4 <= messageLength)												// Decode each attribute one at a time
	{
		STUN_ATTRIBUTES current = (STUN_ATTRIBUTES)ntohs(((unsigned short*)(buffer + offset))[0]);
		int attrLength = ntohs(((unsigned short*)(buffer + offset))[1]);
		if (offset + 4 + attrLength > messageLength) return 0;

		if (attribute == current) { ++retVal; }

		offset += (4 + FOURBYTEBOUNDARY(attrLength));					// Move the ptr forward by the attribute length plus padding.
	}
	return retVal;
}

void ILibTURN_GenerateIntegrityKey(char* username, char* realm, char* password, char* integrityKey)
{
	char key[128];
	int keyLen = snprintf(key, 128, "%s:%s:%s", username, realm, password);
	util_md5(key, keyLen, integrityKey);
}

// integrity must be at least 20 in size
void ILibTURN_CalculateMessageIntegrity(char* buffer, int offset, int length, char* key, int keyLen, char* integrity)
{
	HMAC_CTX hmac;
	unsigned int hmaclen = 20;

	// Setup and perform HMAC-SHA1
	HMAC_CTX_init(&hmac);
	HMAC_Init_ex(&hmac, key, keyLen, EVP_sha1(), NULL);
	HMAC_Update(&hmac, (unsigned char*)buffer + offset, length);
	HMAC_Final(&hmac, (unsigned char*)integrity, &hmaclen); // Put the HMAC in the outgoing result location
	HMAC_CTX_cleanup(&hmac);
}

int ILibTURN_IsPacketAuthenticated(struct ILibTURN_TurnClientObject *turn, char* buffer, int offset, int length)
{
	char integrityKey[16];
	char integrity[20];
	char* fingerprint;
	int fingerprintLen;
	int fingerprintIndex;
	unsigned int calculated, actual;

	char* MessageIntegrityValue;
	int MessageIntegrityValueLen, MessageIntegrityPtr;

	unsigned short tempVal = 0;

	MessageIntegrityValueLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_MESSAGE_INTEGRITY, 0, &MessageIntegrityValue);
	if (MessageIntegrityValueLen == 0) { return 0; } // Anonymous packet, response with challenge

	MessageIntegrityPtr = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_MESSAGE_INTEGRITY, 0, NULL);
	fingerprintLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_FINGERPRINT, 0, &fingerprint);

	// Verify Authenticity, first check Fingerprint
	if (fingerprintLen > 0)
	{
		fingerprintIndex = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_FINGERPRINT, 0, NULL);
		actual = 0x5354554e ^ ntohl(((unsigned int*)fingerprint)[0]);
		calculated = ILibStun_CRC32(buffer + offset, fingerprintIndex);
		if (calculated != actual) { return 0; }
	}

	// Fix Length if there is a fingerprint
	if (fingerprintLen > 0)
	{
		tempVal = ntohs(((unsigned short*)(buffer + offset))[1]);
		((unsigned short*)(buffer + offset))[1] = htons(tempVal - 8);
	}

	ILibTURN_GenerateIntegrityKey(turn->username, turn->currentRealm, turn->password, integrityKey);
	ILibTURN_CalculateMessageIntegrity(buffer, offset, MessageIntegrityPtr, integrityKey, 16, integrity);

	// Put Length Back if we had to adjust the value due to fingerprint
	if (fingerprintLen > 0) { ((unsigned short*)(buffer + offset))[1] = htons(tempVal); }

	if (MessageIntegrityValueLen == 20 && memcmp(MessageIntegrityValue, integrity, 20) == 0) { return 1; }

	return 0;
}

void ILibTURN_ProcessStunFormattedPacket(struct ILibTURN_TurnClientObject *turn, char* buffer, int offset, int length)
{
	void* tmp;
	STUN_TYPE method = ILibTURN_GetMethodType(buffer, offset, length);
	char* TransactionID = ILibTURN_GetTransactionID(buffer, offset, length);

	if (!IS_INDICATION(method) && !IS_ERR_RESP(method) && !ILibTURN_IsPacketAuthenticated(turn, buffer, offset, length))
	{
		// Packet failed Authentication!
		return;
	}

	switch (method)
	{
		case TURN_CHANNEL_BIND_ERROR:
		{
			void **ptr;
			int val;

			ILibGetEntryEx(turn->transactionData, TransactionID, 12, (void**)&ptr, &val);
			if (ptr[0] != NULL) { ((ILibTURN_OnCreateChannelBindingHandler)ptr[0])(turn, (unsigned short)val, 0, ptr[1]); }
			free(ptr);
			ILibDeleteEntry(turn->transactionData, TransactionID, 12);
			break;
		}
		case TURN_CHANNEL_BIND_RESPONSE:
		{
			void **ptr;
			int val;

			ILibGetEntryEx(turn->transactionData, TransactionID, 12, (void**)&ptr, &val);
			if (ptr[0] != NULL) { ((ILibTURN_OnCreateChannelBindingHandler)ptr[0])(turn, (unsigned short)val, 1, ptr[1]); }
			free(ptr);
			ILibDeleteEntry(turn->transactionData, TransactionID, 12);
			break;
		}
		case TURN_DATA: // Data-Indication
		{
			char *remotePeer, *data;
			int remotePeerLen, dataLen;

			remotePeerLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_XOR_PEER_ADDRESS, 0, &remotePeer);
			dataLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_DATA, 0, &data);

			if (remotePeerLen > 0 && dataLen > 0)
			{
				if (turn->OnDataIndicationCallback != NULL)
				{
					struct sockaddr_in6 peer;
					ILibTURN_GetXORMappedAddress(remotePeer, remotePeerLen, TransactionID, &peer);
					turn->OnDataIndicationCallback(turn, &peer, data, 0, dataLen);
				}
			}
			break;
		}
		case TURN_CREATE_PERMISSION_ERROR:
		{
			void *ptr;
			int val;
			ILibGetEntryEx(turn->transactionData, TransactionID, 12, &ptr, &val);

			if (val == 0)
			{
				if (ptr != NULL) { ((ILibTURN_OnCreatePermissionHandler)ptr)(turn, 0, NULL); }
			}
			else
			{
				((ILibTURN_OnCreatePermissionHandler)((void**)ptr)[0])(turn, 0, ((ILibTURN_OnCreatePermissionHandler)((void**)ptr)[1]));
				free(ptr);
			}

			ILibDeleteEntry(turn->transactionData, TransactionID, 12);
			break;
		}
		case TURN_CREATE_PERMISSION_RESPONSE:
		{
			void *ptr;
			int val;
			ILibGetEntryEx(turn->transactionData, TransactionID, 12, &ptr, &val);

			if (val == 0)
			{
				if (ptr != NULL) { ((ILibTURN_OnCreatePermissionHandler)ptr)(turn, 1, NULL); }
			}
			else
			{
				((ILibTURN_OnCreatePermissionHandler)((void**)ptr)[0])(turn, 1, ((ILibTURN_OnCreatePermissionHandler)((void**)ptr)[1]));
				free(ptr);
			}
			ILibDeleteEntry(turn->transactionData, TransactionID, 12);
			break;
		}
		case TURN_ALLOCATE_ERROR:
		{
			if (ILibTURN_GetAttributeCount(buffer, offset, length, STUN_ATTRIB_REALM) == 1 && ILibTURN_GetAttributeCount(buffer, offset, length, STUN_ATTRIB_NONCE) == 1)
			{
				//
				// Resend the Allocate Request, but this time with Message Integrity
				//
				char* nonce;
				char* realm;
				int nonceLen, realmLen, inputKeyLen, packetPtr;
				char inputKey[128];
				char outputKey[16];
				char packet[256];
				char NewTransactionID[12];
				char Transport[4];
				ILibTURN_TransportTypes transport;

				if (ILibHasEntry(turn->transactionData, TransactionID, 12))
				{
					nonceLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_NONCE, 0, &nonce);
					realmLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_REALM, 0, &realm);

					if (turn->currentNonce != NULL) { free(turn->currentNonce); }
					if ((turn->currentNonce = (char*)malloc(nonceLen + 1)) == NULL){ ILIBCRITICALEXIT(254); }
					memcpy(turn->currentNonce, nonce, nonceLen);
					turn->currentNonce[nonceLen] = 0;
					turn->currentNonceLen = nonceLen;

					if (turn->currentRealm != NULL) { free(turn->currentRealm); }
					if ((turn->currentRealm = (char*)malloc(realmLen + 1)) == NULL){ ILIBCRITICALEXIT(254); }
					memcpy(turn->currentRealm, realm, realmLen);
					turn->currentRealm[realmLen] = 0;
					turn->currentRealmLen = realmLen;

					inputKeyLen = snprintf(inputKey, 128, "%s:%s:%s", turn->username, turn->currentRealm, turn->password);

					util_md5(inputKey, inputKeyLen, outputKey);
					util_random(12, NewTransactionID);
					ILibGetEntryEx(turn->transactionData, TransactionID, 12, &tmp, (int*)(&transport));
					ILibDeleteEntry(turn->transactionData, TransactionID, 12); // We're going to delete our TransactionID, without adding the new one, so if we fail again, we'll abort
					((int*)Transport)[0] = 0;
					Transport[0] = (char)transport;

					packetPtr = ILibTURN_GenerateStunFormattedPacketHeader(packet, TURN_ALLOCATE, NewTransactionID);
					packetPtr += ILibTURN_AddAttributeToStunFormattedPacketHeader(packet, packetPtr, STUN_ATTRIB_REQUESTED_TRANSPORT, Transport, 4);
					packetPtr += ILibTURN_AddAttributeToStunFormattedPacketHeader(packet, packetPtr, STUN_ATTRIB_NONCE, nonce, nonceLen);
					packetPtr += ILibTURN_AddAttributeToStunFormattedPacketHeader(packet, packetPtr, STUN_ATTRIB_REALM, realm, realmLen);
					packetPtr += ILibTURN_AddAttributeToStunFormattedPacketHeader(packet, packetPtr, STUN_ATTRIB_USERNAME, turn->username, turn->usernameLen);

					packetPtr += ILibStun_AddMessageIntegrityAttr(packet, packetPtr, outputKey, 16);
					packetPtr += ILibStun_AddFingerprint(packet, packetPtr);

					ILibAsyncSocket_Send(turn->tcpClient, packet, packetPtr, ILibAsyncSocket_MemoryOwnership_USER);
				}
				else
				{
					// TransactionID was not found. This means we already tried to authenticate, but it has failed.
					// Nothing we can do, but error out.
					if (turn->OnAllocateCallback != NULL) { turn->OnAllocateCallback(turn, 0, NULL); }
				}
				return;
			}
			break;
		}
		case TURN_ALLOCATE_RESPONSE:
		{
			char *val, *transportAddress;
			int valLen, transportAddressLen;
			struct sockaddr_in6 address;

			valLen = ILibTURN_GetAttributeValue(buffer, offset, length, TURN_LIFETIME, 0, &val);
			transportAddressLen = ILibTURN_GetAttributeValue(buffer, offset, length, STUN_ATTRIB_XOR_RELAY_ADDRESS, 0, &transportAddress);
			if (transportAddressLen > 0)
			{
				ILibTURN_GetXORMappedAddress(transportAddress, transportAddressLen, TransactionID, &address);
			}
			if (valLen > 0 && transportAddressLen > 0)
			{
				int lifetime = ntohl(((unsigned int*)val)[0]);
				if (turn->OnAllocateCallback != NULL) { turn->OnAllocateCallback(turn, lifetime, &address); }
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

int ILibTURN_GetStunPacketLength(char* buffer, int offset, int length)
{
	unsigned short messageLength = 20 + ntohs(((unsigned short*)(buffer + offset))[1]);
	int magic = ntohl(((unsigned int*)(buffer + offset))[1]);
	if (messageLength > length || magic != 0x2112A442) // Check the length and magic string
	{
		if (magic != 0x2112A442) return ((int)messageLength);
	}
	return 0;
}

void ILibTURN_TCP_OnData(ILibAsyncSocket_SocketModule socketModule, char* buffer, int *p_beginPointer, int endPointer, ILibAsyncSocket_OnInterrupt* OnInterrupt, void **user, int *PAUSE)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)*user;

	UNREFERENCED_PARAMETER(socketModule);
	UNREFERENCED_PARAMETER(OnInterrupt);
	UNREFERENCED_PARAMETER(PAUSE);

	if (endPointer >= 4)
	{
		if (ntohs(((unsigned short*)(buffer + *p_beginPointer))[0]) >> 14 == 1)
		{
			// This is Channel Data
			unsigned short ChannelNumber = (unsigned short)((int)ntohs(((unsigned short*)(buffer + *p_beginPointer))[0]) ^ (int)0x4000);
			unsigned short ChannelDataLength = ntohs(((unsigned short*)(buffer + *p_beginPointer))[1]);
			if (endPointer >= (4 + FOURBYTEBOUNDARY(ChannelDataLength)))
			{
				ILibTURN_ProcessChannelData(turn, ChannelNumber, buffer, *p_beginPointer + 4, ChannelDataLength);
				*p_beginPointer += (4 + FOURBYTEBOUNDARY(ChannelDataLength));
				return;
			}
		}
		else
		{
			if (endPointer >= 8)
			{
				// STUN Formatted Packet
				int stunLength = ILibTURN_GetStunPacketLength(buffer, *p_beginPointer, 8);
				if (endPointer >= stunLength)
				{
					ILibTURN_ProcessStunFormattedPacket(turn, buffer, *p_beginPointer, stunLength);
					*p_beginPointer += stunLength;
				}
			}
		}
	}
}

void ILibTURN_TCP_OnConnect(ILibAsyncSocket_SocketModule socketModule, int Connected, void *user)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)user;

	UNREFERENCED_PARAMETER(socketModule);

	if (turn->OnConnectTurnCallback != NULL) { turn->OnConnectTurnCallback(turn, Connected); }
}

void ILibTURN_TCP_OnDisconnect(ILibAsyncSocket_SocketModule socketModule, void *user)
{
	UNREFERENCED_PARAMETER(socketModule);
	UNREFERENCED_PARAMETER(user);

	// We aren't doing anything here, because when the user goes to send something, the return value will reflect that this disconnected
}

ILibTURN_ClientModule ILibTURN_CreateTurnClient(void* chain, ILibTURN_OnConnectTurnHandler OnConnectTurn, ILibTURN_OnAllocateHandler OnAllocate, ILibTURN_OnDataIndicationHandler OnData, ILibTURN_OnChannelDataHandler OnChannelData)
{
	struct ILibTURN_TurnClientObject* retVal = (struct ILibTURN_TurnClientObject*)malloc(sizeof(struct ILibTURN_TurnClientObject));
	if (retVal == NULL){ ILIBCRITICALEXIT(254); }
	memset(retVal, 0, sizeof(struct ILibTURN_TurnClientObject));
	retVal->Destroy = &ILibTURN_OnDestroy;
	retVal->tcpClient = ILibCreateAsyncSocketModule(chain, 4096, &ILibTURN_TCP_OnData, &ILibTURN_TCP_OnConnect, &ILibTURN_TCP_OnDisconnect, NULL);
	retVal->OnConnectTurnCallback = OnConnectTurn;
	retVal->OnAllocateCallback = OnAllocate;
	retVal->OnDataIndicationCallback = OnData;
	retVal->OnChannelDataCallback = OnChannelData;
	retVal->transactionData = ILibInitHashTree();

	ILibAddToChain(chain, retVal);

	return retVal;
}

void ILibTURN_ConnectTurnServer(ILibTURN_ClientModule turnModule, struct sockaddr_in* turnServer, char* username, int usernameLen, char* password, int passwordLen, struct sockaddr_in* proxyServer)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*) turnModule;

	UNREFERENCED_PARAMETER(proxyServer);

	if ((turn->username = (char*)malloc(usernameLen + 1)) == NULL){ ILIBCRITICALEXIT(254); }
	if ((turn->password = (char*)malloc(passwordLen + 1)) == NULL){ ILIBCRITICALEXIT(254); }
	memcpy(turn->username, username, usernameLen);
	memcpy(turn->password, password, passwordLen);
	turn->username[usernameLen] = 0;
	turn->password[passwordLen] = 0;
	turn->usernameLen = usernameLen;
	turn->passwordLen = passwordLen;

	ILibAsyncSocket_ConnectTo(turn->tcpClient, NULL, (struct sockaddr*)turnServer, NULL, turn);
}

int ILibTURN_GenerateStunFormattedPacketHeader(char* rbuffer, STUN_TYPE packetType, char* transactionID)
{
	((unsigned short*)rbuffer)[0] = htons(packetType);								// Allocate, skip setting length for now
	((unsigned int*)rbuffer)[1] = htonl(0x2112A442);								// Set the magic string																								
	memcpy(rbuffer + 8, transactionID, 12);
	return 20;
}

int ILibTURN_AddAttributeToStunFormattedPacketHeader(char* rbuffer, int rptr, STUN_ATTRIBUTES attrType, char* data, int dataLen)
{
	((unsigned short*)(rbuffer + rptr))[0] = htons(attrType);					// Attribute header
	((unsigned short*)(rbuffer + rptr))[1] = htons((unsigned short)dataLen);	// Attribute length
	if (dataLen > 0) { memcpy(rbuffer + rptr + 4, data, dataLen); }
	return(ILibAlignOnFourByteBoundary(rbuffer+rptr, 4+dataLen));
}

void ILibTURN_GetXORMappedAddress(char* buffer, int bufferLen, char* transactionID, struct sockaddr_in6 *value)
{
	int i;

	UNREFERENCED_PARAMETER(bufferLen);

	switch (buffer[1])
	{
	case 1: // IPv4
	{
		int addr = ntohl(((unsigned int*)buffer)[1]) ^ 0x2112A442;
		memset(value, 0, INET_SOCKADDR_LENGTH(AF_INET));
		value->sin6_family = AF_INET;
		((struct sockaddr_in*)value)->sin_addr.s_addr = htonl(addr);
	}
		break;
	case 2: // IPv6
	{
		unsigned char addr[16];
		unsigned char material[16];
		((unsigned int*)material)[0] = 0x2112A442;
		memcpy(material + 4, transactionID, 12);
		memset(value, 0, INET_SOCKADDR_LENGTH(AF_INET6));
		value->sin6_family = AF_INET6;

		if (htonl(16) == 16)
		{
			for (i = 0; i < 16; ++i) { value->sin6_addr.s6_addr[i] = (buffer + 4)[i] ^ material[i]; }
		}
		else
		{
			for (i = 0; i < 16; ++i) { addr[i] = material[i] ^ (buffer + 4)[15 - i]; value->sin6_addr.s6_addr[15 - i] = addr[i]; }
		}
	}
		break;
	}
	value->sin6_port = htons(ntohs(((unsigned short*)buffer)[1]) ^ (unsigned short)((unsigned int)0x2112A442 >> 16));
}

int ILibTURN_CreateXORMappedAddress(struct sockaddr_in6 *endpoint, char* buffer, char* transactionID)
{
	int i, retVal;
	unsigned short port = ntohs(endpoint->sin6_port);
	port = port ^ (unsigned short)((unsigned int)0x2112A442 >> 16);

	buffer[0] = 0;
	buffer[1] = endpoint->sin6_family == AF_INET ? 1 : 2;
	((unsigned short*)buffer)[1] = htons(port);

	switch (endpoint->sin6_family)
	{
	case AF_INET:
		((unsigned int*)buffer)[1] = htonl(ntohl(((struct sockaddr_in*)endpoint)->sin_addr.s_addr) ^ 0x2112A442);
		retVal = 8;
		break;
	case AF_INET6:
	{
		unsigned char addr[16];
		unsigned char material[16];
		((unsigned int*)material)[0] = 0x2112A442;
		memcpy(material + 4, transactionID, 12);

		if (htonl(16) == 16)
		{
			for (i = 0; i < 16; ++i) { endpoint->sin6_addr.s6_addr[i] = endpoint->sin6_addr.s6_addr[i] ^ material[i]; }
		}
		else
		{
			for (i = 0; i < 16; ++i) { addr[i] = endpoint->sin6_addr.s6_addr[15 - i] ^ material[i]; }
			for (i = 0; i < 16; ++i) { endpoint->sin6_addr.s6_addr[15 - i] = addr[i]; }
		}

		memcpy(buffer + 4, endpoint->sin6_addr.s6_addr, 16);
		retVal = 20;
	}
		break;
	default:
		retVal = 0;
		break;
	}

	return retVal;
}

void ILibTURN_Allocate(ILibTURN_ClientModule turnModule, ILibTURN_TransportTypes transportType)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)turnModule;
	char rbuffer[256];
	char TransactionID[12];
	int rptr;
	char Transport[4];

	((int*)Transport)[0] = 0;
	Transport[0] = (char)transportType;

	util_random(12, TransactionID);																	// Random used for transaction id													
	rptr = ILibTURN_GenerateStunFormattedPacketHeader(rbuffer, TURN_ALLOCATE, TransactionID);
	rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_REQUESTED_TRANSPORT, Transport, 4);
	rptr += ILibStun_AddFingerprint(rbuffer, rptr);													// Set the length in this function

	ILibAddEntryEx(turn->transactionData, TransactionID, 12, NULL, (int)transportType);

	ILibAsyncSocket_Send(turn->tcpClient, rbuffer, rptr, ILibAsyncSocket_MemoryOwnership_USER);
}

void ILibTURN_CreatePermission(ILibTURN_ClientModule turnModule, struct sockaddr_in6* permissions, int permissionsLength, ILibTURN_OnCreatePermissionHandler result, void *user)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)turnModule;
	char peer[20];
	char rbuffer[256];
	char TransactionID[12];
	int rptr, peerLen;
	int i;
	char integrityKey[16];

	util_random(12, TransactionID);																	// Random used for transaction id	
	ILibTURN_GenerateIntegrityKey(turn->username, turn->currentRealm, turn->password, integrityKey);

	rptr = ILibTURN_GenerateStunFormattedPacketHeader(rbuffer, TURN_CREATE_PERMISSION, TransactionID);
	for (i = 0; i < permissionsLength; ++i)
	{
		peerLen = ILibTURN_CreateXORMappedAddress(&(permissions[i]), peer, TransactionID);
		rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_XOR_PEER_ADDRESS, peer, peerLen);
	}

	rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_USERNAME, turn->username, turn->usernameLen);
	rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_REALM, turn->currentRealm, turn->currentRealmLen);
	rptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(rbuffer, rptr, STUN_ATTRIB_NONCE, turn->currentNonce, turn->currentNonceLen);

	rptr += ILibStun_AddMessageIntegrityAttr(rbuffer, rptr, integrityKey, 16);
	rptr += ILibStun_AddFingerprint(rbuffer, rptr);

	if (result != NULL)
	{
		if (user != NULL)
		{
			void **val = (void**)malloc(2 * sizeof(void*));
			if (val == NULL){ ILIBCRITICALEXIT(254); }
			val[0] = result;
			val[1] = user;
			ILibAddEntryEx(turn->transactionData, TransactionID, 12, val, 2);
		}
		else
		{
			ILibAddEntryEx(turn->transactionData, TransactionID, 12, result, 0);
		}
	}

	ILibAsyncSocket_Send(turn->tcpClient, rbuffer, rptr, ILibAsyncSocket_MemoryOwnership_USER);
}

enum ILibAsyncSocket_SendStatus ILibTURN_SendIndication(ILibTURN_ClientModule turnModule, struct sockaddr_in6* remotePeer, char* buffer, int offset, int length)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)turnModule;
	char Address[20];
	char TransactionID[12];
	char *packet = (char*)malloc(50 + length);
	int ptr, AddressLen;

	if (packet == NULL){ ILIBCRITICALEXIT(254); }
	util_random(12, TransactionID);
	ptr = ILibTURN_GenerateStunFormattedPacketHeader(packet, TURN_SEND, TransactionID);
	AddressLen = ILibTURN_CreateXORMappedAddress(remotePeer, Address, TransactionID);

	ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(packet, ptr, STUN_ATTRIB_XOR_PEER_ADDRESS, Address, AddressLen);
	ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(packet, ptr, STUN_ATTRIB_DATA, buffer + offset, length);
	ptr += ILibStun_AddFingerprint(packet, ptr);

	return ILibAsyncSocket_Send(turn->tcpClient, packet, ptr, ILibAsyncSocket_MemoryOwnership_CHAIN);
}

void ILibTURN_CreateChannelBinding(ILibTURN_ClientModule turnModule, unsigned short channelNumber, struct sockaddr_in6* remotePeer, ILibTURN_OnCreateChannelBindingHandler result, void* user)
{
	char TransactionID[12];
	char Peer[20];
	char IntegrityKey[16];
	char Packet[256];
	char Channel[4];
	int Ptr, PeerLen;

	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)turnModule;

	util_random(12, TransactionID);
	PeerLen = ILibTURN_CreateXORMappedAddress(remotePeer, Peer, TransactionID);
	ILibTURN_GenerateIntegrityKey(turn->username, turn->currentRealm, turn->password, IntegrityKey);
	((unsigned short*)Channel)[0] = htons(channelNumber ^ 0x4000);
	((unsigned short*)Channel)[1] = 0;

	Ptr = ILibTURN_GenerateStunFormattedPacketHeader(Packet, TURN_CHANNEL_BIND, TransactionID);
	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_XOR_PEER_ADDRESS, Peer, PeerLen);
	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, TURN_CHANNEL_NUMBER, Channel, 4);
	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_USERNAME, turn->username, turn->usernameLen);
	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_REALM, turn->currentRealm, turn->currentRealmLen);
	Ptr += ILibTURN_AddAttributeToStunFormattedPacketHeader(Packet, Ptr, STUN_ATTRIB_NONCE, turn->currentNonce, turn->currentNonceLen);

	Ptr += ILibStun_AddMessageIntegrityAttr(Packet, Ptr, IntegrityKey, 16);
	Ptr += ILibStun_AddFingerprint(Packet, Ptr);

	if (result != NULL)
	{
		// Only malloc memory if the user actually cares to get called back
		void **u = (void**)malloc(2 * sizeof(void*));
		if (u == NULL){ ILIBCRITICALEXIT(254); }
		u[0] = result;
		u[1] = (void*)user;
		ILibAddEntryEx(turn->transactionData, TransactionID, 12, u, (int)channelNumber);
	}

	ILibAsyncSocket_Send(turn->tcpClient, Packet, Ptr, ILibAsyncSocket_MemoryOwnership_USER);
}

unsigned int ILibTURN_GetPendingBytesToSend(ILibTURN_ClientModule turnModule)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)turnModule;
	return ILibAsyncSocket_GetPendingBytesToSend(turn->tcpClient);
}

enum ILibAsyncSocket_SendStatus ILibTURN_SendChannelData(ILibTURN_ClientModule turnModule, unsigned short channelNumber, char* buffer, int offset, int length)
{
	struct ILibTURN_TurnClientObject *turn = (struct ILibTURN_TurnClientObject*)turnModule;
	char header[4];
	enum ILibAsyncSocket_SendStatus retVal;

	((unsigned short*)header)[0] = htons(channelNumber ^ 0x4000);
	((unsigned short*)header)[1] = htons((unsigned short)length);

	ILibAsyncSocket_Send(turn->tcpClient, header, 4, ILibAsyncSocket_MemoryOwnership_USER);
	retVal = ILibAsyncSocket_Send(turn->tcpClient, buffer + offset, length, ILibAsyncSocket_MemoryOwnership_USER);
	if (length % 4 > 0) { retVal = ILibAsyncSocket_Send(turn->tcpClient, header, 4 - (length % 4), ILibAsyncSocket_MemoryOwnership_USER); }
	return retVal;
}

#ifdef _WEBRTCDEBUG
void ILibSCTP_SetSimulatedInboundLossPercentage(void *stunModule, int lossPercentage)
{
	struct ILibStun_Module *obj = (struct ILibStun_Module*)stunModule;
	obj->lossPercentage = lossPercentage;
}

void ILibSCTP_SetTSNCallback(void *dtlsSession, ILibSCTP_OnTSNChanged tsnHandler)
{
	struct ILibStun_dTlsSession *session = (struct ILibStun_dTlsSession*)dtlsSession;
	session->onTSNChanged = tsnHandler;
}

int ILibSCTP_Debug_SetDebugCallback(void* dtlsSession, char* debugFieldName, ILibSCTP_OnSCTPDebug handler)
{
	struct ILibStun_dTlsSession *session = (struct ILibStun_dTlsSession*)dtlsSession;

	if (strcmp(debugFieldName, "OnHold") == 0)
	{
		session->onHold = handler;
		handler(session, "OnHold", session->holdingCount);
	}
	else if (strcmp(debugFieldName, "OnReceiverCredits") == 0)
	{
		session->onReceiverCredits = handler;
		handler(session, "OnReceiverCredits", session->receiverCredits);
	}
	else if (strcmp(debugFieldName, "OnSendFastRetry") == 0)
	{
		session->onSendFastRetry = handler;
	}
	else if (strcmp(debugFieldName, "OnSendRetry") == 0)
	{
		session->onSendRetry = handler;
	}
	else if (strcmp(debugFieldName, "OnCongestionWindowSizeChanged") == 0)
	{
		session->onCongestionWindowSizeChanged = handler;
		handler(session, "OnCongestionWindowSizeChanged", session->congestionWindowSize);
	}
	else if (strcmp(debugFieldName, "OnSACKReceived") == 0)
	{
		session->onSACKreceived = handler;
	}
	else if (strcmp(debugFieldName, "OnT3RTX") == 0)
	{
		session->onT3RTX = handler;
	}
	else if (strcmp(debugFieldName, "OnFastRecovery") == 0)
	{
		session->onFastRecovery = handler;
	}
	else if (strcmp(debugFieldName, "OnRTTCalculated") == 0)
	{
		session->onRTTCalculated = handler;
	}
	else if (strcmp(debugFieldName, "OnTSNFloorNotRaised") == 0)
	{
		session->onTSNFloorNotRaised = handler;
	}
	else { return 1; }
	return 0;
}
#endif

/* crc32c.c -- compute CRC-32C using the Intel crc32 instruction
* Copyright (C) 2013 Mark Adler
* Version 1.1  1 Aug 2013  Mark Adler
*/

/*
This software is provided 'as-is', without any express or implied
warranty.  In no event will the author be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Mark Adler
madler@alumni.caltech.edu
*/

// Software only version of CRC32C

// Table for a quadword-at-a-time software crc. 
int crc32c_once_sw = 0;
unsigned int crc32c_table[8][256];

// Construct table for software CRC-32C calculation.
void crc32c_init_sw(void)
{
	unsigned int n, crc, k;
	for (n = 0; n < 256; n++) { crc = n; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1; crc32c_table[0][n] = crc; }
	for (n = 0; n < 256; n++) { crc = crc32c_table[0][n]; for (k = 1; k < 8; k++) { crc = crc32c_table[0][crc & 0xff] ^ (crc >> 8); crc32c_table[k][n] = crc; } }
}

// Table-driven software version as a fall-back.  This is about 15 times slower than using the hardware instructions.  This assumes little-endian integers, as is the case on Intel processors that the assembler code here is for.
unsigned int crc32c(unsigned int crci, const void *buf, unsigned int len)
{
	unsigned long long crc;
	const unsigned char *next = (const unsigned char*)buf;
	if (crc32c_once_sw == 0) { crc32c_init_sw(); crc32c_once_sw = 1; }
	crc = crci ^ 0xffffffff;
	while (len && ((uintptr_t)next & 7) != 0) { crc = (unsigned long long)crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8); len--; }
	while (len >= 8) { crc ^= *(unsigned long long *)next; crc = crc32c_table[7][crc & 0xff] ^ crc32c_table[6][(crc >> 8) & 0xff] ^ crc32c_table[5][(crc >> 16) & 0xff] ^ crc32c_table[4][(crc >> 24) & 0xff] ^ crc32c_table[3][(crc >> 32) & 0xff] ^ crc32c_table[2][(crc >> 40) & 0xff] ^ crc32c_table[1][(crc >> 48) & 0xff] ^ crc32c_table[0][crc >> 56]; next += 8; len -= 8; }
	while (len) { crc = crc32c_table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8); len--; }
	return (unsigned int)crc ^ 0xffffffff;
}

