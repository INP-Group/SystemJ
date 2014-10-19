#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "Xh.h"
#include "Xh_globals.h"
#include "Xh_fallbacks.h"
#include "Knobs.h"
#include "cxlib.h"
#include "misc_macros.h"
#include "misclib.h"

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>

#include "Gridbox.h"

#include "tsycamlib.h"
#include "gray2image.h"

#include "pixmaps/btn_start.xpm"
#include "pixmaps/btn_once.xpm"


/*********************************************************************
*  Variables section                                                 *
*********************************************************************/

enum {PIX_W = 660, PIX_H = 500};

enum
{
    TSYCAM_PARAM_W    = 0,
    TSYCAM_PARAM_H    = 1,
    TSYCAM_PARAM_K    = 2,
    TSYCAM_PARAM_T    = 3,
    TSYCAM_PARAM_SYNC = 4,
    TSYCAM_PARAM_MISS = 5,

    NUM_TSYCAM_PARAMS = 6
};

enum {UNDEFECT_CYCLE = 10};

#define USE16

#ifdef USE16
typedef uint16 px_t;
#else
typedef uint32 px_t;
#endif
enum
{
    SRC_BITS = 10,
    EXT_BITS = 10,
    MAXSRC   = (px_t)((1 << SRC_BITS) - 1),
    MAXEXT   = (px_t)((1 << EXT_BITS) - 1)
};

static const char *app_name  = "tsycam";
static const char *app_class = "TsyCam";

static XhWindow    onewin;
static Widget      frame;
static Widget      drawing_area;
static Widget      w_loop, w_once;
static Knob        k_K, k_T, k_SYNC, k_RRQ, k_MEM, k_undefect, k_normalize;

enum  // Copy from ChlI.h
{
    INTERELEM_H_SPACING = 10,
    INTERELEM_V_SPACING = 10,
    INTERKNOB_H_SPACING = 2,
    INTERKNOB_V_SPACING = 2,
};

static GC          gc;

static XImage          *img;
gray2image_converter_t  converter;
void                   *cvt_info;

static Bool        double_buffering = False;

static px_t  curdata[PIX_H][PIX_W];
static px_t  this_min, this_max;
static int   this_sum;

static int  *badpixels         = NULL;
static int   badpixels_allocd  = 0;
static int   badpixels_count   = 0;

void   *ref;
int     params_changed;
int     param_K    = 128;
int     param_T    = 200;
int     param_SYNC = 1;
int     param_RRQ  = 0;
int     param_MEM  = 0;

Bool    loop_get  = False;
Bool    undefect  = True;
Bool    normalize = True;
time_t  last_find_time = 0;

Bool          pulling    = False;
XtSignalId    signal_id  = 0;
XtIntervalId  timeout_id = 0;
XtInputId     input_id   = 0;

Bool          using_cx   = False;
int           conn       = -1;
int           bigc       = -1;
int32         retinfo[CX_MAX_BIGC_PARAMS];


/*********************************************************************
*  Picture management section                                        *
*********************************************************************/

static void FindMinMax(px_t *min_p, px_t *max_p, int *sum_p)
{
  register px_t *p;
  register px_t  v;
  register px_t  curmin, curmax;
  register int   ctr;
  int            sum;
  
    for (p = &(curdata[0][0]), ctr = PIX_W * PIX_H, curmin = MAXSRC, curmax = 0, sum = 0;
         ctr != 0;
         ctr--)
    {
        v = *p++;
        sum += v;
        if (v > curmax) curmax = v;
        if (v < curmin) curmin = v;
    }
    
    *min_p = curmin;
    *max_p = curmax;
    *sum_p = sum;
}

