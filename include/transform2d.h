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

template <typename OrientationT = float, typename TranslationT = float>
struct Transform2D
{
    typedef OrientationT OrientationType;
    typedef TranslationT TranslationType;
    typedef Vector2<OrientationType> OrientationVectorType;
    typedef Vector2<TranslationType> TranslationVectorType;
    OrientationVectorType m[2];
    TranslationVectorType t;

    void setAsIdentity()
    {
        m[0] = OrientationVectorType(1, 0);
        m[1] = OrientationVectorType(0, 1);
        t = TranslationVectorType(0, 0);
    }

    void setAsRotation(OrientationType s, OrientationType c, const TranslationVectorType& origin = TranslationVectorType(0, 0))
    {
        m[0] = OrientationVectorType(c, s);
        m[1] = OrientationVectorType(-s, c);

        t.x = origin.x - ((origin.x * m[0][0]) + (origin.y * m[1][0]));
        t.y = origin.y - ((origin.x * m[0][1]) + (origin.y * m[1][1]));
    }

    void setAsScale(OrientationType scale)
    {
        m[0] = OrientationVectorType(scale, 0);
        m[1] = OrientationVectorType(0, scale);
        t = TranslationVectorType(0, 0);
    }

    void setTranslation(const TranslationVectorType& translation)
    {
        t = translation;
    }

    void translate(const TranslationVectorType& translation)
    {
        t += translation;
    }

    Transform2D& operator*=(const Transform2D& b)
    {
        const Transform2D a = *this;

        m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]);
        m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]);

        m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]);
        m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[1][1] * b.m[1][1]);

        t.x = (a.t.x * b.m[0][0]) + (a.t.y * b.m[1][0]) + b.t.x;
        t.y = (a.t.x * b.m[0][1]) + (a.t.y * b.m[1][1]) + b.t.y;
        return *this;
    }

    Transform2D& operator*=(OrientationType scale)
    {
        m[0] *= scale;
        m[1] *= scale;
        t *= scale;

        return *this;
    }

    template<typename T>
    void transformVector(Vector2<T>& result, const Vector2<T>& v) const
    {
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]) + t.x;
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]) + t.y;
    }

    template<typename T>
    void rotateVector(Vector2<T>& result, const Vector2<T>& v) const
    {
        result.x = (v.x * m[0][0]) + (v.y * m[1][0]);
        result.y = (v.x * m[0][1]) + (v.y * m[1][1]);
    }

    void orthonormalInvert(Transform2D& outTransform) const
    {
        // Transpose the orientation
        outTransform.m[0][0] = m[0][0];
        outTransform.m[0][1] = m[1][0];
        outTransform.m[1][0] = m[0][1];
        outTransform.m[1][1] = m[1][1];

        // Calculate the inverted translation
        const TranslationVectorType negTrans(-t);
        outTransform.t.x = (negTrans.x * outTransform.m[0][0]) + (negTrans.y * outTransform.m[1][0]);
        outTransform.t.y = (negTrans.x * outTransform.m[0][1]) + (negTrans.y * outTransform.m[1][1]);
    }
};

typedef Transform2D<float, float> FloatTransform2D;
typedef Transform2D<FixedPoint<3,16,int32_t,int32_t,false>,
                    FixedPoint<3,16,int32_t,int32_t,false> > FixedTransform2D;
