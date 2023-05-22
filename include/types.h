// Dumping ground for general purpose types and constants.
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

constexpr float kPi = 3.14159265358979323846f;

template <typename T>
struct Vector2
{
    typedef T  ScalarType;
    ScalarType x, y;
    Vector2() {}
    Vector2(ScalarType x, ScalarType y) : x(x), y(y) {}

    const ScalarType& operator[](int idx) const { return (&x)[idx]; }
    ScalarType& operator[](int idx) { return (&x)[idx]; }
};

template <typename T>
struct Vector3
{
    typedef T  ScalarType;
    ScalarType x, y, z;
    Vector3() {}
    Vector3(ScalarType x, ScalarType y, ScalarType z) : x(x), y(y), z(z) {}

    Vector3 operator + (const Vector3& rhs) const
    {
        return Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vector3 operator - (const Vector3& rhs) const
    {
        return Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    template<typename rhsScalarT>
    Vector3 operator * (rhsScalarT rhs) const
    {
        return Vector3(x * rhs, y * rhs, z * rhs);
    }

    Vector3& operator += (const Vector3& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    Vector3& operator -= (const Vector3& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    template<typename rhsScalarT>
    Vector3& operator *= (rhsScalarT rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    const ScalarType& operator[](int idx) const { return (&x)[idx]; }
    ScalarType& operator[](int idx) { return (&x)[idx]; }
};