static int NormalizeCurdata(void)
{
  register px_t *p;
  px_t           curmin, curmax;
  register int   ctr;
  px_t           range;
  px_t           table[MAXSRC + 1];

    curmin = this_min;
    curmax = this_max;
//    curmin = 0; curmax = 1023;
    
    range = curmax - curmin;

#define DETECT_ERRORS
#ifdef DETECT_ERRORS
    if (range == 0)
    {
        bzero(curdata, sizeof(curdata));
        return 1;
    }
#else
    curmin--;
    range++;
#endif

    if (curmin > 0  ||  curmax < MAXSRC)
    {
        for (ctr = curmin, p = &(table[curmin]);  ctr <= (int)curmax;  ctr++)
            *p++ = (ctr - curmin) * MAXEXT / range;

#if defined(CPU_X86)  &&  defined(MAY_USE_ASM)
        __asm__ __volatile__
            (
             "   cld\n"
             "   movl %%esi, %%edi"           // ESI and EDI point to same address
             "   xorl %eax, %eax\n"           // Clear EAX's hi-word
             "1: lodsw\n"
             "   movw (%%ebx,%%eax), %%ax\n"  // AX:=table[AX]
             "   stosw\n"
             "   loop 1\n"
             : /* No output */
             : "S"(&curdata), "D"(&curdata), "b"(&table), "c"(PIX_W * PIX_H)
             : "%eax"
            );
#else
        for (p = &(curdata[0][0]), ctr = PIX_W * PIX_H;
             ctr != 0;
             ctr--)
            *p++ = table[*p];
#endif
    }
    
    return 0;
}

static inline int square(int x){return x*x;}

static void FindBadPixels(void)
{
  int  x, y;
  int  B;
  int  sigma2;
    
    badpixels_count = 0;
    for (y = 1;  y < PIX_H - 1; y++)
        for (x = 1;  x < PIX_W - 1;  x ++)
        {
            B = (curdata[y-1][x-1] + curdata[y-1][x] + curdata[y-1][x+1] +
                 curdata[y]  [x-1] +                   curdata[y]  [x+1] +
                 curdata[y+1][x-1] + curdata[y+1][x] + curdata[y+1][x+1]) / 8;
            sigma2 = (square(curdata[y-1][x-1]-B) + square(curdata[y-1][x]-B) + square(curdata[y-1][x+1]-B) +
                      square(curdata[y]  [x-1]-B) +                             square(curdata[y]  [x+1]-B) +
                      square(curdata[y+1][x-1]-B) + square(curdata[y+1][x]-B) + square(curdata[y+1][x+1]-B)) / 8;
            if (square(curdata[y][x] - B) > 9 * sigma2)
            {
                //curdata[y][x] = B;
                if (badpixels_allocd == 0  ||  badpixels_count >= badpixels_allocd)
                {
                    badpixels_allocd += 100;
                    badpixels = XtRealloc(badpixels,
                                          badpixels_allocd * sizeof(*badpixels));
                }

                badpixels[badpixels_count++] = y * PIX_W + x;
            }
        }
}

static void CorrectBadPixels(void)
{
  int  n;
  int  x, y;
  
    //fprintf(stderr, "bpc=%d\n", badpixels_count);
    for (n = 0;  n < badpixels_count;  n++)
    {
        y = badpixels[n] / PIX_W;
        x = badpixels[n] % PIX_W;
        
        curdata[y][x] =
            (curdata[y-1][x-1] + curdata[y-1][x] + curdata[y-1][x+1] +
             curdata[y]  [x-1] +                   curdata[y]  [x+1] +
             curdata[y+1][x-1] + curdata[y+1][x] + curdata[y+1][x+1]) / 8;
    }
}

static void ProcessPicture(void)
{
  time_t        curtime;
    
    if (undefect)
    {
        curtime = time(NULL);
        if (curtime - last_find_time > UNDEFECT_CYCLE)
        {
            FindBadPixels();
            last_find_time = curtime;
        }
        CorrectBadPixels();
    }

    FindMinMax(&this_min, &this_max, &this_sum);
    if (normalize)
        NormalizeCurdata();
    
    /* Make it X-visible */
    converter(curdata, img, cvt_info);
}

static void DisplayRed(void)
{
    return;
    if (undefect)
    {
      int n, x, y;
        
        for (n = 0;  n < badpixels_count;  n++)
        {
            y = badpixels[n] / PIX_W;
            x = badpixels[n] % PIX_W;
            XDrawPoint(XtDisplay(drawing_area), XtWindow(drawing_area), gc, x, y);
        }
    }
}

static void DisplayPicture(void)
{
    XPutImage(XtDisplay(drawing_area), XtWindow(drawing_area), gc, img,
              0, 0, 0, 0, PIX_W, PIX_H);
    DisplayRed();
    XhMakeMessage(onewin, "%d/%d scanlines; pixels: min=%d, max=%d, sum=%d",
                  PIX_H - tc_missing_count(ref), PIX_H, this_min, this_max, this_sum);
}



