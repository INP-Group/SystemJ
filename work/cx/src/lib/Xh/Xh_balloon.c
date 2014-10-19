#include "Xh_includes.h"

#if 1
#include "XmLiteClue.h"

void XhInitializeBalloon(XhWindow window)
{
    if (window->clue == NULL)
    {
        window->clue = XtVaCreatePopupShell
            ("tipShell", xmLiteClueWidgetClass, window->shell,
             NULL);
    }
}

void XhSetBalloon(XhWidget w, const char *text)
{
  Widget    xw     = CNCRTZE(w);
  XhWindow  window = XhWindowOf(w);
  
    if (window != NULL) XmLiteClueAddWidget(window->clue, xw, text);
}
#else
#include "LiteClue.h"

static Widget clue = NULL;

void XhInitializeBalloon(XhWindow window)
{
    if (clue == NULL)
    {
        clue = XtVaCreatePopupShell
            ("tipShell", xcgLiteClueWidgetClass, window->shell,
             NULL);
    }
}

void XhSetBalloon(XhWidget w, const char *text)
{
  Widget    xw     = CNCRTZE(w);
  
    XcgLiteClueAddWidget(clue, xw, text, 0, 0);
}
#endif
