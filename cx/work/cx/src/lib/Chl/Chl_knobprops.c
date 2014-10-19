#include "Chl_includes.h"

#include <Xm/MwmUtil.h>

#include <Xm/DialogS.h>
#include <Xm/Frame.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>

#include "KnobsI.h" // For SetTextString()


typedef kpropsdlg_t dlgrec_t;
static dlgrec_t *ThisDialog(XhWindow win);


#define VAL_COL_COMMA ", "
#define N_A_S         "n/a"

static char *range_names[] = {"Normal range", "Yellow range", "Graph range"};

//////////////////////////////////////////////////////////////////////

static void MakeRowLabel(Widget grid, int y, const char *caption)
{
  Widget        l;
  XmString      s;
  
    l = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                XmNlabelString, s = XmStringCreateLtoR(caption, xh_charset),
                                NULL);
    XmStringFree(s);
    
    XhGridSetChildPosition (l, 0,   y);
    XhGridSetChildFilling  (l, 0,   0);
    XhGridSetChildAlignment(l, -1, -1);
}

static Widget MakePropLabel(Widget grid, int y)
{
  Widget        l;
  XmString      s;
  
    l = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                XmNlabelString,   s = XmStringCreateLtoR(/*"NNNNNNNNNNNNNNNNNNNN"*/"", xh_charset),
//                                XmNrecomputeSize, False,
                                XmNalignment,     XmALIGNMENT_BEGINNING,
                                NULL);
    XmStringFree(s);
    
    XhGridSetChildPosition (l, 1,   y);
    XhGridSetChildFilling  (l, 1,   0);
    XhGridSetChildAlignment(l, -1, -1);
    
    return l;
}

static Widget MakePropText(Widget grid, int y)
{
  Widget        t;
  
    t = XtVaCreateManagedWidget("text_o", xmTextWidgetClass, grid,
                                XmNscrollHorizontal,      False,
                                XmNcursorPositionVisible, False,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                XmNcolumns,               20/*!!!*/,
                                NULL);
    
    XhGridSetChildPosition (t, 1,   y);
    XhGridSetChildFilling  (t, 0,   0);
    XhGridSetChildAlignment(t, -1, -1);
    
    return t;
}

static Widget MakePropInput(Widget grid, int y)
{
  Widget        t;
  
    t = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, grid,
                                XmNscrollHorizontal,      False,
                                XmNcursorPositionVisible, False,
                                XmNeditable,              True,
                                XmNtraversalOn,           True,
                                XmNcolumns,               20/*!!!*/,
                                NULL);
    
    XhGridSetChildPosition (t, 1,   y);
    XhGridSetChildFilling  (t, 0,   0);
    XhGridSetChildAlignment(t, -1, -1);
    
    SetCursorCallback(t);
    
    return t;
}

static void SetInputValue(dlgrec_t *rec, Widget w, double v)
{
  char      buf[400];

    XtVaSetValues(w,
                  XmNvalue,
                  snprintf_dbl_trim(buf, sizeof(buf), rec->the_knob->dpyfmt, v, 1),
                  NULL);
}

static void SetIntInput(dlgrec_t *rec, Widget w, int iv)
{
  char      buf[400];

    XtVaSetValues(w,
                  XmNvalue,
                  snprintf_dbl_trim(buf, sizeof(buf), "%5.0f", (double)iv, 1),
                  NULL);
}


static void SnprintfRange(char *buf, size_t bufsize,
                          double vmin, double vmax,
                          const char *dpyfmt)
{
  char      minbuf[400],  maxbuf[400];
  
    if (vmin >= vmax)
        snprintf(buf, bufsize, "None");
    else
        snprintf(buf, bufsize, "[%s,%s]",
                 snprintf_dbl_trim(minbuf, sizeof(minbuf), dpyfmt, vmin, 1),
                 snprintf_dbl_trim(maxbuf, sizeof(maxbuf), dpyfmt, vmax, 1));
}

//////////////////////////////////////////////////////////////////////

static void RangeDpyLosingFocusCB(Widget     w,
                                  XtPointer  closure,
                                  XtPointer  call_data __attribute__((unused)))
{
  dlgrec_t *rec = closure;
  double    v;
  
    if (!rec->is_closing)
        ExtractDoubleValue(w, &v); // Just for beep
    //XhHideHilite(w);
}

