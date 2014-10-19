#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_linipp.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(linipp, "linac1:57",
                    "linipp", "LinIpp",
                    "Linac IPP", NULL,
                    (void *)linipp_physinfo_database, -1, linipp_grouping,
                    "ippclient", "freezable");
