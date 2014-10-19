#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_ylinipppanel.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(ylinipppanel, "linac1:57",
                    "ylinipppanel", "yLinIppPanel",
                    "Linac IPP -- panel with numbers", NULL,
                    ylinipppanel_physinfo, countof(ylinipppanel_physinfo), ylinipppanel_grouping,
                    "", "");
