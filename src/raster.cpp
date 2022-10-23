#include "picovectorscope.h"

#define DIR 0

static constexpr uint32_t kWidth = 300;
static constexpr uint32_t kHeight = 200;

static uint8_t s_bitmapDisplay[kHeight][kWidth] = {};
static DisplayList::RasterDisplay s_rasterDisplay;

class Raster : public Demo
{
public:
    Raster() : Demo(-1) {}
    void Init();
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static Raster s_raster;

static const uint8_t* scanlineCallback(uint32_t scanline)
{
    return s_bitmapDisplay[scanline];
}

void Raster::Init()
{
    s_rasterDisplay.width = kWidth;
    s_rasterDisplay.height = kHeight;
    s_rasterDisplay.scanlineCallback = scanlineCallback;
    for(uint32_t y = 0; y < kHeight; ++y)
    {
        uint32_t gy = y >> 3;
        for(uint32_t x = 0; x < kWidth; ++x)
        {
            uint32_t gx = x >> 3;
            s_bitmapDisplay[y][x] = ((gx + gy) & 1) ? 255 : 0;
        }
    }
}

void Raster::UpdateAndRender(DisplayList& displayList, float dt)
{
    displayList.PushRasterDisplay(s_rasterDisplay);

    FixedTransform2D transform;
    transform.setAsScale(0.05f);
    transform.translate(FixedTransform2D::Vector2Type(0.1f, 0.9f));
    TextPrint(displayList, transform, "RASTER", 1.f);


#if 0
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
#endif
}
