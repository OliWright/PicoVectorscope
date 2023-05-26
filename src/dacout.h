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

// This is an internal header for picovectorscope.
// It's only used by the DisplayList class.

#pragma once
#include "dacoutpioconfig.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include <cstdint>

class DacOutput
{
public:
    static void Init(const DacOutputPioSmConfig& idleSm);

    // Try to allocate some entries in the FIFO.
    // This might not be able to allocate everything you ask for.
    static inline uint32_t* AllocateBufferSpace(uint32_t numEntriesRequested, uint32_t& outNumEntriesAllocated)
    {
        uint32_t* pBufferSpace;
        // Do we have room in the current buffer?
        const uint32_t numEntriesAvailable = (kNumEntriesPerBuffer - s_currentEntryIdx);
        if (numEntriesAvailable >= numEntriesRequested)
        {
            // Yes we do.  Happy days!
            outNumEntriesAllocated = numEntriesRequested;
        }
        else if(numEntriesAvailable > 64)
        {
            // They can make do with what there is
            outNumEntriesAllocated = numEntriesAvailable;
        }
        else
        {
            // Let's move to the next buffer
            Flush();
            outNumEntriesAllocated = (numEntriesRequested > kNumEntriesPerBuffer) ? kNumEntriesPerBuffer : numEntriesRequested;
        }
        pBufferSpace = s_buffers[s_currentBufferIdx] + s_currentEntryIdx;
        s_currentEntryIdx += outNumEntriesAllocated;
        checkDmaStatus();
        return pBufferSpace;
    }

    // Allocate some entries in the FIFO. Failure is not an option with this one
    // so it is the caller's responsibility to make sure they don't ask for more
    // entries than we can handle (kNumEntriesPerBuffer).
    static inline uint32_t* AllocateBufferSpace(uint32_t numEntries)
    {
        // Do we have room in the current buffer?
        if (numEntries > (kNumEntriesPerBuffer - s_currentEntryIdx))
        {
            // Let's move to the next buffer
            Flush();
        }
        uint32_t* pBufferSpace = s_buffers[s_currentBufferIdx] + s_currentEntryIdx;
        s_currentEntryIdx += numEntries;
        return pBufferSpace;
    }

    // If not all of the entries from the most recent Allocate call were used
    // then you can give back the unused ones here.
    static inline void GiveBackUnusedEntries(uint32_t numEntries)
    {
        s_currentEntryIdx -= numEntries;
    }

    // Any outstanding data in the FIFOs will be made available for pushing
    // out to the DACs.
    // This should be called after all the data has been put into the FIFOs
    // for an entire frame, to make sure that the last bits of data in
    // the FIFOs don't wait until their buffers are full when the next frame
    // comes along.
    static void Flush(bool finalFlushForFrame = false);

    // Change the PIO SM program that we're using.
    // Subsequent data put into the FIFO will use this program.
    static void SetCurrentPioSm(const DacOutputPioSmConfig& config);

    // For stats.  How much time was spent with active DMA.
    static uint64_t GetFrameDurationUs() {return s_frameDurationUs;}

    static void Poll();

private:
    constexpr static uint32_t kNumBuffers = 3;
    constexpr static uint32_t kNumEntriesPerBuffer = 4096;

    // We have one DMA channel per buffer.
    // *** Chaining is currently disabled and work-in-progress ***
    // *** Instead, DMA channels are manually chained within   ***
    // *** the interrupt handler                               ***
    struct DmaChannel
    {
        int m_channelIdx;
        int m_chainSpinChannelIdx;
        int m_chainToChannelIdx;
        uint32_t* m_pBufferBase;
        io_wo_32* m_pWriteAddress;
        const DacOutputPioSmConfig* m_pDacOutputPioSmConfigToSet;
        dma_channel_config m_config;
        // This config will be poked to the DacOutput::s_dmaChainSpinChannelIdx
        dma_channel_config m_liveChainSpinConfig;
        // If the value is this config, then we'll keep spinning
        dma_channel_config m_chainSpinSpinConfig;
        // If the value is this config, then we'll chain to the next buffer
        dma_channel_config m_chainSpinChainConfig;
        volatile bool m_complete;
        bool m_isFinal;

        void Init(uint32_t* pBufferBase, int chainToDmaChannelIdx);
        void Enable(uint32_t numEntries)
        {
            dma_channel_set_read_addr(m_channelIdx, m_pBufferBase, false);
            dma_channel_set_write_addr(m_channelIdx, m_pWriteAddress, false);
            DisableChaining();
            dma_channel_set_trans_count(m_channelIdx, numEntries, false);
        }
        void Disable()
        {
            dma_channel_set_trans_count(m_channelIdx, 0, false);
            dma_channel_set_read_addr(m_channelIdx, m_pBufferBase, false);
            m_complete = false;
            m_pDacOutputPioSmConfigToSet = nullptr;
            DisableChaining();
        }
        bool IsBusy() const { return dma_channel_is_busy(m_channelIdx); }
        void Wait()
        {
            dma_channel_wait_for_finish_blocking(m_channelIdx);
        }
        void EnableChaining()
        {
            m_liveChainSpinConfig = m_chainSpinChainConfig;
        }
        void DisableChaining()
        {
            m_liveChainSpinConfig = m_chainSpinSpinConfig;
        }
        bool IsChainingEnabled() const
        {
            return m_liveChainSpinConfig.ctrl == m_chainSpinChainConfig.ctrl;
        }
        void ConfigurePioSm(io_wo_32* pioTxFifo, uint32_t dreq)
        {
            m_pWriteAddress = pioTxFifo;
            channel_config_set_dreq(&m_config, dreq);
            dma_channel_set_config(m_channelIdx, &m_config, false);
        }
    };
    friend struct DmaChannel;

public:
    static void wait();
private:
    static bool isDmaRunning();
    static void dmaIrqHandler();
    static void setActivePioSm(const DacOutputPioSmConfig& config);
    static void configureAndStartDma(const DacOutputPioSmConfig* pioConfig, DmaChannel& previousDmaChannel);
    static void checkDmaStatus();

private:
    static const DacOutputPioSmConfig* s_currentPioConfig;
    static const DacOutputPioSmConfig* s_previousPioConfig;
    static int          s_dmaChainSpinChannelIdx;
    static volatile uint32_t* s_dmaChainSpinCtrl;
    static dma_channel_config s_dmaChainSpinConfigTemplate;
    static DmaChannel   s_dmaChannels[kNumBuffers];
    static uint32_t     s_buffers[kNumBuffers][kNumEntriesPerBuffer];
    static uint32_t     s_currentBufferIdx;
    static uint32_t     s_currentEntryIdx;
    static const DacOutputPioSmConfig* s_idlePioSmConfig;
    static uint32_t     s_nextBufferIrq;
    static volatile uint32_t s_numDmaChannelsQueued;
    static uint64_t     s_frameStartUs;
    static uint64_t     s_frameDurationUs;
};
