// Helpers for text rendering to a bitmap display
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

#include "extras/bitmapfont.h"

#include "bitreversetable.h"
#include "extras/bitmap.h"
#include "font8x8/font8x8_basic.h"

static int calcStartX(int x, const char* message, bool centred)
{
    int startX = x;
    if (centred)
    {
        for (const uint8_t* chr = (uint8_t*)message; (*chr != 0) && (*chr != '\n'); ++chr)
        {
            startX -= 4;
        }
    }
    return startX;
}

void DrawTextTo1bitDisplay(
    uint8_t* bitmapDisplay, int pitchInBytes, int x, int y, const char* message, bool centred)
{
    uint8_t charSprite[8];
    int     runningX = calcStartX(x, message, centred);

    for (const uint8_t* chr = (const uint8_t*)message; *chr != 0; ++chr)
    {
        if (*chr == '\n')
        {
            runningX = calcStartX(x, (const char*)chr + 1, centred);
        }
        if (*chr > 32)
        {
            // Reverse the bits in the character to match our layout
            for (int i = 0; i < 8; ++i)
            {
                const uint8_t bits = (uint8_t)font8x8_basic[*chr & 0x7f][i];
                charSprite[i]      = kBitReverseTable256[bits];
            }
            Draw8bitWideSpriteTo1bitDisplay(bitmapDisplay, pitchInBytes, runningX, y, 8, 8, charSprite);
        }
        runningX += 8;
    }
}
