#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_newvac.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(newvac, "linac1:33",
                    "newvac", "NewVac",
                    "New Vacuum", NULL,
                    newvac_physinfo, countof(newvac_physinfo), newvac_grouping,
                    "", "");