/*********************************************************************
*  Picture pulling section                                           *
*********************************************************************/

static void SignalProc(XtPointer     closure __attribute__((unused)),
                       XtSignalId *id __attribute__((unused)));
static void InputProc(XtPointer     closure __attribute__((unused)),
                      int          *source  __attribute__((unused)),
                      XtInputId    *id      __attribute__((unused)));
static void TimeoutProc(XtPointer     closure __attribute__((unused)),
                        XtIntervalId *id      __attribute__((unused)));

static void PictureArrivedProc(int   cd         __attribute__((unused)),
                               int   reason,
                               void *privptr    __attribute__((unused)),
                               const void *info __attribute__((unused)))
{
    if (reason != CAR_ANSWER) return;

    XtNoticeSignal(signal_id);
}

static void KeepaliveProc(XtPointer     closure __attribute__((unused)),
                          XtIntervalId *id      __attribute__((unused)))
{
    XtAppAddTimeOut(xh_context, 1000, KeepaliveProc, NULL);
}


static void InitPictureSource(const char *src)
{
  char   *slash;
  char   *addr = src;
  char    buf[1000];
  size_t  len;
  
    slash = strchr(src, '/');
    if (slash != NULL)
    {
        if      (strncasecmp(src, "udp/", slash - src) == 0)
        {
            addr = slash + 1;
        }
        else if (strncasecmp(src, "cx/",  slash - src) == 0)
        {
            using_cx = 1;

            if (cx_parse_chanref(slash + 1,
                                 buf, sizeof(buf), &bigc) < 0)
            {
                fprintf(stderr, "Invalid CX reference\n");
                exit(1);
            }
        }
        else
        {
            fprintf(stderr, "Invalid access method in \"%s\" spec\n",
                    src);
            exit(1);
        }
    }

    if (using_cx)
    {
        conn = cx_openbigc(buf, "tsycam");
        if (conn < 0)
        {
            fprintf(stderr, "%s %s: cx_openbigc(): %s\n",
                    strcurtime(), __FUNCTION__, cx_strerror(errno));
        }
        signal_id = XtAppAddSignal(xh_context, SignalProc, NULL);
        cx_setnotifier(conn, PictureArrivedProc, NULL);
        cx_nonblock(conn, 1);
        KeepaliveProc(NULL, NULL);
    }
    else
    {
        ref = tc_create_camera_object(addr, 0,
                                      PIX_W, PIX_H,
                                      &(curdata[0][0]), sizeof(curdata[0][0]));
        if (ref == NULL)
        {
            fprintf(stderr, "%s %s: tc_create_camera_object(): %s\n",
                    strcurtime(), __FUNCTION__, strerror(errno));
            exit(1);
        }
    }
}

static void ScheduleTimeout(int usecs)
{
//    fprintf(stderr, "%s(%d), id=%x\n", __FUNCTION__, usecs, timeout_id);
    if (usecs == 0) return;

    if (timeout_id != 0) XtRemoveTimeOut(timeout_id), timeout_id = 0;

    if (usecs > 0)
        timeout_id = XtAppAddTimeOut(xh_context, usecs / 1000, TimeoutProc, NULL);
}

