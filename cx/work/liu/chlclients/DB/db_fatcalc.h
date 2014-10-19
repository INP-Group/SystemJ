
#include "drv_i/dl200me_drv_i.h"
#include "drv_i/tvac320_drv_i.h"
#include "drv_i/xcac208_drv_i.h"
#include "drv_i/canadc40_drv_i.h"
#include "drv_i/panov_ubs_drv_i.h"

#include "common_elem_macros.h"

#include "devmap_rack0_44.h"
#include "devmap_rackX_43.h"


#define SET_LINE(id, l, n) \
    {id, l, "?s", "%8.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FATCALC_BASE+n)}

static groupunit_t fatcalc_grouping[] =
{
    GLOBELEM_START("id", "Settings", 10, 1)
        SET_LINE("chan1", "Time 1", 0),
        SET_LINE("chan2", "Time 2", 1),
        SET_LINE("chan3", "Time 3", 2),
        SET_LINE("chan4", "Time 4", 3),
        SET_LINE("chan5", "Time 5", 4),
        SET_LINE("chan6", "Time 6", 5),
        SET_LINE("chan7", "Time 7", 6),
        SET_LINE("chan8", "Time 8", 7),
        SET_LINE("chan9", "Time 9", 8),
        {"-write", "\nWrite", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
         (excmd_t[]){CMD_RET_I(0)},
         (excmd_t[]){
             CMD_RET
         }
        },
    GLOBELEM_END("nocoltitles", 0),

    {NULL}
};


static physprops_t rack0_44_physinfo[] =
{
#include "pi_rack0_44.h"
};

static physprops_t rack1_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack2_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack3_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack4_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack5_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack6_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack7_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physprops_t rack8_43_physinfo[] =
{
#include "pi_rackX_43.h"
};

static physinfodb_rec_t fatcalc_physinfo_database[] =
{
    {"rack0:44", rack0_44_physinfo, countof(rack0_44_physinfo)},

    {"rack1:43", rack1_43_physinfo, countof(rack1_43_physinfo)},
    {"rack2:43", rack2_43_physinfo, countof(rack2_43_physinfo)},
    {"rack3:43", rack3_43_physinfo, countof(rack3_43_physinfo)},
    {"rack4:43", rack4_43_physinfo, countof(rack4_43_physinfo)},
    {"rack5:43", rack5_43_physinfo, countof(rack5_43_physinfo)},
    {"rack6:43", rack6_43_physinfo, countof(rack6_43_physinfo)},
    {"rack7:43", rack7_43_physinfo, countof(rack7_43_physinfo)},
    {"rack8:43", rack8_43_physinfo, countof(rack8_43_physinfo)},

    {NULL, NULL, 0}
};
