#include "nadc4_data.h"
#include "drv_i/nadc4_drv_i.h"


enum
{
    NADC4_DTYPE = ENCODE_DTYPE(NADC4_DATAUNITS,
                               CXDTYPE_REPR_INT, 0)
};

static void nadc4_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int               nl;

    for (nl = 0;  nl < NADC4_NUM_LINES;  nl++)
    {
        mes_p->plots[nl].on                 = 1;
        mes_p->plots[nl].numpts             = mes_p->inhrt->info[NADC4_PARAM_NUMPTS];
        mes_p->plots[nl].cur_range.int_r[0] = NADC4_MIN_VALUE;
        mes_p->plots[nl].cur_range.int_r[1] = NADC4_MAX_VALUE;
        FastadcSymmMinMaxInt(mes_p->plots[nl].cur_range.int_r + 0,
                             mes_p->plots[nl].cur_range.int_r + 1);
        mes_p->plots[nl].cur_int_zero       = 0;

        mes_p->plots[nl].x_buf              =
            ((uint8 *)(mes_p->inhrt->databuf)) +
            mes_p->inhrt->info[NADC4_PARAM_NUMPTS] * NADC4_DATAUNITS * nl;
    }

    mes_p->exttim = 1;
}

static double nadc4_raw2pvl(int32 *info __attribute__((unused)),
                            int    nl   __attribute__((unused)),
                            int    raw)
{
    return raw;
}

static int    nadc4_x2xs(int32 *info __attribute__((unused)), int x)
{
    return x;
}

static psp_paramdescr_t text2nadc4_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[NADC4_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[NADC4_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[NADC4_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[NADC4_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[NADC4_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[NADC4_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[NADC4_PARAM_PTSOFS],   -1, 0, NADC4_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[NADC4_PARAM_NUMPTS],   -1, 1, NADC4_MAX_NUMPTS),

    PSP_P_END(),
};

static pzframe_paramdscr_t nadc4_param_dscrs[] =
{
    {"shot",      NADC4_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    NADC4_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  NADC4_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      NADC4_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    NADC4_PARAM_PTSOFS,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    NADC4_PARAM_NUMPTS,     0,    0, 0, PZFRAME_PARAM_BIGC},
    {"calcstats", NADC4_PARAM_CALC_STATS, 0,    0, 0, PZFRAME_PARAM_INDIFF    | PZFRAME_PARAM_RW_ONLY_MASK},
    {"elapsed",   NADC4_PARAM_ELAPSED,    1000, 0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *nadc4_line_name_list[] = {"0", "1", "2", "3"};
static const char *nadc4_line_unit_list[] = {"x", "x", "x", "x"};

pzframe_type_dscr_t *nadc4_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "nadc4",
                               0,
                               /* Bigc-related ADC characteristics */
                               NADC4_DTYPE,
                               NADC4_NUM_PARAMS,
                               NADC4_MAX_NUMPTS, NADC4_NUM_LINES,
                               NADC4_PARAM_NUMPTS,
                               nadc4_info2mes,
                               /* Description of bigc-parameters */
                               text2nadc4_params,
                               nadc4_param_dscrs,
                               /* Data specifics */
                               "Channel", nadc4_line_name_list, nadc4_line_unit_list,
                               (plotdata_range_t){.int_r={NADC4_MIN_VALUE,
                                                          NADC4_MAX_VALUE}},
                               "% 4.0f", nadc4_raw2pvl,
                               nadc4_x2xs, "xs"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
