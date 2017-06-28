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

#if defined(WIN32) && !defined(_WIN32_WCE)
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

#include "utils.h"
#include <stdio.h>
#include <string.h>
#if !defined(__APPLE__) && !defined(WIN32) && !defined(_VX_CPU)
#include <malloc.h>
#endif
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/pkcs12.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>


// openssl RAND_bytes does Bad Things when called from multiple threads
// causes insane amounts of warnings from helgrind
// wrap the calls in a lock to shut it up
static sem_t sslRandomLock;


// Setup OpenSSL
void __fastcall util_openssl_init()
{
	sem_init(&sslRandomLock, 0, 1);

#ifdef WIN32
	char* tbuf[64];
	HMODULE g_hAdvLib = NULL;
	BOOLEAN (APIENTRY *g_CryptGenRandomPtr)(void*, ULONG) = NULL;
#endif
#ifdef _POSIX
	char* tbuf[64];
	int l;
#endif

/*
#ifdef _DEBUG
	CRYPTO_malloc_debug_init();
	//CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
	MemCheck_start();
	MemCheck_on();
#endif
*/

#ifndef OPENSSL_IS_BORINGSSL
	SSLeay_add_all_algorithms();
	SSLeay_add_all_ciphers();
	SSLeay_add_all_digests();
#endif

	SSL_library_init(); // TWO LEAKS COMING FROM THIS LINE. Seems to be a well known OpenSSL problem.
	SSL_load_error_strings();
	ERR_load_crypto_strings(); // ONE LEAK IN LINUX

	// Add more random seeding in Windows (This is probably useful since OpenSSL in Windows has weaker seeding)
#ifdef WIN32
	//RAND_screen(); // On Windows, add more random seeding using a screen dump (this is very expensive).
	if ((g_hAdvLib = LoadLibrary(TEXT("ADVAPI32.DLL"))) != 0) g_CryptGenRandomPtr = (BOOLEAN (APIENTRY *)(void*,ULONG))GetProcAddress(g_hAdvLib,"SystemFunction036");
	if (g_CryptGenRandomPtr != 0 && g_CryptGenRandomPtr(tbuf, 64) != 0) RAND_add(tbuf, 64, 64); // Use this high quality random as added seeding
	if (g_hAdvLib != NULL) FreeLibrary(g_hAdvLib);
#endif

	// Add more random seeding in Linux (May be overkill since OpenSSL already uses /dev/urandom)
#ifdef _POSIX
	// Under Linux we use "/dev/urandom" if available. This is the best source of random on Linux & variants
	FILE *pFile = fopen("/dev/urandom","rb");
	if (pFile != NULL)
	{
		l = fread(tbuf, 1, 64, pFile);
		fclose(pFile);
		if (l > 0) RAND_add(tbuf, l, l);
	}
#endif
}

// Cleanup OpenSSL
void __fastcall util_openssl_uninit()
{
/*
#ifdef _DEBUG
	BIO *bio_err = NULL;
#endif
*/
	//RAND_cleanup();											// Does nothing.
	//CRYPTO_set_dynlock_create_callback(NULL);					// Does nothing.
	//CRYPTO_set_dynlock_destroy_callback(NULL);				// Does nothing.
	//CRYPTO_set_dynlock_lock_callback(NULL);					// Does nothing.
	//CRYPTO_set_locking_callback(NULL);						// Does nothing.
	//CRYPTO_set_id_callback(NULL);								// Does nothing.
	CRYPTO_cleanup_all_ex_data();
	//sk_SSL_COMP_free(SSL_COMP_get_compression_methods());		// Does something, but it causes heap corruption...
	//CONF_modules_unload(1);									// Does nothing.
#ifndef OPENSSL_IS_BORINGSSL
	CONF_modules_free();
#endif
	EVP_cleanup();
	ERR_free_strings();
	ERR_remove_state(0);
/*
#ifdef ENGINE_cleanup
	ENGINE_cleanup();
#endif
#ifdef _DEBUG
	MemCheck_stop();
	bio_err = BIO_new_fp(stdout, BIO_NOCLOSE);
	CRYPTO_mem_leaks(bio_err);
	if (bio_err != NULL) { BIO_free(bio_err); bio_err = NULL; }
#endif
*/
	sem_destroy(&sslRandomLock);
}

// Frees a block of memory returned from this module.
void __fastcall util_free(char* ptr)
{
	free(ptr);
	ptr = NULL;
}

