#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_lin485s.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(lin485s, "linac1:3",
                    "lin485s", "Lin485s",
                    "Linac 485s", NULL,
                    lin485s_physinfo, countof(lin485s_physinfo), lin485s_grouping,
                    "", "notoolbar");
