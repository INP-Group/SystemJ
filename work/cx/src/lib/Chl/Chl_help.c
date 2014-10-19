#include "Chl_includes.h"

#include <Xm/MwmUtil.h>

#include <Xm/DialogS.h>
#include <Xm/Frame.h>
#include <Xm/MessageB.h>


typedef helprec_t dlgrec_t;


typedef struct
{
    int         fgcol, bgcol;
    const char *label;
    const char *comment;
} colhelpline_t;

static colhelpline_t colhelp_contents[] =
{
    {-1, XH_COLOR_BG_NORM_YELLOW, "������",        "�������� ��� ����������� ���������"},
    {-1, XH_COLOR_BG_NORM_RED,    "�������",       "�������� ��� ����������� ���������"},
    {XH_COLOR_FG_WEIRD,
         XH_COLOR_BG_WEIRD,       "��������",      "�������� �������"},
    {-1, XH_COLOR_BG_DEFUNCT,     "������� ����",  "�������� ����� �� �����������"},
    {-1, XH_COLOR_BG_HWERR,       "��������",      "���������� �������� (������ ������ ������� ���� �� �������� - ������)"},
    {-1, XH_COLOR_BG_SFERR,       "��������",      "����������� �������� (������ ������ ������� ���� �� �������� - ������)"},
    {-1, XH_COLOR_BG_OTHEROP,     "���������",     "�������� �������� ������ ����������"},
    {-1, XH_COLOR_BG_JUSTCREATED, "�������������", "������ ��� �� ���� �� ���� ���������"},
    {-1, XH_COLOR_JUST_GREEN,     "�������",       "������ ��� ������� �������� ������������"},
    {-1, XH_COLOR_BG_PRGLYCHG,    "�������������", "�������� �������� ����������"},
//    {-1, XH_COLOR_BG_NOTFOUND,    "����������",    "����� �� ������"},
};

typedef struct
{
    const char *keys;
    const char *comment;
} keyhelpline_t;

static keyhelpline_t keyhelp_contents[] =
{
    {"<Tab>/Shift+<Tab>",
        "����������� �� �������"},
    {"������� �����/����\n(� ��������� ����)",
        "����������/���������� �������� �� ���\n"
            "��������� ��� ���� Ctrl ��������� ��� � 10 ���\n"
            "��������� Alt ����������� ��� � 10 ���"},
    {"<Enter>",
        "���������� �����"},
    {"<Esc>",
        "������ ����� (�� ������� <Enter>!)"},
    {"", NULL},
    {"<������ ������ ����>\nAlt+<Enter>\nShift+<F10>",
        "����� ���� \xab�������� ������\xbb"},
    {"Ctrl+<������ ������ ����>\nCtrl+<������>",
        "����� �������� ������� �������"},
    {"Shift+<������ ������ ����>\nShift+<������>",
        "�������� ������ �� ������-���������"},
};

