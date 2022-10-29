// Log helpers.
//
// Copyright (C) 2022 Oli Wright
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// A copy of the GNU General Public License can be found in the file
// COPYING.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.

// Simple logging to stdout.  To use, first declare a channel...
//
//     static LogChannel AboutFood(true /*switch this channel on*/);
//
// Then use it...
//
//    LOG_INFO( AboutFood, "Today I will be mostly eating %s.", menuToday);
//
// To completely disabling logging, make sure that LOG_ENABLED is
// defined to 0.

#pragma once
#include <cstdint>

#if !defined(LOG_ENABLED)
#define LOG_ENABLED 1
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
