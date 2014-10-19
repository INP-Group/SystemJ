#include <stdio.h>
#include <limits.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>

#include "misc_macros.h"
#include "misclib.h"

#include "Xh.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Chl.h"
#include "ChlI.h"

#include "vcamimg_gui.h"


//////////////////////////////////////////////////////////////////////

static void DpyModeKCB  (Knob k __attribute__((unused)), double v, void *privptr)
{
  vcamimg_gui_t       *gui = privptr;

    XhMonoimgSetDpyMode  (gui->view, round(v), 1);
}

static void NormalizeKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  vcamimg_gui_t       *gui = privptr;

    XhMonoimgSetNormalize(gui->view, round(v), 1);
}

static void Red_maxKCB  (Knob k __attribute__((unused)), double v, void *privptr)
{
  vcamimg_gui_t       *gui = privptr;

    XhMonoimgSetRed_max  (gui->view, round(v), 1);
}

static void Violet0KCB  (Knob k __attribute__((unused)), double v, void *privptr)
{
  vcamimg_gui_t       *gui = privptr;

    XhMonoimgSetViolet0  (gui->view, round(v), 1);
}

static Widget vcamimg_gui_mkstdctl (pzframe_gui_t *pzframe_gui,
                                    Widget parent,
                                    int kind, int a, int b)
{
  vcamimg_gui_t       *gui = pzframe2vcamimg_gui(pzframe_gui);
  vcamimg_type_dscr_t *vtd = gui->v.vtd;

  pzframe_gui_dpl_t *prmp;

  Widget     w;
  Knob       k;
  int        i;
  char       spec[1000];

    if      (kind == VCAMIMG_GUI_CTL_COMMONS)
    {
        PzframeGuiMkCommons(&(gui->g), parent);
        return gui->g.commons;
    }
    else if (kind == VCAMIMG_GUI_CTL_DPYMODE)
    {
        k = CreateSimpleKnob(" look=choicebs label=''"
                             " widdepinfo='#TAs-is\vInverse\vRainbow'",
                             0, ABSTRZE(parent), DpyModeKCB, gui);
        SetKnobValue(k, gui->look.dpymode);
        return CNCRTZE(GetKnobWidget(k));
    }
#if 0
    else if (kind == VCAMIMG_GUI_CTL_UNDEFECT)
    {
        return CNCRTZE(GetKnobWidget(k));
    }
#endif
    else if (kind == VCAMIMG_GUI_CTL_NORMALIZE)
    {
        k = CreateSimpleKnob(" look=onoff label='Normalize'"
                             " widdepinfo=''",
                             0, ABSTRZE(parent), NormalizeKCB, gui);
        SetKnobValue(k, gui->look.normalize);
        return CNCRTZE(GetKnobWidget(k));
    }
    else if (kind == VCAMIMG_GUI_CTL_MAX_RED)
    {
        k = CreateSimpleKnob(" look=onoff label='Max=red'"
                             " widdepinfo='color=red'",
                             0, ABSTRZE(parent), Red_maxKCB, gui);
        SetKnobValue(k, gui->look.maxred);
        return CNCRTZE(GetKnobWidget(k));
    }
    else if (kind == VCAMIMG_GUI_CTL_0_VIOLET)
    {
        k = CreateSimpleKnob(" look=onoff label='0=violet'"
                             " widdepinfo='color=violet'",
                             0, ABSTRZE(parent), Violet0KCB, gui);
        SetKnobValue(k, gui->look.violet0);
        return CNCRTZE(GetKnobWidget(k));
    }
    else
    {
        fprintf(stderr, "%s(): request for unknown kind=%d (a=%d, b=%d)\n",
                __FUNCTION__, kind, a, b);

        return NULL;
    }
}

