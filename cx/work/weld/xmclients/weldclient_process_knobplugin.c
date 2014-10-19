#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <Xm/Label.h>

#include "misclib.h"
#include "seqexecauto.h"

#include "cxlib.h"
#include "cda.h"
#include "datatree.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Chl.h"
#include "ChlI.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "drv_i/xcac208_drv_i.h"

#include "weldclient_process_knobplugin.h"


//////////////////////////////////////////////////////////////////////

static void MakeALabel(Widget  parent_grid, int grid_x, int grid_y,
                       const char *caption)
{
  Widget    w;
  XmString  s;

    w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, parent_grid,
                                XmNlabelString, s = XmStringCreateLtoR(caption, xh_charset),
                                NULL);
    XmStringFree(s);
  
    XhGridSetChildPosition (ABSTRZE(w), grid_x, grid_y);
    XhGridSetChildAlignment(ABSTRZE(w), -1,     -1);
    XhGridSetChildFilling  (ABSTRZE(w), 0,      0);
}

static Knob MakeAKnob(Widget parent_grid, int grid_x, int grid_y,
                      const char *spec,
                      simpleknob_cb_t  cb, void *privptr)
{
  Knob  k;

    k = CreateSimpleKnob(spec, 0, ABSTRZE(parent_grid), cb, privptr);
    if (k == NULL) return NULL;

    XhGridSetChildPosition (GetKnobWidget(k), grid_x, grid_y);
    XhGridSetChildAlignment(GetKnobWidget(k), -1,     -1);
    XhGridSetChildFilling  (GetKnobWidget(k), 0,      0);

    return k;
}



//////////////////////////////////////////////////////////////////////

static int important_xcac208_chans[] =
{
    XCAC208_CHAN_OUT_MODE,
    XCAC208_CHAN_DO_TAB_DROP,
    XCAC208_CHAN_DO_TAB_ACTIVATE,
    XCAC208_CHAN_DO_TAB_START,
    XCAC208_CHAN_DO_TAB_STOP,

    XCAC208_CHAN_OUT_MODE,

    XCAC208_CHAN_OUT_CUR_n_BASE + 0,
    XCAC208_CHAN_OUT_CUR_n_BASE + 1,
    XCAC208_CHAN_OUT_CUR_n_BASE + 2,
    XCAC208_CHAN_OUT_CUR_n_BASE + 3,
    XCAC208_CHAN_OUT_CUR_n_BASE + 4,
    XCAC208_CHAN_OUT_CUR_n_BASE + 5,
    XCAC208_CHAN_OUT_CUR_n_BASE + 6,
    XCAC208_CHAN_OUT_CUR_n_BASE + 7,
};

enum
{
    NUMLINES = 4
};

enum
{
    TABLE_STEPS  = 2,
    TABLE_LENGTH = TABLE_STEPS * (1 + XCAC208_CHAN_OUT_n_COUNT)
};


typedef struct
{
    void *p; 
    int   n;
} dpl_t;

typedef struct
{
    char                  *set_names   [NUMLINES];
    char                  *onoff_name;
} my_opts_t;

typedef struct
{
    int                    is_operational;

    XhWindow               win;

    cda_serverid_t         bigc_sid;
    cda_bigchandle_t       bigc_handle;
    cda_physchanhandle_t   chan_handles[XCAC208_NUMCHANS];
    int32                  chan_raws   [XCAC208_NUMCHANS];
    tag_t                  chan_tags   [XCAC208_NUMCHANS];
    rflags_t               chan_flgs   [XCAC208_NUMCHANS];
    ////
    dpl_t                  val_prm     [NUMLINES][2];
    Knob                   k_val       [NUMLINES][2];
    double                 k_val_value [NUMLINES][2];
    int                    k_val_inited[NUMLINES];
    Knob                   k_cur       [NUMLINES];
    Knob                   k_msc;
    int                    msc;
    ////
    Knob                   set_knobs   [NUMLINES];
    int                    set_lines   [NUMLINES];
    double                 set_r       [NUMLINES];
    double                 set_d       [NUMLINES];
    Knob                   onoff_knob;

    sea_context_t          sctx;
} WELD_PROCESS_privrec_t;

