#include "shapes.h"

#include "transform2d.h"



static inline float saturate(float val)
{
    return (val > 1.f) ? 1.f : (val < 0.f) ? 0.f : val;
}

static inline void pushVector(DisplayList& displayList, const ShapeVector2& point, Intensity intensity)
{
    DisplayListVector2 displayPoint(saturate(point.x), saturate(point.y));
    displayList.PushVector(displayPoint, intensity);
}

void PushShapeToDisplayList(
    DisplayList& displayList, const ShapeVector2* points, uint32_t numPoints, Intensity intensity, bool closed)
{
    pushVector(displayList, points[0], Intensity(0.f));
    for (uint32_t i = 1; i < numPoints; ++i)
    {
        pushVector(displayList, points[i], intensity);
    }
    if (closed)
    {
        pushVector(displayList, points[0], intensity);
    }
}

void PushShapeToDisplayList(DisplayList& displayList,
                            const ShapeVector2* points,
                            uint32_t numPoints,
                            Intensity intensity,
                            bool closed,
                            const FloatTransform2D& transform,
                            BurnLength burnLength)
{
    Vector2<float> point0;
    transform.transformVector(point0, points[0]);
    pushVector(displayList, point0, Intensity(0.f));
    Intensity burnBoost = 0;
    for (uint i = 1; i < numPoints; ++i)
    {
        Vector2<float> point;
        transform.transformVector(point, points[i]);
        if(burnLength != 0)
        {
            burnBoost = BurnLength(i+3) - burnLength;
            if(burnBoost < 0)
            {
                burnBoost = 0;
            }
            if(burnBoost > 3)
            {
                return;
            }
        }
        pushVector(displayList, point, intensity + burnBoost);
    }
    if (closed)
    {
        pushVector(displayList, point0, intensity);
    }
}