static void PopulateCpanel(vcamimg_gui_t *gui,
                           vcamimg_gui_mkctls_t mkctls)
{
  Widget    cpanel;
  Widget    ctls;

  XhWindow  vci_win;
  XhWidget  leds_parent;
  int       in_toolbar;

    cpanel = gui->cpanel;
    if (cpanel != NULL  &&  mkctls != NULL)
    {
        ctls = mkctls(&(gui->g), gui->v.vtd, cpanel,
                      vcamimg_gui_mkstdctl, PzframeGuiMkparknob);

        /* Populate the "leds" widget */
        vci_win = XhWindowOf(ABSTRZE(ctls));
        in_toolbar  = 0;
        leds_parent = ABSTRZE(gui->g.local_leds);
        if (leds_parent == NULL)
        {
            in_toolbar  = 1;
            leds_parent = XhGetWindowAlarmleds(vci_win);
        }
        PzframeGuiMkLeds(&(gui->g), leds_parent, in_toolbar);

        /* And do fold, if requested */
        if (gui->g.look.foldctls)
            /*!!!XhPlotSetCpanelState(gui->plot, 0)*/;
    }
}

//////////////////////////////////////////////////////////////////////

static int DoRealize(pzframe_gui_t *pzframe_gui,
                     Widget parent,
                     const char *srvrspec, int bigc_n,
                     cda_serverid_t def_regsid, int base_chan)
{
  vcamimg_gui_t *gui = pzframe2vcamimg_gui(pzframe_gui);

    return VcamimgGuiRealize(gui, parent,
                             srvrspec, bigc_n,
                             def_regsid, base_chan);
}

static void VcamimgEvproc(pzframe_gui_t *inherited_gui,
                          int   reason,
                          int   info_changed,
                          void *privptr)
{
  vcamimg_gui_t *gui = (vcamimg_gui_t *)inherited_gui;

    /* Damn!!!  Why can't we employ zero-copy? */
    memcpy(XhMonoimgGetBuf(gui->view),
           gui->v.mes.inhrt->databuf,
           gui->v.vtd->ftd.max_datasize);
}

static void UpdateBG(pzframe_gui_t *inherited_gui)
{
  vcamimg_gui_t *gui = (vcamimg_gui_t *)inherited_gui;

  XhPixel fg = XhGetColor(XH_COLOR_JUST_WHITE);
  XhPixel bg = XhGetColor(XH_COLOR_JUST_BLACK);

    ChooseKnobColors(LOGC_NORMAL, gui->g.curstate, fg, bg, &fg, &bg);
    if (gui->view != NULL) /*!!!XhPlotSetBG(gui->plot, bg)*/;
}

static void DoRenew(pzframe_gui_t *inherited_gui,
                    int  info_changed)
{
  vcamimg_gui_t *gui = pzframe2vcamimg_gui(inherited_gui);

    XhMonoimgUpdate(gui->view);
}

