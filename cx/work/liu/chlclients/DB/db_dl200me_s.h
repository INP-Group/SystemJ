#include "drv_i/dl200me_drv_i.h"

#include "common_elem_macros.h"


enum
{
    REG_T16_SLOW_AUTO = 90,
    REG_T16_SLOW_SECS = 91,
    REG_T16_SLOW_PREV = 92,
};

static groupunit_t dl200me_s_grouping[] =
{

#define  DL200ME_BASE     0
#define  DL200ME_IDENT    "dl200me_s"
#define  DL200ME_LABEL    "Slow DL200ME"
#define  DL200ME_REG_AUTO REG_T16_SLOW_AUTO
#define  DL200ME_REG_SECS REG_T16_SLOW_SECS
#define  DL200ME_REG_PREV REG_T16_SLOW_PREV
#include "elem_dl200me_slow.h"
,
    {NULL}
};

static physprops_t dl200me_s_physinfo[] =
{
#define  DL200ME_BASE     0
#include "pi_dl200me_slow.h"
};
