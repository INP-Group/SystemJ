#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_subharmonic.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(subharmonic, "linac1:30",
                    "subharmonic", "SubHarmonic",
                    "Управление субгармоникой", NULL,
                    subharmonic_physinfo, countof(subharmonic_physinfo), subharmonic_grouping,
                    "", "compact");
