#include "adc812me_data.h"
#include "drv_i/adc812me_drv_i.h"


enum
{
    ADC812ME_DTYPE = ENCODE_DTYPE(ADC812ME_DATAUNITS,
                                  CXDTYPE_REPR_INT, 0)
};

static void adc812me_info2mes(int32 *Zinfo, fastadc_mes_t *mes_p)
{
  int               nl;

  static int32      quants[4] = {4000, 2000, 1000, 500};

    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        mes_p->plots[nl].on                 = 1;
        mes_p->plots[nl].numpts             = mes_p->inhrt->info[ADC812ME_PARAM_NUMPTS];
        mes_p->plots[nl].cur_range.int_r[0] = (    0-2048) * quants[mes_p->inhrt->info[ADC812ME_PARAM_RANGE0 + nl] & 3];
        mes_p->plots[nl].cur_range.int_r[1] = (0xFFF-2048) * quants[mes_p->inhrt->info[ADC812ME_PARAM_RANGE0 + nl] & 3];
        FastadcSymmMinMaxInt(mes_p->plots[nl].cur_range.int_r + 0,
                             mes_p->plots[nl].cur_range.int_r + 1);
        mes_p->plots[nl].cur_int_zero       = 0;
        mes_p->plots[nl].x_buf              =
            ((uint8 *)(mes_p->inhrt->databuf)) +
            mes_p->inhrt->info[ADC812ME_PARAM_NUMPTS] * ADC812ME_DATAUNITS * nl;
    }

    mes_p->exttim = (mes_p->inhrt->info[ADC812ME_PARAM_TIMING] == ADC812ME_T_EXT);
}

static double adc812me_raw2pvl(int32 *info __attribute__((unused)), 
                               int    nl   __attribute__((unused)),
                               int    raw)
{
    return raw / 1000000.;
}

static int    adc812me_x2xs(int32 *info, int x)
{
  static double timing_scales[2] = {250, 1}; // That's in fact the "exttim" factor
  static int    frqdiv_scales[4] = {1, 2, 4, 8};

    x += info[ADC812ME_PARAM_PTSOFS];
    return (int)(x
                 * timing_scales[info[ADC812ME_PARAM_TIMING] & 1]
                 * frqdiv_scales[info[ADC812ME_PARAM_FRQDIV] & 3]);
}

static psp_lkp_t adc812me_timing_lkp[] =
{
    {"4",   ADC812ME_T_4MHZ},
    {"int", ADC812ME_T_4MHZ},
    {"ext", ADC812ME_T_EXT},
    {NULL, 0}
};

static psp_lkp_t adc812me_frqdiv_lkp[] =
{
    {"1",    ADC812ME_FRQD_250NS},
    {"250",  ADC812ME_FRQD_250NS},
    {"2",    ADC812ME_FRQD_500NS},
    {"500",  ADC812ME_FRQD_500NS},
    {"4",    ADC812ME_FRQD_1000NS},
    {"1000", ADC812ME_FRQD_1000NS},
    {"8",    ADC812ME_FRQD_2000NS},
    {"2000", ADC812ME_FRQD_2000NS},
    {NULL, 0}
};

static psp_lkp_t adc812me_range_lkp [] =
{
    {"8192", ADC812ME_R_8V},
    {"4096", ADC812ME_R_4V},
    {"2048", ADC812ME_R_2V},
    {"1024", ADC812ME_R_1V},
    {NULL, 0}
};

