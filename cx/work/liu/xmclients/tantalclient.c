#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#include <math.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>

#include "misc_macros.h"
#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Xh.h"
#include "Xh_fallbacks.h"
#include "Chl.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "tantalclient.h"


#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_help.xpm"
#include "pixmaps/btn_quit.xpm"


static XFontStruct     *last_finfo;

static GC AllocXhGC(Widget w, int idx, const char *fontspec)
{
  XtGCMask   mask;  
  XGCValues  vals;
    
    mask = GCForeground | GCLineWidth;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 0;

    if (fontspec != NULL  &&  fontspec[0] != '\0')
    {
        last_finfo = XLoadQueryFont(XtDisplay(w), fontspec);
        if (last_finfo == NULL)
        {
            fprintf(stderr, "Unable to load font \"%s\"; using \"fixed\"\n", fontspec);
            last_finfo = XLoadQueryFont(XtDisplay(w), "fixed");
        }
        vals.font = last_finfo->fid;

        mask |= GCFont;
    }

    return XtGetGC(w, mask, &vals);
}

static GC AllocDashGC(Widget w, int idx, int dash_length)
{
  XtGCMask   mask;  
  XGCValues  vals;
    
    mask = GCForeground | GCLineWidth | GCLineStyle | GCDashList;
    vals.foreground = XhGetColor(idx);
    vals.line_width = 0;
    vals.line_style = LineOnOffDash;
    vals.dashes     = dash_length;

    return XtGetGC(w, mask, &vals);
}


//////////////////////////////////////////////////////////////////////

enum
{
    POINT_KIND_UP = 0,
    POINT_KIND_DN = 1,
    POINT_KIND_MA = 2,
    POINT_KIND_XX = 3,
};

typedef struct
{
    Widget  form;
    Widget  plot;
    Knob    k_xmin;
    Knob    k_xmax;
    Knob    k_ymin;
    Knob    k_ymax;
    Knob    k_clear;
    GC      bkgdGC;
    GC      gridGC;
    GC      dataGC[4];
    XFontStruct *data_finfo;

    double  v_xmin;
    double  v_xmax;
    double  v_ymin;
    double  v_ymax;
    
    double  from_i;
    double  to_i;
    int     step_time;
    int     nsteps;

    int     running;
    int     curstep;
    int     cursecs;
    time_t  glob_start;
    time_t  step_start;

    int     plot_used;
    int     plot_k[TANTAL_MAXPLOTSIZE];
    double  plot_x[TANTAL_MAXPLOTSIZE];
    double  plot_y[TANTAL_MAXPLOTSIZE];
} TANTAL_PLOT_privaterec_t;


#define DEF_PLOT_X_MIN 0.0
#define DEF_PLOT_X_MAX 100.0
#define DEF_PLOT_Y_MIN 0.0
#define DEF_PLOT_Y_MAX 10.0
#define MIN_X_RANGE    0.001
#define MIN_Y_RANGE    0.001
enum {POINTSIZE=1};

TANTAL_PLOT_privaterec_t *the_plot = NULL;

static void DrawPlot(TANTAL_PLOT_privaterec_t *me, int do_clear)
{
  Display   *dpy = XtDisplay(me->plot);
  Window     win = XtWindow (me->plot);
  Dimension  plot_w, plot_h;
  int        this_x, this_y;
  int        prev_x, prev_y;
  int        n;
  double     v;
  
    XtVaGetValues(me->plot,
                  XmNwidth,  &plot_w,
                  XmNheight, &plot_h,
                  NULL);
  
    if (do_clear)
        XFillRectangle(dpy, win, me->bkgdGC,
                       0, 0, plot_w, plot_h);

    /* Grid */
    for (v = DEF_PLOT_X_MIN;  v <= DEF_PLOT_X_MAX;  v += (DEF_PLOT_X_MAX-DEF_PLOT_X_MIN)/10)
    {
        this_x = RESCALE_VALUE(v, me->v_xmin, me->v_xmax, 0, plot_w-1);
        XDrawLine(dpy, win, me->gridGC, this_x, 0, this_x, plot_h-1);
    }
    for (v = DEF_PLOT_Y_MIN;  v <= DEF_PLOT_Y_MAX;  v += (DEF_PLOT_Y_MAX-DEF_PLOT_Y_MIN)/10)
    {
        this_y = RESCALE_VALUE(v, me->v_ymin, me->v_ymax, 0, plot_h-1);
        XDrawLine(dpy, win, me->gridGC, 0, this_y, plot_w-1, this_y);
    }
    
    /* Data */
    for (n = 0;  n < me->plot_used;  n++)
    {
        this_x = RESCALE_VALUE(me->plot_x[n], me->v_xmin, me->v_xmax, 0, plot_w-1);
        this_y = RESCALE_VALUE(me->plot_y[n], me->v_ymin, me->v_ymax, plot_h-1, 0);

        if (n > 0) XDrawLine(dpy, win, me->dataGC[me->plot_k[n]&3], prev_x, prev_y, this_x, this_y);
        XFillRectangle(dpy, win, me->dataGC[me->plot_k[n]&3],
                       this_x - POINTSIZE, this_y - POINTSIZE,
                       POINTSIZE*2+1, POINTSIZE*2+1);
        
        prev_x = this_x;
        prev_y = this_y;
    }
}

