#include "text.h"
#include "gameshapes.h"
#include "transform2d.h"

void coolDemo2(DisplayList& displayList, float dt)
{
    static float phase = 0.f;
    phase += dt * 4;
    if (phase > (kPi * 2.f))
    {
        phase -= kPi * 2.f;
    }

    FloatTransform2D transform;
    FloatVector2 centre(0.5f, 0.5f);

    transform.setAsScale(0.15f);
    transform.translate(FloatVector2(0.1f, 0.9f));
    //float intensity = 1.f;
    //AstPrint(displayList, transform, "HELLO\nWORLD", DisplayListScalar(10.f));
    constexpr uint32_t kCount = 4;
    for(uint32_t i = 0; i < kCount; ++i)
    {
        TextPrint(displayList, transform, "HELLO\n\nWORLD", Intensity(1.f - ((float) i / kCount)));
        transform.translate(FloatVector2(0.02f, 0.02f));
    }

    transform.setAsScale(0.018f);

    for(uint32_t i = 0; i < (uint32_t) GameShape::eCount; ++i)
    {
        transform.setTranslation(FloatVector2(0.1f + (0.8f * i / (float) ((int)GameShape::eCount - 1)), 0.25f));
        PushGameShape(displayList, transform, (GameShape) i, Intensity(1.f));
    }

    // transform.setAsScale(0.05f);
    // transform.setTranslation(FloatVector2(0.5f, 0.5f));
    // PushGameShape(displayList, transform, GameShape::eShip, Intensity(2.f));
}
