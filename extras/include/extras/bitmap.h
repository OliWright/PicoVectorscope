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

#pragma once
#include "pico/types.h"

enum class SpriteMode
{
    Default,     // Overwrites all target pixels
    SetPixels,   // Only writes non-zero sprite pixels
    ClearPixels, // Target is cleared from non-zero sprite pixels
};

// These does absolutely no range checking at all.
void DrawSpriteTo1bitDisplay(uint8_t*       bitmapDisplay,
                             int            displayPitchInBytes,
                             int            x,
                             int            y,
                             int            width,
                             int            height,
                             const uint8_t* sprite,
                             int            spritePitchInBytes,
                             SpriteMode     mode);

void ClearRectOn1bitDisplay(
    uint8_t* bitmapDisplay, int pitchInBytes, int x, int y, int width, int height);
