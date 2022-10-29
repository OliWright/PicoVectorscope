#include "picovectorscope.h"

#include "rickroll.hpp"

static constexpr uint32_t kWidth = 300;
static constexpr uint32_t kHeight = 200;

static uint8_t s_bitmapDisplay[kHeight][kWidth] = {};
static DisplayList::RasterDisplay s_rasterDisplay;

class Rickroll : public Demo
{
public:
    Rickroll() : Demo() {}
    void Init();
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static Rickroll s_rickroll;

static const uint8_t* scanlineCallback(uint32_t scanline, void*)
{
    return s_bitmapDisplay[scanline];
    //return s_image[scanline];
}

static void checkerboard()
{
    static uint32_t offset = 0;
    offset  = (offset + 1) & 0xf;
    for(uint32_t y = 0; y < kHeight; ++y)
    {
        uint32_t gy = y >> 3;
        uint8_t* pixel = s_bitmapDisplay[y];
        for(uint32_t x = 0; x < kWidth; ++x)
        {
            uint32_t gx = (x + offset) >> 3;
            *pixel = ((gx + gy) & 1) ? 255 : 0;
            ++pixel;
        }
    }
}

static void circles()
{
    typedef FixedPoint<8,8,int32_t,int32_t> Number;
    static Number offset = 0.f;
    offset += 0.1f;
    if(offset > (kPi * 2.f))
    {
        offset -= kPi * 2.f;
    }
    for(uint32_t y = 0; y < kHeight; ++y)
    {
        for(uint32_t x = 0; x < kWidth; ++x)
        {
            Number dx((int) (x - (kWidth >> 1)));
            Number dy((int) (y - (kHeight >> 1)));
            dx *= (1.f / 8.f);
            dy *= (1.f / 8.f);
            Number dist = ((dx * dx) + (dy * dy)).sqrt();
            Number s = SinTable::LookUp(dist + offset);
            s = (s + 1.f) * 0.5f;
            uint32_t pixel = (s * 255.f).getIntegerPart();
            s_bitmapDisplay[y][x] = (uint8_t) pixel;
        }
    }
}

static void decompressVideoFrame(uint32_t frameIdx)
{
    uint8_t* pixel = s_bitmapDisplay[0];
    const uint8_t* end = pixel + (kWidth * kHeight);
    uint8_t* rle = ((uint8_t*) s_rawData) + s_frameStartOffsets[frameIdx];

    if(frameIdx == 0)
    {
        // Whole frame
        while(pixel < end)
        {
            const uint8_t pixelValue = *rle & 0xf;
            const uint8_t* spanEnd = pixel + ((*rle >> 4) & 0xf) + 1;
            ++rle;
            while(pixel != spanEnd)
            {
                *pixel = pixelValue;
                ++pixel;
            }
        }
    }
    else
    {
        // Delta frame
        while(pixel < end)
        {
            uint32_t count = *rle & 0x7f;
            if((*rle & 0x80) != 0)
            {
                // Skip some pixels because they're unchanged
                ++rle;
                pixel += count;
            }
            else
            {
                // Do some RLE bytes
                ++rle;
                for(uint32_t i = 0; i < count; ++i)
                {
                    const uint8_t pixelValue = *rle & 0xf;
                    const uint8_t* spanEnd = pixel + ((*rle >> 4) & 0xf) + 1;
                    ++rle;
                    while(pixel != spanEnd)
                    {
                        *pixel = pixelValue;
                        ++pixel;
                    }
                }
            }
        }
    }
}

void Rickroll::Init()
{
    s_rasterDisplay.width = kWidth;
    s_rasterDisplay.height = kHeight;
    s_rasterDisplay.mode = DisplayList::RasterDisplay::Mode::e4BitLinear;
    s_rasterDisplay.scanlineCallback = scanlineCallback;
}

void Rickroll::UpdateAndRender(DisplayList& displayList, float dt)
{
    displayList.PushRasterDisplay(s_rasterDisplay);

    FixedTransform2D transform;
    transform.setAsScale(0.1f);
    transform.translate(FixedTransform2D::Vector2Type(0.1f, 0.9f));
    //TextPrint(displayList, transform, "RASTER\nVECTOR", 1.f);

    //checkerboard();
    //circles();
    static uint32_t frameIdx = 0;
    static uint32_t delay = 0;
    if(++delay == 4)
    {
        delay = 0;
        decompressVideoFrame(frameIdx);
        if(++frameIdx == kNumFrames)
        {
            frameIdx = 0;
        }
    }
}
