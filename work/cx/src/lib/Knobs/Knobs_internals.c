#include "Knobs_includes.h"
#include "cxdata.h" /* It is here instead of _includes.h for LOGC_* only */
#include "cx_sysdeps.h"
#include "findfilein.h"
#include <limits.h> /* Is PATH_MAX here? */


psp_lkp_t Knobs_colors_lkp[] =
{
    {"default", -1},

    {"red",     XH_COLOR_JUST_RED},
    {"orange",  XH_COLOR_JUST_ORANGE},
    {"yellow",  XH_COLOR_JUST_YELLOW},
    {"green",   XH_COLOR_JUST_GREEN},
    {"blue",    XH_COLOR_JUST_BLUE},
    {"indigo",  XH_COLOR_JUST_INDIGO},
    {"violet",  XH_COLOR_JUST_VIOLET},
    
    {"black",   XH_COLOR_JUST_BLACK},
    {"white",   XH_COLOR_JUST_WHITE},
    {"gray",    XH_COLOR_JUST_GRAY},

    {"amber",   XH_COLOR_JUST_AMBER},
    {"brown",   XH_COLOR_JUST_BROWN},
    
    {NULL, 0}
};

psp_lkp_t Knobs_sizes_lkp [] =
{
    {"default", -1},
    {"tiny",     8},
    {"small",   10},
    {"normal",  12},
    {"large",   18},
    {"huge",    24},
    {"giant",   34},
    {NULL, 0}
};

const char  Knobs_wdi_equals_c    = '=';
const char *Knobs_wdi_separators  = " \t,";
const char *Knobs_wdi_terminators = "";


int ParseWiddepinfo(knobinfo_t *ki,
                    void *rec, size_t rec_size, psp_paramdescr_t *table)
{
  int  r;
    
    bzero(rec, rec_size);
    r = psp_parse(ki->widdepinfo, NULL,
                  rec,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  table);
    if (r != PSP_R_OK)
    {
        fprintf(stderr, "%s: [%s/\"%s\"].widdepinfo: %s\n",
                __FUNCTION__, ki->ident, ki->label, psp_error());
        if (r == PSP_R_USRERR)
            psp_parse("", NULL,
                      rec,
                      Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                      table);
        else
            bzero(rec, rec_size);
    }
    
    return r;
}


typedef struct
{
    char   *buf;
    size_t  bufsize;
} maybeassign_info_t;

static void *maybeassign_check_proc(const char *name,
                                    const char *path,
                                    void       *privptr)
{
  maybeassign_info_t *info = privptr;
  char                plen = strlen(path);

    check_snprintf(info->buf, info->bufsize, "%s%s%s.xpm",
                   path,
                   plen == 0  ||  path[plen-1] == '/'? "" : "/",
                   name);

    /* Note: use of F_OK and not R_OK is intentional -- for cases when
             a file with "this" name exists but isn't readable or of
             wrong type, an error would occur, helping the user to
             locate and solve a problem (while just skipping the non-R_OK
             file would tangle the situation). */
    return access(info->buf, F_OK) == 0? info->buf : NULL;
}

void MaybeAssignPixmap(Widget w, const char *label, int from_widdepinfo)
{
  const char         *lp;
  char                buf[PATH_MAX];
  maybeassign_info_t  buf_info;
    
    if (label == NULL) return;
    if (from_widdepinfo)
    {
        lp = label;
        if (*lp == '\0') return;
    }
    else
    {
        if (memcmp(label, KNOB_LABEL_IMG_PFX_S, strlen(KNOB_LABEL_IMG_PFX_S)) != 0)
            return;
        lp = label + strlen(KNOB_LABEL_IMG_PFX_S);
    }
  
    buf_info.buf     = buf;
    buf_info.bufsize = sizeof(buf);
    
    if (findfilein(lp,
#if OPTION_HAS_PROGRAM_INVOCATION_NAME
                   program_invocation_name,
#else
                   NULL,
#endif /* OPTION_HAS_PROGRAM_INVOCATION_NAME */
                   maybeassign_check_proc,
                   &buf_info,
                   /*!!!DIRSTRUCT*/
#define PIX_DIRLIST(dir) "!/../"dir \
    FINDFILEIN_SEP "$PULT_ROOT/"dir \
    FINDFILEIN_SEP "~/pult/"dir
                   "./"
                       FINDFILEIN_SEP "!/"
                       FINDFILEIN_SEP PIX_DIRLIST("lib/icons")
                       FINDFILEIN_SEP PIX_DIRLIST("include/pixmaps")) != NULL)
        XhAssignPixmapFromFile(ABSTRZE(w), buf);
}


