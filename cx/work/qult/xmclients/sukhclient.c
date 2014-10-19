#include <stdio.h>

#include "misclib.h"

#include "Chl.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"
#include "fastadc_cpanel_decor.h"

#include "drv_i/nadc200_drv_i.h"
#include "nadc200_data.h"
#include "nadc200_knobplugin.h"

#include "pzframeclient.h"
#include "sukhclient.h"


//////////////////////////////////////////////////////////////////////

#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"


// _drv_i.h
typedef double SUKHVIEW_DATUM_T;
enum
{
    SUKHVIEW_NUM_SUBORD = 2,
    SUKHVIEW_MAX_NUMPTS = NADC200_MAX_NUMPTS,
    SUKHVIEW_NUM_LINES  = SUKHVIEW_NUM_SUBORD * NADC200_NUM_LINES,
    SUKHVIEW_DATAUNITS  = sizeof(SUKHVIEW_DATUM_T),
};
#define SUKHVIEW_MIN_VALUE -100.0
#define SUKHVIEW_MAX_VALUE +100.0
//--------------------------------------------------------------------
// _data.c
enum
{
    SUKHVIEW_DTYPE = ENCODE_DTYPE(SUKHVIEW_DATAUNITS,
                                  CXDTYPE_REPR_FLOAT, 0)
};

static const char *sukhview_line_name_list[] = {"?0", "?1", "?2", "?3"};
static const char *sukhview_line_unit_list[] = {"?a", "?b", "?c", "?d"};


