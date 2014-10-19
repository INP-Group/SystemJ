#ifndef __COORD_CALC_H
#define __COORD_CALC_H

#include "cxsd_driver.h" // for scale_plugin_parser and cx.h

typedef float (*calculate_x_coord_t)(int calibr_id, float  s1, float  s2, float  s3, float  s4);
typedef float (*calculate_z_coord_t)(int calibr_id, float  s1, float  s2, float  s3, float  s4);
typedef float (*calculate_intenst_t)(int calibr_id, float  s1, float  s2, float  s3, float  s4);

typedef float (*amplificationfunc_t)(float coef);

/* CC type = Coordinat Calculation type */
typedef enum
{
    TYPE_UNK = 0,
    TYPE_BEP = 1,
    TYPE_V2K = 2,
    TYPE_V5R = 3,
} type_dscr_t;

typedef struct
{
    calculate_x_coord_t x;
    calculate_z_coord_t z;
    calculate_intenst_t i;

    amplificationfunc_t a;
} coord_calc_dscr_t;

typedef struct
{
    float i;
    float a;
    float s;
} scale_t;

int get_s2c(const type_dscr_t type, coord_calc_dscr_t *dscr);

int scale_plugin_parser(const char *str, const char **endptr,
			void *rec, size_t recsize,
			const char *separators, const char *terminators,
			void *privptr, char **errstr);

#endif /* __COORD_CALC_H */
