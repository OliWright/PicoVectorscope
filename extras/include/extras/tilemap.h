// Tilemap support for pico vectorscope RasterDisplay
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

class TileMap
{
public:
    enum class Mode
    {
        eText,
    };

    typedef const uint8_t* (*RowCallback)(int32_t row);

    TileMap(Mode                      mode,
            int32_t                   widthTiles,
            int32_t                   heightTiles,
            RowCallback               rowCallback,
            int32_t                   visibleWidthTiles = 0,
            int32_t                   visibleHeightTiles = 0,
            const DisplayListVector2& topLeft     = DisplayListVector2(0.f, 1.f),
            const DisplayListVector2& bottomRight = DisplayListVector2(1.f, 0.f));

    void PushToDisplayList(DisplayList& displayList) {displayList.PushRasterDisplay(m_rasterDisplay);}

    void SetScrollOffsetPixels(int32_t x, int32_t y)
    {
        m_scrollOffsetPixelsX = x;
        m_scrollOffsetPixelsY = y;
    }

private:
    const uint8_t* createScanline(int32_t scanline);
    friend const uint8_t* tileMapScanlineCallback(uint32_t, void*);
    void fill1BitPixelSpan(int32_t srcPixelIdx, int32_t dstPixelIdx, int32_t tilePixelRow, int32_t count) const;

    Mode                       m_mode;
    int32_t                    m_tileWidthPixels;
    int32_t                    m_tileHeightPixels;
    int32_t                    m_tileWidthLog2;
    int32_t                    m_tileHeightLog2;
    int32_t                    m_widthTiles;
    int32_t                    m_heightTiles;
    int32_t                    m_widthPixels;
    int32_t                    m_heightPixels;
    int32_t                    m_visibleWidthTiles;
    int32_t                    m_visibleHeightTiles;
    int32_t                    m_visibleWidthPixels;
    int32_t                    m_visibleHeightPixels;
    int32_t                    m_scrollOffsetPixelsX;
    int32_t                    m_scrollOffsetPixelsY;
    RowCallback                m_rowCallback;
    DisplayList::RasterDisplay m_rasterDisplay;
    uint8_t*                   m_scanline;
    int32_t                    m_cachedRow;
    const uint8_t*             m_cachedRowTiles;
};
