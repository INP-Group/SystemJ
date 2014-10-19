#include "Chl_includes.h"

#include "KnobsI.h"


//// multicolopts stuff //////////////////////////////////////////////

psp_paramdescr_t text2_Chl_multicolopts[] =
{
    PSP_P_INCLUDE("container",
                  text2_Chl_containeropts,
                  PSP_OFFSET_OF(_Chl_multicolopts_t, cont)),

    PSP_P_FLAG  ("nocoltitles", _Chl_multicolopts_t, nocoltitles, 1, 0),
    PSP_P_FLAG  ("norowtitles", _Chl_multicolopts_t, norowtitles, 1, 0),
    PSP_P_FLAG  ("transposed",  _Chl_multicolopts_t, transposed,  1, 0),
    PSP_P_INT   ("hspc",        _Chl_multicolopts_t, hspc,        -1, 0, 0),
    PSP_P_INT   ("vspc",        _Chl_multicolopts_t, vspc,        -1, 0, 0),
    PSP_P_INT   ("colspc",      _Chl_multicolopts_t, colspc,      -1, 0, 0),
    PSP_P_FLAG  ("one",         _Chl_multicolopts_t, one,         1, 0),

    PSP_P_END(),
};

//// Knobs' position management //////////////////////////////////////

typedef struct
{
    int  horz;
    int  vert;
} placeopts_t;

static psp_lkp_t horz_place_lkp[] =
{
    {"default", KNOB_B_HALIGN_DEFAULT},
    {"left",    KNOB_B_HALIGN_LEFT},
    {"right",   KNOB_B_HALIGN_RIGHT},
    {"center",  KNOB_B_HALIGN_CENTER},
    {"fill",    KNOB_B_HALIGN_FILL},
    {NULL, 0}
};

static psp_lkp_t vert_place_lkp[] =
{
    {"default", KNOB_B_VALIGN_DEFAULT},
    {"top",     KNOB_B_VALIGN_TOP},
    {"bottom",  KNOB_B_VALIGN_BOTTOM},
    {"center",  KNOB_B_VALIGN_CENTER},
    {"fill",    KNOB_B_VALIGN_FILL},
    {NULL, 0}
};

static psp_paramdescr_t text2placeopts[] =
{
    PSP_P_LOOKUP("horz", placeopts_t, horz, KNOB_B_HALIGN_DEFAULT, horz_place_lkp),
    PSP_P_LOOKUP("vert", placeopts_t, vert, KNOB_B_VALIGN_DEFAULT, vert_place_lkp),
    PSP_P_END()
};

static void PlaceKnob(knobinfo_t *ki, int x, int y)
{
  placeopts_t   placeopts;
  int           hplace;
  int           vplace;
  int           halign;
  int           valign;
  int           hfill;
  int           vfill;

    hplace = ki->behaviour & KNOB_B_HALIGN_MASK;
    vplace = ki->behaviour & KNOB_B_VALIGN_MASK;
  
    if (ki->placement != NULL  &&  ki->placement[0] != '\0')
    {
        if (psp_parse(ki->placement, NULL,
                      &placeopts,
                      Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                      text2placeopts) != PSP_R_OK)
        {
            fprintf(stderr, "Knob %s/\"%s\".placement: %s\n",
                    ki->ident, ki->label, psp_error());
            bzero(&placeopts, sizeof(placeopts));
        }
        
        if (placeopts.horz != 0) hplace = placeopts.horz;
        if (placeopts.vert != 0) vplace = placeopts.vert;
    }
    
    switch (hplace)
    {
        case KNOB_B_HALIGN_LEFT:    halign = -1; hfill = 0; break;
        case KNOB_B_HALIGN_RIGHT:   halign = +1; hfill = 0; break;
        case KNOB_B_HALIGN_CENTER:  halign =  0; hfill = 0; break;
        case KNOB_B_HALIGN_FILL:    halign =  0; hfill = 1; break;
        default:                    halign = -1; hfill = 0; break;
    }
    switch (vplace)
    {
        case KNOB_B_VALIGN_TOP:     valign = -1; vfill = 0; break;
        case KNOB_B_VALIGN_BOTTOM:  valign = +1; vfill = 0; break;
        case KNOB_B_VALIGN_CENTER:  valign =  0; vfill = 0; break;
        case KNOB_B_VALIGN_FILL:    valign =  0; vfill = 1; break;
        default:                    valign = -1; vfill = 0; break;
    }

    XhGridSetChildPosition (ki->indicator, x,      y);
    XhGridSetChildAlignment(ki->indicator, halign, valign);
    XhGridSetChildFilling  (ki->indicator, hfill,  vfill);
}

