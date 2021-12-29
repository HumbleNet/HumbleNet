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

#include "ILibSimpleDataStore.h"
#include "ILibCrypto.h"

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(_MINCORE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define SHA256HASHSIZE 32

#ifdef _WIN64
	#define ILibSimpleDataStore_GetPosition(filePtr) _ftelli64(filePtr)
	#define ILibSimpleDataStore_SeekPosition(filePtr, position, seekMode) _fseeki64(filePtr, position, seekMode)
	typedef long long DS_Long;
#else
	#define ILibSimpleDataStore_GetPosition(filePtr) ftell(filePtr)
	#define ILibSimpleDataStore_SeekPosition(filePtr, position, seekMode) fseek(filePtr, position, seekMode)
	typedef long DS_Long;
#endif

typedef struct ILibSimpleDataStore_Root
{
	FILE* dataFile;
	char* filePath;
	char scratchPad[4096];
	ILibHashtable keyTable; // keys --> ILibSimpleDataStore_TableEntry
	DS_Long fileSize;
	int error;
} ILibSimpleDataStore_Root;

/* File Format                 
------------------------------------------
 4 Bytes	- Node size
 4 Bytes	- Key length
 4 Bytes	- Value length
32 Bytes	- SHA256 hash check value
Variable	- Key
Variable	- Value
------------------------------------------ */

typedef struct ILibSimpleDataStore_RecordHeader
{
	int nodeSize;
	int keyLen;
	int valueLength;
	char hash[SHA256HASHSIZE];
} ILibSimpleDataStore_RecordHeader;

typedef struct ILibSimpleDataStore_RecordNode
{
	int nodeSize;
	int keyLen;
	int valueLength;
	char valueHash[SHA256HASHSIZE];
	DS_Long valueOffset;
	char key[];
} ILibSimpleDataStore_RecordNode;

typedef struct ILibSimpleDataStore_TableEntry
{
	int valueLength;
	char valueHash[SHA256HASHSIZE];
	DS_Long valueOffset;
} ILibSimpleDataStore_TableEntry;

const int ILibMemory_SimpleDataStore_CONTAINERSIZE = sizeof(ILibSimpleDataStore_Root);

// Perform a SHA256 hash of some data
void ILibSimpleDataStore_SHA256(char *data, int datalen, char* result) { util_sha256(data, datalen, result); }

// Write a key/value pair to file, the hash is already calculated
DS_Long ILibSimpleDataStore_WriteRecord(FILE *f, char* key, int keyLen, char* value, int valueLen, char* hash)
{
	char headerBytes[sizeof(ILibSimpleDataStore_RecordNode)];
	ILibSimpleDataStore_RecordHeader *header = (ILibSimpleDataStore_RecordHeader*)headerBytes;
	DS_Long offset;

	fseek(f, 0, SEEK_END);
	header->nodeSize = htonl(sizeof(ILibSimpleDataStore_RecordNode) + keyLen + valueLen);
	header->keyLen = htonl(keyLen);
	header->valueLength = htonl(valueLen);
	if (hash != NULL) { memcpy_s(header->hash, sizeof(header->hash), hash, SHA256HASHSIZE); } else { memset(header->hash, 0, SHA256HASHSIZE); }

	fwrite(headerBytes, 1, sizeof(ILibSimpleDataStore_RecordNode), f);
	fwrite(key, 1, keyLen, f);
	offset = ILibSimpleDataStore_GetPosition(f);
	if (value != NULL) { fwrite(value, 1, valueLen, f); }
	fflush(f);
	return offset;
}

// Read the next record in the file
ILibSimpleDataStore_RecordNode* ILibSimpleDataStore_ReadNextRecord(ILibSimpleDataStore_Root *root)
{
	SHA256_CTX c;
	char data[4096];
	char result[SHA256HASHSIZE];
	int i, bytesLeft;
	ILibSimpleDataStore_RecordNode *node = (ILibSimpleDataStore_RecordNode*)(root->scratchPad );

	// If the current position is the end of the file, exit now.
	if (ILibSimpleDataStore_GetPosition(root->dataFile) == root->fileSize) { return NULL; }

	// Read sizeof(ILibSimpleDataStore_RecordNode) bytes to get record Size
	i = (int)fread((void*)node, 1, sizeof(ILibSimpleDataStore_RecordNode), root->dataFile);
	if (i < sizeof(ILibSimpleDataStore_RecordNode)) { return NULL; }

	// Correct the struct, valueHash stays the same
	node->nodeSize = (int)ntohl(node->nodeSize);
	node->keyLen = (int)ntohl(node->keyLen);
	node->valueLength = (int)ntohl(node->valueLength);
	node->valueOffset = ILibSimpleDataStore_GetPosition(root->dataFile) + (DS_Long)node->keyLen;

	// Read the key name
	i = (int)fread((char*)node + sizeof(ILibSimpleDataStore_RecordNode), 1, node->keyLen, root->dataFile);
	if (i != node->keyLen) { return NULL; } // Reading Key Failed

	// Validate Data, in 4k chunks at a time
	bytesLeft = node->valueLength;

	// Hash SHA256 the data
	SHA256_Init(&c);
	while (bytesLeft > 0)
	{
		i = (int)fread(data, 1, bytesLeft > 4096 ? 4096 : bytesLeft, root->dataFile);
		if (i <= 0) { bytesLeft = 0; break; }
		SHA256_Update(&c, data, i);
		bytesLeft -= i;
	}
	SHA256_Final((unsigned char*)result, &c);
	if (node->valueLength > 0)
	{
		// Check the hash
		if (memcmp(node->valueHash, result, SHA256HASHSIZE) == 0) {
			return node; // Data is correct
		}
		return NULL; // Data is corrupt
	}
	else
	{
		return(node);
	}
}

// ???
void ILibSimpleDataStore_TableClear_Sink(ILibHashtable sender, void *Key1, char* Key2, int Key2Len, void *Data, void *user)
{
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(Key1);
	UNREFERENCED_PARAMETER(Key2);
	UNREFERENCED_PARAMETER(Key2Len);
	UNREFERENCED_PARAMETER(user);

	free(Data);
}

// Rebuild the in-memory key to record table, done when starting up the data store
void ILibSimpleDataStore_RebuildKeyTable(ILibSimpleDataStore_Root *root)
{
	ILibSimpleDataStore_RecordNode *node = NULL;
	ILibSimpleDataStore_TableEntry *entry;

	ILibHashtable_ClearEx(root->keyTable, ILibSimpleDataStore_TableClear_Sink, root); // Wipe the key table, we will rebulit it
	fseek(root->dataFile, 0, SEEK_SET); // See the start of the file
	root->fileSize = -1; // Indicate we can't write to the data store

	// Start reading records
	while ((node = ILibSimpleDataStore_ReadNextRecord(root)) != NULL)
	{
		// Get the entry from the memory table
		entry = (ILibSimpleDataStore_TableEntry*)ILibHashtable_Get(root->keyTable, NULL, node->key, node->keyLen);
		if (node->valueLength > 0)
		{
			// If the value is not empty, we need to create/overwrite this value in memory
			if (entry == NULL) { entry = (ILibSimpleDataStore_TableEntry*)ILibMemory_Allocate(sizeof(ILibSimpleDataStore_TableEntry), 0, NULL, NULL); }
			memcpy_s(entry->valueHash, sizeof(entry->valueHash), node->valueHash, SHA256HASHSIZE);
			entry->valueLength = node->valueLength;
			entry->valueOffset = node->valueOffset;
			ILibHashtable_Put(root->keyTable, NULL, node->key, node->keyLen, entry);
		}
		else if (entry != NULL)
		{
			// If value is empty, remove the in-memory entry.
			ILibHashtable_Remove(root->keyTable, NULL, node->key, node->keyLen);
			free(entry);
		}
	}

	// Set the size of the entire data store file
	root->fileSize = ILibSimpleDataStore_GetPosition(root->dataFile);
}

// Open the data store file
FILE* ILibSimpleDataStore_OpenFileEx(char* filePath, int forceTruncateIfNonZero)
{
	FILE* f = NULL;

#ifdef WIN32
	if (forceTruncateIfNonZero !=0 || fopen_s(&f, filePath, "rb+N") != 0)
	{
		fopen_s(&f, filePath, "wb+N");
	}
#else
	if (forceTruncateIfNonZero != 0 || (f = fopen(filePath, "rb+")) == NULL)
	{
		f = fopen(filePath, "wb+");
	}
#endif

	return f;
}
#define ILibSimpleDataStore_OpenFile(filePath) ILibSimpleDataStore_OpenFileEx(filePath, 0)

// Open the data store file. Optionally allocate spare user memory
__EXPORT_TYPE ILibSimpleDataStore ILibSimpleDataStore_CreateEx(char* filePath, int userExtraMemorySize)
{
	ILibSimpleDataStore_Root* retVal = (ILibSimpleDataStore_Root*)ILibMemory_Allocate(ILibMemory_SimpleDataStore_CONTAINERSIZE, userExtraMemorySize, NULL, NULL);
	
	retVal->filePath = ILibString_Copy(filePath, (int)strnlen_s(filePath, ILibSimpleDataStore_MaxFilePath));
	retVal->dataFile = ILibSimpleDataStore_OpenFile(retVal->filePath);

	if (retVal->dataFile == NULL)
	{
		free(retVal->filePath);
		free(retVal);
		return NULL;
	}

	retVal->keyTable = ILibHashtable_Create();
	ILibSimpleDataStore_RebuildKeyTable(retVal);
	return(retVal);
}

// Close the data store file
__EXPORT_TYPE void ILibSimpleDataStore_Close(ILibSimpleDataStore dataStore)
{
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibHashtable_DestroyEx(root->keyTable, ILibSimpleDataStore_TableClear_Sink, root);

	free(root->filePath);
	fclose(root->dataFile);
	free(root);
}

// Store a key/value pair in the data store
__EXPORT_TYPE int ILibSimpleDataStore_PutEx(ILibSimpleDataStore dataStore, char* key, int keyLen, char* value, int valueLen)
{
	char hash[SHA256HASHSIZE];
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibSimpleDataStore_TableEntry *entry = (ILibSimpleDataStore_TableEntry*)ILibHashtable_Get(root->keyTable, NULL, key, keyLen);
	ILibSimpleDataStore_SHA256(value, valueLen, hash); // Hash the value

	// Create a new record for the key and value
	if (entry == NULL) {
		entry = (ILibSimpleDataStore_TableEntry*)ILibMemory_Allocate(sizeof(ILibSimpleDataStore_TableEntry), 0, NULL, NULL); }
	else {
		if (memcmp(entry->valueHash, hash, SHA256HASHSIZE) == 0) { return 0; }
	}

	memcpy_s(entry->valueHash, sizeof(entry->valueHash), hash, SHA256HASHSIZE);
	entry->valueLength = valueLen;
	entry->valueOffset = ILibSimpleDataStore_WriteRecord(root->dataFile, key, keyLen, value, valueLen, entry->valueHash); // Write the key and value
	root->fileSize = ILibSimpleDataStore_GetPosition(root->dataFile); // Update the size of the data store;

	// Add the record to the data store
	return ILibHashtable_Put(root->keyTable, NULL, key, keyLen, entry) == NULL ? 0 : 1;
}

// Get a value from the data store given a key
__EXPORT_TYPE int ILibSimpleDataStore_GetEx(ILibSimpleDataStore dataStore, char* key, int keyLen, char *buffer, int bufferLen)
{
	char hash[32];
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibSimpleDataStore_TableEntry *entry = (ILibSimpleDataStore_TableEntry*)ILibHashtable_Get(root->keyTable, NULL, key, keyLen);

	if (entry == NULL) return 0; // If there is no in-memory entry for this key, return zero now.
	if ((buffer != NULL) && (bufferLen >= entry->valueLength)) // If the buffer is not null and can hold the value, place the value in the buffer.
	{
		if (ILibSimpleDataStore_SeekPosition(root->dataFile, entry->valueOffset, SEEK_SET) != 0) return 0; // Seek to the position of the value in the data store
		if (fread(buffer, 1, entry->valueLength, root->dataFile) == 0) return 0; // Read the value into the buffer
		util_sha256(buffer, entry->valueLength, hash); // Compute the hash of the read value
		if (memcmp(hash, entry->valueHash, SHA256HASHSIZE) != 0) return 0; // Check the hash, return 0 if not valid
	}
	return entry->valueLength;
}

// Get the reference to the SHA256 hash value from the datastore for a given a key.
__EXPORT_TYPE char* ILibSimpleDataStore_GetHashEx(ILibSimpleDataStore dataStore, char* key, int keyLen)
{
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibSimpleDataStore_TableEntry *entry = (ILibSimpleDataStore_TableEntry*)ILibHashtable_Get(root->keyTable, NULL, key, keyLen);

	if (entry == NULL) return NULL; // If there is no in-memory entry for this key, return zero now.
	return entry->valueHash;
}

// Delete a key and value from the data store
__EXPORT_TYPE int ILibSimpleDataStore_DeleteEx(ILibSimpleDataStore dataStore, char* key, int keyLen)
{
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibSimpleDataStore_TableEntry *entry = (ILibSimpleDataStore_TableEntry*)ILibHashtable_Remove(root->keyTable, NULL, key, keyLen);
	if (entry != NULL) { ILibSimpleDataStore_WriteRecord(root->dataFile, key, keyLen, NULL, 0, NULL); free(entry); return 1; }
	return 0;
}

// Lock the data store file
__EXPORT_TYPE void ILibSimpleDataStore_Lock(ILibSimpleDataStore dataStore)
{
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibHashtable_Lock(root->keyTable);
}

// Unlock the data store file
__EXPORT_TYPE void ILibSimpleDataStore_UnLock(ILibSimpleDataStore dataStore)
{
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	ILibHashtable_UnLock(root->keyTable);
}

// Called by the compaction method, for each key in the enumeration we write the key/value to the temporary data store
void ILibSimpleDataStore_Compact_EnumerateSink(ILibHashtable sender, void *Key1, char* Key2, int Key2Len, void *Data, void *user)
{
	ILibSimpleDataStore_TableEntry *entry = (ILibSimpleDataStore_TableEntry*)Data;
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)((void**)user)[0];
	FILE *compacted = (FILE*)((void**)user)[1];
	DS_Long offset;
	char value[4096];
	int valueLen;
	int bytesLeft = entry->valueLength;
	int totalBytesWritten = 0;
	int bytesWritten = 0;

	if (root->error != 0) return; // There was an error, ABORT!

	offset = ILibSimpleDataStore_WriteRecord(compacted, Key2, Key2Len, NULL, entry->valueLength, entry->valueHash);
	while (bytesLeft > 0)
	{
		if (ILibSimpleDataStore_SeekPosition(root->dataFile, entry->valueOffset + totalBytesWritten, SEEK_SET) == 0)
		{
			valueLen = (int)fread(value, 1, bytesLeft > 4096 ? 4096 : bytesLeft, root->dataFile);
			bytesWritten = (int)fwrite(value, 1, valueLen, compacted);
			if (bytesWritten != valueLen)
			{
				// Error
				root->error = 1;
				break;
			}
			totalBytesWritten += bytesWritten;
			bytesLeft -= valueLen;
		}
		else
		{
			// Error
			root->error = 1;
			break;
		}
	}
	
	if (root->error == 0) { entry->valueOffset = offset; }
}