static void dataExposeCB(Widget     w,
                         XtPointer  closure,
                         XtPointer  call_data  __attribute__((unused)))
{
  knobinfo_t               *ki = (knobinfo_t *)closure;
  TANTAL_PLOT_privaterec_t *me = (TANTAL_PLOT_privaterec_t*)ki->widget_private;

    DrawPlot(me, False);
}

static void ClearPlot     (TANTAL_PLOT_privaterec_t *me)
{
    me->plot_used = 0;
    DrawPlot(me, 1);
}

static void AddPointToPlot(TANTAL_PLOT_privaterec_t *me, int k, double x, double y)
{
    if (me->plot_used < TANTAL_MAXPLOTSIZE)
    {
        me->plot_k[me->plot_used] = k;
        me->plot_x[me->plot_used] = x;
        me->plot_y[me->plot_used] = y;
        me->plot_used++;

        DrawPlot(me, 0);
    }
}

static void UpdateRegs(XhWindow  win, TANTAL_PLOT_privaterec_t *me)
{
    ChlSetLclReg(win, REG_TANTAL_CUR_STEP, me->curstep);
    ChlSetLclReg(win, REG_TANTAL_CUR_TIME, me->cursecs);
    ChlSetLclReg(win, REG_TANTAL_C_GO,     me->running);
}

static void DoStop    (XhWindow  win, TANTAL_PLOT_privaterec_t *me)
{
    me->running = 0;
    me->curstep = me->cursecs = 0;
    UpdateRegs(win, me);
}


static void XminKCB(Knob k, double v, void *privptr)
{
  TANTAL_PLOT_privaterec_t *me = privptr;
  
  if (v > me->v_xmax - MIN_X_RANGE)
      SetKnobValue(k, v = me->v_xmax - MIN_X_RANGE);
  if (me->v_xmin != v)
  {
      me->v_xmin = v;
      DrawPlot(me, True);
  }
}

static void XmaxKCB(Knob k, double v, void *privptr)
{
  TANTAL_PLOT_privaterec_t *me = privptr;
  
  if (v < me->v_xmin + MIN_X_RANGE)
      SetKnobValue(k, v = me->v_xmin + MIN_X_RANGE);
  if (me->v_xmax != v)
  {
      me->v_xmax = v;
      DrawPlot(me, True);
  }
}

static void YminKCB(Knob k, double v, void *privptr)
{
  TANTAL_PLOT_privaterec_t *me = privptr;
  
  if (v > me->v_ymax - MIN_Y_RANGE)
      SetKnobValue(k, v = me->v_ymax - MIN_X_RANGE);
  if (me->v_ymin != v)
  {
      me->v_ymin = v;
      DrawPlot(me, True);
  }
}

static void YmaxKCB(Knob k, double v, void *privptr)
{
  TANTAL_PLOT_privaterec_t *me = privptr;
  
  if (v < me->v_ymin + MIN_Y_RANGE)
      SetKnobValue(k, v = me->v_ymin + MIN_Y_RANGE);
  if (me->v_ymax != v)
  {
      me->v_ymax = v;
      DrawPlot(me, True);
  }
}

static void ClearKCB(Knob k, double v, void *privptr)
{
  TANTAL_PLOT_privaterec_t *me = privptr;
  
    ClearPlot(me);
}

static void SetPointKCB(Knob k, double v, void *privptr)
{
  TANTAL_PLOT_privaterec_t *me  = privptr;
  XhWindow                  win = XhWindowOf(me->plot);
  double                    p_x;
  double                    p_y;
  
    ChlGetChanVal(win, ".mes2", &p_x);
    ChlGetChanVal(win, ".mes5", &p_y);
    AddPointToPlot(me,
                   POINT_KIND_MA,
                   p_x, p_y);
}

