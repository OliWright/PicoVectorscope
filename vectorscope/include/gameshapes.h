#pragma once

#include "displaylist.h"
#include "transform2d.h"

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
void PushGameShape(DisplayList& displayList, const FloatTransform2D& transform, GameShape shape, Intensity intensity);
