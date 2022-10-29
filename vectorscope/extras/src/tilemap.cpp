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

#include "font8x8/font8x8_basic.h"
#include "picovectorscope.h"

static const uint8_t kBitReverseTable256[] = 
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


const uint8_t* tileMapScanlineCallback(uint32_t scanline, void* userData)
{
    return ((TileMap*)userData)->createScanline((int32_t)scanline);
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
      m_cachedRowTiles(nullptr)
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
    m_rasterDisplay.userData = this;
    m_rasterDisplay.scanlineCallback = tileMapScanlineCallback;
    m_rasterDisplay.bottomRight = bottomRight;
    m_rasterDisplay.bottomRight = bottomRight;

    switch(m_rasterDisplay.mode)
    {
        case DisplayList::RasterDisplay::Mode::e1Bit:
            m_scanline = (uint8_t*) malloc(m_visibleWidthPixels >> 3);
            break;
        case DisplayList::RasterDisplay::Mode::e4BitLinear:
        case DisplayList::RasterDisplay::Mode::e8BitGamma:
            m_scanline = (uint8_t*) malloc(m_visibleWidthPixels >> 3);
            break;
    }
}

const uint8_t* TileMap::createScanline(int32_t scanline)
{
    int32_t pixelRow = (m_scrollOffsetPixelsY + scanline) % m_heightPixels;
    int32_t tileRow = pixelRow >> m_tileHeightLog2;
    if(m_cachedRow != tileRow)
    {
        m_cachedRow = tileRow;
        m_cachedRowTiles = m_rowCallback(tileRow);
    }

    int32_t startPixel = m_scrollOffsetPixelsX % m_widthPixels;
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
                fill1BitPixelSpan(startPixel, 0, rowWithinTile, firstSpanCount);
                fill1BitPixelSpan(0, firstSpanCount, rowWithinTile, secondSpanCount);
                break;
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
                fill1BitPixelSpan(startPixel, 0, rowWithinTile, m_visibleWidthPixels);
                break;
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
    int32_t srcByteIdx = srcPixelIdx >> 3;
    int32_t dstByteIdx = dstPixelIdx >> 3;
    uint8_t* dstByte = m_scanline + dstByteIdx;

    // Bit indices here are left to right (MSB to LSB) rather than
    // the more usual right to left when dealing with arithmetic.
    int32_t srcBitIdx = srcPixelIdx & 7;
    int32_t dstBitIdx = dstPixelIdx & 7;
    assert((srcBitIdx == 0) || (dstBitIdx == 0));

    // Let's deal with the first few pixels in order to get the destination pixel byte-aligned
    uint8_t tileByte;
    if((dstPixelIdx & 7) != 0)
    {
        tileByte = get1BitTileRowByte(m_cachedRowTiles[srcByteIdx], tilePixelRow);

        uint8_t dstMask = 0xff >> dstBitIdx;
        uint8_t dstByteValue = *dstByte;
        dstByteValue &= dstMask;
        dstByteValue |= (tileByte >> dstBitIdx);

        // Advance
        int32_t numPixelsHandled = 8 - dstBitIdx;
        srcPixelIdx += numPixelsHandled;
        dstPixelIdx += numPixelsHandled;
        ++dstByteIdx;
        ++dstByte;
        srcByteIdx = srcPixelIdx >> 3;
        srcBitIdx = srcPixelIdx & 7;
        dstBitIdx = 0;
        count -= numPixelsHandled;
    }

    // Now deal with the major part of the span, dealing with a whole number
    // of destination bytes.
    uint8_t* dstByteEnd = dstByte + (count >> 3);
    if(srcBitIdx == 0)
    {
        // Source and destination bits are fully aligned.
        // This is the easy case.
        while(dstByte != dstByteEnd)
        {
            tileByte = get1BitTileRowByte(m_cachedRowTiles[srcByteIdx++], tilePixelRow);
            *dstByte++ = tileByte;
        }
    }
    else
    {
        while(dstByte != dstByteEnd)
        {
            uint8_t dstByteValue = tileByte << (8 - srcBitIdx);
            tileByte = get1BitTileRowByte(m_cachedRowTiles[srcByteIdx++], tilePixelRow);
            dstByteValue |= tileByte >> srcBitIdx;
            *dstByte++ = dstByteValue;
        }
    }

    count = count & 7;
    if(count != 0)
    {
        // Fill in the final pixels

    }
}
