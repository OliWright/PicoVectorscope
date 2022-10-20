#include "lookuptable.h"
#include "pico/stdlib.h"
#include <cstdlib>
#include "log.h"

static LogChannel LookupTesting(false);

LookUpTableBase::LookUpTableBase(uint32_t valueSize, uint32_t numValues, bool wrapped, Index endIndex)
: m_table(malloc(numValues * valueSize))
, m_numValues(numValues)
, m_valueSize(valueSize)
, m_endIndex(endIndex)
, m_lookUpToIndex((float) numValues / (float) endIndex)
{}

LookUpTableBase::Index LookUpTableBase::LookUp(Index index, void*& outA, void*& outB) const
{
    Index actualIndex = index * m_lookUpToIndex;
    unsigned int intIndex = ((unsigned int) actualIndex) & (m_numValues - 1);
    unsigned int nextIntIndex = (intIndex + 1) & (m_numValues - 1);
    outA = (uint8_t*) m_table + (intIndex * m_valueSize);
    outB = (uint8_t*) m_table + (nextIntIndex * m_valueSize);
    return actualIndex.frac();
}
