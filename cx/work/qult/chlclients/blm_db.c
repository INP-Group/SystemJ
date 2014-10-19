#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_blm.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(blm, "ring1:4",
                    "blm", "BLM",
                    "Beam Loss Monitor", NULL,
                    blm_physinfo, countof(blm_physinfo), blm_grouping,
                    "pzframeclient", "notoolbar,resizable");
