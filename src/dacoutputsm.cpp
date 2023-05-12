// Static class owning our PIO SM programs
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

#include "dacoutputsm.h"
#include "log.h"
#include "pico/stdlib.h"
#include "fixedpoint.h"

// The headers for each individual PIO program
#include "idle.pio.h"
#include "vector.pio.h"
#include "points.pio.h"
#include "raster.pio.h"

#define DO_SPEED_RAMPING 1

DacOutputPioSmConfig DacOutputPioSm::s_configs[(int)DacOutputPioSm::SmID::eCount] = {};

struct ProgramInfo
{
    const pio_program_t* m_pProgram;
    uint m_wrapTarget;
    uint m_wrap;
    uint m_numSidesetPins;
    uint16_t m_clockDivider;
};
static const ProgramInfo s_programInfo[] = {
    {&idle_program,   idle_wrap_target,   idle_wrap,   2, 32},
    {&vector_program, vector_wrap_target, vector_wrap, 3, 3},
    {&points_program, points_wrap_target, points_wrap, 2, 32},
    {&raster_program, raster_wrap_target, raster_wrap, 2, 1},
};

static uint s_overloadedStateMachine;
static uint s_overloadedProgramOffset = 0;

void DacOutputPioSm::configureSm(SmID smID)
{
    const ProgramInfo& programInfo = s_programInfo[(int)smID];
    DacOutputPioSmConfig& outConfig = s_configs[(int)smID];

    outConfig.m_pio = pio0;
    outConfig.m_pProgram = programInfo.m_pProgram;
    outConfig.m_id = (uint32_t) smID;

    if(smID == SmID::eIdle)
    {
        // Find a free state machine on our chosen PIO (erroring if there are
        // none). Configure it to run our program, and start it, using the
        // helper function we included in our .s_pio file.
        outConfig.m_stateMachine = pio_claim_unused_sm(outConfig.m_pio, true);

        // We're going to load the idle program at the top of PIO instruction memory
        outConfig.m_programOffset = PIO_INSTRUCTION_COUNT - programInfo.m_pProgram->length;
        pio_add_program_at_offset(outConfig.m_pio, outConfig.m_pProgram, outConfig.m_programOffset);
        outConfig.m_overloaded = false;
    }
    else
    {
        // All other programs will be loaded on demand, at the beginning of PIO instruction memory
        outConfig.m_stateMachine = s_overloadedStateMachine;
        outConfig.m_programOffset = s_overloadedProgramOffset;
        outConfig.m_overloaded = true;
        assert(pio_can_add_program_at_offset(outConfig.m_pio, outConfig.m_pProgram, outConfig.m_programOffset));
    }


    // Set the pin direction to output at the PIO
    pio_sm_set_consecutive_pindirs(outConfig.m_pio, outConfig.m_stateMachine, kDacOutBaseDacPin, kNumDacOutDacPins, true);
    pio_sm_set_consecutive_pindirs(outConfig.m_pio, outConfig.m_stateMachine, kDacOutBaseLatchPin, programInfo.m_numSidesetPins, true);


    // Configure the state machine
    outConfig.m_config = pio_get_default_sm_config();
    sm_config_set_wrap(&outConfig.m_config, outConfig.m_programOffset + programInfo.m_wrapTarget, outConfig.m_programOffset + programInfo.m_wrap);
    sm_config_set_sideset(&outConfig.m_config, programInfo.m_numSidesetPins, false, false);

    // The 12 consecutive pins that are used for DAC output
    sm_config_set_out_pins(&outConfig.m_config, kDacOutBaseDacPin, kNumDacOutDacPins);
    // The base sideset pins that we use to latch the outputs
    sm_config_set_sideset_pins(&outConfig.m_config, kDacOutBaseLatchPin);

    // Set clock divider. Value of 1 for max speed.
    sm_config_set_clkdiv_int_frac(&outConfig.m_config, programInfo.m_clockDivider, 0);
    // Make a single 8 words FIFO from the 4 words TX and RX FIFOs.
    sm_config_set_fifo_join(&outConfig.m_config, PIO_FIFO_JOIN_TX);
    // The OSR register shifts to the right, disable auto-pull
    sm_config_set_out_shift(&outConfig.m_config, true, true, 0);

    if(!outConfig.m_overloaded)
    {
        // Load our configuration, and jump to the start of the program
        pio_sm_init(outConfig.m_pio, outConfig.m_stateMachine, outConfig.m_programOffset, &outConfig.m_config);
    }
}

