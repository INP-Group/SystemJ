#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_linvac.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(linvac, "linac1:33",
                    "linvac", "LinVac",
                    "Контроль вакуума в линаках", NULL,
                    linvac_physinfo, countof(linvac_physinfo), linvac_grouping,
                    "vacclient", "");
