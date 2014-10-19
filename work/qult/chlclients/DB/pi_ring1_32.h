#ifndef PHYSINFO_ARRAY_NAME
  #error Please supply the name for physinfo[] array via "#define PHYSINFO_ARRAY_NAME NAME"
#endif


#ifndef __PI_RING1_32_H
#define __PI_RING1_32_H

#include "magx_macros.h"


enum
{
    B_COR4016    = 0,
    B_COR208     = 3600,

    B_VCH400_ADC = 4972,
    B_VCH400_DAC = 5272,
    B_VCH400_V3H = 5572,

    B_ISTS       = 5732,
};

#define B_IST_C20(n) (B_ISTS+(XCDAC20_NUMCHANS+IST_XCDAC20_NUMCHANS)*(n) + 0)
#define B_IST_IST(n) (B_ISTS+(XCDAC20_NUMCHANS+IST_XCDAC20_NUMCHANS)*(n) + XCDAC20_NUMCHANS)

#endif /* __PI_RING1_32_H */


static physprops_t PHYSINFO_ARRAY_NAME[] =
{
#define XOR4016_PHYS(n) \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+0),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+1),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+2),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+3),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+4),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+5),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+6),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+7),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+8),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+9),  \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+10), \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+11), \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+12), \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+13), \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+14), \
    MAGX_XOR4016_PHYSLINES(B_COR4016, (n)*16+15)
    
    XOR4016_PHYS(0),
    XOR4016_PHYS(1),
    XOR4016_PHYS(2),
    XOR4016_PHYS(3),
    XOR4016_PHYS(4),
    XOR4016_PHYS(5),

#define COR208_PHYS(n) \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 0), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 1), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 2), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 3), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 4), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 5), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 6), \
    MAGX_COR208_PHYSLINES(B_COR208 + ((n) * XCAC208_NUMCHANS), 7)

    COR208_PHYS(0),
    COR208_PHYS(1),
    COR208_PHYS(2),
    COR208_PHYS(3),
    COR208_PHYS(4),
    COR208_PHYS(5),
    COR208_PHYS(6),
    COR208_PHYS(7),
    COR208_PHYS(8),
    COR208_PHYS(9),
    COR208_PHYS(10),
    COR208_PHYS(11),

    MAGX_XCH300_PHYSLINES(B_VCH400_ADC, B_VCH400_DAC, B_VCH400_V3H),

#define RING1_32_IST_PHYS_R(n, r) \
    MAGX_IST_XCDAC20_PHYSLINES(B_IST_C20(n), B_IST_IST(n),               \
                               r,  +1)
#define RING1_32_IST_PHYS(n) RING1_32_IST_PHYS_R(n, 1000000.0/125)

#define RING1_32_X1K_PHYS(n) \
    MAGX_X1K_XCDAC20_PHYSLINES(B_IST_C20(n), B_IST_IST(n), 1000000.0/125)
    RING1_32_X1K_PHYS(0),  // ip2_1l0
    RING1_32_X1K_PHYS(1),  // ip2_1l1
    RING1_32_X1K_PHYS(2),  // ip2_1l2
    RING1_32_X1K_PHYS(3),  // ip2_1l3
    RING1_32_X1K_PHYS(4),  // ip2_1l4

    RING1_32_IST_PHYS(5),  // ip1_3m4
    RING1_32_IST_PHYS(6),  // ip1_4m4
    RING1_32_IST_PHYS(7),  // ip1_3m1
    RING1_32_IST_PHYS(8),  // ip1_4m1
    RING1_32_IST_PHYS(9),  // ip1_r14
    RING1_32_IST_PHYS(10), // ip1_d1
    RING1_32_IST_PHYS(11), // ip1_sm1
    RING1_32_IST_PHYS(12), // ip1_1l7
    RING1_32_IST_PHYS(13), // ip1_r8
    RING1_32_IST_PHYS(14), // ip1_r9
    RING1_32_IST_PHYS(15), // ip1_rm1
    RING1_32_IST_PHYS(16), // ip1_1m5
    RING1_32_IST_PHYS(17), // ip1_d3
    RING1_32_IST_PHYS(18), // ip1_d2
    RING1_32_IST_PHYS(19), // ip1_f4
    RING1_32_IST_PHYS(20), // ip1_f3
    RING1_32_IST_PHYS(21), // ip1_f1
    RING1_32_IST_PHYS(22), // ip1_i12
    RING1_32_IST_PHYS_R(23, 1000000.0/250), // ip1_i13
    RING1_32_IST_PHYS_R(24, 1000000.0/250), // ip1_i14

};