// Convert a block of data to HEX
// The "out" must have (len*2)+1 free space.
char utils_HexTable[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
char utils_HexTable2[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
char* __fastcall util_tohex(char* data, int len, char* out)
{
	int i;
	char *p = out;
	if (data == NULL || len == 0) { *p = 0; return NULL; }
	for(i = 0; i < len; i++)
	{
		*(p++) = utils_HexTable[((unsigned char)data[i]) >> 4];
		*(p++) = utils_HexTable[((unsigned char)data[i]) & 0x0F];
	}
	*p = 0;
	return out;
}
char* __fastcall util_tohex2(char* data, int len, char* out)
{
	int i;
	char *p = out;
	if (data == NULL || len == 0) { *p = 0; return NULL; }
	for(i = 0; i < len; i++)
	{
		*(p++) = utils_HexTable[((unsigned char)data[i]) >> 4];
		*(p++) = utils_HexTable[((unsigned char)data[i]) & 0x0F];
		if(i+1<len)
		{
			*(p++) = ':';
		}
	}
	*p = 0;
	return out;
}
// Convert hex string to int
int __fastcall util_hexToint(char *hexString, int hexStringLength)
{
	int i, res = 0;

	// Ignore the leading zeroes
	while (*hexString == '0' && hexStringLength > 0) { hexString++; hexStringLength--; }

	// Process the rest of the string
	for (i = 0; i < hexStringLength; i++) 
	{
		if (hexString[i] >= '0' && hexString[i] <= '9') {res = (res << 4) + (hexString[i] - '0');}
		else if (hexString[i] >= 'a' && hexString[i] <= 'f') {res = (res << 4) + (hexString[i] - 'a' + 10);}
		else if (hexString[i] >= 'A' && hexString[i] <= 'F') {res = (res << 4) + (hexString[i] - 'A' + 10);}
	}
	return res;
}

// Convert hex string to int 
int __fastcall util_hexToBuf(char *hexString, int hexStringLength, char* output) 
{ 
    int i, x = hexStringLength / 2; 
    for (i = 0; i < x; i++) { output[i] = (char)util_hexToint(hexString + (i * 2), 2); } 
    return i; 
}

unsigned long long __fastcall util_gettime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

long util_Chronometer;
void __fastcall util_startChronometer()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	util_Chronometer = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

long __fastcall util_readChronometer()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000)) - util_Chronometer;
}

// Return 1 if the string is numeric, 0 if not
int   __fastcall util_isnumeric(const char* str) { for (;*str; str++) if (*str < '0' || *str > '9') return 0; return 1; }

/*
// Compresses an input buffer using deflate, the result output buffer must be freed using util_free().
int __fastcall util_compress(char* inbuf, unsigned int inbuflen, char** outbuf, unsigned int headersize)
{
	z_stream c_stream; // compression stream
	int err;
	char* tbuf;

	unsigned int outbuflen = inbuflen - headersize;
	*outbuf = malloc((int)outbuflen + headersize + 4 );
	if (*outbuf == NULL) return 0;
	if (headersize != 0) memcpy(*outbuf, inbuf, headersize);

	c_stream.zalloc = (alloc_func)0;
	c_stream.zfree = (free_func)0;
	c_stream.opaque = (voidpf)0;

	err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
	//CHECK_ERR(err, "deflateInit");

	c_stream.next_in  = (unsigned char*)(inbuf + headersize);
	c_stream.next_out = (unsigned char*)(*outbuf + headersize + 4);

	while (c_stream.total_in != (inbuflen - headersize) && c_stream.total_out < outbuflen) {
		c_stream.avail_in = inbuflen - headersize;
		c_stream.avail_out = outbuflen;
		err = deflate(&c_stream, Z_NO_FLUSH);
		//CHECK_ERR(err, "deflate");
	}

	for (;;) {
		err = deflate(&c_stream, Z_FINISH);
		if (err == Z_STREAM_END || outbuflen <= c_stream.total_out) break;
		//CHECK_ERR(err, "deflate");
	}

	err = deflateEnd(&c_stream);
	//CHECK_ERR(err, "deflateEnd");

	if (outbuflen <= c_stream.total_out)
	{
		// Compression failed (it's larger than before)
		((int*)(*outbuf + headersize))[0] = -1;
		memcpy((*outbuf) + headersize + 4, inbuf + headersize, inbuflen - headersize);
		return outbuflen + headersize + 4;
	}

	outbuflen = c_stream.total_out;
	if ((tbuf = realloc(*outbuf, outbuflen + headersize + 4)) == NULL) return 0;
	*outbuf = tbuf;
	((int*)(*outbuf + headersize))[0] = inbuflen - headersize;
	return outbuflen + headersize + 4;
}

// Decompresses an input buffer using deflate, the result output buffer must be freed using util_free().
int __fastcall util_decompress(char* inbuf, unsigned int inbuflen, char** outbuf, unsigned int headersize)
{
	int err, len;
	z_stream d_stream;
	unsigned int outbuflen = ((int*)(inbuf + headersize))[0];

	if (outbuflen == -1)
	{
		// Failed compression
		*outbuf = malloc(inbuflen - 4);
		if (*outbuf == NULL) return 0;
		if (headersize != 0) memcpy(*outbuf, inbuf, headersize); // Copy the header as-is
		memcpy(*outbuf + headersize, inbuf + headersize + 4, inbuflen - headersize - 4); // Copy the data as-is
		return inbuflen - 4;
	}

	len = outbuflen + headersize;
	*outbuf = malloc(len + 1);
	if (*outbuf == NULL) return 0;
	(*outbuf)[len] = 0;
	if (headersize != 0) memcpy(*outbuf, inbuf, headersize); // Copy the header as-is

	d_stream.zalloc = (alloc_func)0;
	d_stream.zfree = (free_func)0;
	d_stream.opaque = (voidpf)0;

	d_stream.next_in  = (unsigned char*)(inbuf + headersize + 4);
	d_stream.avail_in = 0;
	d_stream.next_out = (unsigned char*)(*outbuf + headersize);

	err = inflateInit(&d_stream);
	//CHECK_ERR(err, "inflateInit");

	while (d_stream.total_out < outbuflen && d_stream.total_in < inbuflen)
	{
		d_stream.avail_in = inbuflen;
		d_stream.avail_out = outbuflen;
		err = inflate(&d_stream, Z_NO_FLUSH);
		if (err == Z_STREAM_END) break;
		//CHECK_ERR(err, "inflate");
	}

	err = inflateEnd(&d_stream);
	//CHECK_ERR(err, "inflateEnd");

	return outbuflen + headersize;
}
*/