void   ChlShowHelp      (XhWindow window, int parts)
{
  dlgrec_t      *rec = &(Priv2Of(window)->hr);

  Widget         w;
  Widget         form;
  Widget         frame;
  Widget         grid;
  Widget         prev = NULL;
  XmString       s;
  colhelpline_t *clp;
  keyhelpline_t *klp;
  int            row;
  int            fg, bg;
  
    if (!(rec->initialized))
    {
        rec->box = XhCreateStdDlg(window, "help", "Help", NULL, "������� �������",
                                  XhStdDlgFOk);

        form = XtVaCreateManagedWidget("form", xmFormWidgetClass, rec->box,
                                       XmNshadowThickness, 0,
                                       NULL);

        /* List of colors */
        if (parts & CHL_HELP_COLORS)
        {
            frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, form,
                                            XmNmarginWidth, 1,
                                            NULL);
            attachtop(frame, prev, 0);
            prev = frame;
            
            XtVaCreateManagedWidget("title", xmLabelWidgetClass, frame,
                                    XmNframeChildType, XmFRAME_TITLE_CHILD,
                                    XmNlabelString, s = XmStringCreateLtoR("�����", xh_charset),
                                    NULL);
            XmStringFree(s);
            
            grid = XhCreateGrid(frame, "grid");
            XhGridSetGrid   (grid, 0, 0);
            XhGridSetSpacing(grid, 0, 0);
            XhGridSetPadding(grid, 0, 0);
            
            for (row = 0, clp = colhelp_contents;
                 row < countof(colhelp_contents);
                 row++,   clp++)
            {
                fg = clp->fgcol; if (fg < 0) fg = XH_COLOR_FG_DEFAULT;
                bg = clp->bgcol; if (bg < 0) bg = XH_COLOR_BG_DEFAULT;
                
                /* A model... */
                w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                            XmNlabelString, s = XmStringCreateLtoR(clp->label, xh_charset),
                                            XmNalignment,  XmALIGNMENT_BEGINNING,
                                            XmNforeground, XhGetColor(fg),
                                            XmNbackground, XhGetColor(bg),
                                            NULL);
                XmStringFree(s);
                
                XhGridSetChildPosition (w, 0,   row);
                XhGridSetChildFilling  (w, 1,   0);
                XhGridSetChildAlignment(w, -1, -1);
                
                /* ...and a comment */
                w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                            XmNlabelString, s = XmStringCreateLtoR(clp->comment, xh_charset),
                                            XmNalignment,  XmALIGNMENT_BEGINNING,
                                            NULL);
                XmStringFree(s);
                
                XhGridSetChildPosition (w, 1,   row);
                XhGridSetChildFilling  (w, 0,   0);
                XhGridSetChildAlignment(w, -1, -1);
            }
        }

        /* List of key/mouse bindings */
        if (parts & CHL_HELP_KEYS)
        {
            frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, form,
                                            XmNmarginWidth, 1,
                                            NULL);
            attachtop(frame, prev, 0);
            prev = frame;
            
            XtVaCreateManagedWidget("title", xmLabelWidgetClass, frame,
                                    XmNframeChildType, XmFRAME_TITLE_CHILD,
                                    XmNlabelString, s = XmStringCreateLtoR("������������� ���������� � ����", xh_charset),
                                    NULL);
            XmStringFree(s);
            
            grid = XhCreateGrid(frame, "grid");
            XhGridSetGrid   (grid, 0, 0);
            XhGridSetSpacing(grid, 0, 0);
            XhGridSetPadding(grid, 0, 0);
            
            for (row = 0, klp = keyhelp_contents;
                 row < countof(keyhelp_contents);
                 row++,   klp++)
            {
                /* Have we reached keys/mouse-elem separator? */
                if (klp->keys[0] == '\0')
                {
                    if (parts & CHL_HELP_MOUSE) continue;
                    else                        break;
                }

                /* A model... */
                w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                            XmNlabelString, s = XmStringCreateLtoR(klp->keys, xh_charset),
                                            XmNalignment,  XmALIGNMENT_END,
                                            NULL);
                XmStringFree(s);
                
                XhGridSetChildPosition (w, 0,   row);
                XhGridSetChildFilling  (w, 0,   0);
                XhGridSetChildAlignment(w, +1, -1);
                
                /* ...and a comment */
                w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, grid,
                                            XmNlabelString, s = XmStringCreateLtoR(klp->comment, xh_charset),
                                            XmNalignment,  XmALIGNMENT_BEGINNING,
                                            NULL);
                XmStringFree(s);
                
                XhGridSetChildPosition (w, 1,   row);
                XhGridSetChildFilling  (w, 0,   0);
                XhGridSetChildAlignment(w, -1, -1);
            }
        }
        
        rec->initialized = 1;
    }

    if (XtIsManaged(rec->box)) 
        XhStdDlgHide(rec->box);
    else
        XhStdDlgShow(rec->box);
}

void   ChlShowHelpWindow(XhWindow window, chl_helppart_t *parts, int nparts)
{
  dlgrec_t       *rec = &(Priv2Of(window)->hr);

  Widget          w;
  Widget          form;
  XmString        s;

  int             n;
  chl_helppart_t *item;
  Widget          prev;

  Widget          parent;
  Widget          to_place;
  Widget          the_thing;
  
    if (!(rec->initialized))
    {
        rec->box = XhCreateStdDlg(window, "help", "Help", NULL, "������� �������",
                                  XhStdDlgFOk);

        form = XtVaCreateManagedWidget("form", xmFormWidgetClass, rec->box,
                                       XmNshadowThickness, 0,
                                       NULL);

        for (n = 0,      item = parts, prev = NULL;
             n < nparts;
             n++)
        {
            if (item->label != NULL)
            {
                parent = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, form,
                                                 NULL);

                XtVaCreateManagedWidget("title", xmLabelWidgetClass, parent,
                                        XmNframeChildType, XmFRAME_TITLE_CHILD,
                                        XmNlabelString, s = XmStringCreateLtoR(item->label, xh_charset),
                                        NULL);
                XmStringFree(s);

                to_place = parent;
            }
            else
            {
                parent = form;
                to_place = NULL;
            }

            switch (item->kind)
            {
                case CHL_HELP_PART_COLORS:
                    break;
                    
                case CHL_HELP_PART_KEYS:
                    the_thing = XhCreateGrid(parent, "grid");
                    XhGridSetGrid(the_thing, 0, 0);
            

                    break;
                    
                case CHL_HELP_PART_TEXT:
                    break;
            }

            if (to_place == NULL) to_place = the_thing;
            attachtop(to_place, prev, 0);
            prev = to_place;
        }
        
        
        rec->initialized = 1;
    }

    if (XtIsManaged(rec->box)) return;
    XhStdDlgShow(rec->box);
}

