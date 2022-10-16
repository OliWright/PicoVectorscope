#pragma once

class Serial
{
public:
    static void Init();

    static char GetLastCharIn() { return s_lastCharIn; }
    static void ClearLastCharIn() { s_lastCharIn = 0; }

private:
    static bool timerCallback(struct repeating_timer *t);

    static volatile char s_lastCharIn;
};
