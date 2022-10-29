// Information about a specific PIO SM program
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

#pragma once
#include <cstdint>
#include "hardware/pio.h"

struct DacOutputPioSmConfig
{
    PIO  m_pio;
    uint m_stateMachine;
    uint m_programOffset;
    bool m_overloaded;
    const pio_program_t* m_pProgram;
    pio_sm_config m_config;
    uint32_t m_id;

    void SetEnabled(bool enabled) const;
};

// 12-bit DAC value output here
static constexpr uint kDacOutBaseDacPin = 2;
static constexpr uint kNumDacOutDacPins = 12;
// Latch pins for X,Y and Z channels
static constexpr uint kDacOutBaseLatchPin = 16;
static constexpr uint kNumDacOutLatchPins = 3;
