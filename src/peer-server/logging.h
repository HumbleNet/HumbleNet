#pragma once

#include <string>
#include <libwebsockets.h>

void log_func_var(int level, const char* fmt, ...);
void logFileOpen(const std::string& logFile);

#define LOG_ERROR(msg, ...)   log_func_var(LLL_ERR, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) log_func_var(LLL_WARN, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)    log_func_var(LLL_NOTICE, msg, ##__VA_ARGS__)

