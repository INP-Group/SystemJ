#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_turnmag.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(turnmag, "ring1:56",
                    "turnmag", "TurnMag",
                    "Поворотный магнит", NULL,
                    turnmag_physinfo, countof(turnmag_physinfo), turnmag_grouping,
                    "", "notoolbar,nostatusline");
