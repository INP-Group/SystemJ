#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Text.h>

#include "misc_macros.h"
#include "misclib.h"
#include "paramstr_parser.h"

#include "cda.h"
#include "Xh.h"
#include "Knobs.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"
#include "ChlI.h"

#include "drv_i/tsycamv_drv_i.h"

#include "ndbp.h"
#include "ndbp_staroio.h"

#include "ndbp_image_elemplugin.h"


//////////////////////////////////////////////////////////////////////

static GC AllocInvrGC(Widget w, const char *spec)
{
  XGCValues  vals;

  Colormap   cmap;
  XColor     xcol;

    XtVaGetValues(w, XmNcolormap, &cmap, NULL);
    if (!XAllocNamedColor(XtDisplay(w), cmap, spec, &xcol, &xcol))
    {
        xcol.pixel = WhitePixelOfScreen(XtScreen(w));
    }
    vals.foreground = xcol.pixel;
  
    vals.function = GXxor;
  
    return XtGetGC(w, GCForeground | GCFunction, &vals);
}

//////////////////////////////////////////////////////////////////////

enum
{
    PIX_W  = 660,
    PIX_H  = 500,

    CUT_L  = 1, CUT_R = 1,
    CUT_T  = 1, CUT_B = 10,
    
    SHOW_W = PIX_W - CUT_L - CUT_R,
    SHOW_H = PIX_H - CUT_T - CUT_B,

    IMG_BYPP     = sizeof(uint32),
    IMG_DATASIZE = IMG_BYPP * PIX_W * PIX_H,

    MES_DATASIZE = sizeof(uint16) * PIX_W * PIX_H,
};

enum
{
    MAX_PIX_VALUE = 1023,
    CELLS_REGULAR = MAX_PIX_VALUE + 1,
    CELL_INVALID  = MAX_PIX_VALUE + 1,
    CELL_FORMAX   = MAX_PIX_VALUE + 2,
    CELLS_SPECIAL = 2,
    CELLS_TOTAL   = CELLS_REGULAR + CELLS_SPECIAL
};

enum
{
    SHOW_AS_IS    = 0,
    SHOW_INVERSE  = 1,
    SHOW_IN_COLOR = 2
};

typedef struct
{
    ElemInfo              e; // A back-reference
    XhWindow              win;

    int                   running;
    int                   oneshot;
    
    Widget                area;

    XImage               *img;
    void                 *img_data;
    int                   red_max,   red_shift;
    int                   green_max, green_shift;
    int                   blue_max,  blue_shift;
    GC                    copyGC;
    GC                    invrGC[3];

    Widget                w_roller;
    int                   roller_ctr;
    
    Knob                  k_params[TSYCAMV_NUM_PARAMS];

    Knob                  k_undefect;
    Knob                  k_normalize;
    Knob                  k_violet0;
    Knob                  k_automax;
    Knob                  k_show_as;

    int                   v_undefect;
    int                   v_normalize;
    int                   v_violet0;
    int                   v_automax;
    int                   v_show_as;

    int                   x_of_mouse;
    int                   y_of_mouse;

    int                   sel_button;
    int                   sel_x0;
    int                   sel_y0;
    int                   sel_x1;
    int                   sel_y1;
    int                   sel_x2;
    int                   sel_y2;
    
    cda_serverid_t        sid;
    cda_bigchandle_t      bigch;
    tag_t                 mes_tag;
    rflags_t              mes_rflags;
    int32                 mes_info[TSYCAMV_NUM_PARAMS];
    void                 *databuf;
    uint16               *mes_data;
    uint16               *dsp_data;

    uint32                ramp[CELLS_TOTAL];
} image_privrec_t;

//////////////////////////////////////////////////////////////////////

/*
 *  Note: SplitComponentMask() supposed that the mask is contiguos, so
 *        this func ...
 */

static void SplitComponentMask(unsigned long component_mask, int *max_p, int *shift_p)
{
  int  shift_factor;
  int  num_bits;
    
    /* Sanity check */
    if (component_mask == 0)
    {
        *max_p = 1;
        *shift_p = 0;
        return;
    }

    /* I. Find out how much is it shifted */
    for (shift_factor = 0;  (component_mask & 1) == 0;  shift_factor++)
        component_mask >>= 1;

    /* II. And how many bits are there */
    for (num_bits = 1;  (component_mask & 2) != 0;  num_bits++)
        component_mask >>= 1;

    /* Return values */
    *max_p   = (1 << num_bits) - 1;
    *shift_p = shift_factor;
}

static unsigned long GetComponentBits(int v, int v_max,
                                      int component_max, int shift_factor)
{
    return ((v * component_max) / v_max) << shift_factor;
}

static unsigned long GetRgbPixelValue(image_privrec_t *me,
                                      int r, int r_max,
                                      int g, int g_max,
                                      int b, int b_max)

{
    return GetComponentBits(r, r_max, me->red_max,   me->red_shift)   |
           GetComponentBits(g, g_max, me->green_max, me->green_shift) |
           GetComponentBits(b, b_max, me->blue_max,  me->blue_shift);
}

static unsigned long GetGryPixelValue(image_privrec_t *me,
                                      int gray, int gray_max)
{
    return GetRgbPixelValue(me,
                            gray, gray_max,
                            gray, gray_max,
                            gray, gray_max);
}



