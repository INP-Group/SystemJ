#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_linslbpms.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(linslbpms, "linac1:36",
                    "linslbpms", "LinSlBpms",
                    "Linac BPMs (Styuf-based)", NULL,
                    linslbpms_physinfo, countof(linslbpms_physinfo), linslbpms_grouping,
                    "bpmclient", "");