bool g_overrideRasterSpeed = false;
#if DO_SPEED_RAMPING
static bool s_overridingRasterSpeed = false;
typedef FixedPoint<14,14,int32_t,int32_t> RasterSpeed;
RasterSpeed g_rasterSpeed = 0.001f;
RasterSpeed g_targetRasterSpeed = 1.f;
RasterSpeed g_rasterSpeedStep = 0.005f;

struct RasterSpeedPreset
{
    RasterSpeed speed;
    RasterSpeed targetSpeed;
    RasterSpeed speedStep;
};
static RasterSpeedPreset s_rasterSpeedPresets[] = 
{
    {0.003f, 0.003f, 0.f},
    {0.003f, 0.0005f, -0.0003f},
    {0.0005f, 0.0001f, -0.0001f},
    {0.0001f, 1.f, 0.02f},
};
static const int kNumRasterSpeedPresets = count_of(s_rasterSpeedPresets);
static int s_rasterSpeedPreset = -1;
static repeating_timer_t s_dacOutputTimer;

static void nextRasterSpeedPreset()
{
    if(++s_rasterSpeedPreset == kNumRasterSpeedPresets)
    {
        s_rasterSpeedPreset = 0;
    }
    g_rasterSpeed = s_rasterSpeedPresets[s_rasterSpeedPreset].speed;
    g_targetRasterSpeed = s_rasterSpeedPresets[s_rasterSpeedPreset].targetSpeed;
    g_rasterSpeedStep = s_rasterSpeedPresets[s_rasterSpeedPreset].speedStep;
}
#endif

bool DacOutputPioSm::timerCallback(struct repeating_timer* t)
{
#if DO_SPEED_RAMPING
    bool pressed = gpio_get(22);
    static bool was_pressed = false;
    if(pressed != was_pressed)
    {
        was_pressed = pressed;
        if(pressed)
        {
            nextRasterSpeedPreset();
        }
    }

    bool override = g_overrideRasterSpeed;
    if(g_overrideRasterSpeed != s_overridingRasterSpeed)
    {
        s_overridingRasterSpeed = g_overrideRasterSpeed;
        override = true;
    }
    if(g_overrideRasterSpeed)
    {
        g_rasterSpeed += g_rasterSpeedStep;
        if(((g_rasterSpeedStep > 0.f) && (g_rasterSpeed >= g_targetRasterSpeed)) || ((g_rasterSpeedStep < 0.f) && (g_rasterSpeed <= g_targetRasterSpeed)))
        {
            g_rasterSpeed = g_targetRasterSpeed;
            //g_overrideRasterSpeed = s_overridingRasterSpeed = false;
        }
    }
    if(override)
        {
        DacOutputPioSmConfig& config = s_configs[(int) SmID::eRaster];
        FixedPoint<16,8,uint32_t,uint32_t> clockDivider = g_rasterSpeed.recip();
        sm_config_set_clkdiv_int_frac(&config.m_config, clockDivider.getIntegerPart(), (uint8_t) (clockDivider.getStorage() & 0xff));
        pio_sm_set_config(pio0, s_overloadedStateMachine, &config.m_config);
    }
#endif
    return true;
}

void DacOutputPioSm::Init()
{
    // Associate all the pins we're going to use with the PIO
    for (uint i = 0; i < kNumDacOutDacPins; ++i)
    {
        pio_gpio_init(pio0, kDacOutBaseDacPin + i);
        //pio_gpio_init(pio1, kDacOutBaseDacPin + i);
    }
    for (uint i = 0; i < kNumDacOutLatchPins; ++i)
    {
        pio_gpio_init(pio0, kDacOutBaseLatchPin + i);
        //pio_gpio_init(pio1, kDacOutBaseLatchPin + i);
    }

    s_overloadedStateMachine = pio_claim_unused_sm(pio0, true);

    for(uint32_t i = 0; i < (uint32_t) SmID::eCount; ++i)
    {
        configureSm((SmID)i);
    }

#if DO_SPEED_RAMPING
    nextRasterSpeedPreset();
    add_repeating_timer_us(-1000000 / 10, timerCallback, NULL, &s_dacOutputTimer);
#endif
}

void DacOutputPioSmConfig::SetEnabled(bool enabled) const
{
    // Switch it on or off
    if(enabled)
    {
        if(m_overloaded)
        {
            // Upload the new program
            pio_add_program_at_offset(m_pio, m_pProgram, m_programOffset);
        }
        pio_sm_init(m_pio, m_stateMachine, m_programOffset, &m_config);
        pio_sm_set_enabled(m_pio, m_stateMachine, true);
    }
    else
    {
        pio_sm_set_enabled(m_pio, m_stateMachine, false);
        if(m_overloaded)
        {
            pio_remove_program(m_pio, m_pProgram, m_programOffset);
        }
    }
}