//////////////////////////////////////////////////////////////////////

/*
   Based on http://en.wikipedia.org/wiki/HSV_color_space#From_HSV_to_RGB
   H is [0,360]; S and V are [0,1]; R, G, B are [0, maxRGB]
 */

static void MapHSVtoRGB(double  H, double  S, double  V,
                        int    *R, int    *G, int    *B, int    maxRGB)
{
  int     Hi = fmod(trunc(H / 60), 6);
  double  f  = H / 60 - Hi;
  double  p  = V*(1 - S);
  double  q  = V*(1 - f*S);
  double  t  = V*(1 - (1-f)*S);

    switch (Hi)
    {
        case 0: *R = V*maxRGB; *G = t*maxRGB; *B = p*maxRGB; break;
        case 1: *R = q*maxRGB; *G = V*maxRGB; *B = p*maxRGB; break;
        case 2: *R = p*maxRGB; *G = V*maxRGB; *B = t*maxRGB; break;
        case 3: *R = p*maxRGB; *G = q*maxRGB; *B = V*maxRGB; break;
        case 4: *R = t*maxRGB; *G = p*maxRGB; *B = V*maxRGB; break;
        case 5: *R = V*maxRGB; *G = p*maxRGB; *B = q*maxRGB; break;
    }
}

static void FillRamp(image_privrec_t *me)
{
  int  n;
  int  r, g, b;
    
    if      (me->v_show_as == SHOW_AS_IS)
    {
        for (n = 0;  n <= MAX_PIX_VALUE;  n++)
            me->ramp[n] = GetGryPixelValue(me, n, MAX_PIX_VALUE);
    }
    else if (me->v_show_as == SHOW_INVERSE)
    {
        for (n = 0;  n <= MAX_PIX_VALUE;  n++)
            me->ramp[n] = GetGryPixelValue(me, MAX_PIX_VALUE - n, MAX_PIX_VALUE);
    }
    else if (me->v_show_as == SHOW_IN_COLOR)
    {
        me->ramp[0] = GetGryPixelValue(me, 0, MAX_PIX_VALUE);
        for (n = 1;  n <= MAX_PIX_VALUE;  n++)
        {
            MapHSVtoRGB(RESCALE_VALUE(n, 0, MAX_PIX_VALUE, 240.0, 0.0), 0.9, 0.9,
                        &r, &g, &b, 255);
            me->ramp[n] = GetRgbPixelValue(me, r, 255, g, 255, b, 255);
        }
    }

    me->ramp[CELL_INVALID] = GetRgbPixelValue(me, 255, 255, 0,   255, 255, 255);
    me->ramp[CELL_FORMAX]  = GetRgbPixelValue(me, 255, 255, 0,   255, 0,   255);
    
    if (me->v_show_as != SHOW_IN_COLOR  &&  me->v_automax)
        me->ramp[MAX_PIX_VALUE] = me->ramp[CELL_FORMAX];

    if (me->v_show_as != SHOW_INVERSE  &&  me->v_violet0)
        me->ramp[0] = GetRgbPixelValue(me, 160, 255, 0,   255, 160, 255);;

#if 0
    for (n = 0;  n <= MAX_PIX_VALUE;  n++)
        fprintf(stderr, "%5d%s", me->ramp[n], (n & 15) == 15? "\n" : " ");
    fprintf(stderr, "%d/%d %d/%d %d/%d %08x %08x %08x\n",
            me->red_max,   me->red_shift,
            me->green_max, me->green_shift,
            me->blue_max,  me->blue_shift,
            me->img->red_mask, me->img->green_mask, me->img->blue_mask);
#endif
}

static inline int square(int x){return x*x;}

static void PerformUndefect(image_privrec_t *me)
{
  register uint16 *src;
  register uint16 *dst;
  register int     ctr;
  register int     B;
  int              sigma2;
  
    for (ctr = PIX_W + 1,
         src = me->mes_data,
         dst = me->dsp_data;
         ctr > 0;
         ctr--)
        *dst++ = (*src++) & 1023;
    for (ctr = PIX_W + 1,
         src = me->mes_data + PIX_W*PIX_H - (PIX_W+1),
         dst = me->dsp_data + PIX_W*PIX_H - (PIX_W+1);
         ctr > 0;
         ctr--)
        *dst++ = (*src++) & 1023;
  
    for (ctr = PIX_W*(PIX_H-2) - 2,
         src = me->mes_data + PIX_W + 1,
         dst = me->dsp_data + PIX_W + 1;
         ctr > 0;
         ctr--, src++, dst++)
    {
        B = (
             (
                        src[-PIX_W] +
              src[-1] +               src[+1] +
                        src[+PIX_W]
             ) & 4095
            ) / 4;
        sigma2 = (
                                      square(src[-PIX_W]-B) +
                  square(src[-1]-B) +                         square(src[+1]-B) +
                                      square(src[+PIX_W]-B)
                 ) / 8;

        *dst = (square(*src - B) > 9 * sigma2)? B : *src & 1023;
    }
}

#define MAY_USE_ASM 1