static void RangeDpyModifyCB(Widget     w,
                             XtPointer  closure,
                             XtPointer  call_data __attribute__((unused)))
{
  int *chg_p = closure;

    *chg_p = 1;
    //XhShowHilite(w);
}

static void RangeDpyCreate(dlgrec_t *rec, int which,
                           Widget grid, int y)
{
  kprops_range_t *rp = rec->z + which;
  int             txt;
  int             mnx;
  int             rw;
  Widget          w;

    rp->form = XtVaCreateManagedWidget("form", xmFormWidgetClass, grid,
                                       NULL);
    XhGridSetChildPosition (rp->form,  1,  y);
    XhGridSetChildAlignment(rp->form, -1, -1);

    rp->dash = XtVaCreateManagedWidget("text_o", xmTextWidgetClass, rp->form,
                                       XmNvalue,                 "-",
                                       XmNcursorPositionVisible, False,
                                       XmNcolumns,               1,
                                       XmNscrollHorizontal,      False,
                                       XmNeditMode,              XmSINGLE_LINE_EDIT,
                                       XmNeditable,              False,
                                       XmNtraversalOn,           False,
                                       NULL);

    /* Note 1: we create shd *first* to be "visible" ABOVE dpy */
    for (txt = 0;  txt < KPROPS_TXT_COUNT; txt++)
        for (mnx = 0;  mnx < KPROPS_MNX_COUNT; mnx++)
        {
            rw = (txt == KPROPS_TXT_INP);
            rp->txt[txt][mnx] = w =
                XtVaCreateManagedWidget(rw? "text_i" : "text_o",
                                        xmTextWidgetClass, rp->form,
                                        XmNvalue,                 " ",
                                        XmNcursorPositionVisible, False,
                                        XmNcolumns,               10,
                                        XmNscrollHorizontal,      False,
                                        XmNeditMode,              XmSINGLE_LINE_EDIT,
                                        XmNeditable,              rw,
                                        XmNtraversalOn,           rw,
                                        NULL);
            if (rw)
            {
                SetCursorCallback(w);
                XtAddCallback(w, XmNlosingFocusCallback,
                              RangeDpyLosingFocusCB, rec);
                XtAddCallback(w, XmNmodifyVerifyCallback,
                              RangeDpyModifyCB,      rec->chg.ranges + which);

            }
        }
    XtVaSetValues(rp->txt[KPROPS_TXT_INP][0],
                  XmNleftAttachment, XmATTACH_FORM,
                  NULL);
    XtVaSetValues(rp->dash,
                  XmNleftAttachment, XmATTACH_WIDGET,
                  XmNleftWidget,     rp->txt[KPROPS_TXT_INP][0],
                  NULL);
    XtVaSetValues(rp->txt[KPROPS_TXT_INP][1],
                  XmNleftAttachment, XmATTACH_WIDGET,
                  XmNleftWidget,     rp->dash,
                  NULL);
    /* Note 2: this loop can NOT be merged with upper one, due to creation order */
    for (mnx = 0;  mnx < KPROPS_MNX_COUNT; mnx++)
    {
        XtVaSetValues(rp->txt[KPROPS_TXT_SHD][mnx],
                      XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
                      XmNrightAttachment,  XmATTACH_OPPOSITE_WIDGET,
                      XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
                      XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                      XmNleftWidget,       rp->txt[KPROPS_TXT_INP][mnx],
                      XmNrightWidget,      rp->txt[KPROPS_TXT_INP][mnx],
                      XmNtopWidget,        rp->txt[KPROPS_TXT_INP][mnx],
                      XmNbottomWidget,     rp->txt[KPROPS_TXT_INP][mnx],
                      NULL);
    }
}

