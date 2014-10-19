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

#include "timeval_utils.h"

#include "Xh.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Chl.h"
#include "ChlI.h"

#include "pzframe_gui.h"

#include "pixmaps/btn_mini_save.xpm"
#include "pixmaps/btn_mini_start.xpm"
#include "pixmaps/btn_mini_once.xpm"


psp_paramdescr_t  pzframe_gui_text2look[] =
{
    PSP_P_FLAG   ("foldctls",     pzframe_gui_look_t, foldctls,     1, 0),
    PSP_P_FLAG   ("nocontrols",   pzframe_gui_look_t, nocontrols,   1, 0),
    PSP_P_FLAG   ("save_button",  pzframe_gui_look_t, savebtn,      1, 0),

    PSP_P_MSTRING("subsys",       pzframe_gui_look_t, subsysname, NULL, 100),
    PSP_P_END()
};

//////////////////////////////////////////////////////////////////////

static void SetMiniToolButtonOnOff(Widget btn, int is_pushed)
{
  XtPointer  state;

    if (btn == NULL) return;

    XtVaGetValues(btn, XmNuserData, &state, NULL);
    if (ptr2lint(state) == is_pushed) return;

    XhInvertButton(ABSTRZE(btn));
    XtVaSetValues(btn, XmNuserData, lint2ptr(is_pushed), NULL);
}

static void ShowRunMode(pzframe_gui_t *gui)
{
    if (gui->loop_button != NULL)
        SetMiniToolButtonOnOff(gui->loop_button,  gui->pfr->cfg.running);
    if (gui->once_button != NULL  &&  0)
        SetMiniToolButtonOnOff(gui->once_button,  gui->pfr->cfg.oneshot);
    if (gui->once_button != NULL)
        XtSetSensitive        (gui->once_button, !gui->pfr->cfg.running);
}

static void SetRunMode (pzframe_gui_t *gui, int is_loop)
{
    PzframeDataSetRunMode(gui->pfr, is_loop, 0);
    ShowRunMode(gui);
}

//////////////////////////////////////////////////////////////////////

// GetCblistSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Cblist, pzframe_gui_cbinfo_t,
                                 cb, cb, NULL, (void*)1,
                                 0, 2, 0,
                                 gui->, gui,
                                 pzframe_gui_t *gui, pzframe_gui_t *gui)

void PzframeGuiCallCBs(pzframe_gui_t *gui,
                       int            reason,
                       int            info_changed)
{
  int  i;

    if (gui->d->vmt.evproc != NULL)
        gui->d->vmt.evproc(gui, reason, info_changed, NULL);

    for (i = 0;  i < gui->cb_list_allocd;  i++)
        if (gui->cb_list[i].cb != NULL)
            gui->cb_list[i].cb(gui, reason, 
                               info_changed, gui->cb_list[i].privptr);
}

//////////////////////////////////////////////////////////////////////

static void PzframeGuiEventProc(pzframe_data_t *pfr,
                                int             reason,
                                int             info_changed,
                                void           *privptr)
{
  pzframe_gui_t       *gui = (pzframe_gui_t *)privptr;
  pzframe_type_dscr_t *ftd = pfr->ftd;

  int                  pn;

  static char          roller_states[4] = "/-\\|";
  char                 buf[100];

  colalarm_t           newstate;

    if      (reason == PZFRAME_REASON_REG_CHANS)
    {
        for (pn = 0;  pn < ftd->num_params;  pn++)
            if (gui->k_regcns[pn] != NULL)
            {
                SetKnobValue(gui->k_regcns[pn],
                             pfr->chan_vals[pn]
                             / 
                             pfr->cfg.param_info[pn].phys_r
                             - 
                             pfr->cfg.param_info[pn].phys_d);
                SetKnobState(gui->k_regcns[pn], COLALARM_NONE);
            }
    }
    else if (reason == PZFRAME_REASON_BIGC_DATA)
    {  
        fps_frme(&(gui->fps_ctr));
        ShowRunMode(gui);

        newstate = datatree_choose_knobstate(NULL, pfr->mes.rflags);
        if (gui->curstate != newstate)
        {
            gui->curstate = newstate;
            if (gui->d->vmt.newstate != NULL)
                gui->d->vmt.newstate(gui);
        }

        PzframeGuiUpdate(gui, info_changed);

        /* Rotate the roller */
        gui->roller_ctr = (gui->roller_ctr + 1) & 3;
        sprintf(buf, "%c", roller_states[gui->roller_ctr]);
        if (gui->roller != NULL) XmTextSetString(gui->roller, buf);

        XhProcessPendingXEvents();
    }


    /* Call upstream */
    PzframeGuiCallCBs(gui, reason, info_changed);
}

