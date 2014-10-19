#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringcamsel.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringcamsel, "ring1:56",
                    "ringcamsel", "RingCamSel",
                    "Селектор камер кольца", NULL,
                    ringcamsel_physinfo, countof(ringcamsel_physinfo), ringcamsel_grouping,
                    "", "notoolbar,nostatusline");
