#pragma once

#include <string>

struct tConfigOptions {
	int port;
	bool daemon;
	std::string iface;
	std::string sslCertFile;
	std::string sslPrivateKeyFile;
	std::string sslCACertFile;
	std::string sslCipherList;
	std::string pidFile;
	std::string logFile;
	std::string stunServerAddress;
	std::string gameDB;

	tConfigOptions() : port(8080), daemon(false) {}

	void parseFile(const std::string& file);
};