///////////////////

void  PzframeGuiInit       (pzframe_gui_t *gui, pzframe_data_t *pfr,
                            pzframe_gui_dscr_t *gkd)
{
    bzero(gui, sizeof(*gui));
    gui->pfr = pfr;
    gui->d   = gkd;
    gui->curstate = (gkd->ftd->behaviour & PZFRAME_B_ARTIFICIAL) != 0 ?
        COLALARM_NONE : COLALARM_JUSTCREATED;
}

void  PzframeGuiRealize    (pzframe_gui_t *gui)
{

    PzframeDataAddEvproc(gui->pfr, PzframeGuiEventProc, gui);
    ShowRunMode   (gui);
}

void  PzframeGuiUpdate     (pzframe_gui_t *gui, int info_changed)
{
  pzframe_data_t      *pfr = gui->pfr;
  pzframe_type_dscr_t *ftd = pfr->ftd;

  int                  pn;

    /* Display current params */
    for (pn = 0;  pn < ftd->num_params;  pn++)
        if (gui->k_params[pn] != NULL)
        {
            SetKnobValue(gui->k_params[pn],
                         pfr->mes.info[pn]
                         / 
                         pfr->cfg.param_info[pn].phys_r
                         - 
                         pfr->cfg.param_info[pn].phys_d);
            SetKnobState(gui->k_params[pn], COLALARM_NONE);
        }

    /* Renew displayed data */
    if (gui->d->vmt.do_renew != NULL)
        gui->d->vmt.do_renew(gui, info_changed);
}

int   PzframeGuiAddEvproc  (pzframe_gui_t *gui,
                            pzframe_gui_evproc_t cb, void *privptr)
{
  int                   id;
  pzframe_gui_cbinfo_t *slot;

    if (cb == NULL) return 0;

    id = GetCblistSlot(gui);
    if (id < 0) return -1;
    slot = AccessCblistSlot(id, gui);

    slot->cb      = cb;
    slot->privptr = privptr;

    return 0;
}

//////////////////////////////////////////////////////////////////////

static void ShowStats  (pzframe_gui_t *gui)
{
  int             dfps = fps_dfps(&(gui->fps_ctr));
  char            buf[100];
  struct timeval  timenow;
  struct timeval  timediff;
  long int        secs;

    if      (!gui->pfr->cfg.running) buf[0] = '\0';
    else if (dfps < 100)    sprintf(buf, "%d.%d fps", dfps / 10, dfps % 10);
    else                    sprintf(buf, "%3d fps",   dfps / 10);
    XmTextSetString(gui->fps_show, buf);

    if      (!gui->pfr->cfg.running  ||  gui->pfr->timeupd.tv_sec == 0) buf[0] = '\0';
    else
    {
        gettimeofday(&timenow, NULL);
        timeval_subtract(&timediff, &timenow, &(gui->pfr->timeupd));
        if (timediff.tv_sec > 3600 * 99)
            sprintf(buf, "---");
        else
        {
            secs = timediff.tv_sec;
            if      (secs > 3600)
                sprintf(buf, " %ld:%02ld:%02ld",
                        secs / 3600, (secs / 60) % 60, secs % 60);
            else if (secs > 59)
                sprintf(buf, " %ld:%02ld",
                        secs / 60, secs % 60);
            else if (secs < 10)
                sprintf(buf, " %ld.%lds",
                        secs, timediff.tv_usec / 100000);
            else
                sprintf(buf, " %lds",
                        secs);
        }
    }
    XmTextSetString(gui->time_show, buf);
}

