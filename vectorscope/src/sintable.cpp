#include "sintable.h"
#include "pico/float.h"
#include "log.h"

static LogChannel SinTableTesting(false);

SinTable::SinTable()
: LookUpTable<SinTableValue>(1024, true, kPi * 2.f)
{
    SinTableValue* pTable = (SinTableValue*)m_table;
    for(uint32_t i = 0; i < m_numValues; ++i)
    {
        pTable[i] = sinf(kPi * 2.f * i / (float) m_numValues);
    }
}

SinTable SinTable::s_sinTable;
