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

#if !STEP_DIV_IN_DISPLAY_LIST
static_assert(sizeof(DisplayListVector) == 6, "");
#endif

static LogChannel DisplayListSynchronisation(false);

DisplayList::DisplayList(uint32_t maxNumItems, uint32_t maxNumPoints)
    : m_pDisplayListVectors((DisplayListVector*)malloc(maxNumItems * sizeof(DisplayListVector))),
      m_numDisplayListVectors(1),
      m_maxDisplayListVectors(maxNumItems),
      m_pDisplayListPoints((DisplayListPoint*)malloc(maxNumPoints * sizeof(DisplayListPoint))),
      m_numDisplayListPoints(0),
      m_maxDisplayListPoints(maxNumPoints)
{
    // Initialise the first vector in the displaylist
    DisplayListVector& vector = m_pDisplayListVectors[0];
    vector.x = 0.f;
    vector.y = 0.f;
    vector.numSteps = 1;
#if STEP_DIV_IN_DISPLAY_LIST
    vector.stepX = 0.f;
    vector.stepY = 0.f;
#endif

    DisplayListPoint& point = m_pDisplayListPoints[0];
    point.x = 0;
    point.y = 0;
    point.brightness = 1.f;
}

static DisplayListVector2 calibrationScale(1.f, 0.875f);
static DisplayListVector2 calibrationBias(0.f, 0.0625f);

