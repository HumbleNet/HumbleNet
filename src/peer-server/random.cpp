#include "random.h"

#pragma message ("TODO switch this to C++11 random generators")

#if 0 // randomization of Ids
#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <windows.h>
#elif defined(__APPLE__)
#	include <mach/mach_time.h>
#elif defined(__linux__)
#	include <time.h>
#endif

#ifdef _WIN32

LARGE_INTEGER temp;
QueryPerformanceCounter(&temp);
uint64_t seed1 = temp.QuadPart;

QueryPerformanceCounter(&temp);
uint64_t seed2 = temp.QuadPart;
#elif __APPLE__
uint64_t seed1 = mach_absolute_time();
uint64_t seed2 = mach_absolute_time();
#elif __linux__

// assume Linux
struct timespec t;
clock_gettime(CLOCK_MONOTONIC, &t);
uint64_t seed1 = t.tv_sec * 1000000ULL + t.tv_nsec;

clock_gettime(CLOCK_MONOTONIC, &t);
uint64_t seed2 = t.tv_sec * 1000000ULL + t.tv_nsec;
#else
#error Unhandled platform
#endif
#endif

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
	uint64_t oldstate = rng->state;
	// Advance internal state
	rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
	// Calculate output function (XSH RR), uses old state for max ILP
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}