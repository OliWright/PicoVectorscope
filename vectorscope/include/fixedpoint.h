#ifndef FIXED_POINT_H
#define FIXED_POINT_H
#include <cstdint>

int32_t FixedPointSqrt(int32_t inValue, int32_t numFractionalBits);
uint32_t SimpleRand();

template <unsigned int numFractionalBits, typename TStorageType>
class MathsIntermediateFixedPoint
{
public:
    typedef TStorageType StorageType;

    static constexpr unsigned int kNumFractionalBits = numFractionalBits;
    static constexpr unsigned int kNumWholeBits = (sizeof(StorageType) * 8) - kNumFractionalBits;
    static constexpr bool kIsSigned = ((StorageType)-1) < 0;
    static constexpr float kFractionalBitsMul = (float)(1 << kNumFractionalBits);
    static constexpr float kRecipFractionalBitsMul = 1.0f / kFractionalBitsMul;
    static constexpr StorageType kFractionalBitsMask = (1 << numFractionalBits) - 1;

    // MathsIntermediateFixedPoint() delete;
    //constexpr MathsIntermediateFixedPoint(float val) : m_storage( floatToStorageType(val) ) {}
    constexpr MathsIntermediateFixedPoint(StorageType val) : m_storage(val)
    {
    }

    static constexpr MathsIntermediateFixedPoint fromFloat(float val)
    {
        return MathsIntermediateFixedPoint(floatToStorageType(val));
    }

    explicit operator float() const
    {
        return storageTypeToFloat(m_storage);
    }

    template <unsigned int rhsNumFractionalBits, typename rhsTStorageType>
    MathsIntermediateFixedPoint(const MathsIntermediateFixedPoint<rhsNumFractionalBits, rhsTStorageType>& rhs)
    {
        m_storage = rhs.getStorage();
        if(rhsNumFractionalBits > kNumFractionalBits)
        {
            m_storage >>= (rhsNumFractionalBits - kNumFractionalBits);
        }
        else if(rhsNumFractionalBits < kNumFractionalBits)
        {
            m_storage <<= (kNumFractionalBits - rhsNumFractionalBits);
        }
    }

    MathsIntermediateFixedPoint& operator=(const MathsIntermediateFixedPoint& rhs)
    {
        m_storage = rhs.m_storage;
        return *this;
    }
    MathsIntermediateFixedPoint& operator=(float rhs)
    {
        m_storage = floatToStorageType(rhs);
        return *this;
    }

    constexpr MathsIntermediateFixedPoint operator+(const MathsIntermediateFixedPoint& rhs) const
    {
        return MathsIntermediateFixedPoint(m_storage + rhs.m_storage);
    }

    constexpr MathsIntermediateFixedPoint operator-(const MathsIntermediateFixedPoint& rhs) const
    {
        return MathsIntermediateFixedPoint(m_storage - rhs.m_storage);
    }

    constexpr MathsIntermediateFixedPoint operator*(int rhs) const
    {
        return MathsIntermediateFixedPoint(((int)m_storage * rhs));
    }

    constexpr MathsIntermediateFixedPoint operator*(float rhs) const
    {
        return *this * fromFloat(rhs);
    }

    constexpr MathsIntermediateFixedPoint operator*(const MathsIntermediateFixedPoint& rhs) const
    {
        return MathsIntermediateFixedPoint((m_storage * rhs.m_storage) >> kNumFractionalBits);
    }
    constexpr MathsIntermediateFixedPoint operator/(const MathsIntermediateFixedPoint& rhs) const
    {
        return MathsIntermediateFixedPoint((m_storage << kNumFractionalBits) / rhs.m_storage);
    }

    MathsIntermediateFixedPoint& operator+=(const MathsIntermediateFixedPoint& rhs)
    {
        m_storage += rhs.m_storage;
        return *this;
    }

    MathsIntermediateFixedPoint& operator<<=(int shift)
    {
        m_storage <<= shift;
        return *this;
    }

    MathsIntermediateFixedPoint& operator/=(int rhs)
    {
        m_storage /= rhs;
        return *this;
    }

    constexpr bool operator < (const MathsIntermediateFixedPoint& rhs) const
    {
        return m_storage < rhs.m_storage;
    }

    constexpr bool operator > (const MathsIntermediateFixedPoint& rhs) const
    {
        return m_storage > rhs.m_storage;
    }

    MathsIntermediateFixedPoint sqrt() const
    {
        return MathsIntermediateFixedPoint((StorageType) FixedPointSqrt(m_storage, kNumFractionalBits));
    }

    constexpr MathsIntermediateFixedPoint frac() const
    {
        return MathsIntermediateFixedPoint(m_storage & kFractionalBitsMask);
    }

