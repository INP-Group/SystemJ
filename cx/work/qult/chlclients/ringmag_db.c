#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringmag.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringmag, "ring1:32",
                    "ringmag", "RingMag",
                    "Магнитная система накопителя", NULL,
                    ringmag_physinfo, countof(ringmag_physinfo), ringmag_grouping,
                    "", "");
