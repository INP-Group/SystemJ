#include "Chl_includes.h"


#define VAL_COL_COMMA ", "
#define N_A_S         "n/a"

enum
{
    UNLIT_RFL_COLOR = XH_COLOR_FG_DIM
};

static char *range_names[4] =
{
    "Input range",
    "Normal range",
    "Yellow range",
    "Graph range",
};


typedef knobprops_rec_t dlgrec_t;

static void CreateKnobPropsDialog(XhWindow  win, DataSubsys subsys, dlgrec_t *rec);

static dlgrec_t *ThisDialog(XhWindow win)
{
  chl_privrec_t *privrec = _ChlPrivOf(win);
  dlgrec_t      *rec     = &(privrec->kp);
  
    if (!rec->initialized) CreateKnobPropsDialog(win, privrec->subsys, rec);
    return rec;
}


static void PropsOkCB(Widget     w          __attribute__((unused)),
                      XtPointer  closure,
                      XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t   *rec = (dlgrec_t *) closure;
}

static void PropsCancelCB(Widget     w          __attribute__((unused)),
                          XtPointer  closure,
                          XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t  *rec = (dlgrec_t *) closure;
  
    rec->is_closing = 1;
    XhStdDlgHide(rec->box);
}

static Widget MakeGridLabel(CxWidget grid, int x, int y, const char *text)
{
  Widget        w;
  XmString      s;
  
    w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, CNCRTZE(grid),
                                XmNlabelString, s = XmStringCreateLtoR(text, xh_charset),
                                XmNalignment,   XmALIGNMENT_BEGINNING,
                                NULL);
    XmStringFree(s);
    
    XhGridSetChildPosition (ABSTRZE(w),  x,  y);
    XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
    XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

    return w;
}

static Widget MakeRowLabel (CxWidget grid, int y, const char *text)
{
#if 1
    return MakeGridLabel(grid, 0, y, text);
#else
  Widget        w;
  XmString      s;
  
    w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, CNCRTZE(grid),
                                XmNlabelString, s = XmStringCreateLtoR(text, xh_charset),
                                XmNalignment,   XmALIGNMENT_BEGINNING,
                                NULL);
    XmStringFree(s);
    
    XhGridSetChildPosition (ABSTRZE(w),  0,  y);
    XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
    XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

    return w;
#endif
}

static Widget MakePropLabel(CxWidget grid, int y)
{
  Widget        w;
  XmString      s;
  
    w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, CNCRTZE(grid),
                                XmNlabelString, s = XmStringCreateLtoR("", xh_charset),
                                XmNalignment,   XmALIGNMENT_BEGINNING,
                                NULL);
    XmStringFree(s);
    
    XhGridSetChildPosition (ABSTRZE(w),  1,  y);
    XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
    XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

    return w;
}

static Widget MakePropText (CxWidget grid, int y)
{
  Widget        w;
  
    w = XtVaCreateManagedWidget("text_o", xmTextWidgetClass, CNCRTZE(grid),
                                XmNscrollHorizontal,      False,
                                XmNcursorPositionVisible, False,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                XmNcolumns,               30/*!!! this would be changed later*/,
                                NULL);

    XhGridSetChildPosition (ABSTRZE(w),  1,  y);
    XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
    XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

    return w;
}

static Widget MakePropInput(CxWidget grid, int y)
{
  Widget        w;
  
    w = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, CNCRTZE(grid),
                                XmNscrollHorizontal,      False,
                                XmNcursorPositionVisible, False,
                                XmNeditable,              True,
                                XmNtraversalOn,           True,
                                XmNcolumns,               20/*!!! this would be changed later*/,
                                NULL);
    
    XhGridSetChildPosition (ABSTRZE(w),  1,  y);
    XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
    XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

    MotifKnobs_SetTextCursorCallback(w);

    return w;
}

//////////////////////////////////////////////////////////////////////

static void RangeDpyLosingFocusCB(Widget     w,
                                  XtPointer  closure,
                                  XtPointer  call_data __attribute__((unused)))
{
  dlgrec_t *rec = closure;
  double    v;
  
    if (!rec->is_closing)
        MotifKnobs_ExtractDoubleValue(w, &v); // Just for beep
    //XhHideHilite(w);
}

static void RangeDpyModifyCB(Widget     w,
                             XtPointer  closure,
                             XtPointer  call_data __attribute__((unused)))
{
  int8 *chg_p = closure;

    *chg_p = 1;
    //XhShowHilite(w);
}