    constexpr MathsIntermediateFixedPoint abs() const
    {
        return (*this < MathsIntermediateFixedPoint(0)) ? (MathsIntermediateFixedPoint(0) - *this) : *this;
    }

    constexpr StorageType getStorage() const
    {
        return m_storage;
    }

    constexpr StorageType getIntegerPart() const
    {
        return m_storage >> kNumFractionalBits;
    }

private:
    static constexpr StorageType floatToStorageType(float val)
    {
        return (StorageType)(val * kFractionalBitsMul);
    }

    static constexpr float storageTypeToFloat(StorageType val)
    {
        return ((float)val) * kRecipFractionalBitsMul;
    }

    StorageType m_storage;
};

template <unsigned int numFractionalBits, typename TStorageType, typename TMathsIntermediateType, bool doClamping = true>
class FixedPoint
{
public:
    typedef TStorageType StorageType;
    typedef TMathsIntermediateType MathsIntermediateStorageType;
    typedef MathsIntermediateFixedPoint<numFractionalBits, TMathsIntermediateType> MathsIntermediateType;

    static constexpr unsigned int kNumFractionalBits = numFractionalBits;
    static constexpr unsigned int kNumWholeBits = (sizeof(StorageType) * 8) - kNumFractionalBits;
    static constexpr bool kIsSigned = ((StorageType)-1) < 0;
    static constexpr float kFractionalBitsMul = (float)(1 << kNumFractionalBits);
    static constexpr float kRecipFractionalBitsMul = 1.0f / kFractionalBitsMul;
    static constexpr MathsIntermediateStorageType kFractionalBitsMask = (1 << numFractionalBits) - 1;
    static constexpr StorageType kStorageMSB = (StorageType)((~0ull) << (kNumWholeBits + kNumFractionalBits - 1));
    static constexpr StorageType kMinStorageType = kIsSigned ? kStorageMSB : (StorageType)0;
    static constexpr StorageType kMaxStorageType = kIsSigned ? ~kStorageMSB : (StorageType)~0ull;
    static constexpr MathsIntermediateStorageType kMinIntermediateType = (MathsIntermediateStorageType)kMinStorageType;
    static constexpr MathsIntermediateStorageType kMaxIntermediateType = (MathsIntermediateStorageType)kMaxStorageType;
    static constexpr float kMinFloat = ((float)kMinStorageType) * kRecipFractionalBitsMul;
    static constexpr float kMaxFloat = ((float)kMaxStorageType) * kRecipFractionalBitsMul;

    constexpr FixedPoint()
    {}
    constexpr FixedPoint(float val) : m_storage(floatToStorageType(val))
    {}
    constexpr FixedPoint(int val) : m_storage(clamp(((MathsIntermediateStorageType)val) << kNumFractionalBits))
    {}
    constexpr FixedPoint(unsigned int val) : m_storage(clamp(((MathsIntermediateStorageType)val) << kNumFractionalBits))
    {}
    constexpr FixedPoint(StorageType val) : m_storage(val)
    {}

    template <unsigned int otherNumFractionalBits, typename otherTStorageType, typename otherTMathsIntermediateType, bool otherDoClamping>
    constexpr FixedPoint(const FixedPoint<otherNumFractionalBits, otherTStorageType, otherTMathsIntermediateType, otherDoClamping>& val)
    : m_storage(storageFromOtherFormat(val.getStorage(), otherNumFractionalBits))
    {}

    template <unsigned int otherNumFractionalBits, typename otherTStorageType>
    constexpr FixedPoint(const MathsIntermediateFixedPoint<otherNumFractionalBits, otherTStorageType>& val)
    : m_storage(storageFromOtherFormat(val.getStorage(), otherNumFractionalBits))
    {}

    explicit constexpr operator float() const
    {
        return storageTypeToFloat(m_storage);
    }
    constexpr operator MathsIntermediateType() const
    {
        return { (MathsIntermediateStorageType)m_storage };
    }

    template <unsigned int otherNumFractionalBits, typename otherTStorageType, typename otherTMathsIntermediateType, bool otherDoClamping>
    FixedPoint& operator=(const FixedPoint<otherNumFractionalBits, otherTStorageType, otherTMathsIntermediateType, otherDoClamping>& rhs)
    {
        storeFromOtherFormat(rhs.getStorage(), otherNumFractionalBits);
        return *this;
    }
    FixedPoint& operator=(float rhs)
    {
        m_storage = floatToStorageType(rhs);
        return *this;
    }

    template <unsigned int otherNumFractionalBits, typename otherTStorageType>
    FixedPoint& operator=(const MathsIntermediateFixedPoint<otherNumFractionalBits, otherTStorageType> rhs)
    {
        storeFromOtherFormat(rhs.getStorage(), otherNumFractionalBits);
        return *this;
    }

