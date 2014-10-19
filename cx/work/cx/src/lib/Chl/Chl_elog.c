#include "Chl_includes.h"

#include <Xm/Text.h>

#include "elog_api.h"


typedef elogrec_t dlgrec_t;


void CallELogWindow(XhWindow win)
{
  dlgrec_t       *rec = &(Priv2Of(win)->el);

  Widget          form;
  Widget          title_l;
  Widget          text_l;
  XmString        s;

    if (!(rec->initialized))
    {
        rec->box = XhCreateStdDlg(win, "elog", "ELog", NULL, "Сделать запись в e-logbook",
                                  XhStdDlgFOk | XhStdDlgFCancel);

        form = XtVaCreateManagedWidget("form", xmFormWidgetClass, rec->box,
                                       XmNshadowThickness, 0,
                                       NULL);

        title_l = XtVaCreateManagedWidget("collabel", xmLabelWidgetClass, form,
                                          XmNlabelString, s = XmStringCreateLtoR("Заголовок", xh_charset),
                                          NULL);
        XmStringFree(s);

        rec->title_w = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, form,
                                               XmNscrollHorizontal, False,
                                               XmNcursorPositionVisible, False,
                                               XmNeditable,              True,
                                               XmNtraversalOn,           True,
                                               XmNcolumns,               60,
                                               NULL);
        SetCursorCallback(rec->title_w);
        attachtop(rec->title_w, title_l, 0);
        
        text_l = XtVaCreateManagedWidget("collabel", xmLabelWidgetClass, form,
                                         XmNlabelString, s = XmStringCreateLtoR("Текст", xh_charset),
                                         NULL);
        XmStringFree(s);
        attachtop(text_l, rec->title_w, 0);
        
        rec->text_w = XtVaCreateManagedWidget("text_i", xmTextWidgetClass, form,
                                              XmNscrollHorizontal, False,
                                              XmNcursorPositionVisible, False,
                                              XmNeditable,              True,
                                              XmNtraversalOn,           True,
                                              XmNcolumns,               60,
                                              XmNeditMode,              XmMULTI_LINE_EDIT,
                                              XmNrows,                  10,
                                              NULL);
        SetCursorCallback(rec->text_w);
        attachtop(rec->text_w, text_l, 0);
        
        rec->initialized = 1;
    }

    if (XtIsManaged(rec->box)) return;
    XhStdDlgShow(rec->box);
}
