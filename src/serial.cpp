// Internal helper for handling input from stdin
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

#include "serial.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include <stdio.h>


#if defined(_PICO_STDIO_USB_H) || LOG_ENABLED || defined(_PICO_STDIO_UART_H)
#    define ENABLE_SERIAL 1
#else
#    define ENABLE_SERIAL 0
#endif

#if ENABLE_SERIAL
static repeating_timer_t s_timer;
static constexpr int32_t kFrequencyHz = 5;
#endif

char volatile Serial::s_lastCharIn = 0;

void Serial::Init()
{
#if ENABLE_SERIAL
    stdio_init_all();
    add_repeating_timer_us(-1000000 / kFrequencyHz, timerCallback, NULL, &s_timer);
#    if defined(_PICO_STDIO_USB_H)
    sleep_ms(2000); //< Give a chance to attach a terminal
#    endif
    printf("\033[2J\033[H***********\nHello\n***********\n\n");
#endif
}

bool Serial::timerCallback(struct repeating_timer* t)
{
    int getChar = getchar_timeout_us(0);

    if (getChar >= 0)
    {
        s_lastCharIn = (char)getChar;

        if (s_lastCharIn == 'r')
        {
            // We handle the reboot command here in case the main code is hung
            reset_usb_boot(0, 0);
        }
    }
    return true;
}