static void PerformCopy1023(image_privrec_t *me)
{
#if defined(CPU_X86)  &&  MAY_USE_ASM
    __asm__ __volatile__
        (
         "    cld\n"
         "1:  lodsw\n"
         "    andw $1023, %%ax\n"
         "    stosw\n"
         "    loop 1b\n"
         "    \n"
         : /* No output */
         : "S"(me->mes_data), "D"(me->dsp_data), "c"((int32)(PIX_W * PIX_H))
         : "%eax", "cc", "memory"
        );
#else
  register uint16 *src;
  register uint16 *dst;
  register int     ctr;
  
    for (ctr = MES_DATASIZE,
         src = me->mes_data,
         dst = me->dsp_data;
         ctr > 0;
         ctr--)
        *dst++ = (*src++) & 1023;
#endif
}

static void PerformNormalize(image_privrec_t *me)
{
#if defined(CPU_X86)  &&  MAY_USE_ASM
  int              ctr;
#else
  register uint16 *p;
  register uint16  v;
  register uint16  curmin, curmax;
  register int     ctr;
  uint16 *max_p __attribute__((unused));
#endif

  uint16           minval;
  uint16           maxval;

  uint16           table[MAX_PIX_VALUE+1];

  ////return;
#if defined(CPU_X86)  &&  MAY_USE_ASM
    __asm__ __volatile__
        (
         // BX -- curmin, DX -- curmax
         "    cld\n"
         "1:  lodsw\n"
         "    cmpw %%ax, %%bx\n"
         "    jl 2f\n"
         "    movw %%ax, %%bx\n"
         "2:  cmpw %%ax, %%dx\n"
         "    jg 3f\n"
         "    movw %%ax, %%dx\n"
         "3:  loop 1b\n"
         : "=b"(minval), "=d"(maxval)
         : "S"(me->dsp_data), "c"(PIX_W * PIX_H), "b"(MAX_PIX_VALUE), "d"(0)
         : "%eax", "cc", "memory"
        );
#else
    for (p = me->dsp_data, ctr = PIX_W*PIX_H, curmin = MAX_PIX_VALUE, curmax = 0;
         ctr != 0;
         ctr--)
    {
        v = *p++;
        if (v > curmax) curmax = v, max_p = p-1;
        if (v < curmin) curmin = v;
    }
    minval = curmin;
    maxval = curmax;
#endif

    ////fprintf(stderr, "[%d,%d], max=@%d,%d\n", minval, maxval, (max_p-me->dsp_data)%PIX_W, (max_p-me->dsp_data)/PIX_W);
    
    if (minval == maxval)
    {
        if (maxval == 0) maxval = MAX_PIX_VALUE;
        else             minval = 0;
    }

    for (ctr = 0;  ctr <= MAX_PIX_VALUE;  ctr++)
        table[ctr] = CELL_INVALID;

    for (ctr = minval;  ctr <= maxval;  ctr++)
        table[ctr] = RESCALE_VALUE(ctr, minval, maxval, 0, MAX_PIX_VALUE);

#if defined(CPU_X86)  &&  MAY_USE_ASM
    __asm__ __volatile__
        (
         "    cld\n"
         "    xorl %%eax, %%eax\n"           // Clear EAX's hi-word
         "1:  lodsw\n"
         "    movw (%%ebx,%%eax,2), %%ax\n"
         "    stosw\n"
         "    loop 1b\n"
         : /* No output */
         : "S"(me->dsp_data), "D"(me->dsp_data), "c"(PIX_W * PIX_H), "b"(table)
         : "%eax", "cc", "memory"
        );
#else
    for (p = me->dsp_data, ctr = PIX_W*PIX_H;
         ctr != 0;
         ctr--, p++)
        *p = table[*p];
#endif
}

static void CopyMesToDsp(image_privrec_t *me)
{
    if (me->v_undefect)  PerformUndefect (me);
    else                 PerformCopy1023 (me);

    if (me->v_normalize) PerformNormalize(me);
    
}

static void BlitDspToImg(image_privrec_t *me)
{
#if defined(CPU_X86)  &&  MAY_USE_ASM
    __asm__ __volatile__
        (
         "    cld\n"
         "1:  xorl %%eax, %%eax\n"
         "    lodsw\n"
         "    movl (%%ebx,%%eax,4), %%eax\n"
         "    stosl\n"
         "    loop 1b\n"
         : /* No output */
         : "S"(me->dsp_data), "D"(me->img_data), "c"(PIX_W * PIX_H), "b"(me->ramp)
         : "%eax", "cc", "memory"
        );
#else
  register uint16 *src;
  register uint32 *dst;
  register uint32 *ramp = me->ramp;
  register int     ctr;
  
    for (ctr = PIX_W*PIX_H, src = me->dsp_data, dst = me->img_data;
         ctr != 0;
         ctr--,             src++,               dst++)
        *dst = ramp[*src & 1023];
#endif

    if (0)
    {
      static int incctr = 0;
      
        memset32(me->img_data + incctr*PIX_W*4, 0xFFFFFFFF, PIX_W*20);
        incctr = (incctr + 1) & 255;
    }
    
}

static void ShowImage(image_privrec_t *me,
                      int x, int y, int width, int height)
{
    XPutImage(XtDisplay(me->area), XtWindow(me->area),
              me->copyGC, me->img,
              x + CUT_L, y + CUT_R, x, y, width, height);
}

