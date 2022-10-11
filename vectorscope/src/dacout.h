#ifndef DACOUT_H
#define DACOUT_H

#include <cstdint>

#include "dacoutpioconfig.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

class DacOutput
{
public:
    static void Init(const DacOutputPioSmConfig& idleSm);

    static inline uint32_t* AllocateBufferSpace(uint32_t numEntries)
    {
        // Do we have room in the current buffer?
        if ((s_currentEntryIdx + numEntries) > kNumEntriesPerBuffer)
        {
            // No.
            Flush();
        }
        // We should do now.
        uint32_t* pBufferSpace = s_buffers[s_currentBufferIdx] + s_currentEntryIdx;
        s_currentEntryIdx += numEntries;
        return pBufferSpace;
    }
    static void Flush(bool finalFlushForFrame = false);
    static void SetCurrentPioSm(const DacOutputPioSmConfig& config);
    static uint64_t GetFrameDurationUs() {return s_frameDurationUs;}

private:
    constexpr static uint32_t kNumBuffers = 3;
    constexpr static uint32_t kNumEntriesPerBuffer = 16384;

    struct DmaChannel
    {
        int m_channelIdx;
        uint32_t* m_pBufferBase;
        io_wo_32* m_pWriteAddress;
        const DacOutputPioSmConfig* m_pDacOutputPioSmConfigToSet;
        dma_channel_config m_configWithChain;
        dma_channel_config m_configWithoutChain;
        bool m_complete;
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
        void Start()
        {
            dma_channel_start(m_channelIdx);
        }
        void EnableChaining()
        {
            dma_channel_set_config(m_channelIdx, &m_configWithChain, false);
            //LOG_INFO("Chain on %d\n", m_channelIdx);
        }
        void DisableChaining()
        {
            dma_channel_set_config(m_channelIdx, &m_configWithoutChain, false);
            //LOG_INFO("Chain off %d\n", m_channelIdx);
        }
        void ConfigurePioSm(io_wo_32* pioTxFifo, uint32_t dreq)
        {
            m_pWriteAddress = pioTxFifo;
            channel_config_set_dreq(&m_configWithChain, dreq);
            channel_config_set_dreq(&m_configWithoutChain, dreq);
        }
    };

public:
    static void wait();
private:
    static bool isDmaRunning();
    static void dmaIrqHandler();
    static void setActivePioSm(const DacOutputPioSmConfig& config);
    static void configureAndStartDma(const DacOutputPioSmConfig& pioConfig, DmaChannel& dmaChannel);

private:
    static const DacOutputPioSmConfig* s_currentPioConfig;
    static const DacOutputPioSmConfig* s_previousPioConfig;
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

#endif
