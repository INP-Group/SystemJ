
#include "Xh_includes.h"

/* Determine which widget to use */
#define GRIDBOX  1
#define TABLE    2
#define SGL      3

#if   USE_WIDGET == GRIDBOX
  #include "Gridbox.h"
#elif USE_WIDGET == TABLE
  //#define WINTERP
  #include "Table.h"
#elif USE_WIDGET == SGL
  #include "XmSepGridLayout.h"
#else
  #error The "USE_WIDGET" macro is undefined
#endif


XhWidget  XhCreateGrid(XhWidget parent, const char *name)
{
#if   USE_WIDGET == GRIDBOX
    return ABSTRZE(XtVaCreateManagedWidget(name, gridboxWidgetClass, CNCRTZE(parent), NULL));
#elif USE_WIDGET == TABLE
    return ABSTRZE(XtVaCreateManagedWidget(name, tableWidgetClass, CNCRTZE(parent), NULL));
#else
    return ABSTRZE(XtVaCreateManagedWidget(name, xmSepGridLayoutWidgetClass, CNCRTZE(parent),
//                                           XmNmarginWidth,  1,
//                                           XmNmarginHeight, 1,
                                           XmNsepHType,     XmSHADOW_ETCHED_IN_DASH,
                                           XmNsepVType,     XmSHADOW_ETCHED_IN_DASH,
//                                           XmNresFlag,      False,
//                                           XmNrows,         10,
//                                           XmNcolumns,      10,

                                           NULL));
#endif
}

void      XhGridSetPadding  (XhWidget w, int hpad, int vpad)
{
#if   USE_WIDGET == GRIDBOX
  int d = 0;
  
    if (hpad == -1  &&  vpad == -1) return;
    if (hpad > d) d = hpad;
    if (vpad > d) d = vpad;
    XtVaSetValues(CNCRTZE(w),
                  XtNdefaultDistance, d,
                  NULL);
#elif USE_WIDGET == TABLE
    if (hpad >= 0) XtVaSetValues(CNCRTZE(w),
                                 XmNmarginWidth,       hpad * 1,
                                 XmNhorizontalSpacing, hpad * 1,
                                 NULL);
    if (vpad >= 0) XtVaSetValues(CNCRTZE(w),
                                 XmNmarginHeight,      vpad * 1,
                                 XmNverticalSpacing,   vpad * 1,
                                 NULL);
#else
    if (hpad >= 0) XtVaSetValues(CNCRTZE(w), XmNmarginWidth,  hpad, NULL);
    if (vpad >= 0) XtVaSetValues(CNCRTZE(w), XmNmarginHeight, vpad, NULL);
#endif
}

void      XhGridSetSpacing  (XhWidget w, int hspc, int vspc)
{
#if   USE_WIDGET == GRIDBOX
#elif USE_WIDGET == TABLE
#else
    if (hspc >= 0) XtVaSetValues(CNCRTZE(w), XmNsepWidth,  hspc, NULL);
    if (vspc >= 0) XtVaSetValues(CNCRTZE(w), XmNsepHeight, vspc, NULL);
#endif
}

void      XhGridSetGrid     (XhWidget w, int horz, int vert)
{
#if   USE_WIDGET == SGL
  unsigned char  hst = XmNO_LINE;
  unsigned char  vst = XmNO_LINE;

    if (horz < 0) hst = XmSHADOW_ETCHED_IN_DASH;
    if (vert < 0) vst = XmSHADOW_ETCHED_IN_DASH;
    if (horz > 0) hst = XmSHADOW_ETCHED_IN;
    if (vert > 0) vst = XmSHADOW_ETCHED_IN;
  
    XtVaSetValues(CNCRTZE(w), XmNsepHType, hst, XmNsepVType, vst, NULL);
#endif
}

void      XhGridSetSize     (XhWidget w, int cols, int rows)
{
#if   USE_WIDGET == GRIDBOX
#elif USE_WIDGET == TABLE
#else
    if (cols >= 0) XtVaSetValues(CNCRTZE(w), XmNcolumns,  cols, NULL);
    if (rows >= 0) XtVaSetValues(CNCRTZE(w), XmNrows,     rows, NULL);
#endif
}

