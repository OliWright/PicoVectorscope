#include "picovectorscope.h"

class TestCard : public Demo
{
public:
    TestCard(int order) : Demo(order) {}

    void Init() override;
    void UpdateAndRender(DisplayList& displayList, float dt) override;
};
static TestCard s_textAndShapes(-1);

static constexpr uint32_t kMaxFragments = 16;
static Fragment s_fragments[kMaxFragments] = {};
static uint32_t s_numFragments = 0;

void TestCard::Init()
{
    FloatTransform2D transform;
    transform.setAsScale(0.05f);
    transform.setTranslation(FloatVector2(0.5f, 0.5f));

    // PushGameShape(displayList, transform, GameShape::eShip, Intensity(2.f));
    s_numFragments = FragmentGameShape(GameShape::eAsteroid0, transform, s_fragments, kMaxFragments);
}

void TestCard::UpdateAndRender(DisplayList& displayList, float dt)
{
    static float phase = 0.f;
    phase += dt * 4;
    if (phase > (kPi * 2.f))
    {
        phase -= kPi * 2.f;
    }

    FloatTransform2D transform;
    FloatVector2 centre(0.5f, 0.5f);

    transform.setAsScale(0.05f);
    transform.translate(FloatVector2(0.1f, 0.9f));
    constexpr uint32_t kCount = 2;
    for(uint32_t i = 0; i < kCount; ++i)
    {
        TextPrint(displayList, transform, "PICO VECTORSCOPE", Intensity(1.f - ((float) i / kCount)), 100);
        transform.translate(FloatVector2(0.01f, 0.01f));
    }

    transform.setAsScale(0.018f);

    for(uint32_t i = 0; i < (uint32_t) GameShape::eCount; ++i)
    {
        transform.setTranslation(FloatVector2(0.1f + (0.8f * i / (float) ((int)GameShape::eCount - 1)), 0.25f));
        PushGameShape(displayList, transform, (GameShape) i, Intensity(1.f));
    }

    constexpr int kRampCount = 64;
    for(int i = 0; i < kRampCount; ++i)
    {
        DisplayListScalar x = DisplayListScalar(0.1f) + (DisplayListScalar(0.8f / (float) kRampCount) * i);
        Intensity intensity = Intensity(1.f / (float) kRampCount) + (Intensity(1.f / (float) kRampCount) * i);
        //intensity = 0.1f;
        displayList.PushVector(x, 0.05f, 0);
        displayList.PushVector(x, 0.15f, intensity);
        displayList.PushPoint(x, 0.16f, intensity);
    }

    // transform.setAsScale(0.05f);
    // transform.setTranslation(FloatVector2(0.5f, 0.5f));
    // PushGameShape(displayList, transform, GameShape::eShip, Intensity(2.f));

    PushFragmentsToDisplayList(displayList, s_fragments, s_numFragments);
}