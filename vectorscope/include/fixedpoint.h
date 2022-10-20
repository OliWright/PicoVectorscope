#ifndef FIXED_POINT_H
#define FIXED_POINT_H
#include <cstdint>

int32_t FixedPointSqrt(int32_t inValue, int32_t numFractionalBits);
uint32_t SimpleRand();

// This will shift right if shift is +ve, or shift left is shift is -ve
template<typename T>
static constexpr inline T SignedShift(T val, int shift)
{
    return (shift < 0) ? (T)((uint32_t) val << -shift) : (val >> shift);
}

// Free-function multiply for fixed-point numbers.
// Requires the caller to indicate the maximum number of whole-number bits
// (non-fractional bits, excluding a sign bit) for each argument, in order
// to maximise precision and prevent overflow
template<int numWholeBitsA, int numWholeBitsB, typename TA, typename TB>
constexpr static inline typename TA::IntermediateType Mul(TA a, TB b)
{
    constexpr int kTotalBitsA = TA::kNumFractionalBits + numWholeBitsA;
    constexpr int kTotalBitsB = TB::kNumFractionalBits + numWholeBitsB;
    constexpr int kNumResultBits = kTotalBitsA + kTotalBitsB + TA::kNumSignBits;
    constexpr int kNumStorageBits = (int) sizeof(typename TA::IntermediateStorageType) * 8;
    constexpr int kIdealPreshiftBits = kNumResultBits - kNumStorageBits;
    constexpr int kActualPreshiftBits = (kIdealPreshiftBits > TB::kNumFractionalBits) ? TB::kNumFractionalBits : (kIdealPreshiftBits > 0) ? kIdealPreshiftBits : 0;
    constexpr int kPostshiftBits = TB::kNumFractionalBits - kActualPreshiftBits;
    // Distribute the preshift between the arguments
    constexpr int kPreshiftA = kActualPreshiftBits >> 1;
    constexpr int kPreshiftB = kActualPreshiftBits - kPreshiftA;

    typename TA::IntermediateStorageType sa = ((typename TA::IntermediateStorageType) a.getStorage()) >> kPreshiftA;
    typename TA::IntermediateStorageType sb = ((typename TA::IntermediateStorageType) b.getStorage()) >> kPreshiftB;
    typename TA::IntermediateStorageType intResult = sa * sb;
    return typename TA::IntermediateType(intResult >> kPostshiftBits);
}

#if 0
// Free-function multiply, that can multiply two different fixed-point numbers
// and also gives you the option to pre-shift (right) either or both arguments
// in order to prevent overflow.
template<typename TA, typename TB>
constexpr static inline typename TA::MathsIntermediateType Mul(TA a, TB b, int32_t preShiftBitsA = 0, int32_t preShiftBitsB = 0)
{
    typename TA::MathsIntermediateStorageType sa = ((typename TA::MathsIntermediateStorageType) a.getStorage()) >> preShiftBitsA;
    typename TA::MathsIntermediateStorageType sb = ((typename TA::MathsIntermediateStorageType) b.getStorage()) >> preShiftBitsB;
    int32_t postShiftBits = (int32_t)TB::kNumFractionalBits - preShiftBitsA - preShiftBitsB;
    return typename TA::MathsIntermediateType((sa * sb) >> postShiftBits);
}
#endif


template <int numWholeBits, int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping = true>
class FixedPoint
{
public:
    typedef TStorageType StorageType;
    typedef TIntermediateStorageType IntermediateStorageType;
    using IntermediateType = FixedPoint<numWholeBits, numFractionalBits, IntermediateStorageType, IntermediateStorageType, doClamping>;

    static constexpr int kNumFractionalBits = numFractionalBits;
    static constexpr int kNumStorageBits = (int) sizeof(StorageType) * 8;
    static constexpr bool kIsSigned = ((StorageType)-1) < 0;
    static constexpr int kNumSignBits = kIsSigned ? 1 : 0;
    static constexpr int kNumStorageBitsLessSign = kNumStorageBits - kNumSignBits;
    static constexpr int kNumWholeBits = numWholeBits;
    static_assert(kNumWholeBits <= (kNumStorageBits - kNumFractionalBits- kNumSignBits), "Too many whole bits for the underlying storage type");
    static constexpr float kFractionalBitsMul = (float)(1 << kNumFractionalBits);
    static constexpr float kRecipFractionalBitsMul = 1.0f / kFractionalBitsMul;
    static constexpr IntermediateStorageType kFractionalBitsMask = (1 << numFractionalBits) - 1;
    static constexpr StorageType kStorageMSB = (StorageType)((~0ull) << (kNumStorageBits - 1));
    static constexpr StorageType kMinStorageType = kIsSigned ? kStorageMSB : (StorageType)0;
    static constexpr StorageType kMaxStorageType = kIsSigned ? ~kStorageMSB : (StorageType)~0ull;
    static constexpr IntermediateStorageType kMinIntermediateType = (IntermediateStorageType)kMinStorageType;
    static constexpr IntermediateStorageType kMaxIntermediateType = (IntermediateStorageType)kMaxStorageType;
    static constexpr float kMinFloat = ((float)kMinStorageType) * kRecipFractionalBitsMul;
    static constexpr float kMaxFloat = ((float)kMaxStorageType) * kRecipFractionalBitsMul;
    static constexpr FixedPoint kMin(kMinStorageType);
    static constexpr FixedPoint kMax(kMaxStorageType);

