// Container for what to draw in a frame.
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

// The main purpose of this code is to interpret a high-level display list
// of vectors and points, and output data to the DacOutputSM FIFO for
// squirting out to the DACs.
//
// This is the code that steps along vectors, calculating step deltas
// to scan along the vector at a speed appropriate for its intensity.
//
// So one entry in the DisplayList for a single vector is converted into
// many entries in the DacOutputSM FIFO for stepping the DACs along
// that vector.

#include "displaylist.h"

#include "dacout.h"
#include "dacoutputsm.h"
#include "log.h"
#include "pico/assert.h"

#include <cstdlib>


#define SPEED_CONSTANT 2048

static const uint kMaxRasterDisplays = 4;

static LogChannel DisplayListSynchronisation(false);
static LogChannel RasterInfo(false);

static const uint8_t s_pixelToHold[256]
    = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
        1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,
        3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,
        4,  4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
        6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9,
        9,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16 };

DisplayList::DisplayList(uint32_t maxNumItems, uint32_t maxNumPoints)
    : m_pDisplayListVectors((Vector*)malloc(maxNumItems * sizeof(Vector))),
      m_numDisplayListVectors(1),
      m_maxDisplayListVectors(maxNumItems),
      m_pDisplayListPoints((Point*)malloc(maxNumPoints * sizeof(Point))),
      m_numDisplayListPoints(0),
      m_maxDisplayListPoints(maxNumPoints),
      m_rasterDisplays((RasterDisplay*)malloc(kMaxRasterDisplays * sizeof(RasterDisplay))),
      m_numRasterDisplays(0)
{
#if !STEP_DIV_IN_DISPLAY_LIST
    static_assert(sizeof(Vector) == 6, "");
#endif

    // Initialise the first vector in the displaylist
    Vector& vector  = m_pDisplayListVectors[0];
    vector.x        = 0.f;
    vector.y        = 0.f;
    vector.numSteps = 1;
#if STEP_DIV_IN_DISPLAY_LIST
    vector.stepX = 0.f;
    vector.stepY = 0.f;
#endif

    Point& point     = m_pDisplayListPoints[0];
    point.x          = 0;
    point.y          = 0;
    point.brightness = 1.f;
}

// TODO: Make these configurable
static DisplayListVector2 s_calibrationScale(0.875f, 0.875f);
static DisplayListVector2 s_calibrationBias(0.0625f, 0.0625f);

void DisplayList::PushVector(DisplayListScalar x, DisplayListScalar y, Intensity intensity)
{
    if (m_numDisplayListVectors >= (m_maxDisplayListVectors - 1)) // Leave space for the Terminator
    {
        return;
    }
    Vector&       vector                   = m_pDisplayListVectors[m_numDisplayListVectors++];
    vector.x                               = (x * s_calibrationScale.x) + s_calibrationBias.x;
    vector.y                               = (y * s_calibrationScale.y) + s_calibrationBias.y;
    vector.numSteps = 1;
    if(intensity > 0)
    {
        const Vector& previous                 = m_pDisplayListVectors[m_numDisplayListVectors - 2];
        DisplayListScalar::IntermediateType dx = vector.x - previous.x;
        DisplayListScalar::IntermediateType dy = vector.y - previous.y;

        Intensity::IntermediateType length = ((dx * dx) + (dy * dy)).sqrt();
        Intensity::IntermediateType time   = intensity * intensity * length;

        uint32_t numSteps = (time * SPEED_CONSTANT).getIntegerPart() + 1;
        const uint32_t kMaxSteps = DacOutput::kNumEntriesPerBuffer - 32; //< Minus nominal value for pre and post steps.
                                                                         //  TODO: Make more rigorous
        vector.numSteps   = (numSteps > kMaxSteps) ? kMaxSteps : (uint16_t)numSteps;
    #if STEP_DIV_IN_DISPLAY_LIST
        vector.stepX = DisplayListIntermediate(dx) / (int) vector.numSteps;
        vector.stepY = DisplayListIntermediate(dy) / (int) vector.numSteps;
    #endif
    }
}

void DisplayList::PushPoint(DisplayListScalar x, DisplayListScalar y, Intensity intensity)
{
    if (m_numDisplayListPoints < m_maxDisplayListPoints)
    {
        Point& dst = m_pDisplayListPoints[m_numDisplayListPoints++];
        dst.x      = (x * s_calibrationScale.x) + s_calibrationBias.x;
        dst.y      = (y * s_calibrationScale.y) + s_calibrationBias.y;

        dst.brightness = intensity;
    }
}

