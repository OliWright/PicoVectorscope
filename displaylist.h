#ifndef DISPLAY_LIST_H
#define DISPLAY_LIST_H

#include "types.h"

#include <cstdint>


#define USE_FIXED_POINT 1

#if USE_FIXED_POINT
#    include "fixedpoint.h"
typedef FixedPoint<14, int16_t, int32_t, false> DisplayListScalar;
//typedef DisplayListScalar::MathsIntermediateType DisplayListIntermediate;
typedef FixedPoint<26, int32_t, int32_t, false> DisplayListIntermediate;
typedef FixedPoint<13, int32_t, int32_t, false> Intensity;
#else
typedef float DisplayListScalar;
typedef float DisplayListIntermediate;
typedef float Intensity;
#endif

#define STEP_DIV_IN_DISPLAY_LIST 0


struct DisplayListVector
{
    DisplayListScalar x, y;
#if STEP_DIV_IN_DISPLAY_LIST
    DisplayListIntermediate stepX, stepY;
#endif
    uint16_t numSteps;
};

struct DisplayListPoint
{
    DisplayListScalar x, y;
    DisplayListScalar brightness;
};

typedef Vector2<DisplayListScalar> DisplayListVector2;

class DisplayList
{
public:
    DisplayList(uint32_t maxNumItems = 8192, uint32_t maxNumPoints = 4096);

    void OutputToDACs();
    void Clear()
    {
        m_numDisplayListVectors = 1;
        m_numDisplayListPoints = 1;
    }
    void PushVector(DisplayListScalar x, DisplayListScalar y, Intensity intensity);
    inline void PushVector(const DisplayListVector2& coord, Intensity intensity)
    {
        PushVector(coord.x, coord.y, intensity);
    }
    void PushPoint(const DisplayListPoint& point);
    inline void PushPoint(DisplayListScalar x, DisplayListScalar y, DisplayListScalar intensity)
    {
        PushPoint({x, y, intensity});
    }
    void DebugDump() const;

private:
    void terminateVectors();
    void terminatePoints();

    DisplayListVector* m_pDisplayListVectors;
    uint32_t m_numDisplayListVectors;
    uint32_t m_maxDisplayListVectors;

    DisplayListPoint* m_pDisplayListPoints;
    uint32_t m_numDisplayListPoints;
    uint32_t m_maxDisplayListPoints;
};

#endif
