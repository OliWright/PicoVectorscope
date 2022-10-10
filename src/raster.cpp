#include "picovectorscope.h"

#define DIR 0

class Raster : public Demo
{
public:
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static Raster s_raster;

void Raster::UpdateAndRender(DisplayList& displayList, float dt)
{
    static float phase = 0.f;
    phase += dt;
    if (phase > (kPi * 2.f))
    {
        phase -= kPi * 2.f;
    }
    float intensity = sinf(phase) * 0.25f + 0.75f;
    intensity = 0.3f;

    ShapeVector2 line[] = {
        ShapeVector2(0, 0),
        ShapeVector2(0, 0),
    };
    line[1][DIR] = 1.f;
#if 0 // Enable this to draw the first line slightly longer than all the others
    // so that the first line can be used as a trigger
    PushShapeToDisplayList(displayList, line, 2, 1.f, false /*closed*/);
    line[1][DIR] = 0.8f;
#endif
    const uint32_t kNumLines = 240;
    for (uint32_t i = 0; i < kNumLines; ++i)
    {
        float x = (float)i / (float)kNumLines;
        line[0][1 - DIR] = x;
        line[1][1 - DIR] = x;
        PushShapeToDisplayList(displayList, line, 2, intensity, false /*closed*/);
    }
}
