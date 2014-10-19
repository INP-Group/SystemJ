#include <stdio.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include "misc_macros.h"
#include "misclib.h"

#include "cda.h"
#include "Chl.h"
#include "Knobs_typesP.h"
#include "KnobsI.h"

#include "weldclient.h"
#include "weldclient_process_knobplugin.h"


//////////////////////////////////////////////////////////////////////

static cda_serverid_t    bigcsid;
static cda_bigchandle_t  bigch;

static void BigcEventProc(cda_serverid_t  sid       __attribute__((unused))__attribute__((unused)),
                          int             reason,
                          void           *privptr  __attribute__((unused)))
{
}

static void lclregchg_proc(int lreg_n, double v)
{
    fprintf(stderr, "reg#%d:=%8.3f\n", lreg_n, v);
}


//////////////////////////////////////////////////////////////////////

static int ELEM_WELD_DUMMY_Create_m(XhWidget parent, ElemInfo e)
{
  cda_serverid_t    mainsid;
  const char       *srvname;

    e->container = ABSTRZE(XtVaCreateManagedWidget("", widgetClass, CNCRTZE(parent),
                                                   XmNwidth,       1,
                                                   XmNheight,      1,
                                                   XmNtraversalOn, False,
                                                   NULL));

    /* Make a server connection */
    mainsid = ChlGetMainsid(XhWindowOf(parent));
    srvname = cda_status_srvname(mainsid, 0);

    if ((bigcsid = cda_new_server(srvname,
                                  BigcEventProc, NULL,
                                  CDA_BIGC)) == CDA_SERVERID_ERROR)
    {
        fprintf(stderr, "%s: %s: cda_new_server(server=%s): %s\n",
                strcurtime(), __FUNCTION__, srvname, cx_strerror(errno));
        exit(1);
    }

    bigch = cda_add_bigc(bigcsid, 0,
                         1,
                         261*sizeof(int32),
                         CX_CACHECTL_SHARABLE, CX_BIGC_IMMED_YES*1);


    return 0;
}

static chl_elemdescr_t custom_table[] =
{
    {ELEM_WELD_DUMMY,  "weld_dummy",  ELEM_WELD_DUMMY_Create_m},
    {0},
};

//////////////////////////////////////////////////////////////////////

static void CommandProc(XhWindow win, int cmd)
{
    switch (cmd)
    {
        default:
            ChlHandleStdCommand(win, cmd);
    }
}


int main(int argc, char *argv[])
{
  int  r;

    RegisterWELD_PROCESS_widgetset(LOGD_WELD_PROCESS);
    ChlSetCustomElementsTable(custom_table);
    cda_set_lclregchg_cb(lclregchg_proc);
    r = ChlRunMulticlientApp(argc, argv,
                             __FILE__,
                             ChlGetStdToolslist(), CommandProc,
                             NULL,
                             NULL,
                             NULL, NULL);
    if (r != 0) fprintf(stderr, "%s: %s: %s\n", strcurtime(), argv[0], ChlLastErr());

    return r;
}
