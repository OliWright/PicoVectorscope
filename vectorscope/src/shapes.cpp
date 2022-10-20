#include "shapes.h"
#include "sintable.h"

#include "transform2d.h"

static constexpr uint kBurnFadeLength = 8;
static constexpr BurnLength kBurnBoostMultiplier = 3.f / kBurnFadeLength;

static inline void pushVector(DisplayList& displayList, const FixedTransform2D::Vector2Type& point, Intensity intensity)
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
                            const FixedTransform2D& transform,
                            BurnLength burnLength)
{
    FixedTransform2D::Vector2Type point0;
    transform.transformVector(point0, points[0]);
    pushVector(displayList, point0, Intensity(0.f));
    Intensity burnBoost = 0;
    FixedTransform2D::Vector2Type previousPoint = point0;
    for (uint i = 1; i < numPoints; ++i)
    {
        FixedTransform2D::Vector2Type point;
        transform.transformVector(point, points[i]);
        if(burnLength != 0)
        {
            burnBoost = BurnLength(i+kBurnFadeLength) - burnLength;
            if(burnBoost < 0)
            {
                burnBoost = 0;
            }
            else if(burnBoost > (kBurnFadeLength - 1))
            {
                if(burnBoost > kBurnFadeLength)
                {
                    return;
                }
                // The final segment
                BurnLength fraction = BurnLength(1) - burnBoost.frac();
                point.x = (point.x - previousPoint.x) * fraction + previousPoint.x;
                point.y = (point.y - previousPoint.y) * fraction + previousPoint.y;

                DisplayListVector2 displayPoint(saturate(point.x), saturate(point.y));
                displayList.PushPoint(displayPoint.x, displayPoint.y, Intensity(1.f));
            }
        }
        pushVector(displayList, point, intensity + (burnBoost * kBurnBoostMultiplier));
        previousPoint = point;
    }
    if (closed)
    {
        pushVector(displayList, point0, intensity);
    }
}

void Fragment::Init(const DisplayListVector2& a, const DisplayListVector2& b)
{
    m_position.x = (a.x + b.x) * 0.5f;
    m_position.y = (a.y + b.y) * 0.5f;

    m_normalisedLineDirection.x = b.x - a.x;
    m_normalisedLineDirection.y = b.y - a.y;

    m_length = ((m_normalisedLineDirection.x * m_normalisedLineDirection.x) + (m_normalisedLineDirection.y * m_normalisedLineDirection.y)).sqrt();
    DisplayListScalar::IntermediateType recipLength = m_length.recip();
    m_normalisedLineDirection.x *= recipLength;
    m_normalisedLineDirection.y *= recipLength;

    m_intensity = 1.f;
    m_rotationSpeed = 0;
    m_velocity.x = 0;
    m_velocity.y = 0;
}

void Fragment::Move()
{
#if 1
    FixedTransform2D transform;
    SinTableValue s, c;
    SinTable::SinCos(m_rotationSpeed, s, c);
    transform.setAsRotation(s, c);

    FixedTransform2D::Vector2Type dir;
    dir.x = m_normalisedLineDirection.x;
    dir.y = m_normalisedLineDirection.y;
    FixedTransform2D::Vector2Type newDir;
    transform.transformVector(newDir, dir);
    m_normalisedLineDirection.x = newDir.x;
    m_normalisedLineDirection.y = newDir.y;
#endif
    m_position.x += m_velocity.x;
    m_position.y += m_velocity.y;
}

uint32_t FragmentShape(const ShapeVector2* points,
                       uint32_t numPoints,
                       bool closed,
                       const FixedTransform2D& transform,
                       Fragment* outFragments,
                       uint32_t outFragmentsCapacity)
{
    if(outFragmentsCapacity == 0)
    {
        return 0;
    }
    uint32_t numFragments = 0;

    FixedTransform2D::Vector2Type fPoint;
    transform.transformVector(fPoint, points[0]);
    DisplayListVector2 point0(saturate(fPoint.x), saturate(fPoint.y));
    DisplayListVector2 previousPoint = point0;
    for (uint i = 1; i < numPoints; ++i)
    {
        transform.transformVector(fPoint, points[i]);
        DisplayListVector2 point(saturate(fPoint.x), saturate(fPoint.y));
        (*outFragments++).Init(previousPoint, point);
        if(++numFragments == outFragmentsCapacity)
        {
            return numFragments;
        }

        previousPoint = point;
    }
    if (closed)
    {
        (*outFragments).Init(previousPoint, point0);
        ++numFragments;
    }

    return numFragments;
}

void PushFragmentsToDisplayList(DisplayList& displayList, const Fragment* fragments, uint32_t numFragments)
{
    const Fragment* endFragment = fragments + numFragments;
    for(;fragments != endFragment; ++fragments)
    {
        const Fragment& fragment = *fragments;
        DisplayListVector2 point;
        DisplayListVector2 halfEdge;
        halfEdge.x = fragment.m_normalisedLineDirection.x * fragment.m_length * 0.5f;
        halfEdge.y = fragment.m_normalisedLineDirection.y * fragment.m_length * 0.5f;
        point.x = fragment.m_position.x - halfEdge.x;
        point.y = fragment.m_position.y - halfEdge.y;
        displayList.PushVector(point, 0.f);
        point.x = fragment.m_position.x + halfEdge.x;
        point.y = fragment.m_position.y + halfEdge.y;
        displayList.PushVector(point, fragment.m_intensity);
    }
}
