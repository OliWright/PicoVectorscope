// Internal helper for handling input from stdin
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

#pragma once

class Serial
{
public:
    static void Init();

    static char GetLastCharIn() { return s_lastCharIn; }
    static void ClearLastCharIn() { s_lastCharIn = 0; }

private:
    static bool timerCallback(struct repeating_timer* t);

    static volatile char s_lastCharIn;
};
