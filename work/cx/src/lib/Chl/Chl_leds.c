#include "Chl_includes.h"


static void ledCB(Widget     w,
                  XtPointer  closure    __attribute__((unused)),
                  XtPointer  call_data  __attribute__((unused)))
{
    XtVaSetValues(w, XmNset, False, NULL);
}

static void BlockButton2Handler(Widget     w        __attribute__((unused)),
                                XtPointer  closure  __attribute__((unused)),
                                XEvent    *event,
                                Boolean   *continue_to_dispatch)
{
  XButtonEvent *ev  = (XButtonEvent *)event;
  
    if (ev->type == ButtonPress  &&  ev->button == Button2)
        *continue_to_dispatch = False;
}

int leds_create    (XhWidget parent_grid, int size,
                    ledinfo_t *leds, int count,
                    cda_serverid_t mainsid)
{
  int          ncns;
  int          cn;
  XhWidget     w;
  XmString     s;
  const char  *srvname;
  int          no_decor = 0;
  
    if ((ncns = cda_status_lastn(mainsid)) < 0) return -1;
    ncns++;
    if (ncns > count) ncns = count;

    if (size < 0)
    {
        size = -size;
        no_decor = 1;
    }
    if (size <= 1) size = 15;
    
    XhGridSetSize(parent_grid, ncns, 1);
    for (cn = 0;  cn < ncns;  cn++)
    {
        w = XtVaCreateManagedWidget("LED",
                                    xmToggleButtonWidgetClass,
                                    parent_grid,
                                    XmNtraversalOn,    False,
                                    XmNlabelString,    s = XmStringCreateSimple(""),
                                    XmNindicatorSize,  abs(size),
                                    XmNspacing,        0,
                                    XmNindicatorOn,    XmINDICATOR_CROSS,
                                    XmNindicatorType,  XmONE_OF_MANY_ROUND,
                                    XmNset,            False,
                                    NULL);
        XmStringFree(s);
        if (no_decor)
            XtVaSetValues(w,
                          XmNmarginWidth,  0,
                          XmNmarginHeight, 0,
                          NULL);
        XhGridSetChildAlignment(w, 0, 0);

        srvname = cda_status_srvname(mainsid, cn);
        if (srvname == NULL  ||  srvname[0] == '\0') srvname = getenv("CX_SERVER");
        if (srvname == NULL  ||  srvname[0] == '\0') srvname = "(default)";
        XhSetBalloon     (w, srvname);
        XtAddCallback    (w, XmNvalueChangedCallback, ledCB, NULL);
        XtAddEventHandler(w, ButtonPressMask, False, BlockButton2Handler, NULL); // Temporary countermeasure against Motif Bug#1117
        
        leds[cn].w = w;
        leds_set_status(&(leds[cn]), cda_status_of(mainsid, cn));
    }

    return 0;
}

int leds_update    (ledinfo_t *leds, int count,
                    cda_serverid_t mainsid)
{
  int          ncns;
  int          cn;

    if (leds == NULL  ||  count <= 0) return 0;
  
    if ((ncns = cda_status_lastn(mainsid)) < 0) return -1;
    ncns++;
    if (ncns > count) ncns = count;

    for (cn = 0;  cn < ncns;  cn++)
        leds_set_status(leds + cn, cda_status_of(mainsid, cn));

    return 0;
}

int leds_set_status(ledinfo_t *led, int status)
{
  int  idx;
    
    idx                                              = XH_COLOR_JUST_GREEN;
    if (status == CDA_SERVERSTATUS_FROZEN)       idx = XH_COLOR_BG_DEFUNCT;
    if (status == CDA_SERVERSTATUS_ALMOSTREADY)  idx = XH_COLOR_JUST_YELLOW;
    if (status == CDA_SERVERSTATUS_DISCONNECTED) idx = XH_COLOR_JUST_RED;
    
    XtVaSetValues(led->w, XmNunselectColor, XhGetColor(idx), NULL);
    
    led->status = status;

    return 0;
}