static void RangeDpyCreate(dlgrec_t *rec, int which,
                           Widget grid, int y)
{
  kprops_range_t *rp = rec->r + which;
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
                MotifKnobs_SetTextCursorCallback(w);
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

    columns = GetTextColumns(rec->k->u.k.dpyfmt, NULL) + 1/*cursor*/ + 2/*user's misprints*/;
    XtVaSetValues(rec->r[which].txt[KPROPS_TXT_INP][0], XmNcolumns, columns, NULL);
    XtVaSetValues(rec->r[which].txt[KPROPS_TXT_INP][1], XmNcolumns, columns, NULL);

    if ((rec->r[which].editable = editable))
    {
        XtSetSensitive    (rec->r[which].txt[KPROPS_TXT_INP][0], True);
        XtSetSensitive    (rec->r[which].txt[KPROPS_TXT_INP][1], True);
        XtUnmanageChildren(rec->r[which].txt[KPROPS_TXT_SHD], 2);
    }
    else
    {
        XtManageChildren  (rec->r[which].txt[KPROPS_TXT_SHD], 2);
        XtSetSensitive    (rec->r[which].txt[KPROPS_TXT_INP][0], False);
        XtSetSensitive    (rec->r[which].txt[KPROPS_TXT_INP][1], False);
    }
}

static void SetInputValue(dlgrec_t *rec, Widget w, double v)
{
  char      buf[400];

    XtVaSetValues(w,
                  XmNvalue,
                  snprintf_dbl_trim(buf, sizeof(buf), rec->k->u.k.dpyfmt, v, 1),
                  NULL);
}
static void RangeDpyShowValues(dlgrec_t *rec, int which)
{
    SetInputValue(rec, rec->r[which].txt[KPROPS_TXT_INP][0], rec->mins[which]);
    SetInputValue(rec, rec->r[which].txt[KPROPS_TXT_INP][1], rec->maxs[which]);
}

//////////////////////////////////////////////////////////////////////

