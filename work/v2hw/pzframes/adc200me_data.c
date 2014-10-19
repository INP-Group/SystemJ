#include "adc200me_data.h"
#include "drv_i/adc200me_drv_i.h"


enum
{
    ADC200ME_DTYPE = ENCODE_DTYPE(ADC200ME_DATAUNITS,
                                  CXDTYPE_REPR_INT, 0)
};

static void adc200me_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int               nl;

  static int32      quants[4] = {2000, 1000, 500, 250};
  
    for (nl = 0;  nl < ADC200ME_NUM_LINES;  nl++)
    {
        mes_p->plots[nl].on                 = 1;
        mes_p->plots[nl].numpts             = mes_p->inhrt->info[ADC200ME_PARAM_NUMPTS];
        mes_p->plots[nl].cur_range.int_r[0] = (    0-2048) * quants[mes_p->inhrt->info[ADC200ME_PARAM_RANGE1 + nl] & 3];
        mes_p->plots[nl].cur_range.int_r[1] = (0xFFF-2048) * quants[mes_p->inhrt->info[ADC200ME_PARAM_RANGE1 + nl] & 3];
        FastadcSymmMinMaxInt(mes_p->plots[nl].cur_range.int_r + 0,
                             mes_p->plots[nl].cur_range.int_r + 1);
        mes_p->plots[nl].cur_int_zero       = 0;

        mes_p->plots[nl].x_buf              =
            ((uint8 *)(mes_p->inhrt->databuf)) +
            mes_p->inhrt->info[ADC200ME_PARAM_NUMPTS] * ADC200ME_DATAUNITS * nl;
    }

    mes_p->exttim = (mes_p->inhrt->info[ADC200ME_PARAM_TIMING] == ADC200ME_T_EXT);
}

static double adc200me_raw2pvl(int32 *info __attribute__((unused)), 
                               int    nl   __attribute__((unused)),
                               int    raw)
{
    return raw / 1000000.;
}

static int    adc200me_x2xs(int32 *info, int x)
{
    x +=   info[ADC200ME_PARAM_PTSOFS];
    return info[ADC200ME_PARAM_TIMING] == ADC200ME_T_200MHZ? x * 5 // Int.200MHz => 5ns/tick
                                                           : x;    // else show as "1ns"/tick
}


static psp_lkp_t adc200me_timing_lkp[] =
{
    {"200", ADC200ME_T_200MHZ},
    {"int", ADC200ME_T_200MHZ},
    {"ext", ADC200ME_T_EXT},
    {NULL, 0}
};

static psp_lkp_t adc200me_range_lkp [] =
{
    {"4096", ADC200ME_R_4V},
    {"2048", ADC200ME_R_2V},
    {"1024", ADC200ME_R_1V},
    {"512",  ADC200ME_R_05V},
    {NULL, 0}
};

static psp_paramdescr_t text2adc200me_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[ADC200ME_PARAM_SHOT],      -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[ADC200ME_PARAM_ISTART],    -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[ADC200ME_PARAM_ISTART],     1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[ADC200ME_PARAM_ISTART],     0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[ADC200ME_PARAM_WAITTIME],  -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[ADC200ME_PARAM_STOP],      -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[ADC200ME_PARAM_PTSOFS],    -1, 0, ADC200ME_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[ADC200ME_PARAM_NUMPTS],    -1, 1, ADC200ME_MAX_NUMPTS),

    PSP_P_LOOKUP ("timing",   pzframe_params_t, p[ADC200ME_PARAM_TIMING],    -1, adc200me_timing_lkp),
    PSP_P_LOOKUP ("rangeA",   pzframe_params_t, p[ADC200ME_PARAM_RANGE1],    -1, adc200me_range_lkp),
    PSP_P_LOOKUP ("rangeB",   pzframe_params_t, p[ADC200ME_PARAM_RANGE2],    -1, adc200me_range_lkp),
    PSP_P_FLAG   ("-calibr",  pzframe_params_t, p[ADC200ME_PARAM_CALIBRATE], -1, 1),

    PSP_P_END(),
};

static pzframe_paramdscr_t adc200me_param_dscrs[] =
{
    {"shot",      ADC200ME_PARAM_SHOT,        0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    ADC200ME_PARAM_ISTART,      0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  ADC200ME_PARAM_WAITTIME,    1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      ADC200ME_PARAM_STOP,        0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    ADC200ME_PARAM_PTSOFS,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    ADC200ME_PARAM_NUMPTS,      0,    0, 0, PZFRAME_PARAM_BIGC},
    {"timing",    ADC200ME_PARAM_TIMING,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"rangea",    ADC200ME_PARAM_RANGE1,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"rangeb",    ADC200ME_PARAM_RANGE2,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"calibrate", ADC200ME_PARAM_CALIBRATE,   0,    0, 0, PZFRAME_PARAM_BIGC | PZFRAME_PARAM_RW_ONLY_MASK},
    {"fgt_clb",   ADC200ME_PARAM_FGT_CLB,     0,    0, 0, PZFRAME_PARAM_INDIFF | PZFRAME_PARAM_RW_ONLY_MASK},
    {"vis_clb",   ADC200ME_PARAM_VISIBLE_CLB, 0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"clb_ofsa",  ADC200ME_PARAM_CLB_OFS1,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofsb",  ADC200ME_PARAM_CLB_OFS2,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cora",  ADC200ME_PARAM_CLB_COR1,    ADC200ME_CLB_COR_R*0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_corb",  ADC200ME_PARAM_CLB_COR2,    ADC200ME_CLB_COR_R*0,    0, 0, PZFRAME_PARAM_BIGC},
    {"elapsed",   ADC200ME_PARAM_ELAPSED,  1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {"clb_state", ADC200ME_PARAM_CLB_STATE,   0,    0, 0, PZFRAME_PARAM_BIGC},
    {"serial",    ADC200ME_PARAM_SERIAL,      0,    0, 0, PZFRAME_PARAM_INDIFF},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *adc200me_line_name_list[] = {"A", "B"};
static const char *adc200me_line_unit_list[] = {"V", "V"};

pzframe_type_dscr_t *adc200me_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "adc200me",
                               0,
                               /* Bigc-related ADC characteristics */
                               ADC200ME_DTYPE,
                               ADC200ME_NUM_PARAMS,
                               ADC200ME_MAX_NUMPTS, ADC200ME_NUM_LINES,
                               ADC200ME_PARAM_NUMPTS,
                               adc200me_info2mes,
                               /* Description of bigc-parameters */
                               text2adc200me_params,
                               adc200me_param_dscrs,
                               /* Data specifics */
                               "Ch", adc200me_line_name_list, adc200me_line_unit_list,
                               (plotdata_range_t){.int_r={ADC200ME_MIN_VALUE,
                                                          ADC200ME_MAX_VALUE}},
                               "% 6.3f", adc200me_raw2pvl,
                               adc200me_x2xs, "ns"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
