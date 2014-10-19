#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>


#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_cpanel_decor.h"

#include "nadc4_data.h"
#include "nadc4_gui.h"

#include "drv_i/nadc4_drv_i.h"


static Widget nadc4_mkctls(pzframe_gui_t           *gui,
                           fastadc_type_dscr_t     *atd,
                           Widget                   parent,
                           pzframe_gui_mkstdctl_t   mkstdctl,
                           pzframe_gui_mkparknob_t  mkparknob)
{
  fastadc_gui_t *fastadc_gui = pzframe2fastadc_gui(gui);

  Widget  cform;    // Controls form
  Widget  commons;  // 
  Widget  dwgrid;   // Device-Wide-parameters grid
  Widget  vline;    // Separator line
  Widget  pcgrid;   // Per-Channel-parameters grid
  Widget  rpgrid;   // Repers grid
  Widget  ptsform;  // NUMPTS@PTSOFS form
  Widget  icform;   // Intra-cell form
  Widget  stsubw;
  Widget  stgrdc;

  Widget  w1;
  Widget  w2;

  int     y;
  int     nl;
  int     nr;

  char    spec[1000];

    /* 1. General layout */
    /* A container form */
    cform = XtVaCreateManagedWidget("form", xmFormWidgetClass, parent,
                                    XmNshadowThickness, 0,
                                    NULL);
    /* A "commons" */
    commons = mkstdctl(gui, cform, FASTADC_GUI_CTL_COMMONS, 0, 0);
    /* A grid for device-wide parameters */
    dwgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (dwgrid, 0, 0);
    XhGridSetSpacing(dwgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(dwgrid, 0, 0);
    /* A separator line */
    vline = XtVaCreateManagedWidget("vline",
                                    xmSeparatorWidgetClass,
                                    cform,
                                    XmNorientation,     XmVERTICAL,
                                    NULL);
    /* A form for PTS@OFS */
    ptsform = XtVaCreateManagedWidget("form", xmFormWidgetClass, cform,
                                      XmNshadowThickness, 0,
                                      NULL);
    /* A commons<->ptsform "connector" */
    w1 = XtVaCreateManagedWidget("", widgetClass, cform,
                                 XmNwidth,       1,
                                 XmNheight,      1,
                                 XmNborderWidth, 0,
                                 NULL);
    attachleft (w1, commons, 0);
    attachright(w1, ptsform, 0);
    /* A grid for per-channel parameters */
    pcgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (pcgrid, 0, 0);
    XhGridSetSpacing(pcgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(pcgrid, 0, 0);
    /* A grid for repers parameters */
    rpgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (rpgrid, 0, 0);
    XhGridSetSpacing(rpgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(rpgrid, 0, 0);
    /* Perform attachment */
    attachright (rpgrid,  NULL,    0);
    attachright (ptsform, rpgrid,  CHL_INTERKNOB_H_SPACING);
    attachright (pcgrid,  rpgrid,  CHL_INTERKNOB_H_SPACING);
    attachtop   (pcgrid,  ptsform, CHL_INTERKNOB_V_SPACING);
    attachright (vline,   pcgrid,  CHL_INTERKNOB_H_SPACING);
    attachtop   (vline,   ptsform, 0);
    attachbottom(vline,   NULL,    0);
    attachright (dwgrid,  vline,   CHL_INTERKNOB_H_SPACING);
    attachtop   (dwgrid,  ptsform, CHL_INTERKNOB_V_SPACING);
    attachleft  (commons, NULL,    0);

    /* 2. Device-wide parameters */
    y = 0;

    /* Cmpr */
    MakeStdCtl(dwgrid, 1, y++, gui, mkstdctl, FASTADC_GUI_CTL_X_CMPR, 0, 0);

    /* IStart */
    MakeALabel (dwgrid, 0, y, "Int. Start");
    MakeParKnob(dwgrid, 1, y++,
                " look=greenled label='Int. Start' widdepinfo='panel'",
                gui, mkparknob, NADC4_PARAM_ISTART);
    

    /* Wait time */
    MakeALabel (dwgrid, 0, y, "Wait time");
    MakeParKnob(dwgrid, 1, y++,
                " look=text"
                " widdepinfo='withunits'"
                " units=ms minnorm=0 maxnorm=1000000000 dpyfmt=%7.0f step=1000",
                gui, mkparknob, NADC4_PARAM_WAITTIME);
    
    XhGridSetSize(dwgrid, 2, y);

    /* Stats */
    stsubw = MakeSubwin(dwgrid, 0, y,
                        "Stats...", "Statistics");
    /* Shot, Stop */
    icform = MakeAForm(dwgrid, 1, y++);
    XhGridSetChildFilling(icform, 1, 0);
    w1 = mkparknob(gui, icform,
                   " look=button label='Stop'",
                   NADC4_PARAM_STOP);
    attachright(w1, NULL, 0);
    w2 = mkparknob(gui, icform,
                   " look=button label='Shot!'",
                   NADC4_PARAM_SHOT);
    attachright(w2, w1, CHL_INTERKNOB_H_SPACING);

    /* NUMPTS@PTSOFS */
    snprintf(spec, sizeof(spec),
             "look=text label='' step=100 "
             "widdepinfo='' "
             "minnorm=%d maxnorm=%d mindisp=%d maxdisp=%d dpyfmt='%%6.0f' ",
             1, NADC4_MAX_NUMPTS, 1, NADC4_MAX_NUMPTS);
    w1 = mkparknob(gui, ptsform, spec, NADC4_PARAM_NUMPTS);
    snprintf(spec, sizeof(spec),
             "look=text label='@' step=100 "
             "widdepinfo='withlabel' "
             "minnorm=%d maxnorm=%d mindisp=%d maxdisp=%d dpyfmt='%%6.0f' ",
             0, NADC4_MAX_NUMPTS-1, 0, NADC4_MAX_NUMPTS-1);
    w2 = mkparknob(gui, ptsform, spec, NADC4_PARAM_PTSOFS);
    attachleft(w2, w1, 0);

    /* 3. Per-channel parameters and repers */
    for (nl = 0;  nl < NADC4_NUM_LINES;  nl++)
    {
        w1 = MakeStdCtl(pcgrid, 0, nl, gui, mkstdctl, FASTADC_GUI_CTL_LINE_LABEL, nl, 1);
        XhGridSetChildFilling  (w1, 1,      0);
        MakeStdCtl     (pcgrid, 1, nl, gui, mkstdctl, FASTADC_GUI_CTL_LINE_MAGN,  nl, 1);
        MakeParKnob    (pcgrid, 2, nl,
                        " look=text label='+/-' dpyfmt='%4.0f'"
                        " widdepinfo='withlabel'",
                        gui, mkparknob, NADC4_PARAM_ZERO0 + nl);

        for (nr = 0;
             nr < XH_PLOT_NUM_REPERS;
             nr++)
        {
            if (nl == 0)
            {
                icform = MakeAForm(rpgrid, nr, 0);
                w1 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_ONOFF, 0, nr);
                w2 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_TIME,  0, nr);
                attachleft(w2, w1, NULL);
            }
            MakeStdCtl(rpgrid, nr, nl + 1, gui, mkstdctl, FASTADC_GUI_CTL_REPER_VALUE, nl, nr);
        }
    }
    XhGridSetSize(pcgrid, 3,                      nl);
    XhGridSetSize(rpgrid, XH_PLOT_NUM_REPERS, nl + 1);

    /* Statistics */
    stgrdc = XhCreateGrid(stsubw, "grid");
    XhGridSetGrid   (stgrdc, 0, 0);
    XhGridSetSpacing(stgrdc, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(stgrdc, 0, 0);
    for (nl = 0;  nl < NADC4_NUM_LINES;  nl++)
    {
        snprintf(spec, sizeof(spec),
                 "%s", fastadc_gui->a.dcnv.lines[nl].descr);
        MakeALabel(stgrdc, 0, nl + 1, spec);

        if (nl == 0)
            MakeALabel(stgrdc, 1, 0, "Min");
        MakeParKnob   (stgrdc, 1, nl + 1,
                       " look=text ro",
                       gui, mkparknob, NADC4_PARAM_MIN0 + nl);

        if (nl == 0)
            MakeALabel(stgrdc, 2, 0, "Max");
        MakeParKnob   (stgrdc, 2, nl + 1,
                       " look=text ro",
                       gui, mkparknob, NADC4_PARAM_MAX0 + nl);

        if (nl == 0)
            MakeALabel(stgrdc, 3, 0, "Avg");
        MakeParKnob   (stgrdc, 3, nl + 1,
                       " look=text ro",
                       gui, mkparknob, NADC4_PARAM_AVG0 + nl);

        if (nl == 0)
            MakeALabel(stgrdc, 4, 0, "Sum");
        MakeParKnob   (stgrdc, 4, nl + 1,
                       " look=text ro",
                       gui, mkparknob, NADC4_PARAM_INT0 + nl);
    }
    MakeParKnob(stgrdc, 0, 0,
                " look=blueled label='Calc'",
                gui, mkparknob, NADC4_PARAM_CALC_STATS);
    XhGridSetSize(stgrdc, 5, nl + 1);

    return cform;
}

pzframe_gui_dscr_t *nadc4_get_gui_dscr(void)
{
  static fastadc_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        FastadcGuiFillStdDscr(&dscr,
                              pzframe2fastadc_type_dscr(nadc4_get_type_dscr()));

        dscr.def_plot_w = 700;
        dscr.min_plot_w = 100;
        dscr.max_plot_w = 3000;
        dscr.def_plot_h = 256;
        dscr.min_plot_h = 100;
        dscr.max_plot_h = 3000;

        dscr.cpanel_loc = XH_PLOT_CPANEL_AT_BOTTOM;
        dscr.mkctls     = nadc4_mkctls;

        dscr_inited = 1;
    }
    return &(dscr.gkd);
}