static void Fps10HzProc(XtPointer     closure,
                        XtIntervalId *id      __attribute__((unused)))
{
  pzframe_gui_t *gui = (pzframe_gui_t *)closure;

    XtAppAddTimeOut(xh_context, 1000, Fps10HzProc, gui);
    fps_beat(&(gui->fps_ctr));
    ShowStats(gui);
}

static void SaveBtnCB(Widget     w,
                      XtPointer  closure,
                      XtPointer  call_data __attribute__((unused)))
{
  pzframe_gui_t     *gui   = closure;
  char               type_sys[PATH_MAX];
  char               filename[PATH_MAX];

    if (gui->pfr->ftd->vmt.save == NULL) return;

    check_snprintf(type_sys, sizeof(type_sys), "%s%s%s",
                   gui->pfr->ftd->type_name,
                   gui->look.subsysname == NULL? "" : "-",
                   gui->look.subsysname == NULL? "" : gui->look.subsysname);
    check_snprintf(filename, sizeof(filename), CHL_DATA_PATTERN_FMT,
                   type_sys);
    XhFindFilename(filename, filename, sizeof(filename));

    if (gui->pfr->ftd->vmt.save(gui->pfr, filename, gui->look.subsysname, NULL) == 0)
//    if (FastadcDataSave(&(gui->a), filename, gui->look.subsysname, NULL) == 0)
        XhMakeMessage(XhWindowOf(ABSTRZE(w)), "Data saved to \"%s\"", filename);
    else
        XhMakeMessage(XhWindowOf(ABSTRZE(w)), "Error saving to file: %s", strerror(errno));
}

static void LoopBtnCB(Widget     w,
                      XtPointer  closure,
                      XtPointer  call_data __attribute__((unused)))
{
  pzframe_gui_t     *gui   = closure;

    SetRunMode(gui, !gui->pfr->cfg.running);
}

static void OnceBtnCB(Widget     w,
                      XtPointer  closure,
                      XtPointer  call_data __attribute__((unused)))
{
  pzframe_gui_t     *gui   = closure;

    PzframeDataSetRunMode(gui->pfr, -1, 1);
    ShowRunMode(gui);
}

