#include "Knobs_includes.h"

/* OpenMotif < 2.2.3 has "ONE_OF_MANY_NNN" behaviour bug (#1188/#1154) */
#define MOTIF_ONE_OF_MANY_NNN_BUG \
    ((XmVERSION*1000000 + XmREVISION * 1000 + XmUPDATE_LEVEL) < 20002003)

typedef enum
{
    LOGD_ALARM,
    LOGD_ONOFF,
    LOGD_GREENLED,
    LOGD_AMBERLED,
    LOGD_REDLED,
    LOGD_BLUELED,
    LOGD_RADIOBTN
} wtype;

typedef enum
{
    SHAPE_DEFAULT = 0,
    SHAPE_CIRCLE,
    SHAPE_DIAMOND
} wshape_t;

typedef struct
{
    int   color;
    int   offcol;
    int   shape;
    int   panel;
    int   nomargins;
    char  victim_name[100];
    int   size;
    int   bold;
    char *icon;
} ledopts_t;


static inline int IsLit(double curv)
{
    return (((int)(curv)) & CX_VALUE_LIT_MASK) != 0;
}

static inline void SetVictimState(knobinfo_t *ki, int state)
{
  knobinfo_t *victim;

    XtVaGetValues(CNCRTZE(ki->indicator), XmNuserData, &victim, NULL);
    if (victim != NULL  &&  victim->indicator != NULL)
        XtSetSensitive(CNCRTZE(victim->indicator), state);
}

static void ChangeCB(Widget     w,
                     XtPointer  closure,
                     XtPointer  call_data)
{
  knobinfo_t                   *ki   = (knobinfo_t *)closure;
  XmToggleButtonCallbackStruct *info = (XmToggleButtonCallbackStruct *) call_data;

    if (ki->is_rw)
    {
        XtVaSetValues(w, XmNset, ki->curv != 0, NULL); /* This extra line appeared somewhen between 2004-07-14 and 2004-10-26; its purpose a bit unknown, but, probably is to overcome Motif bugs #1154 and/or #1188 */
        SetControlValue(ki, info->set == XmUNSET? 0 : CX_VALUE_LIT_MASK, 1);
        SetVictimState (ki, info->set == XmUNSET? 0 : 1);
    }
    else
    {
        /* Restore correct value after user's click */
        XtVaSetValues(w, XmNset, IsLit(ki->curv) ? XmSET : XmUNSET, NULL)/*, fprintf(stderr, "%s: reqd=%d, curv=%d\n", __FUNCTION__, info->set, ki->curv)*/;
        /* And acknowledge alarm, if relevant */
        if ((ki->behaviour & KNOB_B_IS_ALARM)       != 0     &&
            IsLit(ki->curv)                                  &&
            ki->uplink                              != NULL  &&
            ki->uplink->emlink                      != NULL  &&
            ki->uplink->emlink->AckAlarm            != NULL)
            ki->uplink->emlink->AckAlarm(ki->uplink);
    }
}

#if MOTIF_ONE_OF_MANY_NNN_BUG
static void DisarmCB(Widget     w          __attribute__((unused)),
                     XtPointer  closure,
                     XtPointer  call_data  __attribute__((unused)))
{
  knobinfo_t                   *ki   = (knobinfo_t *)closure;

    if ((ki->behaviour & KNOB_B_IS_ALARM)       != 0     &&
        IsLit(ki->curv)                                  &&
        ki->uplink                              != NULL  &&
        ki->uplink->emlink                      != NULL  &&
        ki->uplink->emlink->AckAlarm            != NULL)
        ki->uplink->emlink->AckAlarm(ki->uplink);
}
#endif

static void BlockButton2Handler(Widget     w        __attribute__((unused)),
                                XtPointer  closure  __attribute__((unused)),
                                XEvent    *event,
                                Boolean   *continue_to_dispatch)
{
  XButtonEvent *ev  = (XButtonEvent *)event;
  
    if (ev->type == ButtonPress  &&  ev->button == Button2)
        *continue_to_dispatch = False;
}

