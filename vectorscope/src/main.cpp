#include "log.h"
#include "dacout.h"
#include "dacoutputsm.h"
#include "displaylist.h"
#include "fixedpoint.h"
#include "ledstatus.h"
#include "buttons.h"
#include "demo.h"
#include "serial.h"

#include "math.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#define DAC_OUTPUT_CORE 1

DisplayList* pDisplayList[2];
mutex_t displayListMutex[2];
static uint32_t outputDisplayListIdx = 0;
static uint32_t displayListIdx = 1;
static volatile bool dacOutputRunning = false;
static constexpr uint64_t kNumMicrosBetweenFrames = 16667; // 60FPS
//static constexpr uint64_t kNumMicrosBetweenFrames = 10000; // 100FPS
static constexpr uint32_t kGeneralPurposeButtonPin = 20;
static uint32_t coolDemoIdx = 0;
static bool singleStepMode = false;

static LogChannel FrameSynchronisation(false);
static LogChannel Events(true);

static inline uint32_t floatToOut(const float v)
{
    // int i = float2int(v * 2047.f);
    int i = (int)(v * 2047.f);
    i += 2048;
    return (uint32_t)((i > 4095) ? 4095 : ((i < 0) ? 0 : i));
}

#define kSinTableSize 800
float sinTable[kSinTableSize];
uint32_t intSinTable[kSinTableSize];

constexpr uint kMaxDemos = 16;
static Demo* s_demos[kMaxDemos] = {};
static uint s_numDemos = 0;

Demo::Demo(int order)
: m_order(order)
{
    uint demoInsertion = 0;
    for(; demoInsertion < s_numDemos; ++demoInsertion)
    {
        if(order < s_demos[demoInsertion]->m_order)
        {
            break;
        }
    }
    ++s_numDemos;
    for(uint i = s_numDemos - 1; i > demoInsertion; ++i)
    {
        s_demos[i] = s_demos[i-1];
    }
    s_demos[demoInsertion] = this;
}

void checkButton()
{
    if((Buttons::HoldTimeMs(Buttons::Id::Left) > 500) && (Buttons::HoldTimeMs(Buttons::Id::Right) > 500) && Buttons::IsJustPressed(Buttons::Id::Fire))
    {
        if(++coolDemoIdx == s_numDemos)
        {
            coolDemoIdx = 0;
        }
    }
    switch(Serial::GetLastCharIn())
    {
        default:
            break;

        case 'x':
            singleStepMode = !singleStepMode;
            LOG_INFO(Events, "Single step mode: %b\n", singleStepMode);
            Serial::ClearLastCharIn();
            break;
    }
}

void dacOutputLoop()
{
#if 1
    // Measure the duration of the previous frame, to determine if we need to sleep
    static uint64_t frameStart = 0;
    uint64_t frameEnd = time_us_64();
    uint64_t frameDuration = frameEnd - frameStart;
    if (frameDuration < kNumMicrosBetweenFrames)
    {
        sleep_us(kNumMicrosBetweenFrames - frameDuration);
        /*next*/ frameStart += kNumMicrosBetweenFrames;
    }
    else
    {
        /*next*/ frameStart = frameEnd;
    }
#endif    
    checkButton();

    // Lock the mutex for the next frame's DisplayList
    uint32_t nextDisplayListIdx = 1 - outputDisplayListIdx;
    LOG_INFO(FrameSynchronisation, "DO W: %d\n", nextDisplayListIdx);
    mutex_enter_blocking(displayListMutex + nextDisplayListIdx);
    // Only then do we release this frame's DisplayList
    mutex_exit(displayListMutex + outputDisplayListIdx);
    outputDisplayListIdx = nextDisplayListIdx;

    // Write the entire display list out to the DACsFIFO buffers
    LOG_INFO(FrameSynchronisation, "DO S %d\n", outputDisplayListIdx);
    LedStatus::SetStep(6, LedStatus::Brightness((float)DacOutput::GetFrameDurationUs() * (1.f / kNumMicrosBetweenFrames)), 400);
    uint64_t dacOutStart = time_us_64();
    pDisplayList[outputDisplayListIdx]->OutputToDACs();
    LedStatus::SetStep(4, LedStatus::Brightness((float)(time_us_64() - dacOutStart) * (1.f / kNumMicrosBetweenFrames)), 400);
    LOG_INFO(FrameSynchronisation, "DO E %d\n", outputDisplayListIdx);
}

