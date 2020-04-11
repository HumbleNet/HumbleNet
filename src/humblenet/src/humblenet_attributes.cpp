#include "humblenet_attributes.h"

#include <utility>
#include <cstring>

#include "humblenet_types_internal.h"

#if defined(WIN32) && !defined(strncpy) && _MSC_VER < 1900
#define strncpy(dst, src, len) strcpy_s(dst, len, src)
#endif

struct humblenet_AttributeMap {
	humblenet::AttributeMap attributes;

	humblenet_AttributeMap() = default;
	explicit humblenet_AttributeMap(humblenet::AttributeMap  attribs) : attributes(std::move(attribs)) {}
};

struct humblenet_AttributeMapIT {
	humblenet::AttributeMap::const_iterator it;
	humblenet::AttributeMap& attribs;

	explicit humblenet_AttributeMapIT(const humblenet::AttributeMap::const_iterator& _it, humblenet::AttributeMap& _a)
			: it( _it ), attribs( _a ) {}
};

humblenet_AttributeMap* HUMBLENET_CALL humblenet_attributes_new()
{
	return new humblenet_AttributeMap;
}

humblenet_AttributeMap* humblenet_attributes_new_from(const humblenet::AttributeMap& attributes)
{
	return new humblenet_AttributeMap(attributes);
}

void HUMBLENET_CALL humblenet_attributes_free(humblenet_AttributeMap* attribs)
{
	delete attribs;
}

void HUMBLENET_CALL humblenet_attributes_set(humblenet_AttributeMap* attribs, const char* key, const char* value)
{
	if (attribs) {
		attribs->attributes[key] = value;
	}
}

void HUMBLENET_CALL humblenet_attributes_unset(humblenet_AttributeMap* attribs, const char* key)
{
	if (attribs) {
		attribs->attributes.erase( key );
	}
}

ha_bool HUMBLENET_CALL
humblenet_attributes_get(humblenet_AttributeMap* attribs, const char* key, size_t buff_size, char* value_buff)
{
	if (attribs) {
		auto it = attribs->attributes.find( key );
		if (it != attribs->attributes.end()) {
			if (value_buff) {
				strncpy( value_buff, it->second.c_str(), buff_size );
			}
			return true;
		}
	}
	return false;
}

humblenet::AttributeMap& humblenet_attributes_get_map(humblenet_AttributeMap* attribs)
{
	return attribs->attributes;
}

ha_bool HUMBLENET_CALL humblenet_attributes_contains(humblenet_AttributeMap* attribs, const char* key)
{
	return attribs->attributes.count( key ) > 0;
}

size_t HUMBLENET_CALL humblenet_attributes_size(humblenet_AttributeMap* attribs)
{
	return attribs->attributes.size();
}

humblenet_AttributeMapIT* HUMBLENET_CALL humblenet_attributes_iterate(humblenet_AttributeMap* attribs)
{
	if (attribs) {
		auto it = attribs->attributes.cbegin();

		if (it != attribs->attributes.cend()) {
			return new humblenet_AttributeMapIT( it, attribs->attributes );
		}
	}
	return nullptr;
}

void HUMBLENET_CALL humblenet_attributes_iterate_get_key(humblenet_AttributeMapIT* pit, size_t buff_size, char* key_buff)
{
	if (pit) {
		if (key_buff) {
			strncpy( key_buff, pit->it->first.c_str(), buff_size );
		}
	}
}

humblenet_AttributeMapIT* HUMBLENET_CALL humblenet_attributes_iterate_next(humblenet_AttributeMapIT* pit)
{
	if (pit) {
		++(pit->it);

		if (pit->it == pit->attribs.cend()) {
			delete pit;
			pit = nullptr;
		}
		return pit;
	}

	return nullptr;
}

void HUMBLENET_CALL humblenet_attributes_iterate_free(humblenet_AttributeMapIT* pit)
{
	delete pit;
}