void  PzframeGuiMkCommons  (pzframe_gui_t *gui, Widget parent)
{
  Widget     w;
  XmString   s;

    if (gui->commons != NULL) return;

    gui->commons = XtVaCreateManagedWidget("form", xmFormWidgetClass, parent,
                                           XmNshadowThickness, 0,
                                           NULL);

    w = NULL;

    if (gui->look.noleds == 0)
    {
        gui->local_leds = XtVaCreateManagedWidget("alarmLeds", xmFormWidgetClass, gui->commons,
                                                  XmNtraversalOn, False,
                                                  NULL);
        attachleft(gui->local_leds, w, NULL);

        w = gui->local_leds;
    }

    gui->roller = XtVaCreateManagedWidget("text_o", xmTextWidgetClass, gui->commons,
                                          XmNvalue, ".",
                                          XmNcursorPositionVisible, False,
                                          XmNcolumns,               1,
                                          XmNscrollHorizontal,      False,
                                          XmNeditMode,              XmSINGLE_LINE_EDIT,
                                          XmNeditable,              False,
                                          XmNtraversalOn,           False,
                                          NULL);
    attachleft(gui->roller, w, 0);
    w = gui->roller;

    fps_init(&(gui->fps_ctr));
    gui->fps_show = XtVaCreateManagedWidget("text_o", xmTextWidgetClass, gui->commons,
                                            XmNvalue, "",
                                            XmNcursorPositionVisible, False,
                                            XmNcolumns,               strlen("999fps "),
                                            XmNscrollHorizontal,      False,
                                            XmNeditMode,              XmSINGLE_LINE_EDIT,
                                            XmNeditable,              False,
                                            XmNtraversalOn,           False,
                                            NULL);
    attachleft(gui->fps_show, w, CHL_INTERKNOB_H_SPACING);
    w = gui->fps_show;

    gui->time_show = XtVaCreateManagedWidget("text_o", xmTextWidgetClass, gui->commons,
                                             XmNvalue, "",
                                             XmNcursorPositionVisible, False,
                                             XmNcolumns,               strlen(" hh:mm:ss"),
                                             XmNscrollHorizontal,      False,
                                             XmNeditMode,              XmSINGLE_LINE_EDIT,
                                             XmNeditable,              False,
                                             XmNtraversalOn,           False,
                                             NULL);
    attachleft(gui->time_show, w, CHL_INTERKNOB_H_SPACING);
    w = gui->time_show;

    if (gui->look.savebtn)
    {
        gui->save_button =
            XtVaCreateManagedWidget("miniToolButton", xmPushButtonWidgetClass, gui->commons,
                                    XmNlabelString, s = XmStringCreateLtoR(">F", xh_charset),
                                    XmNtraversalOn, False,
                                    NULL);
        XmStringFree(s);
        XhAssignPixmap(gui->save_button, btn_mini_save_xpm);
        XhSetBalloon  (gui->save_button, "Save data to file");
        XtAddCallback (gui->save_button, XmNactivateCallback, SaveBtnCB, gui);
        attachleft    (gui->save_button, w, 0);
        w = gui->save_button;
    }
    if (gui->look.noleds == 0)
    {
        gui->loop_button =
            XtVaCreateManagedWidget("miniToolButton", xmPushButtonWidgetClass, gui->commons,
                                    XmNlabelString, s = XmStringCreateLtoR(">", xh_charset),
                                    XmNtraversalOn, False,
                                    NULL);
        XmStringFree(s);
        XhAssignPixmap(gui->loop_button, btn_mini_start_xpm);
        XhSetBalloon  (gui->loop_button, "Periodic measurement");
        XtAddCallback (gui->loop_button, XmNactivateCallback, LoopBtnCB, gui);
        attachleft    (gui->loop_button, w, 0);
        w = gui->loop_button;

        gui->once_button =
            XtVaCreateManagedWidget("miniToolButton", xmPushButtonWidgetClass, gui->commons,
                                    XmNlabelString, s = XmStringCreateLtoR("|>", xh_charset),
                                    XmNtraversalOn, False,
                                    NULL);
        XmStringFree(s);
        XhAssignPixmap(gui->once_button, btn_mini_once_xpm);
        XhSetBalloon  (gui->once_button, "One measurement");
        XtAddCallback (gui->once_button, XmNactivateCallback, OnceBtnCB, gui);
        attachleft    (gui->once_button, w, 0);
        w = gui->once_button;
    }

    XtAppAddTimeOut(xh_context, 1000, Fps10HzProc, gui);
}

static void UpdateLeds(pzframe_gui_t *gui)
{
    leds_update(gui->leds, gui->pfr->bigc_srvcount, gui->pfr->bigc_sid);
}

static void LedsKeepaliveProc(XtPointer     closure,
                              XtIntervalId *id      __attribute__((unused)))
{
  pzframe_gui_t *gui = closure;

    XtAppAddTimeOut(xh_context, 1000, LedsKeepaliveProc, gui);
    UpdateLeds(gui);
}

void  PzframeGuiMkLeds     (pzframe_gui_t *gui, Widget parent, int in_toolbar)
{
  XhWidget  leds_grid;
  int       leds_size;

    leds_size = in_toolbar? 20 : -15;
    if (parent != NULL)
    {
        leds_grid = XhCreateGrid(ABSTRZE(parent), "alarmLeds");
        attachtop    (leds_grid, NULL, 0);
        attachbottom (leds_grid, NULL, 0);
        XhGridSetGrid(leds_grid, 0, 0);
        if ((gui->leds = XtCalloc(gui->pfr->bigc_srvcount, sizeof(*(gui->leds)))) != NULL)
            leds_create(leds_grid, leds_size, gui->leds, gui->pfr->bigc_srvcount, gui->pfr->bigc_sid);
        XtAppAddTimeOut(xh_context, 1000, LedsKeepaliveProc, gui);
    }
}

