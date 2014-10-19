#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_sukhphase.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(sukhphase, "linac1:10",
                    "sukhphase", "SukhPhase",
                    "Фазовые измерения им.Суханова", NULL,
                    sukhphase_physinfo, countof(sukhphase_physinfo), sukhphase_grouping,
                    "sukhclient", "resizable");
