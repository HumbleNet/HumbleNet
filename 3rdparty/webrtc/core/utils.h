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

#ifndef __MeshUtils__
#define __MeshUtils__

#include "../Microstack/ILibParsers.h"
#include <openssl/pem.h>
//#include <openssl/err.h>
#include <openssl/pkcs7.h>
#include <openssl/pkcs12.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/engine.h>

#if defined(WIN32) && !defined(snprintf)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif

#if !defined(WIN32) 
#define __fastcall
#endif

// Debugging features
#if defined(_DEBUG)
	// Display & log
    //char spareDebugMemory[4000];
    //int  spareDebugLen;
    //#define MSG(x) printf("%s",x);//mdb_addevent(x, (int)strlen(x));
	//#define MSG(...) spareDebugLen = snprintf(spareDebugMemory, 4000, __VA_ARGS__);printf("%s",spareDebugMemory);//mdb_addevent(spareDebugMemory, spareDebugLen);

	// Display only
	#ifndef MSG
		#define MSG(...) printf(__VA_ARGS__);fflush(NULL)
	#endif
	#define DEBUGSTATEMENT(x) x
	//#if defined(_POSIX)
		//#include <mcheck.h>
	//#endif
#else
	#ifndef MSG
		#define MSG(...)
	#endif
	#define DEBUGSTATEMENT(x)
#endif

#define UTIL_HASHSIZE     32
#define MAX_TOKEN_SIZE    1024
#define SMALL_TOKEN_SIZE  256
#define NONCE_SIZE        32
#define HALF_NONCE_SIZE   16

enum CERTIFICATE_TYPES
{
	CERTIFICATE_ROOT = 1,
	CERTIFICATE_TLS_SERVER = 2,
	CERTIFICATE_TLS_CLIENT = 3
};

// Certificate structure
struct util_cert
{
	X509 *x509;
	EVP_PKEY *pkey;
};

// General methods
void  __fastcall util_openssl_init();
void  __fastcall util_openssl_uninit();
void  __fastcall util_free(char* ptr);
char* __fastcall util_tohex(char* data, int len, char* out);
char* __fastcall util_tohex2(char* data, int len, char* out);
int   __fastcall util_hexToint(char *hexString, int hexStringLength);
int __fastcall util_hexToBuf(char *hexString, int hexStringLength, char* output);

unsigned long long __fastcall util_gettime();
void  __fastcall util_startChronometer();
long  __fastcall util_readChronometer();
int   __fastcall util_isnumeric(const char* str);

// File and data methods
//int    util_compress(char* inbuf, unsigned int inbuflen, char** outbuf, unsigned int headersize);
//int    util_decompress(char* inbuf, unsigned int inbuflen, char** outbuf, unsigned int headersize);
size_t __fastcall util_writefile(char* filename, char* data, int datalen);
size_t __fastcall util_appendfile(char* filename, char* data, int datalen);
size_t __fastcall util_readfile(char* filename, char** data, size_t maxlen);
int    __fastcall util_deletefile(char* filename);
#ifdef _POSIX
int __fastcall util_readfile2(char* filename, char** data);
#endif

// Certificate & crypto methods
void  __fastcall util_freecert(struct util_cert* cert);
int   __fastcall util_to_p12(struct util_cert cert, char *password, char** data);
int   __fastcall util_from_p12(char* data, int datalen, char* password, struct util_cert* cert);
int   __fastcall util_to_cer(struct util_cert cert, char** data);
int   __fastcall util_from_cer(char* data, int datalen, struct util_cert* cert);
int   __fastcall util_from_pem(char* filename, struct util_cert* cert);
int   __fastcall util_mkCert(struct util_cert *rootcert, struct util_cert* cert, int bits, int days, char* name, enum CERTIFICATE_TYPES certtype, struct util_cert* initialcert);
void  __fastcall util_printcert(struct util_cert cert);
void  __fastcall util_printcert_pk(struct util_cert cert);
void  __fastcall util_md5(char* data, int datalen, char* result);
void  __fastcall util_md5hex(char* data, int datalen, char *out);
void  __fastcall util_sha256(char* data, int datalen, char* result);
int   __fastcall util_sha256file(char* filename, char* result);
int   __fastcall util_keyhash(struct util_cert cert, char* result);
int   __fastcall util_keyhash2(X509* cert, char* result);
int   __fastcall util_sign(struct util_cert cert, char* data, int datalen, char** signature);
int   __fastcall util_verify(char* signature, int signlen, struct util_cert* cert, char** data);
int   __fastcall util_encrypt(struct util_cert cert, char* data, int datalen, char** encdata);
int   __fastcall util_encrypt2(STACK_OF(X509) *certs, char* data, int datalen, char** encdata);
int   __fastcall util_decrypt(char* encdata, int encdatalen, struct util_cert cert, char** data);
void  __fastcall util_random(int length, char* result);
void  __fastcall util_randomtext(int length, char* result);
int   __fastcall util_rsaencrypt(X509 *cert, char* data, int datalen, char** encdata);
int   __fastcall util_rsadecrypt(struct util_cert cert, char* data, int datalen, char** decdata);
int   __fastcall util_rsaverify(X509 *cert, char* data, int datalen, char* sign, int signlen);

// Symetric Crypto methods
void  __fastcall util_nodesessionkey(char* nodeid, char* key);
int   __fastcall util_cipher(char* key, unsigned int iv, char* nodeid, char* data, int datalen, char** result, int sendresponsekey);
int   __fastcall util_decipher(char* data, int datalen, char** result, char* nodeid);
void  __fastcall util_genusagekey(char* inkey, char* outkey, char usage);

// Network security methods
int	  __fastcall util_antiflood(int rate, int interval);
int   __fastcall util_ExtractWwwAuthenticate(char *wwwAuthenticate, void *request);
void  __fastcall util_GenerateAuthorizationHeader(void *request, char *requestType, char *uri, char *authorization, char* cnonce);
unsigned int __fastcall util_getiv();
int   __fastcall util_checkiv(unsigned int iv);

#ifdef WIN32
int   __fastcall util_crc(unsigned char *buffer, int len, int initial_value);
#endif

#endif

