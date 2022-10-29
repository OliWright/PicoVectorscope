#include "picovectorscope.h"
#include "extras/tilemap.h"

static constexpr uint32_t kWidth = 40;
static constexpr uint32_t kHeight = 25;

static uint8_t s_textDisplay[kHeight][kWidth] = {};

static const uint8_t* rowCallback(int32_t row)
{
    return s_textDisplay[row];
}

static void print(uint32_t x, uint32_t y, const char* message)
{
    const char* src = message;
    char* dst = (char*) s_textDisplay[y] + x;
    char* end = (char*) s_textDisplay[0] + (kWidth * kHeight);
    while(*src != 0)
    {
        *(dst++) = *(src++);
        if(dst == end)
        {
            dst = (char*) s_textDisplay[0];
        }
    }
}

static TileMap s_tileMap(TileMap::Mode::eText, 40, 25, rowCallback);

class TextMode : public Demo
{
public:
    TextMode() : Demo(-1) {}
    void Init();
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static TextMode s_textMode;

static void checkerboard()
{
    static uint32_t offset = 0;
    offset  = (offset + 1) & 0xf;
    for(uint32_t y = 0; y < kHeight; ++y)
    {
        for(uint32_t x = 0; x < kWidth; ++x)
        {
            s_textDisplay[y][x] = ((x + y) & 1) ? 'X' : 'O';
        }
    }
}

void TextMode::Init()
{
    //checkerboard();
    //print(0, 0, "Hello World");
#if 1
    for(uint i = 0; i < 10; ++i)
    {
        print(SimpleRand() % kWidth, SimpleRand() % kHeight, "Hello World");
    }
#endif
}

void TextMode::UpdateAndRender(DisplayList& displayList, float dt)
{
    s_tileMap.PushToDisplayList(displayList);
}
