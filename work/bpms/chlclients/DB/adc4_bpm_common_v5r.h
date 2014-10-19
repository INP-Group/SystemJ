#include "fastadcclient.h"
#include "common_elem_macros.h"

#include "drv_i/nadc4m_drv_i.h"
#include "drv_i/delayunit_drv_i.h"
#include "drv_i/psctrlunit_drv_i.h"

#ifndef SUBSYS_BASENAME
#error "SUBSYS_BASENAME isn't defined"
#endif

#ifndef SUBSYS_ADC004_N
#error "SUBSYS_ADC004_N isn't defined"
#endif

#ifndef SUBSYS_DELUNT_N
#error "SUBSYS_DELUNT_N isn't defined"
#endif

#ifndef SUBSYS_DELCHN_N
#error "SUBSYS_DELCHN_N isn't defined"
#endif

#ifndef SUBSYS_ATNUNT_N
#error "SUBSYS_ATNUNT_N isn't defined"
#endif

#ifndef SUBSYS_ATNCHN_N
#error "SUBSYS_ATNCHN_N isn't defined"
#endif

enum {ADDITIONAL_COUNT = 1};

enum
{
    BIGC_BASE = 0,
    CHAN_BASE = 0, CHANS_PER_DEV = 100,

    DELAY_CHANS_PER_DEV = 10,
    ATTEN_CHANS_PER_DEV = 10,

    DELAY_DEV_BASE = 1600, // four delay line
    ATTEN_DEV_BASE = 1640, // two  atten line
};

#define DELAY_CHAN_W(nd, nl, base) \
    ((base) + (nd) * CHANS_PER_DEV + (nl)) 

#define ATTEN_CHAN_W(nd, nl, base) \
    ((base) + (nd) * CHANS_PER_DEV + (nl)) 

#define DELAY_CHAN_R(nd, nl, base) \
    ((base) + DELAY_CHANS_PER_DEV * (nd) + (nl))

#define ATTEN_CHAN_R(nd, nl, base) \
    ((base) + ATTEN_CHANS_PER_DEV * (nd) + (nl))

#define DELAY_CONTROL_LINE_W(nd, nl) \
    {"del"__CX_STRINGIZE(nd)__CX_STRINGIZE(nl), "Delay", "ns", "%6.3f", NULL, "delay", \
    LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DELAY_CHAN_W(SUBSYS_ADC004_N, NADC4_PARAM_DELAY, CHAN_BASE)), \
    .minnorm=0, .maxnorm=82, .incdec_step=2.0}

#define ATTEN_CONTROL_LINE_W(nd, nl) \
    {"atn"__CX_STRINGIZE(nd)__CX_STRINGIZE(nl), "Atten", "db", "%2.0f", NULL, "atten", \
    LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ATTEN_CHAN_W(SUBSYS_ADC004_N, NADC4_PARAM_ATTEN, CHAN_BASE)), \
    .minnorm=0, .maxnorm=15}

#define DELAY_CONTROL_LINE_R(nd, nl) \
    {"del"__CX_STRINGIZE(nd)__CX_STRINGIZE(nl), "Delay", "ns", "%6.3f", NULL, "delay", \
    LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DELAY_CHAN_R(nd, nl, CHAN_BASE + DELAY_DEV_BASE)), \
    .minnorm=0, .maxnorm=82, .incdec_step=2.0}

#define ATTEN_CONTROL_LINE_R(nd, nl) \
    {"atn"__CX_STRINGIZE(nd)__CX_STRINGIZE(nl), "Atten", "db", "%2.0f", NULL, "atten", \
    LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ATTEN_CHAN_R(nd, nl, CHAN_BASE + ATTEN_DEV_BASE)), \
    .minnorm=0, .maxnorm=15}

static groupunit_t __CX_CONCATENATE(SUBSYS_BASENAME,_grouping)[] =
{
    {
        &(elemnet_t){
            __CX_STRINGIZE(SUBSYS_BASENAME), __CX_STRINGIZE(SUBSYS_BASENAME),
            NULL, NULL,
            ELEM_CANVAS, 1+ADDITIONAL_COUNT, 1,
            (logchannet_t []){
		/* ADC knobplugin */
                {"adc", NULL, NULL, NULL,
                    " width=700 save_button maxfrq=10"
                    " subsys=\"v5r_bpm"__CX_STRINGIZE(SUBSYS_ADC004_N)"\"",
                 NULL, LOGT_READ, LOGD_NADC4, LOGK_DIRECT,
                 /*!!! A hack: we use 'color' field to pass integer */
                 BIGC_BASE + (SUBSYS_ADC004_N),
                 PHYS_ID(CHAN_BASE + (SUBSYS_ADC004_N) * CHANS_PER_DEV),
                 .placement="left=0,right=0,top=0,bottom=0@add"
		},

		/* Additional control parameters */
		SUBELEM_START("add", "Additional Parameters", 6, 3)
	            SUBELEM_START("del_w", NULL, 1, 1)
			DELAY_CONTROL_LINE_W(SUBSYS_DELUNT_N, SUBSYS_DELCHN_N),
	            SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
		    HLABEL_CELL("   "),
	            SUBELEM_START("atn_w", NULL, 1, 1)
			ATTEN_CONTROL_LINE_W(SUBSYS_ATNUNT_N, SUBSYS_ATNCHN_N),
	            SUBELEM_END("notitle,noshadow,nocoltitles", NULL),

	            SUBELEM_START("del_r", NULL, 1, 1)
			DELAY_CONTROL_LINE_R(SUBSYS_DELUNT_N, SUBSYS_DELCHN_N),
	            SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
		    HLABEL_CELL("   "),
	            SUBELEM_START("atn_r", NULL, 1, 1)
			ATTEN_CONTROL_LINE_R(SUBSYS_ATNUNT_N, SUBSYS_ATNCHN_N),
	            SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
		SUBELEM_END("notitle,noshadow,nocoltitles", "left=0,bottom=0"),

            },
            "notitle,noshadow"
        }, GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT
    },
        
    {NULL}
};

static physprops_t __CX_CONCATENATE(SUBSYS_BASENAME,_physinfo)[] =
{
    {DELAY_CHAN_W(SUBSYS_ADC004_N, NADC4_PARAM_DELAY, CHAN_BASE               ), 1.e+3, 0, 200},
    {ATTEN_CHAN_W(SUBSYS_ADC004_N, NADC4_PARAM_ATTEN, CHAN_BASE               ), 1.e+0, 0, 0},

    {DELAY_CHAN_R(SUBSYS_DELUNT_N, SUBSYS_DELCHN_N, CHAN_BASE + DELAY_DEV_BASE), 1.e+3, 0, 200},
    {ATTEN_CHAN_R(SUBSYS_ATNUNT_N, SUBSYS_ATNCHN_N, CHAN_BASE + ATTEN_DEV_BASE), 1.e+0, 7, 0},
};

#undef SUBSYS_BASENAME
#undef SUBSYS_ADC004_N
#undef SUBSYS_DELUNT_N
#undef SUBSYS_DELCHN_N
#undef SUBSYS_ATNUNT_N
#undef SUBSYS_ATNCHN_N
