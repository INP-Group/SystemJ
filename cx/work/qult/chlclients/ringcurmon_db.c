#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringcurmon.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringcurmon, "ring1:56",
                    "ringcurmon", "RingCurMon",
                    "Датчик тока кольца", NULL,
                    ringcurmon_physinfo, countof(ringcurmon_physinfo), ringcurmon_grouping,
                    "", "resizable,notoolbar,nostatusline,compact,histring_size:864000");
