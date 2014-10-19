#include "tsycamv_data.h"
#include "drv_i/tsycamv_drv_i.h"


// What's missing from tsycamv_drv_i.h
typedef uint32 TSYCAMV_DATUM_T;
enum
{
    TSYCAMV_MAX_W = 660,
    TSYCAMV_MAX_H = 500,
    TSYCAMV_CUT_L = 1, TSYCAMV_CUT_R = 1,
    TSYCAMV_CUT_T = 1, TSYCAMV_CUT_B = 10,
    TSYCAMV_SHOW_W = TSYCAMV_MAX_W - TSYCAMV_CUT_L - TSYCAMV_CUT_R,
    TSYCAMV_SHOW_H = TSYCAMV_MAX_H - TSYCAMV_CUT_T - TSYCAMV_CUT_B,

    TSYCAMV_DATAUNITS = sizeof(TSYCAMV_DATUM_T),

    TSYCAMV_SRCMAXVAL = 1023,
    TSYCAMV_BPP       = 10,
};

enum
{
    TSYCAMV_DTYPE = ENCODE_DTYPE(TSYCAMV_DATAUNITS,
                                 CXDTYPE_REPR_INT, 1)
};

static psp_paramdescr_t text2tsycamv_params[] =
{
    PSP_P_INT    ("k",        pzframe_params_t, p[TSYCAMV_PARAM_K],        -1, 0, 255),
    PSP_P_INT    ("t",        pzframe_params_t, p[TSYCAMV_PARAM_T],        -1, 0, 511),
    PSP_P_INT    ("waittime", pzframe_params_t, p[TSYCAMV_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[TSYCAMV_PARAM_STOP],     -1, 1),

    PSP_P_FLAG   ("sync",     pzframe_params_t, p[TSYCAMV_PARAM_SYNC],      1, 0),
    PSP_P_FLAG   ("immed",    pzframe_params_t, p[TSYCAMV_PARAM_SYNC],      0, 0),

    PSP_P_END(),
};

static pzframe_paramdscr_t tsycamv_param_dscrs[] =
{
    {"waittime",  TSYCAMV_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      TSYCAMV_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},

    {"k",         TSYCAMV_PARAM_K,          0,    0, 0, PZFRAME_PARAM_BIGC},
    {"t",         TSYCAMV_PARAM_T,          0,    0, 0, PZFRAME_PARAM_BIGC},
    {"sync",      TSYCAMV_PARAM_SYNC,       0,    0, 0, PZFRAME_PARAM_BIGC},
    {"miss",      TSYCAMV_PARAM_MISS,       0,    0, 0, PZFRAME_PARAM_BIGC},
//    {"elapsed",   TSYCAMV_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0},
};

pzframe_type_dscr_t *tsycamv_get_type_dscr(void)
{
  static vcamimg_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        VcamimgDataFillStdDscr(&dscr, "tsycamv",
                               0,
                               /* Bigc-related ADC characteristics */
                               TSYCAMV_DTYPE,
                               TSYCAMV_NUM_PARAMS,
                               TSYCAMV_MAX_W,  TSYCAMV_MAX_H,
                               TSYCAMV_SHOW_W, TSYCAMV_SHOW_H,
                               TSYCAMV_CUT_L,  TSYCAMV_CUT_T,
                               TSYCAMV_BPP,
                               TSYCAMV_SRCMAXVAL,
                               NULL/*info2mes*/,
                               /* Description of bigc-parameters */
                               text2tsycamv_params,
                               tsycamv_param_dscrs
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
