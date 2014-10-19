#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include <dlfcn.h>

#include "findfilein.h"

#include "Cdr.h"


static void *dlopen_checker(const char *name,
                            const char *path,
                            void       *privptr __attribute((unused)))
{
  void *handle;
  char  buf[PATH_MAX];
  char  plen = strlen(path);
  
    check_snprintf(buf, sizeof(buf), "%s%s%s_db.so",
                   path,
                   plen == 0  ||  path[plen-1] == '/'? "" : "/",
                   name);
    handle = dlopen(buf, RTLD_NOW);
    dlerror();
    return handle;
}

int CdrOpenDescription (const char *subsys, const char *argv0,
                        void **handle_p, subsysdescr_t **info_p,
                        char **err_p)
{
  void          *handle = NULL;
  char           symbuf[200];
  subsysdescr_t *info;
  char          *err    = NULL;

    handle = findfilein(subsys,
                        argv0,
                        dlopen_checker, NULL,
                        "$CHLCLIENTS_PATH"
                           FINDFILEIN_SEP "./"
                           FINDFILEIN_SEP "!/"
                           FINDFILEIN_SEP "!/../lib/chlclients"
                           FINDFILEIN_SEP "$PULT_ROOT/lib/chlclients"
                           FINDFILEIN_SEP "~/pult/lib/chlclients");
    if (handle == NULL)
    {
        err = "_db.so file not found";
        goto ERREXIT;
    }

    snprintf(symbuf, sizeof(symbuf), "%s%s", subsys, SUBSYSDESCR_SUFFIX_STR);
    info = dlsym(handle, symbuf);
    err = dlerror();
    if (err != NULL) goto ERREXIT;
    
    if (info == NULL)
    {
        err = "subsysdescr==NULL";
        goto ERREXIT;
    }
    
    if (info->magicnumber != CXSS_SUBSYS_MAGIC)
    {
        err = "magicnumber mismatch";
        goto ERREXIT;
    }
    
    if (!CX_VERSION_IS_COMPATIBLE(info->version, CXSS_SUBSYS_VERSION))
    {
        err = "version mismatch";
        goto ERREXIT;
    }
    
    /* Okay, everything seems fine, let's return */
    *handle_p = handle;
    *info_p   = info;
    return 0;

 ERREXIT:
    if (handle != NULL) dlclose(handle);
    *err_p = err;
    return -1;
}

int CdrCloseDescription(void *handle, subsysdescr_t *info)
{
    return dlclose(handle) == 0? 0 : -1;
}
