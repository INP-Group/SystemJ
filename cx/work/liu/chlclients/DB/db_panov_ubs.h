#include "drv_i/panov_ubs_drv_i.h"

#include "common_elem_macros.h"


static groupunit_t panov_ubs_grouping[] =
{

#define  UBS_BASE     0
#define  UBS_IDENT    "ubs"
#define  UBS_LABEL    "Panov's UBS (modulator controller)"
#include "elem_ubs.h"
,
    {NULL}
};

static physprops_t panov_ubs_physinfo[] =
{
#define  UBS_BASE     0
#include "pi_ubs.h"
};
