#pragma once

#if !defined(LOG_ENABLED)
#define LOG_ENABLED 0
#endif

class Log
{
public:
    static void Init();
#if LOG_ENABLED
    static void Print(const char *fmt, ...);
#endif
};

#if LOG_ENABLED
#define LOG_INFO(fmt,...) Log::Print(fmt,##__VA_ARGS__)
#else
#define LOG_INFO(...)
#endif