size_t __fastcall util_writefile(char* filename, char* data, int datalen)
{
	FILE * pFile = NULL;
	size_t count = 0;

#ifdef WIN32 
	fopen_s(&pFile, filename,"wb");
#else
	pFile = fopen(filename,"wb");
#endif
	
	if (pFile != NULL)
	{
		count = fwrite(data, datalen, 1, pFile);
		fclose(pFile);
	}
	return count;
}

size_t __fastcall util_appendfile(char* filename, char* data, int datalen)
{
	FILE * pFile = NULL;
	size_t count = 0;

#ifdef WIN32 
	fopen_s(&pFile, filename,"ab");
#else
	pFile = fopen(filename,"ab");
#endif
	
	if (pFile != NULL)
	{
		count = fwrite(data, datalen, 1, pFile);
		fclose(pFile);
	}
	return count;
}

// Read a file into memory up to maxlen. If *data is NULL, a new buffer is allocated otherwise, the given one is used.
size_t __fastcall util_readfile(char* filename, char** data, size_t maxlen)
{
	FILE *pFile = NULL;
	size_t count = 0;
	size_t len = 0;
	*data = NULL;
	if (filename == NULL) return 0;

	#ifdef WIN32 
		fopen_s(&pFile, filename,"rb");
	#else
		pFile = fopen(filename,"rb");
	#endif

	if (pFile != NULL)
	{
		fseek(pFile, 0, SEEK_END);
		count = ftell(pFile);
		if (count > maxlen) count = maxlen;
		fseek(pFile, 0, SEEK_SET);
		if (*data == NULL) *data = (char*)malloc(count + 1);
		if (*data == NULL) { fclose(pFile); return 0; }
		while (len < count) len += fread(*data, 1, count-len, pFile);
		(*data)[count] = 0;
		fclose(pFile);
	}
	return count;
}

#ifdef _POSIX
// This method reads a stream where the length of the file can't be determined. Useful in POSIX only
int __fastcall util_readfile2(char* filename, char** data)
{
	FILE * pFile;
	int count = 0;
	int len = 0;
	*data = NULL;
	if (filename == NULL) return 0;

	pFile = fopen(filename,"rb");
	if (pFile != NULL)
	{
		*data = malloc(1024);
		do 
		{
			len = fread((*data) + count, 1, 1023, pFile);
			count += len;
			if (len == 1023) *data = realloc(*data, count + 1024);
		}
		while (len == 100);
		(*data)[count] = 0;
		fclose(pFile);
	}

	return count;
}
#endif

int __fastcall util_deletefile(char* filename)
{
	if (filename == NULL) return 0;
	return remove(filename);
}

void __fastcall util_freecert(struct util_cert* cert)
{
	if (cert->x509 != NULL) X509_free(cert->x509);
	if (cert->pkey != NULL) EVP_PKEY_free(cert->pkey);
	cert->x509 = NULL;
	cert->pkey = NULL;
}

int __fastcall util_to_cer(struct util_cert cert, char** data)
{
	*data = NULL;
	return i2d_X509(cert.x509, (unsigned char**)data);
}

int __fastcall util_from_cer(char* data, int datalen, struct util_cert* cert)
{
	cert->pkey = NULL;
	cert->x509 = d2i_X509(NULL, (const unsigned char**)&data, datalen);
	return ((cert->x509) == NULL);
}

int __fastcall util_from_pem(char* filename, struct util_cert* cert)
{
	FILE *pFile = NULL;

	if (filename == NULL) return -1;
	#ifdef WIN32 
		fopen_s(&pFile, filename,"rb");
	#else
		pFile = fopen(filename,"rb");
	#endif
	if (pFile == NULL) goto error;

	if ((cert->pkey = PEM_read_PrivateKey(pFile, NULL, 0, NULL)) == NULL) goto error;
	if ((cert->x509 = PEM_read_X509(pFile, NULL, 0, NULL)) == NULL) goto error;

	fclose(pFile);
	return 0;
error:
	if (pFile != NULL) fclose(pFile);
	return -1;
}

