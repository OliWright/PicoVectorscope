// Container for what to draw in a frame.
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

// The DisplayList class is how the high-level code draws things.
//
// An instance of DisplayList is passed to the application code's
// UpdateAndRender method.  The application can draw vectors
// by using the PushVector methods, or it can draw points with
// PushPoint methods.

#pragma once
#include "fixedpoint.h"
#include "types.h"

typedef FixedPoint<1, 14, int16_t, int32_t, false> DisplayListScalar;
typedef FixedPoint<1, 26, int32_t, int32_t, false> DisplayListIntermediate;
typedef FixedPoint<3, 12, int16_t, int32_t, false> Intensity;

// Calculate the dx and dy steps in the DisplayListVector, or at the
// point that we're filling in the DAC output buffers.
// Trade-off memory vs. DacOut performance.
#define STEP_DIV_IN_DISPLAY_LIST 0

typedef Vector2<DisplayListScalar> DisplayListVector2;

class DisplayList
{
public:
    // Draw a line from the coordinates of the previous call to PushVector.
    // Use intensity=0 to move the 'cursor' without drawing anything.
    void PushVector(DisplayListScalar x, DisplayListScalar y, Intensity intensity);

    // Convenience version
    inline void PushVector(const DisplayListVector2& coord, Intensity intensity)
    {
        PushVector(coord.x, coord.y, intensity);
    }

    // Draw a point.
    // Note that the intensity of points is brighter than vectors for the same value.
    // Why?  Because bright points look really cool and they're reasonably practical
    // if you don't have many of them.  But lines of the same brightness have
    // to be drawn so slowly that they're impractical.
    void PushPoint(DisplayListScalar x, DisplayListScalar y, Intensity intensity);

    // *Experimental* Raster display
    typedef const uint8_t* (*RasterScanlineCallback)(uint32_t scanline);
    struct RasterDisplay
    {
        uint32_t width;
        uint32_t height;
        DisplayListVector2 topLeft = DisplayListVector2(0.f, 1.f);
        DisplayListVector2 bottomRight = DisplayListVector2(1.f, 0.f);
        RasterScanlineCallback scanlineCallback;
    };
    void PushRasterDisplay(const RasterDisplay& rasterDisplay);

public:
    DisplayList(uint32_t maxNumItems = 8192, uint32_t maxNumPoints = 4096);

    void OutputToDACs();
    void Clear()
    {
        m_numDisplayListVectors = 1;
        m_numDisplayListPoints  = 1;
        m_numRasterDisplays = 0;
    }

    void DebugDump() const;

private:
    void terminateVectors();
    void terminatePoints();

    struct Vector
    {
        DisplayListScalar x, y;
#if STEP_DIV_IN_DISPLAY_LIST
        DisplayListIntermediate stepX, stepY;
#endif
        uint16_t numSteps;
    };

    Vector*  m_pDisplayListVectors;
    uint32_t m_numDisplayListVectors;
    uint32_t m_maxDisplayListVectors;

    struct Point
    {
        DisplayListScalar x, y;
        Intensity         brightness;
    };

    Point*   m_pDisplayListPoints;
    uint32_t m_numDisplayListPoints;
    uint32_t m_maxDisplayListPoints;

    RasterDisplay* m_rasterDisplays;
    uint32_t m_numRasterDisplays;
};
