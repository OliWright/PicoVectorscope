#include "text.h"

#include "shapes.h"


struct CompactVector
{
    int8_t x;
    int8_t y;
    int8_t onOff;
    int8_t pad;
};

static const CompactVector s_characterVectors[] = {
    // Vectors for "A"
    { 0, 4, 1 },
    { 2, 6, 1 },
    { 4, 4, 1 },
    { 4, 0, 1 },
    { 0, 2, 0 },
    { 4, 2, 1 },
    // Vectors for "B"
    { 0, 6, 1 },
    { 3, 6, 1 },
    { 4, 5, 1 },
    { 4, 4, 1 },
    { 3, 3, 1 },
    { 0, 3, 1 },
    { 3, 3, 0 },
    { 4, 2, 1 },
    { 4, 1, 1 },
    { 3, 0, 1 },
    { 0, 0, 1 },
    // Vectors for "C"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 0, 0, 0 },
    { 4, 0, 1 },
    // Vectors for "D"
    { 0, 6, 1 },
    { 2, 6, 1 },
    { 4, 4, 1 },
    { 4, 2, 1 },
    { 2, 0, 1 },
    { 0, 0, 1 },
    // Vectors for "E"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 3, 3, 0 },
    { 0, 3, 1 },
    { 0, 0, 0 },
    { 4, 0, 1 },
    // Vectors for "F"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 3, 3, 0 },
    { 0, 3, 1 },
    { 0, 0, 0 },
    // Vectors for "G"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 4, 4, 1 },
    { 2, 2, 0 },
    { 4, 2, 1 },
    { 4, 0, 1 },
    { 0, 0, 1 },
    // Vectors for "H"
    { 0, 6, 1 },
    { 0, 3, 0 },
    { 4, 3, 1 },
    { 4, 6, 0 },
    { 4, 0, 1 },
    // Vectors for "I"
    { 4, 0, 1 },
    { 2, 0, 0 },
    { 2, 6, 1 },
    { 4, 6, 0 },
    { 0, 6, 1 },
    // Vectors for "J"
    { 0, 2, 0 },
    { 2, 0, 1 },
    { 4, 0, 1 },
    { 4, 6, 1 },
    // Vectors for "K"
    { 0, 6, 1 },
    { 3, 6, 0 },
    { 0, 3, 1 },
    { 3, 0, 1 },
    // Vectors for "L"
    { 0, 6, 0 },
    { 0, 0, 1 },
    { 4, 0, 1 },
    // Vectors for "M"
    { 0, 6, 1 },
    { 2, 4, 1 },
    { 4, 6, 1 },
    { 4, 0, 1 },
    // Vectors for "N"
    { 0, 6, 1 },
    { 4, 0, 1 },
    { 4, 6, 1 },
    // Vectors for "O"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 4, 0, 1 },
    { 0, 0, 1 },
    // Vectors for "P"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 4, 3, 1 },
    { 0, 3, 1 },
    { 3, 0, 0 },
    // Vectors for "Q"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 4, 2, 1 },
    { 2, 0, 1 },
    { 0, 0, 1 },
    { 2, 2, 0 },
    { 4, 0, 1 },
    // Vectors for "R"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 4, 3, 1 },
    { 0, 3, 1 },
    { 1, 3, 0 },
    { 4, 0, 1 },
    // Vectors for "S"
    { 4, 0, 1 },
    { 4, 3, 1 },
    { 0, 3, 1 },
    { 0, 6, 1 },
    { 4, 6, 1 },
    // Vectors for "T"
    { 2, 0, 0 },
    { 2, 6, 1 },
    { 0, 6, 0 },
    { 4, 6, 1 },
    // Vectors for "U"
    { 0, 6, 0 },
    { 0, 0, 1 },
    { 4, 0, 1 },
    { 4, 6, 1 },
    // Vectors for "V"
    { 0, 6, 0 },
    { 2, 0, 1 },
    { 4, 6, 1 },
    // Vectors for "W"
    { 0, 6, 0 },
    { 0, 0, 1 },
    { 2, 2, 1 },
    { 4, 0, 1 },
    { 4, 6, 1 },
    // Vectors for "X"
    { 4, 6, 1 },
    { 0, 6, 0 },
    { 4, 0, 1 },
    // Vectors for "Y"
    { 2, 0, 0 },
    { 2, 4, 1 },
    { 0, 6, 1 },
    { 4, 6, 0 },
    { 2, 4, 1 },
    // Vectors for "Z"
    { 0, 6, 0 },
    { 4, 6, 1 },
    { 0, 0, 1 },
    { 4, 0, 1 },
    // Vectors for " "
    // Vectors for "0"
    { 0, 6, 1 },
    { 4, 6, 1 },
    { 4, 0, 1 },
    { 0, 0, 1 },
    // Vectors for "1"
    { 2, 0, 0 },
    { 2, 6, 1 },
    // Vectors for "2"
    { 0, 6, 0 },
    { 4, 6, 1 },
    { 4, 3, 1 },
    { 0, 3, 1 },
    { 0, 0, 1 },
    { 4, 0, 1 },
    // Vectors for "3"
    { 4, 0, 1 },
    { 4, 6, 1 },
    { 0, 6, 1 },
    { 0, 3, 0 },
    { 4, 3, 1 },
    // Vectors for "4"
    { 0, 6, 0 },
    { 0, 3, 1 },
    { 4, 3, 1 },
    { 4, 6, 0 },
    { 4, 0, 1 },
    // Vectors for "5"
    { 4, 0, 1 },
    { 4, 3, 1 },
    { 0, 3, 1 },
    { 0, 6, 1 },
    { 4, 6, 1 },
    // Vectors for "6"
    { 0, 3, 0 },
    { 4, 3, 1 },
    { 4, 0, 1 },
    { 0, 0, 1 },
    { 0, 6, 1 },
    // Vectors for "7"
    { 0, 6, 0 },
    { 4, 6, 1 },
    { 4, 0, 1 },
    // Vectors for "8"
    { 4, 0, 1 },
    { 4, 6, 1 },
    { 0, 6, 1 },
    { 0, 0, 1 },
    { 0, 3, 0 },
    { 4, 3, 1 },
    // Vectors for "9"
    { 4, 0, 0 },
    { 4, 6, 1 },
    { 0, 6, 1 },
    { 0, 3, 1 },
    { 4, 3, 1 },
};