#ifndef OPENSSL_IS_BORINGSSL
int __fastcall util_to_p12(struct util_cert cert, char *password, char** data)
{
	PKCS12 *p12;
	int len;
	p12 = PKCS12_create(password, "Certificate", cert.pkey, cert.x509, NULL, 0, 0, 0, 0, 0);
	*data = NULL;
	len = i2d_PKCS12(p12, (unsigned char**)data);
	PKCS12_free(p12);
	return len;
}

int __fastcall util_from_p12(char* data, int datalen, char* password, struct util_cert* cert)
{
	int r = 0;
	PKCS12 *p12 = NULL;
	if (data == NULL || datalen ==0) return 0;
	cert->x509 = NULL;
	cert->pkey = NULL;
	p12 = d2i_PKCS12(&p12, (const unsigned char**)&data, datalen);
	r = PKCS12_parse(p12, password, &(cert->pkey), &(cert->x509), NULL);
	PKCS12_free(p12);
	return r;
}
#endif // ifndef OPENSSL_IS_BORINGSSL

#ifdef OPENSSL_IS_BORINGSSL
#define X509V3_EXT_conf_nid X509V3_EXT_nconf_nid
#endif

// Add extension using V3 code: we can set the config file as NULL because we wont reference any other sections.
int __fastcall util_add_ext(X509 *cert, int nid, char *value)
{
	X509_EXTENSION *ex;
	X509V3_CTX ctx;
	// This sets the 'context' of the extensions. No configuration database
	X509V3_set_ctx_nodb(&ctx);
	// Issuer and subject certs: both the target since it is self signed, no request and no CRL
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
	if (!ex) return 0;

	X509_add_ext(cert,ex,-1);
	X509_EXTENSION_free(ex);
	return 1;
}

void __fastcall util_printcert(struct util_cert cert)
{
	if (cert.x509 == NULL) return;
	X509_print_fp(stdout,cert.x509);
}

#ifndef OPENSSL_IS_BORINGSSL
void __fastcall util_printcert_pk(struct util_cert cert)
{
	if (cert.pkey == NULL) return;
	RSA_print_fp(stdout,cert.pkey->pkey.rsa,0);
}
#endif