static pzframe_gui_vmt_t vcamimg_gui_std_pzframe_vmt =
{
    .realize  = DoRealize,
    .evproc   = VcamimgEvproc,
    .newstate = UpdateBG,
    .do_renew = DoRenew,
};
void  VcamimgGuiFillStdDscr(vcamimg_gui_dscr_t *gkd, vcamimg_type_dscr_t *vtd)
{
    bzero(gkd, sizeof(*gkd));
    
    gkd->vtd     = vtd;
    gkd->gkd.ftd = &(gkd->vtd->ftd);
    gkd->gkd.bhv = PZFRAME_GUI_BHV_NORESIZE;
    gkd->gkd.vmt = vcamimg_gui_std_pzframe_vmt;
    //gkd->gkd.update  = NULL;
    
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

void  VcamimgGuiInit       (vcamimg_gui_t *gui, vcamimg_gui_dscr_t *gkd)
{
    bzero(gui, sizeof(*gui));
    VcamimgDataInit(&(gui->v), gkd->vtd);
    gui->d = gkd;
    PzframeGuiInit(&(gui->g), &(gui->v.pfr), &(gkd->gkd));
}

int   VcamimgGuiRealize    (vcamimg_gui_t *gui, Widget parent,
                            const char *srvrspec, int bigc_n, 
                            cda_serverid_t def_regsid, int base_chan)
{
  int         r;
  int         cpanel_loc;
  Widget      vw;

    if ((r = VcamimgDataRealize(&(gui->v),
                                srvrspec, bigc_n,
                                def_regsid, base_chan)) < 0) return r;
    PzframeGuiRealize(&(gui->g));

    cpanel_loc = gui->d->cpanel_loc;
    if (gui->g.look.nocontrols) cpanel_loc = 0/*!!!!!!!!!!!!!!!!*/;

    gui->container = XtVaCreateManagedWidget("container", xmFormWidgetClass,
                                             parent,
                                             XmNshadowThickness, 0,
                                             NULL);
    gui->cpanel    = XtVaCreateManagedWidget("cpanel",    xmFormWidgetClass,
                                             gui->container,
                                             XmNshadowThickness, 0,
                                             NULL);

    gui->view = XhCreateMonoimg(gui->container,
                                gui->v.vtd->data_w, gui->v.vtd->data_h,
                                gui->v.vtd->show_w, gui->v.vtd->show_h,
                                gui->v.vtd->sofs_x, gui->v.vtd->sofs_y,
                                gui->v.vtd->ftd.dataunits, gui->v.vtd->bpp, gui->v.vtd->srcmaxval,
                                gui);
    XhMonoimgSetDpyMode  (gui->view, gui->look.dpymode,   0);
    XhMonoimgSetNormalize(gui->view, gui->look.normalize, 0);
    XhMonoimgSetRed_max  (gui->view, gui->look.maxred,    0);
    XhMonoimgSetViolet0  (gui->view, gui->look.violet0,   0);
    vw = XhWidgetOfMonoimg(gui->view);
    attachtop(gui->cpanel, vw, 0);

    gui->g.top = gui->container;

    UpdateBG(&(gui->g));

    PopulateCpanel(gui, gui->d->mkctls);

    return 0;
}


void  VcamimgGuiRenewView  (vcamimg_gui_t *gui, int info_changed)
{
    XhMonoimgUpdate(gui->view);
}

static psp_paramdescr_t  vcamimg_gui_text2look[] =
{
    PSP_P_FLAG   ("direct",    vcamimg_gui_look_t, dpymode,   XH_MONOIMG_DPYMODE_DIRECT,  1),
    PSP_P_FLAG   ("as-is",     vcamimg_gui_look_t, dpymode,   XH_MONOIMG_DPYMODE_DIRECT,  0),
    PSP_P_FLAG   ("inverse",   vcamimg_gui_look_t, dpymode,   XH_MONOIMG_DPYMODE_INVERSE, 0),
    PSP_P_FLAG   ("rainbow",   vcamimg_gui_look_t, dpymode,   XH_MONOIMG_DPYMODE_RAINBOW, 0),

    PSP_P_FLAG   ("normalize", vcamimg_gui_look_t, normalize, 0,  0),
    PSP_P_FLAG   ("denorm",    vcamimg_gui_look_t, normalize, 0,  1),
    PSP_P_FLAG   ("maxred",    vcamimg_gui_look_t, maxred,    1,  1),
    PSP_P_FLAG   ("maxmax",    vcamimg_gui_look_t, maxred,    1,  0),
    PSP_P_FLAG   ("violet0",   vcamimg_gui_look_t, violet0,   1,  1),
    PSP_P_FLAG   ("black0",    vcamimg_gui_look_t, violet0,   1,  0),

    PSP_P_END()
};

psp_paramdescr_t *VcamimgGuiCreateText2LookTable(vcamimg_gui_dscr_t *gkd)
{
  psp_paramdescr_t *ret = NULL;
  size_t            ret_size;

  int               x;

    /* Allocate a table */
    ret_size = countof(vcamimg_gui_text2look) * sizeof(*ret);
    ret = malloc(ret_size);
    if (ret == NULL)
    {
        fprintf(stderr, "%s %s(type=%s): unable to allocate %zu bytes for PSP-table\n",
                strcurtime(), __FUNCTION__, gkd->gkd.ftd->type_name, ret_size);
        return ret;
    }

    /* And fill it */
    for (x = 0;  x < countof(vcamimg_gui_text2look);  x++)
    {
        ret[x] = vcamimg_gui_text2look[x];
        if      (ret[x].name   == NULL);
    }

    return ret;
}