    constexpr FixedPoint()
    {}

    // Construct from some other format
    template<typename T>
    constexpr FixedPoint(const T& rhs)
     : m_storage(fromOtherFormat(rhs).getStorage())
     {}

    explicit constexpr FixedPoint(StorageType storage) : m_storage(storage) {}

    explicit constexpr operator float() const
    {
        return toFloat();
    }

    explicit constexpr operator int() const
    {
        return (int) (m_storage >> kNumFractionalBits);
    }

    explicit constexpr operator unsigned int() const
    {
        return (unsigned int) (m_storage >> kNumFractionalBits);
    }

    template<typename T>
    constexpr IntermediateType operator + (const T& rhs) const
    {
        return IntermediateType((IntermediateStorageType) getStorage() + IntermediateType(rhs).getStorage());
    }

    template<typename T>
    constexpr IntermediateType operator - (const T& rhs) const
    {
        return IntermediateType((IntermediateStorageType) getStorage() - IntermediateType(rhs).getStorage());
    }

    constexpr IntermediateType operator - () const
    {
        return IntermediateType(-(IntermediateStorageType) getStorage());
    }

    constexpr IntermediateType operator * (int rhs) const
    {
        return IntermediateType((IntermediateStorageType) (getStorage() * rhs));
    }

    constexpr IntermediateType operator * (float rhs) const
    {
        return *this * IntermediateType(rhs);
    }

    template<typename T>
    constexpr IntermediateType operator * (const T& rhs) const
    {
        return Mul<kNumWholeBits, T::kNumWholeBits>(*this, rhs);
    }

    template<typename T>
    constexpr IntermediateType operator / (const T& rhs) const
    {
        return IntermediateType(((IntermediateStorageType) getStorage() << kNumFractionalBits) / IntermediateType(rhs).getStorage());
    }

    constexpr IntermediateType operator / (int rhs) const
    {
        return IntermediateType((IntermediateStorageType)((IntermediateStorageType) getStorage() / rhs));
    }

    constexpr IntermediateType operator >> (int rhs) const
    {
        return IntermediateType(((IntermediateStorageType) getStorage()) >> rhs);
    }

    constexpr IntermediateType operator << (int rhs) const
    {
        return IntermediateType(((IntermediateStorageType) getStorage()) << rhs);
    }

