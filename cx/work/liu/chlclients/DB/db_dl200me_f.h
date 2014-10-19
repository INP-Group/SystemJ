#include "drv_i/dl200me_drv_i.h"

#include "common_elem_macros.h"


enum
{
    REG_T16_FAST_AUTO = 80,
    REG_T16_FAST_SECS = 81,
    REG_T16_FAST_PREV = 82,
};

static groupunit_t dl200me_f_grouping[] =
{

#define  DL200ME_BASE     0
#define  DL200ME_IDENT    "dl200me_f"
#define  DL200ME_LABEL    "Fast DL200ME"
#define  DL200ME_REG_AUTO REG_T16_FAST_AUTO
#define  DL200ME_REG_SECS REG_T16_FAST_SECS
#define  DL200ME_REG_PREV REG_T16_FAST_PREV
#include "elem_dl200me_fast.h"
,
    {NULL}
};

static physprops_t dl200me_f_physinfo[] =
{
#define  DL200ME_BASE     0
#include "pi_dl200me_fast.h"
};
