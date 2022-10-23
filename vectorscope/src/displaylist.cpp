#include "log.h"

#include "displaylist.h"
#include "dacout.h"
#include "dacoutputsm.h"
#include "pico/assert.h"
#include "pico/stdlib.h"
#include "pico/float.h"

#include <math.h>
#include <stdio.h>

#define SPEED_CONSTANT 2048

static LogChannel DisplayListSynchronisation(false);

DisplayList::DisplayList(uint32_t maxNumItems, uint32_t maxNumPoints)
    : m_pDisplayListVectors((Vector*)malloc(maxNumItems * sizeof(Vector))),
      m_numDisplayListVectors(1),
      m_maxDisplayListVectors(maxNumItems),
      m_pDisplayListPoints((Point*)malloc(maxNumPoints * sizeof(Point))),
      m_numDisplayListPoints(0),
      m_maxDisplayListPoints(maxNumPoints)
{
#if !STEP_DIV_IN_DISPLAY_LIST
    static_assert(sizeof(Vector) == 6, "");
#endif

    // Initialise the first vector in the displaylist
    Vector& vector = m_pDisplayListVectors[0];
    vector.x = 0.f;
    vector.y = 0.f;
    vector.numSteps = 1;
#if STEP_DIV_IN_DISPLAY_LIST
    vector.stepX = 0.f;
    vector.stepY = 0.f;
#endif

    Point& point = m_pDisplayListPoints[0];
    point.x = 0;
    point.y = 0;
    point.brightness = 1.f;
}

static DisplayListVector2 calibrationScale(1.f, 0.875f);
static DisplayListVector2 calibrationBias(0.f, 0.0625f);

void DisplayList::PushVector(DisplayListScalar x, DisplayListScalar y, Intensity intensity)
{
    if (m_numDisplayListVectors >= (m_maxDisplayListVectors - 1)) // Leave space for the Terminator
    {
        return;
    }
    const Vector& previous = m_pDisplayListVectors[m_numDisplayListVectors - 1];
    Vector& vector = m_pDisplayListVectors[m_numDisplayListVectors++];
    vector.x = (x * calibrationScale.x) + calibrationBias.x;
    vector.y = (y * calibrationScale.y) + calibrationBias.y;
    DisplayListScalar::IntermediateType dx = vector.x - previous.x;
    DisplayListScalar::IntermediateType dy = vector.y - previous.y;

    Intensity::IntermediateType length = ((dx * dx) + (dy * dy)).sqrt();
    Intensity::IntermediateType time = intensity * intensity * length;
    uint32_t numSteps = (time * SPEED_CONSTANT).getIntegerPart() + 1;
    vector.numSteps = (numSteps > 0xffff) ? 0xffff : (uint16_t) numSteps;
#if STEP_DIV_IN_DISPLAY_LIST
    vector.stepX = DisplayListIntermediate(dx) / vector.numSteps;
    vector.stepY = DisplayListIntermediate(dy) / vector.numSteps;
#endif
}

void DisplayList::PushPoint(DisplayListScalar x, DisplayListScalar y, Intensity intensity)
{
    if(m_numDisplayListPoints < m_maxDisplayListPoints)
    {
        Point& dst = m_pDisplayListPoints[m_numDisplayListPoints++];
        dst.x = (x * calibrationScale.x) + calibrationBias.x;
        dst.y = (y * calibrationScale.y) + calibrationBias.y;
        dst.brightness = intensity;
    }
}


void DisplayList::terminateVectors()
{
    assert(m_numDisplayListVectors < m_maxDisplayListVectors);
    Vector& vector = m_pDisplayListVectors[m_numDisplayListVectors++];
    vector.x = 0;
    vector.y = 0;
    vector.numSteps = 1;
#if STEP_DIV_IN_DISPLAY_LIST
    const Vector& previous = m_pDisplayListVectors[m_numDisplayListVectors - 2];
    vector.stepX = vector.x - previous.x;
    vector.stepY = vector.y - previous.y;
#endif
}

void DisplayList::terminatePoints()
{
    assert(m_numDisplayListPoints < m_maxDisplayListPoints);
    Point& point = m_pDisplayListPoints[m_numDisplayListPoints++];
    point.x = 0;
    point.y = 0;
    point.brightness = 0;
}

