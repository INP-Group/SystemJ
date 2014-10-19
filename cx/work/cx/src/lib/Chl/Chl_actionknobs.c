#include "Chl_includes.h"

#include <Xm/PushB.h>

#include "KnobsI.h"


typedef struct
{
    unsigned int  state_rq;
    char          filename[100];
} mode_btn_privrec_t;


static void ActivateCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data)
{
  knobinfo_t                 *ki   = (knobinfo_t *)                 closure;
  mode_btn_privrec_t         *me   = (mode_btn_privrec_t *)         (ki->widget_private);
  XmPushButtonCallbackStruct *info = (XmPushButtonCallbackStruct *) call_data;

  unsigned int                state;
  
    state = 0;
    if (info->event->type == KeyPress     ||  info->event->type == KeyRelease)
        state = ((XKeyEvent *)   (info->event))->state;
    if (info->event->type == ButtonPress  ||  info->event->type == ButtonRelease)
        state = ((XButtonEvent *)(info->event))->state;

    if ((state & me->state_rq) == me->state_rq)
        ChlLoadWindowMode(XhWindowOf(w), me->filename, CDR_OPT_NOLOAD_RANGES);
}

typedef struct
{
    int   color;
    char  filename[100];
    int   rq_shift;
    int   rq_alt;
} mode_btnopts_t;

enum {DEF_COL_IDX = XH_COLOR_CTL3D_BG};

static psp_paramdescr_t text2mode_btnopts[] =
{
    PSP_P_LOOKUP("color", mode_btnopts_t, color,    DEF_COL_IDX, Knobs_colors_lkp),
    PSP_P_STRING("file",  mode_btnopts_t, filename, ""),
    PSP_P_FLAG  ("shift", mode_btnopts_t, rq_shift, 1, 0),
    PSP_P_FLAG  ("alt",   mode_btnopts_t, rq_alt,   1, 0),
    PSP_P_END()
};

static XhWidget MODE_BTN_Create_m(knobinfo_t *ki, XhWidget parent)
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

    strzcpy(me->filename, opts.filename, sizeof(me->filename));
    if (opts.rq_shift) me->state_rq |= ShiftMask;
    if (opts.rq_alt)   me->state_rq |= Mod1Mask;

    cidx = opts.color;
    if (cidx <= 0) cidx = DEF_COL_IDX;
    bg = XhGetColor(cidx);
    XtVaGetValues(parent, XmNcolormap, &cmap, NULL);
    XmGetColors(XtScreen(parent), cmap, bg, &junk, &ts, &bs, &am);

    get_knob_label(ki, &ls);
    w = XtVaCreateManagedWidget(ki->is_rw? "push_i" : "push_o",
                                xmPushButtonWidgetClass,
                                CNCRTZE(parent),
                                XmNtraversalOn,       ki->is_rw,
                                XmNsensitive,         me->filename[0] != '\0',
                                XmNlabelString,       s = XmStringCreateLtoR(ls, xh_charset),
                                XmNbackground,        bg,
                                XmNtopShadowColor,    ts,
                                XmNbottomShadowColor, bs,
                                XmNarmColor,          am,
                                NULL);
    XmStringFree(s);

    XtAddCallback(w, XmNactivateCallback,
                  ActivateCB, (XtPointer)ki);
    
    return ABSTRZE(w);
}

static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

static knobs_vmt_t KNOB_MODE_BTN_VMT  = {MODE_BTN_Create_m,  NULL, NULL, NoColorize_m, NULL};

static knobs_widgetinfo_t actionknobs_widgetinfo[] =
{
    {LOGD_MODE_BTN, "mode_btn", &KNOB_MODE_BTN_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  actionknobs_widgetset = {actionknobs_widgetinfo, NULL};

void RegisterActionknobsWidgetset(void)
{
  static int  is_registered = 0;

    if (is_registered) return;
    is_registered = 1;
    KnobsRegisterWidgetset(&actionknobs_widgetset);
}
