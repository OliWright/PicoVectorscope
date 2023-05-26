// Static class managing the output buffers, DMA and PIO SM programs
// dealing with squirting data out to the DACs. 
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

#include "log.h"
#include "dacout.h"
#include "pico/time.h"
#include "pico/sync.h"

const DacOutputPioSmConfig* DacOutput::s_currentPioConfig = nullptr;
const DacOutputPioSmConfig* DacOutput::s_previousPioConfig = nullptr;
int                DacOutput::s_dmaChainSpinChannelIdx;
volatile uint32_t* DacOutput::s_dmaChainSpinCtrl;
dma_channel_config DacOutput::s_dmaChainSpinConfigTemplate;
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
uint32_t s_chainSpinDmaRead = 0;
uint32_t s_chainSpinDmaWrite = 0;

static bool s_dmaIsRunning = false;
static uint32_t s_flushKickBufferIdx = 0;
static uint32_t s_numBuffersToQueueBeforeKick = 1;//DacOutput::kNumBuffers;

//static critical_section_t s_dmaCriticalSection = {};
static mutex_t s_dmaMutex;

static LogChannel DacOutputSynchronisation(false);

void DacOutput::Init(const DacOutputPioSmConfig& idleSm)
{
    s_idlePioSmConfig = &idleSm;
    //critical_section_init(&s_dmaCriticalSection);
    mutex_init(&s_dmaMutex);

    s_dmaChainSpinChannelIdx = dma_claim_unused_channel(true);
    s_dmaChainSpinCtrl = &(dma_hw->ch[s_dmaChainSpinChannelIdx].ctrl_trig);

    s_dmaChainSpinConfigTemplate = dma_channel_get_default_config(s_dmaChainSpinChannelIdx);

    // Transfer 32 bits each time
    channel_config_set_transfer_data_size(&s_dmaChainSpinConfigTemplate, DMA_SIZE_32);
    // Increment read address
    channel_config_set_read_increment(&s_dmaChainSpinConfigTemplate, false);
    // Write to the same address (the PIO SM TX FIFO)
    channel_config_set_write_increment(&s_dmaChainSpinConfigTemplate, false);
    // Disable address wrapping
    channel_config_set_ring(&s_dmaChainSpinConfigTemplate, false, 0);
    channel_config_set_high_priority(&s_dmaChainSpinConfigTemplate, false);
    // Chain to the 'chainSpin' channel when complete
    //channel_config_set_chain_to(&s_dmaChainSpinConfigTemplate, 0);
    dma_channel_set_irq0_enabled(s_dmaChainSpinChannelIdx, false);
    // Setup the channel
    dma_channel_configure(s_dmaChainSpinChannelIdx, &s_dmaChainSpinConfigTemplate,
                          &s_chainSpinDmaWrite, // Write to here
                          &s_chainSpinDmaRead, // Read from here
                          1,
                          false // don't start yet
    );



    // Claim and initialise a DMA channels for each buffer
    for (uint32_t i = 0; i < kNumBuffers; ++i)
    {
        s_dmaChannels[i].m_channelIdx = dma_claim_unused_channel(true);
        s_dmaChannels[i].m_chainSpinChannelIdx = dma_claim_unused_channel(true);
    }
    for (uint32_t i = 0; i < kNumBuffers; ++i)
    {
        s_dmaChannels[i].Init(s_buffers[i], s_dmaChannels[(i+1)%kNumBuffers].m_channelIdx);
    }
    // Start the final DMA channel's spin-chain running
    s_chainSpinDmaRead = kNumBuffers-1;
    dma_channel_start(s_dmaChannels[kNumBuffers-1].m_chainSpinChannelIdx);
    s_numBuffersToQueueBeforeKick = kNumBuffers - 1;

    // Configure the processor to run dmaIrqHandler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, dmaIrqHandler);
    irq_set_enabled(DMA_IRQ_0, true);

    s_dmaChannels[0].m_complete = false;
}

