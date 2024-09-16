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

template <typename T>
struct Vector2
{
    typedef T  ScalarType;
    ScalarType x, y;
    Vector2() {}
    constexpr Vector2(ScalarType x, ScalarType y) : x(x), y(y) {}
    template<typename rhsScalarT>
    explicit constexpr Vector2(const Vector2<rhsScalarT>& rhs) : x(rhs.x), y(rhs.y) {}


    constexpr Vector2 operator + (const Vector2& rhs) const
    {
        return Vector2(x + rhs.x, y + rhs.y);
    }

    constexpr Vector2 operator - (const Vector2& rhs) const
    {
        return Vector2(x - rhs.x, y - rhs.y);
    }

    template<typename rhsScalarT>
    constexpr Vector2 operator * (rhsScalarT rhs) const
    {
        return Vector2(x * rhs, y * rhs);
    }

    constexpr Vector2& operator += (const Vector2& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    constexpr Vector2& operator -= (const Vector2& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    template<typename rhsScalarT>
    constexpr Vector2& operator *= (rhsScalarT rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template<typename rhsScalarT>
    constexpr void operator = (const Vector2<T>& rhs)
    {
        x = rhs.x;
        y = rhs.y;
    }

    constexpr Vector2 operator - () const
    {
        return Vector2(-x, -y);
    }


    constexpr const ScalarType& operator[](int idx) const { return (&x)[idx]; }
    constexpr ScalarType& operator[](int idx) { return (&x)[idx]; }
};

template <typename T>
struct Vector3
{
    typedef T  ScalarType;
    ScalarType x, y, z;
    Vector3() {}
    constexpr Vector3(ScalarType x, ScalarType y, ScalarType z) : x(x), y(y), z(z) {}
    template<typename rhsScalarT>
    explicit constexpr Vector3(const Vector3<rhsScalarT>& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {}

    constexpr Vector3 operator + (const Vector3& rhs) const
    {
        return Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    constexpr Vector3 operator - (const Vector3& rhs) const
    {
        return Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    template<typename rhsScalarT>
    constexpr Vector3 operator * (Vector3<rhsScalarT> rhs) const
    {
        return Vector3(x * rhs.x, y * rhs.y, z * rhs.z);
    }

    template<typename rhsScalarT>
    constexpr Vector3 operator * (rhsScalarT rhs) const
    {
        return Vector3(x * rhs, y * rhs, z * rhs);
    }

    constexpr Vector3& operator += (const Vector3& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    constexpr Vector3& operator -= (const Vector3& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    template<typename rhsScalarT>
    constexpr Vector3& operator *= (Vector3<rhsScalarT> rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template<typename rhsScalarT>
    constexpr Vector3& operator *= (rhsScalarT rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template<typename rhsScalarT>
    constexpr void operator = (const Vector3<T>& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
    }

    constexpr Vector3 operator - () const
    {
        return Vector3(-x, -y, -z);
    }

    constexpr const ScalarType& operator[](int idx) const { return (&x)[idx]; }
    constexpr ScalarType& operator[](int idx) { return (&x)[idx]; }
};
