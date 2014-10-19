#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>


#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_cpanel_decor.h"

#include "s2v_data.h"
#include "s2v_gui.h"

#include "drv_i/s2v_drv_i.h"


static Widget s2v_mkctls(pzframe_gui_t           *gui,
                         fastadc_type_dscr_t     *atd,
                         Widget                   parent,
                         pzframe_gui_mkstdctl_t   mkstdctl,
                         pzframe_gui_mkparknob_t  mkparknob)
{
  Widget  cform;    // Controls form
  Widget  dwform;   // Device-Wide-parameters form
  Widget  rpgrid;   // Repers grid
  Widget  stgrdc;
  Widget  icform;   // Intra-cell form
  Widget  w1;
  Widget  w2;
  Widget  left;

  int     nr;

  char    spec[1000];

    /* 1. General layout */
    /* A container form */
    cform = XtVaCreateManagedWidget("form", xmFormWidgetClass, parent,
                                    XmNshadowThickness, 0,
                                    NULL);
    /* A form for device-wide parameters */
    dwform = XtVaCreateManagedWidget("form", xmFormWidgetClass, cform,
                                     XmNshadowThickness, 0,
                                     NULL);
    /* A grid for repers parameters */
    rpgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (rpgrid, 0, 0);
    XhGridSetSpacing(rpgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(rpgrid, 0, 0);
    /* Statistics */
    stgrdc = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (stgrdc, 0, 0);
    XhGridSetSpacing(stgrdc, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(stgrdc, 0, 0);
    /* Perform attachment */
    attachtop (rpgrid, dwform, CHL_INTERKNOB_V_SPACING);
    attachtop (stgrdc, dwform, CHL_INTERKNOB_V_SPACING);
    attachleft(stgrdc, rpgrid, CHL_INTERKNOB_H_SPACING);

    /* A "commons" */
    w1 = mkstdctl(gui, dwform, FASTADC_GUI_CTL_COMMONS, 0, 0);
    attachleft  (w1, NULL,   0); left = w1;

    /* 2. Device-wide parameters */
    /* NUMPTS@PTSOFS */
    snprintf(spec, sizeof(spec),
             "look=text label='Numpoints' step=100 "
             "widdepinfo='withlabel' "
             "minnorm=%d maxnorm=%d mindisp=%d maxdisp=%d dpyfmt='%%6.0f' ",
             1, S2V_MAX_NUMPTS, 1, S2V_MAX_NUMPTS);
    w1 = mkparknob(gui, dwform, spec, S2V_PARAM_NUMPTS);
    attachleft(w1, left, CHL_INTERKNOB_H_SPACING); left = w1;
    snprintf(spec, sizeof(spec),
             "look=text label='@' step=100 "
             "widdepinfo='withlabel' "
             "minnorm=%d maxnorm=%d mindisp=%d maxdisp=%d dpyfmt='%%6.0f' ",
             0, S2V_MAX_NUMPTS-1, 0, S2V_MAX_NUMPTS-1);
    w1 = mkparknob(gui, dwform, spec, S2V_PARAM_PTSOFS);
    attachleft(w1, left, 0); left = w1;

    /* IStart, Shot, Stop */
    w1 = mkparknob(gui, dwform,
                   " look=greenled label='Int. Start' widdepinfo='panel'",
                   S2V_PARAM_ISTART);
    attachleft(w1, left, CHL_INTERKNOB_H_SPACING); left = w1;
    w1 = mkparknob(gui, dwform,
                   " look=button label='Stop'",
                   S2V_PARAM_STOP);
    attachleft(w1, left, 0); left = w1;
    w1 = mkparknob(gui, dwform,
                   " look=button label='Shot!'",
                   S2V_PARAM_SHOT);
    attachleft(w1, left, 0); left = w1;

    /* 3. Repers */
    /* Cmpr, magn */
    MakeStdCtl(rpgrid, 0, 0, gui, mkstdctl, FASTADC_GUI_CTL_X_CMPR,    0, 0);
    MakeStdCtl(rpgrid, 0, 1, gui, mkstdctl, FASTADC_GUI_CTL_LINE_MAGN, 0, 1);
    /* Repers themselves */
    for (nr = 0;
         nr < XH_PLOT_NUM_REPERS;
         nr++)
    {
        icform = MakeAForm(rpgrid, 1 + nr, 0);
        w1 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_ONOFF, 0, nr);
        w2 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_TIME,  0, nr);
        attachleft(w2, w1, NULL);
        MakeStdCtl(rpgrid, 1 + nr, 0 + 1, gui, mkstdctl, FASTADC_GUI_CTL_REPER_VALUE, 0, nr);
    }
    XhGridSetSize(rpgrid, 1 + XH_PLOT_NUM_REPERS, 1 + 1);

    /* 4. Statistics */
    MakeALabel (stgrdc, 0, 0, "Min");
    MakeParKnob(stgrdc, 0, 0 + 1,
                " look=text ro",
                gui, mkparknob, S2V_PARAM_MIN);
    MakeALabel (stgrdc, 1, 0, "Max");
    MakeParKnob(stgrdc, 1, 0 + 1,
                " look=text ro",
                gui, mkparknob, S2V_PARAM_MAX);
    MakeALabel (stgrdc, 2, 0, "Avg");
    MakeParKnob(stgrdc, 2, 0 + 1,
                " look=text ro",
                gui, mkparknob, S2V_PARAM_AVG);
    MakeALabel (stgrdc, 3, 0, "Sum");
    MakeParKnob(stgrdc, 3, 0 + 1,
                " look=text ro",
                gui, mkparknob, S2V_PARAM_INT);
    XhGridSetSize(stgrdc, 4, 1 + 1);

    return cform;
}

pzframe_gui_dscr_t *s2v_get_gui_dscr(void)
{
  static fastadc_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        FastadcGuiFillStdDscr(&dscr,
                              pzframe2fastadc_type_dscr(s2v_get_type_dscr()));

        dscr.def_plot_w = 700;
        dscr.min_plot_w = 100;
        dscr.max_plot_w = 3000;
        dscr.def_plot_h = 256;
        dscr.min_plot_h = 100;
        dscr.max_plot_h = 3000;

        dscr.cpanel_loc = XH_PLOT_CPANEL_AT_TOP;
        dscr.mkctls     = s2v_mkctls;

        dscr_inited = 1;
    }
    return &(dscr.gkd);
}
