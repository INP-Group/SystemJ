#include "nadc200_data.h"
#include "drv_i/nadc200_drv_i.h"


enum
{
    NADC200_DTYPE = ENCODE_DTYPE(NADC200_DATAUNITS,
                                 CXDTYPE_REPR_INT, 1)
};

static void nadc200_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int               nl;

  static int32      quants[4] = {2000, 1000, 500, 250};
  
    for (nl = 0;  nl < NADC200_NUM_LINES;  nl++)
    {
        mes_p->plots[nl].on                 = 1;
        mes_p->plots[nl].numpts             = mes_p->inhrt->info[NADC200_PARAM_NUMPTS];
        mes_p->plots[nl].cur_range.int_r[0] = NADC200_MIN_VALUE;
        mes_p->plots[nl].cur_range.int_r[1] = NADC200_MAX_VALUE;
        FastadcSymmMinMaxInt(mes_p->plots[nl].cur_range.int_r + 0,
                             mes_p->plots[nl].cur_range.int_r + 1);
        mes_p->plots[nl].cur_int_zero       = 255 - mes_p->inhrt->info[NADC200_PARAM_ZERO1 + nl];

        mes_p->plots[nl].x_buf              =
            ((uint8 *)(mes_p->inhrt->databuf)) +
            mes_p->inhrt->info[NADC200_PARAM_NUMPTS] * NADC200_DATAUNITS * nl;
    }

    mes_p->exttim = (mes_p->inhrt->info[NADC200_PARAM_TIMING] == NADC200_T_TIMER);
}

static double nadc200_raw2pvl(int32 *info __attribute__((unused)),
                              int    nl   __attribute__((unused)),
                              int    raw)
{
  static double scales[4] = {0.002, 0.004, 0.008, 0.016};
    
    return
        (raw + info[NADC200_PARAM_ZERO1  + nl] - 256) *
        scales[info[NADC200_PARAM_RANGE1 + nl] & 3];
}

static int    nadc200_x2xs(int32 *info, int x)
{
  static double timing_scales[4] = {1e9/200e6/*5.0*/, 1e9/195e6/*5.128*/, 1, 1};
  static int    frqdiv_scales[4] = {1, 2, 4, 8};

    x += info[NADC200_PARAM_PTSOFS];
    return (int)(x
                 * timing_scales[info[NADC200_PARAM_TIMING] & 3]
                 * frqdiv_scales[info[NADC200_PARAM_FRQDIV] & 3]);
}

static psp_lkp_t nadc200_timing_lkp[] =
{
    {"200",   NADC200_T_200MHZ},
    {"195",   NADC200_T_195MHZ},
    {"timer", NADC200_T_TIMER},
    {NULL, 0}
};

static psp_lkp_t nadc200_frqdiv_lkp[] =
{
    {"1",  NADC200_FRQD_5NS},
    {"5",  NADC200_FRQD_5NS},
    {"2",  NADC200_FRQD_10NS},
    {"10", NADC200_FRQD_10NS},
    {"4",  NADC200_FRQD_20NS},
    {"20", NADC200_FRQD_20NS},
    {"8",  NADC200_FRQD_40NS},
    {"40", NADC200_FRQD_40NS},
    {NULL, 0}
};

static psp_lkp_t nadc200_range_lkp [] =
{
    {"256",  NADC200_R_256},
    {"512",  NADC200_R_512},
    {"1024", NADC200_R_1024},
    {"2048", NADC200_R_2048},
    {NULL, 0}
};

static psp_paramdescr_t text2nadc200_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[NADC200_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[NADC200_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[NADC200_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[NADC200_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[NADC200_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[NADC200_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[NADC200_PARAM_PTSOFS],   -1, 0, NADC200_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[NADC200_PARAM_NUMPTS],   -1, 1, NADC200_MAX_NUMPTS),

    PSP_P_LOOKUP ("timing",   pzframe_params_t, p[NADC200_PARAM_TIMING],   -1, nadc200_timing_lkp),
    PSP_P_LOOKUP ("frqdiv",   pzframe_params_t, p[NADC200_PARAM_FRQDIV],   -1, nadc200_frqdiv_lkp),
    PSP_P_LOOKUP ("range0",   pzframe_params_t, p[NADC200_PARAM_RANGE1],   -1, nadc200_range_lkp),
    PSP_P_LOOKUP ("range1",   pzframe_params_t, p[NADC200_PARAM_RANGE2],   -1, nadc200_range_lkp),

    PSP_P_END(),
};

static pzframe_paramdscr_t nadc200_param_dscrs[] =
{
    {"shot",      NADC200_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    NADC200_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  NADC200_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      NADC200_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    NADC200_PARAM_PTSOFS,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    NADC200_PARAM_NUMPTS,     0,    0, 0, PZFRAME_PARAM_BIGC},
    {"calcstats", NADC200_PARAM_CALC_STATS, 0,    0, 0, PZFRAME_PARAM_INDIFF    | PZFRAME_PARAM_RW_ONLY_MASK},
    {"timing",    NADC200_PARAM_TIMING,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"frqdiv",    NADC200_PARAM_FRQDIV,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range0",    NADC200_PARAM_RANGE1,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range1",    NADC200_PARAM_RANGE2,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"zero0",     NADC200_PARAM_ZERO1,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"zero1",     NADC200_PARAM_ZERO2,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"elapsed",   NADC200_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *nadc200_line_name_list[] = {"0", "1"};
static const char *nadc200_line_unit_list[] = {"V", "V"};

pzframe_type_dscr_t *nadc200_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "nadc200",
                               0,
                               /* Bigc-related ADC characteristics */
                               NADC200_DTYPE,
                               NADC200_NUM_PARAMS,
                               NADC200_MAX_NUMPTS, NADC200_NUM_LINES,
                               NADC200_PARAM_NUMPTS,
                               nadc200_info2mes,
                               /* Description of bigc-parameters */
                               text2nadc200_params,
                               nadc200_param_dscrs,
                               "Ch", nadc200_line_name_list, nadc200_line_unit_list,
                               (plotdata_range_t){.int_r={NADC200_MIN_VALUE,
                                                          NADC200_MAX_VALUE}},
                               "% 6.3f", nadc200_raw2pvl,
                               nadc200_x2xs, "ns"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