void DisplayList::PushRasterDisplay(const RasterDisplay& rasterDisplay)
{
    if (m_numRasterDisplays >= kMaxRasterDisplays)
    {
        return;
    }
    m_rasterDisplays[m_numRasterDisplays++] = rasterDisplay;
}

void DisplayList::terminateVectors()
{
    // Move the beam to 0,0 after the final vector because there will be a small
    // delay until the PIO program is switched
    assert(m_numDisplayListVectors < m_maxDisplayListVectors);
    Vector& vector  = m_pDisplayListVectors[m_numDisplayListVectors++];
    vector.x        = 0;
    vector.y        = 0;
    vector.numSteps = 1;
#if STEP_DIV_IN_DISPLAY_LIST
    const Vector& previous = m_pDisplayListVectors[m_numDisplayListVectors - 2];
    vector.stepX           = vector.x - previous.x;
    vector.stepY           = vector.y - previous.y;
#endif
}

void DisplayList::terminatePoints()
{
    // Move the beam to 0,0 after the final point because there will be a small
    // delay until the PIO program is switched
    assert(m_numDisplayListPoints < m_maxDisplayListPoints);
    Point& point     = m_pDisplayListPoints[m_numDisplayListPoints++];
    point.x          = 0;
    point.y          = 0;
    point.brightness = 0;
}

void DisplayList::DebugDump() const
{
#if 0 // LOG_ENABLED
    for (uint32_t i = 0; i < m_numDisplayListVectors; ++i)
    {
        const Vector& vector = m_pDisplayListVectors[i];
        LOG_INFO("%d %d (%.3f, %.3f) (%.3f, %.3f)\n", i, vector.numSteps, (float)vector.x, (float)vector.y, (float)vector.dx,
               (float)vector.dy);
    }
#endif
}

static inline uint32_t scalarTo12bit(DisplayListIntermediate v)
{
    int32_t bits = v.getStorage() >> (DisplayListIntermediate::kNumFractionalBits - 12);
    return bits & 0xfff;
}

static inline uint32_t scalarTo12bitNoWrap(DisplayListIntermediate v)
{
    return v.getStorage() >> (DisplayListIntermediate::kNumFractionalBits - 12);
}

static inline uint32_t scalarTo12bitClamped(DisplayListIntermediate v)
{
    int32_t bits = v.getStorage() >> (DisplayListIntermediate::kNumFractionalBits - 12);
    return (bits > 0xfff) ? 0xfff : ((bits < 0) ? 0 : bits);
}

static inline uint32_t calcDacOutputValue(DisplayListIntermediate x, DisplayListIntermediate y)
{
    uint32_t bitsX = scalarTo12bit(x);
    uint32_t bitsY = scalarTo12bit(y);
    return bitsX | (bitsY << 12); // The z value would take up the top 8 bits if we were using it | (255 << 24);
}