static psp_paramdescr_t text2processopts[] =
{
    PSP_P_MSTRING("line0", my_opts_t, set_names[0], "magsys.is.i_c2x_s",  1000),
    PSP_P_MSTRING("line1", my_opts_t, set_names[1], "magsys.is.i_c2y_s",  1000),
    PSP_P_MSTRING("line2", my_opts_t, set_names[2], "magsys.is.i_dc1x_s", 1000),
    PSP_P_MSTRING("line3", my_opts_t, set_names[3], "magsys.is.i_dc1y_s", 1000),
    PSP_P_MSTRING("onoff", my_opts_t, onoff_name,   "sync.bfp.ctl.work",  1000),
    PSP_P_END()
};

//////////////////////////////////////////////////////////////////////

enum
{
    EVT_DATA = 1,
};

static void one_cleanup_method(sea_context_t *ctx, sea_rslt_t reason)
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

    datatree_SetControlValue(me->onoff_knob, 0, 1);
    cda_setphyschanval(me->chan_handles[XCAC208_CHAN_DO_TAB_STOP],
                       CX_VALUE_COMMAND);
}

static void one_make_message_method(sea_context_t *ctx, const char *format, va_list ap)
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

    //vprintf(format, ap); printf("\n");
    XhVMakeMessage(me->win, format, ap);
}

static sea_rslt_t SendTableAction(sea_context_t *ctx, sea_step_t *step __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

  int32                   nsteps;
  int32                   table[TABLE_LENGTH];

  int                     nl;
  int                     nr;
  int                     nc;

    nsteps = TABLE_STEPS;
    bzero(table, sizeof(table));

    /* Mark all channels as "unused" */
    for (nc = 0;  nc < XCAC208_CHAN_OUT_n_COUNT; nc++)
        table[1 + nc] = XCAC208_VAL_DISABLE_TABLE_CHAN;
    /* Write time of a single step */
    table[1 + XCAC208_CHAN_OUT_n_COUNT] = me->msc * 1000;

    for (nl = 0;  nl < NUMLINES;  nl++)
    {
        nc = me->set_lines[nl];
        for (nr = 0;  nr < 2;  nr++)
        {
            table[nr * (1 + XCAC208_CHAN_OUT_n_COUNT) + 1 + nc] =
                round((me->k_val_value[nl][nr] + me->set_d[nl]) * me->set_r[nl]);
        }
    }

    cda_setbigcparams(me->bigc_handle, 0, 1, &nsteps);
    cda_setbigcdata  (me->bigc_handle, 0, sizeof(table), table, sizeof(table[0]));

    return SEA_RSLT_NONE;
}

static sea_rslt_t ActivateAction(sea_context_t *ctx, sea_step_t *step __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

    cda_setphyschanval(me->chan_handles[XCAC208_CHAN_DO_TAB_ACTIVATE],
                       CX_VALUE_COMMAND);

    return SEA_RSLT_NONE;
}

static sea_rslt_t ActivateChecker(sea_context_t *ctx, sea_step_t *step __attribute__((unused)), int evtype __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;
  int32                   v;

    cda_getphyschanraw(me->chan_handles[XCAC208_CHAN_DO_TAB_START],
                       &v, NULL, NULL);
    return v == 0? SEA_RSLT_SUCCESS : SEA_RSLT_NONE;
}

static sea_rslt_t GoAction(sea_context_t *ctx, sea_step_t *step __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

    datatree_SetControlValue(me->onoff_knob, 1, 1);
    cda_setphyschanval(me->chan_handles[XCAC208_CHAN_DO_TAB_START],
                       CX_VALUE_COMMAND);

    return SEA_RSLT_NONE;
}

