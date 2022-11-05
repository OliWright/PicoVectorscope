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

#include "extras/tilemap.h"
#include "extras/bitmapfont.h"

#include "bitreversetable.h"
#include "picovectorscope.h"

const uint8_t* tileMapScanlineCallback(uint32_t scanline, void* userData)
{
    TileMap::DisplayListInfo& displayListInfo = *(TileMap::DisplayListInfo*)userData;

    return displayListInfo.m_pTileMap->createScanline((int32_t)scanline, displayListInfo);
}

TileMap::TileMap(Mode                      mode,
                 int32_t                   widthTiles,
                 int32_t                   heightTiles,
                 RowCallback               rowCallback,
                 int32_t                   visibleWidthTiles,
                 int32_t                   visibleHeightTiles,
                 const DisplayListVector2& topLeft,
                 const DisplayListVector2& bottomRight)
    : m_mode(mode),
      m_widthTiles(widthTiles),
      m_heightTiles(heightTiles),
      m_visibleWidthTiles(visibleWidthTiles),
      m_visibleHeightTiles(visibleHeightTiles),
      m_rowCallback(rowCallback),
      m_cachedRow(-1),
      m_cachedRowTiles(nullptr),
      m_displayListInfoIdx(0)
{
    switch(m_mode)
    {
        case Mode::eText:
            m_tileWidthPixels = m_tileHeightPixels = 8;
            m_tileWidthLog2 = m_tileHeightLog2 = 3;
            m_rasterDisplay.mode = DisplayList::RasterDisplay::Mode::e1Bit;
            break;
    }

    if(m_visibleWidthTiles == 0)
    {
        m_visibleWidthTiles = m_widthTiles;
    }
    if(m_visibleHeightTiles == 0)
    {
        m_visibleHeightTiles = m_heightTiles;
    }

    m_widthPixels = m_widthTiles << m_tileWidthLog2;
    m_heightPixels = m_heightTiles << m_tileHeightLog2;
    m_visibleWidthPixels = m_visibleWidthTiles << m_tileWidthLog2;
    m_visibleHeightPixels = m_visibleHeightTiles << m_tileHeightLog2;

    m_rasterDisplay.width = (uint32_t) m_visibleWidthPixels;
    m_rasterDisplay.height = (uint32_t) m_visibleHeightPixels;
    m_rasterDisplay.scanlineCallback = tileMapScanlineCallback;
    m_rasterDisplay.bottomRight = bottomRight;
    m_rasterDisplay.bottomRight = bottomRight;

    switch(m_rasterDisplay.mode)
    {
        case DisplayList::RasterDisplay::Mode::e1Bit:
            // Allocate an extra byte because we utilise the horizontal scroll
            // offset in DisplayList::RasterDisplay
            m_scanline = (uint8_t*) malloc((m_visibleWidthPixels + 7 + 8) >> 3);
            break;
        case DisplayList::RasterDisplay::Mode::e4BitLinear:
        case DisplayList::RasterDisplay::Mode::e8BitGamma:
            m_scanline = (uint8_t*) malloc(m_visibleWidthPixels >> 3);
            break;
    }
}

static int32_t mod(int32_t a, int32_t b)
{
    const int32_t r = a % b;
    return (r < 0) ? (r + b) : r;
}

void TileMap::PushToDisplayList(DisplayList& displayList)
{
    DisplayListInfo& displayListInfo = m_displayListInfo[m_displayListInfoIdx];
    m_displayListInfoIdx = 1 - m_displayListInfoIdx;
    displayListInfo.m_pTileMap = this;
    displayListInfo.m_scrollOffsetPixelsX = m_scrollOffsetPixelsX;
    displayListInfo.m_scrollOffsetPixelsY = m_scrollOffsetPixelsY;
    switch(m_rasterDisplay.mode)
    {
        case DisplayList::RasterDisplay::Mode::e1Bit:
            displayListInfo.m_horizontalScrollOffset = m_rasterDisplay.horizontalScrollOffset = mod(m_scrollOffsetPixelsX, m_widthPixels) & 7;
            break;
        default:
            break;
    }
    m_rasterDisplay.userData = (void*) &displayListInfo;
    displayList.PushRasterDisplay(m_rasterDisplay);
}

