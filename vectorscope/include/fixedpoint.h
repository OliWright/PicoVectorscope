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

template <unsigned int numFractionalBits, typename TStorageType, typename TIntermediateStorageType, bool doClamping = true>
class FixedPoint
{
public:
    typedef TStorageType StorageType;
    typedef TIntermediateStorageType IntermediateStorageType;
    using IntermediateType = FixedPoint<numFractionalBits, IntermediateStorageType, IntermediateStorageType, doClamping>;

    static constexpr unsigned int kNumFractionalBits = numFractionalBits;
    static constexpr unsigned int kNumWholeBits = (sizeof(StorageType) * 8) - kNumFractionalBits;
    static constexpr bool kIsSigned = ((StorageType)-1) < 0;
    static constexpr float kFractionalBitsMul = (float)(1 << kNumFractionalBits);
    static constexpr float kRecipFractionalBitsMul = 1.0f / kFractionalBitsMul;
    static constexpr IntermediateStorageType kFractionalBitsMask = (1 << numFractionalBits) - 1;
    static constexpr StorageType kStorageMSB = (StorageType)((~0ull) << (kNumWholeBits + kNumFractionalBits - 1));
    static constexpr StorageType kMinStorageType = kIsSigned ? kStorageMSB : (StorageType)0;
    static constexpr StorageType kMaxStorageType = kIsSigned ? ~kStorageMSB : (StorageType)~0ull;
    static constexpr IntermediateStorageType kMinIntermediateType = (IntermediateStorageType)kMinStorageType;
    static constexpr IntermediateStorageType kMaxIntermediateType = (IntermediateStorageType)kMaxStorageType;
    static constexpr float kMinFloat = ((float)kMinStorageType) * kRecipFractionalBitsMul;
    static constexpr float kMaxFloat = ((float)kMaxStorageType) * kRecipFractionalBitsMul;

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

    template<typename T>
    constexpr IntermediateType operator * (const T& rhs) const
    {
        return IntermediateType(((IntermediateStorageType) getStorage() * IntermediateType(rhs).getStorage()) >> kNumFractionalBits);
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
        constexpr int kPreShift = kIsSigned ? (int)kNumWholeBits - 2 : (int)kNumWholeBits - 1;
        constexpr int kPostShift = kPreShift - (int)kNumFractionalBits;
        return IntermediateType(SignedShift((IntermediateStorageType) ((1 << (kNumFractionalBits + kPreShift)) / (IntermediateStorageType)m_storage), kPostShift));
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
    template <unsigned int rhsNumFrac, typename rhsTStorage, typename rhsTIntermediateStorage, bool rhsDoClamping>
    constexpr FixedPoint fromOtherFormat(const FixedPoint<rhsNumFrac, rhsTStorage, rhsTIntermediateStorage, rhsDoClamping>& rhs)
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

// Free-function multiply, that can multiply two different fixed-point numbers
// and also gives you the option to pre-shift (right) either or both arguments
// in order to prevent overflow.
template<typename TA, typename TB>
constexpr static inline typename TA::IntermediateType Mul(TA a, TB b, int32_t preShiftBitsA = 0, int32_t preShiftBitsB = 0)
{
    typename TA::IntermediateStorageType sa = ((typename TA::IntermediateStorageType) a.getStorage()) >> preShiftBitsA;
    typename TA::IntermediateStorageType sb = ((typename TA::IntermediateStorageType) b.getStorage()) >> preShiftBitsB;
    int32_t postShiftBits = (int32_t)TB::kNumFractionalBits - preShiftBitsA - preShiftBitsB;
    return typename TA::IntermediateType((typename TA::IntermediateStorageType) ((sa * sb) >> postShiftBits));
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