static sea_rslt_t FinishChecker(sea_context_t *ctx, sea_step_t *step __attribute__((unused)), int evtype __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;
  int32                   v;

    cda_getphyschanraw(me->chan_handles[XCAC208_CHAN_OUT_MODE],
                       &v, NULL, NULL);
    return v == XCAC208_OUT_MODE_NORM? SEA_RSLT_SUCCESS : SEA_RSLT_NONE;
}

#if 0
static sea_rslt_t Action(sea_context_t *ctx, sea_step_t *step __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

}

static sea_rslt_t Checker(sea_context_t *ctx, sea_step_t *step __attribute__((unused)), int evtype __attribute__((unused)))
{
  WELD_PROCESS_privrec_t *me = ctx->privptr;

}
#endif

static sea_step_t one_prog[] =
{
    /*!!! Should detect table readyness somehow! args[0] != 0 ? */
    {"SendTable", SendTableAction,  SEA_SUCCESS_CHECK, NULL, 0,        1000000, NULL},
    {"Activate",  ActivateAction,   ActivateChecker,   NULL, EVT_DATA, 0,       NULL},
    {"Go",        GoAction,         SEA_SUCCESS_CHECK, NULL, 0,        0,       NULL},
    {"WELDING",   SEA_EMPTY_ACTION, FinishChecker,     NULL, EVT_DATA, 0,       NULL},
    SEA_END()
};

//////////////////////////////////////////////////////////////////////

static void ProcessBigcEventProc(cda_serverid_t  sid       __attribute__((unused)),
                                 int             reason,
                                 void           *privptr)
{
  WELD_PROCESS_privrec_t *me = privptr;

    if (reason != CDA_R_MAINDATA)                  return;
}

static void ProcessChanEventProc(WELD_PROCESS_privrec_t *me)
{
  int                     x;
  int                     nl;
  int                     cn;
  double                  val;

    for (x = 0;  x < countof(important_xcac208_chans);  x++)
        cda_getphyschanraw(me->chan_handles[important_xcac208_chans[x]],
                           me->chan_raws +  important_xcac208_chans[x],
                           me->chan_tags +  important_xcac208_chans[x],
                           me->chan_flgs +  important_xcac208_chans[x]);

    for (nl = 0;  nl < NUMLINES;  nl++)
    {
        val =
            me->chan_raws[XCAC208_CHAN_OUT_CUR_n_BASE + me->set_lines[nl]]
            /
            me->set_r[nl]
            -
            me->set_d[nl];
        SetKnobValue(me->k_cur[nl], val);
        if (me->k_val_inited[nl] == 0)
        {
            fprintf(stderr, "%d: %d -> %8.3f\n", nl,
                    me->chan_raws[XCAC208_CHAN_OUT_CUR_n_BASE + me->set_lines[nl]], val);
            SetKnobValue(me->k_val[nl][0], me->k_val_value[nl][0] = val);
            SetKnobValue(me->k_val[nl][1], me->k_val_value[nl][1] = val);
            me->k_val_inited[nl] = 1;
        }
    }

    sea_check(&(me->sctx), EVT_DATA);
}

static void ValKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  dpl_t                  *pp = privptr;
  WELD_PROCESS_privrec_t *me = pp->p;
  int                     x  = pp->n % 2;
  int                     nl = pp->n / 2;

    me->k_val_value[nl][x] = v;
}

static void MscKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  WELD_PROCESS_privrec_t *me = privptr;

    me->msc = round(v);
}

static void StartKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  WELD_PROCESS_privrec_t *me = privptr;

    if (!sea_is_running(&(me->sctx))) sea_run(&(me->sctx), "One", one_prog);
}

static void StopKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  WELD_PROCESS_privrec_t *me = privptr;

    sea_stop(&(me->sctx));
}

