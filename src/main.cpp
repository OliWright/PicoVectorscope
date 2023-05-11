// The main entry point for picovectorscope
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
// COPYING.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.

#include "buttons.h"
#include "dacout.h"
#include "dacoutputsm.h"
#include "demo.h"
#include "displaylist.h"
#include "fixedpoint.h"
#include "ledstatus.h"
#include "log.h"
#include "math.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "serial.h"

// Which core (0 or 1) to run the DAC output on
#define DAC_OUTPUT_CORE 1

static DisplayList*  s_pDisplayList[2];
static mutex_t       s_displayListMutex[2];
static uint32_t      s_outputDisplayListIdx = 0; //< Which are we currently outputting to DACs
static uint32_t      s_displayListIdx       = 1; //< Which are we currently filling
static volatile bool s_dacOutputRunning     = false;
static uint32_t      s_demoIdx              = 0;
static bool          s_singleStepMode       = false;

static uint64_t s_numMicrosBetweenFrames = 1000000 / 60; // FPS
static float    s_dt                     = (float)s_numMicrosBetweenFrames / 1000000.f;

static LogChannel FrameSynchronisation(false);
static LogChannel Events(false);
static LogChannel ButtonFeedback(false);

constexpr uint kMaxDemos          = 16;
static Demo*   s_demos[kMaxDemos] = {};
static uint    s_numDemos         = 0;

static void initRefreshRate(const Demo& demo)
{
    s_numMicrosBetweenFrames = 1000000 / demo.GetTargetRefreshRate();
    s_dt                     = (float)s_numMicrosBetweenFrames / 1000000.f;
}

// The Demo constructor self-registers the Demo in our list of Demos
Demo::Demo(int order, int m_targetRefreshRate)
    : m_order(order), m_targetRefreshRate(m_targetRefreshRate)
{
    // A simple inserstion sort keeps the Demos in their priority order
    int demoInsertion = 0;
    for (; demoInsertion < (int) s_numDemos; ++demoInsertion)
    {
        if (order < s_demos[demoInsertion]->m_order)
        {
            break;
        }
    }
    ++s_numDemos;
    for (int i = s_numDemos - 1; i > demoInsertion; --i)
    {
        s_demos[i] = s_demos[i - 1];
    }
    s_demos[demoInsertion] = this;
    if (demoInsertion == 0)
    {
        initRefreshRate(*this);
    }
}

void checkButton()
{
    if (Buttons::IsJustPressed(Buttons::Id::Left))
    {
        LOG_INFO(ButtonFeedback, "Left\n");
    }
    if (Buttons::IsJustPressed(Buttons::Id::Right))
    {
        LOG_INFO(ButtonFeedback, "Right\n");
    }
    if (Buttons::IsJustPressed(Buttons::Id::Thrust))
    {
        LOG_INFO(ButtonFeedback, "Thrust\n");
    }
    if (Buttons::IsJustPressed(Buttons::Id::Fire))
    {
        LOG_INFO(ButtonFeedback, "Fire\n");
    }
    if ((Buttons::HoldTimeMs(Buttons::Id::Left) > 500)
        && (Buttons::HoldTimeMs(Buttons::Id::Right) > 500)
        && Buttons::IsJustPressed(Buttons::Id::Fire))
    {
        Buttons::ClearJustPressed(Buttons::Id::Fire);
        s_demos[s_demoIdx]->End();
        if (++s_demoIdx == s_numDemos)
        {
            s_demoIdx = 0;
        }
        LOG_INFO(ButtonFeedback, "Changing to demo %d\n", s_demoIdx);
        initRefreshRate(*s_demos[s_demoIdx]);
        s_demos[s_demoIdx]->Start();
    }
    switch (Serial::GetLastCharIn())
    {
    default:
        break;

    case 'x':
        s_singleStepMode = !s_singleStepMode;
        LOG_INFO(Events, "Single step mode: %b\n", s_singleStepMode);
        Serial::ClearLastCharIn();
        break;
    }
}

