#ifndef PHYSINFO_ARRAY_NAME
  #error Please supply the name for physinfo[] array via "#define PHYSINFO_ARRAY_NAME NAME"
#endif


#ifndef __PI_LINAC1_10_H
#define __PI_LINAC1_10_H


#include "drv_i/nadc200_drv_i.h"
#include "drv_i/sukh_comm_drv_i.h"
#include "drv_i/c0609_drv_i.h"
#include "drv_i/c0642_drv_i.h"


enum
{
    C0609_BASE        = 0,
    C0642_BASE        = 64,
    COMM_BASE         = 80,
    NADC200_CHAN_BASE = 84, NADC200_CHANS_PER_DEV = NADC200_NUM_PARAMS*0+100,
    NADC200_BIGC_BASE = 0,
};


#endif /* __PI_LINAC1_10_H */


static physprops_t PHYSINFO_ARRAY_NAME[] =
{
    {C0609_BASE+C0609_CHAN_ADC_n_base +  0, 1000000.0, 0, 0},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  0, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  1, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  2, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  3, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  4, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  5, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  6, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  7, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  8, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base +  9, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base + 10, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base + 11, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base + 12, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base + 13, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base + 14, 1000000.0, 0, 305},
    {C0642_BASE+C0642_CHAN_OUT_n_base + 15, 1000000.0, 0, 305},
};
