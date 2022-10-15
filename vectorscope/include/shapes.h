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

struct Fragment
{
    DisplayListVector2 m_position;
    DisplayListVector2 m_normalisedLineDirection;
    DisplayListVector2 m_velocity;
    DisplayListScalar  m_length;
    DisplayListScalar  m_rotationSpeed;
    Intensity          m_intensity;

    void Init(const DisplayListVector2& a, const DisplayListVector2& b);
};

uint32_t FragmentShape(const ShapeVector2* points,
                       uint32_t numPoints,
                       bool closed,
                       const FloatTransform2D& transform,
                       Fragment* outFragments,
                       uint32_t outFragmentsCapacity);

void PushFragmentsToDisplayList(DisplayList& displayList, const Fragment* fragments, uint32_t numFragments);