// Used to help with key enumeration
void ILibSimpleDataStore_EnumerateKeysSink(ILibHashtable sender, void *Key1, char* Key2, int Key2Len, void *Data, void *user)
{
	ILibSimpleDataStore_KeyEnumerationHandler handler = (ILibSimpleDataStore_KeyEnumerationHandler)((void**)user)[0];
	ILibSimpleDataStore_KeyEnumerationHandler dataStore = (ILibSimpleDataStore)((void**)user)[1];
	ILibSimpleDataStore_KeyEnumerationHandler userX = ((void**)user)[2];

	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(Key1);
	UNREFERENCED_PARAMETER(Key2);
	UNREFERENCED_PARAMETER(Key2Len);

	handler(dataStore, Key2, Key2Len, userX); // Call the user
}

// Enumerate each key in the data store, call the handler for each key
__EXPORT_TYPE void ILibSimpleDataStore_EnumerateKeys(ILibSimpleDataStore dataStore, ILibSimpleDataStore_KeyEnumerationHandler handler, void * user)
{
	void* users[3];
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	
	users[0] = (void*)handler;
	users[1] = (void*)dataStore;
	users[2] = (void*)user;

	if (handler != NULL) { ILibHashtable_Enumerate(root->keyTable, ILibSimpleDataStore_EnumerateKeysSink, users); }
}