static void RangeDpySetState(dlgrec_t *rec, int which, int editable)
{
  int       columns;

    columns = GetTextColumns(rec->the_knob->dpyfmt, NULL) + 1/*cursor*/ + 2/*user's misprints*/;
    XtVaSetValues(rec->z[which].txt[KPROPS_TXT_INP][0], XmNcolumns, columns, NULL);
    XtVaSetValues(rec->z[which].txt[KPROPS_TXT_INP][1], XmNcolumns, columns, NULL);

    if ((rec->z[which].editable = editable))
    {
        XtSetSensitive    (rec->z[which].txt[KPROPS_TXT_INP][0], True);
        XtSetSensitive    (rec->z[which].txt[KPROPS_TXT_INP][1], True);
        XtUnmanageChildren(rec->z[which].txt[KPROPS_TXT_SHD], 2);
    }
    else
    {
        XtManageChildren  (rec->z[which].txt[KPROPS_TXT_SHD], 2);
        XtSetSensitive    (rec->z[which].txt[KPROPS_TXT_INP][0], False);
        XtSetSensitive    (rec->z[which].txt[KPROPS_TXT_INP][1], False);
    }
}

static void RangeDpyShowValues(dlgrec_t *rec, int which)
{
    SetInputValue(rec, rec->z[which].txt[KPROPS_TXT_INP][0], rec->mins[which]);
    SetInputValue(rec, rec->z[which].txt[KPROPS_TXT_INP][1], rec->maxs[which]);
}

//////////////////////////////////////////////////////////////////////

#if !RANGES_INLINE
static void DisplayRanges(dlgrec_t *rec)
{
  int       r;
  double    rmin,         rmax;
    
  char      buf[300];
  XmString  s;
  
    for (r = 0;  r < 3;  r++)
    {
        rmin = rec->mins[r];
        rmax = rec->maxs[r];
        
        SnprintfRange(buf, sizeof(buf), rmin, rmax, rec->the_knob->dpyfmt);
        
        XtVaSetValues(rec->range_lbl[r], XmNlabelString, s = XmStringCreateLtoR(buf, xh_charset), NULL);
        XmStringFree(s);
    }
}
#endif

static void DisplayCurVals(dlgrec_t *rec)
{
  knobinfo_t *ki = rec->the_knob;
  char        buf[400];
  int         frsize;
  int         shift;
  int         col;

  Pixel       fg;
  Pixel       bg;

    snprintf_dbl_trim(buf, sizeof(buf), ki->dpyfmt, ki->curv, 0);
    frsize = sizeof(buf)-1 - strlen(buf) - strlen(VAL_COL_COMMA);
    if (frsize > 0  &&  ki->colformula != NULL)
    {
        strcat(buf, VAL_COL_COMMA); // Already accounted in above calc
        snprintf(buf + strlen(buf), frsize,
                 ki->dpyfmt, ki->cur_cfv);
    }
    XtVaSetValues(rec->val_dpy, XmNvalue, buf, NULL);

    if (ki->curv_raw_useful)
        sprintf(buf, "%d", ki->curv_raw);
    else
        sprintf(buf, N_A_S);
    XtVaSetValues(rec->raw_dpy, XmNvalue, buf, NULL);

    sprintf(buf, "%d", ki->curtag);
    XtVaSetValues(rec->age_dpy, XmNvalue, buf, NULL);

    if (((rec->rflags_shown ^ ki->currflags) & ((1 << NUM_PROP_FLAGS) - 1)) != 0)
        for (shift = NUM_PROP_FLAGS - 1;  shift >= 0;  shift--)
            if (((rec->rflags_shown ^ ki->currflags) & (1 << shift)) != 0)
            {
                col = (ki->currflags & (1 << shift)) == 0?
                    XH_COLOR_FG_DIM
                    :
                    (CXCF_FLAG_HWERR_MASK & (1 << shift))? XH_COLOR_BG_HWERR
                                                         : XH_COLOR_BG_SFERR;
                XtVaSetValues(rec->rflags_leds[shift],
                              XmNbackground, XhGetColor(col),
                              NULL);
            }
    rec->rflags_shown = ki->currflags;

    if (ki->colstate != rec->colstate_shown)
    {
        ChooseKnobColors(ki->color, ki->colstate,
                         rec->col_deffg, rec->col_defbg,
                         &fg, &bg);
        XtVaSetValues(rec->col_dpy,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      XmNvalue,      CdrStrcolalarmShort(ki->colstate),
                      NULL);
        rec->colstate_shown = ki->colstate;
    }
}


