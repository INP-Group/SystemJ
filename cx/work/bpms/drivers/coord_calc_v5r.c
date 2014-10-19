#include <math.h>
#include <stdlib.h>

#include "coord_calc_v5r.h"

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

float calculate_x_coord_v5r(int calibr_id, float  s1, float  s2, float  s3, float  s4)
{
    float  h = sig_h(s1, s2, s3, s4);
    float  v = sig_v(s1, s2, s3, s4);

    switch (calibr_id)
    {
        case 0:  return h;
        /* VEPP-5    calibrations */
        case 1: return (h * (12.23 - 1.90 * h * h + 15 * h * h * h * h)); // small bpm
        case 2: h = (s1 - s3)/(s1 + s3);
                 return (h * (19.00 + 14.0 * h * h                     )); // big   bpm
        /* default is unknown     */
        default: break;
    }
    return 0;
}

float calculate_z_coord_v5r(int calibr_id, float  s1, float  s2, float  s3, float  s4)
{
    float  h = sig_h(s1, s2, s3, s4);
    float  v = sig_v(s1, s2, s3, s4);

    switch (calibr_id)
    {
        case 0:  return v;
        /* VEPP-5    calibrations */
        case 1: return (v * (21.70 + 47.0 * v * v)); // small bpm
        case 2: v = (s2 - s4)/(s2 + s4);
                 return (v * (19.00 + 14.0 * v * v)); // big   bpm
        /* default is unknown     */
        default: break;
    }
    return 0;
}

float calculate_intenst_v5r(int calibr_id, float  s1, float  s2, float  s3, float  s4)
{
    float  h   = sig_h  (s1, s2, s3, s4);
    float  v   = sig_v  (s1, s2, s3, s4);
    float  sum = sig_sum(s1, s2, s3, s4);

    switch (calibr_id)
    {
        case 0:  return sqrt(h * h + v * v);
        /* VEPP-5    calibrations */
        case 1:
        case 2: return sum;
        /* default is unknown     */
        default: break;
    }
    return 0;
}
