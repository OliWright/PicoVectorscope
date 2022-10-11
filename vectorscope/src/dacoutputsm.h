#ifndef VECTORPIO_H
#define VECTORPIO_H

#include "dacoutpioconfig.h"

class DacOutputSm
{
public:
    static void Init();

    static const DacOutputPioSmConfig& Vector() { return s_configs[(int) SmID::eVector]; }
    static const DacOutputPioSmConfig& Points() { return s_configs[(int) SmID::ePoints]; }
    static const DacOutputPioSmConfig& Idle()   { return s_configs[(int) SmID::eIdle]; }

private:
    enum class SmID
    {
        eIdle,
        eVector,
        ePoints,

        eCount
    };

    static void configureSm(SmID smID);

private:
    static DacOutputPioSmConfig s_configs[(int)SmID::eCount];
};

#endif
