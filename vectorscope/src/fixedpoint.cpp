#include "fixedpoint.h"
#include "log.h"
#include "sintable.h"

#include <stdio.h>

static LogChannel FixedPointTesting(true);

static uint32_t s_randSeed = 123456789;
uint32_t SimpleRand()
{
    constexpr uint32_t kA = 1103515245;
    constexpr uint32_t kC = 12345;
    s_randSeed = kA * s_randSeed + kC;
    return s_randSeed;
}

typedef FixedPoint<14, int16_t, int32_t, true /*doClamping*/> FixedPoint_S2_14;
typedef FixedPoint<26, int32_t, int32_t, false> FixedPoint_S6_26;
typedef FixedPoint<18, int32_t, int32_t, false> FixedPoint_S14_18;
typedef FixedPoint<11, int32_t, int32_t, false> FixedPoint_S21_11;

static constexpr inline float floatabs(float val)
{
    return (val < 0.f) ? -val : val;
}

template<typename TFixedPoint>
static constexpr inline bool equal(TFixedPoint val, float expected, float epsilon = 0.01f)
{
    return floatabs((float) val - expected) < epsilon;
}

constexpr FixedPoint_S2_14::MathsIntermediateType kTestFixed = FixedPoint_S2_14(0.1f) + (FixedPoint_S2_14(0.8f / (float) 64) * 64);
constexpr float kTestFloat = (float) kTestFixed;

static_assert(equal(FixedPoint_S2_14(0.1f) * 0.1f, 0.01f), "");
static_assert(equal(FixedPoint_S14_18(100.f).recip(), 0.01f), "");
static_assert(equal(FixedPoint_S14_18(100.25f).frac(), 0.25f), "");
static_assert(equal(FixedPoint_S2_14(0.1f) * FixedPoint_S2_14(0.1f).recip(), 1.f), "");
static_assert(equal(FixedPoint_S2_14(0.1f) + (FixedPoint_S2_14(0.8f / (float) 64) * 64), 0.9f), "");
static_assert(equal(FixedPoint_S21_11(1.f / (float) 64) + (FixedPoint_S21_11(1.f / (float) 64) * 63), 1.f), "");

void TestFixedPoint()
{
#if LOG_ENABLED
    FixedPoint_S2_14 a;
    a = 0.25f;
    LOG_INFO(FixedPointTesting, "a = %f (0.25)\n", (float)a);

    a = 1.1f;
    a = a + FixedPoint_S2_14(3.f);
    LOG_INFO(FixedPointTesting, "a = %f (1.99999)\n", (float)a);

    a = 1.4f;
    a *= FixedPoint_S2_14(0.5f);
    LOG_INFO(FixedPointTesting, "a = %f (0.7)\n", (float)a);

    a = 1.4f;
    a = (a * FixedPoint_S2_14(0.5f));
    LOG_INFO(FixedPointTesting, "a = %f (0.7)\n", (float)a);

    a = 0.9f;
    a /= FixedPoint_S2_14(1.5f);
    LOG_INFO(FixedPointTesting, "a = %f (0.6)\n", (float)a);

    a = 0.15f;
    a = a * 2;
    LOG_INFO(FixedPointTesting, "a = %f (0.3)\n", (float)a);

    a = 0.15f;
    a = a * (uint16_t)2;
    LOG_INFO(FixedPointTesting, "a = %f (0.3)\n", (float)a);

    a = FixedPoint_S2_14(1.f).sqrt();
    LOG_INFO(FixedPointTesting, "a = %f (1.0)\n", (float)a);

    a = FixedPoint_S2_14(2.f).sqrt();
    LOG_INFO(FixedPointTesting, "a = %f (1.4142)\n", (float)a);

    a = FixedPoint_S2_14(0.5f).sqrt();
    LOG_INFO(FixedPointTesting, "a = %f (0.7071)\n", (float)a);

    int32_t sq = 0x10000;
    sq = FixedPointSqrt(sq, 16);
    LOG_INFO(FixedPointTesting, "a = 0x%08x (0x00010000)\n", sq);

    sq = 0x4000;
    sq = FixedPointSqrt(sq, 14);
    LOG_INFO(FixedPointTesting, "a = 0x%08x (0x00004000)\n", sq);

    FixedPoint_S6_26 b = FixedPoint_S6_26(FixedPoint_S2_14(0.3f) - FixedPoint_S2_14(0.1f));
    LOG_INFO(FixedPointTesting, "b = %f (0.2)\n", (float)b);

    b = FixedPoint_S6_26(FixedPoint_S2_14(1.f) / FixedPoint_S2_14(0.125f));
    LOG_INFO(FixedPointTesting, "b = %f (8)\n", (float)b);

    b = Div(FixedPoint_S6_26(1.f), FixedPoint_S6_26(8.f), 0, 0);
    LOG_INFO(FixedPointTesting, "b = %f (0.125)\n", (float)b);

    b = Div(FixedPoint_S6_26(1.f), FixedPoint_S6_26(8.f), 4, 1);
    LOG_INFO(FixedPointTesting, "b = %f (0.125)\n", (float)b);

    FixedPoint_S14_18 d = Div(FixedPoint_S14_18(1.f), FixedPoint_S14_18(8.f), 11, 4);
    LOG_INFO(FixedPointTesting, "d = %f (0.125)\n", (float)d);

    LOG_INFO(FixedPointTesting, "s = %f (0.479)\n", (float)SinTable::LookUp(0.5f));
    LOG_INFO(FixedPointTesting, "s = %f (0.0)\n", (float)SinTable::LookUp(0.f));
    LOG_INFO(FixedPointTesting, "s = %f (0.0)\n", (float)SinTable::LookUp(kPi * 2.f));

    SinTableValue s, c;
    SinTable::SinCos(kPi * 2.f, s, c);
    LOG_INFO(FixedPointTesting, "s = %f (0.0), c = %f (1.0)\n", (float)s, (float) c);

#endif  
}