static XhWidget WELD_PROCESS_Create_m(knobinfo_t *ki, XhWidget parent)
{
  XhWidget                ret = NULL;

  WELD_PROCESS_privrec_t *me;

  my_opts_t               opts;
  groupelem_t            *grouplist;
  int                     i_chan_n;

  const char             *ph_srvref;
  int                     ph_chan_n;
  cda_serverid_t          ph_defsid;
  int                     ph_bigc_n;

  int                     x;
  int                     y;
  int                     nl;

  Widget                  dwgrid;

    /* . Obtain parameters */
    if (ki->kind != LOGK_DIRECT)
    {
        fprintf(stderr, "%s/\"%s\": kind!=LOGK_DIRECT!\n", ki->ident, ki->label);
        goto FINISH;
    }
    if (cda_srcof_physchan(ki->physhandle, &ph_srvref, &ph_chan_n) != 1)
    {
        fprintf(stderr, "%s/\"%s\": cda_srcof_physchan()!=1!\n", ki->ident, ki->label);
        goto FINISH;
    }
    ph_defsid = cda_sidof_physchan(ki->physhandle);
    /* Integer info passing hack */
    ph_bigc_n = ki->color;
    ki->color = LOGC_NORMAL;

    /* . Allocate privrec */
    if ((me = ki->widget_private = XtMalloc(sizeof(*me))) == NULL)
        goto FINISH;
    bzero(me, sizeof(*me));
    me->win = XhWindowOf(parent);

    /* . Parse */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2processopts);
    grouplist = ChlGetGrouplist(XhWindowOf(parent));
    for (nl = 0;  nl < NUMLINES;  nl++)
    {
        me->set_knobs[nl] = datatree_FindNode(grouplist, opts.set_names[nl]);
        if (me->set_knobs[nl] == NULL)
        {
            fprintf(stderr, "%s %s: knob for I#%d \"%s\" not found\n",
                    strcurtime(), __FUNCTION__, nl, opts.set_names[nl]);
            goto FINISH;
        }
        if (cda_srcof_physchan(me->set_knobs[nl]->physhandle, NULL, &i_chan_n) != 1)
        {
            fprintf(stderr, "%s %s: I#%d \"%s\": cda_srcof_physchan()!=1\n",
                    strcurtime(), __FUNCTION__, nl, opts.set_names[nl]);
            goto FINISH;
        }

        me->set_lines[nl] = i_chan_n - ph_chan_n - XCAC208_CHAN_OUT_n_BASE;
        fprintf(stderr, "[%d]->%d\n", nl, me->set_lines[nl]);

        cda_getphyschan_rd(me->set_knobs[nl]->physhandle, me->set_r + nl, me->set_d + nl);
    }
    me->onoff_knob = datatree_FindNode(grouplist, opts.onoff_name);
    if (me->onoff_knob == NULL)
    {
        fprintf(stderr, "%s %s: OnOff knob \"%s\" not found\n",
                strcurtime(), __FUNCTION__, opts.onoff_name);
        goto FINISH;
    }

    /* . Do connect */
    /* a. Big channel */
    me->bigc_sid = cda_new_server(ph_srvref,
                                  ProcessBigcEventProc, me,
                                  CDA_BIGC);
    if (me->bigc_sid == CDA_SERVERID_ERROR)
    {
        fprintf(stderr, "%s %s: cda_new_server(server=%s): %s\n",
                strcurtime(), __FUNCTION__, ph_srvref, cx_strerror(errno));
        goto FINISH;
    }
    me->bigc_handle = cda_add_bigc(me->bigc_sid, ph_bigc_n,
                                   1, sizeof(int32) * TABLE_LENGTH,
                                   CX_CACHECTL_SHARABLE, CX_BIGC_IMMED_YES);
    cda_run_server(me->bigc_sid);
    /* b. Regular channels */
    for (x = 0;  x < countof(me->chan_handles);  x++)
    {
        me->chan_handles[x] = CDA_PHYSCHANHANDLE_ERROR;
        me->chan_raws   [x] = -1;
    }
    for (x = 0;  x < countof(important_xcac208_chans);  x++)
    {
        me->chan_handles[important_xcac208_chans[x]] =
            cda_add_physchan(ph_defsid, ph_chan_n + important_xcac208_chans[x]);
    }

    /* . Prepare SEA context */
    me->sctx.privptr      = me;
    me->sctx.cleanup      = one_cleanup_method;
    me->sctx.make_message = one_make_message_method;

    /* . Create GUI */

    ret = dwgrid = XhCreateGrid(parent, "grid");
    XhGridSetGrid   (dwgrid, 0, 0);
    XhGridSetSpacing(dwgrid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(dwgrid, 0, 0);

    y = 0;
    MakeALabel(dwgrid, 1, y, "ТекущУст");
    MakeALabel(dwgrid, 2, y, "Начало");
    MakeALabel(dwgrid, 3, y, "Конец");
    y++;

    for (nl = 0;  nl < NUMLINES;  nl++, y++)
    {
        MakeALabel(dwgrid, 0, y, me->set_knobs[nl]->label);
        me->k_cur[nl] = MakeAKnob(dwgrid, 1, y,
                                  " ro look=text dpyfmt='% 7.4f'",
                                  NULL, NULL);
        for (x = 0;  x < 2;  x++)
        {
            me->val_prm[nl][x].p = me;
            me->val_prm[nl][x].n = nl * 2 + x;
            me->k_val  [nl][x] = MakeAKnob(dwgrid, x + 2, y,
                                           " rw look=text dpyfmt='% 7.4f'",
                                           ValKCB, &(me->val_prm[nl][x]));
            me->k_val  [nl][x]->incdec_step             = me->set_knobs[nl]->incdec_step;
            me->k_val  [nl][x]->rmins[KNOBS_RANGE_NORM] = me->set_knobs[nl]->rmins[KNOBS_RANGE_NORM];
            me->k_val  [nl][x]->rmaxs[KNOBS_RANGE_NORM] = me->set_knobs[nl]->rmaxs[KNOBS_RANGE_NORM];
        }
    }

    MakeALabel(dwgrid, 0, y, "Время, ms");
    me->k_msc = MakeAKnob(dwgrid, 1, y,
                          " rw look=text dpyfmt='%5.0f'"
                          " minnorm=0 maxnorm=99999",
                          MscKCB, me);
    SetKnobValue(me->k_msc, me->msc = 5000);
    MakeAKnob(dwgrid, 2, y,
              " rw look=button label='Start'",
              StartKCB, me);
    MakeAKnob(dwgrid, 3, y,
              " rw look=button label='Stop'",
              StopKCB, me);
    y++;

    XhGridSetSize(dwgrid, 4, y);

    me->is_operational = 1;

 FINISH:
    if (ret == NULL)
        return ABSTRZE(XtVaCreateManagedWidget("", widgetClass, CNCRTZE(parent),
                                               XmNwidth,      20,
                                               XmNheight,     10,
                                               XmNbackground, XhGetColor(XH_COLOR_JUST_RED),
                                               NULL));

    return ret;
}

static void WELD_PROCESS_SetValue_m(knobinfo_t *ki, double v)
{
    ProcessChanEventProc(ki->widget_private);
}

static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}


static knobs_vmt_t KNOB_WELD_PROCESS_VMT =
{
    WELD_PROCESS_Create_m,
    NULL,
    WELD_PROCESS_SetValue_m,
    NoColorize_m,
    NULL
};
static knobs_widgetinfo_t weld_process_widgetinfo[] =
{
    {0, "weld_process", &KNOB_WELD_PROCESS_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  weld_process_widgetset = {weld_process_widgetinfo, NULL};

void  RegisterWELD_PROCESS_widgetset(int look_id)
{
    weld_process_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&weld_process_widgetset);
}
