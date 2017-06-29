#include "random.h"

#include <cstdint>

#include <random>
#include <memory>
#include <chrono>
#include <sstream>

static std::unique_ptr<std::mt19937> randomGenerator;
static std::uniform_int_distribution<uint8_t> randomRange(0, 0xff);

void initDevice()
{
	if (!randomGenerator) {
		auto seed = std::chrono::system_clock::now().time_since_epoch().count();

		randomGenerator.reset(new std::mt19937(seed));
	}
}

std::string generate_random_hash(const std::string& unique_data, size_t length)
{
	initDevice();

	std::stringstream stream;

	auto& rg = *randomGenerator.get();

	stream << unique_data;
	stream << '-';
	stream << std::hex;
	stream.fill('0');
	stream.width(2);

	for (int c = 0; c < length; ++c)
	{
		stream << (int)randomRange(rg);
	}

	return stream.str();
}