void      XhGridSetChildPosition (XhWidget w, int  x,   int  y)
{
#if   USE_WIDGET == GRIDBOX
    XtVaSetValues(CNCRTZE(w),
                  XtNgridx, x,
                  XtNgridy, y,
                  NULL);
#elif USE_WIDGET == TABLE
    XtTblPosition(CNCRTZE(w), x, y);
#else
#if 1
    XtVaSetValues(CNCRTZE(w),
                  XmNcellX, x,
                  XmNcellY, y,
                  NULL);
#endif
#endif
}

void      XhGridGetChildPosition (XhWidget w, int *x_p, int *y_p)
{
#if   USE_WIDGET == GRIDBOX
  Position  x, y;
    
    XtVaGetValues(CNCRTZE(w),
                  XtNgridx, &x,
                  XtNgridy, &y,
                  NULL);
#elif USE_WIDGET == TABLE
  int x = -1, y = -1;
    
  XtErrorMsg(__FUNCTION__, "unimplemented", "XtToolkitError",
             "XhGridGetChildPosition isn't implemented for Table-based XhGrid",
             (String *) NULL, (Cardinal *) NULL);
#else
  short  x, y;
    
    XtVaGetValues(CNCRTZE(w),
                  XmNcellX, &x,
                  XmNcellY, &y,
                  NULL);
#endif
    
    if (x_p != NULL) *x_p = x;
    if (y_p != NULL) *y_p = y;
}

static int sign_of(int x)
{
    if (x < 0) return -1;
    if (x > 0) return 1;
    return 0;
}

void      XhGridSetChildAlignment(XhWidget w, int  h_a, int  v_a)
{
#if USE_WIDGET == GRIDBOX  ||  USE_WIDGET == SGL
  int  a[9] =
        {
            NorthWestGravity, NorthGravity,  NorthEastGravity,
            WestGravity,      CenterGravity, EastGravity,
            SouthWestGravity, SouthGravity,  SouthEastGravity
        };
#endif
    
#if   USE_WIDGET == GRIDBOX
    XtVaSetValues(CNCRTZE(w),
                  XtNgravity, a[sign_of(h_a) + 1 + sign_of(v_a) * 3 + 3],
                  NULL);
#elif USE_WIDGET == TABLE
  int  h_o[3] = {TBL_LEFT, 0, TBL_RIGHT};
  int  v_o[3] = {TBL_TOP,  0, TBL_BOTTOM};
    
    XtTblOptions(CNCRTZE(w),
                 h_o[sign_of(h_a) + 1] | v_o[sign_of(v_a) + 1] |
                 TBL_LK_WIDTH | TBL_LK_HEIGHT);
#else
//    fprintf(stdout, "%10s: grav:=%d\n", XtName(w), a[sign_of(h_a) + 1 + sign_of(v_a) * 3 + 3]);
    XtVaSetValues(CNCRTZE(w),
                  XmNcellGravity, a[sign_of(h_a) + 1 + sign_of(v_a) * 3 + 3],
                  NULL);
#endif
}

void      XhGridSetChildFilling  (XhWidget w, Bool h_f, Bool v_f)
{
#if   USE_WIDGET == GRIDBOX
  int  a[4] = {FillNone, FillWidth, FillHeight, FillBoth};
    
    XtVaSetValues(CNCRTZE(w),
                  XtNfill, a[(h_f != 0) + (v_f != 0)*2],
                  NULL);
#elif USE_WIDGET == TABLE
    /* Do nothing -- in fact, we should set LK_WIDTH and LK_HEIGHT options
       to the values of !h_f and !v_f, respectially, but we can't read the
       current value of "options"... :-( */
#else
    XtVaSetValues(CNCRTZE(w),
                  XmNfillType, (h_f != 0) * 1 + (v_f != 0) * 2,
                  NULL);
#endif
}

