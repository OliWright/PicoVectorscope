// Static class for using the built-in led to flash status messages
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
// LICENSE.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.
//
// oli.wright.github@gmail.com

// TODO: Document this.

#pragma once
#include "fixedpoint.h"
#include "pico/stdlib.h"
#include "types.h"

class LedStatus
{
public:
    typedef FixedPoint<3, 15, uint32_t, uint32_t> Brightness;
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
