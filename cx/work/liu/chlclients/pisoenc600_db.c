#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_pisoenc600.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(pisoenc600, "",
                    "pisoenc600", "PisoEnc600",
                    "PISO-Encoder600 test panel", NULL,
                    pisoenc600_physinfo, countof(pisoenc600_physinfo), pisoenc600_grouping,
                    "", "");
