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

#include <nlohmann/json.hpp>

using json = nlohmann::json;

NORETURN void die(const std::string& error, int code = 2);

std::string loadFile(const std::string& file);

template<typename T>
T json_optional(const json& root, const std::string& key, const T& def = T())
{
	if (root.is_object() && root.contains(key)) {
		return root[key].get<T>();
	} else {
		return def;
	}
}