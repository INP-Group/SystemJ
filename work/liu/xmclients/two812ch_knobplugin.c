#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>

#include "misclib.h"

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_gui.h"
#include "fastadc_knobplugin.h"
#include "fastadc_cpanel_decor.h"

#include "drv_i/adc812me_drv_i.h"
#include "adc812me_data.h"
#include "two812ch_knobplugin.h"




// _drv_i.h
enum
{
    TWO812CH_NUM_LINES = 2
};


//--------------------------------------------------------------------
// _data.c

static const char *two812ch_line_name_list[] = {"A", "B"};

pzframe_type_dscr_t *two812ch_get_type_dscr(void)
{
  static fastadc_type_dscr_t  dscr;
  static int                  dscr_inited;
  
  fastadc_type_dscr_t        *adc812me_atd;

    if (!dscr_inited)
    {
        adc812me_atd = pzframe2fastadc_type_dscr(adc812me_get_type_dscr());
        FastadcDataFillStdDscr(&dscr, "two812ch",
                               PZFRAME_B_ARTIFICIAL | PZFRAME_B_NO_SVD | PZFRAME_B_NO_ALLOC,
                               /* Bigc-related ADC characteristics */
                               adc812me_atd->ftd.dtype,
                               0,
                               adc812me_atd->max_numpts, TWO812CH_NUM_LINES,
                               adc812me_atd->param_numpts,
                               NULL/*_info2mes*/,
                               /* Description of bigc-parameters */
                               NULL/*text2_params*/,
                               NULL/*_param_dscrs*/,
                               "", two812ch_line_name_list, adc812me_atd->line_unit_list,
                               adc812me_atd->range,
                               "% 6.3f", adc812me_atd->raw2pvl,
                               adc812me_atd->x2xs, adc812me_atd->xs
                              );
        dscr_inited = 1;
    }
    return &(dscr.ftd);
}
//--------------------------------------------------------------------
// _gui.c
static Widget two812ch_mkctls(pzframe_gui_t           *gui,
                              fastadc_type_dscr_t     *atd,
                              Widget                   parent,
                              pzframe_gui_mkstdctl_t   mkstdctl,
                              pzframe_gui_mkparknob_t  mkparknob)
{
  Widget  cform;    // Controls form
  Widget  hline;    // Separator line
  Widget  pcgrid;   // Per-Channel-parameters grid
  Widget  icform;   // Intra-cell form

  Widget  w1;
  Widget  w2;

  int     y;
  int     nl;
  int     nr;
  
    /* 1. General layout */
    /* A container form */
    cform = XtVaCreateManagedWidget("form", xmFormWidgetClass, parent,
                                    XmNshadowThickness, 0,
                                    NULL);
    /* A grid for per-channel parameters */
    pcgrid = XhCreateGrid(cform, "grid");
    XhGridSetGrid   (pcgrid, 0, 0);
    XhGridSetSpacing(pcgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(pcgrid, 0, 0);

    /* 3. Per-channel parameters and repers */
    for (nl = 0;  nl < TWO812CH_NUM_LINES;  nl++)
    {
        y = 0;

        MakeStdCtl(pcgrid, nl + 1, y++, gui, mkstdctl, FASTADC_GUI_CTL_LINE_LABEL, nl, 0);

        if (nl == 0)
            MakeStdCtl(pcgrid, 0, y, gui, mkstdctl, FASTADC_GUI_CTL_X_CMPR, 0, 0);
        MakeStdCtl(pcgrid, nl + 1, y++, gui, mkstdctl, FASTADC_GUI_CTL_LINE_MAGN,  nl, 0);

        for (nr = 0;  nr < XH_PLOT_NUM_REPERS;  nr++, y++)
        {
            if (nl == 0)
            {
                icform = MakeAForm(pcgrid, 0, y);
                w1 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_ONOFF, 0, nr);
                w2 = mkstdctl(gui, icform, FASTADC_GUI_CTL_REPER_TIME,  0, nr);
                attachleft(w2, w1, NULL);
            }
            MakeStdCtl(pcgrid, nl + 1, y, gui, mkstdctl, FASTADC_GUI_CTL_REPER_VALUE, nl, nr);
        }
    }

    XhGridSetSize(pcgrid, 3, y);

    return cform;
}

