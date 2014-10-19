#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_sukhpanel.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(sukhpanel, "linac1:10",
                    "sukhpanel", "SukhPanel",
                    "Фазовые измерения им.Суханова - панель прямого доступа", NULL,
                    sukhpanel_physinfo, countof(sukhpanel_physinfo), sukhpanel_grouping,
                    "pzframeclient", "resizable");
