#ifndef LOG_HPP
#define LOG_HPP

#include <cstdarg>

#define LOG_VERBOSE 0

void log_info(const char* format, ...);
void log_warning(const char* format, ...);
void log_error(const char* format, ...);
void log_debug(const char* format, ...);

#endif // LOG_HPP
