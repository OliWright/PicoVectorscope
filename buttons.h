#pragma once

#include <cstdint>

class Buttons
{
public:
    static void Init();
    static void Update();

    enum class Id
    {
        Left,
        Right,
        Thrust,
        Fire,

        Count
    };

    static bool IsJustPressed(Id id);
    static bool IsHeld(Id id);
    static uint64_t HoldTimeMs(Id id);
};
