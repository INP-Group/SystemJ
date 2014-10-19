#include "Chl_includes.h"

#include <Xm/MwmUtil.h>

#include <Xm/DialogS.h>
#include <Xm/Frame.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>

#include "KnobsI.h" // For SetTextString()


typedef bigvalsdlg_t dlgrec_t;
static dlgrec_t *ThisDialog(XhWindow win);

static void RenewBigvRow(dlgrec_t *rec, int  row, Knob  ki)
{
  char     *label;
  XmString  s;

    rec->target[row] = ki;

    label = ki->label;
    if (label == NULL) label = ki->ident;
    if (label == NULL) label = "";
    XtVaSetValues(rec->label[row],
                  XmNlabelString, s = XmStringCreateLtoR(label, xh_charset),
                  NULL);
    XmStringFree(s);
    
    XtVaSetValues(rec->val_dpy[row],
                  XmNcolumns, GetTextColumns(ki->dpyfmt, ki->units),
                  NULL);
    rec->colstate_s[row] = COLALARM_UNINITIALIZED;
}

static void UpdateBigvRow(dlgrec_t *rec, int  row)
{
  Knob   ki = rec->target[row];
  Pixel  fg;
  Pixel  bg;
    
    SetTextString(ki, rec->val_dpy[row], ki->curv);

    if (ki->colstate != rec->colstate_s[row])
    {
        ChooseKnobColors(ki->color, ki->colstate,
                         rec->deffg, rec->defbg,
                         &fg, &bg);
        XtVaSetValues(rec->val_dpy[row],
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
        rec->colstate_s[row] = ki->colstate;
    }
}

static void SetBigvCount(dlgrec_t *rec, int count)
{
  int  row;
    
    rec->rows_used = count;
    
    if (count > 0)
    {
        XtManageChildren(rec->label,   count);
        XtManageChildren(rec->val_dpy, count);
        XtManageChildren(rec->b_up,    count);
        XtManageChildren(rec->b_dn,    count);
        XtManageChildren(rec->b_rm,    count);
    }

    if (count < MAX_BIGVALS)
    {
        XtUnmanageChildren(rec->label   + count, MAX_BIGVALS - count);
        XtUnmanageChildren(rec->val_dpy + count, MAX_BIGVALS - count);
        XtUnmanageChildren(rec->b_up    + count, MAX_BIGVALS - count);
        XtUnmanageChildren(rec->b_dn    + count, MAX_BIGVALS - count);
        XtUnmanageChildren(rec->b_rm    + count, MAX_BIGVALS - count);
    }
    
    for (row = 0;  row < count;  row++)
    {
        XhGridSetChildPosition(rec->label  [row], 0, row);
        XhGridSetChildPosition(rec->val_dpy[row], 1, row);
        XhGridSetChildPosition(rec->b_up   [row], 2, row);
        XhGridSetChildPosition(rec->b_dn   [row], 3, row);
        XhGridSetChildPosition(rec->b_rm   [row], 4, row);
    }

    XhGridSetSize(rec->grid, 5, count);
}

static void DisplayBigVal(dlgrec_t *rec)
{
  int  row;
  
    for (row = 0;  row < rec->rows_used;  row++)
        UpdateBigvRow(rec, row);
}


static void BigUpCB(Widget     w,
                    XtPointer  closure,
                    XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t *rec = ThisDialog(XhWindowOf(w));
  int       row = ptr2lint(closure);
  Knob      k_this;
  Knob      k_prev;
  
    if (row == 0) return;
    k_this = rec->target[row];
    k_prev = rec->target[row - 1];
    RenewBigvRow (rec, row,     k_prev);
    RenewBigvRow (rec, row - 1, k_this);
    UpdateBigvRow(rec, row);
    UpdateBigvRow(rec, row - 1);
}

static void BigDnCB(Widget     w,
                    XtPointer  closure,
                    XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t *rec = ThisDialog(XhWindowOf(w));
  int       row = ptr2lint(closure);
  Knob      k_this;
  Knob      k_next;

    if (row == rec->rows_used - 1) return;
    k_this = rec->target[row];
    k_next = rec->target[row + 1];
    RenewBigvRow (rec, row,     k_next);
    RenewBigvRow (rec, row + 1, k_this);
    UpdateBigvRow(rec, row);
    UpdateBigvRow(rec, row + 1);
}

static void BigRmCB(Widget     w,
                    XtPointer  closure,
                    XtPointer  call_data  __attribute__((unused)))
{
  dlgrec_t *rec = ThisDialog(XhWindowOf(w));
  int       row = ptr2lint(closure);
  int       y;
  
    if (rec->rows_used == 1) XhStdDlgHide(rec->box);

    for (y = row;  y < rec->rows_used - 1;  y++)
    {
        RenewBigvRow (rec, y, rec->target[y + 1]);
        UpdateBigvRow(rec, y);
    }
    SetBigvCount(rec, rec->rows_used - 1);
}

static void BigvMouse3Handler(Widget     w,
                              XtPointer  closure,
                              XEvent    *event,
                              Boolean   *continue_to_dispatch)
{
  dlgrec_t            *rec = ThisDialog(XhWindowOf(w));
  int                  row = ptr2lint(closure);
  Knob                 ki  = rec->target[row];

    KnobsMouse3Handler(w, (XtPointer)ki, event, continue_to_dispatch);
}

static XhWidget MakeBigvButton(XhWidget grid, int x, int row,
                               const char *caption, XtCallbackProc  callback)
{
  XhWidget  b;
  XmString  s;
  
    b = XtVaCreateWidget("push_o", xmPushButtonWidgetClass, CNCRTZE(grid),
                         XmNtraversalOn, False,
                         XmNsensitive,   1,
                         XmNlabelString, s = XmStringCreateLtoR(caption, xh_charset),
                         NULL);
    XhGridSetChildPosition (b,  x,  row);
    XhGridSetChildFilling  (b,  0,  0);
    XhGridSetChildAlignment(b, -1, -1);
    XtAddCallback(b, XmNactivateCallback, callback, lint2ptr(row));

    return b;
}

static dlgrec_t *ThisDialog(XhWindow win)
{
  dlgrec_t *rec = &(Priv2Of(win)->bv);

  char      buf[100];

  XmString  s;
  
  int       row;

    if (rec->initialized) return rec;

    check_snprintf(buf, sizeof(buf), "%s: Big values", Priv2Of(win)->sysname);
    rec->box = XhCreateStdDlg(win, "bigv", buf, NULL, NULL,
                              XhStdDlgFOk);

    rec->grid = XhCreateGrid(rec->box, "grid");
    XhGridSetGrid   (rec->grid, 0,                       0);
    XhGridSetSpacing(rec->grid, CHL_INTERKNOB_H_SPACING, CHL_INTERKNOB_V_SPACING);
    XhGridSetPadding(rec->grid, 0,                       0);

    for (row = 0;  row < MAX_BIGVALS;  row++)
    {
        rec->label[row] =
            XtVaCreateWidget("rowlabel", xmLabelWidgetClass, rec->grid,
                             XmNlabelString, s = XmStringCreateLtoR("", xh_charset),
                             NULL);
        XmStringFree(s);
        XhGridSetChildPosition (rec->label[row],  0,  row);
        XhGridSetChildFilling  (rec->label[row],  0,  0);
        XhGridSetChildAlignment(rec->label[row], -1, -1);

        rec->val_dpy[row] =
            XtVaCreateWidget("text_o", xmTextWidgetClass, rec->grid,
                             XmNscrollHorizontal,      False,
                             XmNcursorPositionVisible, False,
                             XmNeditable,              False,
                             XmNtraversalOn,           False,
                             NULL);
        XhGridSetChildPosition (rec->val_dpy[row],  1,  row);
        XhGridSetChildFilling  (rec->val_dpy[row],  0,  0);
        XhGridSetChildAlignment(rec->val_dpy[row], -1, -1);
        
        XtInsertEventHandler(rec->label  [row], ButtonPressMask, False, BigvMouse3Handler, lint2ptr(row), XtListHead);
        XtInsertEventHandler(rec->val_dpy[row], ButtonPressMask, False, BigvMouse3Handler, lint2ptr(row), XtListHead);

        rec->b_up[row] = MakeBigvButton(rec->grid, 2, row, "^", BigUpCB);
        rec->b_dn[row] = MakeBigvButton(rec->grid, 3, row, "v", BigDnCB);
        rec->b_rm[row] = MakeBigvButton(rec->grid, 4, row, "-", BigRmCB);
    }

    XtVaGetValues(rec->val_dpy[0],
                  XmNforeground, &(rec->deffg),
                  XmNbackground, &(rec->defbg),
                  NULL);

    rec->initialized = 1;
    
    return rec;
}

void Chl_E_ShowBigValWindow(Knob ki)
{
  dlgrec_t *rec = ThisDialog(XhWindowOf(ki->indicator));
  
  int          row;

    /* Check if this knob is alreany in a list */
    for (row = 0;
         row < rec->rows_used  &&  rec->target[row] != ki;
         row++);
  
    if (row >= rec->rows_used)
    {
    
        if (rec->rows_used >= MAX_BIGVALS)
        {
            for (row = 0;  row < rec->rows_used - 1;  row++)
            {
                RenewBigvRow (rec, row, rec->target[row + 1]);
                UpdateBigvRow(rec, row);
            }
            rec->rows_used--;
        }

        row = rec->rows_used;

        RenewBigvRow (rec, row, ki);
        UpdateBigvRow(rec, row);
        SetBigvCount (rec, row + 1);
    }
    
    XhStdDlgShow(rec->box);
}

void UpdateBigvalsWindow(XhWindow win)
{
  dlgrec_t *rec = &(Priv2Of(win)->bv);
    
    if (!rec->initialized) return;

    if (XtIsManaged(rec->box)) DisplayBigVal(rec);
}
