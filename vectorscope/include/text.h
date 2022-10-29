// Text drawing helpers.
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

// Text drawing is built on shapes.h
// This module simply has a bunch of predefined shapes for lots of characters.

#pragma once
#include "displaylist.h"
#include "shapes.h"
#include "transform2d.h"

// Print a message, with optional BurnLength and centering
void TextPrint(DisplayList&            displayList,
               const FixedTransform2D& transform,
               const char*             message,
               Intensity               intensity,
               BurnLength              burnLength = BurnLength::kMaxFloat,
               bool                    centre     = false);

// Helper for creating a FixedTransform2D when you only care about position and scale
void CalcTextTransform(const DisplayListVector2& pos,
                       const DisplayListScalar&  scale,
                       FixedTransform2D&         outTranform);

// Helper to calculate the total burn length of a message
BurnLength CalcBurnLength(const char* message);

// Helper to break apart a message into an array of Fragments
uint32_t FragmentText(const char*             message,
                      const FixedTransform2D& transform,
                      Fragment*               outFragments,
                      uint32_t                outFragmentsCapacity,
                      bool                    centre);
