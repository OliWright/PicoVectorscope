// Helpers for drawing vector 3D shapes
//
// Copyright (C) 2023 Oli Wright
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
#include "transform3d.h"
#include "extras/camera.h"
#include "displaylist.h"

// Construction helper.  You can just pass in const arrays for points and edges
// without having to worry about the counts.
#define SHAPE_3D(points, edges) Shape3D(points, (uint) (sizeof(points) / sizeof(points[0])), edges, (uint) (sizeof(edges) / sizeof(edges[0])))

// 3D wireframe shape, consisting of points and edges
class Shape3D
{
public:
    typedef uint16_t Edge[2];

    // Often when you're constructing a Shape3D, you'll have fixed arrays of points
    // and edges.
    // It's recommended to use the SHAPE_3D helper, to make things a little less wordy.
    constexpr Shape3D(const StandardFixedTranslationVector* points, uint numPoints,
            const Edge* edges, uint numEdges)
        : m_points(points)
        , m_numPoints(numPoints)
        , m_edges(edges)
        , m_numEdges(numEdges)
    {}

    void Draw(DisplayList& displayList,
              const FixedTransform3D& modelToWorld,
              const Camera& camera,
              Intensity intensity) const;
    
private:
    const StandardFixedTranslationVector* m_points;
    uint                                  m_numPoints;
    const Edge*                           m_edges;
    uint                                  m_numEdges;
};

