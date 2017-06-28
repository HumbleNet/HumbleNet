#ifndef HMAC_H
#define HMAC_H

#include "sha1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HMAC_BLOCKSIZE 64
#define HMAC_DIGEST_SIZE SHA1HashSize

typedef struct HMACContext {
	SHA1Context i_ctx;
	SHA1Context o_ctx;
} HMACContext;

void HMACInit(HMACContext *ctx, const uint8_t* key, unsigned key_len);
int HMACInput(HMACContext *ctx, const uint8_t* data, unsigned data_len);
int HMACResult(HMACContext *ctx, uint8_t Message_Digest[HMAC_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
void HMACResultToHex(uint8_t digest[HMAC_DIGEST_SIZE], std::string& out_hex);
#endif

#endif /* HMAC_H */
