#pragma once
#include <cstdint>
#include "fixedpoint.h"

class LookUpTableBase
{
public:
    typedef FixedPoint<12, uint32_t, uint32_t, false> Index;

    LookUpTableBase(uint32_t valueSize, uint32_t numValues, bool wrapped, Index endIndex);

    Index LookUp(Index lookupIndex, void*& outA, void*& outB) const;

protected:
    void*     m_table;
    uint32_t  m_numValues;
    uint32_t  m_valueSize;
    Index     m_endIndex;
    Index     m_lookUpToIndex;
};

template<typename T>
class LookUpTable : public LookUpTableBase
{
public:
    typedef T ValueType;

    LookUpTable(uint32_t numValues, bool wrapped, Index endIndex)
    : LookUpTableBase(sizeof(ValueType), numValues, wrapped, endIndex)
    {}

    ValueType LookUp(LookUpTableBase::Index index) const
    {
        void* pA;
        void* pB;
        Index interpolation = LookUpTableBase::LookUp(index, pA, pB);
        const ValueType& a = *(const ValueType*)pA;
        const ValueType& b = *(const ValueType*)pB;
        return ((b - a) * (ValueType)interpolation) + a;
    }
};