void displayListUpdateLoop()
{
    // Lock the next DisplayList for us to populate with a cool demo frame
    LOG_INFO(FrameSynchronisation, "Fill W: %d\n", displayListIdx);
    mutex_enter_blocking(displayListMutex + displayListIdx);
    LOG_INFO(FrameSynchronisation, "Fill S: %d\n", displayListIdx);

    if(singleStepMode)
    {
        while(Serial::GetLastCharIn() != 's')
        {
            //LOG_INFO(Events, "Erm: '%c'\n", Serial::GetLastCharIn());
            checkButton();
        }
        LOG_INFO(Events, "Step\n");//: '%c'\n", Serial::GetLastCharIn());
        Serial::ClearLastCharIn();
    }

    uint64_t frameStart = time_us_64();

    // Fill in the DisplayList
    DisplayList& displayList = *(pDisplayList[displayListIdx]);
    displayList.Clear();

    Buttons::Update();

    s_demos[coolDemoIdx]->UpdateAndRender(displayList, 0.0167f);

    // Unlock it so that the display output knows it's ready
    LOG_INFO(FrameSynchronisation, "Fill E: %d\n", displayListIdx);
    mutex_exit(displayListMutex + displayListIdx);
    displayListIdx = 1 - displayListIdx;

    uint64_t frameEnd = time_us_64();
    LedStatus::SetStep(2, LedStatus::Brightness((float)(frameEnd - frameStart) * (1.f / kNumMicrosBetweenFrames)), 400);
}

void dacOutputTask()
{
    mutex_enter_blocking(displayListMutex + outputDisplayListIdx);
    dacOutputRunning = true;
    while (true)
    {
        dacOutputLoop();
    }
}

void displayListUpdateTask()
{
    // Wait until the dac output loop is running before we start
    // filling in display lists.
    while (!dacOutputRunning)
    {
        sleep_us(1000);
    }
    while (true)
    {
        displayListUpdateLoop();
    }
}

int main()
{
    Serial::Init();
    Log::Init();
    LedStatus::Init();
    LedStatus::SetStep(0, LedStatus::Brightness(1.f), 400);
    LedStatus::SetStep(1, LedStatus::Brightness(0.f), 100);
    LedStatus::SetStep(3, LedStatus::Brightness(0.f), 100);
    LedStatus::SetStep(5, LedStatus::Brightness(0.f), 100);
    LedStatus::SetStep(7, LedStatus::Brightness(0.f), 800);

    Buttons::Init();

    TestFixedPoint();

    DacOutputSm::Init();
    DacOutput::Init(DacOutputSm::Idle());
    mutex_init(displayListMutex + 0);
    mutex_init(displayListMutex + 1);

    pDisplayList[0] = new DisplayList(2048, 1024);
    pDisplayList[1] = new DisplayList(2048, 1024);

    for(uint i = 0; i < s_numDemos; ++i)
    {
        s_demos[i]->Init();
    }

#if 1
#    if DAC_OUTPUT_CORE == 1
    multicore_launch_core1(dacOutputTask);
    displayListUpdateTask();
#    else
    multicore_launch_core1(displayListUpdateTask);
    dacOutputTask();
#    endif
#else
    // Initialise the sin table
    for (uint32_t i = 0; i < kSinTableSize; ++i)
    {
        float phase = (float)(i % 1024) * (kPi * 2.f / (float)kSinTableSize);
        float s = sinf(phase);
        sinTable[i] = s;
        intSinTable[i] = floatToOut(s);
    }
    uint32_t* pBuffer;
    // int squareAmp = 2047;
    DacOutput::SetCurrentPioSm(DacOutputSm::Vector());
    while (true)
    {
#    if 0
        const uint32_t kSinLength = kSinTableSize * 12;
        pBuffer = DacOutput::AllocateBufferSpace(kSinLength);
        uint32_t phase = 0;
        for (uint32_t i = 0; i < kSinLength; ++i)
        {
            if (++phase == kSinTableSize)
            {
                phase = 0;
            }

            pBuffer[i] = intSinTable[phase] | ((4095 - intSinTable[phase]) << 12);
        }
#    endif
#    if 1
        const uint32_t kSawLength = 16384;
        pBuffer = DacOutput::AllocateBufferSpace(kSawLength);
        int32_t saw = 0;
        for(uint32_t i = 0; i < kSawLength; ++i)
        {
            if(++saw == 4096)
            {
                saw = 0;
            }

            pBuffer[i] = saw;
        }
#    endif
#    if 0
        const uint32_t kSquareLength = 16384;
        pBuffer = DacOutput::AllocateBufferSpace(kSquareLength);
#        if 0
        squareAmp += 600;
        if(squareAmp > 2047)
        {
            squareAmp = 600;
        }
#        endif
        for(uint32_t i = 0; i < kSquareLength; ++i)
        {
            pBuffer[i] = (i & 0x100) ? (2048 + squareAmp) : (2048 - squareAmp);
        }
#    endif
    }
#endif
}
