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
    typedef Vector3<OrientationType> OrientationVectorType;
    typedef Vector3<TranslationType> TranslationVectorType;
    OrientationVectorType m[3];
    TranslationVectorType t;

    // Default constructor will leave all members uninitialised
    // (cannot be used constexpr)
    Transform3D() {}

    //
    // Some constexpr constructors that will initialise all members
    //

    // Identity orientation, with a specified translation
    explicit constexpr Transform3D(const TranslationVectorType& translation)
    : m{{1,0,0}, {0,1,0}, {0,0,1}}
    , t(translation)
    , m_orientationIsIdentity(true)
    {}
    // Scale, with a zero translation
    explicit constexpr Transform3D(const OrientationVectorType& scale)
    : m{{scale.x,0,0}, {0,scale.y,0}, {0,0,scale.z}}
    , t(0,0,0)
    , m_orientationIsIdentity((scale.x == 1) && (scale.y == 1) && (scale.z == 1))
    {}
    // Scale and translation
    explicit constexpr Transform3D(const TranslationVectorType& translation, const OrientationVectorType& scale)
    : m{{scale.x,0,0}, {0,scale.y,0}, {0,0,scale.z}}
    , t(translation)
    , m_orientationIsIdentity((scale.x == 1) && (scale.y == 1) && (scale.z == 1))
    {}

    constexpr void setOrientationAsIdentity()
    {
        m[0] = OrientationVectorType(1,0,0);
        m[1] = OrientationVectorType(0,1,0);
        m[2] = OrientationVectorType(0,0,1);
        m_orientationIsIdentity = true;
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

        m[0] = OrientationVectorType( cz * cy,  sz * cx +  cz * -sy * -sx,  sz * sx +  cz * -sy * cx);
        m[1] = OrientationVectorType(-sz * cy,  cz * cx + -sz * -sy * -sx,  cz * sx + -sz * -sy * cx);
        m[2] = OrientationVectorType( sy,       cy * -sx,                   cy * cx                 );
        m_orientationIsIdentity = false;
    }

    void setTranslation(const TranslationVectorType& translation)
    {
        t = translation;
    }

    void translate(const TranslationVectorType& translation)
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
        m_orientationIsIdentity = false;

        return *this;
    }

    constexpr Transform3D& operator*=(const OrientationVectorType& scale)
    {
        m[0] *= scale.x;
        m[1] *= scale.y;
        m[2] *= scale.z;
        t.x *= scale.x;
        t.y *= scale.y;
        t.z *= scale.z;
        m_orientationIsIdentity = false;

        return *this;
    }

    TranslationVectorType operator * (const TranslationVectorType& v) const
    {
        TranslationVectorType result;
        if(m_orientationIsIdentity)
        {
            result = v + t;
        }
        else
        {
            result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + (v.z * m[2][0]) + t.x;
            result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + (v.z * m[2][1]) + t.y;
            result.z = (v.x * m[0][2]) + (v.y * m[1][2]) + (v.z * m[2][2]) + t.z;
        }
        return result;
    }

    Transform3D operator * (const Transform3D& rhs) const
    {
        // Technically, this is backwards - but it allows for more intuitive
        // matrix concatentation
        // E.g. modelToView = modelToWorld * worldToView
        Transform3D result;
        if(m_orientationIsIdentity)
        {
            // Optimise a common case
            result.m[0] = rhs.m[0];
            result.m[1] = rhs.m[1];
            result.m[2] = rhs.m[2];
        }
        else
        {
            rhs.rotateVector(result.m[0], m[0]);
            rhs.rotateVector(result.m[1], m[1]);
            rhs.rotateVector(result.m[2], m[2]);
        }
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
        const TranslationVectorType negTrans(-t.x, -t.y, -t.z);
        outTransform.t.x = (negTrans.x * outTransform.m[0][0]) + (negTrans.y * outTransform.m[1][0]) + (negTrans.z * outTransform.m[2][0]);
        outTransform.t.y = (negTrans.x * outTransform.m[0][1]) + (negTrans.y * outTransform.m[1][1]) + (negTrans.z * outTransform.m[2][1]);
        outTransform.t.z = (negTrans.x * outTransform.m[0][2]) + (negTrans.y * outTransform.m[1][2]) + (negTrans.z * outTransform.m[2][2]);
    }

    void markAsManuallyManipulated() {m_orientationIsIdentity = false;}
    
private:
    bool m_orientationIsIdentity;
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
