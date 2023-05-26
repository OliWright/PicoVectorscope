// One dimensional lookup table.
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
// COPYING.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.

// For an example of usage, see sintable.h

#pragma once
#include "fixedpoint.h"

// Non-templatised base class, so most of the code isn't duplicated
// with template instantiation.
class LookUpTableBase
{
public:
    typedef FixedPoint<4, 14, uint32_t, uint32_t, false> Index;

protected:
    LookUpTableBase(uint32_t valueSize, uint32_t numValues, bool wrapped, Index endIndex);

    // The base class doesn't know how to interpolate between the value type.
    // This helper will output pointers to the two values that need interpolating
    // between, and it will return the fractional interpolation value that should
    // be used.
    Index LookUp(Index lookupIndex, void*& outA, void*& outB) const;

    void*                                         m_table;
    uint32_t                                      m_numValues;
    uint32_t                                      m_valueSize;
    Index                                         m_endIndex;
    FixedPoint<14, 12, uint32_t, uint32_t, false> m_lookUpToIndex;
};

// Lightweight template wrapper to implement a lookup table for
// any value type.
template <typename T>
class LookUpTable : public LookUpTableBase
{
public:
    typedef T ValueType;

    // An inherited class will should fill in the lookup table
    // in its constructor.
    LookUpTable(uint32_t numValues, bool wrapped, Index endIndex)
        : LookUpTableBase(sizeof(ValueType), numValues, wrapped, endIndex)
    {
    }

    // Lookup a value from the table, with interpolation.
    ValueType LookUp(LookUpTableBase::Index index) const
    {
        // Use LookUpTableBase::LookUp to give us the two values that
        // require interpolation
        void*            pA;
        void*            pB;
        Index            interpolation = LookUpTableBase::LookUp(index, pA, pB);
        // Interpolate
        const ValueType& a             = *(const ValueType*)pA;
        const ValueType& b             = *(const ValueType*)pB;
        return ((b - a) * (ValueType)interpolation) + a;
    }
};
