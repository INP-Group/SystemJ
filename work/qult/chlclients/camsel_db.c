#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_camsel.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(camsel, "linac1:1",
                    "camsel", "CamSel",
                    "Переключатель видеокамер", NULL,
                    camsel_physinfo, countof(camsel_physinfo), camsel_grouping,
                    "", "notoolbar,nostatusline");