static void StartPullPicture(void)
{
  int    r;
  int    usecs;
  int32  args[NUM_TSYCAM_PARAMS];
    

//    fprintf(stderr, "%s: pulling=%d\n", __FUNCTION__, pulling);
  
    if (pulling) return;
    pulling = True;

    if (using_cx)
    {
        bzero(args, sizeof(args));
        args[TSYCAM_PARAM_W]    = -1;
        args[TSYCAM_PARAM_H]    = -1;
        args[TSYCAM_PARAM_K]    = param_K;
        args[TSYCAM_PARAM_T]    = param_T;
        args[TSYCAM_PARAM_SYNC] = param_SYNC;

        r = cx_bigcmsg(conn, bigc,
                       args, (params_changed?sizeof(args)/sizeof(args[0]):0),
                       NULL, 0, 0,
                       retinfo, countof(retinfo), NULL,
                       curdata, sizeof(curdata), NULL, NULL, NULL, NULL/*!!!ERRSTATS*/,
                       CX_CACHECTL_SHARABLE, CX_BIGC_IMMED_YES);
        if (r < 0  &&  errno != CERUNNING)
            fprintf(stderr, "%s %s: cx_bigcmsg: %s\n",
                    strcurtime(), __FUNCTION__, cx_strerror(errno));
        
        params_changed = 0;
    }
    else
    {
        if (input_id == 0)
            input_id = XtAppAddInput(xh_context, tc_fd_of_object(ref),
                                     (XtPointer)XtInputReadMask, InputProc, NULL);
        
        if (params_changed)
        {
            tc_set_parameter(ref, "K",    param_K);
            tc_set_parameter(ref, "T",    param_T);
            tc_set_parameter(ref, "SYNC", param_SYNC);
            tc_set_parameter(ref, "RRQ",  param_RRQ);
            tc_set_parameter(ref, "MEM",  param_MEM);
            params_changed = 0;
        }
        
        r = tc_request_new_frame(ref, &usecs);
        if (r < 0)
            perror("tc_request_new_frame()");
        else
            ScheduleTimeout(usecs);
    }
}

static void EndPullPicture(void)
{
    if (!pulling) return;
    
    if (using_cx)
    {
        
    }
    else
    {
        tc_decode_frame(ref);
    }

    ProcessPicture();
    DisplayPicture();

    pulling = False;

    if (loop_get) StartPullPicture();
}

static void SignalProc(XtPointer     closure __attribute__((unused)),
                       XtSignalId *id __attribute__((unused)))
{
    EndPullPicture();
}

static void InputProc(XtPointer     closure __attribute__((unused)),
                      int          *source  __attribute__((unused)),
                      XtInputId    *id      __attribute__((unused)))
{
  int  r;
  int  usecs;
    
    r = tc_fdio_proc(ref, &usecs);
    if (r >= 0) ScheduleTimeout(usecs);
    if (r == 0) fprintf(stderr, "%s: =0!!!\n", __FUNCTION__), EndPullPicture();
}

static void TimeoutProc(XtPointer     closure __attribute__((unused)),
                        XtIntervalId *id      __attribute__((unused)))
{
  int  r;
  int  usecs;
    
    r = tc_timeout_proc(ref, &usecs);
    if (r == 0) fprintf(stderr, "%s: =0!!!\n", __FUNCTION__), EndPullPicture();
    if (r >= 0) ScheduleTimeout(usecs);
}


/*********************************************************************
*  Initialization / widget creation / callbacks section              *
*********************************************************************/

static void ReadEnvironment(void)
{
    if (getenv("K")    != NULL) param_K    = atoi(getenv("K"));
    if (getenv("T")    != NULL) param_T    = atoi(getenv("T"));
    if (getenv("SYNC") != NULL) param_SYNC = getenv("SYNC")[0] != '0';
    if (getenv("RRQ")  != NULL) param_RRQ  = getenv("RRQ") [0] != '0';
    if (getenv("MEM")  != NULL) param_MEM  = getenv("MEM") [0] != '0';
    params_changed = 1;
}

static void MakeXImage(void)
{
  Visual *visual;
  int     depth  = 24;
  char   *data   = malloc(4 * PIX_W * PIX_H);
  Widget  shell  = XhGetWindowShell(onewin);
  int     r;

    XtVaGetValues(shell, XmNvisual, &visual, NULL);
    XtVaGetValues(shell, XmNdepth,  &depth,  NULL);

    if (visual == NULL)  visual = DefaultVisualOfScreen(XtScreen(shell));
  
    img = XCreateImage(XtDisplay(shell), visual, depth, ZPixmap,
                       0, data, PIX_W, PIX_H,
                       32, 4 * PIX_W*0);
    r = GetGray2ImageConverter(sizeof(px_t), EXT_BITS, img, &converter, &cvt_info, 0);
    if (r != 0)
    {
        fprintf(stderr, "%s %s: GetGray2ImageConverter(bytes=%zu/bits=%d=>%d) failure: %s\n",
                strcurtime(), __FUNCTION__,
                sizeof(px_t), EXT_BITS, img->bits_per_pixel / 8, strerror(errno));
        exit(1);
    }
}

