#include <stdio.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>

#include "cda.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "fastadc_data.h"
#include "fastadc_knobplugin.h"

#include "avgdiff_knobplugin.h"
#include "drv_i/adc200me_drv_i.h"
#include "liuclient.h"


enum {MAXPOINTS = 10000};

typedef struct
{
    char           *src;
    int             nl;
    Knob            sk;
    fastadc_data_t *adc_p;
} srcref_t;

typedef struct
{
    int              num;
    ADC200ME_DATUM_T pts[MAXPOINTS];
} onemes_t;

typedef struct
{
    srcref_t        adc;

    knobinfo_t     *ki;
    
    XhWindow        win;

    double          val;
    Widget          dpy;

    int             flipflop;
    onemes_t        data[2];
} AVGDIFF_privrec_t;

static psp_paramdescr_t text2AVGDIFF_opts[] =
{
    PSP_P_MSTRING("src", AVGDIFF_privrec_t, adc.src,  NULL, 1000),
    PSP_P_INT    ("nl",  AVGDIFF_privrec_t, adc.nl,   0, 0, 1),
    PSP_P_END()
};


//////////////////////////////////////////////////////////////////////

static void DoRecalc(AVGDIFF_privrec_t *me)
{
  int         x1, x2, tmpx;
  int         x;
  int32       sum;
  char        buf[400];

    x1 = manyadcs_x1;
    x2 = manyadcs_x2;
    if (x1 < 0  ||  x2 < 0) goto RET0;
    if (x1 > x2) {tmpx = x2; x2 = x1; x1 = tmpx;}
    if (x1 >= me->data[0].num  ||
        x2 >= me->data[0].num  ||
        x1 >= me->data[1].num  ||
        x2 >= me->data[1].num) goto RET0;

    for (x = x1, sum = 0;  x <= x2;  x++)
        sum += abs(me->data[1].pts[x] - me->data[0].pts[x]);

    me->val = FastadcDataPvl2Dsp(me->adc.adc_p, &(me->adc.adc_p->mes),
                                 me->adc.nl,
                                 FastadcDataRaw2Pvl(me->adc.adc_p, &(me->adc.adc_p->mes),
                                                    me->adc.nl,
                                                    sum / (x2 - x1 + 1)
                                                   )
                                );
    snprintf(buf, sizeof(buf),
             me->adc.adc_p->dcnv.lines[me->adc.nl].dpyfmt,
             me->val);
    XmTextSetString(me->dpy, buf);
    return;

 RET0:
    me->val = 0;
    XmTextSetString(me->dpy, "---");
}

//////////////////////////////////////////////////////////////////////

static void newdata_cb(pzframe_data_t *src_pfr,
                       int   reason,
                       int   info_changed,
                       void *privptr)
{
  AVGDIFF_privrec_t *me = privptr;
  onemes_t          *dp;

    if (reason != PZFRAME_REASON_BIGC_DATA) return;

    /* Shift generations */
    me->flipflop = 1 - me->flipflop;

    /* Get new data */
    dp = me->data + me->flipflop;
    dp->num = me->adc.adc_p->mes.plots[me->adc.nl].numpts;
    if (dp->num > MAXPOINTS)
        dp->num = MAXPOINTS;
    if (dp->num != 0)
        memcpy(dp->pts, 
               me->adc.adc_p->mes.plots[me->adc.nl].x_buf,
               dp->num * sizeof(ADC200ME_DATUM_T));

    /* Update */
    DoRecalc(me);
}

static int ConnectToADC(AVGDIFF_privrec_t *me)
{
  srcref_t *ref = &(me->adc);

    if ((ref->sk = ChlFindKnobFrom(me->win, ref->src, me->ki->uplink)) == NULL)
    {
        fprintf(stderr, "AVGDIFF %s \"%s\" not found\n",
                "adc200me", ref->src);
        return -1;
    }
    if ((ref->adc_p =
             pzframe2fastadc_data
             (
              PzframeKnobpluginRegisterCB(ref->sk, "adc200me",
                                          newdata_cb,
                                          me, NULL)
             )
        ) == NULL)
    {
        fprintf(stderr, "AVGDIFF %s adc200me \"%s\": failure to connect to\n",
                "", ref->src);
        return -1;
    }

    return 0;
}

static XhWidget AVGDIFF_Create_m(knobinfo_t *ki, XhWidget parent)
{
  AVGDIFF_privrec_t *me;

  char               buf[400];

    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(AVGDIFF_privrec_t))) == NULL)
        return NULL;
    me = (AVGDIFF_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, me, sizeof(*me), text2AVGDIFF_opts);

    me->ki  = ki;
    me->win = XhWindowOf(parent);

    if (ConnectToADC(me) < 0) goto ERRRET;

    snprintf(buf, sizeof(buf),
             me->adc.adc_p->dcnv.lines[me->adc.nl].dpyfmt,
             0.0);
    me->dpy =
        XtVaCreateManagedWidget("text_o", xmTextWidgetClass, parent,
                                XmNcolumns,               strlen(buf),
                                XmNscrollHorizontal,      False,
                                XmNcursorPositionVisible, False,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                XmNvalue, "",
                                NULL);

    return me->dpy;

 ERRRET:
    return XtVaCreateManagedWidget("avgDiff", widgetClass, parent,
                                   XmNwidth,       20,
                                   XmNheight,      10,
                                   XmNborderWidth, 0,
                                   XmNbackground,  XhGetColor(XH_COLOR_JUST_RED),
                                   NULL);
}

static knobs_vmt_t KNOB_AVGDIFF_VMT  = {AVGDIFF_Create_m,  NULL, NULL, PzframeKnobpluginNoColorize_m, NULL};
static knobs_widgetinfo_t avgdiff_widgetinfo[] =
{
    {0, "avgdiff", &KNOB_AVGDIFF_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  avgdiff_widgetset = {avgdiff_widgetinfo, NULL};


void  RegisterAVGDIFF_widgetset(int look_id)
{
    avgdiff_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&avgdiff_widgetset);
}
