// Helpers for rendering bitmaps to a bitmap display
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

#include "pico/types.h"

void Draw8bitWideSpriteTo1bitDisplay(uint8_t*       bitmapDisplay,
                                     int            displayPitchInBytes,
                                     int            x,
                                     int            y,
                                     int            width,
                                     int            height,
                                     const uint8_t* sprite,
                                     int            spritePitchInBytes)
{
    uint8_t*       displayByte = bitmapDisplay + (y * displayPitchInBytes) + (x >> 3);
    uint8_t        bitOffset   = x & 7;
    const uint8_t* end         = sprite + (height * spritePitchInBytes);
    if (bitOffset == 0)
    {
        // Sprite is nicely aligned with the destination bytes
        if (width == 8)
        {
            while (sprite != end)
            {
                *displayByte = *sprite;
                sprite += spritePitchInBytes;
                displayByte += displayPitchInBytes;
            }
        }
        else
        {
            uint8_t mask = 0xff >> width;
            while (sprite != end)
            {
                *displayByte = (*displayByte & mask) | *sprite;
                sprite += spritePitchInBytes;
                displayByte += displayPitchInBytes;
            }
        }
    }
    else
    {
        // The sprite is split over two bytes of the destination bitmap.

        // First do the left-hand byte
        uint8_t mask      = ~(((uint8_t)0xff) >> bitOffset);
        int     numPixels = 8 - bitOffset;
        if (numPixels > width)
        {
            mask |= (1 << (numPixels - width)) - 1;
        }
        const uint8_t* src = sprite;
        uint8_t*       dst = displayByte;
        while (src != end)
        {
            *dst = (*dst & mask) | (*src >> bitOffset);
            src += spritePitchInBytes;
            dst += displayPitchInBytes;
        }
        width -= numPixels;

        if (width > 0)
        {
            // Now do the right-hand byte
            src       = sprite;
            dst       = displayByte + 1;
            mask      = 0xff >> width;
            bitOffset = 8 - bitOffset;
            while (src != end)
            {
                *dst = (*dst & mask) | (*src << bitOffset);
                src += spritePitchInBytes;
                dst += displayPitchInBytes;
            }
        }
    }
}

// Totally sub-optimal
void DrawSpriteTo1bitDisplay(uint8_t*       bitmapDisplay,
                             int            displayPitchInBytes,
                             int            x,
                             int            y,
                             int            width,
                             int            height,
                             const uint8_t* sprite,
                             int            spritePitchInBytes)
{
    const int numColumns = (width + 7) >> 3;
    for (int i = 0; i < numColumns; ++i)
    {
        Draw8bitWideSpriteTo1bitDisplay(bitmapDisplay, displayPitchInBytes, x + (i << 3), y,
                                        (width >= 8) ? 8 : width, height, sprite + i,
                                        spritePitchInBytes);
        width -= 8;
    }
}

static void clearColumn(uint8_t* displayByte, int pitchInBytes, int height, const uint8_t mask)
{
    for (int i = 0; i < height; ++i)
    {
        *displayByte &= mask;
        displayByte += pitchInBytes;
    }
}

static void clearAlignedColumn(uint8_t* displayByte, int pitchInBytes, int height)
{
    for (int i = 0; i < height; ++i)
    {
        *displayByte = 0;
        displayByte += pitchInBytes;
    }
}

void ClearRectOn1bitDisplay(uint8_t* bitmapDisplay, int pitchInBytes, int x, int y, int width, int height)
{
    uint8_t* displayByte = bitmapDisplay + (y * pitchInBytes) + (x >> 3);
    uint8_t  bitOffset   = x & 7;
    if (bitOffset != 0)
    {
        // Clear the pixels within the first column of bytes
        uint8_t mask      = ~(((uint8_t)0xff) >> bitOffset);
        int     numPixels = 8 - bitOffset;
        if (numPixels > width)
        {
            mask &= ~((1 << width) - 1);
            numPixels = width;
        }
        clearColumn(displayByte, pitchInBytes, height, mask);

        // Move to the next byte, which should now be aligned
        width -= numPixels;
        x += numPixels;
        ++displayByte;
    }
    // Do the middle part of whole bytes
    while ((width >> 3) != 0)
    {
        clearAlignedColumn(displayByte, pitchInBytes, height);
        ++displayByte;
        width -= 8;
        x += 8;
    }
    if ((width & 7) != 0)
    {
        // Do the remaining column of bits in the final byte
        uint8_t mask = (((uint8_t)0xff) >> width);
        clearColumn(displayByte, pitchInBytes, height, mask);
    }
}