static void CreateGC(void)
{
  XGCValues  vals;
  Colormap   cmap;
  XColor     xcol;
  Widget     shell = XhGetWindowShell(onewin);

    XtVaGetValues(shell, XmNcolormap, &cmap, NULL);
    if (!XAllocNamedColor(XtDisplay(shell), cmap, "red", &xcol, &xcol))
    {
        fprintf(stderr, "Failed to allocate red\n");
        xcol.pixel = WhitePixelOfScreen(XtScreen(shell));
    }
    vals.foreground = xcol.pixel;
    gc = XtGetGC(shell, GCForeground, &vals);
}


static void ExposureCB(Widget     w,
                       XtPointer  closure __attribute__((unused)),
                       XtPointer  call_data)
{
  XExposeEvent *ev = (XExposeEvent *)(((XmDrawingAreaCallbackStruct *)call_data)->event);

    XPutImage(XtDisplay(w), XtWindow(w), gc, img,
              ev->x, ev->y, ev->x, ev->y, ev->width, ev->height);
}

static void CreateArea(void)
{
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                    XhGetWindowWorkspace(onewin),
                                    XmNshadowType,      XmSHADOW_IN,
                                    XmNshadowThickness, 1,
                                    NULL);
    drawing_area = XtVaCreateManagedWidget("drawing_area", xmDrawingAreaWidgetClass,
                                           frame,
                                           XmNwidth,       PIX_W,
                                           XmNheight,      PIX_H,
                                           XmNtraversalOn, False,
                                           NULL);
    if (double_buffering)
    {

    }
    else
    {
        XtAddCallback(drawing_area, XmNexposeCallback, ExposureCB, NULL);
    }
}

static void loopCB(Widget    w,
                   XtPointer closure   __attribute__((unused)),
                   XtPointer call_data __attribute__((unused)))
{
    loop_get = !loop_get;
    
    if (loop_get) StartPullPicture();

    XhInvertButton(w);
    XtSetSensitive(w_once, !loop_get);
}

static void onceCB(Widget    w         __attribute__((unused)),
                   XtPointer closure   __attribute__((unused)),
                   XtPointer call_data __attribute__((unused)))
{
    StartPullPicture();
}

static void ParamKCB(Knob k, double v, void *privptr)
{
  int *p = (int *)privptr;
  
    *p = v;
    params_changed = 1;
    last_find_time = 0;
}

static void ShowKCB(Knob k, double v, void *privptr)
{
  Bool *p = (Bool *)privptr;
  
    *p = v;
    last_find_time = 0;
}

