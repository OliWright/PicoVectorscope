#include "log.h"
#if LOG_ENABLED
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include <stdio.h>
#include <stdarg.h>
#endif

#if LOG_ENABLED
static mutex_t s_logMutex;
static uint s_numChannels = 0;
#endif

void Log::Init()
{
#if LOG_ENABLED
    mutex_init(&s_logMutex);
    stdio_init_all();
    sleep_ms(2000); //< Give a chance to attach a terminal
#endif
}

#if LOG_ENABLED
void Log::Print(const char *fmt, ...)
{
    mutex_enter_blocking(&s_logMutex);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    mutex_exit(&s_logMutex);
}
#endif

#if LOG_ENABLED
LogChannel::LogChannel(bool initialEnabled)
: m_enabled(initialEnabled)
, m_id(s_numChannels++)
{}
#endif
