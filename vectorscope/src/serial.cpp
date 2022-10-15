#include "serial.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/bootrom.h"

#if defined(_PICO_STDIO_USB_H) || LOG_ENABLED || defined(_PICO_STDIO_UART_H)
#define ENABLE_SERIAL 1
#else
#define ENABLE_SERIAL 0
#endif

#if ENABLE_SERIAL
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
#endif

void Serial::Init()
{
#if ENABLE_SERIAL
    stdio_init_all();
    add_repeating_timer_us(-1000000 / kFrequencyHz, timerCallback, NULL, &s_timer);
#if defined(_PICO_STDIO_USB_H)
    sleep_ms(2000); //< Give a chance to attach a terminal
#endif
    printf("\033[2J\033[H***********\nHello\n***********\n\n");
#endif
}
