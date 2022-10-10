#pragma once

#include "displaylist.h"

class Demo
{
public:
    Demo(int order = 0);

    virtual void Init() {}
    virtual void UpdateAndRender(DisplayList&, float dt) {}

private:
    int m_order;
};
