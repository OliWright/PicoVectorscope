#include "ledstatus.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>

LedStatus::Step LedStatus::s_steps[LedStatus::kMaxSteps] = {};
repeating_timer_t LedStatus::s_timer;
uint32_t LedStatus::s_currentStep = 0;
uint32_t LedStatus::s_subStepTickCounter = 0;
uint32_t LedStatus::s_numSteps = 0;
bool LedStatus::s_interruptIsActive = false;

static constexpr int32_t kFrequencyHz = 2;

void LedStatus::Init()
{
    // for(uint32_t i = 0; i < kNumSteps; ++i)
    // {
    //     s_brightness[i] = Brightness((float) i / (kNumSteps - 1));
    //     printf("%d: %d\n", i, s_brightness[i].getStorage());
    // }

    //add_repeating_timer_us(-1000000 / kFrequencyHz, timerInterrupt, NULL, &s_timer);


    // Tell the LED pin that the PWM is in charge of its value.
    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_PWM);

    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, false);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 4.f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);
}

int64_t LedStatus::alarmCallback(alarm_id_t id, void *user_data)
{
    advance();
    const Step& step = s_steps[s_currentStep];

    // Set the LED
    // Square the brightness value to make the LED's brightness appear more linear
    // Note this range matches with the wrap value
    uint32_t bits = (step.m_brightness * step.m_brightness).getStorage() << 1;
    bits = (bits > 65535) ? 65535 : bits;
    pwm_set_gpio_level(PICO_DEFAULT_LED_PIN, bits);

    if(step.m_durationMs == 0)
    {
        // There are no active steps
        s_interruptIsActive = false;
        return 0;
    }
    // Reschedule this alarm
    return -step.m_durationMs * 1000;
}

void LedStatus::advance()
{
    // Look for a non-zero duration next step
    for(uint32_t i = 0; i < kMaxSteps; ++i)
    {
        if(++s_currentStep == kMaxSteps)
        {
            s_currentStep = 0;
        }
        if(s_steps[s_currentStep].m_durationMs != 0)
        {
            break;
        }
    }
}

void LedStatus::startTheAlarm()
{
    advance();

    s_interruptIsActive = true;
    add_alarm_in_ms(s_steps[s_currentStep].m_durationMs, alarmCallback, nullptr/*user_data*/, true/*fire_if_past*/);
}
