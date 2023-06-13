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

// Exmple usage
//
// Create a fixed-point type.
// Here I'll create one that uses 16-bits of storage (its sizeof() is 2)
// where I'll store signed numbers with up to 4 bits of whole numbers
// andh 8-bits of fractional precision.
// For intermediate maths results, I'll use a 32-bit value.
//
// typedef FixedPoint<4, 8, int16_t, int32_t> MyFixedPointType;
//
// Because most of this class is constexpr-friendly, you can do a lot
// of operations at compile time. E.g.
//
// static constexpr MyFixedPointType kExample0 = 3.5f; 
// static constexpr MyFixedPointType kExample1 = kExample0.recip(); 
// static constexpr MyFixedPointType kExample2 = kExample1 * 7;
// Etc.
//
// Lots of usual operators are defined, like + - * /
// but you must really have a good understanding of the limitations
// of fixed-point or you'll run into trouble.
//
// Operators will perform their operations using the intermediate
// format of the left-hand argument.  If the right-hand argument
// is a different format, most operators will convert them first.
//
// Conversion from one fixed point format to another will
// usually be a single shift operation.
//
// There are free-functions for Mul and Div that take template
// parameters for overriding the required precision to have some
// indirect control over preshifts and postshifts.

#pragma once
#include <cstdint>

// Non-templatised sqrt, because it's quite large
int32_t FixedPointSqrt(int32_t inValue, int32_t numFractionalBits);
// 32-bit pseudo random number generator
uint32_t SimpleRand();
extern uint32_t g_randSeed;
typedef unsigned int uint;

constexpr float kPi = 3.14159265358979323846f;
constexpr float k2Pi = kPi * 2.f;

// This will shift right if shift is +ve, or shift left is shift is -ve
template <typename T>
static constexpr inline T SignedShift(T val, int shift)
{
    return (shift < 0) ? (T)((uint32_t)val << -shift) : (val >> shift);
}

template <typename T>
static constexpr inline T ClampToPositive(T val)
{
    return (val > 0) ? val : 0;
}

// Free-function multiply for fixed-point numbers.
// Allows the caller to override the number of whole and fractional bits for each argument
// in order to maximise precision and prevent overflow
template <int numWholeBitsA = -1, int numWholeBitsB = -1, int numFractionalBitsA = -1, int numFractionalBitsB = -1, typename TA, typename TB>
constexpr static inline typename TA::IntermediateType Mul(TA a, TB b)
{
    static_assert(numFractionalBitsA < TA::kNumFractionalBits,
                  "Explicit fractional bit count for argument A is more than its type");
    static_assert(numFractionalBitsA < TA::kNumFractionalBits,
                  "Explicit fractional bit count for argument B is more than its type");

    // Use defaults from the arg types for unspecified parameters
    constexpr int kNumWholeBitsA = (numWholeBitsA == -1) ? TA::kNumWholeBits : numWholeBitsA;
    constexpr int kNumWholeBitsB = (numWholeBitsB == -1) ? TB::kNumWholeBits : numWholeBitsB;
    constexpr int kNumFractionalBitsA = (numFractionalBitsA == -1) ? TA::kNumFractionalBits : numFractionalBitsA;
    constexpr int kNumFractionalBitsB = (numFractionalBitsB == -1) ? TB::kNumFractionalBits : numFractionalBitsB;
    constexpr bool kExplicitFractionalBitsProvided = (numFractionalBitsA != -1) || (numFractionalBitsB != -1);

    // Number of bits of precision lost for each arg
    constexpr int kPrecisionBitsLostA = TA::kNumFractionalBits - kNumFractionalBitsA;
    constexpr int kPrecisionBitsLostB = TB::kNumFractionalBits - kNumFractionalBitsB;

    // How many bits do we need to preserve all the precision we would like
    constexpr int kTotalBitsA = kNumWholeBitsA + kNumFractionalBitsA;
    constexpr int kTotalBitsB = kNumWholeBitsB + kNumFractionalBitsB;
    constexpr int kNumResultBits = kTotalBitsA + kTotalBitsB + TA::kNumSignBits;
    constexpr int kNumStorageBits = (int)sizeof(typename TA::IntermediateStorageType) * 8;

    // Is it too many?
    // If so, we'll need to shift the args further to the right before the multiply
    constexpr int kNumBitsTooMany = ClampToPositive(kNumResultBits - kNumStorageBits);
    constexpr int kPreshiftA = kPrecisionBitsLostA + (kNumBitsTooMany >> 1);
    constexpr int kPreshiftB = kPrecisionBitsLostB + kNumBitsTooMany - (kNumBitsTooMany >> 1);

    // How many bits do we need to shift the multiply result by to put it back into
    // the correct fixed point storage format for the return type
    constexpr int kPostshiftBits = TB::kNumFractionalBits - kPreshiftA - kPreshiftB;

    static_assert(!kExplicitFractionalBitsProvided || (kPostshiftBits >= 0), "More precision loss than necessary");

    // Preshift the arguments, do the multiply, then post-shift the result
    typename TA::IntermediateStorageType sa = ((typename TA::IntermediateStorageType)a.getStorage()) >> kPreshiftA;
    typename TA::IntermediateStorageType sb = ((typename TA::IntermediateStorageType)b.getStorage()) >> kPreshiftB;
    return typename TA::IntermediateType(SignedShift(sa * sb, kPostshiftBits));
}