// Creates a X509 certificate, if rootcert is NULL this creates a root (self-signed) certificate.
// Is the name parameter is NULL, the hex value of the hash of the public key will be the subject name.
int __fastcall util_mkCert(struct util_cert *rootcert, struct util_cert* cert, int bits, int days, char* name, enum CERTIFICATE_TYPES certtype, struct util_cert* initialcert)
{
	X509 *x = NULL;
	X509_EXTENSION *ex = NULL;
	EVP_PKEY *pk = NULL;
	RSA *rsa = NULL;
	X509_NAME *cname=NULL;
	X509 **x509p = NULL;
	EVP_PKEY **pkeyp = NULL;
	char hash[UTIL_HASHSIZE];
	char serial[8];
	char nameStr[(UTIL_HASHSIZE * 2) + 2];
	BIGNUM *oBigNbr;

#ifndef OPENSSL_IS_BORINGSSL
	MemCheck_on();
#endif

	if (initialcert != NULL)
	{
		pk = X509_get_pubkey(initialcert->x509);
		rsa = EVP_PKEY_get1_RSA(initialcert->pkey);
		if ((x=X509_new()) == NULL) goto err;
	}
	else
	{
		if ((pkeyp == NULL) || (*pkeyp == NULL)) { if ((pk = EVP_PKEY_new()) == NULL) return 0; } else pk = *pkeyp;
		if ((x509p == NULL) || (*x509p == NULL)) { if ((x=X509_new()) == NULL) goto err; } else x = *x509p;
		oBigNbr = BN_new();
		rsa = RSA_new();
		BN_set_word(oBigNbr, RSA_F4);
		if (RSA_generate_key_ex(rsa, bits, oBigNbr, NULL) == -1)
		{
			RSA_free(rsa);
			BN_free(oBigNbr);
			abort();
			goto err;
		}
		BN_free(oBigNbr);
	}

	if (!EVP_PKEY_assign_RSA(pk, rsa))
	{
		RSA_free(rsa);
		abort();
		goto err;
	}
	rsa = NULL;

	util_randomtext(8, serial);
	X509_set_version(x, 2);
	ASN1_STRING_set(X509_get_serialNumber(x), serial, 8);
	X509_gmtime_adj(X509_get_notBefore(x),(long)60*60*24*-2);
	X509_gmtime_adj(X509_get_notAfter(x),(long)60*60*24*days);
	X509_set_pubkey(x, pk);

	// Set the subject name
	cname = X509_get_subject_name(x);

	if (name == NULL)
	{
		// Computer the hash of the public key
		util_sha256((char*)x->cert_info->key->public_key->data, x->cert_info->key->public_key->length, hash);
		util_tohex(hash, UTIL_HASHSIZE, nameStr);
		X509_NAME_add_entry_by_txt(cname,"CN", MBSTRING_ASC, (unsigned char*)nameStr, -1, -1, 0);
	}
	else
	{
		// This function creates and adds the entry, working out the correct string type and performing checks on its length. Normally we'd check the return value for errors...
		X509_NAME_add_entry_by_txt(cname,"CN", MBSTRING_ASC, (unsigned char*)name, -1, -1, 0);
	}

	if (rootcert == NULL)
	{
		// Its self signed so set the issuer name to be the same as the subject.
		X509_set_issuer_name(x,cname);

		// Add various extensions: standard extensions
		util_add_ext(x, NID_basic_constraints, "critical,CA:TRUE");
		util_add_ext(x, NID_key_usage, "critical,keyCertSign,cRLSign");

		util_add_ext(x, NID_subject_key_identifier, "hash");
		//util_add_ext(x, NID_netscape_cert_type, "sslCA");
		//util_add_ext(x, NID_netscape_comment, "example comment extension");

		if (!X509_sign(x, pk, EVP_sha256())) goto err;
	}
	else
	{
		// This is a sub-certificate
		cname=X509_get_subject_name(rootcert->x509);
		X509_set_issuer_name(x, cname);

		// Add usual cert stuff
		ex = X509V3_EXT_conf_nid(NULL, NULL, NID_key_usage, "digitalSignature, keyEncipherment, keyAgreement");
		X509_add_ext(x, ex, -1);
		X509_EXTENSION_free(ex);

		// Add usages: TLS server, TLS client, Intel(R) AMT Console
		//ex = X509V3_EXT_conf_nid(NULL, NULL, NID_ext_key_usage, "TLS Web Server Authentication, TLS Web Client Authentication, 2.16.840.1.113741.1.2.1, 2.16.840.1.113741.1.2.2");
		if (certtype == CERTIFICATE_TLS_SERVER)
		{
			// TLS server
			ex = X509V3_EXT_conf_nid(NULL, NULL, NID_ext_key_usage, "TLS Web Server Authentication");
			X509_add_ext(x, ex, -1);
			X509_EXTENSION_free(ex);
		}
		else if (certtype == CERTIFICATE_TLS_CLIENT)
		{
			// TLS client
			ex = X509V3_EXT_conf_nid(NULL, NULL, NID_ext_key_usage, "TLS Web Client Authentication");
			X509_add_ext(x, ex, -1);
			X509_EXTENSION_free(ex);
		}

		if (!X509_sign(x, rootcert->pkey, EVP_sha256())) goto err;
	}

	cert->x509 = x;
	cert->pkey = pk;

	return(1);
err:
	return(0);
}

int __fastcall util_keyhash(struct util_cert cert, char* result)
{
	if (cert.x509 == NULL) return -1;
	util_sha256((char*)(cert.x509->cert_info->key->public_key->data), cert.x509->cert_info->key->public_key->length, result);
	return 0;
}

int __fastcall util_keyhash2(X509* cert, char* result)
{
	if (cert == NULL) return -1;
	util_sha256((char*)(cert->cert_info->key->public_key->data), cert->cert_info->key->public_key->length, result);
	return 0;
}

// Perform a MD5 hash on the data
void __fastcall util_md5(char* data, int datalen, char* result)
{
	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c, data, datalen);
	MD5_Final((unsigned char*)result, &c);
}

// Perform a MD5 hash on the data and convert result to HEX and store in output
// This is useful for HTTP Digest
void __fastcall util_md5hex(char* data, int datalen, char *out)
{
	int i = 0;
	unsigned char *temp = (unsigned char*)out;
	MD5_CTX mdContext;
	unsigned char digest[16];

	MD5_Init(&mdContext);
	MD5_Update(&mdContext, (unsigned char *)data, datalen);
	MD5_Final(digest, &mdContext);

	for(i = 0; i < HALF_NONCE_SIZE; i++)
	{
		*(temp++) = utils_HexTable2[(unsigned char)digest[i] >> 4];
		*(temp++) = utils_HexTable2[(unsigned char)digest[i] & 0x0F];
	}

	*temp = '\0';
}

// Perform a SHA256 hash on the data
void __fastcall util_sha256(char* data, int datalen, char* result)
{
	SHA256_CTX c;
	SHA256_Init(&c);
	SHA256_Update(&c, data, datalen);
	SHA256_Final((unsigned char*)result, &c);
}

