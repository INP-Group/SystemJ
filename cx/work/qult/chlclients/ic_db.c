#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ic.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ic, "linac1:5",
                    "ic", "IC",
                    "IC - Fedya's middleware for Injection Complex", NULL,
                    ic_physinfo, countof(ic_physinfo), ic_grouping,
                    "", "");
