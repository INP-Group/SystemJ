#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "cx_version.h"

#include "cxsd_logger.h"
#include "cxsd_module.h"


int  CxsdModuleChecker_cm(const char *name, void *symptr, void *privptr)
{
  cx_module_rec_t  *mr   = symptr;
  cx_module_desc_t *desc = privptr;

    if (mr->magicnumber != desc->magicnumber)
    {
        logline(LOGF_MODULES, 0,
                "%s (%s).magicnumber=0x%x, mismatch with 0x%x",
                desc->kindname, name, mr->magicnumber, desc->magicnumber);
        return -1;
    }
    if (!CX_VERSION_IS_COMPATIBLE(mr->version, desc->version))
    {
        logline(LOGF_MODULES, 0,
                "%s (%s).version=%d.%d.%d.%d, incompatible with %d.%d.%d.%d",
                desc->kindname, name,
                CX_MAJOR_OF(mr->version),
                CX_MINOR_OF(mr->version),
                CX_PATCH_OF(mr->version),
                CX_SNAP_OF (mr->version),
                CX_MAJOR_OF(desc->version),
                CX_MINOR_OF(desc->version),
                CX_PATCH_OF(desc->version),
                CX_SNAP_OF (desc->version));
        return -1;
    }
    
    return 0;
}

int  CxsdModuleInit_cm   (const char *name, void *symptr)
{
  cx_module_rec_t  *mr   = symptr;

    if (mr->init_mod == NULL  ||
        mr->init_mod() == 0)
        return 0;
    else
        return -1;
}

void CxsdModuleFini_cm   (const char *name, void *symptr)
{
  cx_module_rec_t  *mr   = symptr;

    if (mr->term_mod != NULL)
        mr->term_mod();
}
