#include "nadc333_data.h"
#include "drv_i/nadc333_drv_i.h"


enum
{
    NADC333_DTYPE = ENCODE_DTYPE(NADC333_DATAUNITS,
                                 CXDTYPE_REPR_INT, 0)
};

static void nadc333_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int               nl;
  int8             *bufp;

  static int32      quants[4] = {2000, 1000, 500, 250};
  
    for (nl = 0, bufp = mes_p->inhrt->databuf;  nl < NADC333_NUM_LINES;  nl++)
    {
        mes_p->plots[nl].on                 = (mes_p->inhrt->info[NADC333_PARAM_RANGE0 + nl] != NADC333_R_OFF);
        mes_p->plots[nl].numpts             = mes_p->inhrt->info[NADC333_PARAM_NUMPTS];
        mes_p->plots[nl].cur_range.int_r[0] = (    0-2048) * quants[mes_p->inhrt->info[NADC333_PARAM_RANGE0 + nl] & 3];
        mes_p->plots[nl].cur_range.int_r[1] = (0xFFF-2048) * quants[mes_p->inhrt->info[NADC333_PARAM_RANGE0 + nl] & 3];
        FastadcSymmMinMaxInt(mes_p->plots[nl].cur_range.int_r + 0,
                             mes_p->plots[nl].cur_range.int_r + 1);
        mes_p->plots[nl].cur_int_zero       = 0;

        if (mes_p->plots[nl].on)
        {
            mes_p->plots[nl].x_buf          = bufp;
            bufp += mes_p->inhrt->info[NADC333_PARAM_NUMPTS] * NADC333_DATAUNITS;
        }
        else
        {
            mes_p->plots[nl].x_buf          = NULL;
            mes_p->plots[nl].numpts         = 0;
        }
    }

    mes_p->exttim = (mes_p->inhrt->info[NADC333_PARAM_TIMING] == NADC333_T_EXT);
}

static double nadc333_raw2pvl(int32 *info __attribute__((unused)),
                              int    nl   __attribute__((unused)),
                              int    raw)
{
    return raw / 1000000.;
}

static int    nadc333_x2xs(int32 *info, int x)
{
  int        nl;
  int        num_on;

  static int timing_scales[8] = {500, 1000, 2000, 4000, 8000, 16000, 32000, 1000/*ext*/};

    for (nl = 0, num_on = 0;  nl < NADC333_NUM_LINES;  nl++)
        num_on += (info[NADC333_PARAM_RANGE0 + nl] != NADC333_R_OFF);

    return
        (x + info[NADC333_PARAM_PTSOFS])
        *
        num_on
        *
        timing_scales[info[NADC333_PARAM_TIMING] & 7]
        /
        1000;
}

static psp_lkp_t nadc333_timing_lkp[] =
{
    {"500", NADC333_T_500NS},
    {"1",   NADC333_T_1US},
    {"2",   NADC333_T_2US},
    {"4",   NADC333_T_4US},
    {"8",   NADC333_T_8US},
    {"16",  NADC333_T_16US},
    {"32",  NADC333_T_32US},
    {"ext", NADC333_T_EXT},
    {NULL, 0}
};

static psp_lkp_t nadc333_range_lkp [] =
{
    {"8192", NADC333_R_8192},
    {"4096", NADC333_R_4096},
    {"2048", NADC333_R_2048},
    {"1024", NADC333_R_1024},
    {"OFF",  NADC333_R_OFF},
    {NULL, 0}
};

static psp_paramdescr_t text2nadc333_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[NADC333_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[NADC333_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[NADC333_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[NADC333_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[NADC333_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[NADC333_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[NADC333_PARAM_PTSOFS],   -1, 0, NADC333_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[NADC333_PARAM_NUMPTS],   -1, 1, NADC333_MAX_NUMPTS),

    PSP_P_LOOKUP ("timing",   pzframe_params_t, p[NADC333_PARAM_TIMING],   -1, nadc333_timing_lkp),
    PSP_P_LOOKUP ("rangeA",   pzframe_params_t, p[NADC333_PARAM_RANGE0],   -1, nadc333_range_lkp),
    PSP_P_LOOKUP ("rangeB",   pzframe_params_t, p[NADC333_PARAM_RANGE1],   -1, nadc333_range_lkp),
    PSP_P_LOOKUP ("rangeC",   pzframe_params_t, p[NADC333_PARAM_RANGE2],   -1, nadc333_range_lkp),
    PSP_P_LOOKUP ("rangeD",   pzframe_params_t, p[NADC333_PARAM_RANGE3],   -1, nadc333_range_lkp),

    PSP_P_END(),
};

static pzframe_paramdscr_t nadc333_param_dscrs[] =
{
    {"shot",      NADC333_PARAM_SHOT,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    NADC333_PARAM_ISTART,     0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  NADC333_PARAM_WAITTIME,   1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      NADC333_PARAM_STOP,       0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    NADC333_PARAM_PTSOFS,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    NADC333_PARAM_NUMPTS,     0,    0, 0, PZFRAME_PARAM_BIGC},
    {"timing",    NADC333_PARAM_TIMING,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"rangeA",    NADC333_PARAM_RANGE0,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"rangeB",    NADC333_PARAM_RANGE1,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"rangeC",    NADC333_PARAM_RANGE2,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"rangeD",    NADC333_PARAM_RANGE3,     0,    0, 1, PZFRAME_PARAM_BIGC},
    {"elapsed",   NADC333_PARAM_ELAPSED, 1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {NULL, 0, 0, 0, 0, 0},
};


static const char *nadc333_line_name_list[] = {"A", "B", "C", "D"};
static const char *nadc333_line_unit_list[] = {"V", "V", "V", "V"};

pzframe_type_dscr_t *nadc333_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "nadc333",
                               0,
                               /* Bigc-related ADC characteristics */
                               NADC333_DTYPE,
                               NADC333_NUM_PARAMS,
                               NADC333_MAX_NUMPTS, NADC333_NUM_LINES,
                               NADC333_PARAM_NUMPTS,
                               nadc333_info2mes,
                               /* Description of bigc-parameters */
                               text2nadc333_params,
                               nadc333_param_dscrs,
                               /* Data specifics */
                               "Channel", nadc333_line_name_list, nadc333_line_unit_list,
                               (plotdata_range_t){.int_r={NADC333_MIN_VALUE,
                                                          NADC333_MAX_VALUE}},
                               "% 6.3f", nadc333_raw2pvl,
                               nadc333_x2xs, "us"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