static XhWidget TANTAL_PLOT_Create_m(knobinfo_t *ki, XhWidget parent)
{
  TANTAL_PLOT_privaterec_t *me = (TANTAL_PLOT_privaterec_t*)ki->widget_private;
  char                      spec[1000];
  Dimension                 w_w;

    if ((me = ki->widget_private = XtMalloc(sizeof(*me))) == NULL)
        return NULL;
    bzero(me, sizeof(*me));

    me->v_xmin = DEF_PLOT_X_MIN;
    me->v_xmax = DEF_PLOT_X_MAX;
    me->v_ymin = DEF_PLOT_Y_MIN;
    me->v_ymax = DEF_PLOT_Y_MAX;

    me->form = XtVaCreateManagedWidget("ctl", xmFormWidgetClass, CNCRTZE(parent),
                                       XmNshadowThickness, 0,
                                       NULL);
    
    me->plot = XtVaCreateManagedWidget("tantal_plot",
                                       xmDrawingAreaWidgetClass,
                                       me->form,
                                       XmNtraversalOn, False,
                                       XmNbackground,  XhGetColor(XH_COLOR_GRAPH_BG),
                                       XmNwidth,       600,
                                       XmNheight,      400,
                                       NULL);

    me->bkgdGC    = AllocXhGC  (me->plot, XH_COLOR_GRAPH_BG,    NULL);
    me->gridGC    = AllocDashGC(me->plot, XH_COLOR_GRAPH_GRID,  1);
    me->dataGC[0] = AllocXhGC  (me->plot, XH_COLOR_GRAPH_LINE2, XH_TINY_FIXED_FONT);
    me->dataGC[1] = AllocXhGC  (me->plot, XH_COLOR_GRAPH_LINE1, XH_TINY_FIXED_FONT);
    me->dataGC[2] = AllocXhGC  (me->plot, XH_COLOR_GRAPH_LINE3, XH_TINY_FIXED_FONT);
    me->dataGC[3] = AllocXhGC  (me->plot, XH_COLOR_GRAPH_LINE4, XH_TINY_FIXED_FONT);

    XtAddCallback(me->plot, XmNexposeCallback, dataExposeCB, (XtPointer)ki);

    check_snprintf(spec, sizeof(spec),
                   " rw local look=text label='Xmin'"
                   " dpyfmt='%s'"
                   " minnorm='"TANTAL_I_FMT"' maxnorm='"TANTAL_I_FMT"'"
                   " widdepinfo=''",
                   TANTAL_I_FMT,
                   DEF_PLOT_X_MIN, DEF_PLOT_X_MAX);
    me->k_xmin = CreateSimpleKnob(spec, 0, ABSTRZE(me->form), XminKCB, me);
    SetKnobValue(me->k_xmin, me->v_xmin);
    XtVaSetValues(GetKnobWidget(me->k_xmin),
                  XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
                  XmNleftWidget,       me->plot,
                  XmNtopAttachment,    XmATTACH_WIDGET,
                  XmNtopWidget,        me->plot,
                  NULL);
    
    check_snprintf(spec, sizeof(spec),
                   " rw local look=text label='Xmax'"
                   " dpyfmt='%s'"
                   " minnorm='"TANTAL_I_FMT"' maxnorm='"TANTAL_I_FMT"'"
                   " widdepinfo=''",
                   TANTAL_I_FMT,
                   DEF_PLOT_X_MIN, DEF_PLOT_X_MAX);
    me->k_xmax = CreateSimpleKnob(spec, 0, ABSTRZE(me->form), XmaxKCB, me);
    SetKnobValue(me->k_xmax, me->v_xmax);
    XtVaSetValues(GetKnobWidget(me->k_xmax),
                  XmNrightAttachment,  XmATTACH_OPPOSITE_WIDGET,
                  XmNrightWidget,      me->plot,
                  XmNtopAttachment,    XmATTACH_WIDGET,
                  XmNtopWidget,        me->plot,
                  NULL);
    
    check_snprintf(spec, sizeof(spec),
                   " rw local look=text label='Ymin'"
                   " dpyfmt='%s'"
                   " minnorm='"TANTAL_I_FMT"' maxnorm='"TANTAL_I_FMT"'"
                   " widdepinfo=''",
                   TANTAL_I_FMT,
                   DEF_PLOT_Y_MIN, DEF_PLOT_Y_MAX);
    me->k_ymin = CreateSimpleKnob(spec, 0, ABSTRZE(me->form), YminKCB, me);
    SetKnobValue(me->k_ymin, me->v_ymin);
    XtVaSetValues(GetKnobWidget(me->k_ymin),
                  XmNleftAttachment,   XmATTACH_WIDGET,
                  XmNleftWidget,       me->plot,
                  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                  XmNbottomWidget,     me->plot,
                  NULL);

    check_snprintf(spec, sizeof(spec),
                   " rw local look=text label='Ymax'"
                   " dpyfmt='%s'"
                   " minnorm='"TANTAL_I_FMT"' maxnorm='"TANTAL_I_FMT"'"
                   " widdepinfo=''",
                   TANTAL_I_FMT,
                   DEF_PLOT_Y_MIN, DEF_PLOT_Y_MAX);
    me->k_ymax = CreateSimpleKnob(spec, 0, ABSTRZE(me->form), YmaxKCB, me);
    SetKnobValue(me->k_ymax, me->v_ymax);
    XtVaSetValues(GetKnobWidget(me->k_ymax),
                  XmNleftAttachment,   XmATTACH_WIDGET,
                  XmNleftWidget,       me->plot,
                  XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
                  XmNtopWidget,        me->plot,
                  NULL);

    check_snprintf(spec, sizeof(spec),
                   " rw local look=button label='Забыть все точки'");
    me->k_clear = CreateSimpleKnob(spec, 0, ABSTRZE(me->form), ClearKCB, me);
    XtVaGetValues(GetKnobWidget(me->k_clear), XmNwidth, &w_w, NULL);
    XtVaSetValues(GetKnobWidget(me->k_clear),
                  XmNrightAttachment,  XmATTACH_POSITION,
                  XmNrightPosition,    50,
                  XmNrightOffset,      -w_w / 2,
                  XmNtopAttachment,    XmATTACH_WIDGET,
                  XmNtopWidget,        me->plot,
                  NULL);
    
    check_snprintf(spec, sizeof(spec),
                   " rw local look=button label='Пост.\nточку'");
    me->k_clear = CreateSimpleKnob(spec, 0, ABSTRZE(me->form), SetPointKCB, me);
    XtVaSetValues(GetKnobWidget(me->k_clear),
                  XmNleftAttachment,   XmATTACH_WIDGET,
                  XmNleftWidget,       me->plot,
                  XmNbottomAttachment, XmATTACH_POSITION,
                  XmNbottomPosition,   50,
                  NULL);
    
    the_plot = me;
    
    return ABSTRZE(me->form);
}