void ChooseKnobColors(int color, colalarm_t newstate,
                      XhPixel  deffg, Pixel  defbg,
                      XhPixel *newfg, Pixel *newbg)
{
  int  fgi = -1;
  int  bgi = -1;
    
    switch (color)
    {
        case LOGC_HILITED:   fgi = XH_COLOR_FG_HILITED;    break;
        case LOGC_IMPORTANT: fgi = XH_COLOR_FG_IMPORTANT;  break;
        case LOGC_VIC:       bgi = XH_COLOR_BG_VIC;        break;
        case LOGC_DIM:       fgi = XH_COLOR_FG_DIM;        break;
        case LOGC_HEADING:   fgi = XH_COLOR_FG_HEADING;
                             bgi = XH_COLOR_BG_HEADING;    break;
        default: ;
    }
    switch (newstate)
    {
        case COLALARM_JUSTCREATED:
            bgi = XH_COLOR_BG_JUSTCREATED;
            break;
        
        case COLALARM_YELLOW:
            bgi = (color == LOGC_IMPORTANT)? XH_COLOR_BG_IMP_YELLOW
                                           : XH_COLOR_BG_NORM_YELLOW;
            break;

        case COLALARM_WEIRD:
            fgi = XH_COLOR_FG_WEIRD;
            bgi = XH_COLOR_BG_WEIRD;
            break;
            
        case COLALARM_RED:
            bgi = (color == LOGC_IMPORTANT)? XH_COLOR_BG_IMP_RED
                                           : XH_COLOR_BG_NORM_RED;
            break;
            
        case COLALARM_DEFUNCT:
            bgi = XH_COLOR_BG_DEFUNCT;
            break;
            
        case COLALARM_HWERR:
            bgi = XH_COLOR_BG_HWERR;
            break;
            
        case COLALARM_SFERR:
            bgi = XH_COLOR_BG_SFERR;
            break;
            
        case COLALARM_NOTFOUND:
            bgi = XH_COLOR_BG_NOTFOUND;
            break;
            
        case COLALARM_RELAX:
            bgi = XH_COLOR_JUST_GREEN;
            break;

        case COLALARM_OTHEROP:
            bgi = XH_COLOR_BG_OTHEROP;
            break;

        case COLALARM_PRGLYCHG:
            bgi = XH_COLOR_BG_PRGLYCHG;
            break;

        default:;
    }
    
    *newfg = fgi < 0? deffg : XhGetColor(fgi);
    *newbg = bgi < 0? defbg : XhGetColor(bgi);
}

void ChooseWColors(knobinfo_t *ki, colalarm_t newstate,
                   Pixel *newfg, Pixel *newbg)
{
    ChooseKnobColors(ki->color, newstate,
                     ki->wdci.deffg, ki->wdci.defbg,
                     newfg, newbg);
}

void CommonColorize_m(knobinfo_t *ki, colalarm_t newstate)
{
  Pixel         fg;
  Pixel         bg;
  
    ChooseWColors(ki, newstate, &fg, &bg);
    
    XtVaSetValues(CNCRTZE(ki->indicator),
                  XmNforeground, fg,
                  XmNbackground, bg,
                  NULL);
}


static GC AllocXhGC(Widget w, int idx, const char *fontspec)
{
  XtGCMask     mask;
  XGCValues    vals;
  XFontStruct *finfo;

    mask = GCFunction | GCForeground | GCLineWidth;
    vals.function   = GXcopy;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 0;

    if (fontspec != NULL  &&  fontspec[0] != '\0')
    {
        finfo = XLoadQueryFont(XtDisplay(w), fontspec);
        if (finfo == NULL)
        {
            fprintf(stderr, "Unable to load font \"%s\"; using \"fixed\"\n", fontspec);
            finfo = XLoadQueryFont(XtDisplay(w), "fixed");
        }
        vals.font = finfo->fid;

        mask |= GCFont;
    }

    return XtGetGC(w, mask, &vals);
}

