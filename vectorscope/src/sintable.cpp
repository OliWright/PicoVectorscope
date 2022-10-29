// Fixed point sin lookup table, with static class lookup helpers.
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

#include "sintable.h"

#include "pico/float.h"


SinTable::SinTable() : LookUpTable<SinTableValue>(1024, true, kPi * 2.f)
{
    SinTableValue* pTable = (SinTableValue*)m_table;
    for (uint32_t i = 0; i < m_numValues; ++i)
    {
        pTable[i] = sinf(kPi * 2.f * i / (float)m_numValues);
    }
}

SinTable SinTable::s_sinTable;
