#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_canvastest.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(canvastest, "",
                    "canvastest", "CanvasTest",
                    "Тестовая программа для ELEM_CANVAS", NULL,
                    canvastest_physinfo, countof(canvastest_physinfo), canvastest_grouping,
                    "", "");
