#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v3h_xa40d16.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v3h_xa40d16, "",
                    "v3h_xa40d16", "V3H_XA40D16",
                    "VCh300@XCANADC40+XCANDAC16 test app", NULL,
                    v3h_xa40d16_physinfo, countof(v3h_xa40d16_physinfo), v3h_xa40d16_grouping,
                    "", "");
