#pragma once
#include "lookuptable.h"
#include "types.h"

typedef FixedPoint<1, 14,int16_t,int32_t> SinTableValue;

class SinTable : public LookUpTable<SinTableValue>
{
public:
    SinTable();

    static SinTableValue LookUp(Index angle)
    {
        const LookUpTable<SinTableValue>& st = s_sinTable;
        return st.LookUp(angle);
    }

    static void SinCos(Index angle, SinTableValue& outS, SinTableValue& outC)
    {
        const LookUpTable<SinTableValue>& st = s_sinTable;
        outS = st.LookUp(angle);
        outC = st.LookUp(angle + Index(kPi/2.f));
    }

private:
    static SinTable s_sinTable;
};
