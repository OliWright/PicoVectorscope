#include "buttons.h"
#include "pico/stdlib.h"
#include "pico/time.h"

static constexpr uint kNumButtons = (uint) Buttons::Id::Count;

static const uint32_t s_gpioPins[] =
{
    22, // Left
    21, // Right
    20, // Thrust
    19, // Fire
};

static_assert(count_of(s_gpioPins) == kNumButtons, "");

static uint s_buttonWasPressed = 0;
static uint s_buttonIsPressed = 0;
static uint64_t s_timePressedMs[kNumButtons] = {0};
static uint64_t s_lastUpdateTimeMs = 0;

void Buttons::Init()
{
    for(uint i = 0; i < kNumButtons; ++i)
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
    for(uint i = 0; i < kNumButtons; ++i)
    {
        // High is not pressed
        if(gpio_get(s_gpioPins[i]))
        {
            s_buttonIsPressed &= ~(1 << i);
        }
        else
        {
            s_buttonIsPressed |= (1 << i);
            if((s_buttonWasPressed & (1 << i)) == 0)
            {
                s_timePressedMs[i] = s_lastUpdateTimeMs;
            }
        }
    }
}

bool Buttons::IsJustPressed(Id id)
{
    return IsHeld(id) && ((s_buttonWasPressed & (1 << (int) id)) == 0);
}

bool Buttons::IsHeld(Id id)
{
    return (s_buttonIsPressed & (1 << (int) id)) != 0;
}

uint64_t Buttons::HoldTimeMs(Id id)
{
    if(IsHeld(id))
    {
        return s_lastUpdateTimeMs - s_timePressedMs[(int)id];
    }
    return 0;
}
