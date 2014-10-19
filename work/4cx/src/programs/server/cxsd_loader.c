#include "cxsd_includes.h"

#include "cxldr.h"
#include "cxsd_module.h"
#include "cxsd_extension.h"


static cx_module_desc_t fnd_desc =
{
    "frontend",
    CXSD_FRONTEND_MODREC_MAGIC,
    CXSD_FRONTEND_MODREC_VERSION
};

static cx_module_desc_t ext_desc =
{
    "ext",
    CXSD_EXT_MODREC_MAGIC,
    CXSD_EXT_MODREC_VERSION
};

static cx_module_desc_t lib_desc =
{
    "lib",
    CXSD_LIB_MODREC_MAGIC,
    CXSD_LIB_MODREC_VERSION
};

static CXLDR_DEFINE_CONTEXT(fnd_ldr_context,
                            CXLDR_FLAG_NONE,
                            NULL,
                            "cxsd_fe_", ".so",
                            "", CXSD_FRONTEND_MODREC_SUFFIX_STR,
                            CxsdModuleChecker_cm,
                            CxsdModuleInit_cm, CxsdModuleFini_cm,
                            NULL,
                            NULL, 0);


static CXLDR_DEFINE_CONTEXT(ext_ldr_context,
                            CXLDR_FLAG_NONE,
                            NULL,
                            "", "_ext.so",
                            "", CXSD_EXT_MODREC_SUFFIX_STR,
                            CxsdModuleChecker_cm,
                            CxsdModuleInit_cm, CxsdModuleFini_cm,
                            NULL,
                            NULL, 0);

static CXLDR_DEFINE_CONTEXT(lib_ldr_context,
                            CXLDR_FLAG_GLOBAL,
                            NULL,
                            "", "_lib.so",
                            "", CXSD_LIB_MODREC_SUFFIX_STR,
                            CxsdModuleChecker_cm,
                            CxsdModuleInit_cm, CxsdModuleFini_cm,
                            NULL,
                            NULL, 0);
/////////////////


int  CxsdLoadFrontend(const char *argv0, const char *name)
{
  int                 r;
  CxsdFrontendModRec *metric;

    r = cxldr_get_module(&fnd_ldr_context,
                         name,
                         argv0,
                         (void **)&metric,
                         &fnd_desc);
      
    if (r < 0) return r;
    
    return 0;
}

int  CxsdLoadExt     (const char *argv0, const char *name)
{
  int                 r;

    r = cxldr_get_module(&ext_ldr_context,
                         name,
                         argv0,
                         NULL,
                         &ext_desc);
    
    return r;
}

int  CxsdLoadLib     (const char *argv0, const char *name)
{
  int                 r;

    r = cxldr_get_module(&lib_ldr_context,
                         name,
                         argv0,
                         NULL,
                         &lib_desc);

    return r;
}

