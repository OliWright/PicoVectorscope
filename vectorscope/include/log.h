#pragma once
#include <cstdint>

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

class LogChannel
{
#if LOG_ENABLED
public:
    LogChannel(bool initialEnabled);
    uint32_t GetId() const { return m_id; }
    bool IsEnabled() const { return m_enabled; }
private:
    uint32_t m_id;
    bool m_enabled;
#else
public:
    LogChannel(bool) {}
#endif
};

#if LOG_ENABLED
#define LOG_INFO(channel, fmt, ...) if(channel.IsEnabled()) Log::Print(fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(...)
#endif
