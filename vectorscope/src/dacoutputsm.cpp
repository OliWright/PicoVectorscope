#include "dacoutputsm.h"
#define LOG_ENABLED 0
#include "log.h"

#include "vector.pio.h"
#include "points.pio.h"
#include "idle.pio.h"

DacOutputPioSmConfig DacOutputSm::s_configs[(int)DacOutputSm::SmID::eCount] = {};

struct ProgramInfo
{
    const pio_program_t* m_pProgram;
    uint m_wrapTarget;
    uint m_wrap;
    PIO m_pio;
    uint m_numSidesetPins;
    uint16_t m_clockDivider;
};
static const ProgramInfo s_programInfo[] = {
    {&vector_program, vector_wrap_target, vector_wrap, pio0, 3, 2},
    {&points_program, points_wrap_target, points_wrap, pio0, 2, 32},
    {&idle_program,   idle_wrap_target,   idle_wrap,   pio0, 2, 32},
};

void DacOutputSm::configureSm(SmID smID)
{
    const ProgramInfo& programInfo = s_programInfo[(int)smID];
    DacOutputPioSmConfig& outConfig = s_configs[(int)smID];

    outConfig.m_pio = programInfo.m_pio;

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .s_pio file.
    outConfig.m_stateMachine = pio_claim_unused_sm(outConfig.m_pio, true);


    // Set the pin direction to output at the PIO
    pio_sm_set_consecutive_pindirs(outConfig.m_pio, outConfig.m_stateMachine, kDacOutBaseDacPin, kNumDacOutDacPins, true);
    pio_sm_set_consecutive_pindirs(outConfig.m_pio, outConfig.m_stateMachine, kDacOutBaseLatchPin, programInfo.m_numSidesetPins, true);

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember this location!
    outConfig.m_programOffset = pio_add_program(outConfig.m_pio, programInfo.m_pProgram);

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

    // Load our configuration, and jump to the start of the program
    pio_sm_init(outConfig.m_pio, outConfig.m_stateMachine, outConfig.m_programOffset, &outConfig.m_config);
}

void DacOutputSm::Init()
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

    for(uint32_t i = 0; i < (uint32_t) SmID::eCount; ++i)
    {
        configureSm((SmID)i);
    }
}

void DacOutputPioSmConfig::SetEnabled(bool enabled) const
{
    // Switch it on or off
    if(enabled)
    {
        pio_sm_restart(m_pio, m_stateMachine);
        pio_sm_exec(m_pio, m_stateMachine, pio_encode_jmp(m_programOffset));
    }
    pio_sm_set_enabled(m_pio, m_stateMachine, enabled);
}