static const uint16_t s_characters[][2] = {
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 128, 4 }, // 0
    { 132, 2 }, // 1
    { 134, 6 }, // 2
    { 140, 5 }, // 3
    { 145, 5 }, // 4
    { 150, 5 }, // 5
    { 155, 5 }, // 6
    { 160, 3 }, // 7
    { 163, 6 }, // 8
    { 169, 5 }, // 9
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 6 }, // A
    { 6, 11 }, // B
    { 17, 4 }, // C
    { 21, 6 }, // D
    { 27, 6 }, // E
    { 33, 5 }, // F
    { 38, 7 }, // G
    { 45, 5 }, // H
    { 50, 5 }, // I
    { 55, 4 }, // J
    { 59, 4 }, // K
    { 63, 3 }, // L
    { 66, 4 }, // M
    { 70, 3 }, // N
    { 73, 4 }, // O
    { 77, 5 }, // P
    { 82, 7 }, // Q
    { 89, 6 }, // R
    { 95, 5 }, // S
    { 100, 4 }, // T
    { 104, 4 }, // U
    { 108, 3 }, // V
    { 111, 5 }, // W
    { 116, 3 }, // X
    { 119, 5 }, // Y
    { 124, 4 }, // Z
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },   { 0, 0 },
    { 0, 0 },   { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
};

static int32_t calcXOffset(const char* message, bool centre)
{
    if(!centre)
    {
        return 0;
    }
    int32_t x_offset = 0;
    for (const uint8_t* chr = (uint8_t*)message; (*chr != 0) && (*chr != '\n'); ++chr)
    {
        x_offset -= 3;
    }

    return x_offset;
}

static bool processChar(const uint8_t* chr, int32_t& x_offset, int32_t& y_offset, const CompactVector*& vector, const CompactVector*& end, bool centre)
{
    uint32_t ascii_code = *chr;
    if (ascii_code == '\n')
    {
        x_offset = calcXOffset((const char*)chr+1, centre);
        y_offset -= 10;
        return false;
    }
    if (ascii_code == ' ')
    {
        x_offset += 6;
        return false;
    }
    vector = s_characterVectors + s_characters[ascii_code][0];
    end = vector + s_characters[ascii_code][1];
    return vector != end;
}