static void CreateKnobPropsDialog(XhWindow  win, DataSubsys subsys, dlgrec_t *rec)
{
  char        buf[100];
  const char *sysname = subsys->sysname;

  Widget      w;
  XmString    s;
  
  CxWidget    grid;
  int         row;

  CxWidget    fc;
  int         shift;

  int         n;

  CxWidget    pfrm;

    snprintf(buf, sizeof(buf),
             "%s: Knob properties", sysname != NULL? sysname : "???SYSNAME???");
    rec->box =
        CNCRTZE(XhCreateStdDlg(win, "knobProps", buf, NULL, "Knob properties",
                               XhStdDlgFOk    | XhStdDlgFCancel |
                               XhStdDlgFModal*0 | XhStdDlgFNoAutoUnmng));
    XtAddCallback(XtParent(rec->box), XmNpopdownCallback, PropsCancelCB, (XtPointer)rec);
    XtAddCallback(rec->box, XmNokCallback,     PropsOkCB,     (XtPointer)rec);
    XtAddCallback(rec->box, XmNcancelCallback, PropsCancelCB, (XtPointer)rec);

    grid = XhCreateGrid(ABSTRZE(rec->box), "grid");
    XhGridSetGrid   (grid, 0, 0);
    XhGridSetSpacing(grid, 0, 0);
    XhGridSetPadding(grid, 0, 0);

    row = 0;

    /* Ident */
    MakeRowLabel(grid, row, "Name");
    rec->nam_lbl = MakePropLabel(grid, row);
    row++;
    
    /* Label */
    MakeRowLabel(grid, row, "Label");
    rec->lbl_lbl = MakePropLabel(grid, row);
    row++;
    
    /* Value */
    MakeRowLabel(grid, row, "Value");
    rec->val_dpy = MakePropText(grid, row);
    row++;
    
    /* Raw */
    MakeRowLabel(grid, row, "Raw");
    rec->raw_dpy = MakePropText(grid, row);
    row++;
    
    /* Age */
    MakeRowLabel(grid, row, "Age (ms)");
    rec->age_dpy = MakePropText(grid, row);
    row++;

    /* Flags */
    MakeRowLabel(grid, row, "Flags");
    fc = XhCreateGrid(grid, "rflags_grid");
    XhGridSetChildPosition (fc,  1,  row);
    XhGridSetChildFilling  (fc,  0,  0);
    XhGridSetChildAlignment(fc, -1,  0);
    XhGridSetGrid   (fc, 0, 0);
    XhGridSetPadding(fc, 0, 0);
    XtVaSetValues(CNCRTZE(fc), XmNmarginHeight, 0, NULL);
    for (shift = NUM_PROP_FLAGS - 1;  shift >= 0;  shift--)
    {
        sprintf(buf, "%02d", shift);
        w = rec->rflags_leds[shift] =
            XtVaCreateManagedWidget("rflagLed", xmLabelWidgetClass, CNCRTZE(fc),
                                    XmNlabelString,  s = XmStringCreateLtoR(buf, xh_charset),
                                    XmNtraversalOn,  False,
                                    XmNbackground,   XhGetColor(UNLIT_RFL_COLOR),
                                    NULL);
        XmStringFree(s);

        XhGridSetChildPosition (ABSTRZE(w), NUM_PROP_FLAGS - 1 - shift,   0);
        XhGridSetChildFilling  (ABSTRZE(w), 0,   0);
        XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

        XhSetBalloon(ABSTRZE(w), cx_strrflag_long(shift));
    }
    XhGridSetSize(fc, NUM_PROP_FLAGS, 1);
    row++;

    /* State */
    MakeRowLabel(grid, row, "State");
    rec->col_dpy = MakePropText(grid, row);
    XtVaGetValues(rec->col_dpy,
                  XmNforeground, &(rec->col_deffg),
                  XmNbackground, &(rec->col_defbg),
                  NULL);
    row++;

    /* Data references */
    MakeRowLabel(grid, row, "Source");
    rec->src_dpy = MakePropText(grid, row);
    row++;
    MakeRowLabel(grid, row, "Write");
    rec->wrt_dpy = MakePropText(grid, row);
    row++;
    MakeRowLabel(grid, row, "Colrz");
    rec->clz_dpy = MakePropText(grid, row);
    row++;

    /* Ranges */
    for (n = 0;  n < 4;  n++)
    {
        MakeRowLabel(grid, row, range_names[n]);
        RangeDpyCreate(rec, n, grid, row);
        row++;
    }

    /* Step */
    MakeRowLabel(grid, row, "Step");
    rec->stp_inp = MakePropInput(grid, row);
    row++;
    
    /* GrpCoeff */
    MakeRowLabel(grid, row, "Group coeff.");
    rec->gcf_inp = MakePropInput(grid, row);
    row++;
    
    /* Parameters */
    MakeRowLabel(grid, row, "Parameters");
    pfrm =
        ABSTRZE(XtVaCreateManagedWidget("paramsForm", xmFormWidgetClass, CNCRTZE(grid),
                                        XmNshadowThickness, 0,
                                        NULL));
    XhGridSetChildPosition (pfrm,  1,  row);
    XhGridSetChildFilling  (pfrm,  0,  0);
    XhGridSetChildAlignment(pfrm, -1,  0);

    rec->prm_none =
        XtVaCreateManagedWidget("text_o", xmTextWidgetClass, CNCRTZE(pfrm),
                                XmNscrollHorizontal,      False,
                                XmNcursorPositionVisible, False,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                XmNcolumns,               strlen(N_A_S),
                                XmNvalue,                 N_A_S,
                                NULL);

    rec->prm_grid = XhCreateGrid(pfrm, "params_grid");
    XhGridSetGrid   (rec->prm_grid, 0, 0);
    XhGridSetSpacing(rec->prm_grid, 0, 0);
    XhGridSetPadding(rec->prm_grid, 0, 0);
    MakeGridLabel(rec->prm_grid, 0, 0, "Ident");
    MakeGridLabel(rec->prm_grid, 1, 0, "Label");
    MakeGridLabel(rec->prm_grid, 2, 0, "Value");
    for (n = 0;  n < MAX_DPY_PARAMS;  n++)
    {
        rec->prm_idn[n] = w =
            XtVaCreateWidget("text_o", xmTextWidgetClass, rec->prm_grid,
                             XmNscrollHorizontal,      False,
                             XmNcursorPositionVisible, False,
                             XmNeditable,              False,
                             XmNtraversalOn,           False,
                             XmNcolumns,               10,
                             NULL);
        XhGridSetChildPosition (ABSTRZE(w),  0,  n + 1);
        XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
        XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

        rec->prm_lbl[n] = w =
            XtVaCreateWidget("text_o", xmTextWidgetClass, rec->prm_grid,
                             XmNscrollHorizontal,      False,
                             XmNcursorPositionVisible, False,
                             XmNeditable,              False,
                             XmNtraversalOn,           False,
                             XmNcolumns,               16,
                             NULL);
        XhGridSetChildPosition (ABSTRZE(w),  1,  n + 1);
        XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
        XhGridSetChildAlignment(ABSTRZE(w), -1, -1);

        rec->prm_inp[n] = w =
            XtVaCreateWidget("text_i", xmTextWidgetClass, rec->prm_grid,
                             XmNscrollHorizontal,      False,
                             XmNcursorPositionVisible, False,
                             XmNeditable,              True,
                             XmNtraversalOn,           True,
                             XmNcolumns,               20/*!!! this would be changed later*/,
                             NULL);
        XhGridSetChildPosition (ABSTRZE(w),  1,  n + 1);
        XhGridSetChildFilling  (ABSTRZE(w),  0,  0);
        XhGridSetChildAlignment(ABSTRZE(w), -1, -1);
        MotifKnobs_SetTextCursorCallback(w);

    }

    row++;
    
    XhGridSetSize(grid, 3, row);
    
    rec->initialized = 1;
}