void DacOutput::DmaChannel::Init(uint32_t* pBufferBase, int chainToDmaChannelIdx)
{
    m_chainToChannelIdx = chainToDmaChannelIdx;
    m_pBufferBase = pBufferBase;
    m_config = dma_channel_get_default_config(m_channelIdx);
    m_complete = true;
    m_isFinal = false;

    //
    // Configure the DMA channel
    //

    // Transfer 32 bits each time
    channel_config_set_transfer_data_size(&m_config, DMA_SIZE_32);
    // Increment read address
    channel_config_set_read_increment(&m_config, true);
    // Write to the same address (the PIO SM TX FIFO)
    channel_config_set_write_increment(&m_config, false);
    // Disable address wrapping
    channel_config_set_ring(&m_config, false, 0);
    // Chain to the 'chainSpin' channel when complete
    channel_config_set_chain_to(&m_config, m_chainSpinChannelIdx);
    dma_channel_set_irq0_enabled(m_channelIdx, true);

    // Setup the channel
    dma_channel_configure(m_channelIdx, &m_config,
                          nullptr, // Write to PIO TX FIFO
                          pBufferBase, // Read values from appropriate outputBuffer
                          0,
                          false // don't start yet
    );

    //
    // Configure the ChainSpin DMA channel
    // Setup the channel to copy whatever is in m_liveChainSpinConfig to the control
    // register of the global spin-chain channel, and then chain to it.
    // 
    dma_channel_config chainConfig = dma_channel_get_default_config(m_chainSpinChannelIdx);

    // Transfer 32 bits each time
    channel_config_set_transfer_data_size(&chainConfig, DMA_SIZE_32);
    // Increment read address
    channel_config_set_read_increment(&chainConfig, false);
    // Write to the same address (the PIO SM TX FIFO)
    channel_config_set_write_increment(&chainConfig, false);
    // Disable address wrapping
    channel_config_set_ring(&chainConfig, false, 0);
    // Chain to the 'chainSpin' channel when complete
    channel_config_set_chain_to(&chainConfig, DacOutput::s_dmaChainSpinChannelIdx);
    dma_channel_set_irq0_enabled(m_chainSpinChannelIdx, false);

    dma_channel_configure(m_chainSpinChannelIdx, &chainConfig,
                          DacOutput::s_dmaChainSpinCtrl, // Copy m_liveChainSpinConfig to the ctrl 
                          &m_liveChainSpinConfig,        // register of the spin channel
                          1,
                          false // don't start yet
    );

    //
    // Configure the 'Spin' and 'Chain' configs to control whether the chainspin
    // DMA will spin or chain to the next buffer
    //
    m_chainSpinSpinConfig = DacOutput::s_dmaChainSpinConfigTemplate;
    m_chainSpinChainConfig = DacOutput::s_dmaChainSpinConfigTemplate;
    // Chain to the 'chainSpin' channel when complete
    channel_config_set_chain_to(&m_chainSpinSpinConfig, m_chainSpinChannelIdx);
    // Chain to the next buffer channel when complete
    channel_config_set_chain_to(&m_chainSpinChainConfig, chainToDmaChannelIdx);
    m_liveChainSpinConfig = m_chainSpinSpinConfig;
}