static void ShowFrame(image_privrec_t *me)
{
    CopyMesToDsp(me);
    BlitDspToImg(me);
    ShowImage   (me, 0, 0, SHOW_W, SHOW_H);
}

static void RedisplayFrame(image_privrec_t *me)
{
    BlitDspToImg(me);
    ShowImage   (me, 0, 0, SHOW_W, SHOW_H);
}

//////////////////////////////////////////////////////////////////////

static void ExposureCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data)
{
  image_privrec_t *me = (image_privrec_t*) closure;
  XExposeEvent    *ev = (XExposeEvent *)   (((XmDrawingAreaCallbackStruct *)call_data)->event);

    ShowImage(me, ev->x, ev->y, ev->width, ev->height);
}

static void ShowCurData(image_privrec_t *me)
{
  char  buf[100];
  int   n;
    
    if (me->x_of_mouse < 0  ||  me->x_of_mouse >= SHOW_W  ||
        me->y_of_mouse < 0  ||  me->y_of_mouse >= SHOW_H) return;
    
    sprintf(buf, "%3d,%-3d - %d",
            me->x_of_mouse, me->y_of_mouse,
            me->dsp_data[(me->y_of_mouse + CUT_T)*PIX_W + me->x_of_mouse + CUT_L]);

    XhMakeTempMessage(me->win, buf);
}

static void ShowSelection(image_privrec_t *me)
{
    XFillRectangle(XtDisplay(me->area),
                   XtWindow (me->area),
                   me->invrGC[me->sel_button - 1],
                   me->sel_x1, me->sel_y1,
                   me->sel_x2 - me->sel_x1 + 1,
                   me->sel_y2 - me->sel_y1 + 1);
}

static void HideSelection(image_privrec_t *me)
{
    ShowImage(me,
              me->sel_x1, me->sel_y1,
              me->sel_x2 - me->sel_x1 + 1,
              me->sel_y2 - me->sel_y1 + 1);
}

#define DO_SWAP(a, b) do {int t; t = a; a = b; b = t;} while (0)

static void MouseHandler(Widget     w                    __attribute__((unused)),
                         XtPointer  closure,
                         XEvent    *event,
                         Boolean   *continue_to_dispatch __attribute__((unused)))
{
  image_privrec_t     *me   = (image_privrec_t*) closure;
  XMotionEvent        *mev  = (XMotionEvent *)   event;
  XButtonEvent        *bev  = (XButtonEvent *)   event;
  XCrossingEvent      *cev  = (XCrossingEvent *) event;

  int                  x;
  int                  y;
  uint16              *p;
  uint16               v;
  uint16               curmin, curmax;
  uint32              *dst;

    if (event->type == LeaveNotify)
    {
        me->x_of_mouse = me->y_of_mouse = -1;
        XhMakeTempMessage(me->win, NULL);
        return;
    }

    if      (event->type == ButtonPress  ||  event->type == ButtonRelease)
    {
        me->x_of_mouse = bev->x;
        me->y_of_mouse = bev->y;

        if      (event->type == ButtonPress       &&
                 me->sel_button == 0             &&
                 !(me->running || me->oneshot)  &&
                 (bev->button == Button1  ||  bev->button == Button3))
        {
            me->sel_button = 1 + bev->button - Button1;
            me->sel_x0 = me->sel_x1 = me->sel_x2 = bev->x;
            me->sel_y0 = me->sel_y1 = me->sel_y2 = bev->y;
            ShowSelection(me);
        }
        else if (event->type == ButtonRelease     &&
                 me->sel_button == 1 + bev->button - Button1)
        {
            /* Find min/max anyway */
            for (y  = me->sel_y1, curmin = MAX_PIX_VALUE, curmax = 0;
                 y <= me->sel_y2;
                 y++)
                for (x  = me->sel_x1, p = me->dsp_data + (y + CUT_T) * PIX_W + x + CUT_L;
                     x <= me->sel_x2;
                     x++,              p++)
                {
                    v = *p;
                    if (v > curmax) curmax = v;
                    if (v < curmin) curmin = v;
                }
            if (curmin == curmax)
            {
                if (curmax == 0) curmax = MAX_PIX_VALUE;
                else             curmin = 0;
            }
            
            if (me->sel_button == 1)
            {
                /* Button1 -- renormalize rectangle */
                for (y  = me->sel_y1;
                     y <= me->sel_y2;
                     y++)
                    for (x  = me->sel_x1,
                             p = me->dsp_data + (y + CUT_T) * PIX_W + x + CUT_L,
                             dst = ((uint32 *)me->img_data) + (y + CUT_T) * PIX_W + x + CUT_L;
                         x <= me->sel_x2;
                         x++,
                             p++,
                             dst++)
                        *dst =
                            me->ramp[RESCALE_VALUE(*p, curmin, curmax, 0, MAX_PIX_VALUE)];
            }
            else
            {
                /* Button3 -- show maximums */
                for (y  = me->sel_y1;
                     y <= me->sel_y2;
                     y++)
                    for (x  = me->sel_x1,
                             p = me->dsp_data + (y + CUT_T) * PIX_W + x + CUT_L,
                             dst = ((uint32 *)me->img_data) + (y + CUT_T) * PIX_W + x + CUT_L;
                         x <= me->sel_x2;
                         x++,
                             p++,
                             dst++)
                        if (*p == curmax)
                            *dst = me->ramp[CELL_FORMAX];
            }

            HideSelection(me);
            me->sel_button = 0;
        }
    }
    else if (event->type == MotionNotify)
    {
        me->x_of_mouse = mev->x;
        me->y_of_mouse = mev->y;

        if (me->sel_button != 0)
        {
            HideSelection(me);

            me->sel_x1 = me->sel_x0;
            me->sel_y1 = me->sel_y0;
            me->sel_x2 = mev->x;
            me->sel_y2 = mev->y;

            if (me->sel_x2 < 0)          me->sel_x2 = 0;
            if (me->sel_y2 < 0)          me->sel_y2 = 0;
            if (me->sel_x2 > SHOW_W - 1) me->sel_x2 = SHOW_W - 1;
            if (me->sel_y2 > SHOW_H - 1) me->sel_y2 = SHOW_H - 1;

            if (me->sel_x1 > me->sel_x2) DO_SWAP(me->sel_x1, me->sel_x2);
            if (me->sel_y1 > me->sel_y2) DO_SWAP(me->sel_y1, me->sel_y2);
            
            ShowSelection(me);
        }
    }
    else if (event->type == EnterNotify)
    {
        me->x_of_mouse = cev->x;
        me->y_of_mouse = cev->y;
    }
    
    ShowCurData(me);
}