const uint8_t* TileMap::createScanline(int32_t scanline, struct DisplayListInfo& displayListInfo)
{
    int32_t pixelRow = mod((displayListInfo.m_scrollOffsetPixelsY + scanline), m_heightPixels);
    int32_t tileRow = pixelRow >> m_tileHeightLog2;
    if(m_cachedRow != tileRow)
    {
        m_cachedRow = tileRow;
        m_cachedRowTiles = m_rowCallback(tileRow);
    }

    int32_t startPixel = mod(displayListInfo.m_scrollOffsetPixelsX, m_widthPixels);
    int32_t rowWithinTile = pixelRow & ((1 << m_tileHeightLog2) - 1);
    if((startPixel + m_visibleWidthPixels) > m_widthPixels)
    {
        // This row will wrap around due to the scroll offset, so we'll
        // fill it in in two parts
        const int32_t firstSpanCount = m_widthPixels - startPixel;
        const int32_t secondSpanCount = m_visibleWidthPixels - firstSpanCount;
        switch(m_rasterDisplay.mode)
        {
            case DisplayList::RasterDisplay::Mode::e1Bit:
            {
                assert(displayListInfo.m_horizontalScrollOffset == (uint32_t)(startPixel & 7));
                fill1BitPixelSpan(startPixel, displayListInfo.m_horizontalScrollOffset, rowWithinTile, firstSpanCount);
                fill1BitPixelSpan(0, firstSpanCount + displayListInfo.m_horizontalScrollOffset, rowWithinTile, secondSpanCount);
                break;
            }
            case DisplayList::RasterDisplay::Mode::e4BitLinear:
            case DisplayList::RasterDisplay::Mode::e8BitGamma:
                break;
        }
    }
    else
    {
        // No wrapping needed for this scanline
        switch(m_rasterDisplay.mode)
        {
            case DisplayList::RasterDisplay::Mode::e1Bit:
            {
                assert(displayListInfo.m_horizontalScrollOffset == (uint32_t)(startPixel & 7));
                fill1BitPixelSpan(startPixel, displayListInfo.m_horizontalScrollOffset, rowWithinTile, m_visibleWidthPixels);
                break;
            }
            case DisplayList::RasterDisplay::Mode::e4BitLinear:
            case DisplayList::RasterDisplay::Mode::e8BitGamma:
                break;
        }
    }
    return m_scanline;
}

static uint8_t get1BitTileRowByte(uint32_t tileIdx, int32_t tilePixelRow)
{
    if(tileIdx <= 32)
    {
        return 0;
    }
#if 1
    return kBitReverseTable256[(uint8_t) font8x8_basic[tileIdx & 0x7f][tilePixelRow]];
#else
    uint8_t srcByte = (uint8_t) font8x8_basic[tileIdx & 0x7f][tilePixelRow];
    uint8_t dstByte = 0;
    for(uint i = 0; i < 4; ++i)
    {
        dstByte |= (srcByte & (1 << i)) << (7 - i * 2);
    }
    for(uint i = 0; i < 4; ++i)
    {
        dstByte |= (srcByte & (1 << (7 - i))) >> (7 - i * 2);
    }
    return dstByte;
#endif
}

void TileMap::fill1BitPixelSpan(int32_t srcPixelIdx, int32_t dstPixelIdx, int32_t tilePixelRow, int32_t count) const
{
    const uint8_t* srcByte = m_cachedRowTiles + (srcPixelIdx >> 3);
    uint8_t* dstByte = m_scanline + (dstPixelIdx >> 3);

    // The calling code is expected to utilise the DisplayList::RasterDisplay horizontal scroll
    // offset in such a way as to maky the src and dst bytes align.
    assert((srcPixelIdx & 7) == (dstPixelIdx & 7));

    // Bit indices here are left to right (MSB to LSB) rather than
    // the more usual right to left when dealing with arithmetic.

    // Handle the pixels in the first byte if we're not aligned to a byte boundary
    uint8_t tileByte = 0;
    if((dstPixelIdx & 7) != 0)
    {
        tileByte = get1BitTileRowByte(*srcByte++, tilePixelRow);
        uint8_t dstMask = 0xff >> (dstPixelIdx & 7);
        uint8_t dstByteValue = *dstByte;
        dstByteValue &= ~dstMask;
        dstByteValue |= tileByte & dstMask;
        *dstByte++ = dstByteValue;
        count -= (8 - (dstPixelIdx & 7));
    }

    // Now deal with the major part of the span, dealing with a whole number of bytes.
    uint8_t* dstByteEnd = dstByte + (count >> 3);
    while(dstByte != dstByteEnd)
    {
        tileByte = get1BitTileRowByte(*srcByte++, tilePixelRow);
        *dstByte++ = tileByte;
    }

    // Handle any remaining pixels in the final byte
    count = count & 7;
    if(count != 0)
    {
        // Fill in the final pixels
        uint8_t dstMask = ~(0xff >> count);
        uint8_t dstByteValue = *dstByte;
        dstByteValue &= ~dstMask;
        tileByte = get1BitTileRowByte(*srcByte, tilePixelRow);
        dstByteValue |= tileByte & dstMask;
        *dstByte = dstByteValue;
    }
}