pzframe_type_dscr_t *sukhview_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;
  
  fastadc_type_dscr_t        *nadc200_atd;

    if (!dscr_inited)
    {
        nadc200_atd = pzframe2fastadc_type_dscr(nadc200_get_type_dscr());
        FastadcDataFillStdDscr(&dscr, "sukhview",
                               PZFRAME_B_ARTIFICIAL | PZFRAME_B_NO_SVD | PZFRAME_B_NO_ALLOC,
                               /* Bigc-related ADC characteristics */
                               SUKHVIEW_DTYPE,
                               0,
                               nadc200_atd->max_numpts, SUKHVIEW_NUM_LINES,
                               nadc200_atd->param_numpts,
                               NULL/*nadc200_info2mes*/,
                               /* Description of bigc-parameters */
                               NULL/*text2nadc200_params*/,
                               NULL/*nadc200_param_dscrs*/,
                               "", sukhview_line_name_list, sukhview_line_unit_list,
                               (plotdata_range_t){.dbl_r={SUKHVIEW_MIN_VALUE,
                                                          SUKHVIEW_MAX_VALUE}},
                               "% 6.3f", NULL/*nadc200_raw2pvl*/,
                               nadc200_atd->x2xs, nadc200_atd->x2xs
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
//--------------------------------------------------------------------
// _gui.c
static Widget sukhview_mkctls(pzframe_gui_t           *gui,
                              fastadc_type_dscr_t     *atd,
                              Widget                   parent,
                              pzframe_gui_mkstdctl_t   mkstdctl,
                              pzframe_gui_mkparknob_t  mkparknob)
{
  Widget              cform;    // Controls form
  Widget              cmpr;     // Per-Channel-parameters grid
  Widget              pcgrid;   // Per-Channel-parameters grid
  Widget              rpgrid;   // Repers grid
  Widget              icform;   // Intra-cell form

  Widget  w1;
  Widget  w2;

  int     nl;
  int     nr;

    /* 1. General layout */
    /* A container form */
    cform = XtVaCreateManagedWidget("form", xmFormWidgetClass, parent,
                                    XmNshadowThickness, 0,
                                    NULL);
    /* Cmpr */
    cmpr = mkstdctl(gui, cform, FASTADC_GUI_CTL_X_CMPR, 0, 0);
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
    attachright (cmpr,    rpgrid,  CHL_INTERKNOB_H_SPACING);
    attachright (pcgrid,  rpgrid,  CHL_INTERKNOB_H_SPACING);
    attachtop   (pcgrid,  cmpr,    CHL_INTERKNOB_V_SPACING);

    /* 3. Per-channel parameters and repers */
    for (nl = 0;  nl < SUKHVIEW_NUM_LINES;  nl++)
    {
        w1 = MakeStdCtl(pcgrid, 0, nl, gui, mkstdctl, FASTADC_GUI_CTL_LINE_LABEL, nl, 1);
        XhGridSetChildFilling  (w1, 1,      0);
        MakeStdCtl     (pcgrid, 1, nl, gui, mkstdctl, FASTADC_GUI_CTL_LINE_MAGN,  nl, 1);

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

    return cform;
}

pzframe_gui_dscr_t *sukhview_get_gui_dscr(void)
{
  static fastadc_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        FastadcGuiFillStdDscr(&dscr,
                              pzframe2fastadc_type_dscr(sukhview_get_type_dscr()));

        dscr.def_plot_w = 700;
        dscr.min_plot_w = 100;
        dscr.max_plot_w = 3000;
        dscr.def_plot_h = 256;
        dscr.min_plot_h = 100;
        dscr.max_plot_h = 3000;

        dscr.cpanel_loc = XH_PLOT_CPANEL_AT_BOTTOM;
        dscr.mkctls     = sukhview_mkctls;

        dscr_inited = 1;
    }
    return &(dscr.gkd);
}
//--------------------------------------------------------------------

typedef struct
{
    fastadc_knobplugin_t  kpn;

    char              *sources[SUKHVIEW_NUM_SUBORD];

    fastadc_data_t    *srcadc [SUKHVIEW_NUM_SUBORD];
    pzframe_gui_dpl_t  cb_prm [SUKHVIEW_NUM_SUBORD];

    knobinfo_t        *ki;
    XhWindow           win;

    SUKHVIEW_DATUM_T   x_bufs[SUKHVIEW_NUM_LINES][SUKHVIEW_MAX_NUMPTS];
} SUKHVIEW_privrec_t;

static psp_paramdescr_t text2SUKHVIEW_opts[] =
{
    PSP_P_MSTRING("src1", SUKHVIEW_privrec_t, sources[0],  NULL, 1000),
    PSP_P_MSTRING("src2", SUKHVIEW_privrec_t, sources[1],  NULL, 1000),
    PSP_P_END()
};


static void sukhview_newdata_cb(pzframe_data_t *src_pfr,
                                int   reason,
                                int   info_changed,
                                void *privptr)
{
  pzframe_gui_dpl_t  *pp  = privptr;
  SUKHVIEW_privrec_t *me  = pp->p;
  int                 s_n = pp->n;

  fastadc_gui_t      *gui = &(me->kpn.g);
  fastadc_data_t     *adc = &(gui->a);
  pzframe_data_t     *pfr = &(adc->pfr);
  int                 maxpts;

  int                 a_n;
  fastadc_data_t     *as;
  fastadc_mes_t      *ms;
  int                 nl;
  plotdata_t         *s_p;
  plotdata_t         *d_p;
  int                 x;
  SUKHVIEW_DATUM_T   *dstp;

    if (reason != PZFRAME_REASON_BIGC_DATA) return;

    /* Copy info from sources */
    pfr->mes.rflags = 0;
    pfr->mes.tag    = 0;
    adc->mes.exttim = 0;
    maxpts          = 0;
    for (a_n = 0;  a_n < SUKHVIEW_NUM_SUBORD;  a_n++)
    {
        ms = &(me->srcadc[a_n]->mes);

        pfr->mes.rflags |= ms->inhrt->rflags;
        if (pfr->mes.tag < ms->inhrt->tag)
            pfr->mes.tag = ms->inhrt->tag;
        adc->mes.exttim |= ms->exttim;

        for (nl = 0;  nl < NADC200_NUM_LINES;  nl++)
            if (maxpts < ms->plots[nl].numpts)
                maxpts = ms->plots[nl].numpts;
    }
    pfr->mes.info[adc->atd->param_numpts] = maxpts;

    /* Copy data */
    as = me->srcadc[s_n];
    ms = &(as->mes);
    for (nl = 0;  nl < NADC200_NUM_LINES;  nl++)
    {
        s_p = ms->plots + nl;
        d_p = adc->mes.plots + (s_n * NADC200_NUM_LINES) + nl;

        d_p->on          = s_p->on;
        d_p->numpts      = s_p->numpts;
        if (d_p->numpts > SUKHVIEW_MAX_NUMPTS)
            d_p->numpts = SUKHVIEW_MAX_NUMPTS;
        d_p->x_buf       = me->x_bufs[(s_n * NADC200_NUM_LINES) + nl];
        d_p->x_buf_dtype = SUKHVIEW_DTYPE;

        dstp = d_p->x_buf;

        for (x = 0;  x < d_p->numpts; x++)
            *dstp++ = FastadcDataGetDsp(as, ms, nl, x) * 10;
    }

    /* Perform update */
    FastadcGuiRenewPlot(gui, maxpts, info_changed);
}

static int  sukhview_kp_realize(pzframe_knobplugin_t *kpn,
                                Knob                  k)
{
  SUKHVIEW_privrec_t *me = (SUKHVIEW_privrec_t *)(k->widget_private);
  int                 stage;
  int                 a_n;
  knobinfo_t         *sk;

    for (stage = 0;  stage < 2;  stage++)
        for (a_n = 0;  a_n < SUKHVIEW_NUM_SUBORD;  a_n++)
        {
            sk = ChlFindKnob(me->win, me->sources[a_n]);
            if (sk == NULL)
            {
                fprintf(stderr, "%s(): nadc200#%d \"%s\" not found\n",
                        __FUNCTION__, a_n+1, me->sources[a_n]);
                return -1;
            }

            me->cb_prm[a_n].p = me;
            me->cb_prm[a_n].n = a_n;
            if (stage == 1)
                me->srcadc[a_n] =
                    pzframe2fastadc_data
                    (
                     PzframeKnobpluginRegisterCB(sk, "nadc200",
                                                 sukhview_newdata_cb,
                                                 me->cb_prm + a_n, NULL)
                    );
        }

    return 0;
}

static XhWidget SUKHVIEW_Create_m(knobinfo_t *ki, XhWidget parent)
{
  SUKHVIEW_privrec_t         *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));

    me->ki    = ki;
    me->win   = XhWindowOf(parent);

    return FastadcKnobpluginDoCreate(ki, parent,
                                     &(me->kpn),
                                     sukhview_kp_realize,
                                     sukhview_get_gui_dscr(),
                                     text2SUKHVIEW_opts, me);
}

static knobs_vmt_t KNOB_SUKHVIEW_VMT  = {SUKHVIEW_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t sukhview_widgetinfo[] =
{
    {0, "sukhview", &KNOB_SUKHVIEW_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  sukhview_widgetset = {sukhview_widgetinfo, NULL};


static void  RegisterSUKHVIEW_widgetset(int look_id)
{
    sukhview_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&sukhview_widgetset);
}


//////////////////////////////////////////////////////////////////////


static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


int main(int argc, char *argv[])
{
  int  r;

    XhSetColorBinding("GRAPH_REPERS", "#00FF00");

    RegisterNADC200_widgetset (LOGD_NADC200);
    RegisterSUKHVIEW_widgetset(LOGD_SUKHVIEW);

    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