static void UndefectKCB(Knob k, double v, void *privptr)
{
  image_privrec_t *me  = (image_privrec_t *) privptr;
  int              val = round(v);

    me->v_undefect = val;
    FillRamp (me);
    ShowFrame(me);
}

static void SetViolet0KnobSensitivity(image_privrec_t *me)
{
    XtSetSensitive(GetKnobWidget(me->k_violet0),
                   me->v_show_as != SHOW_INVERSE);
}

static void SetAutomaxKnobSensitivity(image_privrec_t *me)
{
    XtSetSensitive(GetKnobWidget(me->k_automax),
                   me->v_normalize  &&  me->v_show_as != SHOW_IN_COLOR);
}

static void NormalizeKCB(Knob k, double v, void *privptr)
{
  image_privrec_t *me  = (image_privrec_t *) privptr;
  int              val = round(v);

    me->v_normalize = val;
    SetAutomaxKnobSensitivity(me);
    FillRamp (me);
    ShowFrame(me);
}

static void Violet0KCB(Knob k, double v, void *privptr)
{
  image_privrec_t *me  = (image_privrec_t *) privptr;
  int              val = round(v);

    me->v_violet0 = val;
    FillRamp (me);
    ShowFrame(me);
}

static void AutomaxKCB(Knob k, double v, void *privptr)
{
  image_privrec_t *me  = (image_privrec_t *) privptr;
  int              val = round(v);

    me->v_automax = val;
    FillRamp (me);
    ShowFrame(me);
}

static void ShowKCB(Knob k, double v, void *privptr)
{
  image_privrec_t *me  = (image_privrec_t *) privptr;
  int              val = round(v);

    me->v_show_as = val;
    SetViolet0KnobSensitivity(me);
    SetAutomaxKnobSensitivity(me);
    FillRamp (me);
    ShowFrame(me);
}

static void ParamKCB(Knob k, double v, void *privptr)
{
  image_privrec_t *me  = (image_privrec_t *) privptr;
  int              val = round(v);

  int              n;

    for (n = 0;  n < countof(me->k_params);  n++)
        if (k == me->k_params[n])
            cda_setbigcparams(me->bigch, n, 1, &val);
}


static void UpdateConnStatus(image_privrec_t *me)
{
    if (me->running || me->oneshot) cda_run_server(me->sid);
    else                            cda_hlt_server(me->sid);
}

static void BigcEventProc(cda_serverid_t  sid __attribute__((unused)),
                          int             reason,
                          void           *privptr)
{
  image_privrec_t *me  = privptr;
  int              n;

  static char      roller_states[4] = "/-\\|";
  char             buf[100];
  
    if (reason != CDA_R_MAINDATA) return;
    if (!(me->running  ||  me->oneshot)) return;
    me->oneshot = 0;
    UpdateConnStatus(me);

    cda_getbigcstats (me->bigch, &(me->mes_tag), &(me->mes_rflags));
    cda_getbigcparams(me->bigch, 0, TSYCAMV_NUM_PARAMS, me->mes_info);
    cda_getbigcdata  (me->bigch, 0, MES_DATASIZE, me->mes_data);
    cda_continue(me->sid);

    /* Display current params */
    for (n = 0;  n < TSYCAMV_NUM_PARAMS;  n++)
        if (me->k_params[n] != NULL)
            SetKnobValue(me->k_params[n], me->mes_info[n]);

    /* Clear cut pixels */
    {
      int     y;
      uint16 *p;
        
        bzero(me->mes_data,                       PIX_W * sizeof(uint16) * CUT_T);
        bzero(me->mes_data + PIX_W*(PIX_H-CUT_B), PIX_W * sizeof(uint16) * CUT_B);

        for (y = 0, p = me->mes_data + PIX_W * CUT_T;
             y < PIX_H - CUT_T - CUT_B;
             y++)
            *p = *(p + PIX_W - 1) = 0;
    }
    
    ShowFrame(me);
    ShowCurData(me);

    me->roller_ctr = (me->roller_ctr + 1) & 3;
    sprintf(buf, "%c", roller_states[me->roller_ctr]);
    XmTextSetString(me->w_roller, buf);
}