static psp_paramdescr_t text2adc812me_params[] =
{
    PSP_P_FLAG   ("-shot",    pzframe_params_t, p[ADC812ME_PARAM_SHOT],     -1, 1),
    PSP_P_FLAG   ("-istart",  pzframe_params_t, p[ADC812ME_PARAM_ISTART],   -1, 1),
    PSP_P_FLAG   ("istart",   pzframe_params_t, p[ADC812ME_PARAM_ISTART],    1, 0),
    PSP_P_FLAG   ("noistart", pzframe_params_t, p[ADC812ME_PARAM_ISTART],    0, 0),
    PSP_P_INT    ("waittime", pzframe_params_t, p[ADC812ME_PARAM_WAITTIME], -1, 0, 2*1000*1000),
    PSP_P_FLAG   ("-stop",    pzframe_params_t, p[ADC812ME_PARAM_STOP],     -1, 1),
    PSP_P_INT    ("ptsofs",   pzframe_params_t, p[ADC812ME_PARAM_PTSOFS],   -1, 0, ADC812ME_MAX_NUMPTS-1),
    PSP_P_INT    ("numpts",   pzframe_params_t, p[ADC812ME_PARAM_NUMPTS],   -1, 1, ADC812ME_MAX_NUMPTS),

    PSP_P_LOOKUP ("timing",   pzframe_params_t, p[ADC812ME_PARAM_TIMING],   -1, adc812me_timing_lkp),
    PSP_P_LOOKUP ("frqdiv",   pzframe_params_t, p[ADC812ME_PARAM_FRQDIV],   -1, adc812me_frqdiv_lkp),
    PSP_P_LOOKUP ("range0A",  pzframe_params_t, p[ADC812ME_PARAM_RANGE0],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range0B",  pzframe_params_t, p[ADC812ME_PARAM_RANGE1],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range1A",  pzframe_params_t, p[ADC812ME_PARAM_RANGE2],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range1B",  pzframe_params_t, p[ADC812ME_PARAM_RANGE3],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range2A",  pzframe_params_t, p[ADC812ME_PARAM_RANGE4],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range2B",  pzframe_params_t, p[ADC812ME_PARAM_RANGE5],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range3A",  pzframe_params_t, p[ADC812ME_PARAM_RANGE6],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range3B",  pzframe_params_t, p[ADC812ME_PARAM_RANGE7],   -1, adc812me_range_lkp),

    PSP_P_END(),
};

static pzframe_paramdscr_t adc812me_param_dscrs[] =
{
    {"shot",      ADC812ME_PARAM_SHOT,        0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"istart",    ADC812ME_PARAM_ISTART,      0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"waittime",  ADC812ME_PARAM_WAITTIME,    1,    0, 0, PZFRAME_PARAM_INDIFF},
    {"stop",      ADC812ME_PARAM_STOP,        0,    0, 0, PZFRAME_PARAM_CHAN_ONLY | PZFRAME_PARAM_RW_ONLY_MASK},
    {"ptsofs",    ADC812ME_PARAM_PTSOFS,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"numpts",    ADC812ME_PARAM_NUMPTS,      0,    0, 0, PZFRAME_PARAM_BIGC},
    {"timing",    ADC812ME_PARAM_TIMING,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"frqdiv",    ADC812ME_PARAM_FRQDIV,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range0a",   ADC812ME_PARAM_RANGE0,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range0b",   ADC812ME_PARAM_RANGE1,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range1a",   ADC812ME_PARAM_RANGE2,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range1b",   ADC812ME_PARAM_RANGE3,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range2a",   ADC812ME_PARAM_RANGE4,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range2b",   ADC812ME_PARAM_RANGE5,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range3a",   ADC812ME_PARAM_RANGE6,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"range3b",   ADC812ME_PARAM_RANGE7,      0,    0, 1, PZFRAME_PARAM_BIGC},
    {"calibrate", ADC812ME_PARAM_CALIBRATE,   0,    0, 0, PZFRAME_PARAM_BIGC | PZFRAME_PARAM_RW_ONLY_MASK},
    {"fgt_clb",   ADC812ME_PARAM_FGT_CLB,     0,    0, 0, PZFRAME_PARAM_INDIFF | PZFRAME_PARAM_RW_ONLY_MASK},
    {"vis_clb",   ADC812ME_PARAM_VISIBLE_CLB, 0,    0, 0, PZFRAME_PARAM_INDIFF},
    {"clb_ofs0a", ADC812ME_PARAM_CLB_OFS0,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs0b", ADC812ME_PARAM_CLB_OFS1,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs1a", ADC812ME_PARAM_CLB_OFS2,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs1b", ADC812ME_PARAM_CLB_OFS3,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs2a", ADC812ME_PARAM_CLB_OFS4,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs2b", ADC812ME_PARAM_CLB_OFS5,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs3a", ADC812ME_PARAM_CLB_OFS6,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_ofs3b", ADC812ME_PARAM_CLB_OFS7,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor0a", ADC812ME_PARAM_CLB_COR0,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor0b", ADC812ME_PARAM_CLB_COR1,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor1a", ADC812ME_PARAM_CLB_COR2,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor1b", ADC812ME_PARAM_CLB_COR3,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor2a", ADC812ME_PARAM_CLB_COR4,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor2b", ADC812ME_PARAM_CLB_COR5,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor3a", ADC812ME_PARAM_CLB_COR6,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"clb_cor3b", ADC812ME_PARAM_CLB_COR7,    0,    0, 0, PZFRAME_PARAM_BIGC},
    {"elapsed",   ADC812ME_PARAM_ELAPSED,  1000,    0, 0, PZFRAME_PARAM_CHAN_ONLY},
    {"clb_state", ADC812ME_PARAM_CLB_STATE,   0,    0, 0, PZFRAME_PARAM_BIGC},
    {"serial",    ADC812ME_PARAM_SERIAL,      0,    0, 0, PZFRAME_PARAM_INDIFF},
    {NULL, 0, 0, 0, 0, 0},
};

static const char *adc812me_line_name_list[] =
{
    "0A", "0B", "1A", "1B", "2A", "2B", "3A", "3B"
};

static const char *adc812me_line_unit_list[] =
{
    "V", "V", "V", "V", "V", "V", "V", "V"
};

pzframe_type_dscr_t *adc812me_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        FastadcDataFillStdDscr(&dscr, "adc812me",
                               0,
                               /* Bigc-related ADC characteristics */
                               ADC812ME_DTYPE,
                               ADC812ME_NUM_PARAMS,
                               ADC812ME_MAX_NUMPTS, ADC812ME_NUM_LINES,
                               ADC812ME_PARAM_NUMPTS,
                               adc812me_info2mes,
                               /* Description of bigc-parameters */
                               text2adc812me_params,
                               adc812me_param_dscrs,
                               /* Data specifics */
                               "Channel", adc812me_line_name_list, adc812me_line_unit_list,
                               (plotdata_range_t){.int_r={ADC812ME_MIN_VALUE,
                                                          ADC812ME_MAX_VALUE}},
                               "% 6.3f", adc812me_raw2pvl,
                               adc812me_x2xs, "ns"
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
