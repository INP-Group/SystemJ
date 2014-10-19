#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_linthermcan.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(linthermcan, "linac1:34",
                    "linthermcan", "LinThermCAN",
                    "Система термостабилизации - CAN", NULL,
                    linthermcan_physinfo, countof(linthermcan_physinfo), linthermcan_grouping,
                    "", "");