void AllocStateGCs(XhWidget w, GC table[], int norm_idx)
{
  Widget xw = CNCRTZE(w);
  int    i;
    
    table[COLALARM_UNINITIALIZED] = AllocXhGC(xw, XH_COLOR_BG_JUSTCREATED, NULL);
    for (i = 0;  i <= COLALARM_LAST;  i++)
        table[i] = table[COLALARM_UNINITIALIZED];

    table[COLALARM_NONE]     = AllocXhGC(xw, norm_idx,                NULL);
    table[COLALARM_YELLOW]   = AllocXhGC(xw, XH_COLOR_BG_NORM_YELLOW, NULL);
    table[COLALARM_RED]      = AllocXhGC(xw, XH_COLOR_BG_NORM_RED,    NULL);
    table[COLALARM_DEFUNCT]  = AllocXhGC(xw, XH_COLOR_BG_DEFUNCT,     NULL);
    table[COLALARM_HWERR]    = AllocXhGC(xw, XH_COLOR_BG_HWERR,       NULL);
    table[COLALARM_SFERR]    = AllocXhGC(xw, XH_COLOR_BG_SFERR,       NULL);
    table[COLALARM_NOTFOUND] = AllocXhGC(xw, XH_COLOR_BG_NOTFOUND,    NULL);
    table[COLALARM_RELAX]    = AllocXhGC(xw, XH_COLOR_JUST_GREEN,     NULL);
    table[COLALARM_OTHEROP]  = AllocXhGC(xw, XH_COLOR_BG_OTHEROP,     NULL);
    table[COLALARM_PRGLYCHG] = AllocXhGC(xw, XH_COLOR_BG_PRGLYCHG,    NULL);
}

int  CheckForDoubleClick(Widget   w,
                         XEvent  *event,
                         Boolean *continue_to_dispatch)
{
  XButtonEvent *ev  = (XButtonEvent *)event;
  int           ret;

  /*!!! Note: this "static" approach would be incorrect in
   case of multi-display application, where the right solution would be
   to add eleminfo_t.prev_click_{w,t}.
   12.11.2005: Correction: not eleminfo_t -- but datainfo_t.prec_click_{w,t}.
   But who cares? :-) */
  static Widget  prev_w = NULL; 
  static Time    prev_t = 0;
  
    if (ev->type != ButtonPress  ||  ev->button != Button1) return 0;

    ret = (prev_w == w  &&
           ev->time - prev_t <= XtGetMultiClickTime(XtDisplay(w)));

    prev_w = w;
    prev_t = ev->time;
    if (ret) *continue_to_dispatch = False;

    return ret;
}

void UserBeganInputEditing(knobinfo_t *ki)
{
    ki->usertime = time(NULL);
    XhShowHilite(ki->indicator);
}

void CancelInputEditing   (knobinfo_t *ki)
{
    if (ki->usertime != 0)
        SetControlValue(ki,
                        ki->userval_valid? ki->userval : ki->curv,
                        0);
}


static void StoreUndoValue(knobinfo_t *ki)
{
    ki->undo_val       = ki->userval_valid? ki->userval : ki->curv;
    ki->undo_val_saved = 1;
}

