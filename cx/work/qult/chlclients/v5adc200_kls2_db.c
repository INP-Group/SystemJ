#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v5adc200_kls2.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v5adc200_kls2, "linac1:1",
                    "v5adc200_kls2", "V5ADC200_Kls2",
                    "Измерения ВЧ 2-го клистрона", NULL,
                    v5adc200_kls2_physinfo, countof(v5adc200_kls2_physinfo), v5adc200_kls2_grouping,
                    "pzframeclient", "notoolbar,resizable");
