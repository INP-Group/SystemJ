#include "Chl_includes.h"

#include <Xm/PushB.h>

#include "KnobsI.h"


typedef struct
{
} exec_btn_privrec_t;


typedef struct
{
} exec_btnopts_t;

enum {DEF_COL_IDX = XH_COLOR_CTL3D_BG};

static psp_paramdescr_t text2exec_btnopts[] =
{
    PSP_P_END()
};

static XhWidget EXEC_BTN_Create_m(knobinfo_t *ki, XhWidget parent)
{
  mode_btn_privrec_t  *me;

  Widget          w;
  XmString        s;
  char           *ls;

  mode_btnopts_t  opts;
  int             cidx;
  XhPixel         bg;
  Colormap        cmap;
  XhPixel         junk;
  XhPixel         ts;
  XhPixel         bs;
  XhPixel         am;
    
    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2mode_btnopts);
    
    /* Allocate the private info structure... */
    if ((ki->widget_private = XtMalloc(sizeof(mode_btn_privrec_t))) == NULL)
        return NULL;
    me = (mode_btn_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    return ABSTRZE(w);
}

static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

static knobs_vmt_t KNOB_EXEC_BTN_VMT  = {EXEC_BTN_Create_m,  NULL, NULL, NoColorize_m, NULL};

static knobs_widgetinfo_t exec_knob_widgetinfo[] =
{
    {LOGD_EXEC_BTN, "exec_btn", &KNOB_EXEC_BTN_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  exec_knob_widgetset = {exec_knob_widgetinfo, NULL};

void RegisterExecKnobWidgetset(void)
{
  static int  is_registered = 0;

    if (is_registered) return;
    is_registered = 1;
    KnobsRegisterWidgetset(&exec_knob_widgetset);
}
