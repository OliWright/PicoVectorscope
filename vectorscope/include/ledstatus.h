#pragma once

#include "fixedpoint.h"
#include "pico/stdlib.h"
#include "types.h"


class LedStatus
{
public:
    typedef FixedPoint<15, uint32_t, uint32_t> Brightness;
    static void Init();

    static constexpr uint32_t kMaxSteps = 32;
    static void SetStep(uint32_t step, Brightness brightness, uint32_t durationMs = 200)
    {
        s_steps[step].m_brightness = brightness;
        s_steps[step].m_durationMs = (int32_t) durationMs;

        if(!s_interruptIsActive && (durationMs != 0))
        {
            startTheAlarm();
        }
    }

private:
    static int64_t alarmCallback(alarm_id_t id, void *user_data);
    static void advance();
    static void startTheAlarm();

private:
    struct Step
    {
        Brightness m_brightness;
        int32_t    m_durationMs;
    };

    static Step s_steps[kMaxSteps];
    static uint32_t s_currentStep;
    static uint32_t s_subStepTickCounter;
    static uint32_t s_numSteps;
    static bool s_interruptIsActive;
};
