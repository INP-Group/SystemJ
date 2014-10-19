#include <math.h>
#include <stdlib.h>

#include "coord_calc_v2k.h"

#include "calibration_1.h"
#include "calibration_2.h"
#include "calibration_3.h"
#include "calibration_4.h"
#include "calibration_5.h"

static float sig_sum(float  s1, float  s2, float  s3, float  s4)
{
    return (s1 + s2 + s3 + s4);
}

static float sig_h  (float  s1, float  s2, float  s3, float  s4)
{
    float  sum = sig_sum(s1, s2, s3, s4);
    float  h   = ( sum != 0 ? (s1 - s2 - s3 + s4) / sum : 0 );
    int signum = (h > 0) - (h < 0);

    return ( abs(h) > 1 ? 1.0 * signum : h );
}

static float sig_v  (float  s1, float  s2, float  s3, float  s4)
{
    float  sum = sig_sum(s1, s2, s3, s4);
    float  v   = ( sum != 0 ? (s1 + s2 - s3 - s4) / sum : 0 );
    int signum = (v > 0) - (v < 0);

    return ( abs(v) > 1 ? 1.0 * signum : v );
}

float calculate_x_coord_v2k(int calibr_id, float  s1, float  s2, float  s3, float  s4)
{
    float  h = sig_h(s1, s2, s3, s4);
    float  v = sig_v(s1, s2, s3, s4);

    switch (calibr_id)
    {
        case 0:  return h;

        /* VEPP-2000 calibrations */
        case 1:  return bpm_1_x(h, v);
        case 2:  return bpm_2_x(h, v);
        case 3:  return bpm_3_x(h, v);
        case 4:  return bpm_4_x(h, v);
        case 5:  return bpm_5_x(h, v);

        /* default is unknown     */
        default: break;
    }
    return 0;
}

float calculate_z_coord_v2k(int calibr_id, float  s1, float  s2, float  s3, float  s4)
{
    float  h = sig_h(s1, s2, s3, s4);
    float  v = sig_v(s1, s2, s3, s4);

    switch (calibr_id)
    {
        case 0:  return v;

        /* VEPP-2000 calibrations */
        case 1:  return bpm_1_z(h, v);
        case 2:  return bpm_2_z(h, v);
        case 3:  return bpm_3_z(h, v);
        case 4:  return bpm_4_z(h, v);
        case 5:  return bpm_5_z(h, v);

        /* default is unknown     */
        default: break;
    }
    return 0;
}

float calculate_intenst_v2k(int calibr_id, float  s1, float  s2, float  s3, float  s4)
{
    float  h   =   sig_h(s1, s2, s3, s4);
    float  v   =   sig_v(s1, s2, s3, s4);
    float  sum = sig_sum(s1, s2, s3, s4);

    switch (calibr_id)
    {
        case 0:  return sqrt(h * h + v * v);

        /* VEPP-2000 calibrations */
        case 1:  return sum / bpm_1_i(h, v);
        case 2:  return sum / bpm_2_i(h, v);
        case 3:  return sum / bpm_3_i(h, v);
        case 4:  return sum / bpm_4_i(h, v);
        case 5:  return sum / bpm_5_i(h, v);

        /* default is unknown     */
        default: break;
    }
    return 0;
}
