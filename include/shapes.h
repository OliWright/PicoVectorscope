// Helpers for drawing predefined 2d shapes.
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

#pragma once
#include "displaylist.h"
#include "transform2d.h"

typedef FixedTransform2D::Vector2Type              ShapeVector2;
typedef FixedPoint<12, 8, int32_t, int32_t, false> BurnLength;

// Draw a shape, as defined by an array of 2D points.
void PushShapeToDisplayList(DisplayList&        displayList,
                            const ShapeVector2* points,
                            uint32_t            numPoints,
                            Intensity           intensity,
                            bool                closed);

// As above, but apply a 2D transform to each point first.
// Also introduces 'BurnLength', which is an optional effect to
// draw the shape up to a point, with the head being drawn brighter
// than the rest.
// The BurnLength can be animated by increasing its value over
// time to give the impression of the shape being drawn slowly
// and 'burning' in to the display.
void PushShapeToDisplayList(DisplayList&            displayList,
                            const ShapeVector2*     points,
                            uint32_t                numPoints,
                            Intensity               intensity,
                            bool                    closed,
                            const FixedTransform2D& transform,
                            BurnLength              burnLength = 0.f);

// A Fragment is a piece of a shape.  Shapes can be 'fragmented' to
// break them up from a list of points, to a list of disjoint Fragments.
// The Fragments can then be drawn and animated individually.
struct Fragment
{
    DisplayListVector2 m_position;
    DisplayListVector2 m_normalisedLineDirection;
    DisplayListVector2 m_velocity;
    DisplayListScalar  m_length;
    DisplayListScalar  m_rotationSpeed;
    Intensity          m_intensity;

    void Init(const DisplayListVector2& a, const DisplayListVector2& b);
    void Move();
};

// Break apart a shape into a list of Fragments
uint32_t FragmentShape(const ShapeVector2*     points,
                       uint32_t                numPoints,
                       bool                    closed,
                       const FixedTransform2D& transform,
                       Fragment*               outFragments,
                       uint32_t                outFragmentsCapacity);

// Draw a list of Fragments
void PushFragmentsToDisplayList(DisplayList&    displayList,
                                const Fragment* fragments,
                                uint32_t        numFragments);