    template<typename T>
    constexpr FixedPoint& operator += (const T& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    template<typename T>
    constexpr FixedPoint& operator -= (const T& rhs)
    {
        *this = *this - rhs;
        return *this;
    }

    template<typename T>
    constexpr FixedPoint& operator *= (const T& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template<typename T>
    constexpr FixedPoint& operator /= (const T& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    template<typename T>
    constexpr bool operator < (const T& rhs) const
    {
        return m_storage < FixedPoint(rhs).getStorage();
    }

    template<typename T>
    constexpr bool operator > (const T& rhs) const
    {
        return m_storage > FixedPoint(rhs).getStorage();
    }

    template<typename T>
    constexpr bool operator != (const T& rhs) const
    {
        return m_storage != FixedPoint(rhs).getStorage();
    }

    IntermediateType sqrt() const
    {
        return IntermediateType((IntermediateStorageType) FixedPointSqrt(m_storage, kNumFractionalBits));
    }

    constexpr IntermediateType frac() const
    {
        return IntermediateType((IntermediateStorageType) m_storage & kFractionalBitsMask);
    }

    constexpr IntermediateType abs() const
    {
        return (*this < 0) ? -*this : *this;
    }

    constexpr IntermediateType recip() const
    {
        // How many bits can we shift a 1.0 left until we hit the top of the storage
        constexpr int kPreShiftLeft = IntermediateType::kNumStorageBits - kNumFractionalBits - kNumSignBits - 1;
        // The numerator will be that 1.0, shifted left as far as we can
        constexpr IntermediateStorageType kNumerator = (IntermediateStorageType) (1 << (kNumFractionalBits + kPreShiftLeft));
        // If we just divide now, how many bits will be need to shift the result left
        // (The more we're able to pre-shift left, the less accuracy we'll lose in the result)
        constexpr int kNominalPostShiftLeft = kNumFractionalBits - kPreShiftLeft;

        constexpr int kMaxDenominatorShiftRight = kNominalPostShiftLeft;//IntermediateType::kNumStorageBits - kNumWholeBits - kNumFractionalBits - kNumSignBits;
        //constexpr int kDenominatorShiftRight = (kNominalPostShiftLeft > 0) ? ((kNominalPostShiftLeft > kMaxDenominatorShiftRight) ? kMaxDenominatorShiftRight : kNominalPostShiftLeft) : 0;
        constexpr int kDenominatorShiftRight = kNominalPostShiftLeft >> 1;
        constexpr int kPostShiftLeft = kNominalPostShiftLeft - kDenominatorShiftRight;

        IntermediateStorageType denominator = SignedShift((IntermediateStorageType) m_storage, kDenominatorShiftRight);
        return  IntermediateType(SignedShift(kNumerator / denominator, -kPostShiftLeft));
    }

    constexpr StorageType getStorage() const
    {
        return m_storage;
    }

    constexpr StorageType getIntegerPart() const
    {
        return m_storage >> kNumFractionalBits;
    }

    static FixedPoint randFullRange()
    {
        return FixedPoint((StorageType) SimpleRand());
    }

    static FixedPoint randZeroToOne()
    {
        return FixedPoint((StorageType) (SimpleRand() & kFractionalBitsMask));
    }

    static FixedPoint randMinusOneToOne()
    {
        return FixedPoint((StorageType) (SimpleRand() & ((1 << (kNumFractionalBits + 1)) - 1))) - FixedPoint(1.f);
    }

private:

    // Specialisation to convert from a different fixed point format
    template <int rhsNumWhole, int rhsNumFrac, typename rhsTStorage, typename rhsTIntermediateStorage, bool rhsDoClamping>
    constexpr FixedPoint fromOtherFormat(const FixedPoint<rhsNumWhole, rhsNumFrac, rhsTStorage, rhsTIntermediateStorage, rhsDoClamping>& rhs)
    {
        return FixedPoint((StorageType)clamp(SignedShift((IntermediateStorageType)rhs.getStorage(), rhs.kNumFractionalBits - kNumFractionalBits)));
    }
    // Specialisation to convert from float
    constexpr FixedPoint fromOtherFormat(const float& rhs)
    {
        return FixedPoint((StorageType)clamp((IntermediateStorageType)(rhs * kFractionalBitsMul)));
    }

    constexpr float toFloat() const
    {
        return ((float)m_storage) * kRecipFractionalBitsMul;
    }

    static constexpr float clamp(float val)
    {
        return doClamping ? ((val >= kMaxFloat) ? kMaxFloat : ((val <= kMinFloat) ? kMinFloat : val)) : val;
    }
    static constexpr IntermediateStorageType clamp(IntermediateStorageType val)
    {
        return doClamping ?
                   ((val >= kMaxIntermediateType) ? kMaxIntermediateType : ((val <= kMinIntermediateType) ? kMinIntermediateType : val)) :
                   val;
    }

    StorageType m_storage;
};

// Deprecated
template<typename TA, typename TB>
constexpr static inline typename TA::IntermediateType OldMul(TA a, TB b, int32_t preShiftBitsA = 0, int32_t preShiftBitsB = 0)
{
    return a * b;
    // typename TA::IntermediateStorageType sa = ((typename TA::IntermediateStorageType) a.getStorage()) >> preShiftBitsA;
    // typename TA::IntermediateStorageType sb = ((typename TA::IntermediateStorageType) b.getStorage()) >> preShiftBitsB;
    // int32_t postShiftBits = (int32_t)TB::kNumFractionalBits - preShiftBitsA - preShiftBitsB;
    // return typename TA::IntermediateType((typename TA::IntermediateStorageType) ((sa * sb) >> postShiftBits));
}

// Free-function divide, that can divide two different fixed-point numbers
// and also gives you the option to post-shift (left) in order to reduce the
// pre-shift in order to prevent overflow.
template<typename TA, typename TB>
constexpr static inline typename TA::IntermediateType Div(TA a, TB b, int32_t preShiftBitsA = (int32_t)TB::kNumFractionalBits, int32_t preShiftBitsB = 0 )
{
    typename TA::IntermediateStorageType sa = ((typename TA::IntermediateStorageType) a.getStorage()) << preShiftBitsA;
    typename TA::IntermediateStorageType sb = ((typename TA::IntermediateStorageType) b.getStorage()) >> preShiftBitsB;
    int32_t postShiftBits = (int32_t)TB::kNumFractionalBits - preShiftBitsA - preShiftBitsB;
    return typename TA::IntermediateType((typename TA::IntermediateStorageType) ((uint32_t)(sa / sb) << postShiftBits));
}

template<typename T>
static inline T saturate(const T& val)
{
    return (val > 1.f) ? 1.f : (val < 0.f) ? 0.f : val;
}

void TestFixedPoint();

#endif