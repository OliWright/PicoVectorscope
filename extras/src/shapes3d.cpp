// Helpers for drawing vector 3D shapes
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

#include "extras/shapes3d.h"
#include <alloca.h>

enum class ClipPlane
{
    Left,
    Right,
    Top,
    Bottom,
};
static const uint16_t kClipFlagLeft   = (1 << (int) ClipPlane::Left);
static const uint16_t kClipFlagRight  = (1 << (int) ClipPlane::Right);
static const uint16_t kClipFlagTop    = (1 << (int) ClipPlane::Top);
static const uint16_t kClipFlagBottom = (1 << (int) ClipPlane::Bottom);

static uint16_t clipPoint(const StandardFixedTranslationVector& v)
{
    uint16_t clipFlags = 0;
    if(-v.x > v.z) clipFlags |= kClipFlagLeft;
    if(v.x > v.z)  clipFlags |= kClipFlagRight;
    if(-v.y > v.z) clipFlags |= kClipFlagTop;
    if(v.y > v.z)  clipFlags |= kClipFlagBottom;
    return clipFlags;
}

static void projectPoint(DisplayListVector2& outScreenSpacePoint, const StandardFixedTranslationVector& v)
{
    outScreenSpacePoint.x = (v.x / v.z) * 0.5f + 0.5f;
    outScreenSpacePoint.y = (v.y / v.z) * 0.5f + 0.5f;
}

static bool clip(StandardFixedTranslationVector& a, StandardFixedTranslationVector& b,
                 StandardFixedTranslationScalar da, StandardFixedTranslationScalar db,
                 uint16_t& clipFlagsA, uint16_t& clipFlagsB, uint16_t& planesTested, const uint16_t clipPlane)
{
    //StandardFixedOrientationScalar ratio = da / (da - db);
    StandardFixedOrientationScalar ratio = (float) da / (float) (da - db); // Tiny bit of float maths for now, just to make things easy
    StandardFixedTranslationVector clipped = a + (b - a) * ratio;
    planesTested |= clipPlane;
    const uint16_t newClipFlags = clipPoint(clipped) & ~planesTested;
    if(clipFlagsA & clipPlane)
    {
        a = clipped;
        clipFlagsA = newClipFlags;
    }
    else
    {
        b = clipped;
        clipFlagsB = newClipFlags;
    }
    return (clipFlagsA & clipFlagsB) != 0;
}

static void drawClippedLine(DisplayList& displayList, Intensity intensity,
        const StandardFixedTranslationVector& viewA, const StandardFixedTranslationVector& viewB,
        const DisplayListVector2& screenA, const DisplayListVector2& screenB,
        uint16_t clipFlagsA, uint16_t clipFlagsB, bool previousEndMatch)
{
    if((clipFlagsA == 0) && (clipFlagsB == 0))
    {
        // Both points are on the screen.  Happy days.
        if(!previousEndMatch)
        {
            // Start a new line
            displayList.PushVector(screenA, 0);
        }
        displayList.PushVector(screenB, intensity);
        return;
    }
    if((clipFlagsA & clipFlagsB) != 0)
    {
        // Both points are clipped by the same plane, so definitely off screen
        return;
    }
    uint16_t planesTested = 0;
    StandardFixedTranslationVector a = viewA;
    StandardFixedTranslationVector b = viewB;
    if((clipFlagsA | clipFlagsB) & kClipFlagLeft)
    {
        StandardFixedTranslationScalar da = a.x + a.z;
        StandardFixedTranslationScalar db = b.x + b.z;
        if(clip(a, b, da, db, clipFlagsA, clipFlagsB, planesTested, kClipFlagLeft)) return;
    }
    if((clipFlagsA | clipFlagsB) & kClipFlagRight)
    {
        StandardFixedTranslationScalar da = -a.x + a.z;
        StandardFixedTranslationScalar db = -b.x + b.z;
        if(clip(a, b, da, db, clipFlagsA, clipFlagsB, planesTested, kClipFlagRight)) return;
    }
    if((clipFlagsA | clipFlagsB) & kClipFlagTop)
    {
        StandardFixedTranslationScalar da = a.y + a.z;
        StandardFixedTranslationScalar db = b.y + b.z;
        if(clip(a, b, da, db, clipFlagsA, clipFlagsB, planesTested, kClipFlagTop)) return;
    }
    if((clipFlagsA | clipFlagsB) & kClipFlagBottom)
    {
        StandardFixedTranslationScalar da = -a.y + a.z;
        StandardFixedTranslationScalar db = -b.y + b.z;
        if(clip(a, b, da, db, clipFlagsA, clipFlagsB, planesTested, kClipFlagBottom)) return;
    }

    //assert((clipFlagsA == 0) && (clipFlagsB == 0));
    //assert((a.z > 0.f) && (b.z > 0.f));

    // Both points should now be on the screen
    DisplayListVector2 screenPos;
    projectPoint(screenPos, a);
    displayList.PushVector(screenPos, 0);
    projectPoint(screenPos, b);
    displayList.PushVector(screenPos, intensity);
}

void Shape3D::Draw(DisplayList& displayList,
                   const FixedTransform3D& modelToWorld,
                   const FixedTransform3D& worldToView,
                   Intensity intensity) const
{
    FixedTransform3D modelToView = modelToWorld * worldToView;

    // Transform all the points to view space, then screen space (unless clipped)
    StandardFixedTranslationVector* viewSpacePoints = (StandardFixedTranslationVector*) alloca(sizeof(StandardFixedTranslationVector) * m_numPoints);
    DisplayListVector2* screenSpacePoints = (DisplayListVector2*) alloca(sizeof(DisplayListVector2) * m_numPoints);
    uint16_t* clipFlags = (uint16_t*) alloca(sizeof(uint16_t) * m_numPoints);
    for (uint32_t i = 0; i < m_numPoints; ++i)
    {
        modelToView.transformVector(viewSpacePoints[i], m_points[i]);
        viewSpacePoints[i].x *= (3.f / 4.f);
        clipFlags[i] = clipPoint(viewSpacePoints[i]);
        if(clipFlags[i] == 0)
        {
            projectPoint(screenSpacePoints[i], viewSpacePoints[i]);
        }
    }

    // Draw all the edges
    uint16_t previousPoint = 0xffff;
    for (uint32_t i = 0; i < m_numEdges; ++i)
    {
        const uint16_t a = m_edges[i][0];
        const uint16_t b = m_edges[i][1];
        drawClippedLine(displayList, intensity,
                        viewSpacePoints[a], viewSpacePoints[b],
                        screenSpacePoints[a], screenSpacePoints[b],
                        clipFlags[a], clipFlags[b],
                        a == previousPoint);
        previousPoint = b;
    }
}