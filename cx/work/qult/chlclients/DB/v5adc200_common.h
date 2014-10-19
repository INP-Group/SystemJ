#include "fastadcclient.h"
#include "drv_i/l0403_drv_i.h"

#include "common_elem_macros.h"


#ifndef SUBSYS_BASENAME
#error "SUBSYS_BASENAME isn't defined"
#endif

#ifndef SUBSYS_ADC200_N
#error "SUBSYS_ADC200_N isn't defined"
#endif

#ifdef  SUBSYS_SELECTOR
enum {SELECTOR_COUNT = 1};
#else
enum {SELECTOR_COUNT = 0};
#endif

enum
{
    BIGC_BASE = 0,
    CHAN_BASE = 49, CHANS_PER_DEV = 100,
};

enum
{
    WAVESEL_L0403_N1_BASE = 25,
    WAVESEL_L0403_N2_BASE = 31,
    WAVESEL_L0403_N3_BASE = 37,
};


static groupunit_t __CX_CONCATENATE(SUBSYS_BASENAME,_grouping)[] =
{
    {
        &(elemnet_t){
            __CX_STRINGIZE(SUBSYS_BASENAME), __CX_STRINGIZE(SUBSYS_BASENAME),
            NULL, NULL,
            ELEM_CANVAS, 1+SELECTOR_COUNT, 1,
            (logchannet_t []){
                {"adc", NULL, NULL, NULL,
                    " width=700 save_button maxfrq=10"
                    " subsys=\"v5adc200-"__CX_STRINGIZE(SUBSYS_ADC200_N)"\"",
                 NULL, LOGT_READ, LOGD_NADC200, LOGK_DIRECT,
                 /*!!! A hack: we use 'color' field to pass integer */
                 BIGC_BASE + (SUBSYS_ADC200_N),
                 PHYS_ID(CHAN_BASE + (SUBSYS_ADC200_N) * CHANS_PER_DEV),
                 .placement="left=0,right=0,top=0,bottom=0"
#ifdef SUBSYS_SELECTOR
                 "@selector"
#endif
                },
#ifdef SUBSYS_SELECTOR
                SUBSYS_SELECTOR
#endif
            },
            "notitle,noshadow"
        }, GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT
    },
        
    {NULL}
};


static physprops_t __CX_CONCATENATE(SUBSYS_BASENAME,_physinfo)[] =
{
};


#undef SUBSYS_BASENAME
#undef SUBSYS_ADC200_N
#undef SUBSYS_SELECTOR
