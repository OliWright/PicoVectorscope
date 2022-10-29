// Some predefined shapes for making cool games.
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

// This probably isn't a good place for this.
// Will probably move it to 'extras', or delete entirely.

#pragma once
#include "displaylist.h"
#include "transform2d.h"
#include "shapes.h"

enum class GameShape
{
    eAsteroid0,
    eAsteroid1,
    eAsteroid2,
    eAsteroid3,
    eSaucer,
    eShip,
    eThrust,

    eCount
};
void PushGameShape(DisplayList& displayList, const FixedTransform2D& transform, GameShape shape, Intensity intensity);

uint32_t FragmentGameShape(GameShape shape,
                           const FixedTransform2D& transform,
                           Fragment* outFragments,
                           uint32_t outFragmentsCapacity);
