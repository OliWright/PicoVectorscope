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
#include "sintable.h"

template <typename OrientationT = float, typename TranslationT = float>
struct Transform3D
{
    typedef OrientationT OrientationType;
    typedef TranslationT TranslationType;
    typedef Vector3<OrientationType> OrientationVector3Type;
    typedef Vector3<TranslationType> TranslationVector3Type;
    OrientationVector3Type m[3];
    TranslationVector3Type t;

    // Default constructor will leave all members uninitialised
    // (cannot be used constexpr)
    Transform3D() {}

    //
    // Some constexpr constructors that will initialise all members
    //

    // Identity orientation, with a specified translation
    explicit constexpr Transform3D(const TranslationVector3Type& translation)
    : m{{1,0,0}, {0,1,0}, {0,0,1}}
    , t(translation)
    {}
    // Scale, with a zero translation
    explicit constexpr Transform3D(const OrientationVector3Type& scale)
    : m{{scale.x,0,0}, {0,scale.y,0}, {0,0,scale.z}}
    , t(0,0,0)
    {}
    // Scale and translation
    explicit constexpr Transform3D(const TranslationVector3Type& translation, const OrientationVector3Type& scale)
    : m{{scale.x,0,0}, {0,scale.y,0}, {0,0,scale.z}}
    , t(translation)
    {}

    constexpr void setOrientationAsIdentity()
    {
        m[0] = OrientationVector3Type(1,0,0);
        m[1] = OrientationVector3Type(0,1,0);
        m[2] = OrientationVector3Type(0,0,1);
    }

    constexpr void setAsIdentity()
    {
        setOrientationAsIdentity();
        t[0] = 0.f;
        t[1] = 0.f;
        t[2] = 0.f;
    }

    void setRotationXYZ(SinTable::Index x, SinTable::Index y, SinTable::Index z)
    {
        SinTableValue s, c;
        OrientationType sx, cx, sy, cy, sz, cz;

        SinTable::SinCos(x, s, c);
        sx = (OrientationType) s; cx = (OrientationType) c;
        SinTable::SinCos(y, s, c);
        sy = (OrientationType) s; cy = (OrientationType) c;
        SinTable::SinCos(z, s, c);
        sz = (OrientationType) s; cz = (OrientationType) c;

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

    constexpr Transform3D& operator*=(OrientationType scale)
    {
        m[0] *= scale;
        m[1] *= scale;
        m[2] *= scale;

        return *this;
    }

    constexpr Transform3D& operator*=(const OrientationVector3Type& scale)
    {
        m[0] *= scale.x;
        m[1] *= scale.y;
        m[2] *= scale.z;
        t.x *= scale.x;
        t.y *= scale.y;
        t.z *= scale.z;

        return *this;
    }

    TranslationVector3Type operator * (const TranslationVector3Type& v) const
    {
        TranslationVector3Type result;
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + (v.z * m[2][0]) + t.x;
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + (v.z * m[2][1]) + t.y;
        result.z = (v.x * m[0][2]) + (v.y * m[1][2]) + (v.z * m[2][2]) + t.z;
        return result;
    }

    Transform3D operator * (const Transform3D& rhs) const
    {
        // Technically, this is backwards - but it allows for more intuitive
        // matrix concatentation
        // E.g. modelToView = modelToWorld * worldToView
        Transform3D result;
        rhs.rotateVector(result.m[0], m[0]);
        rhs.rotateVector(result.m[1], m[1]);
        rhs.rotateVector(result.m[2], m[2]);
        rhs.transformVector(result.t, t);
        return result;
    }

    Transform3D& operator *= (const Transform3D& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template<typename T>
    void transformVector(Vector3<T>& result, const Vector3<T>& v) const
    {
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + (v.z * m[2][0]) + t.x;
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + (v.z * m[2][1]) + t.y;
        result.z = (v.x * m[0][2]) + (v.y * m[1][2]) + (v.z * m[2][2]) + t.z;
    }

    template<typename T>
    void rotateVector(Vector3<T>& result, const Vector3<T>& v) const
    {
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + (v.z * m[2][0]);
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + (v.z * m[2][1]);
        result.z = (v.x * m[0][2]) + (v.y * m[1][2]) + (v.z * m[2][2]);
    }

    void orthonormalInvert(Transform3D& outTransform) const
    {
        // Transpose the orientation
        outTransform.m[0][0] = m[0][0];
        outTransform.m[0][1] = m[1][0];
        outTransform.m[0][2] = m[2][0];
        outTransform.m[1][0] = m[0][1];
        outTransform.m[1][1] = m[1][1];
        outTransform.m[1][2] = m[2][1];
        outTransform.m[2][0] = m[0][2];
        outTransform.m[2][1] = m[1][2];
        outTransform.m[2][2] = m[2][2];

        // Calculate the inverted translation
        const TranslationVector3Type negTrans(-t.x, -t.y, -t.z);
        outTransform.t.x = (negTrans.x * outTransform.m[0][0]) + (negTrans.y * outTransform.m[1][0]) + (negTrans.z * outTransform.m[2][0]);
        outTransform.t.y = (negTrans.x * outTransform.m[0][1]) + (negTrans.y * outTransform.m[1][1]) + (negTrans.z * outTransform.m[2][1]);
        outTransform.t.z = (negTrans.x * outTransform.m[0][2]) + (negTrans.y * outTransform.m[1][2]) + (negTrans.z * outTransform.m[2][2]);
    }
};

// Not recommended for the Pico due to lack of floating point hardware
typedef Transform3D<float> FloatTransform3D;

// Use this fixed point implementation instead
#if 0 // Switch to a floating-point reference implementation, to help debug
      // suspected fixed-point issues.
typedef float StandardFixedOrientationScalar;
typedef float StandardFixedTranslationScalar;
#else
typedef FixedPoint<3,16,int32_t,int64_t,false> StandardFixedOrientationScalar;
typedef FixedPoint<12,16,int32_t,int64_t,false> StandardFixedTranslationScalar;
#endif
typedef Vector3<StandardFixedOrientationScalar> StandardFixedOrientationVector;
typedef Vector3<StandardFixedTranslationScalar> StandardFixedTranslationVector;
typedef Transform3D<StandardFixedOrientationScalar, StandardFixedTranslationScalar> FixedTransform3D;
