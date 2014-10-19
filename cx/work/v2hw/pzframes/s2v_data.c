#include "s2v_data.h"
#include "drv_i/s2v_drv_i.h"


enum
{
    S2V_DTYPE = ENCODE_DTYPE(S2V_DATAUNITS,
                             CXDTYPE_REPR_INT, 0)
};

static void s2vme_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
    mes_p->plots[0].on                 = 1;
    mes_p->plots[0].numpts             = mes_p->inhrt->info[S2V_PARAM_NUMPTS];
    mes_p->plots[0].cur_range.int_r[0] = S2V_MIN_VALUE;
    mes_p->plots[0].cur_range.int_r[1] = S2V_MAX_VALUE;
    FastadcSymmMinMaxInt(mes_p->plots[0].cur_range.int_r + 0,
                         mes_p->plots[0].cur_range.int_r + 1);
    mes_p->plots[0].cur_int_zero       = 0;
    mes_p->plots[0].x_buf              = mes_p->inhrt->databuf;

    mes_p->exttim = 1;
}

static double s2v_raw2pvl(int32 *info __attribute__((unused)),
                          int    nl   __attribute__((unused)),
                          int    raw)
{
    return raw / 1000000.;
}

static int    s2v_x2xs(int32 *info, int x)
{
    return x + info[S2V_PARAM_PTSOFS];
}

static psp_paramdescr_t text2s2v_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[S2V_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[S2V_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[S2V_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[S2V_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[S2V_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[S2V_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[S2V_PARAM_PTSOFS],   -1, 0, S2V_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[S2V_PARAM_NUMPTS],   -1, 1, S2V_MAX_NUMPTS),
    PSP_P_END(),
};

static pzframe_paramdscr_t s2v_param_dscrs[] =
{
    {"shot",      S2V_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    S2V_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  S2V_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      S2V_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    S2V_PARAM_PTSOFS,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    S2V_PARAM_NUMPTS,     0,    0, 0, PZFRAME_PARAM_BIGC},
    {"calcstats", S2V_PARAM_CALC_STATS, 0,    0, 0, PZFRAME_PARAM_INDIFF    | PZFRAME_PARAM_RW_ONLY_MASK},
    {"elapsed",   S2V_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *s2v_line_name_list[] = {"DATA"};
static const char *s2v_line_unit_list[] = {"V"};

pzframe_type_dscr_t *s2v_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "s2v",
                               0,
                               /* Bigc-related ADC characteristics */
                               S2V_DTYPE,
                               S2V_NUM_PARAMS,
                               S2V_MAX_NUMPTS, S2V_NUM_LINES,
                               S2V_PARAM_NUMPTS,
                               s2vme_info2mes,
                               /* Description of bigc-parameters */
                               text2s2v_params,
                               s2v_param_dscrs,
                               /* Data specifics */
                               "Ch", s2v_line_name_list, s2v_line_unit_list,
                               (plotdata_range_t){.int_r={S2V_MIN_VALUE,
                                                          S2V_MAX_VALUE}},
                               "% 6.3f", s2v_raw2pvl,
                               s2v_x2xs, "x"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
