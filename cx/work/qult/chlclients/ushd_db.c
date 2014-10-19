#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ushd.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ushd, "",
                    "ushd", "UShD",
                    "‰≈Õœ ı˚‰", NULL,
                    ushd_physinfo, countof(ushd_physinfo), ushd_grouping,
                    "", "notoolbar,nostatusline");
