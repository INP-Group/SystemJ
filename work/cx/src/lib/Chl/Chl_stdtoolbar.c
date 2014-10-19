#include "Chl_includes.h"

#include "pixmaps/btn_open.xpm"
#include "pixmaps/btn_save.xpm"
#include "pixmaps/btn_quit.xpm"
#include "pixmaps/btn_log.xpm"
#include "pixmaps/btn_elog.xpm"
#include "pixmaps/btn_pause.xpm"
#include "pixmaps/btn_help.xpm"


static actdescr_t stdtoolslist[] =
{
    XhXXX_TOOLCMD(cmChlNfLoadMode, "��������� �����",            btn_open_xpm),
    XhXXX_TOOLCMD(cmChlNfSaveMode, "��������� �����",            btn_save_xpm),
    XhXXX_SEPARATOR(),
    XhXXX_TOOLCHK(cmChlSwitchLog,  "���������� ����������",      btn_log_xpm),
    //XhXXX_TOOLCMD(cmChlDoElog,     "������� ������ � e-logbook", btn_elog_xpm),
    XhXXX_TOOLCHK(cmChlFreeze,     "������� �����, ������� \253�����\273",
                                                                 btn_pause_xpm),
    XhXXX_TOOLLEDS(),
    XhXXX_TOOLCMD(cmChlHelp,       "������� �������",            btn_help_xpm),
    XhXXX_TOOLFILLER(),
    XhXXX_TOOLCMD(cmChlQuit,       "����� �� ���������",         btn_quit_xpm),
    XhXXX_END()
};

actdescr_t *ChlGetStdToolslist(void)
{
    return stdtoolslist;
}
