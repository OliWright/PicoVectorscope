#include "log.h"
#if LOG_ENABLED
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdarg.h>
#endif

void Log::Init()
{
#if LOG_ENABLED
    stdio_init_all();
    sleep_ms(2000); //< Give a chance to attach a terminal
#endif
}

#if LOG_ENABLED
void Log::Print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
#endif
