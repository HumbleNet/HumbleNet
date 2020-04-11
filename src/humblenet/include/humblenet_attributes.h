#ifndef HUMBLENET_ATTRIBUTES_H
#define HUMBLENET_ATTRIBUTES_H

#include "humblenet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct humblenet_AttributeMap humblenet_AttributeMap;
typedef struct humblenet_AttributeMapIT humblenet_AttributeMapIT;

HUMBLENET_API humblenet_AttributeMap* HUMBLENET_CALL humblenet_attributes_new();

HUMBLENET_API void HUMBLENET_CALL humblenet_attributes_free(humblenet_AttributeMap*);

HUMBLENET_API void HUMBLENET_CALL humblenet_attributes_set(humblenet_AttributeMap*, const char* key, const char* value);

HUMBLENET_API void HUMBLENET_CALL humblenet_attributes_unset(humblenet_AttributeMap*, const char* key);

HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_attributes_get(humblenet_AttributeMap*, const char* key, size_t buff_size, char* value_buff);

HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_attributes_contains(humblenet_AttributeMap*, const char* key);

HUMBLENET_API size_t HUMBLENET_CALL humblenet_attributes_size(humblenet_AttributeMap*);

HUMBLENET_API humblenet_AttributeMapIT* HUMBLENET_CALL humblenet_attributes_iterate(humblenet_AttributeMap*);

HUMBLENET_API void HUMBLENET_CALL humblenet_attributes_iterate_get_key(humblenet_AttributeMapIT*, size_t buff_size, char* key_buff);

HUMBLENET_API humblenet_AttributeMapIT* HUMBLENET_CALL humblenet_attributes_iterate_next(humblenet_AttributeMapIT*);

HUMBLENET_API void HUMBLENET_CALL humblenet_attributes_iterate_free(humblenet_AttributeMapIT*);


#ifdef __cplusplus
}
#endif

#endif /* HUMBLENET_ATTRIBUTES_H */