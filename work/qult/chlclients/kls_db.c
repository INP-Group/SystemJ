#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_kls.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(kls, "",
                    "kls", "KLS",
                    "Klystron Control Demo", NULL,
                    kls_physinfo, countof(kls_physinfo), kls_grouping,
                    "", "");
