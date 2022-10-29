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
: m_id(s_numChannels++)
, m_enabled(initialEnabled)
{}
#endif
