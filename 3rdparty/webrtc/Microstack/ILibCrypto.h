#ifndef __ILIB_CRYPTO__
#define __ILIB_CRYPTO__

#include "ILibParsers.h"

#if !defined(WIN32) 
#define __fastcall
#endif

void  __fastcall util_md5(char* data, int datalen, char* result);
void  __fastcall util_md5hex(char* data, int datalen, char *out);
void  __fastcall util_sha1(char* data, int datalen, char* result);
char* __fastcall util_tohex(char* data, int len, char* out);
char* __fastcall util_tohex2(char* data, int len, char* out);
char* __fastcall util_tohex_lower(char* data, int len, char* out);
int   __fastcall util_hexToint(char *hexString, int hexStringLength);
int __fastcall util_hexToBuf(char *hexString, int hexStringLength, char* output);

void  __fastcall util_sha256(char* data, int datalen, char* result);
int   __fastcall util_sha256file(char* filename, char* result);

// File and data methods
size_t __fastcall util_writefile(char* filename, char* data, int datalen);
size_t __fastcall util_appendfile(char* filename, char* data, int datalen);
size_t __fastcall util_readfile(char* filename, char** data, size_t maxlen);
int    __fastcall util_deletefile(char* filename);

#ifdef WIN32
int   __fastcall util_crc(unsigned char *buffer, int len, int initial_value);
BOOL util_MoveFile(_In_ LPCSTR lpExistingFileName, _In_  LPCSTR lpNewFileName);
BOOL util_CopyFile(_In_ LPCSTR lpExistingFileName, _In_ LPCSTR lpNewFileName, _In_ BOOL bFailIfExists);
#endif

void  __fastcall util_random(int length, char* result);
void  __fastcall util_randomtext(int length, char* result);

#define UTIL_HASHSIZE     32
#define NONCE_SIZE        32
#define HALF_NONCE_SIZE   16

#ifdef MICROSTACK_NOTLS
#include "md5.h"
#include "sha1.h"
#include "microstack/SHA256.h"

#define SHA256_CTX struct sha256_ctx
#define SHA256_Init(ctx) __sha256_init_ctx(ctx)
#define SHA256_Update(ctx, data, len) __sha256_process_bytes(data, len, ctx)
#define SHA256_Final(md, ctx) __sha256_finish_ctx(ctx, md)
#endif


#ifndef MICROSTACK_NOTLS
#include <openssl/ssl.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/err.h>

// Certificate structure
typedef struct util_cert
{
	X509 *x509;
	EVP_PKEY *pkey;
}util_cert;
typedef enum CERTIFICATE_TYPES
{
	CERTIFICATE_ROOT = 1,
	CERTIFICATE_TLS_SERVER = 2,
	CERTIFICATE_TLS_CLIENT = 3
}CERTIFICATE_TYPES;


void  __fastcall util_openssl_init();
void  __fastcall util_openssl_uninit();
void  __fastcall util_free(char* ptr);

// Certificate & crypto methods
void  __fastcall util_freecert(struct util_cert* cert);
int   __fastcall util_to_p12(struct util_cert cert, char *password, char** data);
int   __fastcall util_from_p12(char* data, int datalen, char* password, struct util_cert* cert);
int   __fastcall util_to_cer(struct util_cert cert, char** data);
int   __fastcall util_from_cer(char* data, int datalen, struct util_cert* cert);
int   __fastcall util_from_pem(char* filename, struct util_cert* cert);
int	  __fastcall util_from_pem_string(char *data, int datalen, struct util_cert* cert);
int   __fastcall util_mkCert(struct util_cert *rootcert, struct util_cert* cert, int bits, int days, char* name, enum CERTIFICATE_TYPES certtype, struct util_cert* initialcert);
void  __fastcall util_printcert(struct util_cert cert);
void  __fastcall util_printcert_pk(struct util_cert cert);

int   __fastcall util_keyhash(struct util_cert cert, char* result);
int   __fastcall util_keyhash2(X509* cert, char* result);
int   __fastcall util_sign(struct util_cert cert, char* data, int datalen, char** signature);
int   __fastcall util_verify(char* signature, int signlen, struct util_cert* cert, char** data);
int   __fastcall util_encrypt(struct util_cert cert, char* data, int datalen, char** encdata);
int   __fastcall util_encrypt2(STACK_OF(X509) *certs, char* data, int datalen, char** encdata);
int   __fastcall util_decrypt(char* encdata, int encdatalen, struct util_cert cert, char** data);

int   __fastcall util_rsaencrypt(X509 *cert, char* data, int datalen, char** encdata);
int   __fastcall util_rsadecrypt(struct util_cert cert, char* data, int datalen, char** decdata);
int   __fastcall util_rsaverify(X509 *cert, char* data, int datalen, char* sign, int signlen);
#endif


#if defined(_DEBUG)

// Display only
#ifdef ANDROID
#include <android/log.h>
#define MSG(...)   __android_log_print(ANDROID_LOG_DEBUG, "MeshAgent", __VA_ARGS__); 
#elif NACL
#include "chrome/nacl.h"
#define MSG(...)  ni_log(__VA_ARGS__); 
#else
#define MSG(...) printf(__VA_ARGS__);fflush(NULL)
#endif
#define DEBUGSTATEMENT(x) x
#else
#ifndef MSG
#define MSG(...)
#endif
#define DEBUGSTATEMENT(x)
#endif
#endif