    constexpr MathsIntermediateType operator+(const FixedPoint& rhs) const
    {
        return (MathsIntermediateType) * this + (MathsIntermediateType)rhs;
    }

    constexpr MathsIntermediateType operator-(const FixedPoint& rhs) const
    {
        return (MathsIntermediateType)m_storage - (MathsIntermediateType)rhs.m_storage;
    }

    constexpr MathsIntermediateType operator*(int rhs) const
    {
        return (MathsIntermediateType)((int)m_storage * rhs);
    }

    constexpr MathsIntermediateType operator*(const FixedPoint& rhs) const
    {
        return (MathsIntermediateType)m_storage * (MathsIntermediateType)rhs.m_storage;
    }

    constexpr MathsIntermediateType operator*(float rhs) const
    {
        return (MathsIntermediateType)m_storage * MathsIntermediateType::fromFloat(rhs);
    }

    constexpr MathsIntermediateType operator/(const FixedPoint& rhs) const
    {
        return (MathsIntermediateType)m_storage / (MathsIntermediateType)rhs.m_storage;
    }

    constexpr MathsIntermediateType operator/(int rhs) const
    {
        return (MathsIntermediateType)((int)m_storage / rhs);
    }

    FixedPoint& operator+=(const FixedPoint& rhs)
    {
        m_storage = clamp(*this + rhs);
        return *this;
    }

    FixedPoint& operator-=(const FixedPoint& rhs)
    {
        m_storage = clamp(*this - rhs);
        return *this;
    }

    FixedPoint& operator*=(const FixedPoint& rhs)
    {
        m_storage = clamp(*this * rhs);
        return *this;
    }

    FixedPoint& operator*=(int rhs)
    {
        m_storage = clamp(*this * rhs);
        return *this;
    }

    FixedPoint& operator/=(const FixedPoint& rhs)
    {
        m_storage = clamp(*this / rhs);
        return *this;
    }

    FixedPoint& operator/=(int rhs)
    {
        m_storage = clamp(*this / rhs);
        return *this;
    }

    constexpr bool operator < (const FixedPoint& rhs) const
    {
        return m_storage < rhs.m_storage;
    }

    constexpr bool operator > (const FixedPoint& rhs) const
    {
        return m_storage > rhs.m_storage;
    }

    constexpr bool operator != (const FixedPoint& rhs) const
    {
        return m_storage != rhs.m_storage;
    }

    MathsIntermediateType sqrt() const
    {
        return MathsIntermediateType((MathsIntermediateStorageType) FixedPointSqrt(m_storage, kNumFractionalBits));
    }

    MathsIntermediateType frac() const
    {
        return MathsIntermediateType((MathsIntermediateStorageType) m_storage & kFractionalBitsMask);
    }

    constexpr StorageType getStorage() const
    {
        return m_storage;
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
        return FixedPoint((StorageType) (SimpleRand() & ((1 << (kNumFractionalBits + 1)) - 1))) - FixedPoint(1);
    }

private:
    static constexpr StorageType floatToStorageType(float val)
    {
        return (StorageType)(clamp(val) * kFractionalBitsMul);
    }

    static constexpr float storageTypeToFloat(StorageType val)
    {
        return ((float)val) * kRecipFractionalBitsMul;
    }

    static constexpr float clamp(float val)
    {
        return doClamping ? ((val >= kMaxFloat) ? kMaxFloat : ((val <= kMinFloat) ? kMinFloat : val)) : val;
    }
    static constexpr StorageType clamp(MathsIntermediateType val)
    {
        return doClamping ?
                   ((val.getStorage() >= kMaxIntermediateType) ?
                        kMaxStorageType :
                        ((val.getStorage() <= kMinIntermediateType) ? kMinStorageType : (StorageType)val.getStorage())) :
                   (StorageType)val.getStorage();
    }

    constexpr StorageType storageFromOtherFormat(MathsIntermediateStorageType src, uint32_t srcNumFractionalBits)
    {
        if(srcNumFractionalBits > kNumFractionalBits)
        {
            src >>= (srcNumFractionalBits - kNumFractionalBits);
        }
        else if(srcNumFractionalBits < kNumFractionalBits)
        {
            src <<= (kNumFractionalBits - srcNumFractionalBits);
        }
        return clamp(MathsIntermediateType(src));
    }
    constexpr void storeFromOtherFormat(MathsIntermediateStorageType src, uint32_t srcNumFractionalBits)
    {
        m_storage = storageFromOtherFormat(src, srcNumFractionalBits);
    }

    StorageType m_storage;
};

void TestFixedPoint();

#endif