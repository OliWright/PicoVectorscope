#pragma once

#include "displaylist.h"
#include "transform2d.h"

typedef Vector2<float> ShapeVector2;

typedef FixedPoint<8, int32_t, int32_t, false> BurnLength;

void PushShapeToDisplayList(
    DisplayList& displayList, const ShapeVector2* points, uint32_t numPoints, Intensity intensity, bool closed);
void PushShapeToDisplayList(DisplayList& displayList,
                            const ShapeVector2* points,
                            uint32_t numPoints,
                            Intensity intensity,
                            bool closed,
                            const FloatTransform2D& transform,
                            BurnLength burnLength = 0.f);