static psp_lkp_t shapes_lkp[] =
{
    {"default", SHAPE_DEFAULT},
    {"circle",  SHAPE_CIRCLE},
    {"diamond", SHAPE_DIAMOND},
    {NULL, 0}
};

static psp_paramdescr_t text2ledopts[] =
{
    PSP_P_LOOKUP ("color",  ledopts_t, color,       -1,             Knobs_colors_lkp),
    PSP_P_LOOKUP ("offcol", ledopts_t, offcol,      -1,             Knobs_colors_lkp),
    PSP_P_LOOKUP ("shape",  ledopts_t, shape,       SHAPE_DEFAULT,  shapes_lkp),
    PSP_P_FLAG   ("panel",  ledopts_t, panel,       1,    0),
    PSP_P_FLAG   ("nomargins", ledopts_t, nomargins, 1, 0),
    PSP_P_STRING ("victim", ledopts_t, victim_name, ""),
    PSP_P_LOOKUP ("size",   ledopts_t, size,        -1,             Knobs_sizes_lkp),
    PSP_P_FLAG   ("bold",   ledopts_t, bold,        1,    0),
    PSP_P_FLAG   ("medium", ledopts_t, bold,        0,    1),
    PSP_P_MSTRING("icon",   ledopts_t, icon,        NULL, 100),
    PSP_P_END()
};

