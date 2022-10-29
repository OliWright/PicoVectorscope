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

#include "lookuptable.h"
#include "pico/stdlib.h"
#include <cstdlib>
#include "log.h"

static LogChannel LookupTesting(false);

LookUpTableBase::LookUpTableBase(uint32_t valueSize, uint32_t numValues, bool wrapped, Index endIndex)
: m_table(malloc(numValues * valueSize))
, m_numValues(numValues)
, m_valueSize(valueSize)
, m_endIndex(endIndex)
, m_lookUpToIndex((float) numValues / (float) endIndex)
{}

LookUpTableBase::Index LookUpTableBase::LookUp(Index index, void*& outA, void*& outB) const
{
    Index actualIndex = index * m_lookUpToIndex;
    unsigned int intIndex = ((unsigned int) actualIndex) & (m_numValues - 1);
    unsigned int nextIntIndex = (intIndex + 1) & (m_numValues - 1);
    outA = (uint8_t*) m_table + (intIndex * m_valueSize);
    outB = (uint8_t*) m_table + (nextIntIndex * m_valueSize);
    return actualIndex.frac();
}
