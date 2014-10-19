#include "c061621_data.h"
#include "drv_i/c061621_drv_i.h"


enum
{
    C061621_DTYPE = ENCODE_DTYPE(C061621_DATAUNITS,
                                 CXDTYPE_REPR_INT, 0)
};

static void c061621me_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int              base0;
  int              quant;

  static int param2quant[8] = { 390,   780, 1560,   3120,
                               6250, 12500, 25000, 50000};

    quant = param2quant[mes_p->inhrt->info[C061621_PARAM_RANGE] & 7];
    base0 = 4095 /* ==2047.5*2 */ * quant / 2;

    mes_p->plots[0].on                 = 1;
    mes_p->plots[0].numpts             = mes_p->inhrt->info[C061621_PARAM_NUMPTS];
    mes_p->plots[0].cur_range.int_r[0] = C061621_MIN_VALUE;
    mes_p->plots[0].cur_range.int_r[1] = C061621_MAX_VALUE;
    mes_p->plots[0].cur_range.int_r[0] = quant *    0 - base0;
    mes_p->plots[0].cur_range.int_r[1] = quant * 4095 - base0;
    FastadcSymmMinMaxInt(mes_p->plots[0].cur_range.int_r + 0,
                         mes_p->plots[0].cur_range.int_r + 1);
    mes_p->plots[0].cur_int_zero       = 0;
    mes_p->plots[0].x_buf              = mes_p->inhrt->databuf;

    mes_p->exttim = (mes_p->inhrt->info[C061621_PARAM_TIMING] == C061621_T_CPU  ||
                     mes_p->inhrt->info[C061621_PARAM_TIMING] == C061621_T_EXT);
}

static double c061621_raw2pvl(int32 *info __attribute__((unused)),
                              int    nl   __attribute__((unused)),
                              int    raw)
{
    return raw / 1000000.;
}

static int    c061621_x2xs(int32 *info, int x)
{
  int        qv;

  static int q2xs[] =
  {
      50, 100, 200, 400, 500,
      1000, 2000, 4000, 5000, 10000, 20000, 40000, 50000,
      100000, 200000, 400000, 500000, 1000000, 2000000,
      1, 1
  };

    qv = 1;
    if (info[C061621_PARAM_TIMING] >= 0  &&
        info[C061621_PARAM_TIMING] < countof(q2xs))
        qv = q2xs[info[C061621_PARAM_TIMING]];

    return (x + info[C061621_PARAM_PTSOFS]) * qv;
}

static psp_lkp_t c061621_timing_lkp[] =
{
    {"50ns",   C061621_T_50NS},
    {"100ns",  C061621_T_100NS},
    {"200ns",  C061621_T_200NS},
    {"400ns",  C061621_T_400NS},
    {"500ns",  C061621_T_500NS},
    {"1us",    C061621_T_1US},
    {"2us",    C061621_T_2US},
    {"4us",    C061621_T_4US},
    {"5us",    C061621_T_5US},
    {"10us",   C061621_T_10US},
    {"20us",   C061621_T_20US},
    {"40us",   C061621_T_40US},
    {"50us",   C061621_T_50US},
    {"100us",  C061621_T_100US},
    {"200us",  C061621_T_200US},
    {"400us",  C061621_T_400US},
    {"500us",  C061621_T_500US},
    {"1ms",    C061621_T_1MS},
    {"2ms",    C061621_T_2MS},
    {"cpu",    C061621_T_CPU},
    {"ext",    C061621_T_EXT},
    {NULL, 0}
};

static psp_lkp_t c061621_range_lkp [] =
{
    {"80mv",   C061621_R_80MV},
    {"160mv",  C061621_R_160MV},
    {"320mv",  C061621_R_320MV},
    {"639mv",  C061621_R_639MV},
    {"1.279v", C061621_R_1_279V},
    {"2.558v", C061621_R_2_558V},
    {"5.115v", C061621_R_5_115V},
    {"10.02v", C061621_R_10_02V},
    {NULL, 0}
};

static psp_paramdescr_t text2c061621_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[C061621_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[C061621_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[C061621_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[C061621_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[C061621_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[C061621_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[C061621_PARAM_PTSOFS],   -1, 0, C061621_MAX_NUMPTS),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[C061621_PARAM_NUMPTS],   -1, 0, C061621_MAX_NUMPTS-1),

    PSP_P_LOOKUP ("timing",   pzframe_params_t, p[C061621_PARAM_TIMING],   -1, c061621_timing_lkp),
    PSP_P_LOOKUP ("range",    pzframe_params_t, p[C061621_PARAM_RANGE],    -1, c061621_range_lkp),
    PSP_P_END(),
};

static pzframe_paramdscr_t c061621_param_dscrs[] =
{
    {"shot",      C061621_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    C061621_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  C061621_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      C061621_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    C061621_PARAM_PTSOFS,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    C061621_PARAM_NUMPTS,     0,    0, 0, PZFRAME_PARAM_BIGC},
    {"calcstats", C061621_PARAM_CALC_STATS, 0,    0, 0, PZFRAME_PARAM_INDIFF    | PZFRAME_PARAM_RW_ONLY_MASK},
    {"timing",    C061621_PARAM_TIMING,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range1",    C061621_PARAM_RANGE,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"elapsed",   C061621_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *c061621_line_name_list[] = {"DATA"};
static const char *c061621_line_unit_list[] = {"V"};

pzframe_type_dscr_t *c061621_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "c061621",
                               0,
                               /* Bigc-related ADC characteristics */
                               C061621_DTYPE,
                               C061621_NUM_PARAMS,
                               C061621_MAX_NUMPTS, C061621_NUM_LINES,
                               C061621_PARAM_NUMPTS,
                               c061621me_info2mes,
                               /* Description of bigc-parameters */
                               text2c061621_params,
                               c061621_param_dscrs,
                               /* Data specifics */
                               "Ch", c061621_line_name_list, c061621_line_unit_list,
                               (plotdata_range_t){.int_r={C061621_MIN_VALUE,
                                                          C061621_MAX_VALUE}},
                               "% 6.3f", c061621_raw2pvl,
                               c061621_x2xs, "ns"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
