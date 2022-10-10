#define LOG_ENABLED 0
#include "log.h"
#include "dacout.h"
#include "pico/time.h"
#include "pico/sync.h"

const DacOutputPioSmConfig* DacOutput::s_currentPioConfig = nullptr;
DacOutput::DmaChannel DacOutput::s_dmaChannels[kNumBuffers];
uint32_t DacOutput::s_buffers[kNumBuffers][kNumEntriesPerBuffer];
uint32_t DacOutput::s_currentBufferIdx = 0;
uint32_t DacOutput::s_currentEntryIdx = 0;
bool DacOutput::s_previousFrameRunning = false;
const DacOutputPioSmConfig* DacOutput::s_pendingDacOutputPioSmConfigChange = nullptr;
const DacOutputPioSmConfig* DacOutput::s_idlePioSmConfig = nullptr;
uint32_t DacOutput::s_nextBufferIrq = 0;
uint32_t DacOutput::s_numDmaChannelsRunning = 0;

static critical_section_t s_dmaCriticalSection = {};

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
    m_complete = false;
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
    if(s_previousFrameRunning)
    {
        LOG_INFO("Wait IRQ\n");
        wait();
    }
    if(s_pendingDacOutputPioSmConfigChange)
    {
        const DacOutputPioSmConfig& config = *s_pendingDacOutputPioSmConfigChange;
        s_pendingDacOutputPioSmConfigChange = nullptr;

        // Wait for all the DMA to complete
        wait();

        activatePioSm(config);

        // Reconfigure the DMA channels to write to the new PIO SM
        io_wo_32* pioTxFifo = &(config.m_pio->txf[config.m_stateMachine]);
        uint32_t dreq = pio_get_dreq(config.m_pio, config.m_stateMachine, true);
        for (uint32_t i = 0; i < kNumBuffers; ++i)
        {
            s_dmaChannels[i].ConfigurePioSm(pioTxFifo, dreq);
        }
    }

    if (s_currentEntryIdx == 0)
    {
        return;
    }

    LOG_INFO(" Flush [%d, %d", s_currentBufferIdx, s_currentEntryIdx);
    // Our new buffer is ready to go, so let's configure its DMA channel with
    // the correct size.
    DmaChannel& dmaChannel = s_dmaChannels[s_currentBufferIdx];
    if(finalFlushForFrame)
    {
        // Tell the DMA to raise IRQ line 0 when the channel finishes a block
        dmaChannel.m_isFinal = true;
        s_previousFrameRunning = true;
    }

    

    critical_section_enter_blocking(&s_dmaCriticalSection);
        dmaChannel.Enable(s_currentEntryIdx);
        // Enable chaining on the previous DMA channel so that if it's still running,
        // it will chain to this one automatically
        s_dmaChannels[(s_currentBufferIdx + kNumBuffers - 1) % kNumBuffers].EnableChaining();

        if(++s_numDmaChannelsRunning == 1)
        {
            //dmaChannel.Start();
        }
    critical_section_exit(&s_dmaCriticalSection);

#if 1
    // If any of the other DMA channels are currently running, then this channel
    // will automatically be chained when they're done
    bool dmaIsRunning = false;
    for (uint32_t i = 1; i < kNumBuffers; ++i)
    {
        if (s_dmaChannels[(s_currentBufferIdx + i) % kNumBuffers].IsBusy())
        {
            dmaIsRunning = true;
            break;
        }
    }
    if (!dmaIsRunning)
    {
        // Uh-oh, none of the other channels are doing anything.
        // But maybe it just kicked off our new channel while we were thinking about it?
        if (!dmaChannel.IsBusy())
        {
            // Nope.
            // We're late filling in the buffer and the DMA is starved.
            // Best kick it off now.
            LOG_INFO(" -START- ");
            dmaChannel.Start();
        }
    }
#endif
    LOG_INFO("] ");
    // Move on to the next buffer
    if (++s_currentBufferIdx == kNumBuffers)
    {
        s_currentBufferIdx = 0;
    }
    s_currentEntryIdx = 0;
    // Wait for the DMA to complete on our new buffer.
    DmaChannel& nextDmaChannel = s_dmaChannels[s_currentBufferIdx];
    //nextDmaChannel.DisableChaining();
    LOG_INFO(" Wait [%d", s_currentBufferIdx);
    nextDmaChannel.Wait();
    LOG_INFO("]\n");
    // Prevent this channel from doing anything before we've finished filling in its buffer
    nextDmaChannel.Disable();
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
    LOG_INFO("Big wait: [");
    while(isDmaRunning())
    {
        tight_loop_contents();
    }
    LOG_INFO("]-[");
    while(s_previousFrameRunning)
    {
        tight_loop_contents();
    }
    LOG_INFO("]\n");
}

void DacOutput::dmaIrqHandler()
{
    DmaChannel& dmaChannel = s_dmaChannels[s_nextBufferIrq];
    if(dmaChannel.m_isFinal)
    {
        // Activate the idle SM
        activatePioSm(*s_idlePioSmConfig);
        s_previousFrameRunning = false;
        dmaChannel.m_isFinal = false;
    }
    dmaChannel.m_complete = true;

    s_nextBufferIrq = (s_nextBufferIrq + 1) % kNumBuffers;
    dma_channel_acknowledge_irq0(dmaChannel.m_channelIdx);

    critical_section_enter_blocking(&s_dmaCriticalSection);
        bool lookAtNextChannel = (--s_numDmaChannelsRunning > 0);
    critical_section_exit(&s_dmaCriticalSection);
    if(lookAtNextChannel)
    {
        // There are more buffers queued
        DmaChannel& nextDmaChannel = s_dmaChannels[s_nextBufferIrq];
        if(nextDmaChannel.m_pDacOutputPioSmConfigToSet != nullptr)
        {
            // We need to configure the PIO, and kick off
            // this DMA channel right here.
        }
    }
}

void DacOutput::activatePioSm(const DacOutputPioSmConfig& config)
{
    LOG_INFO("Switch SM: %d\n", config.m_stateMachine);
    if(s_currentPioConfig != nullptr)
    {
        // Wait for the current PIO SM to drain its FIFO
        while(!pio_sm_is_tx_fifo_empty(s_currentPioConfig->m_pio, s_currentPioConfig->m_stateMachine))
        {
            tight_loop_contents();
        }
        // Turn off the current PIO SM
        s_currentPioConfig->SetEnabled(false);
    }

    // Turn on the new PIO SM
    config.SetEnabled(true);
    s_currentPioConfig = &config;
    LOG_INFO("Done Switch SM: %d\n", config.m_stateMachine);
}

void DacOutput::configureAndStartDma(DmaChannel& dmaChannel)
{
    if(dmaChannel.m_pDacOutputPioSmConfigToSet != nullptr)
    {
        activatePioSm(*dmaChannel.m_pDacOutputPioSmConfigToSet);
    }
    dmaChannel.Start();
}

void DacOutput::SetCurrentPioSm(const DacOutputPioSmConfig& config)
{
    if(s_pendingDacOutputPioSmConfigChange != nullptr)
    {
        // We already have a change pending, so flush that first
        Flush();
    }
    if(s_currentPioConfig == &config)
    {
        // No change.  Don't do anything
        return;
    }
    Flush();
    s_pendingDacOutputPioSmConfigChange = &config;
}
