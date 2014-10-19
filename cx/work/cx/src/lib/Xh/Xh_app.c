#include "Xh_includes.h"

#define TOUCH_CURSOR

#ifdef TOUCH_CURSOR
#include "arrow32_src.xbm"
#include "arrow32_msk.xbm"
static XColor  a_fore = {0, 65535,     0,     0};  /* red */
static XColor  a_back = {0, 65535, 65535, 65535};  /* white */

static void ChangeHookCB(Widget     w __attribute__ ((unused)),
                         XtPointer  closure,
                         XtPointer  call_data)
{
  XtChangeHookDataRec *info = (XtChangeHookDataRec *) call_data;
  Cursor               cur  = (Cursor)                closure;

    if (info->type == XtHrealizeWidget  &&  XtIsShell(info->widget))
    {
        XDefineCursor(XtDisplay(info->widget), XtWindow(info->widget), cur);
#if 0
        if(1)
        {
          Widget  xms;
          
            xms = XmGetXmScreen(XtScreen(info->widget));
            if (xms != NULL  &&  0)
            {
                XtVaGetValues(xms, XmNmenuCursor, &cur, NULL);
                XRecolorCursor(XtDisplay(info->widget), cur, &a_fore, &a_back);
            }
        }
#endif
    }
}
#endif /* TOUCH_CURSOR */

#define TRY_HOTKEYS 0

#if TRY_HOTKEYS
static XtActionsRec actions[] =
{
    {TOOLBARHANDLEKEY2CMD_PROC, ToolbarHandleKey2CmdAP},
};
#endif /* TRY_HOTKEYS */

void  XhInitApplication(int         *argc,     char **argv,
                        const char  *app_name,
                        const char  *app_class,
                        const char **fallbacks)
{
    XtSetLanguageProc(NULL, NULL, NULL);
    
    XtToolkitInitialize();
    xh_context = XtCreateApplicationContext();

    XtAppSetFallbackResources(xh_context, (String*)fallbacks);
#if TRY_HOTKEYS
    XtAppAddActions          (xh_context, actions, countof(actions));
#endif /* TRY_HOTKEYS */
    
    TheDisplay = XtOpenDisplay(xh_context, NULL,
                               app_name, app_class,
                               NULL, 0,
                               argc, argv);
    if (!TheDisplay) {
	XtWarning ("Can't open display\n");
	exit(0);
    }
    _XhColorPatchXrmDB(TheDisplay);
#ifdef TOUCH_CURSOR
    {
      Widget         hookobj;
      Pixmap         a_src;
      Pixmap         a_msk;
      Cursor         cur;
      Window         win;

        hookobj = XtHooksOfDisplay(TheDisplay);
        if (hookobj != NULL)
        {
#if 0
  #include <X11/cursorfont.h>
            cur = XCreateFontCursor(TheDisplay, XC_top_left_arrow);
            XRecolorCursor(TheDisplay, cur, &a_fore, &a_back);
#else
            win = DefaultRootWindow(TheDisplay);
            a_src = XCreateBitmapFromData(TheDisplay, win, arrow32_src_bits, arrow32_src_width, arrow32_src_height);
            a_msk = XCreateBitmapFromData(TheDisplay, win, arrow32_msk_bits, arrow32_msk_width, arrow32_msk_height);
            cur = XCreatePixmapCursor(TheDisplay, a_src, a_msk, &a_fore, &a_back, 1, 1);
            XFreePixmap(TheDisplay, a_msk);
            XFreePixmap(TheDisplay, a_src);
#endif

            XtAddCallback(hookobj, XtNchangeHook, ChangeHookCB, (XtPointer)cur);
        }
    }
#endif /* TOUCH_CURSOR */
}

void  XhRunApplication(void)
{
    XtAppMainLoop(xh_context);
}