static void sethv(int *h, int *v, int x, int y, int transposed)
{
    if (!transposed)
    {
        *h = x;
        *v = y;
    }
    else
    {
        *h = y;
        *v = x;
    }
}


//// Labels' management //////////////////////////////////////////////

static void LabelClickHandler(Widget     w,
                              XtPointer  closure,
                              XEvent    *event,
                              Boolean   *continue_to_dispatch)
{
  eleminfo_t   *e   = (eleminfo_t *)  closure;
  XtPointer     user_data;
  int           col;
  int           my_x;
  int           my_y;
  WidgetList    children;
  Cardinal      numChildren;
  Cardinal      i;
  int           count;
  int           its_x;
  int           its_y;
  Widget        victims[200]; /* 200 seems to be a fair limit; in unlimited case we'll need to XtMalloc() */
  
  char          lbuf[1000];
  char          tbuf[1000];
  char         *caption;
  char         *tip;
  XmString      s;
  
    if (!CheckForDoubleClick(w, event, continue_to_dispatch)) return;
  
///    return;/*!!!*/

    XtVaGetValues(w, XmNuserData, &user_data, NULL);
    col = ptr2lint(user_data);
    
    XhGridGetChildPosition(ABSTRZE(w), &my_x, &my_y);
    XtVaGetValues(XtParent(w),
                  XmNchildren,    &children,
                  XmNnumChildren, &numChildren,
                  NULL);

    for (i = 0,               count = 0;
         i < numChildren  &&  count < countof(victims);
         i++)
    {
        XhGridGetChildPosition(ABSTRZE(children[i]), &its_x, &its_y);
        if (children[i] != w  &&  its_x == my_x)
            victims[count++] = children[i];
    }

    if (count == 0) return; /*!?What can it be?!*/

    if (XtIsManaged(victims[0]))
    {
        XtUnmanageChildren(victims, count);
        caption = "!";
    }
    else
    {
        XtManageChildren(victims, count);
        if (col >= 0)
            get_knob_rowcol_label_and_tip(e->colnames, col, e->content + col,
                                          lbuf, sizeof(lbuf), tbuf, sizeof(tbuf),
                                          &caption, &tip);
        else
            caption="???";
    }

    XtVaSetValues(w, XmNlabelString, s = XmStringCreateLtoR(caption, xh_charset), NULL);
    XmStringFree(s);
}

/*
 *  make_label
 *      Creates a column- or row-label,
 *      + appropriately assigning it a label-string and a tooltip
 *        (both either from the element description or from an
 *        appropriate source-knob);
 *      + positions and aligns the label in a parent_grid as requested;
 *      + optionally adds some space to the label's left margin;
 *      + optionally colorizes the label based on source-knob's color;
 *      + optionally adds a double-click callback
 *
 *  Args:
 *      name         resource name for the widget
 *      parent_grid  parent grid widget
 *      ms           multistring which (probably) contains the label-string
 *      n            index in the above multistring
 *      sk           source-knob (source of label-string, tip, and color)
 *      clone_color  whether to colorize the label as source-knob
 *      loffset      space to add to marginLeft
 *      x,y          position in parent_grid
 *      h_a,v_a      horizontal and vertical alignment (-1/0/+1)
 *      e            element reference
 *      col          column to set
 */