static void Run_n_Once(int code, void *privptr)
{
  image_privrec_t *me  = privptr;
  
    switch (code)
    {
        case RUNNONCE_RUN:
        case RUNNONCE_STOP:
            me->running = code == RUNNONCE_RUN;
            UpdateConnStatus(me);
            break;

        case RUNNONCE_ONCE:
            me->oneshot = 1;
            UpdateConnStatus(me);
            break;
    }
}

static const char *mode_lp_s       = "# CDMpic";
static const char *subsys_lp_s     = "#!SUBSYSTEM:";
static const char *crtime_lp_s     = "#!CREATE-TIME:";
static const char *comment_lp_s    = "#!COMMENT:";
static const char *dimensions_lp_s = "#!DIMENSIONS:";

static  int DoFilter(const char *filespec,
                     time_t *cr_time,
                     char *commentbuf, size_t commentbuf_size,
                     void *privptr)
{
  FILE        *fp;
  char         line[1000];
  int          lineno = 0;
  char        *cp;
  char        *err;
  struct tm    brk_time;
  time_t       sng_time;

    if ((fp = fopen(filespec, "r")) == NULL) return -1;

    if (commentbuf_size > 0) *commentbuf = '\0';

    while (lineno < 10  &&  fgets(line, sizeof(line) - 1, fp) != NULL)
    {
        lineno++;
    
        if      (memcmp(line, mode_lp_s,    strlen(mode_lp_s)) == 0)
        {
            cp = line + strlen(mode_lp_s);
            while (isspace(*cp)) cp++;
            
            bzero(&brk_time, sizeof(brk_time));
            if (strptime(cp, "%a %b %d %H:%M:%S %Y", &brk_time) != NULL)
            {
                sng_time = mktime(&brk_time);
                if (sng_time != (time_t)-1)
                    *cr_time = sng_time;
            }
        }
        else if (memcmp(line, crtime_lp_s,  strlen(crtime_lp_s)) == 0)
        {
            cp = line + strlen(crtime_lp_s);
            while (isspace(*cp)) cp++;

            sng_time = (time_t)strtol(cp, &err, 0);
            if (err != cp  &&  (*err == '\0'  ||  isspace(*err)))
                *cr_time = sng_time;
        }
        else if (memcmp(line, comment_lp_s, strlen(comment_lp_s)) == 0)
        {
            cp = line + strlen(comment_lp_s);
            while (isspace(*cp)) cp++;

            strzcpy(commentbuf, cp, commentbuf_size);
        }
    }

    fclose(fp);

    return 0;
}

static void DoLoad  (const char *filespec)
{
  image_privrec_t *me  = runnonce_privptr;

  FILE        *fp;
  char         buf[4000];
  int          y;
  int          x;
  char        *cp;
  char        *err;
  int          v;

    if ((fp = fopen(filespec, "r")) == NULL) return -1;

    for (y = 0;
         y < SHOW_H  &&  fgets(buf, sizeof(buf) - 1, fp) != NULL;
         /**/)
    {
        cp = buf;
        while (*cp != '\0'  &&  isspace(*cp)) cp++;
        if (*cp == '\0'  ||  *cp == '#') continue;
        
        for (x = 0;
             *cp != '\0'  &&  x < SHOW_W;
             x++)
        {
            v = strtol(cp, &err, 10);
            if (err == cp  ||  (*err != '\0'  &&  !isspace(*err)))
                goto END_OF_FILE;
            cp = err;

            me->dsp_data[(y + CUT_T)*PIX_W + x + CUT_L] =
            me->mes_data[(y + CUT_T)*PIX_W + x + CUT_L] = v & 1023;
        }

        y++;
    }

 END_OF_FILE:
    fclose(fp);
    
    BlitDspToImg(me);
    ShowImage   (me, 0, 0, SHOW_W, SHOW_H);
}

static void DoSave  (const char *filespec, const char *comment, const char *subsys)
{
  image_privrec_t *me  = runnonce_privptr;

  FILE        *fp;
  time_t       timenow = time(NULL);
  const char  *cp;
  
  int          y;
  int          x;
  
    if ((fp = fopen(filespec, "w")) == NULL) return -1;

    fprintf(fp, "##########\n%s %s\n", mode_lp_s, ctime(&timenow));
    fprintf(fp, "%s %s\n",  subsys_lp_s, subsys);
    fprintf(fp, "%s %lu %s", crtime_lp_s, (unsigned long)(timenow), ctime(&timenow)); /*!: No '\n' because ctime includes a trailing one. */
    fprintf(fp, "%s %d %d\n", dimensions_lp_s, SHOW_W, SHOW_H);
    fprintf(fp, "%s ", comment_lp_s);
    if (comment != NULL)
        for (cp = comment;  *cp != '\0';  cp++)
            fprintf(fp, "%c", !iscntrl(*cp)? *cp : ' ');
    fprintf(fp, "\n\n");

    for (y = 0;  y < SHOW_H;  y++)
    {
        for (x = 0;  x < SHOW_W;  x++)
            fprintf(fp, "%s%4d", x == 0? "" : " ",
                    me->dsp_data[(y + CUT_T)*PIX_W + x + CUT_L]);
        fprintf(fp, "\n");
    }
    
    fclose(fp);

    return 0;
}


