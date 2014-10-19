#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v1k_xcdac20.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v1k_xcdac20, "",
                    "v1k_xcdac20", "V1K_XCDAC20",
                    "V1000@XCDAC20 test app", NULL,
                    v1k_xcdac20_physinfo, countof(v1k_xcdac20_physinfo), v1k_xcdac20_grouping,
                    "", "");
