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
