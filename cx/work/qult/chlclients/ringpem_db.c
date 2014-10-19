#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ringpem.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ringpem, "linac1:50",
                    "ringpem", "RingPem",
                    "ФЭУ - туда-сюда-обратно, тебе и мне приятно", NULL,
                    ringpem_physinfo, countof(ringpem_physinfo), ringpem_grouping,
                    "", "notoolbar,nostatusline");