static void make_label(const char *name, XhWidget  parent_grid,
                       const char *ms,   int       n,
                       knobinfo_t *sk,   int       clone_color,
                       int         loffset,
                       int         x,    int       y,
                       int         h_a,  int       v_a,
                       eleminfo_t *e,    int       col)
{
  XhWidget      l;
  XmString      s;
  char          lbuf[1000];
  char          tbuf[1000];
  char         *caption;
  char         *tip;
  Pixel         fg;
  Pixel         bg;
  Dimension     lmargin;

    get_knob_rowcol_label_and_tip(ms, n, sk,
                                  lbuf, sizeof(lbuf), tbuf, sizeof(tbuf),
                                  &caption, &tip);

    if (caption != NULL  &&  *caption == KNOB_LABELTIP_NOLABELTIP_PFX) caption++;
    
    l = ABSTRZE(XtVaCreateManagedWidget(name, xmLabelWidgetClass, CNCRTZE(parent_grid),
                                        XmNlabelString, s = XmStringCreateLtoR(caption, xh_charset),
                                        NULL));
    XmStringFree(s);
    
    if (tip != NULL  &&  tip[0] != '\0')
    {
        if (*tip == KNOB_LABELTIP_NOLABELTIP_PFX) tip++;
        XhSetBalloon(l, tip);
    }

    if (loffset > 0)
    {
        XtVaGetValues(CNCRTZE(l), XmNmarginLeft, &lmargin, NULL);
        lmargin += loffset;
        XtVaSetValues(CNCRTZE(l), XmNmarginLeft, lmargin,  NULL);
    }
    
    XhGridSetChildPosition (l, x,   y);
    XhGridSetChildFilling  (l, 0,   0);
    XhGridSetChildAlignment(l, h_a, v_a);
    
    if (clone_color  &&  sk->color != LOGC_VIC /*!!! A dirty hack!!!*/)
    {
        XtVaGetValues(CNCRTZE(l),
                      XmNforeground, &fg,
                      XmNbackground, &bg,
                      NULL);
        ChooseKnobColors(sk->color, COLALARM_NONE, fg, bg, &fg, &bg);
        XtVaSetValues(CNCRTZE(l),
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }

    if (clone_color               &&  /*!!! Or should rename "clone_color_ to "clone_from_knob"? */
        sk->type != LOGT_SUBELEM  &&  /*!!! } Also a "hack" -- copy of conditions */
        sk->kind != LOGK_NOP)         /*!!! } from CdrCvtLogchannets2Knobs() */
        XtInsertEventHandler(CNCRTZE(l), ButtonPressMask, False, KnobsMouse3Handler, (XtPointer)sk, XtListHead);
    
    if (e != NULL)
    {
        XtAddEventHandler(CNCRTZE(l), ButtonPressMask, False,
                          LabelClickHandler, (XtPointer)e);
        XtVaSetValues(CNCRTZE(l), XmNuserData, lint2ptr(col), NULL);
    }
}


//////////////////////////////////////////////////////////////////////

static knobs_emethods_t multicol_emethods =
{
    Chl_E_SetPhysValue_m,
    Chl_E_ShowAlarm_m,
    Chl_E_AckAlarm_m,
    Chl_E_ShowPropsWindow,
    Chl_E_ShowBigValWindow,
    Chl_E_NewData_m,
    Chl_E_ToHistPlot
};

static int ChlGridMaker(ElemInfo e, void *privptr)
{
  XhWidget             grid;
  _Chl_multicolopts_t *opts_p = (_Chl_multicolopts_t *)privptr;
    
    grid = *(KnobsGetElemInnageP(e)) =
      XhCreateGrid(KnobsGetElemContainer(e), "grid");
    XhGridSetGrid   (grid, -1*0,         -1*0);
    XhGridSetSpacing(grid, opts_p->hspc, opts_p->vspc);
    XhGridSetPadding(grid, 1*0,          1*0);
    
    return 0;
}

static int ChlFormMaker(ElemInfo e, void *privptr)
{
  XhWidget             form;
  _Chl_multicolopts_t *opts_p = (_Chl_multicolopts_t *)privptr;
    
    form = *(KnobsGetElemInnageP(e)) =
        ABSTRZE(XtVaCreateManagedWidget("form",
                                        xmFormWidgetClass,
                                        CNCRTZE(KnobsGetElemContainer(e)),
                                        XmNshadowThickness, 0,
                                        NULL));

    return 0;
}

int ELEM_MULTICOL_Create_m   (XhWidget parent, ElemInfo e)
{
  _Chl_multicolopts_t  opts;

    bzero(&opts, sizeof(opts)); // Is required, since none of the flags have defaults specified
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2_Chl_multicolopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
        opts.hspc = opts.vspc = opts.colspc = -1;
    }

    return ELEM_MULTICOL_Create_m_as(parent, e, &opts);
}