#if !RANGES_INLINE
static void ShowRangeDialog(dlgrec_t *rec, int which)
{
  XmString  s;
  int       columns;
  
    rec->r.which = which;
  
    XtVaSetValues(rec->r.title,
                  XmNlabelString, s = XmStringCreateLtoR(range_names[which], xh_charset),
                  NULL);
    
    SetInputValue(rec, rec->r.i1, rec->mins[which]);
    SetInputValue(rec, rec->r.i2, rec->maxs[which]);

    columns = GetTextColumns(rec->the_knob->dpyfmt, NULL) + 1/*cursor*/ + 2/*user's misprints*/;
    XtVaSetValues(rec->r.i1, XmNcolumns, columns, NULL);
    XtVaSetValues(rec->r.i2, XmNcolumns, columns, NULL);
    
    XhStdDlgShow(rec->r.box);
}

static void RangeNormCB(Widget     w          __attribute__((unused)),
                        XtPointer  closure,
                        XtPointer  call_data  __attribute__((unused)))
{
    ShowRangeDialog((dlgrec_t *)closure, KNOBS_RANGE_NORM);
}

static void RangeYelwCB(Widget     w          __attribute__((unused)),
                        XtPointer  closure,
                        XtPointer  call_data  __attribute__((unused)))
{
    ShowRangeDialog((dlgrec_t *)closure, KNOBS_RANGE_YELW);
}

static void RangeDispCB(Widget     w          __attribute__((unused)),
                        XtPointer  closure,
                        XtPointer  call_data  __attribute__((unused)))
{
    ShowRangeDialog((dlgrec_t *)closure, KNOBS_RANGE_DISP);
}

static void MakeChgButton(Widget grid, int y, dlgrec_t *rec, int which)
{
  Widget          b;
  XmString        s;
  
  static XtCallbackProc  cbs[3] = {RangeNormCB, RangeYelwCB, RangeDispCB};
  
    b = XtVaCreateManagedWidget("push_i", xmPushButtonWidgetClass, grid,
                                XmNtraversalOn, False,
                                XmNsensitive,   1,
                                XmNlabelString, s = XmStringCreateLtoR("...", xh_charset),
                                NULL);
    XmStringFree(s);

    XhGridSetChildPosition (b, 2,   y);
    XhGridSetChildFilling  (b, 0,   0);
    XhGridSetChildAlignment(b, -1, -1);

    XtAddCallback(b, XmNactivateCallback, cbs[which], (XtPointer) rec);
}
#endif

static void StepChgCB(Widget     w          __attribute__((unused)),
                      XtPointer  closure,
                      XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  
    rec->chg.step = 1;
}

static void GrpcChgCB(Widget     w          __attribute__((unused)),
                      XtPointer  closure,
                      XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  
    rec->chg.grpcoeff = 1;
}

static void FrqdChgCB(Widget     w          __attribute__((unused)),
                      XtPointer  closure,
                      XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  
    rec->chg.frqdiv = 1;
}

static void PropsOkCB(Widget     w          __attribute__((unused)),
                      XtPointer  closure,
                      XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t   *rec = (dlgrec_t *) closure;
  knobinfo_t *ki  = rec->the_knob;
  knobinfo_t  old;
  int         changed;
  double      v;
  int         iv;
  int         r;
  int         stage;

    old = *ki;
    changed = 0;

    for (stage = 0;  stage <= 1;  stage++)
    {
        if (rec->chg.step)
        {
            if (!ExtractDoubleValue(rec->stp_inp, &v)) return;
            
            if (stage != 0)
            {
                ki->incdec_step = v;
                changed = 1;
            }
        }
    
        if (rec->chg.grpcoeff)
        {
            if (!ExtractDoubleValue(rec->gcf_inp, &v)) return;
            
            if (stage != 0)
            {
                ki->grpcoeff = v;
                changed = 1;
                ki->grpcoeffchg = 1;
            }
        }
    
        if (rec->chg.frqdiv)
        {
            if (!ExtractIntValue(rec->fqd_inp, &iv)) return;
    
            if (stage != 0)
            {
                ki->histring_frqdiv    = iv;
                ki->histring_frqdivchg = 1;
            }
        }
    
        /*!!! NOTE: because of checks from ExtractZzzValue() intermixed with
                    storing into ki->, a PARTIAL modification is possible:
                    1) first some value is stored,
                    2) than some error is detected and [OK] stopped,
                    3) Esc is pressed;
                       but some values are already stored in ki!
                    (see also ki->incdes_step and others above)*/
        for (r = 0;  r < 3;  r++)
            if (rec->chg.ranges[r])
            {
#if RANGES_INLINE
                if (!ExtractDoubleValue(rec->z[r].txt[KPROPS_TXT_INP][0], rec->mins + r)  ||
                    !ExtractDoubleValue(rec->z[r].txt[KPROPS_TXT_INP][1], rec->maxs + r)) return;
#endif
                if (stage != 0)
                {
                    ki->rmins[r] = rec->mins[r];
                    ki->rmaxs[r] = rec->maxs[r];
                    ki->rchg [r] = 1;
                    changed = 1;
                }
            }
    }

    XhStdDlgHide(rec->box);

    if (changed  &&
        ki->vmtlink != NULL  &&
        ki->vmtlink->PropsChg != NULL)
        ki->vmtlink->PropsChg(ki, &old);

    /*!!! Here should also re-colorize, if ranges have changed */

    /*!!! Here should also notify graphs, if any */
    if (changed)
        UpdatePlotProps(XhWindowOf(w));
}


