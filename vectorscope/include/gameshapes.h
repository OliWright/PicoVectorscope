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
