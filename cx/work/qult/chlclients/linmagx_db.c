#include "misc_macros.h" /* For countof()...*/
#include "cx.h"
#include "cxdata.h"

#include "DB/db_linmagx.h"

#include "pixmaps/magnet.xpm"

DEFINE_SUBSYS_DESCR(linmagx, "linac1:31",
                    "linmagx", "LinMag",
                    "Управление Магнитной Системой Линака", magnet_xpm,
                    linmagx_physinfo, countof(linmagx_physinfo), linmagx_grouping,
                    "", "loaddefsettings");
