#include "Xh_includes.h"


#define TRY_HOTKEYS 0

enum {
    BUTTONS_SPACING = 4,
    BUTTONS_PADDING = 2,

    SEPARATOR_SIZE = 8,

    MAX_BUTTONS = 100
};


enum {TOOLBUTTON_PRIVREC_MAGIC = 0xDEADBABA};

typedef struct
{
    unsigned int  magic;
    int           type;
    int           cmd;
    int           pressed;
} privrec_t;


static void toolCB(Widget     w,
                   XtPointer  closure,
                   XtPointer  call_data  __attribute__((unused)))
{
  int            cmd    = ptr2lint(closure);
  XhWindow       window = XhWindowOf(ABSTRZE(w));
  XhCommandProc  proc   = (XhCommandProc)(window->commandproc);
  privrec_t     *privrec;

    XtVaGetValues(w, XmNuserData, &privrec, NULL);
    if (privrec != NULL)
    {
        if (privrec->type == XhACT_CHECKBOX)
        {
            XhInvertButton(ABSTRZE(w));
            privrec->pressed = !(privrec->pressed);
        }
    }
  
    if (proc == NULL) return;
    proc(window, cmd);
}

#if TRY_HOTKEYS

static void HotkeyHandler(Widget     w  __attribute__((unused)),
                          XtPointer  closure,
                          XEvent    *event,
                          Boolean   *continue_to_dispatch)
{
  XKeyEvent    *ev = (XKeyEvent *)event;
  
    fprintf(stderr, "%s(w=%p)\n!", __FUNCTION__, w);
}


void ToolbarHandleKey2CmdAP(Widget    w,
                            XEvent   *event   __attribute__((unused)),
                            String   *params,
                            Cardinal *num_params)
{
  int   cmd;
  char *endptr;
  
#if 1
    {
      int  x;
        
        fprintf(stderr, "%s([%d]", __FUNCTION__, *num_params);
        if (*num_params > 0)
        {
            fprintf(stderr, "=");
            for (x = 0;  x < *num_params;  x++)
            {
                if (x > 0) fprintf(stderr, ", ");
                fprintf(stderr, params[x]);
            }
        }
        fprintf(stderr, ")\n");
    }
#endif
    
    if (*num_params == 1)
    {
        cmd = strtol(params[0], &endptr, 10);
        if (endptr != params[0]  &&  *endptr == '\0')
        {
            /*!!! Should perform the same actions as in toolCB() here, including XhInvertButton() */
            /* Where to get privrec? Also via parameter, as a numeric string? */
        }
        else
        {
            /* Bark somehow? */
        }
    }
}

#endif /* TRY_HOTKEYS */

static void destroyCB(Widget     w,
                      XtPointer  closure    __attribute__((unused)),
                      XtPointer  call_data  __attribute__((unused)))
{
  void *privrec;
    
    XtVaGetValues(w, XmNuserData, &privrec, NULL);
    if (privrec != NULL) XtFree(privrec);
}

