#ifndef DACOUTPIOCONFIG_H
#define DACOUTPIOCONFIG_H

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

    void SetEnabled(bool enabled) const;
};

// 12-bit DAC value output here
static constexpr uint kDacOutBaseDacPin = 2;
static constexpr uint kNumDacOutDacPins = 12;
// Latch pins for X,Y and Z channels
static constexpr uint kDacOutBaseLatchPin = 16;
static constexpr uint kNumDacOutLatchPins = 3;


#endif