template <int numWholeBitsA>
constexpr static inline float Div(float a, float b)
{
    return a / b;
}

template <int numWholeBitsA, typename TA, typename TB>
constexpr static inline typename TA::IntermediateType Div(TA a, TB b)
{
    // How many bits can we shift left until we hit the top of the storage
    constexpr int kPreShiftLeft = a.kNumStorageBits - a.kNumFractionalBits - a.kNumSignBits - numWholeBitsA;
    // The numerator will be that 1.0, shifted left as far as we can
    typename TA::IntermediateStorageType kNumerator
        = (typename TA::IntermediateStorageType)(a.getStorage() << kPreShiftLeft);
    // If we just divide now, how many bits will be need to shift the result left
    // (The more we're able to pre-shift left, the less accuracy we'll lose in the result)
    constexpr int kNominalPostShiftLeft = b.kNumFractionalBits - kPreShiftLeft;

    // Need to think more about this whole shifting the denominator right vs. result left thing.
#if 0
    constexpr int kDenominatorShiftRight = 0;
#else
    // constexpr int kMaxDenominatorShiftRight = kNominalPostShiftLeft;
    constexpr int kDenominatorShiftRight = kNominalPostShiftLeft >> 1;
#endif
    constexpr int kPostShiftLeft = kNominalPostShiftLeft - kDenominatorShiftRight;

    typename TA::IntermediateStorageType denominator
        = SignedShift((typename TA::IntermediateStorageType)b.getStorage(), kDenominatorShiftRight);
    return typename TA::IntermediateType(SignedShift(kNumerator / denominator, -kPostShiftLeft));
}

template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping = true>
class FixedPoint
{
public:
    typedef TStorageType StorageType;
    typedef TIntermediateStorageType IntermediateStorageType;
    using IntermediateType
        = FixedPoint<numWholeBits, numFractionalBits, IntermediateStorageType, IntermediateStorageType, doClamping>;

    static constexpr int kNumStorageBits = (int)sizeof(StorageType) * 8;
    static constexpr int kNumWholeBits = numWholeBits;
    static constexpr int kNumFractionalBits = numFractionalBits;
    static constexpr bool kIsSigned = ((StorageType)-1) < 0;
    static constexpr int kNumSignBits = kIsSigned ? 1 : 0;
    static_assert((kNumWholeBits + kNumFractionalBits + kNumSignBits) <= kNumStorageBits,
                  "Too many bits for the underlying storage type");

    static constexpr float kFractionalBitsMul = (float)(1 << kNumFractionalBits);
    static constexpr float kRecipFractionalBitsMul = 1.0f / kFractionalBitsMul;
    static constexpr IntermediateStorageType kFractionalBitsMask = (1 << numFractionalBits) - 1;
    static constexpr StorageType kStorageMSB = (StorageType)((~0ull) << (kNumStorageBits - 1));
    static constexpr StorageType kMinStorageType = kIsSigned ? kStorageMSB : (StorageType)0;
    static constexpr StorageType kMaxStorageType = kIsSigned ? ~kStorageMSB : (StorageType)~0ull;
    static constexpr float kMinFloat = ((float)kMinStorageType) * kRecipFractionalBitsMul;
    static constexpr float kMaxFloat = ((float)kMaxStorageType) * kRecipFractionalBitsMul;
    static constexpr FixedPoint kMin = FixedPoint(kMinStorageType);
    static constexpr FixedPoint kMax = FixedPoint(kMaxStorageType);

    constexpr FixedPoint() {}

    // Allow implicit construction from some other fixed point format
    template <int rhsNumWhole, int rhsNumFrac, typename rhsTStorage, typename rhsTIntermediateStorage, bool rhsDoClamping>
    constexpr FixedPoint(const FixedPoint<rhsNumWhole, rhsNumFrac, rhsTStorage, rhsTIntermediateStorage, rhsDoClamping>& rhs)
    : m_storage(fromOtherFormat(rhs).getStorage())
    {}

    // Explicit construction from the StorageType will just store that value directly.
    explicit constexpr FixedPoint(StorageType storage) : m_storage(storage) {}

