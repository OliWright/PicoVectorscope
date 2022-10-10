#include "shapes.h"
#include "transform2d.h"

static const ShapeVector2 points[] = {
    ShapeVector2(-1, -1),
    ShapeVector2(1, -1),
    ShapeVector2(1, 1),
    ShapeVector2(-1, 1),
};
static const uint32_t kNumPoints = sizeof(points) / sizeof(points[0]);

void coolDemo0(DisplayList& displayList, float dt)
{
    static float phase = 0.f;
    phase += dt * 2;
    if (phase > (kPi * 2.f))
    {
        phase -= kPi * 2.f;
    }
    float intensity = sinf(phase) * 0.5f + 0.5f;
    intensity = 0.5f;

    constexpr uint32_t kCount = 15;

    static float brightCycle = 0.f;
    brightCycle += (dt * 20.f);
    if(brightCycle >= (float) kCount)
    {
        brightCycle -= (float) kCount;
    }
    uint32_t brightIdx = (uint32_t) brightCycle;

    PushShapeToDisplayList(displayList, points, kNumPoints, 1.f, true /*closed*/);

    FloatTransform2D transform;
    FloatVector2 centre(0.5f, 0.5f);
    transform.setAsRotation(phase/* + (float)i * 0.25f*/, centre);
    transform *= 0.35f;
    FloatTransform2D localTransform;
    for (uint32_t i = 0; i < kCount; ++i)
    {
        localTransform.setAsRotation(phase - (float)i * 0.02f, FloatVector2(0.f, 0.f));
        localTransform *= 0.35f;
        float scale = 1.f - ((float) i / kCount);
        localTransform *= scale;
        localTransform.setTranslation(FloatVector2(0.5f, 0.5f));
        intensity = 0.5f;//1.f - ((float) i / kCount);
        if(i == brightIdx)
        {
            intensity = 2.f;
        }
        PushShapeToDisplayList(displayList, points, kNumPoints, intensity, true /*closed*/, localTransform);
    }
}