void XhCreateToolbar(XhWindow window, actdescr_t  buttons[])
{
  Dimension       fst;
  Widget          toolBar;
  Widget          container;
  Widget          tools[MAX_BUTTONS];
  Widget          toolSepr;
  Widget          filler;

  XmString        s;

  int             b;
  int             count;
  int             firstflushed;
  int             leftlimit;
  privrec_t      *privrec;

  XtTranslations  trans;
  char            transl_buf[1000];
    
    if (buttons == NULL) return;

    /* First, create a container */
    XtVaGetValues(window->mainForm, XmNshadowThickness, &fst, NULL);
  
    toolBar = XtVaCreateManagedWidget("toolBar", xmFormWidgetClass, window->mainForm,
                                      XmNleftAttachment,  XmATTACH_FORM,
                                      XmNrightAttachment, XmATTACH_FORM,
                                      XmNtopAttachment,   XmATTACH_FORM,
                                      XmNleftOffset,      fst,
                                      XmNrightOffset,     fst,
                                      XmNtopOffset,       fst,
                                      NULL);
    window->toolBar = toolBar;

    container = XtVaCreateManagedWidget("zzz", xmFormWidgetClass, toolBar,
                                        XmNshadowThickness, 0,
                                        XmNleftAttachment,  XmATTACH_FORM,
                                        XmNleftOffset,      BUTTONS_PADDING,
                                        XmNrightAttachment, XmATTACH_FORM,
                                        XmNrightOffset,     BUTTONS_PADDING,
                                        XmNtopAttachment,   XmATTACH_FORM,
                                        XmNtopOffset,       BUTTONS_PADDING,
                                        NULL);
    window->toolHolder = container;

    for (b = 0, firstflushed = -1, count = 0;
         b < MAX_BUTTONS  &&  buttons[b].type != XhACT_END;
         b++)
    {
        if (buttons[b].type == XhACT_FILLER)
        {
            firstflushed = count;
            goto NEXT_BUTTON;
        }

        switch (buttons[b].type)
        {
            case XhACT_SEPARATOR:
#if 1
                // Case 1: a vertical line
                tools[count] = XtVaCreateManagedWidget("toolSepr", xmSeparatorWidgetClass, container,
                                                       XmNorientation,      XmVERTICAL,
                                                       XmNtopAttachment,    XmATTACH_FORM,
                                                       XmNbottomAttachment, XmATTACH_FORM,
                                                       NULL);
#else
                // Case 2: just a spacer
                tools[count] = XtVaCreateManagedWidget("toolSepr", widgetClass, container,
                                                       XmNborderWidth,     0,
                                                       XmNwidth,           4,
                                                       XmNheight,          1,
                                                       NULL);
#endif
                break;

            case XhACT_LEDS:
                tools[count] = XtVaCreateManagedWidget("alarmLeds", xmFormWidgetClass, container,
                                                       XmNtraversalOn, False,
                                                       NULL);
                attachtop   (tools[count], NULL, 0);
                attachbottom(tools[count], NULL, 0);

                window->alarmLeds = tools[count];
                break;

            case XhACT_LABEL:
                tools[count] = XtVaCreateManagedWidget("toolLabel", xmLabelWidgetClass, container,
                                                       XmNlabelString, s = XmStringCreateLtoR(buttons[b].label, xh_charset),
                                                       NULL);
                XmStringFree(s);
                attachtop   (tools[count], NULL, 0);
                attachbottom(tools[count], NULL, 0);
                break;

            case XhACT_NOP:
                count--; // To undo following "count++"
                break;
                
            default:
                privrec = (void *)XtMalloc(sizeof(*privrec));
                bzero(privrec, sizeof(*privrec));
                privrec->magic = TOOLBUTTON_PRIVREC_MAGIC;
                privrec->type  = buttons[b].type;
                privrec->cmd   = buttons[b].cmd;
                
                tools[count] = XtVaCreateManagedWidget("toolButton", xmPushButtonWidgetClass, container,
                                                       XmNtraversalOn, False,
                                                       XmNlabelType,   XmPIXMAP,
                                                       XmNuserData,    (XtPointer)privrec,
                                                       NULL);
                XhAssignPixmap(ABSTRZE(tools[count]), buttons[b].pixmap);
                XhSetBalloon(ABSTRZE(tools[count]), buttons[b].tip);

                XtAddCallback(tools[count], XmNactivateCallback,
                              toolCB, lint2ptr(buttons[b].cmd));
                XtAddCallback(tools[count], XmNdestroyCallback,
                              destroyCB, NULL);

                if (buttons[b].hotkey != NULL  &&  buttons[b].hotkey[0] != '\0')
                {
                    check_snprintf(transl_buf, sizeof(transl_buf),
                                   "%s:%s(%d)",
                                   buttons[b].hotkey,
                                   TOOLBARHANDLEKEY2CMD_PROC, buttons[b].cmd);
                    
                    trans = XtParseTranslationTable(transl_buf);
                    //XtOverrideTranslations(window->shell, trans);

#if TRY_HOTKEYS
                    XtAddEventHandler(tools[count], KeyPressMask, False, HotkeyHandler, NULL);
                    XtGrabKey(tools[count],
                              68, AnyModifier,
                              True, GrabModeAsync, GrabModeAsync);
#endif
                }
        }

        count++;
        
 NEXT_BUTTON:;
    }

    leftlimit = count;
    if (firstflushed >= 0  &&  firstflushed < count)
    {
        for (b = count - 1;  b >= firstflushed;  b--)
        {
            if (b == count - 1)
                attachright(tools[b], NULL, 0*BUTTONS_PADDING);
            else
                attachright(tools[b], tools[b + 1], BUTTONS_SPACING);
        }
        
        leftlimit = firstflushed;

        if (firstflushed > 0)
        {
            filler = XtVaCreateManagedWidget("", widgetClass, container,
                                             XmNborderWidth,     0,
                                             NULL);
            attachleft (filler, tools[firstflushed - 1], 0);
            attachright(filler, tools[firstflushed],     0);
        }
    }
    for (b = 0;  b < leftlimit;  b++)
    {
        //attachtop(tools[b], NULL, BUTTONS_PADDING);

        if (b == 0)
            attachleft(tools[b], NULL, 0*BUTTONS_PADDING);
        else
            attachleft(tools[b], tools[b - 1], BUTTONS_SPACING);
    }

    toolSepr = XtVaCreateManagedWidget("toolLine", xmSeparatorWidgetClass, toolBar,
                                       XmNorientation,     XmHORIZONTAL,
                                       XmNleftAttachment,  XmATTACH_FORM,
                                       XmNleftOffset,      0,
                                       XmNrightAttachment, XmATTACH_FORM,
                                       XmNrightOffset,     0,
                                       XmNtopAttachment,   XmATTACH_WIDGET,
                                       XmNtopWidget,       container,
                                       XmNtopOffset,       BUTTONS_PADDING,
                                       NULL);
}


