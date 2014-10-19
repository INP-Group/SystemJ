#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringcor23.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringcor23, "ring1:32",
                    "ringcor23", "RingCor23",
                    "Магнитная коррекция накопителя - RST2/3", NULL,
                    ringcor23_physinfo, countof(ringcor23_physinfo), ringcor23_grouping,
                    "", "");