void image_NewData_m(ElemInfo ei __attribute__((unused)), int synthetic)
{
    if (synthetic) return;

    ndbp_staroio_NewData();
}

static knobs_emethods_t ELEM_IMAGE_emethods =
{
    Chl_E_SetPhysValue_m,
    NULL,
    NULL,
    Chl_E_ShowPropsWindow,
    Chl_E_ShowBigValWindow,
    NULL,
    Chl_E_ToHistPlot
};

typedef struct
{
    int   bigc;
    int   port1;
    int   port2;
    char  chanlist[2000];
} image_elemopts_t;

static psp_paramdescr_t text2image_elemopts[] =
{
    PSP_P_INT   ("bigc",     image_elemopts_t, bigc,     0, 0, 0),
    PSP_P_INT   ("port1",    image_elemopts_t, port1,    0, 0, 0),
    PSP_P_INT   ("port2",    image_elemopts_t, port2,    0, 0, 0),
    PSP_P_STRING("chanlist", image_elemopts_t, chanlist, ""),
    PSP_P_END()
};

int ELEM_IMAGE_Create_m(XhWidget parent, ElemInfo e)
{
  image_privrec_t  *me;
  image_elemopts_t  opts;

  Widget            ctl;
  Widget            frame;

  Widget            shell;
  Visual           *visual;
  int               depth;
  
  XGCValues         vals;

  Widget            cur;
  Widget            top;
  
  cda_serverid_t    mainsid;
  const char       *srvname;

    if ((me = e->elem_private = XtMalloc(sizeof(image_privrec_t))) == NULL)
        return -1;
    bzero(me, sizeof(*me));
    me->e   = e;
    me->win = XhWindowOf(parent);

    runnonce_proc    = Run_n_Once;
    runnonce_privptr = me;

    LoadPicFilter    = DoFilter;
    DoLoadPic        = DoLoad;
    DoSavePic        = DoSave;
        
    bzero(&opts, sizeof(opts));
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2image_elemopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
    }
    
    e->container = ABSTRZE(XtVaCreateManagedWidget("elementForm",
                                                   xmFormWidgetClass,
                                                   CNCRTZE(parent),
                                                   XmNshadowThickness, 0,
                                                   NULL));
    e->innage = e->container;
    e->emlink = &ELEM_IMAGE_emethods;

    ctl = XtVaCreateManagedWidget("elementForm",
                                  xmFormWidgetClass,
                                  CNCRTZE(e->innage),
                                  XmNshadowThickness, 0,
                                  NULL);
    
    frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
                                    e->innage,
                                    XmNshadowType,      XmSHADOW_IN,
                                    XmNshadowThickness, 1,
                                    NULL);
    attachleft(frame, ctl, CHL_INTERKNOB_H_SPACING);

    me->area = XtVaCreateManagedWidget("drawing_area", xmDrawingAreaWidgetClass,
                                       frame,
                                       XmNwidth,       SHOW_W,
                                       XmNheight,      SHOW_H,
                                       XmNtraversalOn, False,
                                       NULL);
    me->x_of_mouse = me->y_of_mouse = -1;
    XtAddCallback    (me->area, XmNexposeCallback, ExposureCB, (XtPointer)me);
    XtAddEventHandler(me->area,
                      (ButtonPressMask   | ButtonReleaseMask |
                       PointerMotionMask |
                       Button1MotionMask | Button2MotionMask | Button3MotionMask |
                       EnterWindowMask   | LeaveWindowMask),
                      False, MouseHandler,  (XtPointer)me);

    me->img_data = XtMalloc(IMG_DATASIZE);
    bzero(me->img_data, IMG_DATASIZE);
    shell  = XhGetWindowShell(XhWindowOf(me->area));
    XtVaGetValues(shell,
                  XmNvisual, &visual,
                  XmNdepth, &depth,
                  NULL);
    if (visual == NULL)  visual = DefaultVisualOfScreen(XtScreen(shell));
    me->img = XCreateImage(XtDisplay(me->area), visual, depth, ZPixmap,
                           0, me->img_data, PIX_W, PIX_H,
                           32, 0);
    SplitComponentMask(me->img->red_mask,   &(me->red_max),   &(me->red_shift));
    SplitComponentMask(me->img->green_mask, &(me->green_max), &(me->green_shift));
    SplitComponentMask(me->img->blue_mask,  &(me->blue_max),  &(me->blue_shift));
    
