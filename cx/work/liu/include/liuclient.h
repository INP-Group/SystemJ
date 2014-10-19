#ifndef __LIUCLIENT_H
#define __LIUCLIENT_H


#include "misc_macros.h"


#if defined(D) || defined(V)
  #error Sorry, unable to compile when D or V defined
  #error (see warning(s) below for details).
#endif

#ifdef __LIUCLIENT_C
  #define D
  #define V(value) = value
#else
  #define D extern
  #define V(value)
#endif /* __LIUCLIENT_C */

enum
{
    LOGD_LIU_ADC200ME_ONE      = 30000,
    LOGD_LIU_ADC200ME_ANY      = 30001,
    LOGD_LIU_ADC812ME_ONE      = 30002,
    LOGD_LIU_TWO812CH          = 30003,
    LOGD_LIU_BEAMPROF          = 30004,
    LOGD_LIU_AVGDIFF           = 30005,
    LOGD_LIU_ADC200ME_ONE_CALC = 30100,
    LOGD_LIU_GLOBACT           = 31000,
    LOGD_LIU_KEY_OFFON         = 31001,
};

#define UBS_HEAT_FILE_NAME(N, M, x) \
    "liucc_ubs_" __CX_STRINGIZE(N) M "_heat" __CX_STRINGIZE(x) "time.val"
#define UBS_HEAT_REG_NAME( RACK_N, UBS_M, c, x, t) \
    __CX_CONCATENATE(REG_R, __CX_CONCATENATE(__CX_CONCATENATE(RACK_N,UBS_M), __CX_CONCATENATE(__CX_CONCATENATE(_, __CX_CONCATENATE(c, x)), t)))

#define UBS_HEAT_ALWD_DELTA 0.03

enum
{
    REG_TBGI_ON_TIME  = 80,

    REG_ALLDL200_CURSHIFT = 81,
    REG_ALLDL200_NEWSHIFT = 82,

    REG_T16_SLOW_AUTO = 90,
    REG_T16_SLOW_SECS = 91,
    REG_T16_SLOW_PREV = 92,
    REG_T16_SLOW_CNTR = 93,
    REG_T16_SLOW_CGRD = 94,
    REG_T16_SLOW_WPRV = 95,
    REG_T16_SLOW_HTGR = 96,

    REG_CALB_ADC200ME = 97,
    REG_ADC200ME_NUMPTS = 98,

#define DEFINE_UBS_N_M_REGS(RACK_N, UBS_M) \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,IH,2,N) = 200 + 12*RACK_N + 2*UBS_M + 0, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,IH,1,N) = 200 + 12*RACK_N + 2*UBS_M + 1, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,ST,2,T) = 300 + 36*RACK_N + 6*UBS_M + 0, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,ST,2,W) = 300 + 36*RACK_N + 6*UBS_M + 1, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,ST,2,P) = 300 + 36*RACK_N + 6*UBS_M + 2, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,ST,1,T) = 300 + 36*RACK_N + 6*UBS_M + 3, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,ST,1,W) = 300 + 36*RACK_N + 6*UBS_M + 4, \
    UBS_HEAT_REG_NAME(RACK_N,UBS_M,ST,1,P) = 300 + 36*RACK_N + 6*UBS_M + 5

#define DEFINE_RACK_N_REGS(N)                   \
    __CX_CONCATENATE(REG_R, __CX_CONCATENATE(N,_T16_FAST_AUTO)) = 100 + 10*N + 0, \
    __CX_CONCATENATE(REG_R, __CX_CONCATENATE(N,_T16_FAST_SECS)) = 100 + 10*N + 1, \
    __CX_CONCATENATE(REG_R, __CX_CONCATENATE(N,_T16_FAST_PREV)) = 100 + 10*N + 2, \
    DEFINE_UBS_N_M_REGS(N, 1),                                                    \
    DEFINE_UBS_N_M_REGS(N, 2),                                                    \
    DEFINE_UBS_N_M_REGS(N, 3),                                                    \
    DEFINE_UBS_N_M_REGS(N, 4),                                                    \
    DEFINE_UBS_N_M_REGS(N, 5),                                                    \
    DEFINE_UBS_N_M_REGS(N, 6)

    DEFINE_RACK_N_REGS(1),
    DEFINE_RACK_N_REGS(2),
    DEFINE_RACK_N_REGS(3),
    DEFINE_RACK_N_REGS(4),
    DEFINE_RACK_N_REGS(5),
    DEFINE_RACK_N_REGS(6),
    DEFINE_RACK_N_REGS(7),
    DEFINE_RACK_N_REGS(8),

#undef DEFINE_RACK_N_REGS
#undef DEFINE_UBS_N_M_REGS
};


D int manyadcs_x1 V(0);
D int manyadcs_x2 V(0);


#undef D
#undef V


#endif /* __LIUCLIENT_H */
