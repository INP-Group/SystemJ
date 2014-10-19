#include <stdio.h>
#include <limits.h>

/* As for now, this is a standalone-like library,
   so we refrain from using "cxsd_includes.h" */
#include "cxsd_loader.h"

/* And for now -- since we still use cxsd's libs, we include this... */
#include "cxsd_lib.h"
#include "misc_macros.h"

#include <dlfcn.h> /* THIS ONE should better be included here -- it has nothing to do in other files */


int LoadModule(const char *name, const char *fdir, const char *fsuffix,
               const char *symname,                const char *symsuffix,
               int do_global,
               cxsd_ld_vchk_t checker, void *checker_privinfo,
               void **metric_p)
{
  char        fname[PATH_MAX];
  void       *handle;
  const char *err;
  char        symbuf[200];
  void       *symptr;

    if (fdir == NULL) fdir = "";
    
    /* Obtain the full filename */
    if (check_snprintf(fname, sizeof(fname), "%s%s%s%s",
                       fdir, fdir[0] != '\0' ? "/":"", name, fsuffix))
    {
        /*!!! Grrrrrr!!!!! */
    }

    /* Load the module (!!!What a dirty way!  Must search the "already loaded" table first, and do reference-counting) */
    handle = dlopen(fname, RTLD_NOW | (do_global? RTLD_GLOBAL:0));
    err = dlerror();
    if (handle == NULL)
    {
        logline(LOGF_SYSTEM, 0, "%s: dlopen(\"%s\"): %s",
                __FUNCTION__, fname, err);
        return -1;
    }

    /* Get access to requested symbol */
    if (check_snprintf(symbuf, sizeof(symbuf), "%s%s", symname, symsuffix))
    {
        /*!!! Grrrrrr!!!!! */
    }
    symptr = dlsym(handle, symbuf);
    if (symptr == NULL)
    {
        logline(LOGF_SYSTEM, 0, "%s: symbol \"%s\" not found in \"%s\"",
                __FUNCTION__, symbuf, fname);
        dlclose(handle);
        return -1;
    }

    /* Check signature */
    if (checker != NULL  &&  (checker(name, symptr, checker_privinfo) != 0))
    {
        dlclose(handle);
        return -1;
    }

    /* Okay -- return the pointer */
    *metric_p = symptr;

    return 0;
}