//    *(int*)0xFFFFFF = 0;
    
    vals.foreground = XhGetColor(XH_COLOR_JUST_RED);
    me->copyGC = XtGetGC(me->area, GCForeground, &vals);
    me->invrGC[0] = AllocInvrGC(me->area, "green");
    me->invrGC[0] = AllocInvrGC(me->area, "blue");
    me->invrGC[2] = AllocInvrGC(me->area, "red");

    me->w_roller =
        XtVaCreateManagedWidget("text_o", xmTextWidgetClass, ctl,
                                XmNvalue, "",
                                XmNcursorPositionVisible, False,
                                XmNcolumns,               2,
                                XmNscrollHorizontal,      False,
                                XmNeditMode,              XmSINGLE_LINE_EDIT,
                                XmNeditable,              False,
                                XmNtraversalOn,           False,
                                NULL);
    me->roller_ctr = 0;
    cur = me->w_roller;
    attachtop (cur, NULL, 0);
    top = cur;
    
    me->k_params[TSYCAMV_PARAM_K] =
        CreateSimpleKnob(" rw look=text label='Brightness'"
                         " dpyfmt='%3.0f' "
                         " mindisp=0 maxdisp=255"
                         " minnorm=0 maxnorm=255"
                         " comment='Brightness (K): 0-255'"
                         " widdepinfo='withlabel compact'",
                         0, ctl, ParamKCB, me);
    cur = GetKnobWidget(me->k_params[TSYCAMV_PARAM_K]);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;
    
    me->k_params[TSYCAMV_PARAM_T] =
        CreateSimpleKnob(" rw look=text label='Exposition'"
                         " dpyfmt='%3.0f' "
                         " mindisp=0 maxdisp=511"
                         " minnorm=0 maxnorm=511"
                         " comment='Exposition (T): 0-511'"
                         " widdepinfo='withlabel compact'",
                         0, ctl, ParamKCB, me);
    cur = GetKnobWidget(me->k_params[TSYCAMV_PARAM_T]);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;
    
    me->k_params[TSYCAMV_PARAM_SYNC] =
        CreateSimpleKnob(" rw look=selector label='Sync'"
                         " comment='Synchronization mode'"
                         " widdepinfo='#C#TProg\vExt'",
                         0, ctl, ParamKCB, me);
    cur = GetKnobWidget(me->k_params[TSYCAMV_PARAM_SYNC]);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;

    me->k_params[TSYCAMV_PARAM_MISS] =
        CreateSimpleKnob(" ro look=text label='Miss'"
                         " dpyfmt='%3.0f'"
                         " comment='Number of lost scanlines'"
                         " widdepinfo='withlabel compact'",
                         0, ctl, ParamKCB, me);
    cur = GetKnobWidget(me->k_params[TSYCAMV_PARAM_MISS]);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;

    me->k_undefect = CreateSimpleKnob(" rw look=greenled label='Undefect'"
                                      " comment='Remove defect \'star\' pixels'",
                                      0, ctl, UndefectKCB, me);
    SetKnobValue(me->k_undefect, me->v_undefect = 1);
    cur = GetKnobWidget(me->k_undefect);
    attachtop (cur, top, CHL_INTERELEM_V_SPACING*2);
    top = cur;

    me->k_normalize = CreateSimpleKnob(" rw look=amberled label='Normalize'"
                                       " comment='Automatically rescale image brightness'",
                                       0, ctl, NormalizeKCB, me);
    SetKnobValue(me->k_normalize, me->v_normalize = 0);
    cur = GetKnobWidget(me->k_normalize);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;

    me->k_violet0 = CreateSimpleKnob(" rw look=redled label='0=violet'"
                                     " comment='Show 0 in violet'",
                                     0, ctl, Violet0KCB, me);
    SetKnobValue(me->k_violet0, me->v_violet0 = 0);
    cur = GetKnobWidget(me->k_violet0);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;

    me->k_automax = CreateSimpleKnob(" rw look=redled label='Max=red'"
                                     " comment='Automatically mark maximum-brightness pixels with red'",
                                     0, ctl, AutomaxKCB, me);
    SetKnobValue(me->k_automax, me->v_automax = 0);
    cur = GetKnobWidget(me->k_automax);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;

    me->k_show_as =
        CreateSimpleKnob(" rw look=selector label='Show:'"
                         " comment='Image display mode'"
                         " widdepinfo='#C#-#TAs-is\vInverse\vIn-color'",
                         0, ctl, ShowKCB, me);
    SetKnobValue(me->k_show_as, me->v_show_as = SHOW_AS_IS);
    cur = GetKnobWidget(me->k_show_as);
    attachtop (cur, top, CHL_INTERKNOB_V_SPACING);
    top = cur;

    SetViolet0KnobSensitivity(me);
    SetAutomaxKnobSensitivity(me);
    
    /*  */
    me->databuf  = XtMalloc(MES_DATASIZE * 2);
    me->mes_data = ((int8 *)me->databuf) + 0;
    me->dsp_data = ((int8 *)me->databuf) + MES_DATASIZE;
    FillRamp(me);
    
    /* Make a server connection */
    mainsid = ChlGetMainsid(XhWindowOf(parent));
    srvname = cda_status_srvname(mainsid, 0);

    if ((me->sid = cda_new_server(srvname,
                                  BigcEventProc, me,
                                  CDA_BIGC)) == CDA_SERVERID_ERROR)
    {
        fprintf(stderr, "%s: %s: cda_new_server(server=%s): %s\n",
                strcurtime(), __FUNCTION__, srvname, cx_strerror(errno));
        exit(1);
    }
    
    me->bigch = cda_add_bigc(me->sid, opts.bigc,
                             TSYCAMV_NUM_PARAMS,
                             MES_DATASIZE,
                             CX_CACHECTL_SHARABLE, CX_BIGC_IMMED_YES*1);

    UpdateConnStatus(me);

    ndbp_staroio_Listen(XhWindowOf(parent), opts.port1, opts.port2, opts.chanlist);
    
    return 0;
}
