#include "serial.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/bootrom.h"

static repeating_timer_t s_timer;

static constexpr int32_t kFrequencyHz = 5;

bool timerCallback(struct repeating_timer *t)
{
    if(getchar_timeout_us(0) == 'r')
    {
        reset_usb_boot(0, 0);
    }
    return true;
}

void Serial::Init()
{
#if defined(_PICO_STDIO_USB_H) || LOG_ENABLED || defined(_PICO_STDIO_UART_H)
    stdio_init_all();
    add_repeating_timer_us(-1000000 / kFrequencyHz, timerCallback, NULL, &s_timer);
#endif
#if defined(_PICO_STDIO_USB_H)
    sleep_ms(2000); //< Give a chance to attach a terminal
#endif
    printf("\033[2J\033[H***********\nHello\n***********\n\n");
}
