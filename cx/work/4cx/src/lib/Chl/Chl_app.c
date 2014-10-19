#include "Chl_includes.h"


typedef struct
{
    int  notoolbar;
    int  nostatusline;
    int  compact;
    int  resizable;
} appopts_t;

static psp_paramdescr_t text2appopts[] =
{
    PSP_P_FLAG("notoolbar",    appopts_t, notoolbar,    1, 0),
    PSP_P_FLAG("nostatusline", appopts_t, nostatusline, 1, 0),
    PSP_P_FLAG("compact",      appopts_t, compact,      1, 0),
    PSP_P_FLAG("resizable",    appopts_t, resizable,    1, 0),
    PSP_P_END()
};



static void CmdProc(XhWindow window, const char *cmd)
{
  chl_privrec_t *cr     = _ChlPrivOf(window);
  DataKnob       root_k = CdrGetMainGrouping(cr->subsys);
  
    if (CdrBroadcastCmd(root_k, cmd) != 0) return;
}

int  ChlRunSubsystem(DataSubsys subsys)
{
  XhWindow       win;
  chl_privrec_t *cr;
  const char    *win_title    = CdrFindSection(subsys, DSTN_WINTITLE,   "main");
  const char    *win_name     = CdrFindSection(subsys, DSTN_WINNAME,    "main");
  const char    *win_class    = CdrFindSection(subsys, DSTN_WINCLASS,   "main");
  const char    *win_options  = CdrFindSection(subsys, DSTN_WINOPTIONS, "main");

  appopts_t      opts;
  int            toolbar_mask    = XhWindowToolbarMask;
  int            statusline_mask = XhWindowStatuslineMask;
  int            compact_mask    = 0;
  int            resizable_mask  = XhWindowUnresizableMask;

    ChlClearErr();
  
    if (win_title == NULL) win_title = "(UNTITLED)";
    if (win_name  == NULL) win_name  = "pult";//"unnamed";
    if (win_class == NULL) win_class = "Pult";//"UnClassed";

    bzero(&opts, sizeof(opts)); // Is required, since none of the flags have defaults specified
    if (psp_parse(win_options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2appopts) != PSP_R_OK)
    {
        ChlSetErr("%s(): win_options: %s",
                  __FUNCTION__, psp_error());
        return -1;
    }

    if (opts.notoolbar)    toolbar_mask    = 0;
    if (opts.nostatusline) statusline_mask = 0;
    if (opts.compact)      compact_mask    = XhWindowCompactMask;
    if (opts.resizable)    resizable_mask  = 0;

    win = XhCreateWindow(win_title, win_name, win_class,
                         toolbar_mask | statusline_mask | compact_mask | resizable_mask,
                         NULL, NULL);
    if (win == NULL)
    {
        ChlSetErr("XhCreateWindow=NULL!");
        return -1;
    }
    
    cr = _ChlPrivOf(win);
    if (cr == NULL)
    {
        ChlSetErr("%s: malloc(chl_privrec_t) fail", __FUNCTION__);
        return -1;
    }
    cr->subsys = subsys;

    XhSetWindowCmdProc(win, CmdProc);

    if (CreateKnob(CdrGetMainGrouping(subsys), XhGetWindowWorkspace(win)) < 0)
    {
        ChlSetErr("%s", KnobsLastErr());
        return -1;
    }
    _ChlBindMethods(CdrGetMainGrouping(subsys));

    XhShowWindow      (win);
    XhRunApplication();
  
    return 0;
}
