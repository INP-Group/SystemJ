#include "u0632_data.h"
#include "drv_i/u0632_drv_i.h"


enum
{
    U0632_DTYPE = ENCODE_DTYPE(U0632_DATAUNITS,
                               CXDTYPE_REPR_INT, 1)
};

static psp_lkp_t u0632_m_lkp[] =
{
    {"400",    0},
    {"100",    1},
    { "25",    2},
    {  "6.25", 3},
    {NULL, 0}
};

static psp_lkp_t u0632_p_lkp[] =
{
    {"0",     0},
    {"0.00",  0},
    {"0.11",  1},
    {"0.22",  2},
    {"0.33",  3},
    {"0.44",  4},
    {"0.54",  5},
    {"0.65",  6},
    {"0.76",  7},
    {"0.87",  8},
    {"0.98",  9},
    {"1.09",  10},
    {"1.20",  11},
    {"1.31",  12},
    {"1.41",  13},
    {"1.52",  14},
    {"1.63",  15},
    {"1.74",  16},
    {"1.85",  17},
    {"1.96",  18},
    {"2.07",  19},
    {"2.18",  20},
    {"2.28",  21},
    {"2.39",  22},
    {"2.50",  23},
    {"2.61",  24},
    {"2.72",  25},
    {"2.83",  26},
    {"2.94",  27},
    {"3.05",  28},
    {"3.16",  29},
    {"3.26",  30},
    {"3.37",  31},
    {NULL, 0}
};

static psp_paramdescr_t text2u0632_params[] =
{
    PSP_P_LOOKUP ("m", pzframe_params_t, p[U0632_PARAM_M],   -1, u0632_m_lkp),
    PSP_P_LOOKUP ("p", pzframe_params_t, p[U0632_PARAM_P],   -1, u0632_p_lkp),

    PSP_P_END(),
};

static pzframe_paramdscr_t u0632_param_dscrs[] =
{
    {"m", U0632_PARAM_M, 0, 0, 1, PZFRAME_PARAM_BIGC},
    {"p", U0632_PARAM_P, 0, 0, 1, PZFRAME_PARAM_BIGC},
    {NULL, 0, 0, 0, 0, 0},
};

pzframe_type_dscr_t *u0632_get_type_dscr(void)
{
  static pzframe_type_dscr_t  dscr;
  static int                  dscr_inited;

    if (!dscr_inited)
    {
        PzframeFillDscr(&dscr, "u0632",
                        0,
                        U0632_DTYPE,
                        U0632_NUM_PARAMS,
                        U0632_CHANS_PER_UNIT,
                        text2u0632_params,
                        u0632_param_dscrs);
        dscr_inited = 1;
    }
    return &dscr;
}
