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

#pragma once
#include "pico/types.h"

// This does absolutely no range checking at all.
// So only use it when you absolutely know that the message will fit
// within your bitmap.
// It uses an 8x8 pixel fixed pitch font.
void DrawTextTo1bitDisplay(
    uint8_t* bitmap, int pitchInBytes, int x, int y, const char* message, bool centred);

extern char font8x8_basic[128][8];