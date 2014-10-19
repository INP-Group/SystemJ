#ifndef __TANTALCLIENT_H
#define __TANTALCLIENT_H


enum
{
    LOGD_TANTAL_PLOT   = 't' + ('n'<<8) + ('t'<<16) + ('l'<<24),
};

enum
{
    REG_TANTAL_FROM_I    = 1,
    REG_TANTAL_TO_I      = 2,
    REG_TANTAL_STEP_TIME = 3,
    REG_TANTAL_NSTEPS    = 4,
    REG_TANTAL_CUR_TIME  = 5,
    REG_TANTAL_CUR_STEP  = 6,
    REG_TANTAL_MANUAL    = 7,
    REG_TANTAL_GOING     = 8,
    REG_TANTAL_C_GO      = 9,
    REG_TANTAL_C_STOP    = 10,
};

enum
{
    TANTAL_MAXPLOTSIZE = 1000,
};


#define TANTAL_I_FMT "%7.3f"


#endif /* __TANTALCLIENT_H */
