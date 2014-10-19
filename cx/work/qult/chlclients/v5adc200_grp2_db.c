#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v5adc200_grp2.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v5adc200_grp2, "linac1:1",
                    "v5adc200_grp2", "V5ADC200_Grp2",
                    "Измерения ВЧ Группирователя (2/2)", NULL,
                    v5adc200_grp2_physinfo, countof(v5adc200_grp2_physinfo), v5adc200_grp2_grouping,
                    "pzframeclient", "notoolbar,resizable");
