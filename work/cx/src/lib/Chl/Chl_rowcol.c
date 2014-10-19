#include "Chl_includes.h"

#include "KnobsI.h"

#include <Xm/RowColumn.h>


//// rowcolopts stuff //////////////////////////////////////////////

typedef struct
{
    _Chl_containeropts_t cont;
    int                  vertical;
} rowcolopts_t;

static psp_paramdescr_t text2rowcolopts[] =
{
    PSP_P_INCLUDE("container",
                  text2_Chl_containeropts,
                  PSP_OFFSET_OF(rowcolopts_t, cont)),

    PSP_P_FLAG   ("horizontal", rowcolopts_t, vertical, 0, 1),
    PSP_P_FLAG   ("vertical",   rowcolopts_t, vertical, 1, 0),

    PSP_P_END(),
};


//////////////////////////////////////////////////////////////////////

static knobs_emethods_t rowcol_emethods =
{
    Chl_E_SetPhysValue_m,
    Chl_E_ShowAlarm_m,
    Chl_E_AckAlarm_m,
    Chl_E_ShowPropsWindow,
    Chl_E_ShowBigValWindow,
    Chl_E_NewData_m,
    Chl_E_ToHistPlot
};

static int ChlRowcolMaker(ElemInfo e, void *privptr)
{
  XhWidget      form;
  rowcolopts_t *opts_p = (rowcolopts_t *)privptr;

    form = *(KnobsGetElemInnageP(e)) =
        ABSTRZE(XtVaCreateManagedWidget("form",
                                        xmRowColumnWidgetClass,
                                        CNCRTZE(KnobsGetElemContainer(e)),
                                        XmNshadowThickness, 0,
                                        XmNpacking,         XmPACK_COLUMN,
                                        XmNnumColumns,      1,
                                        XmNorientation,     opts_p->vertical? XmVERTICAL
                                                                            : XmHORIZONTAL,
                                        XmNadjustLast,      True,
                                        XmNmarginWidth,     0,
                                        XmNmarginHeight,    0,
                                        NULL));

    return 0;
}

int ELEM_ROWCOL_Create_m(XhWidget parent, ElemInfo e)
{
  rowcolopts_t  opts;

  knobinfo_t           *kl;
  int                   ncols;
  int                   nours;
  int                   nattl;

  int                   n;

    bzero(&opts, sizeof(opts)); // Is required, since none of the flags have defaults specified
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2rowcolopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
    }

    ncols = e->ncols & 0x0000FFFF;           if (ncols <= 0) ncols = 1;
    nattl = (e->ncols >> 24) & 0x000000FF;   if (opts.cont.notitle) nattl = 0;
    nours = e->count - nattl;
    kl    = e->content + nattl;

    /* Give birth to element */
    ChlCreateContainer(parent, e,
                       (opts.cont.noshadow? CHL_CF_NOSHADOW : 0) |
                       (opts.cont.notitle?  CHL_CF_NOTITLE  : 0) |
                       (opts.cont.nohline?  CHL_CF_NOHLINE  : 0) |
                       (opts.cont.nofold?   CHL_CF_NOFOLD   : 0) |
                       (opts.cont.folded?   CHL_CF_FOLDED   : 0) |
                       (opts.cont.fold_h?   CHL_CF_FOLD_H   : 0) |
                       (opts.cont.sepfold?  CHL_CF_SEPFOLD  : 0) |
                       (nattl != 0       ?  CHL_CF_ATTITLE  : 0) |
                       (opts.cont.title_side << CHL_CF_TITLE_SIDE_shift),
                       ChlRowcolMaker, &opts);
    e->emlink = &rowcol_emethods;

    ChlPopulateContainerAttitle(e, nattl, opts.cont.title_side);

    /* Populate the element */
    XtVaSetValues(e->innage, XmNnumColumns, ncols, NULL);
    for (n = 0;  n < nours;  n++)
        if (ChlGiveBirthToKnob(kl + n) != 0) /*!!!*/;

    return 0;
}
