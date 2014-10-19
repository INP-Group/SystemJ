#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v5adc200_kls3.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v5adc200_kls3, "linac1:1",
                    "v5adc200_kls3", "V5ADC200_Kls3",
                    "Измерения ВЧ 3-го клистрона", NULL,
                    v5adc200_kls3_physinfo, countof(v5adc200_kls3_physinfo), v5adc200_kls3_grouping,
                    "pzframeclient", "notoolbar,resizable");
