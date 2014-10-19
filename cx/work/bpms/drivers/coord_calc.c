#include "coord_calc.h"

#include <math.h>
#include <stdio.h>

#include "coord_calc_bep.h"
#include "coord_calc_v2k.h"
#include "coord_calc_v5r.h"

static float amplificationfunc_bep(float coef);
static float amplificationfunc_v2k(float coef);
static float amplificationfunc_v5r(float coef);

static coord_calc_dscr_t bep_cc =
{
    calculate_x_coord_bep,
    calculate_z_coord_bep,
    calculate_intenst_bep,

    amplificationfunc_bep
};

static coord_calc_dscr_t v2k_cc =
{
    calculate_x_coord_v2k,
    calculate_z_coord_v2k,
    calculate_intenst_v2k,

    amplificationfunc_v2k
};

static coord_calc_dscr_t v5r_cc =
{
    calculate_x_coord_v5r,
    calculate_z_coord_v5r,
    calculate_intenst_v5r,

    amplificationfunc_v5r
};

static float amplificationfunc_all(float k, float x, float x0, float b)
{
    return pow(10.0, k * (x - x0) + b );
}

static float amplificationfunc_bep(float coef)
{
    return amplificationfunc_all( -3.05/20.0, coef, 7.0, 0); //  3db
}

static float amplificationfunc_v2k(float coef)
{
    return amplificationfunc_all(-10.00/20.0, coef, 0.0, 0); // 10db
}

static float amplificationfunc_v5r(float coef)
{
    return amplificationfunc_all( -3.05/20.0, coef, 7.0, 0); //  3db
}

int get_s2c(const type_dscr_t type, coord_calc_dscr_t *dscr)
{
  int r = -1;

    if      (type == TYPE_BEP) { *dscr = bep_cc; r = type; }
    else if (type == TYPE_V2K) { *dscr = v2k_cc; r = type; }
    else if (type == TYPE_V5R) { *dscr = v5r_cc; r = type; }
    else                                         r = -1;

    return r;
}

int scale_plugin_parser(const char *str, const char **endptr,
			void *rec, size_t recsize __attribute__((unused)),
			const char *separators  __attribute__((unused)),
			const char *terminators __attribute__((unused)),
			void *privptr, char **errstr)
{
  scale_t     result;
  const char *what = privptr;
  const char *p;
        char *errp;

    result.i = result.a = result.s = NAN;

    if (str == NULL)
    {
	*errstr = "there is no initialisation behaviour for this plugin";
	return PSP_R_USRERR;
    }

    if (what == NULL) what = "???";

    p = str;
    while (*p != '\0'  &&  isspace(*p)) p++;

    /* 0. guard */
    if (p == NULL  ||  p[0] == '\0')
    {
	*errstr = "empty scale description is forbidden";
	return PSP_R_USRERR;
    }

    /* 1. parse I */
    result.i = strtof(p, &errp);
    if (errp == p)
    {
	*errstr = "Invalid 'I' specification";
	return PSP_R_USRERR;
    }
    p = errp;
    if (p[0] != 'm'  ||  p[1] != 'A'  ||  p[2] != '@')
    {
	*errstr = "\"mA@\" is mandatory after 'I' specification";
	return PSP_R_USRERR;
    }
    p += 3;

    /* 2. parse a */
    if (p[0] != 'a'  ||  p[1] != ':')
    {
	*errstr = "\"a:VAL\" expected after 'I' specification";
	return PSP_R_USRERR;
    }
    p += 2;
    result.a = strtof(p, &errp);
    if (errp == p)
    {
	*errstr = "Invalid 'a' specification";
	return PSP_R_USRERR;
    }
    p = errp;

    /* 3. parse s */
    if (p[0] != ','  ||  p[1] != 's'  ||  p[2] != ':')
    {
	*errstr = "\",s:VAL\" expected after 'a' specification";
	return PSP_R_USRERR;
    }
    p += 3;
    result.s = strtof(p, &errp);
    if (errp == p)
    {
	*errstr = "Invalid 's' specification";
	return PSP_R_USRERR;
    }
    p = errp;

    *endptr = p;

    fprintf(stderr, "%s) %s i=%f, a=%f, s=%f\n", __FUNCTION__, what, result.i, result.a, result.s);

    *((scale_t*)rec) = result;

    return PSP_R_OK;
}
#if 0
float get_didsg(const scale_t *scale, float cur_ampl)
{
  float val;

    val = me->scale.i / me->scale.s
	        *
	        me->s2c.a(me->scale.a) / me->s2c.a(me->meas_params[BPMD_PARAM_ATTEN]);

    return val;
}
#endif

