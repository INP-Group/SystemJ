#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>

#include "paramstr_parser.h"

#include "cda.h"
#include "Cdr.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Chl.h"
#include "Knobs.h"
#include "KnobsI.h"

//#include "drv_i/u0632_drv_i.h"

#include "pixmaps/btn_quit.xpm"
#include "pixmaps/btn_help.xpm"

#include "pixmaps/btn_pause.xpm"



/*#### _db #########################################################*/

const char *app_name      = "sukhphase";
const char *app_class     = "SukhPhase";
const char *app_title     = "Сухановские фазовые измерения";
const char *app_defserver = "";


enum
{
    LOGD_SUKHANOV = 10000
};


static groupunit_t app_grouping[] =
{
    {NULL}
};

static physprops_t app_phys_info[] =
{
};


//////////////////////////////////////////////////////////////////////

typedef struct
{
} sukhanov_privrec_t;

static XhWidget SUKHANOV_Create_m(knobinfo_t *ki, XhWidget parent)
{
  sukhanov_privrec_t *info;
    
    /* Allocate the private info structure... */
    if ((ki->widget_private = XtMalloc(sizeof(sukhanov_privrec_t))) == NULL)
        return NULL;
    bzero(ki->widget_private, sizeof(sukhanov_privrec_t));
    info = (sukhanov_privrec_t *)(ki->widget_private);
    
    return NULL;
}

static void SUKHANOV_Colorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
}

static knobs_vmt_t KNOB_SUKHANOV_VMT = {SUKHANOV_Create_m,     NULL, NULL, SUKHANOV_Colorize_m, NULL};

static knobs_widgetinfo_t sukhanov_widgetinfo[] =
{
    {LOGD_SUKHANOV,  "sukhanov",  &KNOB_SUKHANOV_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  sukhanov_widgetset = {sukhanov_widgetinfo, NULL};


/*####  ####*/

enum
{
    cmZZZ = 1
};

static actdescr_t toolslist[] =
{
    XhXXX_TOOLCHK(cmChlFreeze, "Сделать паузу, скушать \253Твикс\273",
                                                     btn_pause_xpm),
    XhXXX_TOOLLEDS(),
    XhXXX_TOOLCMD(cmChlHelp,   "Краткая справка",    btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD(cmChlQuit,   "Выйти из программы", btn_quit_xpm),
    XhXXX_END()
};

static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {
        case cmChlHelp:
            ChlShowHelp(win, CHL_HELP_COLORS);
            break;
            
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


int main(int argc, char *argv[])
{
    KnobsRegisterWidgetset(&sukhanov_widgetset);
    return ChlRunApp(argc, argv,
                     app_name, app_class,
                     app_title, NULL,
                     toolslist, CommandProc,
                     "", NULL,
                     app_defserver, app_grouping,
                     app_phys_info, countof(app_phys_info),
                     NULL, NULL);
}