static inline float Q_rsqrt( float number )
{
	int32_t i;
	float x2, y;
	const float threehalfs = 1.5F;
    union BitCast
    {
        float f;
        int32_t i;
        BitCast(float val) { f = val; }
        BitCast(int32_t val) { i = val; }
    };

	x2 = number * 0.5F;
	y  = number;
	i  = BitCast(number).i;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck? 
	y  = BitCast(i).f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

void DisplayList::PushVector(DisplayListScalar x, DisplayListScalar y, Intensity intensity)
{
    if (m_numDisplayListVectors >= (m_maxDisplayListVectors - 1)) // Leave space for the Terminator
    {
        return;
    }
    const DisplayListVector& previous = m_pDisplayListVectors[m_numDisplayListVectors - 1];
    DisplayListVector& vector = m_pDisplayListVectors[m_numDisplayListVectors++];
    vector.x = (x * calibrationScale.x) + calibrationBias.x;
    vector.y = (y * calibrationScale.y) + calibrationBias.y;
    DisplayListScalar::MathsIntermediateType dx = vector.x - previous.x;
    DisplayListScalar::MathsIntermediateType dy = vector.y - previous.y;
    intensity = Mul(intensity, intensity);
#if 1
    // Fixed point sqrt
    Intensity::MathsIntermediateType length = ((dx * dx) + (dy * dy)).sqrt();
    Intensity::MathsIntermediateType time = Mul(intensity, length);
    uint32_t numSteps = (time * SPEED_CONSTANT).getIntegerPart() + 1;
#elif 1
    // Quake rsqrt
    float length2 = fix2float(((dx * dx) + (dy * dy)).getStorage(), DisplayListIntermediate::kNumFractionalBits);
    Intensity::MathsIntermediateType recipLength = (Intensity::MathsIntermediateType::StorageType) float2fix(Q_rsqrt(length2), Intensity::MathsIntermediateType::kNumFractionalBits);
    Intensity::MathsIntermediateType time = intensity / recipLength;
    uint32_t numSteps = ((time * SPEED_CONSTANT).getIntegerPart() + 1);
#else
    // Built in floating point sqrtf
    float length2 = fix2float(((dx * dx) + (dy * dy)).getStorage(), DisplayListIntermediate::kNumFractionalBits);
    float length = sqrtf(length2);
    float time = length * (float)intensity;
    uint32_t numSteps = (time * SPEED_CONSTANT) + 1;
#endif
    vector.numSteps = (numSteps > 0xffff) ? 0xffff : (uint16_t) numSteps;
#if STEP_DIV_IN_DISPLAY_LIST
    vector.stepX = DisplayListIntermediate(dx) / vector.numSteps;
    vector.stepY = DisplayListIntermediate(dy) / vector.numSteps;
#endif

    //LOG_INFO("x: %f, y: %f, dx: %f, dy: %f\n", (float) vector.x, (float) vector.y, (float) vector.dx, (float) vector.dy);
}

void DisplayList::PushPoint(const DisplayListPoint& point)
{
    if(m_numDisplayListPoints < m_maxDisplayListPoints)
    {
        DisplayListPoint& dst = m_pDisplayListPoints[m_numDisplayListPoints++];
        dst.x = (point.x * calibrationScale.x) + calibrationBias.x;
        dst.y = (point.y * calibrationScale.y) + calibrationBias.y;
        dst.brightness = point.brightness;
    }
}


void DisplayList::terminateVectors()
{
    assert(m_numDisplayListVectors < m_maxDisplayListVectors);
    DisplayListVector& vector = m_pDisplayListVectors[m_numDisplayListVectors++];
    vector.x = 0;
    vector.y = 0;
    vector.numSteps = 1;
#if STEP_DIV_IN_DISPLAY_LIST
    const DisplayListVector& previous = m_pDisplayListVectors[m_numDisplayListVectors - 2];
    vector.stepX = vector.x - previous.x;
    vector.stepY = vector.y - previous.y;
#endif
}

void DisplayList::terminatePoints()
{
    assert(m_numDisplayListPoints < m_maxDisplayListPoints);
    DisplayListPoint& point = m_pDisplayListPoints[m_numDisplayListPoints++];
    point.x = 0;
    point.y = 0;
    point.brightness = 0;
}

void DisplayList::DebugDump() const
{
#if 0//LOG_ENABLED
    for (uint32_t i = 0; i < m_numDisplayListVectors; ++i)
    {
        const DisplayListVector& vector = m_pDisplayListVectors[i];
        LOG_INFO("%d %d (%.3f, %.3f) (%.3f, %.3f)\n", i, vector.numSteps, (float)vector.x, (float)vector.y, (float)vector.dx,
               (float)vector.dy);
    }
#endif
}

static inline uint32_t scalarTo12bit(DisplayListIntermediate v)
{
#if USE_FIXED_POINT
    int32_t bits = v.getStorage() >> (DisplayListIntermediate::kNumFractionalBits - 12);
#else
    int32_t bits = (int32_t)(v * 4095.f);
#endif
    return bits & 0xfff;
}

void DisplayList::OutputToDACs()
{
    LOG_INFO(DisplayListSynchronisation, "DL: %d, %d\n", m_numDisplayListVectors, m_numDisplayListPoints);
    if(m_numDisplayListVectors > 1)
    {
        terminateVectors();
        DisplayListVector* pItem = m_pDisplayListVectors;
        DisplayListVector* pEnd = pItem + m_numDisplayListVectors;
        DisplayListIntermediate x(0), y(0);

        DacOutput::SetCurrentPioSm(DacOutputSm::Vector());
        for (; pItem != pEnd; ++pItem)
        {
            const DisplayListVector& vector = *pItem;
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
            DisplayListPoint* pPoint = m_pDisplayListPoints;
            uint32_t numPointsRemaining = m_numDisplayListPoints;

            while(numPointsRemaining)
            {
                uint32_t numPointsInBatch;
                uint32_t* pOutput = DacOutput::AllocateBufferSpace(numPointsRemaining, numPointsInBatch);

                //DisplayListPoint* pEnd = pPoint + m_numDisplayListPoints;
                const uint32_t* pOutputEnd = pOutput + numPointsInBatch;
                for (; pOutput != pOutputEnd; ++pPoint, ++pOutput)
                {
                    const DisplayListPoint& point = *pPoint;
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
