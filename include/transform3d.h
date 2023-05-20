// Work-in-progress Transform3D class
//
// Copyright (C) 2023 Oli Wright
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

template <typename OrientationT = float, typename TranslationT = float>
struct Transform3D
{
    typedef OrientationT OrientationType;
    typedef TranslationT TranslationType;
    typedef Vector3<OrientationType> OrientationVector3Type;
    typedef Vector3<TranslationType> TranslationVector3Type;
    OrientationVector3Type m[3];
    TranslationVector3Type t;

    void setAsIdentity()
    {
        m[0][0] = 1.f;
        m[0][1] = 0.f;
        m[0][2] = 0.f;
        m[1][0] = 0.f;
        m[1][1] = 1.f;
        m[1][2] = 0.f;
        m[2][0] = 0.f;
        m[2][1] = 0.f;
        m[2][2] = 1.f;
        t[0] = 0.f;
        t[1] = 0.f;
        t[2] = 0.f;
    }

    void setRotationXYZ(SinTable::Index x, SinTable::Index y, SinTable::Index z)
    {
        SinTableValue s, c;
        OrientationType sx, cx, sy, cy, sz, cz;

        SinTable::SinCos(x, s, c);
        sx = s; cx = c;
        SinTable::SinCos(y, s, c);
        sy = s; cy = c;
        SinTable::SinCos(z, s, c);
        sz = s; cz = c;

        m[0] = OrientationVector3Type( cz * cy,  sz * cx +  cz * -sy * -sx,  sz * sx +  cz * -sy * cx);
        m[1] = OrientationVector3Type(-sz * cy,  cz * cx + -sz * -sy * -sx,  cz * sx + -sz * -sy * cx);
        m[2] = OrientationVector3Type( sy,       cy * -sx,                   cy * cx                 );
    }

    void setTranslation(const TranslationVector3Type& translation)
    {
        t = translation;
    }

    void translate(const TranslationVector3Type& translation)
    {
        t.x += translation.x;
        t.y += translation.y;
        t.z += translation.z;
    }

    Transform3D& operator*=(const Transform3D& b)
    {
        const Transform3D a = *this;

        m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]) + (a.m[0][2] * b.m[2][0]);
        m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]) + (a.m[0][2] * b.m[2][1]);
        m[0][2] = (a.m[0][0] * b.m[0][2]) + (a.m[0][1] * b.m[1][2]) + (a.m[0][2] * b.m[2][2]);

        m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]) + (a.m[1][2] * b.m[2][0]);
        m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[1][1] * b.m[1][1]) + (a.m[1][2] * b.m[2][1]);
        m[1][2] = (a.m[1][0] * b.m[0][2]) + (a.m[1][1] * b.m[1][2]) + (a.m[1][2] * b.m[2][2]);

        m[2][0] = (a.m[2][0] * b.m[0][0]) + (a.m[2][1] * b.m[1][0]) + (a.m[2][2] * b.m[2][0]);
        m[2][1] = (a.m[2][0] * b.m[0][1]) + (a.m[2][1] * b.m[1][1]) + (a.m[2][2] * b.m[2][1]);
        m[2][2] = (a.m[2][0] * b.m[0][2]) + (a.m[2][1] * b.m[1][2]) + (a.m[2][2] * b.m[2][2]);
 
        return *this;
    }

    Transform3D& operator*=(OrientationType scale)
    {
        m[0][0] *= scale;
        m[0][1] *= scale;
        m[0][2] *= scale;
        m[1][0] *= scale;
        m[1][1] *= scale;
        m[1][2] *= scale;
        m[2][0] *= scale;
        m[2][1] *= scale;
        m[2][2] *= scale;

        return *this;
    }

    void transformVector(TranslationVector3Type& result, const TranslationVector3Type& v) const
    {
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + (v.z * m[2][0]) + t.x;
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + (v.z * m[2][1]) + t.y;
        result.z = (v.x * m[0][2]) + (v.y * m[1][2]) + (v.z * m[2][2]) + t.z;
    }
};

// Not recommended for the Pico due to lack of floating point hardware
typedef Transform3D<float> FloatTransform3D;

// Use this fixed point implementation instead
typedef FixedPoint<3,16,int32_t,int64_t,false> StandardFixedOrientationScalar;
typedef FixedPoint<12,8,int32_t,int64_t,false> StandardFixedTranslationScalar;
typedef Vector3<StandardFixedTranslationScalar> StandardFixedTranslationVector;
typedef Transform3D<StandardFixedOrientationScalar, StandardFixedTranslationScalar> FixedTransform3D;
