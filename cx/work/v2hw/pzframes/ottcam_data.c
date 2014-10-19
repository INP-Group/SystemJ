#include "ottcam_data.h"
#include "drv_i/ottcam_drv_i.h"


enum
{
    OTTCAM_DTYPE = ENCODE_DTYPE(OTTCAM_DATAUNITS,
                                CXDTYPE_REPR_INT, 1)
};

static psp_paramdescr_t text2ottcam_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[OTTCAM_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[OTTCAM_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[OTTCAM_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[OTTCAM_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[OTTCAM_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[OTTCAM_PARAM_STOP],     -1, 1),

    PSP_P_INT    ("k",        pzframe_params_t, p[OTTCAM_PARAM_K],        -1, 16, 64),
    PSP_P_INT    ("t",        pzframe_params_t, p[OTTCAM_PARAM_T],        -1, 0, 65535),
    PSP_P_FLAG   ("sync",     pzframe_params_t, p[OTTCAM_PARAM_SYNC],      1, 0),
    PSP_P_FLAG   ("immed",    pzframe_params_t, p[OTTCAM_PARAM_SYNC],      0, 0),

    PSP_P_END(),
};

static pzframe_paramdscr_t ottcam_param_dscrs[] =
{
    {"shot",      OTTCAM_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    OTTCAM_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  OTTCAM_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      OTTCAM_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},

    {"k",         OTTCAM_PARAM_K,          0,    0, 0, PZFRAME_PARAM_BIGC},
    {"t",         OTTCAM_PARAM_T,          0,    0, 0, PZFRAME_PARAM_BIGC},
    {"sync",      OTTCAM_PARAM_SYNC,       0,    0, 0, PZFRAME_PARAM_BIGC},
    {"rrq_msecs", OTTCAM_PARAM_RRQ_MSECS,  0,    0, 0, PZFRAME_PARAM_BIGC},
    {"miss",      OTTCAM_PARAM_MISS,       0,    0, 0, PZFRAME_PARAM_BIGC},
    {"elapsed",   OTTCAM_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0},
};

pzframe_type_dscr_t *ottcam_get_type_dscr(void)
{
  static vcamimg_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        VcamimgDataFillStdDscr(&dscr, "ottcam",
                               0,
                               /* Bigc-related ADC characteristics */
                               OTTCAM_DTYPE,
                               OTTCAM_NUM_PARAMS,
                               OTTCAM_MAX_W,   OTTCAM_MAX_H,
                               OTTCAM_MAX_W-1, OTTCAM_MAX_H,
                               0,              0,
                               OTTCAM_BPP,
                               OTTCAM_SRCMAXVAL,
                               NULL/*info2mes*/,
                               /* Description of bigc-parameters */
                               text2ottcam_params,
                               ottcam_param_dscrs
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
