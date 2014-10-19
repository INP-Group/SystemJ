#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_widgettest.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(widgettest, "",
                    "widgettest", "WidgetTest",
                    "Всяческие виджеты", NULL,
                    widgettest_physinfo, countof(widgettest_physinfo), widgettest_grouping,
                    "", "");
