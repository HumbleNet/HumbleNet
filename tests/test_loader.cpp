#include "humblenet.h"

#include <iostream>

#define debug_out(x) { std::cout << x << std::endl; }

int main(int argc, char* argv[])
{
	debug_out("Initializing HumbleNet");
	if (!humblenet_init()) {
		debug_out("HumbleNet init failed : " << humblenet_get_error());
		return 1;
	}
	const char* err = humblenet_get_error();
	debug_out("HumbleNet Init Success");
	humblenet_shutdown();
	debug_out("HumbleNet shutdown");

	debug_out("Initializing HumbleNet (Round 2)");
	if (!humblenet_init()) {
		debug_out("HumbleNet init failed : " << humblenet_get_error());
		return 1;
	}
	err = humblenet_get_error();
	debug_out("HumbleNet Init Success");
	humblenet_shutdown();
	debug_out("HumbleNet shutdown");

	return 0;
}