int ELEM_MULTICOL_Create_m_as(XhWidget parent, ElemInfo e,
                              _Chl_multicolopts_t *opts_p)
{
  knobinfo_t          *kl;
  int                  ncols;
  int                  nflrs;
  int                  nattl;
  int                  ngrid;
  int                  nrows;
  int                  nlanes;
    
  knobinfo_t          *ki;
  
  int                  x;
  int                  y;
  
  int                  x0;
  int                  y0;

  int                  gx;
  int                  gy;
  
  int                  eh;  // Effective Horz.
  int                  ev;  // Effective Vert.
  const char          *collabel_name;
  int                  collabel_halign;
  int                  collabel_valign;
  const char          *rowlabel_name;
  int                  rowlabel_halign;
  int                  rowlabel_valign;
  int                  rowlabel_ofs;

    /* Zeroth, obtain parameters */
    ncols = e->ncols & 0x0000FFFF;           if (ncols <= 0) ncols = 1;
    nflrs = (e->ncols >> 16) & 0x000000FF;
    nattl = (e->ncols >> 24) & 0x000000FF;   if (opts_p->cont.notitle) nattl = 0;
                                             if (nattl > e->count)     nattl = e->count;
    ngrid = e->count - nattl;
    nrows = (ngrid + ncols - 1) / ncols;
    kl    = e->content + nattl;
  
    /* First, sanitize options */
    if (opts_p->hspc   < 0) opts_p->hspc   = CHL_INTERKNOB_H_SPACING;
    if (opts_p->vspc   < 0) opts_p->vspc   = CHL_INTERKNOB_V_SPACING;
    if (opts_p->colspc < 0) opts_p->colspc = CHL_INTERELEM_H_SPACING;
    if (ngrid != 1)         opts_p->one    = 0;

    /* Second, simulate ELEM_SINGLECOL with appropriate MULTICOL parameters */
    if (e->type == ELEM_SINGLECOL)
    {
        ncols = 1;
        nrows = ngrid;
        opts_p->nocoltitles = 1;
    }

    /* Third, what's with "wrap at row"/"number of floors"? */
    if (nflrs <= 0  ||  nflrs > nrows) nflrs = nrows;
    if (nflrs == 0) nflrs = 1;
    nlanes = nrows / nflrs;
  
    /* Give birth to element */
    ChlCreateContainer(parent, e,
                       (opts_p->cont.noshadow? CHL_CF_NOSHADOW : 0) |
                       (opts_p->cont.notitle?  CHL_CF_NOTITLE  : 0) |
                       (opts_p->cont.nohline?  CHL_CF_NOHLINE  : 0) |
                       (opts_p->cont.nofold?   CHL_CF_NOFOLD   : 0) |
                       (opts_p->cont.folded?   CHL_CF_FOLDED   : 0) |
                       (opts_p->cont.fold_h?   CHL_CF_FOLD_H   : 0) |
                       (opts_p->cont.sepfold?  CHL_CF_SEPFOLD  : 0) |
                       (nattl != 0          ?  CHL_CF_ATTITLE  : 0) |
                       (opts_p->cont.title_side << CHL_CF_TITLE_SIDE_shift),
                       opts_p->one? ChlFormMaker : ChlGridMaker, opts_p);
    e->emlink = &multicol_emethods;

    ChlPopulateContainerAttitle(e, nattl, opts_p->cont.title_side);

    /* Populate the element */
    x0 = opts_p->norowtitles? 0 : 1;
    y0 = opts_p->nocoltitles? 0 : 1;
    
    sethv(&eh, &ev, nlanes * (x0 + ncols), nflrs + y0, opts_p->transposed);
    XhGridSetSize(e->innage, eh, ev);

    sethv(&collabel_halign, &rowlabel_halign,  0, -1, opts_p->transposed);
    sethv(&collabel_valign, &rowlabel_valign, +1, -1, opts_p->transposed);
    if (!opts_p->transposed)
    {
        collabel_name   = "collabel";
        rowlabel_name   = "rowlabel";
        rowlabel_ofs    = opts_p->colspc;
    }
    else
    {
        collabel_name   = "rowlabel";
        rowlabel_name   = "collabel";
        rowlabel_ofs    = 0;
    }

    if (opts_p->one)
    {
        if (ChlGiveBirthToKnob(kl) == 0)
        {
            attachleft  (kl->indicator, NULL, 0);
            attachright (kl->indicator, NULL, 0);
            attachtop   (kl->indicator, NULL, 0);
            attachbottom(kl->indicator, NULL, 0);
        }

        return 0;
    }
    
    for (y = 0, ki = kl;  y < nrows;  y++)
    {
        for (x = 0;  x < ncols  &&  y*ncols+x < ngrid;  x++, ki++)
        {
            gx = x0 + x + (y / nflrs) * (ncols + x0);
            gy = y0 + (y % nflrs);

            /* Collabel? */
            sethv(&eh, &ev, gx, gy - 1, opts_p->transposed);
            if (opts_p->nocoltitles == 0  &&  gy == y0)
                make_label(collabel_name, e->innage,
                           e->colnames, x,
                           kl + x, 0,
                           0,
                           eh, ev,
                           collabel_halign, collabel_valign,
                           !opts_p->transposed? e : NULL, x);
            
            /* Rowlabel? */
            sethv(&eh, &ev, gx - 1, gy, opts_p->transposed);
            if (opts_p->norowtitles == 0  &&  x == 0)
                make_label(rowlabel_name, e->innage,
                           e->rownames, y,
                           ki, 1,
                           y < nflrs? 0 : rowlabel_ofs,
                           eh, ev,
                           rowlabel_halign, rowlabel_valign,
                           /*opts_p->transposed?  e : */NULL, y);

            /* Knob itself */
            if (ChlGiveBirthToKnob(ki) != 0) /*!!!*/;
            sethv(&eh, &ev, gx, gy, opts_p->transposed);
            PlaceKnob(ki, eh, ev);
        }
    }

    return 0;
}