static void TANTAL_PLOT_SetValue_m(knobinfo_t *ki, double v __attribute__((unused)))
{
  TANTAL_PLOT_privaterec_t *me       = (TANTAL_PLOT_privaterec_t*)ki->widget_private;
  time_t                    timenow  = time(NULL);
  XhWindow                  win      = XhWindowOf(ki->indicator);
  double                    reg_go;
  double                    reg_stop;
  double                    dv;
  double                    p_x;
  double                    p_y;
  
    ChlGetLclReg(win, REG_TANTAL_C_GO,   &reg_go);
    ChlGetLclReg(win, REG_TANTAL_C_STOP, &reg_stop);
    ChlSetLclReg(win, REG_TANTAL_C_STOP, 0);
    if (me->running)
    {
        if (reg_stop != 0)
        {
            DoStop(win, me);
            return;
        }
    }
    else
    {
        if (reg_go == 0) return;
        ChlGetLclReg(win, REG_TANTAL_FROM_I,    &(me->from_i));
        ChlGetLclReg(win, REG_TANTAL_TO_I,      &(me->to_i));
        ChlGetLclReg(win, REG_TANTAL_STEP_TIME, &dv); me->step_time = dv;
        ChlGetLclReg(win, REG_TANTAL_NSTEPS,    &dv); me->nsteps    = dv;
        
        me->running    = 1;
        me->glob_start = timenow;
        me->curstep    = -1;
    }
    
    // Should go to next step?
    if (me->curstep < 0  ||  difftime(timenow, me->step_start) >= me->step_time)
    {
        if (me->curstep >= 0)
        {
            // Record data for plot
            ChlGetChanVal(win, ".mes2", &p_x);
            ChlGetChanVal(win, ".mes5", &p_y);
            AddPointToPlot(me,
                           me->from_i < me->to_i? POINT_KIND_UP : POINT_KIND_DN,
                           p_x, p_y);
        }
        else
            /*ClearPlot(me)*/;

        me->curstep++;
        me->step_start = timenow;

        if (me->curstep <= me->nsteps)
        {
            dv = me->from_i + (me->to_i - me->from_i) / me->nsteps * me->curstep;
            ChlSetChanVal(win, ".iset", dv);
        }
        else
        {
            DoStop(win, me);
        }
    }
    me->cursecs = difftime(timenow, me->step_start);

    UpdateRegs(win, me);
}

