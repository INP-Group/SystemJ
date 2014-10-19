#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_z0fed208.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(z0fed208, "princess:4",
                    "z0fed208", "Z0Fed208",
                    "UMs test", NULL,
                    z0fed208_physinfo, countof(z0fed208_physinfo), z0fed208_grouping,
                    "", "notoolbar");
