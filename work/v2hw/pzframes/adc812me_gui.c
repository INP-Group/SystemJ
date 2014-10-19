#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>


#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_cpanel_decor.h"

#include "adc812me_data.h"
#include "adc812me_gui.h"

#include "drv_i/adc812me_drv_i.h"


static Widget adc812me_mkctls(pzframe_gui_t           *gui,
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
  Widget  clsubw;
  Widget  clform;
  Widget  clgrdg;
  Widget  clgrdc;

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

    /* Timing */
    MakeALabel (dwgrid, 0, y, "Timing");
    MakeParKnob(dwgrid, 1, y++,
                " look=selector label=''"
                " widdepinfo='#C#TInt 4MHz\vExt \"TIMER\"'",
                gui, mkparknob, ADC812ME_PARAM_TIMING);
    
    /* Frqdiv */
    MakeALabel (dwgrid, 0, y, "Int. frq. div");
    MakeParKnob(dwgrid, 1, y++,
                " look=selector label=''"
                " widdepinfo='#C#T/1 (4MHz)\v/2 (2MHz)\v/4 (1MHz)\v/8 (0.5MHz)'",
                gui, mkparknob, ADC812ME_PARAM_FRQDIV);

    /* IStart */
    MakeALabel (dwgrid, 0, y, "Int. Start");
    MakeParKnob(dwgrid, 1, y++,
                " look=greenled label='Int. Start' widdepinfo='panel'",
                gui, mkparknob, ADC812ME_PARAM_ISTART);
    
    /* Wait time */
    MakeALabel (dwgrid, 0, y, "Wait time");
    MakeParKnob(dwgrid, 1, y++,
                " look=text"
                " widdepinfo='withunits'"
                " units=ms minnorm=0 maxnorm=1000000000 dpyfmt=%7.0f step=1000",
                gui, mkparknob, ADC812ME_PARAM_WAITTIME);
    
    XhGridSetSize(dwgrid, 2, y);

    /* Calibrate */
    clsubw = MakeSubwin(dwgrid, 0, y,
                        "Calibr...", "Calibrations");
    
    /* Shot, Stop */
    icform = MakeAForm(dwgrid, 1, y++);
    XhGridSetChildFilling(icform, 1, 0);
    w1 = mkparknob(gui, icform,
                   " look=button label='Stop'",
                   ADC812ME_PARAM_STOP);
    attachright(w1, NULL, 0);
    w2 = mkparknob(gui, icform,
                   " look=button label='Shot!'",
                   ADC812ME_PARAM_SHOT);
    attachright(w2, w1, CHL_INTERKNOB_H_SPACING);

    /* Serial */
    MakeALabel (dwgrid, 0, y, "Serial");
    MakeParKnob(dwgrid, 1, y++,
                " look=text ro dpyfmt=%-6.0f label='#' widdepinfo=withlabel",
                gui, mkparknob, ADC812ME_PARAM_SERIAL);

    /* NUMPTS@PTSOFS */
    snprintf(spec, sizeof(spec),
             "look=text label='' step=100 "
             "widdepinfo='' "
             "minnorm=%d maxnorm=%d mindisp=%d maxdisp=%d dpyfmt='%%6.0f' ",
             1, ADC812ME_MAX_NUMPTS, 1, ADC812ME_MAX_NUMPTS);
    w1 = mkparknob(gui, ptsform, spec, ADC812ME_PARAM_NUMPTS);
    snprintf(spec, sizeof(spec),
             "look=text label='@' step=100 "
             "widdepinfo='withlabel' "
             "minnorm=%d maxnorm=%d mindisp=%d maxdisp=%d dpyfmt='%%6.0f' ",
             0, ADC812ME_MAX_NUMPTS-1, 0, ADC812ME_MAX_NUMPTS-1);
    w2 = mkparknob(gui, ptsform, spec, ADC812ME_PARAM_PTSOFS);
    attachleft(w2, w1, 0);

    /* 3. Per-channel parameters and repers */
    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        w1 = MakeStdCtl(pcgrid, 0, nl, gui, mkstdctl, FASTADC_GUI_CTL_LINE_LABEL, nl, 1);
        XhGridSetChildFilling  (w1, 1,      0);
        MakeStdCtl     (pcgrid, 1, nl, gui, mkstdctl, FASTADC_GUI_CTL_LINE_MAGN,  nl, 1);
        MakeParKnob    (pcgrid, 2, nl,
                        " look=selector label=''"
                        " widdepinfo='#+\v#T8.192V\v4.096V\v2.048V\v1.024V'",
                        gui, mkparknob, ADC812ME_PARAM_RANGE0 + nl);

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

    /* 4. Calibration */
    clform = XtVaCreateManagedWidget("form", xmFormWidgetClass, clsubw,
                                     XmNshadowThickness, 0,
                                     NULL);
    clgrdg = XhCreateGrid(clform, "grid");
    XhGridSetGrid   (clgrdg, 0, 0);
    XhGridSetSpacing(clgrdg, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(clgrdg, 0, 0);
    clgrdc = XhCreateGrid(clform, "grid");
    XhGridSetGrid   (clgrdc, 0, 0);
    XhGridSetSpacing(clgrdc, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(clgrdc, 0, 0);
    attachtop(clgrdc, clgrdg, CHL_INTERKNOB_V_SPACING);

    y = 0;
    MakeParKnob(clgrdg, 0, y++,
                " look=button label='Calibrate'",
                gui, mkparknob, ADC812ME_PARAM_CALIBRATE);
    MakeParKnob(clgrdg, 0, y++,
                " look=button label='Reset calibration'",
                gui, mkparknob, ADC812ME_PARAM_FGT_CLB);
    MakeParKnob(clgrdg, 0, y++,
                " look=blueled label='Visible calibration'",
                gui, mkparknob, ADC812ME_PARAM_VISIBLE_CLB);
    MakeParKnob(clgrdg, 0, y++,
                " look=greenled ro label='Active'",
                gui, mkparknob, ADC812ME_PARAM_CLB_STATE);
    XhGridSetSize(clgrdg, 1, y);

    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        snprintf(spec, sizeof(spec),
                 "%s", fastadc_gui->a.dcnv.lines[nl].descr);
        MakeALabel(clgrdc, 0, nl + 1, spec);

        if (nl == 0)
            MakeALabel(clgrdc, 1, 0, "d0");
        MakeParKnob(clgrdc, 1, nl + 1,
                    " look=text ro",
                    gui, mkparknob, ADC812ME_PARAM_CLB_OFS0 + nl);

        if (nl == 0)
            MakeALabel(clgrdc, 2, 0, "Cor");
        MakeParKnob(clgrdc, 2, nl + 1,  
                    " look=text ro",
                    gui, mkparknob, ADC812ME_PARAM_CLB_COR0 + nl);
    }

    XhGridSetSize(clgrdc, 3, nl + 1);   

    return cform;
}

pzframe_gui_dscr_t *adc812me_get_gui_dscr(void)
{
  static fastadc_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        FastadcGuiFillStdDscr(&dscr,
                              pzframe2fastadc_type_dscr(adc812me_get_type_dscr()));

        dscr.def_plot_w = 700;
        dscr.min_plot_w = 100;
        dscr.max_plot_w = 3000;
        dscr.def_plot_h = 256;
        dscr.min_plot_h = 100;
        dscr.max_plot_h = 3000;

        dscr.cpanel_loc = XH_PLOT_CPANEL_AT_BOTTOM;
        dscr.mkctls     = adc812me_mkctls;

        dscr_inited = 1;
    }
    return &(dscr.gkd);
}