static void PerformUndo   (knobinfo_t *ki)
{
    if (ki->undo_val_saved) SetControlValue(ki, ki->undo_val, 1);
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

void SetControlValue      (knobinfo_t *ki, double v, int fromlocal)
{
    if (datatree_SetControlValue(ki, v, fromlocal) > 0) XBell(XtDisplay(CNCRTZE(ki->indicator)), 100);
}


void KnobsMouse3Handler(Widget     w  __attribute__((unused)),
                        XtPointer  closure,
                        XEvent    *event,
                        Boolean   *continue_to_dispatch)
{
  knobinfo_t          *ki  = (knobinfo_t *)          closure;
  XButtonPressedEvent *ev  = (XButtonPressedEvent *) event;
  eleminfo_t          *ei  = ki->uplink;

////    fprintf(stderr, "zzz!\n");
    if (ev->button != Button3) return;
    *continue_to_dispatch = False;

    if (ei != NULL  &&  ei->emlink!= NULL)
    {
        if      (ev->state & ControlMask)
        {
            if (ei->emlink->ShowBigVal != NULL)
                ei->emlink->ShowBigVal(ki);
        }
        else if (ev->state & ShiftMask)
        {
            if (ei->emlink->ToHistPlot != NULL)
                ei->emlink->ToHistPlot(ki);
        }
        else
        {
            if (ei->emlink->ShowProps != NULL)
                ei->emlink->ShowProps(ki);
        }
    }
}

static void KnobsKbdHandler(Widget     w  __attribute__((unused)),
                            XtPointer  closure,
                            XEvent    *event,
                            Boolean   *continue_to_dispatch)
{
  XKeyEvent    *ev = (XKeyEvent *)  event;
  knobinfo_t   *ki = (knobinfo_t *) closure;
  eleminfo_t   *ei  = ki->uplink;
  KeySym        ks;
  Modifiers     mr;

    XtTranslateKeycode(XtDisplay(w), ev->keycode, ev->state, &mr, &ks);

    if (ei != NULL  &&  ei->emlink!= NULL)
    {
        if      (ks == XK_space  &&  (ev->state & ControlMask))
        {
            if (ei->emlink->ShowBigVal != NULL)
                ei->emlink->ShowBigVal(ki);
        }
        else if (ks == XK_space  &&  (ev->state & ShiftMask))
        {
            if (ei->emlink->ToHistPlot != NULL)
                ei->emlink->ToHistPlot(ki);
        }
        else if (
                 ((ks == XK_Return  ||  ks == XK_KP_Enter  ||  ks == osfXK_SwitchDirection  ||  ks == osfXK_Activate)  &&  (ev->state & Mod1Mask))
                 ||
                 ((ks == XK_F10  ||  ks == osfXK_Menu  ||  ks == osfXK_MenuBar)  &&  (ev->state & ShiftMask))
                )
        {
            if (ei->emlink->ShowProps != NULL)
                ei->emlink->ShowProps(ki);
        }
        else return;

        *continue_to_dispatch = False;
    }
}


static void KnobsChangeHiliteState(Knob k, int onoff)
{
    if (onoff) XhShowHilite(k->indicator);
    else       XhHideHilite(k->indicator);
}

void HookPropsWindow(knobinfo_t *ki, Widget w)
{
    XtInsertEventHandler(w, ButtonPressMask, False, KnobsMouse3Handler, (XtPointer)ki, XtListHead);
    XtAddEventHandler   (w, KeyPressMask,    False, KnobsKbdHandler,    (XtPointer)ki);
    set_knob_editstate_hook(KnobsChangeHiliteState);
}


/* Common-textfield interface */

static void TextChangeCB(Widget     w,
                         XtPointer  closure,
                         XtPointer  call_data)
{
  knobinfo_t          *ki   = (knobinfo_t *)          closure;
  XmAnyCallbackStruct *info = (XmAnyCallbackStruct *) call_data;
  double               v;

    /* Do a CANCEL if losing focus */
    if (info->reason != XmCR_ACTIVATE)
    {
        CancelInputEditing(ki);
        return;
    }

    if (ExtractDoubleValue(w, &v))
    {
        StoreUndoValue (ki);
        SetControlValue(ki, v, 1);
    }
}

static void TextUserCB(Widget     w __attribute__((unused)),
                       XtPointer  closure,
                       XtPointer  call_data)
{
  knobinfo_t                 *ki   = (knobinfo_t *)                 closure;
  XmTextVerifyCallbackStruct *info = (XmTextVerifyCallbackStruct *) call_data;
    
    if (info->reason == XmCR_MOVING_INSERT_CURSOR ||
        info->event  != NULL)
        UserBeganInputEditing(ki);
}


static void HandleTextUpDown(Widget        w,
                             knobinfo_t   *ki,
                             unsigned int  state,
                             Boolean       up)
{
  int           factor;
  Boolean       immed;
  double        v;

  int           x;
  knobinfo_t   *victim;
  double        grpcoeff;

    factor = 100;
    if (state & MOD_FINEGRAIN_MASK) factor = 10;
    if (state & MOD_BIGSTEPS_MASK)  factor = 1000;
    immed = (state & MOD_NOSEND_MASK) == 0;

//    fprintf(stderr, "ks=%04x, factor=%d, immed=%d\n", ks, factor, immed);
    
    if (!ExtractDoubleValue(w, &v)) return;

//    fprintf(stderr, "%8.3f\n", v);

    if (up)
        v += (ki->incdec_step * factor) / 100;
    else
        v -= (ki->incdec_step * factor) / 100;
    
    if (immed)
    {
        StoreUndoValue (ki);
        SetControlValue(ki, v, 1);
        if (ki->is_ingroup)
        {
            if (ki->uplink != NULL)
            {
                for (x = 0, victim = ki->uplink->content;
                     x < ki->uplink->count;
                     x++,   victim++)
                    if (victim != ki                 &&
                        victim->type == LOGT_WRITE1  &&
                        victim->is_ingroup)
                    {
                        grpcoeff = victim->grpcoeff;
                        if (grpcoeff == 0.0) grpcoeff = 1.0;
                        ////fprintf(stderr, "%p: curv=%8.3f userval=%8.3f from=%8.3f delta=%8.3f\n", victim, victim->curv, victim->userval, (victim->userval_valid? victim->userval : victim->curv), (ki->incdec_step * factor) / 100 * (up? +1 : -1));
                        SetControlValue(victim,
                                        (
                                         (victim->userval_valid? victim->userval : victim->curv) +
                                         (ki->incdec_step * factor) / 100
                                         * grpcoeff
                                         * (up? +1 : -1)
                                        ),
                                        1);
                    }
            }
        }
    }
    else
    {
        UserBeganInputEditing(ki);
      
        /* Set the value in input line */
        ki->vmtlink->SetValue(ki, v);
    }
}

static void TextKbdHandler(Widget     w,
                           XtPointer  closure,
                           XEvent    *event,
                           Boolean   *continue_to_dispatch)
{
  knobinfo_t   *ki = (knobinfo_t *) closure;
  XKeyEvent    *ev = (XKeyEvent *)  event;
  KeySym        ks;
  Modifiers     mr;
  Boolean       up;

    if (event->type != KeyPress) return;
    XtTranslateKeycode(XtDisplay(w), ev->keycode, ev->state, &mr, &ks);
    
    ////fprintf(stderr, "key=%04x\n", ks);
    
    if ((ks == XK_Escape  ||  ks == osfXK_Cancel)  &&
        (ev->state & (ShiftMask | ControlMask | Mod1Mask)) == 0
        &&  ki->usertime != 0)
    {
        CancelInputEditing(ki);
        *continue_to_dispatch = False;
        return;
    }

    if (
        ((ks == XK_Z  ||  ks == XK_z)  &&  (ev->state & ControlMask) != 0)
        ||
        ((ks == XK_BackSpace  ||  ks == osfXK_BackSpace)  &&  (ev->state & Mod1Mask) != 0)
       )
    {
        PerformUndo(ki);
        *continue_to_dispatch = False;
        return;
    }
    
    if      (ks == XK_Up    ||  ks == XK_KP_Up    ||  ks == osfXK_Up)   up = True;
    else if (ks == XK_Down  ||  ks == XK_KP_Down  ||  ks == osfXK_Down) up = False;
    else return;
    
    *continue_to_dispatch = False;

    HandleTextUpDown(w, ki, ev->state, up);
}

static void TextWheelHandler(Widget     w,
                             XtPointer  closure,
                             XEvent    *event,
                             Boolean   *continue_to_dispatch)
{
  knobinfo_t          *ki  = (knobinfo_t *)          closure;
  XButtonPressedEvent *ev  = (XButtonPressedEvent *) event;
  Boolean              up;

    if      (ev->button == Button4) up = True;
    else if (ev->button == Button5) up = False;
    else return;

    *continue_to_dispatch = False;

    HandleTextUpDown(w, ki, ev->state, up);
}


static void TextSelectionCB(Widget     w,
                            XtPointer  closure,
                            XtPointer  call_data)
{
  XmTextVerifyCallbackStruct *info = (XmTextVerifyCallbackStruct *) call_data;
    
    ////fprintf(stderr, "zzz\n");
    //XtCallActionProc(w, "copy-clipboard", info->event, NULL, 0);
    //XmTextCopy(w, XtLastTimestampProcessed(XtDisplay(w)));
}

static Widget MakeTextWidget(Widget parent, int rw, int columns)
{
  Widget  w;
    
    w = XtVaCreateManagedWidget(rw? "text_i" : "text_o",
                                xmTextWidgetClass,
                                parent,
                                XmNvalue,                 rw? "": "--------",
                                XmNcursorPositionVisible, False,
                                XmNcolumns,               columns,
                                XmNscrollHorizontal,      False,
                                XmNeditMode,              XmSINGLE_LINE_EDIT,
                                XmNeditable,              rw,
                                XmNtraversalOn,           rw,
                                NULL);

    XtAddCallback(w, XmNgainPrimaryCallback, TextSelectionCB, NULL);

    return w;
}

Widget  CreateTextValue(knobinfo_t *ki, Widget parent)
{
  Widget  w;
  int     columns = GetTextColumns(ki->dpyfmt, ki->units);

    w = MakeTextWidget(parent, False, columns);
  
    return w;
}

Widget  CreateTextInput(knobinfo_t *ki, Widget parent)
{
  Widget  w;
  int     columns = GetTextColumns(ki->dpyfmt, NULL) + 1;
    
    w = MakeTextWidget(parent, True, columns);

    XtAddCallback(w, XmNactivateCallback,     TextChangeCB, (XtPointer)ki);
    XtAddCallback(w, XmNlosingFocusCallback,  TextChangeCB, (XtPointer)ki);
    
    XtAddCallback(w, XmNmodifyVerifyCallback, TextUserCB,   (XtPointer)ki);
    XtAddCallback(w, XmNmotionVerifyCallback, TextUserCB,   (XtPointer)ki);
    
    SetCursorCallback(w);
    
    XtAddEventHandler(w, KeyPressMask,    False, TextKbdHandler,   (XtPointer)ki);
    XtAddEventHandler(w, ButtonPressMask, False, TextWheelHandler, (XtPointer)ki);

    return w;
}

void    SetTextString  (knobinfo_t *ki, Widget w, double v)
{
  char      buf[1000];
  int       len;
  int       frsize;
  char     *p;
  Boolean   ed;

    XtVaGetValues(w, XmNeditable, &ed, NULL);

    len = snprintf(buf, sizeof(buf), ki->dpyfmt, v);
    if (len < 0  ||  (size_t)len > sizeof(buf)-1) buf[len = sizeof(buf)-1] = '\0';
    p = buf + len;
    while (p > buf  &&  *(p - 1) == ' ') *--p = '\0'; /* Trim trailing spaces */
    if (!ed  &&  ki->units != NULL)
    {
        frsize = sizeof(buf)-1 - len;
        if (frsize > 0) snprintf(buf + len, frsize, "%s", ki->units);
    }
    
    p = buf;
    if (ed)
        while (*p == ' ') p++; /* Skip spaces */
    XmTextSetString(w, p);
}


static void TextFocusCursorCB(Widget     w,
                              XtPointer  closure,
                              XtPointer  call_data __attribute__((unused)))
{
    XtVaSetValues(w, XmNcursorPositionVisible, (int)closure, NULL);
}

void    SetCursorCallback (Widget w)
{
    XtAddCallback(w, XmNfocusCallback,
                  (XtCallbackProc) TextFocusCursorCB, (XtPointer)True);
    XtAddCallback(w, XmNlosingFocusCallback,
                  (XtCallbackProc) TextFocusCursorCB, (XtPointer)False);
}

int     ExtractDoubleValue(Widget w, double *result)
{
  double  v;
  char   *err;
  char   *text = XmTextGetString(w);
  int     ret;

  char   *p;
  int     minprio;
  char    op;
  int     prio;
  double  v2;

    v = strtod(text, &err);
#if 0
    ret = !(err == text  ||  *err != '\0');
#else
    ret = !(err == text);

    for (p = err, minprio = 1;
         ret != 0;
         p = err)
    {
        while (*p != '\0'  &&  isspace(*p)) p++;
        if (*p == '\0') break;

        err = p;

        op = *p;
        if (op != '+'  &&  op != '-'  &&  op != '*'  &&  op != '/')
        {
            ret = 0;
            goto END_PARSE;
        }

        /* Check priority -- against "a+b*c" (=>"(a+b)*c") */
        prio = (op == '*'  ||  op == '/');
        if (prio > minprio)
        {
            ret = 0;
            goto END_PARSE;
        }
        minprio = prio;

        p++;
        v2 = strtod(p, &err);
        ret = !(err == p);

        if      (!ret)      break;
        else if (op == '+') v += v2;
        else if (op == '-') v -= v2;
        else if (op == '*') v *= v2;
        else if (op == '/') v /= v2;
    }

 END_PARSE:
#endif
    if (ret)
    {
        *result = v;
    }
    else
    {
       XBell(XtDisplay(w), 100);
       XmTextSetInsertionPosition (w, err - text);
       XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    }

    XtFree(text);

    return ret;
}

int     ExtractIntValue   (Widget w, int    *result)
{
  int     v;
  char   *err;
  char   *text = XmTextGetString(w);
  int     ret;

    v = strtol(text, &err, 10);
    ret = !(err == text  ||  *err != '\0');

    if (ret)
    {
        *result = v;
    }
    else
    {
       XBell(XtDisplay(w), 100);
       XmTextSetInsertionPosition (w, err - text);
       XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    }
    
    XtFree(text);

    return ret;
}


/* Common-button interface */

psp_paramdescr_t text2Knobs_buttonopts[] =
{
    PSP_P_LOOKUP ("color",  Knobs_buttonopts_t, color, XH_COLOR_JUST_RED, Knobs_colors_lkp),
    PSP_P_LOOKUP ("bg",     Knobs_buttonopts_t, bg,    -1,                Knobs_colors_lkp),
    PSP_P_LOOKUP ("size",   Knobs_buttonopts_t, size,  -1,                Knobs_sizes_lkp),
    PSP_P_FLAG   ("bold",   Knobs_buttonopts_t, bold,  1,                 0),
    PSP_P_FLAG   ("medium", Knobs_buttonopts_t, bold,  0,                 1),
    PSP_P_MSTRING("icon",   Knobs_buttonopts_t, icon,  NULL,              100),
    PSP_P_END()
};

void              TuneButtonKnob(Widget              w,
                                 Knobs_buttonopts_t *opts_p, 
                                 int                 flags)
{
  XhPixel         bg;
  Colormap        cmap;
  XhPixel         junk;
  XhPixel         ts;
  XhPixel         bs;
  XhPixel         am;

  char            fontspec[100];

    if (opts_p->bg > 0)
    {
        bg = XhGetColor(opts_p->bg);
        XtVaGetValues(XtParent(w), XmNcolormap, &cmap, NULL);
        XmGetColors(XtScreen(XtParent(w)), cmap, bg, &junk, &ts, &bs, &am);

        XtVaSetValues(w,
                      XmNbackground,        bg,
                      XmNtopShadowColor,    ts,
                      XmNbottomShadowColor, bs,
                      XmNarmColor,          am,
                      NULL);
    }

    if (opts_p->size > 0  &&  (flags & TUNE_BUTTON_KNOB_F_NO_FONT) == 0)
    {
        check_snprintf(fontspec, sizeof(fontspec),
                       "-*-" XH_FONT_PROPORTIONAL_FMLY "-%s-r-*-*-%d-*-*-*-*-*-*-*",
                       opts_p->bold? XH_FONT_BOLD_WGHT : XH_FONT_MEDIUM_WGHT,
                       opts_p->size);
        XtVaSetValues(w,
                      XtVaTypedArg, XmNfontList, XmRString, fontspec, strlen(fontspec)+1,
                      NULL);
    }

    if ((flags & TUNE_BUTTON_KNOB_F_NO_PIXMAP) == 0)
        MaybeAssignPixmap(w, opts_p->icon, 1);
}
