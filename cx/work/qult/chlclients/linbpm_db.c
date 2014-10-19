#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_linbpm.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(linbpm, "linac1:59",
                    "linbpm", "LinBpm",
                    "Linac BPMs", NULL,
                    linbpm_physinfo, countof(linbpm_physinfo), linbpm_grouping,
                    "bpmclient", "");
