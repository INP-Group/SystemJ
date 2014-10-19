#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringrf.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringrf, "",
                    "ringrf", "RingRF",
                    "ВЧ-система накопителя", NULL,
                    ringrf_physinfo, countof(ringrf_physinfo), ringrf_grouping,
                    "", "");