static void PropsCancelCB(Widget     w          __attribute__((unused)),
                          XtPointer  closure,
                          XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;

#if RANGES_INLINE
    rec->is_closing = 1;
#endif
    XhStdDlgHide(rec->box);
#if !RANGES_INLINE
    XhStdDlgHide(rec->r.box);
    XhStdDlgHide(rec->r.yesno_box);
#endif
}

#if !RANGES_INLINE
static void PropsRangeYesNoOkCB(Widget     w          __attribute__((unused)),
                                XtPointer  closure,
                                XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;

    rec->mins      [rec->r.which] = rec->r.v1;
    rec->maxs      [rec->r.which] = rec->r.v2;
    rec->chg.ranges[rec->r.which] = 1;
    DisplayRanges(rec);
  
    XhStdDlgHide(rec->r.yesno_box);
}

static void PropsRangeYesNoCancelCB(Widget     w          __attribute__((unused)),
                                    XtPointer  closure,
                                    XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  
    XhStdDlgHide(rec->r.yesno_box);
}

static void SetYesNoLabel(dlgrec_t *rec)
{
  int       r;
  char      buf[1000];
  char      fbuf[100], tbuf[100];
  XmString  s;
    
    SnprintfRange(fbuf, sizeof(fbuf),
                  rec->mins[rec->r.which], rec->maxs[rec->r.which],
                  rec->the_knob->dpyfmt);
    SnprintfRange(tbuf, sizeof(tbuf),
                  rec->r.v1, rec->r.v2,
                  rec->the_knob->dpyfmt);
    
    snprintf(buf, sizeof(buf),
             "Do you really want to change\n"
             "\"%s\"\n"
             "from\n"
             "%s\n"
             "to\n"
             "%s?",
             range_names[rec->r.which],
             fbuf,
             tbuf
            );

    XtVaSetValues(rec->r.yesno_box,
                  XmNmessageString, s = XmStringCreateLtoR(buf, xh_charset),
                  NULL);
    XmStringFree(s);
}

static void PropsRangeOkCB(Widget     w          __attribute__((unused)),
                           XtPointer  closure,
                           XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  double     v1, v2;
  
    if (!ExtractDoubleValue(rec->r.i1, &v1)  ||
        !ExtractDoubleValue(rec->r.i2, &v2)) return;
        
    rec->r.v1 = v1;
    rec->r.v2 = v2;

    XhStdDlgHide(rec->r.box);

    SetYesNoLabel(rec);
    XhStdDlgShow(rec->r.yesno_box);
}

static void PropsRangeCancelCB(Widget     w          __attribute__((unused)),
                               XtPointer  closure,
                               XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  
    XhStdDlgHide(rec->r.box);
}
#endif

