// Static class for handling buttons attached to GPIO inputs
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

// This is a work-in-progress.  Currently it is using hard-coded
// GPIO pins and button definitions.
//
// TODO: Make it configurable.

#include "buttons.h"

#include "pico/stdlib.h"
#include "pico/time.h"


static constexpr uint kNumButtons = (uint)Buttons::Id::Count;

static const uint32_t s_gpioPins[] = {
    19, // Left
    20, // Right
    21, // Thrust
    22, // Fire
};

static_assert(count_of(s_gpioPins) == kNumButtons, "");

// Stores the bit-mask of which buttons were pressed in the previous frame
static uint s_buttonWasPressed = 0;
// Stores the bit-mask of which buttons were pressed in this frame
static uint s_buttonIsPressed = 0;
// The absolute time in milliseconds that each button was initially pressed
static uint64_t s_timePressedMs[kNumButtons] = { 0 };
// The absolute time in milliseconds of the most recent call to Buttons::Update
static uint64_t s_lastUpdateTimeMs = 0;

void Buttons::Init()
{
    for (uint i = 0; i < kNumButtons; ++i)
    {
        gpio_init(s_gpioPins[i]);
        gpio_set_dir(s_gpioPins[i], GPIO_IN);
        gpio_pull_up(s_gpioPins[i]);
    }
}

void Buttons::Update()
{
    s_buttonWasPressed = s_buttonIsPressed;
    s_lastUpdateTimeMs = time_us_64() / 1000;
    for (uint i = 0; i < kNumButtons; ++i)
    {
        if (gpio_get(s_gpioPins[i]))
        {
            // High (not pressed)
            s_buttonIsPressed &= ~(1 << i);
        }
        else
        {
            // Low (pressed)
            s_buttonIsPressed |= (1 << i);
            if ((s_buttonWasPressed & (1 << i)) == 0)
            {
                // It wasn't pressed on the previous update
                s_timePressedMs[i] = s_lastUpdateTimeMs;
            }
        }
    }
}

bool Buttons::IsJustPressed(Id id)
{
    return IsHeld(id) && ((s_buttonWasPressed & (1 << (int)id)) == 0);
}

void Buttons::ClearJustPressed(Id id)
{
    s_buttonWasPressed |= (1 << (int)id);
}

bool Buttons::IsHeld(Id id) { return (s_buttonIsPressed & (1 << (int)id)) != 0; }

void Buttons::FakePress(Id id, bool pressed)
{
    uint bit = 1 << (int) id;
    if (pressed)
    {
        s_buttonIsPressed |= bit;
        if ((s_buttonWasPressed & bit) == 0)
        {
            // It wasn't pressed on the previous update
            s_timePressedMs[(int) id] = s_lastUpdateTimeMs;
        }
    }
    else
    {
        s_buttonIsPressed &= ~bit;
    }
}


uint64_t Buttons::HoldTimeMs(Id id)
{
    if (IsHeld(id))
    {
        return s_lastUpdateTimeMs - s_timePressedMs[(int)id];
    }
    return 0;
}