static knobs_vmt_t TANTAL_PLOT_VMT   = {TANTAL_PLOT_Create_m, NULL, TANTAL_PLOT_SetValue_m, NULL, NULL};

static knobs_widgetinfo_t tantal_widgetinfo[] =
{
    {LOGD_TANTAL_PLOT,   "tantal_plot", &TANTAL_PLOT_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  tantal_widgetset   = {tantal_widgetinfo, NULL};

//////////////////////////////////////////////////////////////////////

static const char *sign_lp_s    = "# Data tantal";
static const char *subsys_lp_s  = "#!SUBSYSTEM:";
static const char *crtime_lp_s  = "#!CREATE-TIME:";
static const char *comment_lp_s = "#!COMMENT:";

static int TantalSaveData(const char *sysname, const char *filespec, const char *comment)
{
  FILE        *fp;
  time_t       timenow = time(NULL);
  const char  *cp;
  int          i;

    if (the_plot == NULL) return -1;
  
    if ((fp = fopen(filespec, "w")) == NULL) return -1;

    fprintf(fp, "%s\n", sign_lp_s);
    fprintf(fp, "%s tantal,%s\n", subsys_lp_s, sysname);
    fprintf(fp, "%s %lu %s", crtime_lp_s, (unsigned long)(timenow), ctime(&timenow)); /*!: No '\n' because ctime includes a trailing one. */
    fprintf(fp, "%s ", comment_lp_s);
    if (comment != NULL)
        for (cp = comment;  *cp != '\0';  cp++)
            fprintf(fp, "%c", !iscntrl(*cp)? *cp : ' ');
    fprintf(fp, "\n");

    for (i = 0;  i < the_plot->plot_used;  i++)
        fprintf(fp, "%4d %6.3f %6.3f\n", i, the_plot->plot_x[i], the_plot->plot_y[i]);
    
    fclose(fp);

    return 0;
}


static actdescr_t toolslist[] =
{
    XhXXX_TOOLCMD(cmChlNfSaveMode, "Сохранить режим",    btn_save_xpm),
    XhXXX_TOOLLEDS(),
    XhXXX_TOOLCMD(cmChlHelp,       "Краткая справка",    btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD(cmChlQuit,       "Выйти из программы", btn_quit_xpm),
    XhXXX_END()
};

static XhWidget save_nfndlg = NULL;

static void CommandProc(XhWindow win, int cmd)
{
  char          filename[PATH_MAX];
  char          comment[400];
  
    switch (cmd)
    {
        case cmChlNfSaveMode:
            if (save_nfndlg != NULL) XhSaveDlgHide(save_nfndlg);
            if (save_nfndlg == NULL)
                save_nfndlg = XhCreateSaveDlg(win, "saveDataDialog", NULL, NULL, cmChlNfSaveOk);
            XhSaveDlgShow(save_nfndlg);
            break;
            
        case cmChlNfSaveOk:
        case cmChlNfSaveCancel:
            XhSaveDlgHide(save_nfndlg);
            if (cmd == cmChlNfSaveOk)
            {
                XhSaveDlgGetComment(save_nfndlg, comment, sizeof(comment));
                check_snprintf(filename, sizeof(filename), CHL_DATA_PATTERN_FMT, ChlGetSysname(win));
                XhFindFilename(filename, filename, sizeof(filename));
                if (TantalSaveData(ChlGetSysname(win), filename, comment) == 0)
                    XhMakeMessage(win, "Data saved to \"%s\"", filename);
                else
                    XhMakeMessage(win, "Error saving to file: %s", strerror(errno));
            }
            break;
            
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


static void lclregchg_proc(int lreg_n, double v)
{
    //fprintf(stderr, "reg#%d:=%8.3f\n", lreg_n, v);
}

int main(int argc, char *argv[])
{
  int  r;

    cda_set_lclregchg_cb(lclregchg_proc);
    KnobsRegisterWidgetset(&tantal_widgetset);
    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             toolslist, CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
