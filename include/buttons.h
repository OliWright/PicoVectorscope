// Static class for handling buttons attached to GPIO inputs
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


// This is a work-in-progress.  Currently it is using hard-coded
// GPIO pins and button definitions.
//
// TODO: Make it configurable.

#pragma once
#include <cstdint>

class Buttons
{
public:
    // Button identifiers.
    // Need to replace this with something less hard-coded
    enum class Id
    {
        Left,
        Right,
        Thrust,
        Fire,

        Count
    };

    // Returns true for the first frame that a button is pressed.
    static bool IsJustPressed(Id id);
    // Clear the JustPressed state so that nobody else consumes it.
    static void ClearJustPressed(Id id);
    // Returns true if a button is pressed right now.
    static bool IsHeld(Id id);
    // How long has a button being held for.
    // Returns 0 if the button is not pressed.
    static uint64_t HoldTimeMs(Id id);
    // Fake a button press.  Can be used to for demo playback.
    static void FakePress(Id id, bool pressed);

public:
    // The picovectorscope framework will call this on startup
    static void Init();
    // The picovectorscope framework will call this each frame
    static void Update();
};