void TextPrint(DisplayList& displayList, const FixedTransform2D& transform, const char* message, Intensity intensity, BurnLength burnLength, bool centre)
{
    constexpr uint32_t kMaxPoints = 8;
    ShapeVector2 points[kMaxPoints];

    int32_t x_offset = calcXOffset(message, centre);
    int32_t y_offset = -7;
    for (const uint8_t* chr = (uint8_t*)message; *chr != 0; ++chr)
    {
        const CompactVector* vector;
        const CompactVector* end;
        if(!processChar(chr, x_offset, y_offset, vector, end, centre))
        {
            continue;
        }
        points[0].x = ((float)x_offset) * 0.166f;
        points[0].y = ((float)y_offset) * 0.166f;
        uint numPoints = 1;
        while (vector != end)
        {
            if (vector->onOff == 0)
            {
                // Flush the current shape
                if (numPoints > 1)
                {
                    PushShapeToDisplayList(displayList, points, numPoints, intensity, false, transform, burnLength);
                    burnLength -= BurnLength(numPoints - 1);
                    if(burnLength < 0)
                    {
                        return;
                    }
                }
                numPoints = 0;
            }
            points[numPoints].x = ((float)(x_offset + vector->x)) * 0.166f;
            points[numPoints].y = ((float)(y_offset + vector->y)) * 0.166f;
            ++numPoints;
            ++vector;
        }
        PushShapeToDisplayList(displayList, points, numPoints, intensity, false, transform, burnLength);
        burnLength -= BurnLength(numPoints - 1);
        if(burnLength < 0)
        {
            return;
        }
        x_offset += 6;
    }
}

BurnLength CalcBurnLength(const char* message)
{
    BurnLength burnLength = 0;
    for (const uint8_t* chr = (uint8_t*)message; *chr != 0; ++chr)
    {
        uint32_t ascii_code = *chr;
        const CompactVector* vector = s_characterVectors + s_characters[ascii_code][0];
        const CompactVector* end = vector + s_characters[ascii_code][1];
        while (vector != end)
        {
            if (vector->onOff != 0)
            {
                burnLength += 1;
            }
            ++vector;
        }
    }
    return burnLength + 3;
}

void CalcTextTransform(const DisplayListVector2& pos, const DisplayListScalar& scale, FixedTransform2D& outTranform)
{
    outTranform.setAsScale(scale);
    outTranform.setTranslation(FixedTransform2D::Vector2Type(pos.x, pos.y));
}

uint32_t FragmentText(const char* message,
                      const FixedTransform2D& transform,
                      Fragment* outFragments,
                      uint32_t outFragmentsCapacity,
                      bool centre)
{
    constexpr uint32_t kMaxPoints = 8;
    ShapeVector2 points[kMaxPoints];
    uint32_t numFragments = 0;

    int32_t x_offset = calcXOffset(message, centre);
    int32_t y_offset = -7;
    for (const uint8_t* chr = (uint8_t*)message; *chr != 0; ++chr)
    {
        const CompactVector* vector;
        const CompactVector* end;
        if(!processChar(chr, x_offset, y_offset, vector, end, centre))
        {
            continue;
        }
        points[0].x = ((float)x_offset) * 0.166f;
        points[0].y = ((float)y_offset) * 0.166f;
        uint numPoints = 1;
        while (vector != end)
        {
            if (vector->onOff == 0)
            {
                // Flush the current shape
                if (numPoints > 1)
                {
                    numFragments += FragmentShape(points, numPoints, false, transform, outFragments + numFragments, outFragmentsCapacity - numFragments);
                }
                numPoints = 0;
                if(numFragments >= outFragmentsCapacity)
                {
                    break;
                }
            }
            points[numPoints].x = ((float)(x_offset + vector->x)) * 0.166f;
            points[numPoints].y = ((float)(y_offset + vector->y)) * 0.166f;
            ++numPoints;
            ++vector;
        }
        numFragments += FragmentShape(points, numPoints, false, transform, outFragments + numFragments, outFragmentsCapacity - numFragments);
        if(numFragments >= outFragmentsCapacity)
        {
            break;
        }
        x_offset += 6;
    }
    return numFragments;
}
