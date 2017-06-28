#include "humblenet.h"
#include "humblepeer.h"

#include <cassert>
#include <string>


ha_bool parseServerAddress(std::string server_string, std::string &realaddr, std::string &path, int &port, ha_bool &use_ssl);

#define START_TEST(server) { server_string = server; port = 0; use_ssl = false; }
#define START_TEST_PORT(server, p) { server_string = server; port = p; use_ssl = false; }

int main(int /* argc */, char * /* argv */ []) {
	std::string server_string;
	std::string realaddr;
	std::string path;
	ha_bool use_ssl = false;
	int port = 0;

	ha_bool retval = false;

	START_TEST("localhost:8080/ws")
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws");
	assert(!use_ssl);
	assert(port == 8080);


	START_TEST("ws://localhost:8080/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws");
	assert(!use_ssl);
	assert(port == 8080);

	START_TEST("ws://localhost/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws");
	assert(!use_ssl);
	assert(port == 80);

	START_TEST("wss://localhost:8080/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws");
	assert(use_ssl);
	assert(port == 8080);

	START_TEST("wss://localhost/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws");
	assert(use_ssl);
	assert(port == 443);

	START_TEST_PORT("wss://localhost/ws", 80);
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws");
	assert(use_ssl);
	assert(port == 80);


	START_TEST_PORT("wss://localhost/ws/ws", 80);
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(retval);
	assert(realaddr == "localhost");
	assert(path == "/ws/ws");
	assert(use_ssl);
	assert(port == 80);


	START_TEST("badprotocol://localhost/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(!retval);


	START_TEST("ws://localhost:-1/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(!retval);


	START_TEST("ws://localhost:131287/ws");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(!retval);


	START_TEST(":80");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(!retval);


	START_TEST("/ws/");
	retval = parseServerAddress(server_string, realaddr, path, port, use_ssl);
	assert(!retval);


	return 0;

}