// Compact the data store
__EXPORT_TYPE int ILibSimpleDataStore_Compact(ILibSimpleDataStore dataStore)
{
	ILibSimpleDataStore_Root *root = (ILibSimpleDataStore_Root*)dataStore;
	char* tmp = ILibString_Cat(root->filePath, -1, ".tmp", -1); // Create the name of the temporary data store
	FILE* compacted;
	void* state[2];
	int retVal = 1;

	// Start by opening a temporary .tmp file. Will be used to write the compacted data store.
	if ((compacted = ILibSimpleDataStore_OpenFileEx(tmp, 1)) == NULL) { free(tmp);  return(1); }

	// Enumerate all keys and write them all into the temporary data store
	state[0] = root;
	state[1] = compacted;
	root->error = 0;
	ILibHashtable_Enumerate(root->keyTable, ILibSimpleDataStore_Compact_EnumerateSink, state);

	// Check if the enumeration went as planned
	if (root->error == 0)
	{
		// Success in writing new temporary file
		fclose(root->dataFile); // Close the data store
		fclose(compacted); // Close the temporary data store

		// Now we copy the temporary data store over the data store, making it the new valid version
#ifdef WIN32
		if (CopyFileA(tmp, root->filePath, FALSE) == FALSE) { root->dataFile = ILibSimpleDataStore_OpenFile(root->filePath); retVal = 2; }
#else
		if (rename(tmp, root->filePath) != 0) { root->dataFile = ILibSimpleDataStore_OpenFile(root->filePath); retVal = 2; }
#endif

		// We then open the newly compacted data store
		if ((root->dataFile = ILibSimpleDataStore_OpenFile(root->filePath)) != NULL)
		{
			root->fileSize = ILibSimpleDataStore_GetPosition(root->dataFile);
			if (retVal == 1) { retVal = 0; } // Success
		}
	}

	free(tmp); // Free the temporary file name
	return retVal; // Return 1 if we got an error, 0 if everything finished correctly
}


