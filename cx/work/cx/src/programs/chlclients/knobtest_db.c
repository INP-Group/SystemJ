#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_knobtest.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(knobtest, "",
                    "knobtest", "KnobTest",
                    "Various knobs testing application", NULL,
                    knobtest_physinfo, countof(knobtest_physinfo), knobtest_grouping,
                    "", "");
