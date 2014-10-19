#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringcor45.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringcor45, "ring1:32",
                    "ringcor45", "RingCor45",
                    "Магнитная коррекция накопителя - RST4/5", NULL,
                    ringcor45_physinfo, countof(ringcor45_physinfo), ringcor45_grouping,
                    "", "");
