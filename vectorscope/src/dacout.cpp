#include "log.h"
#include "dacout.h"
#include "pico/time.h"
#include "pico/sync.h"

const DacOutputPioSmConfig* DacOutput::s_currentPioConfig = nullptr;
const DacOutputPioSmConfig* DacOutput::s_previousPioConfig = nullptr;
DacOutput::DmaChannel DacOutput::s_dmaChannels[kNumBuffers];
uint32_t DacOutput::s_buffers[kNumBuffers][kNumEntriesPerBuffer];
uint32_t DacOutput::s_currentBufferIdx = 0;
uint32_t DacOutput::s_currentEntryIdx = 0;
const DacOutputPioSmConfig* DacOutput::s_idlePioSmConfig = nullptr;
uint32_t DacOutput::s_nextBufferIrq = 0;
volatile uint32_t DacOutput::s_numDmaChannelsQueued = 0;
uint64_t DacOutput::s_frameStartUs = 0;
uint64_t DacOutput::s_frameDurationUs = 0;

static const DacOutputPioSmConfig* s_activePioSmConfig = nullptr;

static critical_section_t s_dmaCriticalSection = {};

static LogChannel DacOutputSynchronisation(false);

void DacOutput::Init(const DacOutputPioSmConfig& idleSm)
{
    s_idlePioSmConfig = &idleSm;
    critical_section_init(&s_dmaCriticalSection);

    // Claim and initialise a DMA channels for each buffer
    for (uint32_t i = 0; i < kNumBuffers; ++i)
    {
        s_dmaChannels[i].m_channelIdx = dma_claim_unused_channel(true);
    }
    for (uint32_t i = 0; i < kNumBuffers; ++i)
    {
        s_dmaChannels[i].Init(s_buffers[i], s_dmaChannels[(i+1)%kNumBuffers].m_channelIdx);
    }
    // Configure the processor to run dmaIrqHandler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, dmaIrqHandler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void DacOutput::DmaChannel::Init(uint32_t* pBufferBase, int chainToDmaChannelIdx)
{
    m_pBufferBase = pBufferBase;
    m_configWithChain = dma_channel_get_default_config(m_channelIdx);
    m_complete = true;
    m_isFinal = false;

    // Transfer 32 bits each time
    channel_config_set_transfer_data_size(&m_configWithChain, DMA_SIZE_32);
    // Increment read address
    channel_config_set_read_increment(&m_configWithChain, true);
    // Write to the same address (the PIO SM TX FIFO)
    channel_config_set_write_increment(&m_configWithChain, false);
    // Disable address wrapping
    channel_config_set_ring(&m_configWithChain, false, 0);
    // Transfer when PIO SM TX FIFO has space
    //channel_config_set_dreq(&m_configWithChain, );
    m_configWithoutChain = m_configWithChain;
    // Kick off the next channel when complete
    channel_config_set_chain_to(&m_configWithChain, chainToDmaChannelIdx);
    dma_channel_set_irq0_enabled(m_channelIdx, true);

    // Setup the channel
    dma_channel_configure(m_channelIdx, &m_configWithoutChain,
                          nullptr, // Write to PIO TX FIFO
                          pBufferBase, // Read values from appropriate outputBuffer
                          0,
                          false // don't start yet
    );
}

void DacOutput::Flush(bool finalFlushForFrame)
{
    if (s_currentEntryIdx == 0)
    {
        // Nothing to flush
        return;
    }

    LOG_INFO(DacOutputSynchronisation, "Flush [%d, %d]\n", s_currentBufferIdx, s_currentEntryIdx);
    // Our new buffer is ready to go, so let's configure its DMA channel with
    // the correct size.
    DmaChannel& dmaChannel = s_dmaChannels[s_currentBufferIdx];
    if(finalFlushForFrame)
    {
        // Tell the DMA to raise IRQ line 0 when the channel finishes a block
        dmaChannel.m_isFinal = true;
    }

    // Configure the DMA channel for the current PIO program
    const DacOutputPioSmConfig& pioConfig = *s_currentPioConfig;
    io_wo_32* pioTxFifo = &(pioConfig.m_pio->txf[pioConfig.m_stateMachine]);
    uint32_t dreq = pio_get_dreq(pioConfig.m_pio, pioConfig.m_stateMachine, true);
    dmaChannel.ConfigurePioSm(pioTxFifo, dreq);
    dmaChannel.m_pDacOutputPioSmConfigToSet = nullptr;

    critical_section_enter_blocking(&s_dmaCriticalSection);
        dmaChannel.Enable(s_currentEntryIdx);
        if(s_currentPioConfig == s_previousPioConfig)
        {
            // Enable chaining on the previous DMA channel so that if it's still running,
            // it will chain to this one automatically
            s_dmaChannels[(s_currentBufferIdx + kNumBuffers - 1) % kNumBuffers].EnableChaining();
        }
        else
        {
            // We can't chain it, because it requires a change in PIO SM program
            dmaChannel.m_pDacOutputPioSmConfigToSet = s_currentPioConfig;
            s_previousPioConfig = s_currentPioConfig;
        }

        if(++s_numDmaChannelsQueued == 1)
        {
            LOG_INFO(DacOutputSynchronisation, "Flush Kick %d\n", s_currentBufferIdx);
            configureAndStartDma(*s_currentPioConfig, dmaChannel);
        }
    critical_section_exit(&s_dmaCriticalSection);

    // Move on to the next buffer
    if (++s_currentBufferIdx == kNumBuffers)
    {
        s_currentBufferIdx = 0;
    }
    s_currentEntryIdx = 0;
    // Wait for the DMA to complete on our new buffer.
    DmaChannel& nextDmaChannel = s_dmaChannels[s_currentBufferIdx];
    LOG_INFO(DacOutputSynchronisation, "Wait [%d]\n", s_currentBufferIdx);
    while(!nextDmaChannel.m_complete)
    {
        tight_loop_contents();
    }
    LOG_INFO(DacOutputSynchronisation, "Done [%d]\n", s_currentBufferIdx);

    //nextDmaChannel.Wait();
    // Prevent this channel from doing anything before we've finished filling in its buffer
    nextDmaChannel.Disable();

    if(finalFlushForFrame)
    {
        // The IRQ handler might put the PIO into idle state at the end of the 
        // frame if there's nothing left to do.
        // So let's make sure that we always assert the correct PIO SM program
        // for the first flush of the next frame
        s_previousPioConfig = nullptr;
    }

}

bool DacOutput::isDmaRunning()
{
    for (uint32_t i = 1; i <= kNumBuffers; ++i)
    {
        if (s_dmaChannels[(s_currentBufferIdx + i) % kNumBuffers].IsBusy())
        {
            return true;
        }
    }
    return false;
}

void DacOutput::wait()
{
    LOG_INFO(DacOutputSynchronisation, "Big wait: [");
    while(s_numDmaChannelsQueued > 0)
    {
        tight_loop_contents();
    }
    LOG_INFO(DacOutputSynchronisation, "]\n");
}

void DacOutput::dmaIrqHandler()
{
    critical_section_enter_blocking(&s_dmaCriticalSection);
    DmaChannel& dmaChannel = s_dmaChannels[s_nextBufferIrq];
    LOG_INFO(DacOutputSynchronisation, "IRQ %d %d\n", s_nextBufferIrq, dma_channel_get_irq0_status(dmaChannel.m_channelIdx) ? 1 : 0);
    bool makeIdle = false;
    if(dmaChannel.m_isFinal)
    {
        dmaChannel.m_isFinal = false;
        s_frameDurationUs = time_us_64() - s_frameStartUs;
        s_frameStartUs = 0;
        makeIdle = true;
    }

    s_nextBufferIrq = (s_nextBufferIrq + 1) % kNumBuffers;
    dma_channel_acknowledge_irq0(dmaChannel.m_channelIdx);

    if((--s_numDmaChannelsQueued > 0))
    {
        // There are more buffers queued.
        // But chaining would have been used unless a change in PIO SM program
        // is required.
        DmaChannel& nextDmaChannel = s_dmaChannels[s_nextBufferIrq];
        // We need to configure the PIO, and kick off
        // this DMA channel right here.
        if(nextDmaChannel.m_pDacOutputPioSmConfigToSet != nullptr)
        {
            LOG_INFO(DacOutputSynchronisation, "IRQ Kick %d\n", s_nextBufferIrq);
            configureAndStartDma(*nextDmaChannel.m_pDacOutputPioSmConfigToSet, nextDmaChannel);
            makeIdle = false;
        }
    }

    if(makeIdle)
    {
        // Activate the idle SM
        setActivePioSm(*s_idlePioSmConfig);
    }
    
    dmaChannel.m_complete = true;
    critical_section_exit(&s_dmaCriticalSection);
}

void DacOutput::setActivePioSm(const DacOutputPioSmConfig& config)
{
    if(s_activePioSmConfig != &config)
    {
        LOG_INFO(DacOutputSynchronisation, "Switch SM: %d\n", config.m_stateMachine);
        if(s_activePioSmConfig != nullptr)
        {
            // Wait for the current PIO SM to drain its FIFO
            while(!pio_sm_is_tx_fifo_empty(s_activePioSmConfig->m_pio, s_activePioSmConfig->m_stateMachine))
            {
                tight_loop_contents();
            }
            // Turn off the current PIO SM
            s_activePioSmConfig->SetEnabled(false);
        }

        // Turn on the new PIO SM
        config.SetEnabled(true);
        s_activePioSmConfig = &config;
        LOG_INFO(DacOutputSynchronisation, "Done Switch SM: %d\n", config.m_stateMachine);
    }
}

void DacOutput::configureAndStartDma(const DacOutputPioSmConfig& pioConfig, DmaChannel& dmaChannel)
{
    if(s_frameStartUs == 0)
    {
        s_frameStartUs = time_us_64();
    }
    setActivePioSm(pioConfig);
    dmaChannel.Start();
}

void DacOutput::SetCurrentPioSm(const DacOutputPioSmConfig& config)
{
    if(s_currentPioConfig == &config)
    {
        // No change.  Don't do anything
        return;
    }
    Flush();
    s_currentPioConfig = &config;
}