    // Allow implicit construction from float
    constexpr FixedPoint(float rhs) : m_storage(fromOtherFormat(rhs).getStorage()) {}

    // Allow implicit construction from int
    constexpr FixedPoint(int rhs) : m_storage((StorageType)(int)((uint)rhs << kNumFractionalBits)) {}

    // Allow implicit construction from uint
    constexpr FixedPoint(uint rhs) : m_storage(rhs << kNumFractionalBits) {}

    // Cast to float must be explicit, to prevent accidents
    explicit constexpr operator float() const { return toFloat(); }
    // Cast to int must be explicit, to prevent accidents
    explicit constexpr operator int() const { return (int)(m_storage >> kNumFractionalBits); }
    // Cast to unsigned int must be explicit, to prevent accidents
    explicit constexpr operator unsigned int() const { return (unsigned int)(m_storage >> kNumFractionalBits); }

    template <typename T>
    constexpr IntermediateType operator+(const T& rhs) const
    {
        return IntermediateType((IntermediateStorageType)getStorage() + IntermediateType(rhs).getStorage());
    }

    template <typename T>
    constexpr IntermediateType operator-(const T& rhs) const
    {
        return IntermediateType((IntermediateStorageType)getStorage() - IntermediateType(rhs).getStorage());
    }

    constexpr IntermediateType operator-() const { return IntermediateType(-(IntermediateStorageType)getStorage()); }

    constexpr IntermediateType operator*(int rhs) const
    {
        return IntermediateType((IntermediateStorageType)(getStorage() * rhs));
    }

    constexpr IntermediateType operator*(float rhs) const { return *this * IntermediateType(rhs); }

    template <typename T>
    constexpr IntermediateType operator*(const T& rhs) const
    {
        return Mul<kNumWholeBits, T::kNumWholeBits>(*this, rhs);
    }

    template <typename T>
    constexpr IntermediateType operator/(const T& rhs) const
    {
        return Div<kNumWholeBits>(*this, rhs);
    }

    constexpr IntermediateType operator/(int rhs) const
    {
        return IntermediateType((IntermediateStorageType)((IntermediateStorageType)getStorage() / rhs));
    }

    constexpr IntermediateType operator>>(int rhs) const
    {
        return IntermediateType(((IntermediateStorageType)getStorage()) >> rhs);
    }

    constexpr IntermediateType operator<<(int rhs) const
    {
        return IntermediateType(((IntermediateStorageType)getStorage()) << rhs);
    }

