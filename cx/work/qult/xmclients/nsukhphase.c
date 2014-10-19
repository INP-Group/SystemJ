#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>

#include "misc_macros.h"
#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Xh_plot.h"
#include "Chl.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_help.xpm"
#include "pixmaps/btn_quit.xpm"


/*#### _db #########################################################*/

const char *app_name      = "sukhphase";
const char *app_class     = "SukhPhase";
const char *app_title     = "Сухановские фазовые измерения";
const char *app_defserver = "";


enum
{
    LOGD_SUKHANOV_PLOT = 10000
};


static groupunit_t app_grouping[] =
{
    {
        &(elemnet_t){
            "main", "Application controls",
            NULL,
            NULL,
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"plots", "Place for plots...", NULL, NULL, "color=white width=600,height=400 border=1 textpos=0c0m fontsize=huge fontmod=bolditalic", NULL, LOGT_READ, LOGD_SUKHANOV_PLOT, LOGK_NOP, .placement="horz=right,vert=bottom"},
            },
            "noshadow,notitle,nocoltitles,norowtitles,fold_h"
        }, GU_FLAG_FROMNEWLINE | GU_FLAG_FILLHORZ | GU_FLAG_FILLVERT
    },

    {NULL}
};

static physprops_t app_phys_info[] =
{
};

//////////////////////////////////////////////////////////////////////

static XhWidget SUKHANOV_PLOT_Create_m(knobinfo_t *ki, XhWidget parent)
{
  Xh_plot_t *plot;
  
    plot = XhCreatePlot(parent, XH_PLOT_Y_OF_X, 5, 800, 400,
                        XH_PLOT_F_HORZ_SCROLLBAR*1 | XH_PLOT_F_VERT_SCROLLBAR*0,
                        XH_PLOT_P_PARENTRESZ, XtParent(XtParent(parent)),
                        XH_PLOT_P_X_DPYFMT,   "%.0f\xB5s",
                        XH_PLOT_P_END);
    
    return plot->top;
}

static void SUKHANOV_PLOT_SetValue_m(knobinfo_t *ki, double v __attribute__((unused)))
{
}

static knobs_vmt_t SUKHANOV_PLOT_VMT   = {SUKHANOV_PLOT_Create_m, NULL, SUKHANOV_PLOT_SetValue_m, NULL, NULL};

static knobs_widgetinfo_t app_widgetinfo[] =
{
    {LOGD_SUKHANOV_PLOT,   "sukhanov_plot", &SUKHANOV_PLOT_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  app_widgetset   = {app_widgetinfo, NULL};


//////////////////////////////////////////////////////////////////////

static actdescr_t toolslist[] =
{
    XhXXX_TOOLCMD(cmChlNfSaveMode, "Сохранить режим",    btn_save_xpm),
    XhXXX_TOOLLEDS(),
    XhXXX_TOOLCMD(cmChlHelp,       "Краткая справка",    btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD(cmChlQuit,       "Выйти из программы", btn_quit_xpm),
    XhXXX_END()
};

static void CommandProc(XhWindow win, int cmd)
{
  char          filename[PATH_MAX];
  char          comment[400];
  
    switch (cmd)
    {
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


//////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  int  r;

    KnobsRegisterWidgetset(&app_widgetset);

    r = ChlRunApp(argc, argv,
                  app_name, app_class,
                  app_title, NULL,
                  toolslist, CommandProc,
                  "resizable", NULL,
                  app_defserver, app_grouping,
                  app_phys_info, countof(app_phys_info),
                  NULL, NULL);

    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