/* The square root algorithm is quite directly from
 * http://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_.28base_2.29
 * An important difference is that it is split to two parts
 * in order to use only 32-bit operations.
 *
 * Note that for negative numbers we return -sqrt(-inValue).
 * Not sure if someone relies on this behaviour, but not going
 * to break it for now. It doesn't slow the code much overall.
 */
int32_t FixedPointSqrt(int32_t inValue, int32_t numFractionalBits)
{
    uint8_t neg = (inValue < 0);
    uint32_t num = (neg ? -inValue : inValue);
    if(numFractionalBits < 16)
    {
        num <<= (16 - numFractionalBits);
    }
    else
    {
        num >>= (numFractionalBits - 16);
    }
    uint32_t result = 0;
    uint32_t bit;
    uint8_t n;

    // Many numbers will be less than 15, so
    // this gives a good balance between time spent
    // in if vs. time spent in the while loop
    // when searching for the starting value.
    if (num & 0xFFF00000)
        bit = (uint32_t)1 << 30;
    else
        bit = (uint32_t)1 << 18;

    while (bit > num)
        bit >>= 2;

    // The main part is executed twice, in order to avoid
    // using 64 bit values in computations.
    for (n = 0; n < 2; n++)
    {
        // First we get the top 24 bits of the answer.
        while (bit)
        {
            if (num >= result + bit)
            {
                num -= result + bit;
                result = (result >> 1) + bit;
            }
            else
            {
                result = (result >> 1);
            }
            bit >>= 2;
        }

        if (n == 0)
        {
            // Then process it again to get the lowest 8 bits.
            if (num > 65535)
            {
                // The remainder 'num' is too large to be shifted left
                // by 16, so we have to add 1 to result manually and
                // adjust 'num' accordingly.
                // num = a - (result + 0.5)^2
                //	 = num + result^2 - (result + 0.5)^2
                //	 = num - result - 0.5
                num -= result;
                num = (num << 16) - 0x8000;
                result = (result << 16) + 0x8000;
            }
            else
            {
                num <<= 16;
                result <<= 16;
            }

            bit = 1 << 14;
        }
    }

#ifndef FIXMATH_NO_ROUNDING
    // Finally, if next bit would have been 1, round the result upwards.
    if (num > result)
    {
        result++;
    }
#endif

    if(numFractionalBits < 16)
    {
        result >>= (16 - numFractionalBits);
    }
    else
    {
        result <<= (numFractionalBits - 16);
    }

    return (neg ? -(int32_t)result : (int32_t)result);
}