static dlgrec_t *ThisDialog(XhWindow win)
{
  dlgrec_t  *rec = &(Priv2Of(win)->kp);
  
  char       buf[100];

  Widget     w;
  XmString   s;

  Widget     grid;
  Widget     fc;

  int        row;

  int        shift;

  int        r;
  
  Widget     rf;
  Widget     rg;
  
    if (rec->initialized) return rec;

    check_snprintf(buf, sizeof(buf), "%s: Knob properties", Priv2Of(win)->sysname);
    rec->box = XhCreateStdDlg(win, "knobProps", buf, NULL, "Свойства ручки",
                              XhStdDlgFOk | XhStdDlgFCancel |
                              XhStdDlgFModal*0 | XhStdDlgFNoAutoUnmng);
    XtAddCallback(XtParent(rec->box), XmNpopdownCallback, PropsCancelCB, (XtPointer)rec);
    XtAddCallback(rec->box, XmNokCallback,     PropsOkCB,     (XtPointer)rec);
    XtAddCallback(rec->box, XmNcancelCallback, PropsCancelCB, (XtPointer)rec);

    grid = XhCreateGrid(rec->box, "grid");
    XhGridSetGrid(grid, 0, 0);
    
    row = 0;

    MakeRowLabel(grid, row, "Path");
    rec->pth_dpy = MakePropText(grid, row); XtVaSetValues(rec->pth_dpy, XmNcolumns, 30, NULL);
    row++;

    MakeRowLabel(grid, row, "Name");
    rec->nam_dpy = MakePropText(grid, row); XtVaSetValues(rec->nam_dpy, XmNcolumns, 30, NULL);
    row++;
    
    MakeRowLabel(grid, row, "Label");
    rec->lbl_lbl = MakePropLabel(grid, row);
    row++;

    MakeRowLabel(grid, row, "Value");
    rec->val_dpy = MakePropText(grid, row);
    row++;
    
    MakeRowLabel(grid, row, "Raw");
    rec->raw_dpy = MakePropText(grid, row);
    row++;
    
    MakeRowLabel(grid, row, "Age");
    rec->age_dpy = MakePropText(grid, row);
    row++;
    
    MakeRowLabel(grid, row, "Flags");
    fc = XhCreateGrid(grid, "rflags_grid");
    XhGridSetChildPosition (fc, 1,   row);
    XhGridSetChildFilling  (fc, 0,   0);
    XhGridSetChildAlignment(fc, -1,  0);
    XhGridSetGrid(fc, 0, 0);
    XhGridSetPadding(fc, 0, 0);
    XtVaSetValues(fc,
                  XmNmarginHeight, 0,
                  NULL);
    for (shift = NUM_PROP_FLAGS - 1;  shift >= 0;  shift--)
    {
        snprintf(buf, sizeof(buf), "%02d", shift);
        w = rec->rflags_leds[shift] =
            XtVaCreateManagedWidget("rflagLed", xmLabelWidgetClass, fc,
                                    XmNlabelString,  s = XmStringCreateLtoR(buf, xh_charset),
                                    XmNtraversalOn,  False,
                                    XmNbackground,   XhGetColor(XH_COLOR_FG_DIM),
                                    NULL);
        XmStringFree(s);
        
        XhGridSetChildPosition (w, NUM_PROP_FLAGS - 1 - shift,   0);
        XhGridSetChildFilling  (w, 0,   0);
        XhGridSetChildAlignment(w, -1, -1);

        XhSetBalloon(w, cx_strrflag_long(shift));
    }
    XhGridSetSize(fc, NUM_PROP_FLAGS, 1);
    row++;

    MakeRowLabel(grid, row, "State");
    rec->col_dpy = MakePropText(grid, row);
    row++;
    XtVaGetValues(rec->col_dpy,
                  XmNforeground, &(rec->col_deffg),
                  XmNbackground, &(rec->col_defbg),
                  NULL);
    
    MakeRowLabel(grid, row, "Source");
    rec->src_lbl = MakePropLabel(grid, row);
    row++;

    for (r = 0;  r < 3;  r++)
    {
        MakeRowLabel(grid, row, range_names[r]);
#if RANGES_INLINE
        RangeDpyCreate(rec, r, grid, row);
#else
        rec->range_lbl[r] = MakePropLabel(grid, row);
        MakeChgButton(grid, row, rec, r);
#endif
        row++;
    }
    
    MakeRowLabel(grid, row, "Step");
    rec->stp_inp = MakePropInput(grid, row);
    XtAddCallback(rec->stp_inp, XmNmodifyVerifyCallback, StepChgCB, (XtPointer)rec);
    row++;

    MakeRowLabel(grid, row, "Group. coeff.");
    rec->gcf_inp = MakePropInput(grid, row);
    XtAddCallback(rec->gcf_inp, XmNmodifyVerifyCallback, GrpcChgCB, (XtPointer)rec);
    row++;

    MakeRowLabel(grid, row, "Hist. frqdiv.");
    rec->fqd_inp = MakePropInput(grid, row);
    XtAddCallback(rec->fqd_inp, XmNmodifyVerifyCallback, FrqdChgCB, (XtPointer)rec);
    row++;


#if !RANGES_INLINE
    /* Range dialog */
    rec->r.box = XhCreateStdDlg(win, "knobPropsRange", "Range", NULL, NULL,
                                XhStdDlgFOk | XhStdDlgFCancel |
                                XhStdDlgFModal | XhStdDlgFNoAutoUnmng);
    XtAddCallback(rec->r.box, XmNokCallback,     PropsRangeOkCB,     (XtPointer)rec);
    XtAddCallback(rec->r.box, XmNcancelCallback, PropsRangeCancelCB, (XtPointer)rec);

    rf = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, rec->r.box,
                                 NULL);
    
    rec->r.title = XtVaCreateManagedWidget("title", xmLabelWidgetClass, rf,
                                           XmNframeChildType, XmFRAME_TITLE_CHILD,
                                           NULL);

    rg = XhCreateGrid(rf, "grid");
    XhGridSetGrid(rg, 0, 0);
    XhGridSetSize(rg, 3, 1);
    
    rec->r.i1 = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, rg,
                                        XmNscrollHorizontal,      False,
                                        XmNcursorPositionVisible, False,
                                        XmNeditable,              True,
                                        XmNtraversalOn,           True,
                                        XmNcolumns,               20/*!!!*/,
                                        NULL);
    XhGridSetChildPosition(rec->r.i1, 0, 0);
    SetCursorCallback(rec->r.i1);

    w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, rg,
                                XmNlabelString, s = XmStringCreateLtoR("-", xh_charset),
                                NULL);
    XmStringFree(s);
    XhGridSetChildPosition(w, 1, 0);
    
    rec->r.i2 = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, rg,
                                        XmNscrollHorizontal,      False,
                                        XmNcursorPositionVisible, False,
                                        XmNeditable,              True,
                                        XmNtraversalOn,           True,
                                        XmNcolumns,               20/*!!!*/,
                                        NULL);
    XhGridSetChildPosition(rec->r.i2, 2, 0);
    SetCursorCallback(rec->r.i2);

    /* Range "Yes/No" dialog */
    rec->r.yesno_box = XhCreateStdDlg(win, "knobPropsRangeYesNo", "Confirm", NULL, "",
                                      XhStdDlgFOk | XhStdDlgFCancel | XhStdDlgFCenterMsg |
                                      XhStdDlgFModal | XhStdDlgFNoAutoUnmng);
    XtAddCallback(rec->r.yesno_box, XmNokCallback,     PropsRangeYesNoOkCB,     (XtPointer)rec);
    XtAddCallback(rec->r.yesno_box, XmNcancelCallback, PropsRangeYesNoCancelCB, (XtPointer)rec);
