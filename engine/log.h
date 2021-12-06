#ifndef LOG_H
#define LOG_H

#include <stdint.h>

enum LOG_TYPES
{
    LOG_TYPE_NOTICE = 0,
    LOG_TYPE_WARNING,
    LOG_TYPE_ERROR,
    LOG_TYPE_FATAL
};

#ifdef __cplusplus
extern "C"
{
#endif

void log_Init(char *exe_name, uint32_t use_stdout);

void log_Shutdown();

void log_FlushLog();

void log_LogMessage(uint32_t type, char *format, ...);

#define log_ScopedLogMessage(type, format, ...) log_LogMessage(type, "%s: " format, __func__ __VA_OPT__(,) __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // LOG_H