static XhWidget DoCreate(knobinfo_t *ki, XhWidget parent, wtype the_look)
{
  Widget      w;
  XmString    s;

  ledopts_t   opts;

  char       *ls;
  int         no_label;
  Dimension   isize;
  Dimension   mt;
  Dimension   mb;
  Dimension   lh;
  XmFontList  lfl;
  unsigned char  ltype;
  
  int         selcol;

  wshape_t    shape = SHAPE_DEFAULT;
  Dimension   t;
  
  int         n;

  void       *top;
  knobinfo_t *victim;

    ki->behaviour |= KNOB_B_IS_LIGHT | KNOB_B_INCDECSTEP_FXD;
  
    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2ledopts);
  
    if (the_look == LOGD_ALARM)      shape = SHAPE_CIRCLE;
    if (the_look == LOGD_RADIOBTN)   shape = SHAPE_DIAMOND;
    if (opts.shape != SHAPE_DEFAULT) shape = opts.shape;

    if ((no_label = !get_knob_label(ki, &ls)))
        ls = " ";
  
    w = XtVaCreateManagedWidget(ki->is_rw? "onoff_i" : "onoff_o",
                                xmToggleButtonWidgetClass,
                                CNCRTZE(parent),
                                XmNlabelString, s = XmStringCreateLtoR(ls, xh_charset),
                                XmNtraversalOn, ki->is_rw,
                                XmNset,         XmINDETERMINATE, // for value to be neither SET nor UNSET, so that it *always* changes on initial read
                                NULL);
    XmStringFree(s);
    MaybeAssignPixmap(w, opts.icon, 1);
    XtVaGetValues(w, XmNlabelType, &ltype, NULL);

    if (opts.size > 0)
    {
      char       fontspec[100];
      Dimension  dth;
      
        check_snprintf(fontspec, sizeof(fontspec),
                       "-*-" XH_FONT_PROPORTIONAL_FMLY "-%s-r-*-*-%d-*-*-*-*-*-*-*",
                       opts.bold? XH_FONT_BOLD_WGHT : XH_FONT_MEDIUM_WGHT,
                       opts.size);
        XtVaSetValues(w,
                      XtVaTypedArg, XmNfontList, XmRString, fontspec, strlen(fontspec)+1,
                      NULL);

        /* Additionally, reduce shadow for small sizes */
        if (opts.size <= 10)
        {
            XtVaGetValues(w, XmNdetailShadowThickness, &dth, NULL);
            if (dth > 1)
                XtVaSetValues(w, XmNdetailShadowThickness, dth / 2, NULL);
        }
    }

    if      (ltype == XmPIXMAP)
    {
        XtVaSetValues(w, XmNlabelType,      XmSTRING, NULL);
        XtVaGetValues(w, XmNindicatorSize, &isize,    NULL);
        XtVaSetValues(w, XmNlabelType,      XmPIXMAP, NULL);
        XtVaSetValues(w, XmNindicatorSize,  isize,    NULL);
    }
    else if (no_label)
    {
        XtAddEventHandler(w, ButtonPressMask, False, BlockButton2Handler, NULL); // Temporary countermeasure against Motif Bug#1117

        XtVaGetValues(w,
                      XmNindicatorSize, &isize,
                      XmNmarginTop,     &mt,
                      XmNmarginBottom,  &mb,
                      XmNfontList,      &lfl,
                      NULL);

        s = XmStringCreate(" ", xh_charset);
        lh = XmStringHeight(lfl, s);
        XmStringFree(s);
        mt += lh / 2;
        mb += lh - lh / 2;
        
        XtVaSetValues(w, XmNindicatorSize, 1, NULL);
        XtVaSetValues(w,
                      XmNlabelString,   s = XmStringCreate("", xh_charset),
                      XmNindicatorSize, isize,
                      XmNmarginTop,     mt,
                      XmNmarginBottom,  mb,
                      XmNspacing,       0,
                      NULL);
        XmStringFree(s);
        if (opts.nomargins  && 1)
        {
            XtVaSetValues(w,
                          XmNmarginLeft,   isize-2,
                          XmNmarginRight,  0,
                          XmNmarginTop,    isize-2,
                          XmNmarginBottom, 0,
                          XmNmarginHeight, 0,
                          XmNmarginWidth,  0,
                          XmNhighlightThickness, 0,
                          NULL);
        }
        if (ki->ident != NULL  &&  strcmp(ki->ident, "zzz") == 0  &&  0){
          Dimension l, r, t, b, mh, mw;
          
            XtVaGetValues(w,
                          XmNmarginLeft,   &l,
                          XmNmarginRight,  &r,
                          XmNmarginTop,    &t,
                          XmNmarginBottom, &b,
                          XmNmarginHeight, &mh,
                          XmNmarginWidth,  &mw,
                          NULL);
            fprintf(stderr, "%d %d %d %d, %d %d isize=%d\n", l, r, t, b, mh, mw, isize);
        }
    }

    /* Remember the dependent/victim knob */
    if (opts.victim_name[0] != '\0'  &&
        ki->uplink != NULL           &&
        ki->uplink->content != NULL)
    {
        top = (ki->uplink != NULL)?  ki->uplink->grouplist_link : NULL;
        victim = datatree_FindNodeFrom(top, opts.victim_name, ki->uplink);
        if (victim != NULL)
            XtVaSetValues(w, XmNuserData, victim, NULL);
        else
            fprintf(stderr, "%s::%s(%s/\"%s\"): victim=\"%s\" not found\n",
                    __FILE__, __FUNCTION__, ki->ident, ki->label, opts.victim_name);
    }
    
    /* "Color management": */
    /* a. Choose color, depending on kind... */
    switch (the_look)
    {
        case LOGD_ONOFF:    selcol = -1;                  break;
        case LOGD_ALARM:    selcol = XH_COLOR_JUST_RED;   break;
        case LOGD_GREENLED: selcol = XH_COLOR_JUST_GREEN; break;
        case LOGD_AMBERLED: selcol = XH_COLOR_JUST_AMBER; break;
        case LOGD_REDLED:   selcol = XH_COLOR_JUST_RED;   break;
        case LOGD_BLUELED:  selcol = XH_COLOR_JUST_BLUE;  break;
        case LOGD_RADIOBTN: selcol = XH_COLOR_JUST_BLUE;  break;
        default:            selcol = XH_COLOR_BG_DEFUNCT;
    }

    /* b. ...and allow it to be overridden from widdepinfo */
    if (opts.color > 0) selcol = opts.color;

    /* c. Set if required */
    if (selcol >= 0)
        XtVaSetValues(w, XmNselectColor, XhGetColor(selcol), NULL);

    /* d. Use off-color, if specified */
    if (opts.offcol >= 0)
        XtVaSetValues(w, XmNunselectColor, XhGetColor(opts.offcol), NULL);
    
    if (opts.panel)
    {
        XtVaGetValues(w, XmNmarginHeight, &t, NULL);
        XtVaSetValues(w,
                      XmNfillOnSelect,    True,
                      XmNindicatorOn,     XmINDICATOR_NONE,
                      XmNmarginHeight,    t - 1,
                      XmNshadowThickness, 1,
                      XmNalignment,       XmALIGNMENT_CENTER,
                      NULL);
    }
    else
    {
        if (shape == SHAPE_CIRCLE)
            XtVaSetValues(w, XmNindicatorType, XmONE_OF_MANY_ROUND,   NULL);
        if (shape == SHAPE_DIAMOND)
            XtVaSetValues(w, XmNindicatorType, XmONE_OF_MANY_DIAMOND, NULL);
    }

    XtAddCallback(w, XmNvalueChangedCallback,
                  ChangeCB, (XtPointer)ki);
