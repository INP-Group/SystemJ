#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_wavesel.h"

/*No icon for #include*/

DEFINE_SUBSYS_DESCR(wavesel, "linac1:1",
                    "wavesel", "WaveSel",
                    "������������� ������������ adc200 ��", NULL,
                    wavesel_physinfo, countof(wavesel_physinfo), wavesel_grouping,
                    "", "notoolbar,nostatusline");
