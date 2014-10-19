#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_nsukhphase.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(nsukhphase, "",
                    "nsukhphase", "NSukhPhase",
                    "Sukhanov's phase measurements - v.2", NULL,
                    nsukhphase_physinfo, countof(nsukhphase_physinfo), nsukhphase_grouping,
                    "", "");
