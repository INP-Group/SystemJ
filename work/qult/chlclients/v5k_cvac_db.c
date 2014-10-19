#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v5k_cvac.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v5k_cvac, "v5c-kuzn1:33",
                    "v5k_cvac", "V5KCVac",
                    "V5@Kuzn Vacuum", NULL,
                    v5k_cvac_physinfo, countof(v5k_cvac_physinfo), v5k_cvac_grouping,
                    "", "");
