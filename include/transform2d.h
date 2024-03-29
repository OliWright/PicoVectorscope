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

#pragma once
#include "types.h"
#include "fixedpoint.h"
//#include <math.h>
//#include "pico/float.h"

template <typename T = float>
struct Transform2D
{
    typedef T ScalarType;
    typedef Vector2<ScalarType> Vector2Type;
    ScalarType m[3][2];

    void setAsIdentity()
    {
        m[0][0] = 1.f;
        m[0][1] = 0.f;
        m[1][0] = 0.f;
        m[1][1] = 1.f;
        m[2][0] = 0.f;
        m[2][1] = 0.f;
    }

    void setAsRotation(ScalarType s, ScalarType c, const Vector2Type& origin = Vector2Type(0.f, 0.f))
    {
        m[0][0] = c;
        m[0][1] = s;
        m[1][0] = -s;
        m[1][1] = c;

        m[2][0] = origin.x - ((origin.x * m[0][0]) + (origin.y * m[1][0]));
        m[2][1] = origin.y - ((origin.x * m[0][1]) + (origin.y * m[1][1]));
    }

    void setAsScale(ScalarType scale)
    {
        m[0][0] = scale;
        m[0][1] = 0.f;
        m[1][0] = 0.f;
        m[1][1] = scale;
        m[2][0] = 0.f;
        m[2][1] = 0.f;
    }

    void setTranslation(const Vector2Type& translation)
    {
        m[2][0] = translation.x;
        m[2][1] = translation.y;
    }

    void translate(const Vector2Type& translation)
    {
        m[2][0] += translation.x;
        m[2][1] += translation.y;
    }

    Transform2D& operator*=(const Transform2D& b)
    {
        const Transform2D a = *this;

        m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]);
        m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]);

        m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]);
        m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[1][1] * b.m[1][1]);

        m[2][0] = (a.m[2][0] * b.m[0][0]) + (a.m[2][1] * b.m[1][0]) + b.m[2][0];
        m[2][1] = (a.m[2][0] * b.m[0][1]) + (a.m[2][1] * b.m[1][1]) + b.m[2][1];

        return *this;
    }

    Transform2D& operator*=(ScalarType scale)
    {
        m[0][0] *= scale;
        m[0][1] *= scale;
        m[1][0] *= scale;
        m[1][1] *= scale;
        m[2][0] *= scale;
        m[2][1] *= scale;

        return *this;
    }

    void transformVector(Vector2Type& result, const Vector2Type& v) const
    {
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + m[2][0];
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + m[2][1];
    }
};

typedef Transform2D<float> FloatTransform2D;
typedef Transform2D<FixedPoint<3,16,int32_t,int32_t,false> > FixedTransform2D;