// Run SHA256 on a file and return the result
int __fastcall util_sha256file(char* filename, char* result)
{
	FILE *pFile = NULL;
	SHA256_CTX c;
	size_t len = 0;
	char *buf = NULL;

	if (filename == NULL) return -1;
	#ifdef WIN32 
		fopen_s(&pFile, filename,"rb");
	#else
		pFile = fopen(filename,"rb");
	#endif
	if (pFile == NULL) goto error;
	SHA256_Init(&c);
	if ((buf = (char*)malloc(4096)) == NULL) goto error;
	while ((len = fread(buf, 1, 4096, pFile)) > 0) SHA256_Update(&c, buf, len);
	free(buf);
	buf = NULL;
	fclose(pFile);
	pFile = NULL;
	SHA256_Final((unsigned char*)result, &c);
	return 0;

error:
	if (buf != NULL) free(buf);
	if (pFile != NULL) fclose(pFile);
	return -1;
}

#ifndef OPENSSL_IS_BORINGSSL
// Sign this block of data, the first 32 bytes of the block must be avaialble to add the certificate hash.
int __fastcall util_sign(struct util_cert cert, char* data, int datalen, char** signature)
{
	int size = 0;
	unsigned int hashsize = UTIL_HASHSIZE;
	BIO *in = NULL;
	PKCS7 *message = NULL;
	*signature = NULL;
	if (datalen <= UTIL_HASHSIZE) return 0;

	// Add hash of the certificate to start of data
	X509_digest(cert.x509, EVP_sha256(), (unsigned char*)data, &hashsize);

	// Sign the block
	in = BIO_new_mem_buf(data, datalen);
	message = PKCS7_sign(cert.x509, cert.pkey, NULL, in, PKCS7_BINARY);
	if (message == NULL) goto error;
	size = i2d_PKCS7(message, (unsigned char**)signature);
	
error:
	if (message != NULL) PKCS7_free(message);
	if (in != NULL) BIO_free(in);
	return size;
}

// Verify the signed block, the first 32 bytes of the data must be the certificate hash to work.
int __fastcall util_verify(char* signature, int signlen, struct util_cert* cert, char** data)
{
	unsigned int size, r;
	BIO *out = NULL;
	PKCS7 *message = NULL;
	char* data2 = NULL;
	char hash[UTIL_HASHSIZE];
	STACK_OF(X509) *st = NULL;

	cert->x509 = NULL;
	cert->pkey = NULL;
	*data = NULL;
	message = d2i_PKCS7(NULL, (const unsigned char**)&signature, signlen);
	if (message == NULL) goto error;
	out = BIO_new(BIO_s_mem());

	// Lets rebuild the original message and check the size
	size = i2d_PKCS7(message, NULL);
	if (size < (unsigned int)signlen) goto error;

	// Check the PKCS7 signature, but not the certificate chain.
	r = PKCS7_verify(message, NULL, NULL, NULL, out, PKCS7_NOVERIFY);
	if (r == 0) goto error;

	// If data block contains less than 32 bytes, fail.
	size = BIO_get_mem_data(out, &data2);
	if (size <= UTIL_HASHSIZE) goto error;

	// Copy the data block
	*data = (char*)malloc(size + 1);
	if (*data == NULL) goto error;
	memcpy(*data, data2, size);
	(*data)[size] = 0;

	// Get the certificate signer
	st = PKCS7_get0_signers(message, NULL, PKCS7_NOVERIFY);
	cert->x509 = X509_dup(sk_X509_value(st, 0));
	sk_X509_free(st);

	// Get a full certificate hash of the signer
	r = UTIL_HASHSIZE;
	X509_digest(cert->x509, EVP_sha256(), (unsigned char*)hash, &r);

	// Check certificate hash with first 32 bytes of data.
	if (memcmp(hash, *data, UTIL_HASHSIZE) != 0) goto error;

	// Approved, cleanup and return.
	BIO_free(out);
	PKCS7_free(message);

	return size;

error:
	if (out != NULL) BIO_free(out);
	if (message != NULL) PKCS7_free(message);
	if (*data != NULL) free(*data);
	if (cert->x509 != NULL) { X509_free(cert->x509); cert->x509 = NULL; }

	return 0;
}

// Encrypt a block of data for a target certificate
int __fastcall util_encrypt(struct util_cert cert, char* data, int datalen, char** encdata)
{
	int size = 0;
	BIO *in = NULL;
	PKCS7 *message = NULL;
	STACK_OF(X509) *encerts = NULL;
	*encdata = NULL;
	if (datalen == 0) return 0;

	// Setup certificates
	encerts = sk_X509_new_null();
	sk_X509_push(encerts, cert.x509);

	// Encrypt the block
	*encdata = NULL;
	in = BIO_new_mem_buf(data, datalen);
	message = PKCS7_encrypt(encerts, in, EVP_aes_128_cbc(), PKCS7_BINARY);
	if (message == NULL) return 0;
	size = i2d_PKCS7(message, (unsigned char**)encdata);
	BIO_free(in);
	PKCS7_free(message);
	sk_X509_free(encerts);
	return size;
}