void DacOutput::Flush(bool finalFlushForFrame)
{
    if (s_currentEntryIdx == 0)
    {
        // Nothing to flush
        return;
    }

    //LOG_INFO(DacOutputSynchronisation, "Flush [%d, %d]\n", s_currentBufferIdx, s_currentEntryIdx);
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

    //critical_section_enter_blocking(&s_dmaCriticalSection);
    mutex_enter_blocking(&s_dmaMutex);
        dmaChannel.Enable(s_currentEntryIdx);
        ++s_numDmaChannelsQueued;

        if(s_currentPioConfig == s_previousPioConfig)
        {
            // Enable chaining on the previous DMA channel so that if it's still running,
            // it will chain to this one automatically
            const uint32_t previousBufferIdx = (s_currentBufferIdx + kNumBuffers - 1) % kNumBuffers;
            LOG_INFO(DacOutputSynchronisation, "Flush Chained %d->%d\n", previousBufferIdx, s_currentBufferIdx);
            s_dmaChannels[previousBufferIdx].EnableChaining();
            s_chainSpinDmaRead = s_currentBufferIdx;
        }
        else
        {
            // We can't chain it, because it requires a change in PIO SM program
            LOG_INFO(DacOutputSynchronisation, "Flush Break %d\n", s_currentBufferIdx);
            dmaChannel.m_pDacOutputPioSmConfigToSet = s_currentPioConfig;
            s_previousPioConfig = s_currentPioConfig;
        }


        if(!s_dmaIsRunning && (finalFlushForFrame || (s_numDmaChannelsQueued >= s_numBuffersToQueueBeforeKick)))
        {
            // Wait for the first kick until all the buffers are filled, to give us our best chance
            // of not draining all the buffers.
            const uint32_t kickBufferIdx = (s_currentBufferIdx + kNumBuffers - s_numDmaChannelsQueued + 1) % kNumBuffers;
            LOG_INFO(DacOutputSynchronisation, "Flush Kick %d\n", kickBufferIdx);
            configureAndStartDma(s_dmaChannels[kickBufferIdx].m_pDacOutputPioSmConfigToSet, s_dmaChannels[(kickBufferIdx + kNumBuffers - 1) % kNumBuffers]);
            s_chainSpinDmaRead = kickBufferIdx;
            s_dmaIsRunning = true;
            s_numBuffersToQueueBeforeKick = 1;
        }
    mutex_exit(&s_dmaMutex);
    //critical_section_exit(&s_dmaCriticalSection);

    // Move on to the next buffer
    if (++s_currentBufferIdx == kNumBuffers)
    {
        s_currentBufferIdx = 0;
    }
    s_currentEntryIdx = 0;
    // Wait for the DMA to complete on our new buffer.
    DmaChannel& nextDmaChannel = s_dmaChannels[s_currentBufferIdx];
    //LOG_INFO(DacOutputSynchronisation, "Wait [%d]\n", s_currentBufferIdx);
    while(!nextDmaChannel.m_complete)
    {
        checkDmaStatus();
        //tight_loop_contents();
    }
    //LOG_INFO(DacOutputSynchronisation, "Done [%d]\n", s_currentBufferIdx);

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
        s_numBuffersToQueueBeforeKick = 1;//kNumBuffers;
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

//static uint32_t s_int0Bits = 0;

void DacOutput::dmaIrqHandler()
{
    uint32_t int0Bits = dma_hw->ints0;
    hw_set_bits(&dma_hw->ints0, int0Bits);
#if 0
    mutex_enter_blocking(&s_dmaMutex);
    s_int0Bits |= int0Bits;
    mutex_exit(&s_dmaMutex);
    //dma_hw->ints0 = int0Bits;
    //DmaChannel& dmaChannel = s_dmaChannels[s_nextBufferIrq];
    //dma_channel_acknowledge_irq0(dmaChannel.m_channelIdx);
    //dma_hw->ints0 = (1u << dmaChannel.m_channelIdx);
#endif    
}

void DacOutput::checkDmaStatus()
{
    if(!s_dmaIsRunning)
    {
        return;
    }
    DmaChannel& dmaChannel = s_dmaChannels[s_nextBufferIrq];
    //dma_channel_acknowledge_irq0(dmaChannel.m_channelIdx);
    //dma_hw->ints0 = (1u << dmaChannel.m_channelIdx);
    //uint32_t channelBit = (1u << dmaChannel.m_channelIdx);
    if(dma_hw->ch[dmaChannel.m_channelIdx].transfer_count > 0)
    //if(dma_channel_is_busy(dmaChannel.m_channelIdx))
    //if((s_int0Bits & channelBit) == 0)
    {
        //LOG_INFO(DacOutputSynchronisation, "DMA busy\n", s_nextBufferIrq);
        return;
    }
    sleep_us(100);

    //mutex_enter_blocking(&s_dmaMutex);
    //s_int0Bits &= ~channelBit;
    //mutex_exit(&s_dmaMutex);
    //dma_channel_acknowledge_irq0(dmaChannel.m_channelIdx);
    //dma_hw->ints0 = channelBit;

    //critical_section_enter_blocking(&s_dmaCriticalSection);
    mutex_enter_blocking(&s_dmaMutex);
    //LOG_INFO(DacOutputSynchronisation, "IRQ %d %d\n", s_nextBufferIrq, dma_channel_get_irq0_status(dmaChannel.m_channelIdx) ? 1 : 0);
    bool makeIdle = false;
    assert(!dmaChannel.m_complete);
    if(dmaChannel.m_isFinal)
    {
        dmaChannel.m_isFinal = false;
        s_frameDurationUs = time_us_64() - s_frameStartUs;
        s_frameStartUs = 0;
        makeIdle = true;
    }

    const uint32_t completedBufferIdx = s_nextBufferIrq;
    s_nextBufferIrq = (s_nextBufferIrq + 1) % kNumBuffers;

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
            assert(!dmaChannel.IsChainingEnabled());
            configureAndStartDma(nextDmaChannel.m_pDacOutputPioSmConfigToSet, dmaChannel);
            makeIdle = false;
        }
        else
        {
            LOG_INFO(DacOutputSynchronisation, "IRQ Chained %d->%d\n", completedBufferIdx, s_nextBufferIrq);
            assert(dmaChannel.IsChainingEnabled());
        }
    }
    else
    {
        // DMA has been drained, so will be left spinning
        LOG_INFO(DacOutputSynchronisation, "IRQ Drained %d\n", completedBufferIdx);
        assert(!dmaChannel.IsChainingEnabled());
        s_dmaIsRunning = false;
        s_flushKickBufferIdx = s_nextBufferIrq;
    }

    if(makeIdle)
    {
        // Activate the idle SM
        LOG_INFO(DacOutputSynchronisation, "Setting idle\n");
        setActivePioSm(*s_idlePioSmConfig);
        s_numBuffersToQueueBeforeKick = kNumBuffers - 1;
    }
    
    dmaChannel.m_complete = true;
    //LOG_INFO(DacOutputSynchronisation, "IRQ End\n");
    mutex_exit(&s_dmaMutex);
    //critical_section_exit(&s_dmaCriticalSection);
}

void DacOutput::Poll()
{
    checkDmaStatus();
}


void DacOutput::setActivePioSm(const DacOutputPioSmConfig& config)
{
    if(s_activePioSmConfig != &config)
    {
        LOG_INFO(DacOutputSynchronisation, "Switch SM: %d\n", config.m_id);
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
        //LOG_INFO(DacOutputSynchronisation, "Done Switch SM: %d\n", config.m_id);
    }
}

void DacOutput::configureAndStartDma(const DacOutputPioSmConfig* pioConfig, DmaChannel& previousDmaChannel)
{
    if(s_frameStartUs == 0)
    {
        s_frameStartUs = time_us_64();
    }
    if(pioConfig != nullptr)
    {
        setActivePioSm(*pioConfig);
    }
    previousDmaChannel.EnableChaining();
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
