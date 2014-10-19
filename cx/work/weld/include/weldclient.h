#ifndef __WELDCLIENT_H
#define __WELDCLIENT_H


enum
{
    ELEM_WELD_DUMMY = 4000,
};

enum
{
    LOGD_WELD_PROCESS = 30000,
};

enum
{
    REG_T_UP    = 30,
    REG_T_CONST = 31,
    REG_T_DOWN  = 32,

    REG_SET_UH  = 40,
    REG_SET_UL  = 41,
    REG_SET_UN  = 42,

    REG_WATCH        = 50,
    REG_WATCH_BDPREV = 51,
    REG_WATCH_TIME   = 52,
    
    REG_WELD_START  = 100,
    REG_WELD_STOP   = 101,
    REG_WELD_STEP_N = 102,

    REG_WELD_FR_X = 110,
    REG_WELD_FR_Y = 111,
    REG_WELD_TO_X = 112,
    REG_WELD_TO_Y = 113,
    REG_WELD_BY_X = 112,
    REG_WELD_BY_Y = 113,
    REG_WELD_RPT  = 114,

    R_RESET_TIME  = 200,
};


#endif /* __WELDCLIENT_H */