static inline const char *NOTEMPTY(const char *s)
{
    return s != NULL  &&  *s != '\0'? s : " ";
}

static void SetTargetKnob(dlgrec_t *rec, DataKnob k)
{
  XmString  s;
  int       nc;
    
    rec->k = k;
    rec->knobstate_shown = KNOBSTATE_UNINITIALIZED;
    
    XtVaSetValues(rec->nam_lbl, XmNlabelString, s = XmStringCreateLtoR(NOTEMPTY(k->ident), xh_charset), NULL);
    XmStringFree(s);

    XtVaSetValues(rec->lbl_lbl, XmNlabelString, s = XmStringCreateLtoR(NOTEMPTY(k->label), xh_charset), NULL);
    XmStringFree(s);

    nc = GetTextColumns(k->u.k.dpyfmt, NULL);
    if (cda_ref_is_sensible(k->u.k.cl_ref)) nc = nc * 2 + strlen(VAL_COL_COMMA);
    XtVaSetValues(rec->val_dpy, XmNcolumns, nc, NULL);

    XtVaSetValues(rec->raw_dpy, XmNcolumns, 10/*4294967296*/, NULL);

    XtVaSetValues(rec->age_dpy, XmNcolumns, 10/*4294967296*/, NULL);
}

static void ShowOneRef(CxDataRef_t ref, const char *src, Widget ref_dpy)
{
  const char *true_src;

    if (src != NULL)
    {
        if (cda_src_of_ref(ref, &true_src) == +1)
            XmTextSetString(ref_dpy, true_src);
        else
            XmTextSetString(ref_dpy, N_A_S);
    }
    else
        XmTextSetString(ref_dpy, "-");
}

static void ShowOneRange(dlgrec_t *rec, int n, int do_copy)
{
  DataKnob  k = rec->k;
  int       base = DATAKNOB_PARAM_ALWD_MIN + n * 2;
  
    if (k->u.k.num_params > base + 1  /* Is this range present? */
        &&
        /* And it isn't an "input" range in ro-knob */
        (k->u.k.is_rw  ||  base != DATAKNOB_PARAM_ALWD_MIN))
    {
        if (do_copy)
        {
            rec->mins[n] = k->u.k.params[base+0].value;
            rec->maxs[n] = k->u.k.params[base+1].value;
        }
        RangeDpyShowValues(rec, n);
        RangeDpySetState  (rec, n,
                           (k->u.k.behaviour & DATAKNOB_B_RANGES_FXD) == 0);
    }
    else
    {
        RangeDpySetState  (rec, n, 0);
    }
}

static void ShowOneParam(DataKnob k, int pn, int avl, const char *dpyfmt, Widget prm_inp)
{
  char      buf[400]; // So that 1e308 fits
  
    if (avl)
        snprintf(buf, sizeof(buf), dpyfmt, k->u.k.params[pn].value);
    else
        strcpy  (buf, N_A_S);
    
    XtVaSetValues  (prm_inp, XmNcolumns, GetTextColumns(dpyfmt, NULL), NULL);
    XtSetSensitive (prm_inp, avl);
    XmTextSetString(prm_inp, buf);
}

