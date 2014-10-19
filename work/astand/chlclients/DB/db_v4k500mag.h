#include "common_elem_macros.h"

#include "magx_macros.h"


enum
{
    B_IST_C20_1 = 1088,
    B_IST_IST_1 = 1388,
};

static groupunit_t v4k500mag_grouping[] =
{

    GLOBELEM_START_CRN("ringmag", "Магнитная система",
                       "Задано, A\vИзм., A\vd/dT\v\v", NULL,
                       1*6, 6)
        MAGX_IST_XCDAC20_LINE(istr, "ISTR", "", B_IST_C20_1, B_IST_IST_1, 1000, 1, 0, 1)
    GLOBELEM_END("", 0),
    

    GLOBELEM_START("histplot", "Histplot", 1, 1)
        {"histplot", "HistPlot", NULL, NULL, "width=700,height=300,add=.istr_set,add=.istr_mes", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
    GLOBELEM_END("notitle,nocoltitles,norowtitles", 1),

    {NULL}
};

static physprops_t v4k500mag_physinfo[] =
{
    MAGX_IST_XCDAC20_PHYSLINES(B_IST_C20_1, B_IST_IST_1, 1000000.0/250,  +1)
};
