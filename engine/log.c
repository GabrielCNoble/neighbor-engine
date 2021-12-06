#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include "backtrace.h"

FILE *log_output_file;
uint32_t log_use_stdout;
struct backtrace_state *log_backtrace_state;

#ifdef __cplusplus
extern "C"
{
#endif

void log_BacktraceErrorCallback(void *data, const char *message, int error)
{
    printf("%s\n", message);
}

void log_BacktraceSymbolCallback(void *data, uintptr_t pc, const char *symbol_name, uintptr_t symbol_value, uintptr_t symbol_size)
{
    printf("syminfo -- %p: %s[%p]\n", (void *)pc, symbol_name, (void *)symbol_value);
}

int log_BacktracePcCallback(void *data, uintptr_t pc, const char *file_name, int line, const char *function)
{
    printf("pcinfo -- %s [%d]: %s : %p\n", file_name, line, function, (void *)pc);
    return 0;
}

int log_BacktraceFullCallback(void *data, uintptr_t pc, const char *file_name, int line, const char *function)
{
    if(!file_name)
    {
        file_name = "??";
    }

    if(!function)
    {
        function = "??";
    }

    backtrace_syminfo(log_backtrace_state, pc, log_BacktraceSymbolCallback, log_BacktraceErrorCallback, NULL);
    backtrace_pcinfo(log_backtrace_state, pc, log_BacktracePcCallback, log_BacktraceErrorCallback, NULL);
    printf("%s [%d]: %s:%p\n", file_name, line, function, (void *)pc);
    return 0;
}

void log_SigSegv(int singal_value)
{
    log_LogMessage(LOG_TYPE_FATAL, "SIGSEGV: Halting and catching fire, please hold...\nBacktrace:\n");
//    backtrace_full(log_backtrace_state, 2, log_BacktraceFullCallback, log_BacktraceErrorCallback, NULL);
    log_FlushLog();
    fclose(log_output_file);
}

void log_Init(char *exe_name, uint32_t use_stdout)
{
    log_use_stdout = use_stdout;

    if(!log_use_stdout)
    {
        log_output_file = fopen("output.log", "w");
    }
    else
    {
        log_output_file = stdout;
    }
    signal(SIGSEGV, log_SigSegv);
    log_backtrace_state = backtrace_create_state(exe_name, 0, log_BacktraceErrorCallback, NULL);
}

void log_Shutdown()
{
    fclose(log_output_file);
}

void log_FlushLog()
{
    fflush(log_output_file);
}

void log_LogMessage(uint32_t type, char *format, ...)
{
    char *type_str;

    switch(type)
    {
        case LOG_TYPE_NOTICE:
            type_str = "NOTICE";
        break;

        case LOG_TYPE_WARNING:
            type_str = "WARNING";
        break;

        case LOG_TYPE_ERROR:
            type_str = "ERROR";
        break;

        case LOG_TYPE_FATAL:
            type_str = "FATAL";
        break;
    }

    va_list list;
    va_start(list, format);
    fprintf(log_output_file, "[%s]: ", type_str);
    vfprintf(log_output_file, format, list);
    fprintf(log_output_file, "\n");
    va_end(list);
}

#ifdef __cplusplus
}
#endif