// Encrypt a block of data using multiple target certificates
int __fastcall util_encrypt2(STACK_OF(X509) *certs, char* data, int datalen, char** encdata)
{
	int size = 0;
	BIO *in = NULL;
	PKCS7 *message = NULL;
	*encdata = NULL;
	if (datalen == 0) return 0;

	// Encrypt the block
	*encdata = NULL;
	in = BIO_new_mem_buf(data, datalen);
	message = PKCS7_encrypt(certs, in, EVP_aes_128_cbc(), PKCS7_BINARY);
	if (message == NULL) return 0;
	size = i2d_PKCS7(message, (unsigned char**)encdata);
	BIO_free(in);
	PKCS7_free(message);
	return size;
}

// Decrypt a block of data using the specified certificate. The certificate must have a private key.
int __fastcall util_decrypt(char* encdata, int encdatalen, struct util_cert cert, char** data)
{
	unsigned int size, r;
	BIO *out = NULL;
	PKCS7 *message = NULL;
	char* data2 = NULL;

	*data = NULL;
	if (cert.pkey == NULL) return 0;

	message = d2i_PKCS7(NULL, (const unsigned char**)&encdata, encdatalen);
	if (message == NULL) goto error;
	out = BIO_new(BIO_s_mem());

	// Lets rebuild the original message and check the size
	size = i2d_PKCS7(message, NULL);
	if (size < (unsigned int)encdatalen) goto error;

	// Decrypt the PKCS7
	r = PKCS7_decrypt(message, cert.pkey, cert.x509, out, 0);
	if (r == 0) goto error;

	// If data block contains 0 bytes, fail.
	size = BIO_get_mem_data(out, &data2);
	if (size == 0) goto error;

	// Copy the data block
	*data = (char*)malloc(size + 1);
	if (*data == NULL) goto error;
	memcpy(*data, data2, size);
	(*data)[size] = 0;

	// Cleanup and return.
	BIO_free(out);
	PKCS7_free(message);

	return size;

error:
	if (out != NULL) BIO_free(out);
	if (message != NULL) PKCS7_free(message);
	if (*data != NULL) free(*data);
	if (data2 != NULL) free(data2);
	return 0;
}
#endif // ifndef OPENSSL_IS_BORINGSSL

// Generates a random string of data. TODO: Use Hardware RNG if possible
void __fastcall util_random(int length, char* result)
{
	sem_wait(&sslRandomLock);
	RAND_bytes((unsigned char*)result, length);
	sem_post(&sslRandomLock);
}

// Generates a random text string, useful for HTTP nonces.
void __fastcall util_randomtext(int length, char* result)
{
	int l;
	util_random(length, result);
	for (l=0;l<length;l++) result[l] = (unsigned char)((((unsigned char)result[l]) % 10) + '0');
}

// Encrypt a block of data using raw RSA. This is used to handle data in the most compact possible way.
int __fastcall util_rsaencrypt(X509 *cert, char* data, int datalen, char** encdata)
{
	int len;
	RSA *rsa;
	EVP_PKEY *pkey;

	pkey = X509_get_pubkey(cert);
	rsa = EVP_PKEY_get1_RSA(pkey);
	if (datalen > RSA_size(rsa)) { EVP_PKEY_free(pkey); RSA_free(rsa); return 0; }
	*encdata = (char*)malloc(RSA_size(rsa));
	len = RSA_public_encrypt(datalen, (const unsigned char*)data, (unsigned char*)*encdata, rsa, RSA_PKCS1_OAEP_PADDING);
	EVP_PKEY_free(pkey);
	RSA_free(rsa);
	if (len == RSA_size(rsa)) return len;
	free(*encdata);
	*encdata = NULL;
	return 0;
}

// Decrypt a block of data using raw RSA. This is used to handle data in the most compact possible way.
int __fastcall util_rsadecrypt(struct util_cert cert, char* data, int datalen, char** decdata)
{
	int len;
	RSA *rsa;

	rsa = EVP_PKEY_get1_RSA(cert.pkey);
	*decdata = (char*)malloc(RSA_size(rsa));
	len = RSA_private_decrypt(datalen, (const unsigned char*)data, (unsigned char*)*decdata, rsa, RSA_PKCS1_OAEP_PADDING);
	RSA_free(rsa);
	if (len != 0) return len;
	free(*decdata);
	*decdata = NULL;
	return 0;
}

#ifndef OPENSSL_IS_BORINGSSL
// Verify the RSA signature of a block using SHA1 hash
int __fastcall util_rsaverify(X509 *cert, char* data, int datalen, char* sign, int signlen)
{
	int r;
	RSA *rsa = NULL;
	EVP_PKEY *pkey = NULL;
	SHA_CTX c;
	char hash[20];

	SHA1_Init(&c);
	SHA1_Update(&c, data, datalen);
	SHA1_Final((unsigned char*)hash, &c);
	pkey = X509_get_pubkey(cert);
	rsa = EVP_PKEY_get1_RSA(pkey);
	rsa->pad = RSA_PKCS1_PADDING;
#ifdef WIN32
	r = RSA_verify(NID_sha1, (const unsigned char*)hash, 20, (const unsigned char*)sign, signlen, rsa);
#else
	r = RSA_verify(NID_sha1, (const unsigned char*)hash, 20, (unsigned char*)sign, signlen, rsa);
#endif
	EVP_PKEY_free(pkey);
	RSA_free(rsa);
	return r;
}
#endif // ifndef OPENSSL_IS_BORINGSSL

