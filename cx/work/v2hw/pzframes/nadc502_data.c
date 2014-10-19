#include "nadc502_data.h"
#include "drv_i/nadc502_drv_i.h"


enum
{
    NADC502_DTYPE = ENCODE_DTYPE(NADC502_DATAUNITS,
                                 CXDTYPE_REPR_INT, 0)
};

static void nadc502me_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int               nl;

  static int param2base0[4] = {2048, 1024, 3072, 2048};
  static int param2quant[4] = {39072, 19536, 9768, 4884};
  
    for (nl = 0;  nl < NADC502_NUM_LINES;  nl++)
    {
        mes_p->plots[nl].on                 = 1;
        mes_p->plots[nl].numpts             = mes_p->inhrt->info[NADC502_PARAM_NUMPTS];
        mes_p->plots[nl].cur_range.int_r[0] = 
            param2quant         [mes_p->inhrt->info[NADC502_PARAM_RANGE1 + nl] & 3] *
            (    0 - param2base0[mes_p->inhrt->info[NADC502_PARAM_SHIFT1 + nl] & 3])
            / 10;
        mes_p->plots[nl].cur_range.int_r[1] = 
            param2quant         [mes_p->inhrt->info[NADC502_PARAM_RANGE1 + nl] & 3] *
            (0xFFF - param2base0[mes_p->inhrt->info[NADC502_PARAM_SHIFT1 + nl] & 3])
            / 10;
        FastadcSymmMinMaxInt(mes_p->plots[nl].cur_range.int_r + 0,
                             mes_p->plots[nl].cur_range.int_r + 1);
        mes_p->plots[nl].cur_int_zero       = 0;

        mes_p->plots[nl].x_buf              =
            ((uint8 *)(mes_p->inhrt->databuf)) +
            mes_p->inhrt->info[NADC502_PARAM_NUMPTS] * NADC502_DATAUNITS * nl;
    }

    mes_p->exttim = (mes_p->inhrt->info[NADC502_PARAM_TIMING] == NADC502_TIMING_EXT);
}

static double nadc502_raw2pvl(int32 *info __attribute__((unused)),
                              int    nl   __attribute__((unused)),
                              int    raw)
{
    return raw / 1000000.;
}

static int    nadc502_x2xs(int32 *info, int x)
{
    x +=   info[NADC502_PARAM_PTSOFS];
    return info[NADC502_PARAM_TIMING] == NADC502_TIMING_INT? x * 20 // Int.50MHz => 20ns/tick
                                                           : x;     // else show as "1"/tick
}

static psp_lkp_t nadc502_timing_lkp[] =
{
    {"int",   NADC502_TIMING_INT},
    {"ext",   NADC502_TIMING_EXT},
    {"50",    NADC502_TIMING_INT},
    {"timer", NADC502_TIMING_EXT},
    {NULL, 0}
};

static psp_lkp_t nadc502_tmode_lkp[] =
{
    {"hf", NADC502_TMODE_HF},
    {"lf", NADC502_TMODE_LF},
    {NULL, 0}
};

static psp_lkp_t nadc502_range_lkp [] =
{
    {"8192", NADC502_RANGE_8192},
    {"4096", NADC502_RANGE_4096},
    {"2048", NADC502_RANGE_2048},
    {"1024", NADC502_RANGE_1024},
    {NULL, 0}
};

static psp_paramdescr_t text2nadc502_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[NADC502_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[NADC502_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[NADC502_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[NADC502_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[NADC502_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[NADC502_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[NADC502_PARAM_PTSOFS],   -1, 0, NADC502_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[NADC502_PARAM_NUMPTS],   -1, 1, NADC502_MAX_NUMPTS),

    PSP_P_LOOKUP ("timing",   pzframe_params_t, p[NADC502_PARAM_TIMING],   -1, nadc502_timing_lkp),
    PSP_P_LOOKUP ("tmode",    pzframe_params_t, p[NADC502_PARAM_TMODE],    -1, nadc502_tmode_lkp),
    PSP_P_LOOKUP ("range0",   pzframe_params_t, p[NADC502_PARAM_RANGE1],   -1, nadc502_range_lkp),
    PSP_P_LOOKUP ("range1",   pzframe_params_t, p[NADC502_PARAM_RANGE2],   -1, nadc502_range_lkp),

    PSP_P_END(),
};

static pzframe_paramdscr_t nadc502_param_dscrs[] =
{
    {"shot",      NADC502_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    NADC502_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  NADC502_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      NADC502_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    NADC502_PARAM_PTSOFS,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    NADC502_PARAM_NUMPTS,     0,    0, 0, PZFRAME_PARAM_BIGC},
    {"calcstats", NADC502_PARAM_CALC_STATS, 0,    0, 0, PZFRAME_PARAM_INDIFF    | PZFRAME_PARAM_RW_ONLY_MASK},
    {"timing",    NADC502_PARAM_TIMING,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"tmode",     NADC502_PARAM_TMODE,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range1",    NADC502_PARAM_RANGE1,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range2",    NADC502_PARAM_RANGE2,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"shift1",    NADC502_PARAM_SHIFT1,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"shift2",    NADC502_PARAM_SHIFT2,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"elapsed",   NADC502_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *nadc502_line_name_list[] = {"0", "1"};
static const char *nadc502_line_unit_list[] = {"V", "V"};

pzframe_type_dscr_t *nadc502_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "nadc502",
                               0,
                               /* Bigc-related ADC characteristics */
                               NADC502_DTYPE,
                               NADC502_NUM_PARAMS,
                               NADC502_MAX_NUMPTS, NADC502_NUM_LINES,
                               NADC502_PARAM_NUMPTS,
                               nadc502me_info2mes,
                               /* Description of bigc-parameters */
                               text2nadc502_params,
                               nadc502_param_dscrs,
                               /* Data specifics */
                               "Ch", nadc502_line_name_list, nadc502_line_unit_list,
                               (plotdata_range_t){.int_r={NADC502_MIN_VALUE,
                                                          NADC502_MAX_VALUE}},
                               "% 6.3f", nadc502_raw2pvl,
                               nadc502_x2xs, "ns"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
