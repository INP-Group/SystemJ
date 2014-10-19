#include "Chl_includes.h"

#if XM_TABBER_AVAILABLE

#if (XmVERSION*1000+XmREVISION) < 2003
  #define _XmExt_h_
  #define XmNstackedEffect "stackedEffect"
  #define XmNtabSide "tabSide"
  #define XmNtabSelectedCallback "tabSelectedCallback"
  #define XmNtabSelectPixmap "tabSelectPixmap"
  #define XmNtabSelectColor "tabSelectColor"
  #define XmNtabRaisedCallback "tabRaisedCallback"
  #define XmNtabPixmapPlacement "tabPixmapPlacement"
  #define XmNtabOrientation "tabOrientation"
  #define XmNtabOffset "tabOffset"
  #define XmNtabMode "tabMode"
  #define XmNtabMarginWidth "tabMarginWidth"
  #define XmNtabMarginHeight "tabMarginHeight"
  #define XmNtabList "tabList"
  #define XmNtabLabelString "tabLabelString"
  #define XmNtabLabelSpacing "tabLabelSpacing"
  #define XmNtabLabelPixmap "tabLabelPixmap"
  #define XmNtabForeground "tabForeground"
  #define XmNtabEdge "tabEdge"
  #define XmNtabCornerPercent "tabCornerPercent"
  #define XmNtabBoxWidget "tabBoxWidget"
  #define XmNtabBackgroundPixmap "tabBackgroundPixmap"
  #define XmNtabBackground "tabBackground"
  #define XmNtabAutoSelect "tabAutoSelect"
  #define XmNtabArrowPlacement "tabArrowPlacement"
  #define XmNtabAlignment "tabAlignment"
  #define XmNuniformTabSize ((char*)&_XmStrings[1773])
#endif
#include <Xm/TabStack.h>


static knobs_emethods_t tabber_emethods =
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
    int tab_side;
    int stacked;
} tabberopts_t;

static psp_paramdescr_t text2tabberopts[] =
{
    PSP_P_FLAG("top",     tabberopts_t, tab_side, XmTABS_ON_TOP,    1),
    PSP_P_FLAG("bottom",  tabberopts_t, tab_side, XmTABS_ON_BOTTOM, 0),
    PSP_P_FLAG("left",    tabberopts_t, tab_side, XmTABS_ON_LEFT,   0),
    PSP_P_FLAG("right",   tabberopts_t, tab_side, XmTABS_ON_RIGHT,  0),
    PSP_P_FLAG("oneline", tabberopts_t, stacked,  0,                1),
    PSP_P_FLAG("stacked", tabberopts_t, stacked,  1,                0),
    PSP_P_END()
};

int ELEM_TABBER_Create_m(XhWidget parent, ElemInfo e)
{
  tabberopts_t  opts;
  int           y;
  knobinfo_t   *ki;
  Widget        tabbox;
  Widget        intmdt;
  char          lbuf[1000];
  char          tbuf[1000];
  char         *caption;
  char         *tip;
  XmString      s;

    if (e->uplink != NULL) e->emlink = e->uplink->emlink;
  
    bzero(&opts, sizeof(opts));
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2tabberopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
        opts.tab_side = XmTABS_ON_TOP;
    }

    e->container = e->innage = ABSTRZE(
        XtVaCreateManagedWidget("tabStack", xmTabStackWidgetClass, CNCRTZE(parent),
                                XmNtabSide,         opts.tab_side,
                                XmNtabMode,         opts.stacked? XmTABS_STACKED:XmTABS_BASIC,
                                XmNuniformTabSize,  opts.stacked?           True:False,
                                XmNstackedEffect,   False,
                                XmNtabOffset,       0,
                                XmNmarginWidth,     0,
                                XmNmarginHeight,    0,
                                XmNtabMarginWidth,  0,
                                XmNtabMarginHeight, 0,
                                NULL));
    /* A pseudo-fix for OpenMotif bug#1337 "Keyboard traversal in XmTabStack is nonfunctional" */
    if ((tabbox = XtNameToWidget(CNCRTZE(e->container), "tabBox")) != NULL)
        XtVaSetValues(tabbox, XmNtraversalOn, False, NULL);

    e->emlink = &tabber_emethods;

    for (y = 0;  y < e->count;  y++)
    {
        e->innage =
            /*ABSTRZE(*/intmdt = /*!!! For some reason, gcc-4 barks "invalid lvalue in assignment..." */
                    XtVaCreateManagedWidget("intmdtForm", xmFormWidgetClass, CNCRTZE(e->container),
                                            XmNshadowThickness, 0,
                                            NULL)/*)*/;
        
        ki = e->content + y;
        ChlGiveBirthToKnob(ki);

        get_knob_rowcol_label_and_tip(e->rownames, y, ki,
                                      lbuf, sizeof(lbuf), tbuf, sizeof(tbuf),
                                      &caption, &tip);
        if (*caption == '\0') caption = " "; /* An escape from OpenMotif bug#1346 <<XmTabStack segfaults with side tabs and XmNtabLabelString=="">> */
        XtVaSetValues(intmdt,
                      XmNtabLabelString, s = XmStringCreateLtoR(caption, xh_charset),
                      NULL);
        XmStringFree(s);
    }

    e->innage = e->container;
         
    return 0;
}

#endif /* XM_TABBER_AVAILABLE */
