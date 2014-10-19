#include "cda.h"
/* Include all subsystems */
#include "DB/db_subharmonic.h"
#include "DB/db_linmagx.h"
#include "DB/db_turnmag.h"
#include "DB/db_ringpem.h"
#include "DB/db_linthermcan.h"
#include "DB/db_linvac.h"
#include "DB/db_newvac.h"
#include "DB/db_v5k_cvac.h"
#include "DB/db_camsel.h"
#include "DB/db_wavesel.h"
#include "DB/db_ushd.h"
#include "DB/db_ringcamsel.h"
#include "DB/db_linipp.h"
#include "DB/db_ylinipppanel.h"
#include "DB/db_linbpm.h"
#include "DB/db_linslbpms.h"
#include "DB/db_lin485s.h"
#include "DB/db_ringcor23.h"
#include "DB/db_ringcor45.h"
#include "DB/db_ringmag.h"
#include "DB/db_ringcurmon.h"
#include "DB/db_blm.h"
#include "DB/db_v5adc200_kls1.h"
#include "DB/db_v5adc200_kls2.h"
#include "DB/db_v5adc200_kls3.h"
#include "DB/db_v5adc200_grp1.h"
#include "DB/db_v5adc200_grp2.h"
#include "DB/db_sukhpanel.h"
#include "DB/db_sukhphase.h"
#include "DB/db_z0fed208.h"
#include "DB/db_ic.h"
/* Physinfo database */
static physinfodb_rec_t physinfo_database[] =
{
    {"linac1:30", subharmonic_physinfo, countof(subharmonic_physinfo)},
    {"linac1:31", linmagx_physinfo, countof(linmagx_physinfo)},
    {"ring1:56", turnmag_physinfo, countof(turnmag_physinfo)},
    {"linac1:50", ringpem_physinfo, countof(ringpem_physinfo)},
    {"linac1:34", linthermcan_physinfo, countof(linthermcan_physinfo)},
    {"linac1:33", linvac_physinfo, countof(linvac_physinfo)},
    {"linac1:33", newvac_physinfo, countof(newvac_physinfo)},
    {"v5c-kuzn1:33", v5k_cvac_physinfo, countof(v5k_cvac_physinfo)},
    {"linac1:1", camsel_physinfo, countof(camsel_physinfo)},
    {"linac1:1", wavesel_physinfo, countof(wavesel_physinfo)},
    {"", ushd_physinfo, countof(ushd_physinfo)},
    {"ring1:56", ringcamsel_physinfo, countof(ringcamsel_physinfo)},
    {"linac1:57", linipp_physinfo, countof(linipp_physinfo)},
    {"linac1:57", ylinipppanel_physinfo, countof(ylinipppanel_physinfo)},
    {"linac1:59", linbpm_physinfo, countof(linbpm_physinfo)},
    {"linac1:36", linslbpms_physinfo, countof(linslbpms_physinfo)},
    {"linac1:3", lin485s_physinfo, countof(lin485s_physinfo)},
    {"ring1:32", ringcor23_physinfo, countof(ringcor23_physinfo)},
    {"ring1:32", ringcor45_physinfo, countof(ringcor45_physinfo)},
    {"ring1:32", ringmag_physinfo, countof(ringmag_physinfo)},
    {"ring1:56", ringcurmon_physinfo, countof(ringcurmon_physinfo)},
    {"ring1:4", blm_physinfo, countof(blm_physinfo)},
    {"linac1:1", v5adc200_kls1_physinfo, countof(v5adc200_kls1_physinfo)},
    {"linac1:1", v5adc200_kls2_physinfo, countof(v5adc200_kls2_physinfo)},
    {"linac1:1", v5adc200_kls3_physinfo, countof(v5adc200_kls3_physinfo)},
    {"linac1:1", v5adc200_grp1_physinfo, countof(v5adc200_grp1_physinfo)},
    {"linac1:1", v5adc200_grp2_physinfo, countof(v5adc200_grp2_physinfo)},
    {"linac1:10", sukhpanel_physinfo, countof(sukhpanel_physinfo)},
    {"linac1:10", sukhphase_physinfo, countof(sukhphase_physinfo)},
    {"princess:4", z0fed208_physinfo, countof(z0fed208_physinfo)},
    {"linac1:5", ic_physinfo, countof(ic_physinfo)},
    {NULL, NULL, 0}
};