void DisplayList::OutputToDACs()
{
    LOG_INFO(DisplayListSynchronisation, "DL: %d, %d\n", m_numDisplayListVectors,
             m_numDisplayListPoints);

    // We do the points first, because they're time-consuming to output to the DACs, but
    // very lightweight from the CPU-side, filling in the output buffers.
    // So if there are any points to draw, then it gives us a head start in filling
    // the buffers.
    if (m_numDisplayListPoints > 0)
    {
        // LOG_INFO("Out Points Start\n");
        terminatePoints();
        terminatePoints();
        DacOutput::SetCurrentPioSm(DacOutputPioSm::Points());
        const uint32_t kRepeatCount = 1;
        for (uint32_t i = 0; i < kRepeatCount; ++i)
        {
            Point*   pPoint             = m_pDisplayListPoints;
            uint32_t numPointsRemaining = m_numDisplayListPoints;

            while (numPointsRemaining)
            {
                uint32_t  numPointsInBatch;
                uint32_t* pOutput
                    = DacOutput::AllocateBufferSpace(numPointsRemaining, numPointsInBatch);

                // Point* pEnd = pPoint + m_numDisplayListPoints;
                const uint32_t* pOutputEnd = pOutput + numPointsInBatch;
                for (; pOutput != pOutputEnd; ++pPoint, ++pOutput)
                {
                    const Point& point = *pPoint;
                    uint32_t     bitsX = point.x.getStorage() >> (point.x.kNumFractionalBits - 12);
                    uint32_t     bitsY = point.y.getStorage() >> (point.y.kNumFractionalBits - 12);
                    uint32_t     bits  = bitsX | (bitsY << 12);

                    // Get Z in terms of how many points.pio cycles do we want the point to be held
                    // for. The max time we can have is 2044 cycles, so let's go with 11-bits for
                    // now
                    int32_t cycles = (int32_t)(point.brightness * point.brightness).getStorage()
                                     >> (point.brightness.kNumFractionalBits - 11);
                    // Subtract the 12 cycle per-point constant
                    cycles -= 12;
                    uint32_t bitsZ;
                    if (cycles > 254)
                    {
                        // We need to use the long delay loop to accomplish this length of delay
                        bits |= (1 << 24);

                        bitsZ = cycles >> 4;
                        if (bitsZ > 127)
                        {
                            bitsZ = 127;
                        }
                    }
                    else
                    {
                        // Short delay is good
                        bitsZ = (cycles < 0) ? 0 : (cycles >> 1);
                    }
                    bits |= (bitsZ << 25);
                    *pOutput = bits;
                }
                numPointsRemaining -= numPointsInBatch;
            }
        }
        // LOG_INFO("Out Points End\n");
    }

    for (uint32_t i = 0; i < m_numRasterDisplays; ++i)
    {
        DacOutput::SetCurrentPioSm(DacOutputPioSm::Raster());

        const RasterDisplay& rasterDisplay = m_rasterDisplays[i];
        DisplayListVector2   topLeft;
        topLeft.x = (rasterDisplay.topLeft.x * s_calibrationScale.x) + s_calibrationBias.x;
        topLeft.y = (rasterDisplay.topLeft.y * s_calibrationScale.y) + s_calibrationBias.y;
        DisplayListVector2 bottomRight;
        bottomRight.x = (rasterDisplay.bottomRight.x * s_calibrationScale.x) + s_calibrationBias.x;
        bottomRight.y = (rasterDisplay.bottomRight.y * s_calibrationScale.y) + s_calibrationBias.y;
        DisplayListIntermediate dx = (bottomRight.x - topLeft.x) / (int)rasterDisplay.width;
        DisplayListIntermediate dy = (bottomRight.y - topLeft.y) / (int)rasterDisplay.height;
        DisplayListIntermediate y  = topLeft.y;
        const uint32_t num32BitEntriesToAllocatePerScanline = ((rasterDisplay.width + 1) >> 1) + 1;
        // LOG_INFO(RasterInfo, "dx: %f, dy: %f, x12: %d\n", (float) dx, (float) dy,
        // scalarTo12bit(bottomRight.x));
        for (uint32_t scanlineIdx = 0; scanlineIdx < rasterDisplay.height; ++scanlineIdx)
        {
            uint16_t* pOutputStart
                = (uint16_t*)DacOutput::AllocateBufferSpace(num32BitEntriesToAllocatePerScanline);
            uint16_t* pOutput = pOutputStart;
            // uint16_t* pEnd = pOutput + (numEntriesToAllocatePerScanline * 2);
            DisplayListIntermediate x = topLeft.x;
            *(pOutput++)              = 0;
            *(pOutput++)              = scalarTo12bit(y);

            switch (rasterDisplay.mode)
            {
            case RasterDisplay::Mode::e1Bit:
            {
                const uint8_t* pixel
                    = rasterDisplay.scanlineCallback(scanlineIdx, rasterDisplay.userData);
                const uint8_t* end      = pixel + ((rasterDisplay.width + 7 + rasterDisplay.horizontalScrollOffset) >> 3);
                const uint16_t holdBits = 15 << 12;
                uint bitStart = rasterDisplay.horizontalScrollOffset;
                uint bytesWithoutOutput = 0;
                for (; pixel != end; ++pixel)
                {
                    uint8_t pixelBlock = *pixel;
                    ++bytesWithoutOutput;
                    for (uint bitIdx = bitStart; bitIdx < 8; ++bitIdx)
                    {
                        x += dx;
                        if ((pixelBlock & (0x80 >> bitIdx)) != 0)
                        {
                            *(pOutput++) = scalarTo12bitNoWrap(x) | holdBits;
                            bytesWithoutOutput = 0;
                        }
                    }
                    bitStart = 0;

                    if(bytesWithoutOutput == 2)
                    {
                        bytesWithoutOutput = 0;
                        *(pOutput++) = scalarTo12bitNoWrap(x);
                    }
                }
                break;
            }

            case RasterDisplay::Mode::e4BitLinear:
            {
                const uint8_t* pixel
                    = rasterDisplay.scanlineCallback(scanlineIdx, rasterDisplay.userData);
                const uint8_t* end = pixel + rasterDisplay.width;
                for (; pixel != end; ++pixel)
                {
                    x += dx;
                    uint32_t hold = *pixel; // s_pixelToHold[*pixel];
                    if (hold != 0)
                    {
                        *(pOutput++) = scalarTo12bitNoWrap(x) | ((hold - 1) << 12);
                    }
                }
                break;
            }

            case RasterDisplay::Mode::e8BitGamma:
            {
                const uint8_t* pixel
                    = rasterDisplay.scanlineCallback(scanlineIdx, rasterDisplay.userData);
                const uint8_t* end = pixel + rasterDisplay.width;
                for (; pixel != end; ++pixel)
                {
                    x += dx;
                    uint32_t hold = s_pixelToHold[*pixel];
                    if (hold != 0)
                    {
                        *(pOutput++) = scalarTo12bitNoWrap(x) | ((hold - 1) << 12);
                    }
                }
                break;
            }
            }

            uint32_t num16BitEntriesUsed = pOutput - pOutputStart;
            if (num16BitEntriesUsed & 1)
            {
                // We used an odd number.  Add an inert output to make it even.
                *(pOutput++) = 1;
                ++num16BitEntriesUsed;
            }
            DacOutput::GiveBackUnusedEntries(num32BitEntriesToAllocatePerScanline
                                             - (num16BitEntriesUsed >> 1));

            y += dy;
        }
    }


    if (m_numDisplayListVectors > 1)
    {
        terminateVectors();
        Vector*                 pItem = m_pDisplayListVectors;
        Vector*                 pEnd  = pItem + m_numDisplayListVectors;
        DisplayListIntermediate x(0), y(0);

        DacOutput::SetCurrentPioSm(DacOutputPioSm::Vector());
        DisplayListIntermediate dx;
        DisplayListIntermediate dy;
        bool previousVectorWasJump = true;
        for (; pItem != pEnd; ++pItem)
        {
            const Vector& vector = *pItem;
            const uint32_t numSteps = vector.numSteps;
#if STEP_DIV_IN_DISPLAY_LIST
            dx = vector.stepX;
            dy = vector.stepY;
#else
            dx = DisplayListIntermediate(vector.x - x);
            dy = DisplayListIntermediate(vector.y - y);
            if(numSteps > 1)
            {
                dx /= (int) numSteps;
                dy /= (int) numSteps;
            }
#endif
            const bool thisVectorIsJump = (numSteps == 1);

            // TODO: Figure out how many 'pre-hold' steps to do, where we
            //       hold the beam at the start of the vector
            uint32_t numPreHoldSteps = 0;
            if(previousVectorWasJump && !thisVectorIsJump)
            {
                // Start of a new vector after a jump
                // **** Disabled for now
                //numPreHoldSteps = 1;
            }
            else
            {
                // This is a vector continuing from before
            }

            // TODO: Figure out how many 'overshoot' steps to do, where for
            //       a dim fast-beam vector, we deliberatly overshoot
            //       if the next vector is a jump
            uint32_t numOvershootSteps = 0;
             // If this vector is real, but the next vector is a jump...
             // (Should be safe to only look at the next vector if this one is real)
             // (But does assume short-circuit boolean evaluation)
            if(!thisVectorIsJump && (pItem[1].numSteps == 1))
            {
                // Add a step of overshoot
                // **** Disabled for now
                //numOvershootSteps = 1;
            }

            uint32_t numStepsRemaining = numSteps + numPreHoldSteps + numOvershootSteps;
            while (numStepsRemaining)
            {
                uint32_t  numStepsInBatch;
                uint32_t* pOutput
                    = DacOutput::AllocateBufferSpace(numStepsRemaining, numStepsInBatch);
                uint32_t* pOutputEnd;
                if(numPreHoldSteps > 0)
                {
                    assert(numStepsInBatch > numPreHoldSteps);
                    pOutputEnd = pOutput + numPreHoldSteps;
                    const uint32_t bits = calcDacOutputValue(x, y);
                    for (; pOutput != pOutputEnd; ++pOutput)
                    {
                        *pOutput = bits;
                    }
                    numStepsInBatch -= numPreHoldSteps;
                    numPreHoldSteps = 0;
                }
                pOutputEnd = pOutput + numStepsInBatch;
                for (; pOutput != pOutputEnd; ++pOutput)
                {
                    // Calculate the coordinate for this step of the vector
                    x += dx;
                    y += dy;
                    *pOutput = calcDacOutputValue(x, y);
                }
                numStepsRemaining -= numStepsInBatch;
            }
            // Snap to the true end of the vector
            x = vector.x;
            y = vector.y;
            previousVectorWasJump = thisVectorIsJump;
        }
        // LOG_INFO("Out Vectors End\n");
    }

    DacOutput::Flush(true);
}
