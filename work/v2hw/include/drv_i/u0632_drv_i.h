#ifndef __U0632_DRV_I_H
#define __U0632_DRV_I_H


/* Note: U0632 is Repkov's KIPP */
/* Recommended blklist main_info+bigc_info: r960,w30,w30 30*2+0i/2+32i */

enum
{
    U0632_MAXUNITS          = 30,  // 14 boxes per line, 2 lines
    U0632_CHANS_PER_UNIT    = 32,  // X and Y, 15 wires per X/Y

    U0632_CHAN_INTDATA_BASE = 0,
    U0632_CHAN_M_BASE       = U0632_CHANS_PER_UNIT*U0632_MAXUNITS,
    U0632_CHAN_P_BASE       = U0632_CHAN_M_BASE + U0632_MAXUNITS,
    
    U0632_NUMCHANS          = U0632_CHANS_PER_UNIT*U0632_MAXUNITS // Int data
                            + U0632_MAXUNITS*2                    // M,P
};

enum
{
    U0632_PARAM_M = 0,
    U0632_PARAM_P = 1,

    U0632_NUM_PARAMS = 2
};

enum
{
    U0632_NUM_BIGCS = U0632_MAXUNITS
};

typedef int32 U0632_DATUM_T;
enum
{
    U0632_DATAUNITS = sizeof(U0632_DATUM_T),
};


#endif /* __U0632_DRV_I_H */
