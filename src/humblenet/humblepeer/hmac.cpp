#include "hmac.h"

#include <cstring>

void HMACInit(HMACContext *ctx, const uint8_t* key, unsigned key_len)
{
	uint8_t key_block[HMAC_BLOCKSIZE];
	uint8_t pad[HMAC_BLOCKSIZE];
	unsigned key_block_len;

	if (HMAC_BLOCKSIZE < key_len) {
		SHA1Context sha;
		SHA1Reset(&sha);
		SHA1Input(&sha, key, key_len);
		SHA1Result(&sha, key_block);
		key_block_len = HMAC_DIGEST_SIZE;
	} else {
		memcpy(key_block, key, key_len);
		key_block_len = key_len;
	}
	memset(&key_block[key_block_len], 0, sizeof(key_block) - key_block_len);

	for (int i = 0; i < HMAC_BLOCKSIZE; ++i) {
		pad[i] = 0x36 ^ key_block[i];
	}
	SHA1Reset(&ctx->i_ctx);
	SHA1Input(&ctx->i_ctx, pad, HMAC_BLOCKSIZE);


	for (int i = 0; i < HMAC_BLOCKSIZE; ++i) {
		pad[i] = 0x5c ^ key_block[i];
	}
	SHA1Reset(&ctx->o_ctx);
	SHA1Input(&ctx->o_ctx, pad, HMAC_BLOCKSIZE);
}

int HMACInput(HMACContext *ctx, const uint8_t* data, unsigned data_len)
{
	return SHA1Input(&ctx->i_ctx, data, data_len);
}

int HMACResult(HMACContext *ctx, uint8_t Message_Digest[HMAC_DIGEST_SIZE])
{
	SHA1Result(&ctx->i_ctx, Message_Digest);

	SHA1Input(&ctx->o_ctx, Message_Digest, HMAC_DIGEST_SIZE);

	SHA1Result(&ctx->o_ctx, Message_Digest);

	return shaSuccess;
}

static const char kHexLookup[] = "0123456789abcdef";

void HMACResultToHex(uint8_t digest[HMAC_DIGEST_SIZE], std::string& out_hex)
{
	out_hex.resize(HMAC_DIGEST_SIZE * 2);

	for (int i = 0; i < HMAC_DIGEST_SIZE; ++i) {
		auto b =  digest[i];
		out_hex[i * 2] = kHexLookup[b >> 4];
		out_hex[i * 2 + 1] = kHexLookup[b & 0xf];
	}
}