static inline int stp_is_sensible(DataKnob k)
{
    return
        k->u.k.num_params > DATAKNOB_PARAM_STEP        &&
        (k->u.k.behaviour & DATAKNOB_B_STEP_FXD) == 0  &&
        k->u.k.is_rw;
}

static inline int gcf_is_sensible(DataKnob k)
{
    return
        k->u.k.num_params > DATAKNOB_PARAM_GRPCOEFF        &&
        (k->u.k.behaviour & DATAKNOB_B_IS_GROUPABLE) != 0  &&
        k->u.k.is_rw;
}

static void ShowCurProps (dlgrec_t *rec)
{
  DataKnob  k = rec->k;
  int       n;
  int       count;
  char     *p;

    ShowOneRef(k->u.k.rd_ref, k->u.k.rd_src, rec->src_dpy);
    ShowOneRef(k->u.k.wr_ref, k->u.k.wr_src, rec->wrt_dpy);
    ShowOneRef(k->u.k.cl_ref, k->u.k.cl_src, rec->clz_dpy);

    for (n = 0;  n < 4;  n++) ShowOneRange(rec, n, 1);
    rec->is_closing = 0;

    ShowOneParam(k, DATAKNOB_PARAM_STEP,     stp_is_sensible(k),
                 k->u.k.dpyfmt, rec->stp_inp);
    ShowOneParam(k, DATAKNOB_PARAM_GRPCOEFF, gcf_is_sensible(k),
                 "%8.3f",       rec->gcf_inp);
    
    if (k->u.k.num_params <= DATAKNOB_NUM_STD_PARAMS)
    {
        XtChangeManagedSet(&(rec->prm_grid), 1,
                           NULL, NULL,
                           &(rec->prm_none), 1);
    }
    else
    {
        XtChangeManagedSet(&(rec->prm_none), 1,
                           NULL, NULL,
                           &(rec->prm_grid), 1);

        count = k->u.k.num_params - DATAKNOB_NUM_STD_PARAMS;

        if (count > 0)
        {
            XtManageChildren(rec->prm_idn, count);
            XtManageChildren(rec->prm_lbl, count);
            XtManageChildren(rec->prm_inp, count);
        }

        if (count < MAX_DPY_PARAMS)
        {
            XtUnmanageChildren(rec->prm_idn + count, MAX_DPY_PARAMS - count);
            XtUnmanageChildren(rec->prm_lbl + count, MAX_DPY_PARAMS - count);
            XtUnmanageChildren(rec->prm_inp + count, MAX_DPY_PARAMS - count);
        }
        
        for (n = 0;  n < count; n++)
        {
            XhGridSetChildPosition(rec->prm_idn[n], 0, n + 1);
            XhGridSetChildPosition(rec->prm_lbl[n], 1, n + 1);
            XhGridSetChildPosition(rec->prm_inp[n], 2, n + 1);

            fprintf(stderr, "%s[%d]:'%s':'%s'\n", k->ident, n,
                    k->u.k.params[DATAKNOB_NUM_STD_PARAMS + n].ident, k->u.k.params[DATAKNOB_NUM_STD_PARAMS + n].label);
            p = k->u.k.params[DATAKNOB_NUM_STD_PARAMS + n].ident;
            XmTextSetString(rec->prm_idn[n], p != NULL? p : "");
            p = k->u.k.params[DATAKNOB_NUM_STD_PARAMS + n].label;
            XmTextSetString(rec->prm_lbl[n], p != NULL? p : "");

            ShowOneParam(k, DATAKNOB_NUM_STD_PARAMS + n, 1, "%8.3f", rec->prm_inp[n]);
        }

        //raise(11);
        XhGridSetSize(rec->prm_grid, 3, count + 1);
    }
}

