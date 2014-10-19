#include "fastadcclient.h"
#include "common_elem_macros.h"

#include "drv_i/c0642_drv_i.h"
#include "drv_i/v0628_drv_i.h"

#ifndef SUBSYS_BASENAME
#error "SUBSYS_BASENAME isn't defined"
#endif

#ifndef SUBSYS_ADC502_N
#error "SUBSYS_ADC004_N isn't defined"
#endif

#ifndef SUBSYS_C0642_N
#error "SUBSYS_C0642_N isn't defined"
#endif

#ifndef SUBSYS_C0642_AMPL
#error "SUBSYS_C0642_AMPL isn't defined"
#endif

#ifndef SUBSYS_C0642_PHAS
#error "SUBSYS_C0642_PHAS isn't defined"
#endif

#ifndef SUBSYS_V0628_N
#error "SUBSYS_V0628_N isn't defined"
#endif

#ifndef SUBSYS_V0628_SYNC
#error "SUBSYS_V0628_SYNC isn't defined"
#endif

#ifndef SUBSYS_V0628_SLOP
#error "SUBSYS_V0628_SLOP isn't defined"
#endif

enum {ADDITIONAL_COUNT = 1};

enum
{
    BIGC_BASE = 0,
    CHAN_BASE = 0,   CHANS_PER_DEV = 100, /* 100 */

    C0642_CHANS_PER_DEV = C0642_NUMCHANS, /* 16  */
    V0628_CHANS_PER_DEV = V0628_NUMCHANS, /* 58  */

    C0642_BASE = CHAN_BASE + 200,
    V0628_BASE = CHAN_BASE + 220,
};

#define C0642_CHAN(nd, nl, base) \
    (base + C0642_CHANS_PER_DEV * (nd) + nl)

#define V0628_CHAN(nd, nl, base) \
    (base + V0628_CHANS_PER_DEV * (nd) + nl)

#define C0642_CONTROL_LINE(nd, base) \
    SUBELEM_START("c0642", NULL, 2, 2) \
        {"amp", "Ampl.", "V", "%6.4f", "withlabel", "Amplitude of RF swiper", \
             LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0642_CHAN(nd, SUBSYS_C0642_AMPL, base)), \
             .minnorm=0, .maxnorm=9, .incdec_step = 0.1, .defnorm=2 }, \
	{"phz", "Phase", "V", "%6.4f", "withlabel", "Phase     of RF swiper", \
             LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(C0642_CHAN(nd, SUBSYS_C0642_PHAS, base)), \
             .minnorm=0, .maxnorm=9, .incdec_step = 0.1, .defnorm=2 }  \
    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL)

#define V0628_CONTROL_LINE(nd, base) \
    SUBELEM_START("v0628", NULL, 2, 2) \
        {"sync", "Sync",  NULL, NULL, "", "Switch OFF linear swiper", \
             LOGT_WRITE1, LOGD_BLUELED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0628_CHAN(nd, SUBSYS_V0628_SYNC, base))}, \
        {"slop", "Slope", NULL, NULL, "", "Decrease slope of linear swiper by 1/2",  \
             LOGT_WRITE1, LOGD_BLUELED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(V0628_CHAN(nd, SUBSYS_V0628_SLOP, base))}  \
    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL)

static groupunit_t __CX_CONCATENATE(SUBSYS_BASENAME,_grouping)[] =
{
    {
        &(elemnet_t){
            __CX_STRINGIZE(SUBSYS_BASENAME), __CX_STRINGIZE(SUBSYS_BASENAME),
            NULL, NULL,
            ELEM_CANVAS, 1+ADDITIONAL_COUNT, 1,
            (logchannet_t []){
                {"adc", NULL, NULL, NULL,
                    " width=700 save_button maxfrq=50"
                    " waittime=2000000 noistart"
                    " ptsofs=0 numpts=32767"
                    " range0=1024 range1=1024"
                    " timing=ext tmode=lf"
                    " subsys=\"nadc502_based_"__CX_STRINGIZE(SUBSYS_ADC502_N)"\"",
                 NULL, LOGT_READ, LOGD_NADC502, LOGK_DIRECT,
                 /*!!! A hack: we use 'color' field to pass integer */
                 BIGC_BASE + (SUBSYS_ADC502_N),
                 PHYS_ID(CHAN_BASE + (SUBSYS_ADC502_N) * CHANS_PER_DEV),
                 .placement="left=0,right=0,top=0,bottom=0@add"
		},

		/* Additional control parameters */
		SUBELEM_START("add", "Additional Parameters", 3, 3)
		    C0642_CONTROL_LINE(SUBSYS_C0642_N, C0642_BASE),
		    HLABEL_CELL("     "),
		    V0628_CONTROL_LINE(SUBSYS_V0628_N, V0628_BASE)
		SUBELEM_END("notitle,noshadow,nocoltitles", "left=0,bottom=0"),

	    },
            "notitle,noshadow"
        }, GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT
    },
        
    {NULL}
};

static physprops_t __CX_CONCATENATE(SUBSYS_BASENAME,_physinfo)[] =
{
    {C0642_CHAN(SUBSYS_C0642_N, SUBSYS_C0642_AMPL, C0642_BASE), 1000000.0, 0, 305},
    {C0642_CHAN(SUBSYS_C0642_N, SUBSYS_C0642_PHAS, C0642_BASE), 1000000.0, 0, 305},
};

#undef SUBSYS_BASENAME
#undef SUBSYS_ADC004_N

#undef SUBSYS_C0642_N
#undef SUBSYS_C0642_AMPL
#undef SUBSYS_C0642_PHAS

#undef SUBSYS_V0628_N
#undef SUBSYS_V0628_SYNC
#undef SUBSYS_V0628_SLOP
