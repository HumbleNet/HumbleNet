#pragma once

#if __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

#ifdef NDEBUG
#define debug_out(x)
#else
#define debug_out(x) { std::cout << x << std::endl; }
#endif

#include <cassert>
#include <string>
#include <iostream>

#include "json.h"

NORETURN void die(const std::string& error, int code = 2);

std::string loadFile(const std::string& file);

json_value* get_object_key(json_value *root, const std::string& key);
std::string get_object_string_key(json_value *root, const std::string& key);
std::string get_object_string(json_value *val);

class jsonIteratorBase {
protected:
	json_value* m_root;
	jsonIteratorBase(json_value* root) : m_root(root) {}
};

class jsonObjectIterator : jsonIteratorBase {
public:
	jsonObjectIterator(json_value* root) : jsonIteratorBase(root) {
		assert(m_root->type == json_object);
	}

	json_object_entry* begin() const {
		return m_root->u.object.values;
	}
	json_object_entry* end() const {
		return m_root->u.object.values + m_root->u.object.length;
	}
};

class jsonArrayIterator : jsonIteratorBase {
public:
	jsonArrayIterator(json_value* root) : jsonIteratorBase(root) {
		assert(m_root->type == json_array);
	}

	json_value** begin() const {
		return m_root->u.array.values;
	}
	json_value** end() const {
		return m_root->u.array.values + m_root->u.array.length;
	}
};