// We need these as the master key for most of the symetric crypto methods
extern unsigned int g_SessionRandomId;
extern char g_SessionRandom[32];
extern char g_selfid[UTIL_HASHSIZE];
extern unsigned int g_nextiv;

// Get the private session key for a given nodeid.
// Nodeid is 32 bytes, key must point to 36 bytes of free space.
void __fastcall util_nodesessionkey(char* nodeid, char* key)
{
	SHA256_CTX c;
	SHA256_Init(&c);
	SHA256_Update(&c, nodeid, 32);
	SHA256_Update(&c, g_SessionRandom, 32);
	SHA256_Final((unsigned char*)(key + 4), &c);
	((unsigned int*)key)[0] = g_SessionRandomId; // Here, endianness does not matter, leave as-is for speed.
}

// Convert a private key into a usage specific key
void __fastcall util_genusagekey(char* inkey, char* outkey, char usage)
{
	SHA256_CTX c;
	SHA256_Init(&c);
	SHA256_Update(&c, &usage, 1);
	SHA256_Update(&c, inkey, 32);
	SHA256_Update(&c, &usage, 1);
	SHA256_Final((unsigned char*)outkey, &c);
}


// Starts dropping packets is rate is too high
long util_af_time = 0;
long util_af_count = 0;
int __fastcall util_antiflood(int rate, int interval)
{
	long seconds;
	struct timeval clock;
	gettimeofday(&clock,NULL);
	seconds = clock.tv_sec;

	if ((seconds - util_af_time) > interval)
	{
		util_af_time = seconds;
		util_af_count = 0;
		return 1;
	}
	util_af_count++;
	return util_af_count < rate;
}

#define AUTHORIZATION_FIELDDATA_TEMPLATE "Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\", qop=%s, nc=%08x, cnonce=\"%s\""
unsigned int util_iv_mask = 0;


unsigned int __fastcall util_getiv()
{
	if (g_nextiv == 0)
	{
		util_random(UTIL_HASHSIZE, g_SessionRandom);
		g_SessionRandomId++;
		g_nextiv = 1;
		util_iv_mask = 0;
	}

	util_iv_mask <<= 1;
	util_iv_mask |= 1;
	return g_nextiv++;
}


int __fastcall util_checkiv(unsigned int iv)
{
	int r = 0;
	int dt = g_nextiv - iv - 1;
	unsigned int m;

	if (dt > 31)
	{
		//MSG("BAD IV: %d\r\n", iv);
		return 0;
	}
	m = 1 << dt;
	r = util_iv_mask & m;
	util_iv_mask &= ~m;
	//if (r != 0) {MSG("GOOD IV: %d\r\n", iv);} else {MSG("BAD IV: %d\r\n", iv);}
	return r;
}

/*
static unsigned short c_remainders[256];

//Using the 16-bit CRC-16-CCITT generator polynomial x16 + x12 + x5 + 1 for CRC computation.
//x^16+x^12+x^5+1 = (1) 0001 0000 0010 0001 = 0x1021
//Using MSB first style of CRC computation.
void __fastcall util_setupcrc()
{
	short j;
	unsigned short databyte = 0;
	unsigned short remainder = 0;
	unsigned short generator = 0x1021;
	for (databyte = 0; databyte < 256; databyte++)
	{
		remainder = (databyte << 8);	
		for (j = 0; j < 8; j++)
		{
			if ((remainder & 0x8000) != 0) 
				remainder = (remainder << 1) ^ generator;			
			else
				remainder = (remainder << 1);
		}
		c_remainders[databyte] = remainder;
	}
}

unsigned short __fastcall util_crc(unsigned char *buffer, unsigned long num_bytes)
{
	unsigned short crc = 0;
	unsigned long byte;
	for(byte = 0; byte < num_bytes; byte++)
	{
		crc ^= ((*buffer) << 8);
		crc = (crc << 8) ^ c_remainders[(crc & 0xFF00) >> 8];
		buffer++;
	}
	return crc;
}
*/

#ifdef WIN32
// Really fast CRC-like method. Used for the KVM.
int __fastcall util_crc(unsigned char *buffer, int len, int initial_value)
{
	int hval = initial_value;
	int *bp = (int*)buffer;
    int *be = bp + (len >> 2);
    while (bp < be)
	{
		//hval *= 0x01000193;
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
		hval ^= *bp++;
    }
	// TODO: Handle left over bytes (len % 4)
    return hval;
}
#endif


