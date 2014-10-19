#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v5adc200_grp1.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v5adc200_grp1, "linac1:1",
                    "v5adc200_grp1", "V5ADC200_Grp1",
                    "Измерения ВЧ Группирователя (1/2)", NULL,
                    v5adc200_grp1_physinfo, countof(v5adc200_grp1_physinfo), v5adc200_grp1_grouping,
                    "pzframeclient", "notoolbar,resizable");