void dacOutputLoop()
{
#if 1
    // Measure the duration of the previous frame, to determine if we need to sleep
    static uint64_t frameStart    = 0;
    uint64_t        frameEnd      = time_us_64();
    uint64_t        frameDuration = frameEnd - frameStart;
    if (frameDuration < s_numMicrosBetweenFrames)
    {
        sleep_us(s_numMicrosBetweenFrames - frameDuration);
        /*next*/ frameStart += s_numMicrosBetweenFrames;
    }
    else
    {
        /*next*/ frameStart = frameEnd;
    }
#endif
    checkButton();

    // Lock the mutex for the next frame's DisplayList
    uint32_t nextDisplayListIdx = 1 - s_outputDisplayListIdx;
    LOG_INFO(FrameSynchronisation, "DO W: %d\n", nextDisplayListIdx);
    mutex_enter_blocking(s_displayListMutex + nextDisplayListIdx);
    // Only then do we release this frame's DisplayList
    mutex_exit(s_displayListMutex + s_outputDisplayListIdx);
    s_outputDisplayListIdx = nextDisplayListIdx;

    // Write the entire display list out to the DACsFIFO buffers
    LOG_INFO(FrameSynchronisation, "DO S %d\n", s_outputDisplayListIdx);
    LedStatus::SetStep(6,
                       LedStatus::Brightness((float)DacOutput::GetFrameDurationUs()
                                             * (1.f / s_numMicrosBetweenFrames)),
                       400);
    uint64_t dacOutStart = time_us_64();
    s_pDisplayList[s_outputDisplayListIdx]->OutputToDACs();
    LedStatus::SetStep(
        4,
        LedStatus::Brightness((float)(time_us_64() - dacOutStart) * (1.f / s_numMicrosBetweenFrames)),
        400);
    LOG_INFO(FrameSynchronisation, "DO E %d\n", s_outputDisplayListIdx);
}

void displayListUpdateLoop()
{
    // Lock the next DisplayList for us to populate with a cool demo frame
    LOG_INFO(FrameSynchronisation, "Fill W: %d\n", s_displayListIdx);
    mutex_enter_blocking(s_displayListMutex + s_displayListIdx);
    LOG_INFO(FrameSynchronisation, "Fill S: %d\n", s_displayListIdx);

    if (s_singleStepMode)
    {
        while (Serial::GetLastCharIn() != 's')
        {
            // LOG_INFO(Events, "Erm: '%c'\n", Serial::GetLastCharIn());
            checkButton();
        }
        LOG_INFO(Events, "Step\n"); //: '%c'\n", Serial::GetLastCharIn());
        Serial::ClearLastCharIn();
    }

    uint64_t frameStart = time_us_64();

    // Fill in the DisplayList
    DisplayList& displayList = *(s_pDisplayList[s_displayListIdx]);
    displayList.Clear();

    Buttons::Update();

    s_demos[s_demoIdx]->UpdateAndRender(displayList, s_dt);

    // Unlock it so that the display output knows it's ready
    LOG_INFO(FrameSynchronisation, "Fill E: %d\n", s_displayListIdx);
    mutex_exit(s_displayListMutex + s_displayListIdx);
    s_displayListIdx = 1 - s_displayListIdx;

    uint64_t frameEnd = time_us_64();
    LedStatus::SetStep(
        2, LedStatus::Brightness((float)(frameEnd - frameStart) * (1.f / s_numMicrosBetweenFrames)),
        400);
}

void dacOutputTask()
{
    mutex_enter_blocking(s_displayListMutex + s_outputDisplayListIdx);
    s_dacOutputRunning = true;
    while (true)
    {
        dacOutputLoop();
    }
}

void displayListUpdateTask()
{
    // Wait until the dac output loop is running before we start
    // filling in display lists.
    while (!s_dacOutputRunning)
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

    DacOutputPioSm::Init();
    DacOutput::Init(DacOutputPioSm::Idle());
    mutex_init(s_displayListMutex + 0);
    mutex_init(s_displayListMutex + 1);

    s_pDisplayList[0] = new DisplayList(2048, 1024);
    s_pDisplayList[1] = new DisplayList(2048, 1024);

    for (uint i = 0; i < s_numDemos; ++i)
    {
        s_demos[i]->Init();
    }
    s_demos[s_demoIdx]->Start();

#if DAC_OUTPUT_CORE == 1
    multicore_launch_core1(dacOutputTask);
    displayListUpdateTask();
#else
    multicore_launch_core1(displayListUpdateTask);
    dacOutputTask();
#endif
}