static void CreateControls(void)
{
  Widget    cpanel;

    cpanel = XtVaCreateManagedWidget("controls", xmFormWidgetClass,
                                     XhGetWindowWorkspace(onewin),
                                     XmNshadowThickness,  0,
                                     XmNtopAttachment,    XmATTACH_WIDGET,
                                     XmNtopWidget,        frame,
                                     XmNleftAttachment,   XmATTACH_FORM,
                                     XmNrightAttachment,  XmATTACH_FORM,
                                     NULL);

    w_loop = XtVaCreateManagedWidget("toolButton", xmPushButtonWidgetClass, cpanel,
                                     XmNlabelType,   XmPIXMAP,
                                     NULL);
    XhAssignPixmap(w_loop, btn_start_xpm);
    XhSetBalloon(w_loop, "Periodic measurement");
    XtAddCallback(w_loop, XmNactivateCallback, loopCB, NULL);

    w_once = XtVaCreateManagedWidget("toolButton", xmPushButtonWidgetClass, cpanel,
                                     XmNlabelType,   XmPIXMAP,
                                     NULL);
    XhAssignPixmap(w_once, btn_once_xpm);
    XhSetBalloon(w_once, "Take one picture");
    attachleft(w_once, w_loop, INTERKNOB_H_SPACING);
    XtAddCallback(w_once, XmNactivateCallback, onceCB, NULL);

    k_K = CreateSimpleKnob(" rw look=hslider label='Brightness'"
                           " dpyfmt='%3.0f' "
                           " mindisp=0 maxdisp=255"
                           " minnorm=0 maxnorm=255"
                           " comment='Brightness (K): 0-255'"
                           " widdepinfo='withlabel compact'",
                           0, cpanel, ParamKCB, &param_K);
    SetKnobValue(k_K, param_K);
    attachleft(GetKnobWidget(k_K), w_once, INTERELEM_H_SPACING);

    k_T = CreateSimpleKnob(" rw look=hslider label='Exposition'"
                           " dpyfmt='%3.0f' "
                           " mindisp=0 maxdisp=511"
                           " minnorm=0 maxnorm=511"
                           " comment='Exposition (T): 0-511'"
                           " widdepinfo='withlabel compact'",
                           0, cpanel, ParamKCB, &param_T);
    SetKnobValue(k_T, param_T);
    attachleft(GetKnobWidget(k_T), GetKnobWidget(k_K), INTERKNOB_H_SPACING);

    k_SYNC = CreateSimpleKnob(" rw look=onoff label='SYNC'"
                              " comment='External SYNC'"
                              " widdepinfo='color=blue'",
                              0, cpanel, ParamKCB, &param_SYNC);
    SetKnobValue(k_SYNC, param_SYNC);
    attachleft(GetKnobWidget(k_SYNC), GetKnobWidget(k_T), INTERKNOB_H_SPACING);

    k_RRQ = CreateSimpleKnob(" rw look=onoff label='RRQ'"
                             " comment='Re-request scanlines if some are missing'"
                             " widdepinfo='color=green'",
                             0, cpanel, ParamKCB, &param_RRQ);
    SetKnobValue(k_RRQ, param_RRQ);
    attachleft(GetKnobWidget(k_RRQ), GetKnobWidget(k_SYNC), INTERKNOB_H_SPACING);

    k_MEM = CreateSimpleKnob(" rw look=onoff label='MEM'"
                             " comment='Pull data from memory, without reading CCD'"
                             " widdepinfo='color=black'",
                             0, cpanel, ParamKCB, &param_MEM);
    SetKnobValue(k_MEM, param_MEM);
    attachleft(GetKnobWidget(k_MEM), GetKnobWidget(k_SYNC), INTERKNOB_H_SPACING);
    attachtop (GetKnobWidget(k_MEM), GetKnobWidget(k_RRQ),  INTERKNOB_V_SPACING);

    k_normalize = CreateSimpleKnob(" rw look=onoff label='Normalize'"
                                   " comment='Automatically rescale image brightness'"
                                   " widdepinfo='color=amber'",
                                   0, cpanel, ShowKCB, &normalize);
    attachright(GetKnobWidget(k_normalize), NULL, 0);
    
    k_undefect = CreateSimpleKnob(" rw look=onoff label='Undefect'"
                                  " comment='Remove defect star pixels'"
                                  " widdepinfo='color=red'",
                                  0, cpanel, ShowKCB, &undefect);
    attachright(GetKnobWidget(k_undefect), GetKnobWidget(k_normalize), INTERKNOB_H_SPACING);
}

static void PopulateWindow(void)
{
    CreateGC();
    CreateArea();
    CreateControls();
}


/*********************************************************************
*  Main() section                                                    *
*********************************************************************/

static char *fallbacks[] =
{
    "*.onoffSYNC.selectColor: blue",
    "*.onoffRRQ.selectColor:  violet",
    "*.onoffMEM.selectColor:  black",
    "*.loop.selectColor:      green",
    "*.undefect.selectColor:  red",
    "*.normalize.selectColor: yellow",
    XH_STANDARD_FALLBACKS
};

int main(int argc, char *argv[])
{
  char *src = "192.168.6.11";
  char  title[1000];
    
    XhInitApplication(&argc, argv, app_name, app_class, fallbacks);
    if (argv[1] != NULL) src = argv[1];

    snprintf(title, sizeof(title), "Tsycam test utility - %s", src);
    
    onewin = XhCreateWindow(title, app_name, app_class,
                            XhWindowStatuslineMask | XhWindowUnresizableMask,
                            NULL,
                            NULL);

    ReadEnvironment();
    MakeXImage();
    PopulateWindow();

    InitPictureSource(src);
    StartPullPicture();
    
    XhShowWindow(onewin);
    XhRunApplication();
    
    return 0;
}