#endif

    rec->initialized = 1;
    
    return rec;
}

static inline char *NOTEMPTY(char *s)
{
    return s != NULL  &&  *s != '\0'? s : " ";
}

#if 0
static void SetTargetKnob(dlgrec_t *rec, Knob ki)
{
    rec->the_knob = ki;
    rec->colstate_shown = COLALARM_UNINITIALIZED;

    if (CdrSrcOf(ki, &srvref, &chan_n) == 1)
        snprintf(buf, sizeof(buf), "%s!%d", srvref, chan_n);
    else
        strcpy(buf, "n/a");

    XtVaSetValues(rec->nam_dpy, XmNvalue,       NOTEMPTY(ki->ident), NULL);
    XtVaSetValues(rec->lbl_lbl, XmNlabelString, s = XmStringCreateLtoR(NOTEMPTY(ki->label), xh_charset), NULL);
    XmStringFree(s);
    XtVaSetValues(rec->src_lbl, XmNlabelString, s = XmStringCreateLtoR(buf, xh_charset), NULL);
    XmStringFree(s);
}
#endif

void Chl_E_ShowPropsWindow(Knob ki)
{
  XhWindow    win = XhWindowOf(ki->indicator);
  dlgrec_t   *rec = ThisDialog(win);
  XmString    s;
  int         r;
  const char *srvref;
  int         chan_n;
  char        buf[100];

  char        pth_buf[200];
  int         pbx;
  int         id_len;
  ElemInfo    pe;
    
    if (XtIsManaged(rec->box)&&0)
        return;

    /* Register new "client" */
    rec->the_knob = ki;
    rec->colstate_shown = COLALARM_UNINITIALIZED;

    if (CdrSrcOf(ki, &srvref, &chan_n) == 1)
        snprintf(buf, sizeof(buf), "%s!%d", srvref, chan_n);
    else
        strcpy(buf, N_A_S);

    for (pe =  ki->uplink, pbx = sizeof(pth_buf) - 1, pth_buf[pbx] = '\0';
         pe != NULL;
         pe =  pe->uplink)
    {
        if (pe->ident == NULL)
        {
            strcpy(pth_buf, "-");
            pbx = 0;
            goto END_PTH_BUILD;
        }

        if (strcmp(pe->ident, ":") == 0) continue;

        id_len = strlen(pe->ident);

        /* Is there a room for "ident."? */
        if (id_len + 1 > pbx)
        {
            if (pbx == 0) pbx = 1;
            pth_buf[--pbx] = '*';
            goto END_PTH_BUILD;
        }

        /* Pre-pend '.' if not at EOL */
        if (pth_buf[pbx] != '\0') pth_buf[--pbx] = '.';
        /* And pre-pend the name */
        pbx -= id_len;
        memcpy(pth_buf + pbx, pe->ident, id_len);
    }
 END_PTH_BUILD:;

    XtVaSetValues(rec->pth_dpy, XmNvalue, pth_buf + pbx, NULL);
    XtVaSetValues(rec->nam_dpy, XmNvalue, NOTEMPTY(ki->ident), NULL);
    XtVaSetValues(rec->lbl_lbl, XmNlabelString, s = XmStringCreateLtoR(NOTEMPTY(ki->label), xh_charset), NULL);
    XmStringFree(s);
    XtVaSetValues(rec->src_lbl, XmNlabelString, s = XmStringCreateLtoR(buf, xh_charset), NULL);
    XmStringFree(s);

    for (r = 0;  r < 3; r++)
    {
        rec->mins[r] = ki->rmins[r];
        rec->maxs[r] = ki->rmaxs[r];
    }

#if RANGES_INLINE
    for (r = 0;  r < 3;  r++)
    {
        RangeDpyShowValues(rec, r);
        RangeDpySetState  (rec, r, 1);
    }
    rec->is_closing = 0;
#else
    DisplayRanges(rec);
#endif
    DisplayCurVals(rec);

    if (ki->is_rw  &&  !(ki->behaviour & KNOB_B_INCDECSTEP_FXD))
    {
        SetInputValue(rec, rec->stp_inp, ki->incdec_step);
        XtSetSensitive(rec->stp_inp, True);
    }
    else
    {
        XtVaSetValues (rec->stp_inp, XmNvalue, N_A_S, NULL);
        XtSetSensitive(rec->stp_inp, False);
    }

    if (ki->is_rw  &&  (ki->behaviour & KNOB_B_IS_GROUPABLE))
    {
        SetInputValue(rec, rec->gcf_inp, ki->grpcoeff);
        XtSetSensitive(rec->gcf_inp, True);
    }
    else
    {
        XtVaSetValues (rec->gcf_inp, XmNvalue, N_A_S, NULL);
        XtSetSensitive(rec->gcf_inp, False);
    }

    SetIntInput(rec, rec->fqd_inp, ki->histring_frqdiv);
    
    /* Note: we must "bzero" AFTER setting the value in stp_inp, since that setting triggers the callback */
    bzero(&(rec->chg), sizeof(rec->chg));

    /* Show the window */
    XhStdDlgShow(rec->box);
////    fprintf(stderr, "mmm!\n");
}

void UpdatePropsWindow(XhWindow win)
{
  dlgrec_t *rec = &(Priv2Of(win)->kp);
    
    if (!rec->initialized) return;

    if (XtIsManaged(rec->box)) DisplayCurVals(rec);
}
