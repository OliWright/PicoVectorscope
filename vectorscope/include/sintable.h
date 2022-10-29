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

// Use the static class methods SinTable::Lookup and SinTable::SinCos

#pragma once
#include "lookuptable.h"
#include "types.h"

typedef FixedPoint<1, 14,int16_t,int32_t> SinTableValue;

class SinTable : public LookUpTable<SinTableValue>
{
public:
    SinTable();

    // Lookup the value of sin(angle)
    // `angle` should be in radians
    static SinTableValue LookUp(Index angle)
    {
        const LookUpTable<SinTableValue>& st = s_sinTable;
        return st.LookUp(angle);
    }

    // Lookup sin(angle) and cos(angle)
    // `angle` should be in radians
    static void SinCos(Index angle, SinTableValue& outS, SinTableValue& outC)
    {
        const LookUpTable<SinTableValue>& st = s_sinTable;
        outS = st.LookUp(angle);
        outC = st.LookUp(angle + Index(kPi/2.f));
    }

private:
    static SinTable s_sinTable;
};
