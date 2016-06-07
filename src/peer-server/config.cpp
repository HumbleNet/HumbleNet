#include "config.h"

#include <fstream>

#define CONFIG_STRING(key) CONFIG_STRING_EX(key, key)
#define CONFIG_STRING_EX(k, v) else if (key == #k) { v = value; }

void tConfigOptions::parseFile(const std::string& file)
{
	std::ifstream in(file.c_str());

	if (!in) return;

	std::string line;
	for (std::getline(in, line); in; std::getline(in, line)) {
		// skip comments
		if (line[0] == '#') continue;

		auto pos = line.find('=');
		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);
		if (key == "port") {
			port = std::stoul(value);
		} else if (key == "daemon") {
			daemon = (value == "yes" || value == "1");
		}
		CONFIG_STRING(iface)
		CONFIG_STRING(sslCertFile)
		CONFIG_STRING(sslPrivateKeyFile)
		CONFIG_STRING(sslCACertFile)
		CONFIG_STRING(sslCipherList)
		CONFIG_STRING_EX(pidfile, pidFile)
		CONFIG_STRING_EX(logfile, logFile)
		CONFIG_STRING(stunServerAddress)
		CONFIG_STRING(gameDB)
		CONFIG_STRING(peerDB)
	}
}

