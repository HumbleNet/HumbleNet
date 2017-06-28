#include "logging.h"

#include <libwebsockets.h>
#include <cstdarg>
#include <cstdio>

static FILE* logFP = NULL;

const char* logLevel(int level)
{
	switch(level) {
		case LLL_ERR: return "ERROR";
		case LLL_WARN: return "WARNING";
		case LLL_NOTICE: return "NOTICE";
		case LLL_INFO: return "INFO";
		case LLL_DEBUG: return "DEBUG";
		case LLL_PARSER: return "PARSER";
		case LLL_HEADER: return "HEADER";
		case LLL_EXT: return "EXT";
		case LLL_CLIENT: return "CLIENT";
		case LLL_LATENCY: return "LATENCY";
		default: return "UNKNOWN";
	}
}

std::string logTime()
{
#ifdef _WIN32
	return std::string("");
#else
	struct tm tparts;
	struct timeval t;
	gettimeofday(&t, NULL);
	gmtime_r(&t.tv_sec, &tparts);
	char buff[100], buff2[40];
	strftime(buff2, sizeof(buff2), "%y-%m-%d %H:%M:%S", &tparts);
	snprintf(buff, sizeof(buff), "%s.%03d", buff2, t.tv_usec / 1000);
	return std::string(buff);
#endif
}

void log_func_var(int level, const char* fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	fprintf(logFP, "[%s]: %s: ", logTime().c_str(), logLevel(level));
	vfprintf(logFP, fmt, vl);
	va_end(vl);
}

void log_func(int level, const char* message)
{
	fprintf(logFP, "[%s]: %s: %s", logTime().c_str(), logLevel(level), message);
}

void logFileOpen(const std::string& logFile)
{
	if (logFile.empty()) {
		logFP = stderr;
	} else {
#if defined(WIN32)
		fopen_s(&logFP, logFile.c_str(), "a");
#else
		logFP = fopen(logFile.c_str(), "a");
#endif
		setvbuf(logFP, NULL, _IOLBF, BUFSIZ);
	}
	lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, &log_func);
}

