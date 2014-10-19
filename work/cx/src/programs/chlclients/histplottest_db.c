#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_histplottest.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(histplottest, "",
                    "histplottest", "HistplotTest",
                    "Histplot knob testing application", NULL,
                    histplottest_physinfo, countof(histplottest_physinfo), histplottest_grouping,
                    "", "");