static void ParamKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  pzframe_gui_dpl_t *pp    = privptr;
  pzframe_gui_t     *gui   = pp->p;
  int                pn    = pp->n;
  int32              value = round(v + gui->pfr->cfg.param_info[pn].phys_d)
                             *
                                       gui->pfr->cfg.param_info[pn].phys_r;


    if (gui->pfr->cfg.readonly) return;

    cda_setbigcparams (gui->pfr->bigc_handle, pn, 1, &value);

    ////fprintf(stderr, "Param #%d:=%d\n", param, value);
}

static void RgParKCB(Knob k __attribute__((unused)), double v, void *privptr)
{
  pzframe_gui_dpl_t *pp    = privptr;
  pzframe_gui_t     *gui   = pp->p;
  int                pn    = pp->n;
  int32              value = round(v + gui->pfr->cfg.param_info[pn].phys_d)
                             *
                                       gui->pfr->cfg.param_info[pn].phys_r;

    if (gui->pfr->cfg.readonly) return;

    cda_setphyschanraw(gui->pfr->chan_handles[pn],    value);
    cda_setbigcparams (gui->pfr->bigc_handle, pn, 1, &value);

    ////fprintf(stderr, "RgPar #%d:=%d\n", param, value);
}

Widget PzframeGuiMkparknob(pzframe_gui_t *gui,                            
                           Widget parent, const char *spec, int pn)
{
  pzframe_type_dscr_t *ftd = gui->pfr->ftd;
  int                  options = gui->pfr->cfg.readonly? SIMPLEKNOB_OPT_READONLY
                                                       : 0;

  pzframe_gui_dpl_t   *prmp;
  int                  param_type = PZFRAME_PARAM_BIGC;
  int                  rw_only;
  int                  x;

    if (ftd->param_dscrs != NULL) /*!!! Why not use gui->pfr->cfg.param_info[pn].type? */
        for (x = 0;
             ftd->param_dscrs[x].name != NULL;
             x++)
            if (ftd->param_dscrs[x].n == pn)
            {
                param_type = ftd->param_dscrs[x].param_type;
                break;
            }
    rw_only = ((param_type & PZFRAME_PARAM_RW_ONLY_MASK) != 0);
    param_type &=~ PZFRAME_PARAM_RW_ONLY_MASK;

    prmp = gui->Param_prm + pn;
    prmp->p = gui;
    prmp->n = pn;

    if      ((param_type == PZFRAME_PARAM_CHAN_ONLY  &&
             gui->pfr->chan_sid == CDA_SERVERID_ERROR)
             ||
             (rw_only  &&  gui->pfr->cfg.readonly))
    {
        return XtVaCreateManagedWidget("", widgetClass, parent,
                                       XmNwidth,       1,
                                       XmNheight,      1,
                                       XmNborderWidth, 0,
                                       NULL);
    }
    else if (param_type != PZFRAME_PARAM_BIGC  &&
             gui->pfr->chan_sid != CDA_SERVERID_ERROR)
    {
        gui->k_regcns[pn] = CreateSimpleKnob(spec, options, ABSTRZE(parent), RgParKCB, prmp);
        SetKnobState(gui->k_regcns[pn], COLALARM_JUSTCREATED);
        return CNCRTZE(GetKnobWidget(gui->k_regcns[pn]));
    }
    else
    {
        gui->k_params[pn] = CreateSimpleKnob(spec, options, ABSTRZE(parent), ParamKCB, prmp);
        if (!gui->pfr->cfg.readonly  &&  gui->pfr->cfg.param_iv.p[pn] >= 0)
                SetKnobValue(gui->k_params[pn], gui->pfr->cfg.param_iv.p[pn]);
        else    SetKnobState(gui->k_params[pn], COLALARM_JUSTCREATED);
        return CNCRTZE(GetKnobWidget(gui->k_params[pn]));
    }
}
