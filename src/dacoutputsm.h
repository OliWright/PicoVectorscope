// Static class owning our PIO SM programs
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
#include "dacoutpioconfig.h"

class DacOutputPioSm
{
public:
    static void Init();

    static const DacOutputPioSmConfig& Idle()   { return s_configs[(int) SmID::eIdle]; }
    static const DacOutputPioSmConfig& Vector() { return s_configs[(int) SmID::eVector]; }
    static const DacOutputPioSmConfig& Points() { return s_configs[(int) SmID::ePoints]; }
    static const DacOutputPioSmConfig& Raster() { return s_configs[(int) SmID::eRaster]; }

private:
    enum class SmID
    {
        eIdle,
        eVector,
        ePoints,
        eRaster,

        eCount
    };

    static void configureSm(SmID smID);

private:
    static DacOutputPioSmConfig s_configs[(int)SmID::eCount];
    static bool timerCallback(struct repeating_timer* t);
};
