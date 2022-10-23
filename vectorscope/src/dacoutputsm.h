#ifndef VECTORPIO_H
#define VECTORPIO_H

#include "dacoutpioconfig.h"

class DacOutputSm
{
public:
    static void Init();

    static const DacOutputPioSmConfig& Idle()   { return s_configs[(int) SmID::eIdle]; }
    static const DacOutputPioSmConfig& Vector() { return s_configs[(int) SmID::eVector]; }
    static const DacOutputPioSmConfig& Points() { return s_configs[(int) SmID::ePoints]; }
    static const DacOutputPioSmConfig& Raster() { return s_configs[(int) SmID::eRaster]; }

private:
    enum class SmID
    {
        eIdle,
        eVector,
        ePoints,
        eRaster,

        eCount
    };

    static void configureSm(SmID smID);

private:
    static DacOutputPioSmConfig s_configs[(int)SmID::eCount];
};

#endif
