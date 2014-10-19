#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ist_xcdac20.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ist_xcdac20, "",
                    "ist_xcdac20", "Ist_XCDAC20",
                    "IST@XCDAC20 test app", NULL,
                    ist_xcdac20_physinfo, countof(ist_xcdac20_physinfo), ist_xcdac20_grouping,
                    "", "");
