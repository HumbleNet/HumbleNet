/*
Copyright 2015 Intel Corporation

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


#ifndef __SHA1H__
#define __SHA1H__

#include <stdint.h>

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

typedef struct sha1nfo {
	uint32_t buffer[BLOCK_LENGTH / 4];
	uint32_t state[HASH_LENGTH / 4];
	uint32_t byteCount;
	uint8_t bufferOffset;
	uint8_t keyBuffer[BLOCK_LENGTH];
	uint8_t innerHash[HASH_LENGTH];
} sha1nfo;
typedef sha1nfo SHA_CTX;

void sha1_write(sha1nfo *s, const char *data, size_t len);
uint8_t* sha1_result(sha1nfo *s);

void SHA1_Init(SHA_CTX*);
#define SHA1_Update(pctx, pkeyResult, keyResultLen) sha1_write(pctx, pkeyResult, keyResultLen)
#define SHA1_Final(shavalue, pctx) memcpy(shavalue, sha1_result(pctx), 20);

#endif