void XhSetCommandOnOff  (XhWindow window, int cmd, int is_pushed)
{
  WidgetList  children;
  Cardinal    numChildren;
  Cardinal    i;
  privrec_t  *privrec;

    if (window->toolHolder == NULL) return;
  
//    fprintf(stderr, "%s(,cmd=%d, on=%d)\n", __FUNCTION__, cmd, is_pushed);
    is_pushed = (is_pushed != 0);
  
    XtVaGetValues(window->toolHolder,
                  XmNchildren,    &children,
                  XmNnumChildren, &numChildren,
                  NULL);
    
    for (i = 0;  i < numChildren;  i++)
        if (XmIsPushButton(children[i]))
        {
            XtVaGetValues(children[i], XmNuserData, &privrec, NULL);
            if (privrec != NULL                               &&
                privrec->magic   == TOOLBUTTON_PRIVREC_MAGIC  &&
                privrec->type    == XhACT_CHECKBOX            &&
                privrec->cmd     == cmd                       &&
                privrec->pressed != is_pushed)
            {
                XhInvertButton(ABSTRZE(children[i]));
                privrec->pressed = !(privrec->pressed);
            }
        }
}

void XhSetCommandEnabled(XhWindow window, int cmd, int is_enabled)
{
  WidgetList  children;
  Cardinal    numChildren;
  Cardinal    i;
  privrec_t  *privrec;

    if (window->toolHolder == NULL) return;
  
    is_enabled = (is_enabled != 0);

    XtVaGetValues(window->toolHolder,
                  XmNchildren,    &children,
                  XmNnumChildren, &numChildren,
                  NULL);
    
    for (i = 0;  i < numChildren;  i++)
        if (XmIsPushButton(children[i]))
        {
            XtVaGetValues(children[i], XmNuserData, &privrec, NULL);
            if (privrec != NULL                               &&
                privrec->magic   == TOOLBUTTON_PRIVREC_MAGIC  &&
                privrec->cmd     == cmd                       &&
                XtIsSensitive(children[i]) != is_enabled)
            {
                XtSetSensitive(children[i], is_enabled);
            }
        }
}
