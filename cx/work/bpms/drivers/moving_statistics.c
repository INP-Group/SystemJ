#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "moving_statistics.h"

float  min_value   (const float  vect[], size_t count)
{
    // http://en.wikipedia.org/wiki/C_data_types
    // http://ru.wikipedia.org/wiki/Float.h
    // why FLT_MIN is equal 0 ??!!
    
    const  float *val_p = vect;
    size_t it     = 0;
    float  min    = +FLT_MAX;

    for (it = 0; it < count; ++it)
    {
        if (min > *val_p) min = *val_p;
        ++val_p;
    }

    return min;
}

float  max_value   (const float  vect[], size_t count)
{
    // http://en.wikipedia.org/wiki/C_data_types
    // http://ru.wikipedia.org/wiki/Float.h
    // why FLT_MIN is equal 0 ??!!

    const  float *val_p = vect;
    size_t it     = 0;
    float  max    = -FLT_MAX;

    for (it = 0; it < count; ++it)
    {
        if (max < *val_p) max = *val_p;
        ++val_p;
    }

    return max;
}

float  mean_value  (const float  vect[], size_t count)
{
    // http://en.wikipedia.org/wiki/Standard_deviation#Rapid_calculation_methods
    // http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
    // http://en.wikipedia.org/wiki/Moving_average

    const  float *val_p = vect;
    size_t it     = 0;
    float  mean   = 0;
    float  delt   = 0;

    for (it = 0; it < count; ++it)
    {
        delt   = *val_p - mean;
        mean   = mean + delt / (it + 1);

        ++val_p;
    }

    return mean;
}

float  stddev_value(const float  vect[], size_t count)
{
    // http://en.wikipedia.org/wiki/Standard_deviation#Rapid_calculation_methods
    // http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
    // http://en.wikipedia.org/wiki/Moving_average

    const  float *val_p = vect;
    size_t it     = 0;
    float  mean   = 0;
    float  delt   = 0;
    float  stddev = 0;

    for (it = 0; it < count; ++it)
    {
        delt   = *val_p - mean;
        mean   = mean + delt / (it + 1);
        stddev = stddev + delt * (*val_p - mean);

        ++val_p;
    }

    return sqrt(stddev / (it - 1));
}