    template <typename T>
    constexpr FixedPoint& operator+=(const T& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    template <typename T>
    constexpr FixedPoint& operator-=(const T& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    template <typename T>
    constexpr FixedPoint& operator*=(const T& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template <typename T>
    constexpr FixedPoint& operator/=(const T& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    template <typename T>
    constexpr bool operator<(const T& rhs) const
    {
        return m_storage < FixedPoint(rhs).getStorage();
    }

    template <typename T>
    constexpr bool operator<=(const T& rhs) const
    {
        return m_storage <= FixedPoint(rhs).getStorage();
    }

    template <typename T>
    constexpr bool operator>(const T& rhs) const
    {
        return m_storage > FixedPoint(rhs).getStorage();
    }

    template <typename T>
    constexpr bool operator>=(const T& rhs) const
    {
        return m_storage >= FixedPoint(rhs).getStorage();
    }

    template <typename T>
    constexpr bool operator!=(const T& rhs) const
    {
        return m_storage != FixedPoint(rhs).getStorage();
    }

    IntermediateType sqrt() const
    {
        return IntermediateType((IntermediateStorageType)FixedPointSqrt(m_storage, kNumFractionalBits));
    }

    constexpr IntermediateType frac() const
    {
        return IntermediateType((IntermediateStorageType)m_storage & kFractionalBitsMask);
    }

    constexpr IntermediateType round() const
    {
        return IntermediateType((*this + 0.5f).m_storage & ~kFractionalBitsMask);
    }

    constexpr IntermediateType abs() const { return (*this < 0) ? (IntermediateType) -*this : (IntermediateType) *this; }

    constexpr IntermediateType recip() const { return Div<1>(IntermediateType(1.f), *this); }

    constexpr StorageType getStorage() const { return m_storage; }

    constexpr StorageType getIntegerPart() const { return m_storage >> kNumFractionalBits; }

    static FixedPoint randFullRange() { return FixedPoint((StorageType)SimpleRand()); }

    static FixedPoint randZeroToOne() { return FixedPoint((StorageType)(SimpleRand() & kFractionalBitsMask)); }

    static FixedPoint randMinusOneToOne()
    {
        return FixedPoint((StorageType)(SimpleRand() & ((1 << (kNumFractionalBits + 1)) - 1))) - FixedPoint(1.f);
    }

    static FixedPoint ApproxATan2(FixedPoint y, FixedPoint x)
    {
        constexpr FixedPoint kCoeff1 = kPi * 0.25f;
        constexpr FixedPoint kCoeff2 = kCoeff1 * 3;
        FixedPoint::IntermediateType absY = Abs(y) + FixedPoint((FixedPoint::StorageType)1);     // kludge to prevent 0/0 condition
        FixedPoint angle;
        if (x >= 0)
        {
            FixedPoint r = (x - absY) / (x + absY);
            angle = kCoeff1 - kCoeff1 * r;
        }
        else
        {
            FixedPoint r = (x + absY) / (absY - x);
            angle = kCoeff2 - kCoeff1 * r;
        }
        if (y < 0)
            return(-angle);     // negate if in quad III or IV
        else
            return(angle);
    }


private:
    // Specialisation to convert from a different fixed point format
    template <int rhsNumWhole, int rhsNumFrac, typename rhsTStorage, typename rhsTIntermediateStorage, bool rhsDoClamping>
    constexpr FixedPoint fromOtherFormat(
        const FixedPoint<rhsNumWhole, rhsNumFrac, rhsTStorage, rhsTIntermediateStorage, rhsDoClamping>& rhs)
    {
        return FixedPoint((StorageType)clamp(
            SignedShift((IntermediateStorageType)rhs.getStorage(), rhs.kNumFractionalBits - kNumFractionalBits)));
    }
    // Specialisation to convert from float
    constexpr FixedPoint fromOtherFormat(const float& rhs)
    {
        return FixedPoint((StorageType)clamp((IntermediateStorageType)(rhs * kFractionalBitsMul)));
    }

    constexpr float toFloat() const { return ((float)m_storage) * kRecipFractionalBitsMul; }

    static constexpr float clamp(float val)
    {
        return doClamping ? ((val >= kMaxFloat) ? kMaxFloat : ((val <= kMinFloat) ? kMinFloat : val)) : val;
    }
    static constexpr IntermediateStorageType clamp(IntermediateStorageType val)
    {
        constexpr IntermediateStorageType kMin = (IntermediateStorageType)kMinStorageType;
        constexpr IntermediateStorageType kMax = (IntermediateStorageType)kMaxStorageType;
        return doClamping ? ((val >= kMax) ? kMax : ((val <= kMin) ? kMin : val)) : val;
    }

    StorageType m_storage;
};

template <typename T>
static inline T saturate(const T& val)
{
    return (val > 1.f) ? 1.f : (val < 0.f) ? 0.f : val;
}

//
// Some free operators, with float as the lhs.
// We do this, rather than have an implicit cast to float because then
// it's safer against accidental float usage
//
template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping>
static inline float operator * (float a, FixedPoint<numWholeBits, numFractionalBits, TStorageType, TIntermediateStorageType, doClamping> b)
{
    return a * (float) b;
}

#if 0 // We can't override the assignment operator
template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping>
static inline float& operator = (float& a, FixedPoint<numWholeBits, numFractionalBits, TStorageType, TIntermediateStorageType, doClamping> b)
{
    a = (float) b;
    return a;
}
#endif

template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping>
static inline float& operator += (float& a, FixedPoint<numWholeBits, numFractionalBits, TStorageType, TIntermediateStorageType, doClamping> b)
{
    a = a + (float) b;
    return a;
}

template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping>
static inline float& operator -= (float& a, FixedPoint<numWholeBits, numFractionalBits, TStorageType, TIntermediateStorageType, doClamping> b)
{
    a = a - (float) b;
    return a;
}

template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping>
static inline bool operator < (float a, FixedPoint<numWholeBits, numFractionalBits, TStorageType, TIntermediateStorageType, doClamping> b)
{
    return a < (float) b;
}

template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping>
static inline bool operator > (float a, FixedPoint<numWholeBits, numFractionalBits, TStorageType, TIntermediateStorageType, doClamping> b)
{
    return a > (float) b;
}

//
// Some free-function helpers, with specialistion for float types.
// Prefer these over the member variables, because it makes it easier to
// temporarily switch your fixed-point types to floats, to help debugging
//
constexpr static inline float Recip(float a)
{
    return 1.f / a;
}
template <typename  TA>
constexpr static inline typename TA::IntermediateType Recip(TA a)
{
    return a.recip();
}

constexpr static inline float Round(float a)
{
    return (float) int(a+0.5f);
}
template <typename  TA>
constexpr static inline typename TA::IntermediateType Round(TA a)
{
    return a.round();
}

constexpr static inline float Abs(float a)
{
    return (a < 0.f) ? -a : a;
}
template <typename  TA>
constexpr static inline typename TA::IntermediateType Abs(TA a)
{
    return a.abs();
}

// Unit tests
void TestFixedPoint();