pzframe_gui_dscr_t *two812ch_get_gui_dscr(void)
{
  static fastadc_gui_dscr_t  dscr;
  static int                 dscr_inited;

    if (!dscr_inited)
    {
        FastadcGuiFillStdDscr(&dscr,
                              pzframe2fastadc_type_dscr(two812ch_get_type_dscr()));

        dscr.def_plot_w = 700;
        dscr.min_plot_w = 100;
        dscr.max_plot_w = 3000;
        dscr.def_plot_h = 256;
        dscr.min_plot_h = 100;
        dscr.max_plot_h = 3000;

        dscr.cpanel_loc = XH_PLOT_CPANEL_AT_LEFT;
        dscr.mkctls     = two812ch_mkctls;

        dscr_inited = 1;
    }
    return &(dscr.gkd);
}
//--------------------------------------------------------------------

typedef struct
{
    fastadc_knobplugin_t  kpn;

    char           *source;

    int             seg_n;
    fastadc_data_t *srcadc_p;

    knobinfo_t     *ki;
    XhWindow        win;
} TWO812CH_privrec_t;

static psp_paramdescr_t text2TWO812CH_opts[] =
{
    PSP_P_MSTRING("source", TWO812CH_privrec_t, source,  NULL, 1000),
    PSP_P_END()
};


static void two812ch_newdata_cb(pzframe_data_t *src_pfr,
                                int   reason,
                                int   info_changed,
                                void *privptr)
{
  TWO812CH_privrec_t *me  = privptr;
  fastadc_gui_t      *gui = &(me->kpn.g);
  fastadc_data_t     *adc = &(gui->a);
  fastadc_data_t     *src_adc = pzframe2fastadc_data(src_pfr);

  int                 nl;

    //fprintf(stderr, "%s(seg_n=%d)!\n", __FUNCTION__, me->seg_n);

    if (reason != PZFRAME_REASON_BIGC_DATA) return;

    /* Copy all data from source */
    memcpy(adc->mes.inhrt->info, src_pfr->mes.info, sizeof(adc->mes.inhrt->info));
    adc->mes.inhrt->rflags = src_pfr->mes.rflags;
    adc->mes.inhrt->tag    = src_pfr->mes.tag;
    adc->mes.exttim        = src_adc->mes.exttim;
    for (nl = 0;  nl < TWO812CH_NUM_LINES;  nl++)
    {
        adc->mes.plots[nl] =
            src_adc->mes.plots[me->seg_n * 2 + nl];
        adc->mes.inhrt-> info[ADC812ME_PARAM_RANGE0 + nl] =
            src_pfr->mes.info[ADC812ME_PARAM_RANGE0 + me->seg_n * 2 + nl];
    }

    /* Perform update */
    FastadcGuiRenewPlot(gui,
                        gui->a.mes.inhrt->info[gui->a.atd->param_numpts],
                        info_changed);
}

static int  two812ch_kp_realize(pzframe_knobplugin_t *kpn,
                                Knob                  k)
{
  TWO812CH_privrec_t *me = (TWO812CH_privrec_t *)(k->widget_private);
  knobinfo_t         *sk = ChlFindKnob(me->win, me->source);

  int                 nl;

    if (sk == NULL)
    {
        fprintf(stderr, "%s(): adc812me \"%s\" not found\n",
                __FUNCTION__, me->source);
        return -1;
    }
    me->srcadc_p =
        pzframe2fastadc_data
        (
         PzframeKnobpluginRegisterCB(sk, "adc812me",
                                     two812ch_newdata_cb,
                                     me, NULL)
        );
    for (nl = 0;  nl < TWO812CH_NUM_LINES;  nl++)
        me->kpn.g.a.dcnv.lines[nl] = 
            me->srcadc_p->dcnv.lines[me->seg_n * 2 + nl];

    return 0;
}

static XhWidget TWO812CH_Create_m(knobinfo_t *ki, XhWidget parent)
{
  TWO812CH_privrec_t         *me;

    me = ki->widget_private = XtMalloc(sizeof(*me));
    if (me == NULL) return NULL;
    bzero(me, sizeof(*me));

    me->ki    = ki;
    me->seg_n = ki->color; ki->color = LOGC_NORMAL;
    me->win   = XhWindowOf(parent);

    return FastadcKnobpluginDoCreate(ki, parent,
                                     &(me->kpn),
                                     two812ch_kp_realize,
                                     two812ch_get_gui_dscr(),
                                     text2TWO812CH_opts, me);
}


static knobs_vmt_t KNOB_TWO812CH_VMT  = {TWO812CH_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t two812ch_widgetinfo[] =
{
    {0, "two812ch", &KNOB_TWO812CH_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  two812ch_widgetset = {two812ch_widgetinfo, NULL};


void  RegisterTWO812CH_widgetset(int look_id)
{
    two812ch_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&two812ch_widgetset);
}
