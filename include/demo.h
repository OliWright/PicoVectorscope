// Base class for a cool demo or application.
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

// Create an instance of this class to register your demo with
// the picovectorscope framework.
//
// Multiple Demos can be created and cycled through by holding
// Left and Right, and then pressing Fire.
//
// E.g.
//     class SpaceRocksInSpace2 : public Demo
//     {
//     public:
//         SpaceRocksInSpace2() : Demo(-1/*Make this run first*/, 240/*Hz*/) {}
//
//         void Init() override
//         {
//             // Set up all the rocks
//         }
//
//         void UpdateAndRender(DisplayList& displayList, float dt) override
//         {
//             // Pew pew
//         }
//     };


#pragma once
#include "displaylist.h"

class Demo
{
public:
    Demo(int order = 0, int m_targetRefreshRate = 60);

    virtual void Init() {}
    virtual void UpdateAndRender(DisplayList&, float dt) {}

    int GetTargetRefreshRate() const { return m_targetRefreshRate; }

private:
    int m_order;
    int m_targetRefreshRate;
};
