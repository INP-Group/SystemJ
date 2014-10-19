#include "cxsd_includes.h"


//////////////////////////////////////////////////////////////////////

extern DECLARE_CXSD_FRONTEND(cx);
extern DECLARE_CXSD_DBLDR   (file);

cx_module_rec_t *builtins_list[] =
{
    &(CXSD_FRONTEND_MODREC_NAME(cx).mr),
    &(CXSD_DBLDR_MODREC_NAME   (file).mr),
};


//////////////////////////////////////////////////////////////////////

void InitBuiltins(void)
{
  int  n;

    for (n = 0;  n < countof(builtins_list);  n++)
    {
        if (builtins_list[n]->init_mod != NULL  &&
            builtins_list[n]->init_mod() < 0)
            logline(LOGF_MODULES, 0, "%s(): %08x.%s.init_mod() failed",
                    __FUNCTION__,
                    builtins_list[n]->magicnumber, builtins_list[n]->name);
    }
}

void TermBuiltins(void)
{
  int  n;

    for (n = countof(builtins_list) - 1;  n >= 0;  n--)
        if (builtins_list[n]->term_mod != NULL)
            builtins_list[n]->term_mod();
}
