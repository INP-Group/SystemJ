#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_v5adc200_kls1.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(v5adc200_kls1, "linac1:1",
                    "v5adc200_kls1", "V5ADC200_Kls1",
                    "Измерения ВЧ 1-го клистрона", NULL,
                    v5adc200_kls1_physinfo, countof(v5adc200_kls1_physinfo), v5adc200_kls1_grouping,
                    "pzframeclient", "notoolbar,resizable");
