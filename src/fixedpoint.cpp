// Fixed point maths constexpr template class
//
// Copyright (C) 2022 Oli Wright
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// A copy of the GNU General Public License can be found in the file
// COPYING.txt in the root of this project.
// If not, see <https://www.gnu.org/licenses/>.

#include "fixedpoint.h"
#include "log.h"
#include "sintable.h"

// Extremely simple random number generator
uint32_t g_randSeed = 123456789;
uint32_t SimpleRand()
{
    constexpr uint32_t kA = 1103515245;
    constexpr uint32_t kC = 12345;
    g_randSeed = kA * g_randSeed + kC;
    return g_randSeed;
}

// Some example fixed point types for testing
typedef FixedPoint<1,  14, int16_t, int32_t, true /*doClamping*/> FixedPoint_S1_14;
typedef FixedPoint<5,  26, int32_t, int32_t, false> FixedPoint_S5_26;
typedef FixedPoint<13, 18, int32_t, int32_t, false> FixedPoint_S13_18;
typedef FixedPoint<20, 11, int32_t, int32_t, false> FixedPoint_S20_11;
typedef FixedPoint<7,  24, int32_t, int32_t, false> FixedPoint_S7_24;
typedef FixedPoint<8,  23, int32_t, int32_t, false> FixedPoint_S8_23;

constexpr FixedPoint_S1_14 kTestFixed = FixedPoint_S1_14(0.1f) + (FixedPoint_S1_14(0.8f / (float) 64) * 64);
constexpr float kTestFloat = (float) kTestFixed;

constexpr FixedPoint_S1_14 kTestFixed2a = FixedPoint_S1_14(0.8f / (float) 64);
constexpr FixedPoint_S1_14 kTestFixed2 = kTestFixed2a * (int) 64;
constexpr float kTestFloat2 = (float) kTestFixed2;

constexpr float kTestFloat3 = (float) Mul<0,0>(FixedPoint_S1_14(0.1f), FixedPoint_S20_11(0.1f));
constexpr FixedPoint_S7_24 kTestFixed3 = Mul<6,2>(FixedPoint_S7_24(63.f), FixedPoint_S8_23(-2.f));
constexpr FixedPoint_S7_24 kTestFixed4 = -128;
constexpr float kTestFloat4 = (float) kTestFixed3;

constexpr FixedPoint_S1_14 kTestFixed5 = FixedPoint_S1_14(0.1f) * 0.1f;

constexpr FixedPoint_S7_24 kTestFixed6 = Div<6>(FixedPoint_S7_24(63.f),  FixedPoint_S8_23(-2.f));
constexpr float kTestFloat6 = (float) kTestFixed6;

// Some constexpr testing, to catch problems at compile-time
static constexpr inline float floatabs(float val)
{
    return (val < 0.f) ? -val : val;
}
template<typename TFixedPoint>
static constexpr inline bool equal(TFixedPoint val, float expected, float epsilon = 0.01f)
{
    return floatabs((float) val - expected) < epsilon;
}
static_assert(equal(FixedPoint_S1_14(0.1f) * 0.1f, 0.01f), "");
static_assert(equal(FixedPoint_S13_18(100.f).recip(), 0.01f), "");
static_assert(equal(FixedPoint_S13_18(100.25f).frac(), 0.25f), "");
static_assert(equal(FixedPoint_S5_26(0.1f) * FixedPoint_S1_14(0.1f).recip(), 1.f), "");
static_assert(equal(FixedPoint_S1_14(0.1f) + (FixedPoint_S1_14(0.8f / (float) 64) * 64), 0.9f), "");
static_assert(equal(FixedPoint_S20_11(1.f / (float) 64) + (FixedPoint_S20_11(1.f / (float) 64) * 63), 1.f), "");
static_assert(equal(Mul<6,2>(FixedPoint_S7_24(63.f), FixedPoint_S8_23(-2.f)), -126.f), "");
static_assert(equal((FixedPoint_S7_24(63.f) / FixedPoint_S8_23(-2.f)), -31.5f), "");

// Some run-time testing
static LogChannel FixedPointTesting(true);

static void testFloat(float fixedPointResult, float expected)
{
    LOG_INFO(FixedPointTesting, "Test value %f, expected %f : %s\n", fixedPointResult, expected, equal(fixedPointResult, expected) ? "SUCCESS" : "FAIL");
}
template<typename T>
void test(T fixedPointResult, float expected)
{
    testFloat((float) fixedPointResult, expected);
}

void TestFixedPoint()
{
#if LOG_ENABLED
    FixedPoint_S1_14 a;
    a = 0.25f;
    test(a, 0.25f);

    a = 1.1f;
    a = a + FixedPoint_S1_14(3.f);
    test(a, 1.9999f);

    a = 1.4f;
    a *= FixedPoint_S1_14(0.5f);
    test(a, 0.7f);

    a = 1.4f;
    a = (a * FixedPoint_S1_14(0.5f));
    test(a, 0.7f);

    a = 0.9f;
    a /= FixedPoint_S1_14(1.5f);
    test(a, 0.6f);

    a = 0.15f;
    a = a * 2;
    test(a, 0.3f);

    // a = 0.15f;
    // a = a * (uint16_t)2;
    // LOG_INFO(FixedPointTesting, "a = %f (0.3)\n", (float)a);

    a = FixedPoint_S1_14(1.f).sqrt();
    test(a, 1.f);

    a = FixedPoint_S1_14(2.f).sqrt();
    test(a, 1.4142f);

    a = FixedPoint_S1_14(0.5f).sqrt();
    test(a, 0.7071f);

    int32_t sq = 0x10000;
    sq = FixedPointSqrt(sq, 16);
    LOG_INFO(FixedPointTesting, "a = 0x%08x (0x00010000)\n", sq);

    sq = 0x4000;
    sq = FixedPointSqrt(sq, 14);
    LOG_INFO(FixedPointTesting, "a = 0x%08x (0x00004000)\n", sq);

    FixedPoint_S5_26 b = FixedPoint_S5_26(FixedPoint_S1_14(0.3f) - FixedPoint_S1_14(0.1f));
    test(b, 0.2f);

    b = FixedPoint_S5_26(FixedPoint_S1_14(1.f) / FixedPoint_S13_18(0.125f));
    test(b, 8.f);

    b = Div<1>(FixedPoint_S5_26(1.f), FixedPoint_S5_26(8.f));
    test(b, 0.125f);

    b = Div<1>(FixedPoint_S5_26(1.f), FixedPoint_S5_26(8.f));
    test(b, 0.125f);

    FixedPoint_S13_18 d = Div<1>(FixedPoint_S13_18(1.f), FixedPoint_S13_18(8.f));
    test(d, 0.125f);

    test(SinTable::LookUp(0.5f), 0.479f);
    test(SinTable::LookUp(0.f), 0.);
    test(SinTable::LookUp(kPi * 2.f), 0.);

    SinTableValue s, c;
    SinTable::SinCos(kPi * 2.f, s, c);
    test(s, 0.f);
    test(c, 1.f);
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

