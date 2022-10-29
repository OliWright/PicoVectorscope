#include "picovectorscope.h"

static constexpr uint32_t kWidth = 200;
static constexpr uint32_t kHeight = 150;

static uint8_t s_bitmapDisplay[kHeight][kWidth >> 3] = {};
static DisplayList::RasterDisplay s_rasterDisplay;

static const uint16_t s_invaderSprites[3][2][8] = {
    {
        {
            0b0000001111000000,
            0b0001111111111000,
            0b0011111111111100,
            0b0011100110011100,
            0b0011111111111100,
            0b0000011001100000,
            0b0000110110110000,
            0b0011000000001100,
        },
        {
            0b0000001111000000,
            0b0001111111111000,
            0b0011111111111100,
            0b0011100110011100,
            0b0011111111111100,
            0b0000011001100000,
            0b0000110110110000,
            0b0000011001100000,
        },
    },
    {
        {
            0b0000010000010000,
            0b0001001000100100,
            0b0001011111110100,
            0b0001110111011100,
            0b0001111111111100,
            0b0000111111111000,
            0b0000010000010000,
            0b0000100000001000,
        },
        {
            0b0000010000010000,
            0b0000001000100000,
            0b0000011111110000,
            0b0001110111011100,
            0b0001111111111100,
            0b0001011111110100,
            0b0001010000010100,
            0b0000001101100000,
        },
    },
    {
        {
            0b0000000110000000,
            0b0000001111000000,
            0b0000011111100000,
            0b0000110110110000,
            0b0000111111110000,
            0b0000001001000000,
            0b0000010110100000,
            0b0000101001010000,
        },
        {
            0b0000000110000000,
            0b0000001111000000,
            0b0000011111100000,
            0b0000110110110000,
            0b0000111111110000,
            0b0000010110100000,
            0b0000100000010000,
            0b0000010000100000,
        },
    },
};

static void draw16x8BitSprite(const uint16_t* sprite, uint32_t x, uint32_t y)
{
    for(uint32_t i = 0; i < 8; ++i)
    {
        // Manually unrolled to write a 16-bit sprite row
        uint8_t* pixels = s_bitmapDisplay[y + i] + (x >> 3);
        uint16_t spriteRow = sprite[i];

        // Setup to write whatever pixels we can to the first byte
        uint32_t shift = x & 7;
        int32_t spriteShift = 16 - 8 + (int32_t) shift;
        uint8_t maskToWrite = 0xff >> shift;
        
        uint8_t pixelBlock = *pixels;

        // Clear the pixels we're going to write
        pixelBlock &= ~maskToWrite;
        pixelBlock |= (uint8_t) (spriteRow >> spriteShift);
        *pixels++ = pixelBlock;
        pixelBlock = *pixels;

        // The second (middle) byte is always written in its entirity
        // No masking required
        *pixels++ = spriteRow >> (spriteShift - 8);

        spriteShift -= 16;
        if(spriteShift == -8)
        {
            continue;
        }

        // The third byte needs masking
        pixelBlock = *pixels;
        maskToWrite = 0xff << -spriteShift;
        pixelBlock &= ~maskToWrite;
        pixelBlock |= (uint8_t) (spriteRow << -spriteShift);
        *pixels = pixelBlock;
    }
}

static void clearScreen()
{
    uint32_t *pixels = (uint32_t*) s_bitmapDisplay[0];
    const uint32_t* end = pixels + ((kHeight * (kWidth >> 3)) >> 2);
    while(pixels != end)
    {
        *pixels++ = 0;
    }
}

struct Invader
{
    uint16_t x;
    uint16_t y;
    uint8_t sprite;
    bool alive;
};

static constexpr uint32_t kNumInvadersX = 11;
static constexpr uint32_t kNumInvadersY = 5;
static constexpr uint32_t kInvaderSpacingX = 18;
static constexpr uint32_t kInvaderSpacingY = 16;

static Invader s_invaders[kNumInvadersY][kNumInvadersX];

void ResetInvaders(uint32_t top = 20)
{
    clearScreen();
    uint32_t left = 20;
    constexpr uint8_t rowSprites[] = {2,1,1,0,0};
    static_assert(count_of(rowSprites) == kNumInvadersY, "");
    for(uint32_t y = 0; y < kNumInvadersY; ++y)
    {
        for(uint32_t x = 0; x < kNumInvadersX; ++x)
        {
            Invader& invader = s_invaders[y][x];
            invader.x = left + (x * kInvaderSpacingX);
            invader.y = top + (y * kInvaderSpacingY);
            invader.sprite = rowSprites[y];
            invader.alive = true;
            draw16x8BitSprite(s_invaderSprites[invader.sprite][0], invader.x, invader.y);
        }
    }
}

class InvadersFromSpace : public Demo
{
public:
    InvadersFromSpace() : Demo() {}
    void Init();
    void UpdateAndRender(DisplayList& displayList, float dt);
};
static InvadersFromSpace s_invadersFromSpace;

static const uint8_t* scanlineCallback(uint32_t scanline, void*)
{
    return s_bitmapDisplay[scanline];
}

static void checkerboard()
{
    static uint32_t offset = 0;
    offset  = (offset + 1) & 0xf;
    for(uint32_t y = 0; y < kHeight; ++y)
    {
        uint32_t gy = y >> 3;
        uint8_t pixelBlock = 0;
        for(uint32_t x = 0; x < kWidth; ++x)
        {
            uint32_t gx = (x + offset) >> 3;
            pixelBlock |= ((gx + gy) & 1) << (7 - (x & 7));
            if((x & 7) == 7)
            {
                s_bitmapDisplay[y][x >> 3] = pixelBlock;
                pixelBlock = 0;
            }
        }
    }
}

static void stripes()
{
    for(uint32_t y = 0; y < kHeight; ++y)
    {
        for(uint32_t x = 0; x < (kWidth >> 3); ++x)
        {
            if(x & 1)
            {
                s_bitmapDisplay[y][x] = 0xff;
            }
            else
            {
                s_bitmapDisplay[y][x] = 0;
            }
        }
    }
}

void InvadersFromSpace::Init()
{
    s_rasterDisplay.width = kWidth;
    s_rasterDisplay.height = kHeight;
    s_rasterDisplay.mode = DisplayList::RasterDisplay::Mode::e1Bit;
    s_rasterDisplay.scanlineCallback = scanlineCallback;
    //s_rasterDisplay.topLeft = DisplayListVector2(0.25f, 1.f);
    //s_rasterDisplay.bottomRight = DisplayListVector2(0.75f, 0.f);

    ResetInvaders();
}

void InvadersFromSpace::UpdateAndRender(DisplayList& displayList, float dt)
{
    displayList.PushRasterDisplay(s_rasterDisplay);

    FixedTransform2D transform;
    transform.setAsScale(0.1f);
    transform.translate(FixedTransform2D::Vector2Type(0.1f, 0.9f));
    //TextPrint(displayList, transform, "RASTER\nVECTOR", 1.f);

    //checkerboard();
    //stripes();
}