void DisplayList::DebugDump() const
{
#if 0//LOG_ENABLED
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

void DisplayList::OutputToDACs()
{
    LOG_INFO(DisplayListSynchronisation, "DL: %d, %d\n", m_numDisplayListVectors, m_numDisplayListPoints);
    if(m_numDisplayListVectors > 1)
    {
        terminateVectors();
        Vector* pItem = m_pDisplayListVectors;
        Vector* pEnd = pItem + m_numDisplayListVectors;
        DisplayListIntermediate x(0), y(0);

        DacOutput::SetCurrentPioSm(DacOutputSm::Vector());
        for (; pItem != pEnd; ++pItem)
        {
            const Vector& vector = *pItem;
            uint32_t numSteps = vector.numSteps;
            if(numSteps > 8192) numSteps = 8192;
#if STEP_DIV_IN_DISPLAY_LIST
            const DisplayListIntermediate dx = vector.stepX;
            const DisplayListIntermediate dy = vector.stepY;
#else
            const DisplayListIntermediate dx = DisplayListIntermediate(vector.x - x) / (int) numSteps;
            const DisplayListIntermediate dy = DisplayListIntermediate(vector.y - y) / (int) numSteps;
#endif
            uint32_t numStepsRemaining = numSteps;
            while(numStepsRemaining)
            {
                uint32_t numStepsInBatch;
                uint32_t* pOutput = DacOutput::AllocateBufferSpace(numStepsRemaining, numStepsInBatch);
                const uint32_t* pOutputEnd = pOutput + numStepsInBatch;
                for (; pOutput != pOutputEnd; ++pOutput)
                {
                    // Calculate the coordinate for this step of the vector
                    x += dx;
                    y += dy;
                    uint32_t bitsX = scalarTo12bit(x);
                    uint32_t bitsY = scalarTo12bit(y);
                    *pOutput = bitsX | (bitsY << 12) | (255 << 24);
                }
                numStepsRemaining -= numStepsInBatch;
            }
            // Snap to the true end of the vector
            x = vector.x;
            y = vector.y;
        }
        //LOG_INFO("Out Vectors End\n");
    }
#if 1
    if(m_numDisplayListPoints > 0)
    {
        //LOG_INFO("Out Points Start\n");
        terminatePoints();
        terminatePoints();
        DacOutput::SetCurrentPioSm(DacOutputSm::Points());
        const uint32_t kRepeatCount = 1;
        for(uint32_t i = 0; i < kRepeatCount; ++i)
        {
            Point* pPoint = m_pDisplayListPoints;
            uint32_t numPointsRemaining = m_numDisplayListPoints;

            while(numPointsRemaining)
            {
                uint32_t numPointsInBatch;
                uint32_t* pOutput = DacOutput::AllocateBufferSpace(numPointsRemaining, numPointsInBatch);

                //Point* pEnd = pPoint + m_numDisplayListPoints;
                const uint32_t* pOutputEnd = pOutput + numPointsInBatch;
                for (; pOutput != pOutputEnd; ++pPoint, ++pOutput)
                {
                    const Point& point = *pPoint;
                    uint32_t bitsX = point.x.getStorage() >> (point.x.kNumFractionalBits - 12);
                    uint32_t bitsY = point.y.getStorage() >> (point.y.kNumFractionalBits - 12);
                    uint32_t bits = bitsX | (bitsY << 12);

                    // Get Z in terms of how many points.pio cycles do we want the point to be held for.
                    // The max time we can have is 2044 cycles, so let's go with 11-bits for now
                    int32_t cycles = (int32_t) (point.brightness * point.brightness).getStorage() >> (point.brightness.kNumFractionalBits - 11);
                    // Subtract the 12 cycle per-point constant
                    cycles -= 12;
                    uint32_t bitsZ;
                    if(cycles > 254)
                    {
                        // We need to use the long delay loop to accomplish this length of delay
                        bits |= (1 << 24);

                        bitsZ = cycles >> 4;
                        if(bitsZ > 127)
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
        //LOG_INFO("Out Points End\n");
    }
#endif
    DacOutput::Flush(true);
}
