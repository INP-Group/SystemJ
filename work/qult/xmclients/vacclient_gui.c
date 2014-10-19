#include "vacclient_includes.h"

#include "Xh_fallbacks.h"
#include "Xh_globals.h"

#include "pixmaps/btn_open.xpm"
#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_log.xpm"
#include "pixmaps/btn_help.xpm"
#include "pixmaps/btn_quit.xpm"

#include "pixmaps/btn_histogram.xpm"
#include "pixmaps/btn_scale.xpm"


static actdescr_t toolslist[] =
{
    XhXXX_TOOLCMD(cmChlNfLoadMode,  "��������� �����",         btn_open_xpm),
    XhXXX_TOOLCMD(cmChlNfSaveMode,  "��������� �����",         btn_save_xpm),
    XhXXX_SEPARATOR(),
    XhXXX_TOOLCHK(cmChlSwitchLog,   "���������� ����������",   btn_log_xpm),
    XhXXX_SEPARATOR(),
    XhXXX_TOOLCHK(cmSwitchView,     "�����/�����������",       btn_histogram_xpm),
    XhXXX_TOOLCHK(cmSwitchScale,    "��������������� �������", btn_scale_xpm),
    XhXXX_TOOLLEDS(),
    XhXXX_TOOLCMD(cmChlHelp,        "������� �������",         btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD(cmChlQuit,        "����� �� ���������",      btn_quit_xpm),
    XhXXX_END()
};

static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {
        
        case cmSwitchView:
            SetDisplayedPart (win, -1);
            break;

        case cmSwitchScale:
            SetDisplayedScale(win, -1);
            break;

        default:
            ChlHandleStdCommand(win, cmd);
            break;
    }
}

static char *fallbacks[] = {XH_STANDARD_FALLBACKS};

XhWindow  CreateApplicationWindow(int argc, char *argv[])
{
  XhWindow  win;
  
//    XhSetColorBinding("GRAPH_BAR1", "#00A000");
//    XhSetColorBinding("GRAPH_BAR1", "#00c080");
    XhInitApplication(&argc, argv, app_name, app_class, fallbacks);
    win = XhCreateWindow(app_title, app_name, app_class,
                         XhWindowToolbarMask | XhWindowStatuslineMask | XhWindowUnresizableMask*1,
                         NULL,
                         toolslist);
    XhSetWindowIcon(win, app_icon);

    XhSetWindowCmdProc(win, CommandProc);
                         
    return win;
}

void      RunApplication         (void)
{
    XhRunApplication();
}
