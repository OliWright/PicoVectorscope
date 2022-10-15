#include "gameshapes.h"
#include "shapes.h"

struct CompactVector
{
    int8_t x;
    int8_t y;
    int8_t onOff;
    int8_t pad;
};

static const CompactVector s_gameShapeVectors[] = 
{
	// Vectors for "Asteroid0"
	{0, 8, 0},
	{8, 16, 1},
	{16, 8, 1},
	{12, 0, 1},
	{16, -8, 1},
	{4, -16, 8},
	{-8, -16, 8},
	{-16, -8, 1},
	{-16, 8, 1},
	{-8, 16, 1},
	{0, 8, 1},
	// Vectors for "Asteroid1"
	{8, 4, 0},
	{16, 8, 1},
	{8, 16, 1},
	{0, 12, 1},
	{-8, 16, 1},
	{-16, 8, 1},
	{-12, 0, 1},
	{-16, -8, 1},
	{-8, -16, 1},
	{-4, -12, 1},
	{8, -16, 8},
	{16, -4, 8},
	{8, 4, 1},
	// Vectors for "Asteroid2"
	{-8, 0, 0},
	{-16, -4, 1},
	{-8, -16, 1},
	{0, -4, 1},
	{0, -16, 1},
	{8, -16, 1},
	{16, -4, 1},
	{16, 4, 1},
	{8, 16, 1},
	{-4, 16, 1},
	{-16, 4, 1},
	{-8, 0, 1},
	// Vectors for "Asteroid3"
	{4, 0, 0},
	{16, 4, 1},
	{16, 8, 6},
	{4, 16, 1},
	{-8, 16, 1},
	{-4, 8, 6},
	{-16, 8, 1},
	{-16, -4, 1},
	{-8, -16, 1},
	{4, -12, 1},
	{8, -16, 6},
	{16, -8, 6},
	{4, 0, 1},
	// Vectors for "Saucer"
	{-8, 4, 0},
	{8, 4, 12},
	{20, -4, 0},
	{-20, -4, 13},
	{-8, -12, 13},
	{8, -12, 12},
	{20, -4, 13},
	{8, 4, 13},
	{4, 12, 12},
	{-4, 12, 12},
	{-8, 4, 12},
	{-20, -4, 13},
	// Vectors for "Ship"
	{0, 16, 12},
	{-8, 24, 11},
	{40, 8, 12},
	{-8, -8, 12},
	{0, 0, 11},
	// Vectors for "Thrust"
	{-16, 8, 12},
	{0, 16, 12},
};

static const uint16_t s_gameShapes[][2] = 
{
	{0, 11}, // Asteroid0
	{11, 13}, // Asteroid1
	{24, 12}, // Asteroid2
	{36, 13}, // Asteroid3
	{49, 12}, // Saucer
	{61, 5}, // Ship
	{66, 2}, // Thrust
};

static const int8_t s_gameShapeOffsets[][2] = 
{
	{0, 0}, // Asteroid0
	{0, 0}, // Asteroid1
	{0, 0}, // Asteroid2
	{0, 0}, // Asteroid3
	{0, 0}, // Saucer
	{-16, -8}, // Ship
	{-16, -8}, // Thrust
};


void PushGameShape(DisplayList& displayList, const FloatTransform2D& transform, GameShape shape, Intensity intensity)
{
    const CompactVector* vector = s_gameShapeVectors + s_gameShapes[(int)shape][0];
    const CompactVector* end = vector + s_gameShapes[(int)shape][1];

    constexpr uint32_t kMaxPoints = 32;
    ShapeVector2 points[kMaxPoints];

    const int8_t* offset = s_gameShapeOffsets[(int) shape];
    points[0].x = (float) offset[0] * 0.125f;
    points[0].y = (float) offset[1] * 0.125f;
    uint32_t numPoints = 1;
    while (vector != end)
    {
        if (vector->onOff == 0)
        {
            // Flush the current shape
            if (numPoints > 1)
            {
                PushShapeToDisplayList(displayList, points, numPoints, intensity, false, transform);
            }
            numPoints = 0;
        }
        points[numPoints].x = ((float)(vector->x + offset[0])) * 0.125f;
        points[numPoints].y = ((float)(vector->y + offset[1])) * 0.125f;
        ++numPoints;
        ++vector;
    }
    PushShapeToDisplayList(displayList, points, numPoints, intensity, false, transform);
}

uint32_t FragmentGameShape(GameShape shape,
                           const FloatTransform2D& transform,
                           Fragment* outFragments,
                           uint32_t outFragmentsCapacity)
{
    const CompactVector* vector = s_gameShapeVectors + s_gameShapes[(int)shape][0];
    const CompactVector* end = vector + s_gameShapes[(int)shape][1];

    constexpr uint32_t kMaxPoints = 32;
    ShapeVector2 points[kMaxPoints];

	uint32_t numFragments = 0;

    const int8_t* offset = s_gameShapeOffsets[(int) shape];
    points[0].x = (float) offset[0] * 0.125f;
    points[0].y = (float) offset[1] * 0.125f;
    uint32_t numPoints = 1;
    while (vector != end)
    {
        if (vector->onOff == 0)
        {
            // Flush the current shape
            if (numPoints > 1)
            {
				numFragments += FragmentShape(points, numPoints, false, transform, outFragments + numFragments, outFragmentsCapacity - numFragments);
            }
			if(numFragments >= outFragmentsCapacity)
			{
				break;
			}
            numPoints = 0;
        }
        points[numPoints].x = ((float)(vector->x + offset[0])) * 0.125f;
        points[numPoints].y = ((float)(vector->y + offset[1])) * 0.125f;
        ++numPoints;
        ++vector;
    }
	numFragments += FragmentShape(points, numPoints, false, transform, outFragments + numFragments, outFragmentsCapacity - numFragments);
	return numFragments;
}