#if MOTIF_ONE_OF_MANY_NNN_BUG
    XtAddCallback(w, XmNdisarmCallback,
                  DisarmCB, (XtPointer)ki);
#endif

    HookPropsWindow(ki, w);
    
    return ABSTRZE(w);
}

static XhWidget ALARM_Create_m   (knobinfo_t *ki, XhWidget parent)
{
    ki->behaviour |= KNOB_B_IS_ALARM;
    return DoCreate(ki, parent, LOGD_ALARM);
}

static XhWidget ONOFF_Create_m   (knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, LOGD_ONOFF);
}

static XhWidget GREENLED_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, LOGD_GREENLED);
}

static XhWidget AMBERLED_Create_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, LOGD_AMBERLED);
}

static XhWidget REDLED_Create_m  (knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, LOGD_REDLED);
}

static XhWidget BLUELED_Create_m (knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, LOGD_BLUELED);
}

static XhWidget RADIOBTN_Create_m (knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, LOGD_RADIOBTN);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
  unsigned char  is_set     = IsLit(v) ? XmSET : XmUNSET;
  Boolean        is_enabled = (((int)v) & CX_VALUE_DISABLED_MASK) == 0  &&  ki->is_rw;
  unsigned char  cur_set;

    /* Handle B_IS_LIGHT protocol */
    XtVaGetValues(CNCRTZE(ki->indicator), XmNset, &cur_set, NULL);
    if (is_set != cur_set)  // A countermeasure/overcome for OpenMotif bug #1134
    {
        XtVaSetValues(CNCRTZE(ki->indicator), XmNset, is_set, NULL);
        SetVictimState(ki, IsLit(v));
    }

    /* Handle B_IS_BUTTON::disableable protocol */
    if (ki->is_rw  &&  XtIsSensitive(CNCRTZE(ki->indicator)) != is_enabled)
        XtSetSensitive(CNCRTZE(ki->indicator), is_enabled);
}

knobs_vmt_t KNOBS_ALARM_VMT    = {ALARM_Create_m,    NULL, SetValue_m, CommonColorize_m, NULL};
knobs_vmt_t KNOBS_ONOFF_VMT    = {ONOFF_Create_m,    NULL, SetValue_m, CommonColorize_m, NULL};
knobs_vmt_t KNOBS_GREENLED_VMT = {GREENLED_Create_m, NULL, SetValue_m, CommonColorize_m, NULL};
knobs_vmt_t KNOBS_AMBERLED_VMT = {AMBERLED_Create_m, NULL, SetValue_m, CommonColorize_m, NULL};
knobs_vmt_t KNOBS_REDLED_VMT   = {REDLED_Create_m,   NULL, SetValue_m, CommonColorize_m, NULL};
knobs_vmt_t KNOBS_BLUELED_VMT  = {BLUELED_Create_m,  NULL, SetValue_m, CommonColorize_m, NULL};
knobs_vmt_t KNOBS_RADIOBTN_VMT = {RADIOBTN_Create_m, NULL, SetValue_m, CommonColorize_m, NULL};