static void ShowCurVals  (dlgrec_t *rec)
{
  DataKnob  k = rec->k;
  char      buf[400]; // So that 1e308 fits
  int       frsize;

  int       shift;
  int       col;

  Pixel     fg;
  Pixel     bg;

    snprintf_dbl_trim(buf, sizeof(buf), k->u.k.dpyfmt, k->u.k.curv, 0);
    frsize = sizeof(buf)-1 - strlen(buf) - strlen(VAL_COL_COMMA);
    if (frsize > 0  &&  cda_ref_is_sensible(k->u.k.cl_ref))
    {
        strcat(buf, VAL_COL_COMMA); // Already accounted in above calc
        snprintf(buf + strlen(buf), frsize,
                 k->u.k.dpyfmt, k->u.k.curcolv);
    }
    XmTextSetString(rec->val_dpy, buf);

#if 1
    switch (k->u.k.curv_raw_dtype)
    {
        case CXDTYPE_INT32:  sprintf(buf,   "%d", k->u.k.curv_raw.i32); break;
        case CXDTYPE_UINT32: sprintf(buf,   "%u", k->u.k.curv_raw.u32); break;
        case CXDTYPE_INT16:  sprintf(buf,   "%d", k->u.k.curv_raw.i16); break;
        case CXDTYPE_UINT16: sprintf(buf,   "%u", k->u.k.curv_raw.u16); break;
        case CXDTYPE_DOUBLE: sprintf(buf,   "%f", k->u.k.curv_raw.f64); break;
        case CXDTYPE_SINGLE: sprintf(buf,   "%f", k->u.k.curv_raw.f32); break;
        case CXDTYPE_INT64:  sprintf(buf, "%lld", k->u.k.curv_raw.i64); break;
        case CXDTYPE_UINT64: sprintf(buf, "%llu", k->u.k.curv_raw.u64); break;
        case CXDTYPE_INT8:   sprintf(buf,   "%d", k->u.k.curv_raw.i8);  break;
        case CXDTYPE_UINT8:  sprintf(buf,   "%u", k->u.k.curv_raw.u8);  break;

        case CXDTYPE_TEXT:   sprintf(buf, "\\x%02x", k->u.k.curv_raw.c8);  break;
        case CXDTYPE_UCTEXT:
            if ((k->u.k.curv_raw.c32 & 0xFFFF) == k->u.k.curv_raw.c32)
                sprintf(buf, "U+%02x%02x",
                        (k->u.k.curv_raw.c32 >>  8) & 0xFF,
                        (k->u.k.curv_raw.c32      ) & 0xFF);
            else
                sprintf(buf, "U+%02x%02x%02x%02x",
                        (k->u.k.curv_raw.c32 >> 24) & 0xFF,
                        (k->u.k.curv_raw.c32 >> 16) & 0xFF,
                        (k->u.k.curv_raw.c32 >>  8) & 0xFF,
                        (k->u.k.curv_raw.c32      ) & 0xFF);
            break;
        
        default: sprintf(buf, N_A_S);
    }
#else
    if (0/*k->u.k.curv_raw_useful*/)
        /*sprintf(buf, "%d", k->u.k.curv_raw)*/;
    else
        sprintf(buf, N_A_S);
#endif
    XmTextSetString(rec->raw_dpy, buf);

    sprintf(buf, "%d", k->u.k.age_ms);
    XmTextSetString(rec->age_dpy, buf);

    if ((
         (rec->rflags_shown ^ k->currflags) & ((1 << NUM_PROP_FLAGS) - 1)
        ) != 0)
    {
        for (shift = NUM_PROP_FLAGS - 1;  shift >= 0;  shift--)
            if ((
                 (rec->rflags_shown ^ k->currflags) & (1 << shift)
                ) != 0)
            {
                col = (k->currflags & (1 << shift)) == 0?
                    UNLIT_RFL_COLOR
                    :
                    (CXCF_FLAG_HWERR_MASK & (1 << shift)) ? XH_COLOR_BG_HWERR
                                                          : XH_COLOR_BG_SFERR;
                    XtVaSetValues(rec->rflags_leds[shift],
                                  XmNbackground, XhGetColor(col),
                                  NULL);
            }
        rec->rflags_shown = k->currflags;
    }

    if (k->u.k.curstate != rec->knobstate_shown)
    {
        MotifKnobs_ChooseColors(000, k->u.k.curstate,
                                rec->col_deffg, rec->col_defbg,
                                &fg,            &bg);
        XtVaSetValues(rec->col_dpy,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      XmNvalue,      strknobstate_short(k->u.k.curstate),
                      NULL);
        rec->knobstate_shown = k->u.k.curstate;
    }
}

void _ChlShowProps_m(DataKnob k)
{
  XhWindow    win = XhWindowOf(k->w);
  dlgrec_t   *rec = ThisDialog(win);

    SetTargetKnob(rec, k);
    ShowCurProps (rec);
    ShowCurVals  (rec);
    /* Note: we must "bzero" AFTER setting values in XmText widgets, since that setting triggers modifyVeryfiCallback */
    bzero        (&(rec->chg), sizeof(rec->chg));
    XhStdDlgShow (rec->box);
}
