#ifndef __CXSD_DB_H
#define __CXSD_DB_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "cxsd_module.h"
#include "cxsd_driver.h"


enum
{
    CXSD_DB_MAX_CHANGRPCOUNT = 30,
    CXSD_DB_MAX_BUSINFOCOUNT = 20,
};


typedef struct
{
    int                 is_builtin;
    
    char                instname[32];
    char                typename[32];
    char                drvname [32];
    char                lyrname [32];
    
    int                 changrpcount;
    int                 businfocount;

    CxsdChanInfoRec     changroups[CXSD_DB_MAX_CHANGRPCOUNT];
    int                 businfo   [CXSD_DB_MAX_BUSINFOCOUNT];
    
    const char         *options;
    const char         *auxinfo;
} CxsdDbDevLine_t;

typedef struct
{
    char                lyrname [32];
    int                 bus_n;
    const char         *info;
} CxsdDbLayerinfo_t;

typedef struct _CxsdDbInfo_t_struct
{
    int                 numdevs;
    CxsdDbDevLine_t    *devlist;

    int                 numlios;
    CxsdDbLayerinfo_t  *liolist;
} CxsdDbInfo_t;

typedef struct _CxsdDbInfo_t_struct *CxsdDb;


CxsdDb  CxsdDbCreate(void);
int     CxsdDbDestroy(CxsdDb db);

int     CxsdDbAddDev (CxsdDb db, CxsdDbDevLine_t   *dev_p,
                      const char *options, const char *auxinfo);
int     CxsdDbAddLyrI(CxsdDb db, CxsdDbLayerinfo_t *lio_p,
                      const char *info);


//////////////////////////////////////////////////////////////////////

CxsdDb  CxsdDbLoadDb(const char *argv0,
                     const char *def_scheme, const char *reference);


#define CXSD_DBLDR_MODREC_SUFFIX _dbldr_rec
#define CXSD_DBLDR_MODREC_SUFFIX_STR __CX_STRINGIZE(CXSD_DBLDR_MODREC_SUFFIX)

enum {CXSD_DBLDR_MODREC_MAGIC = 0x644c6244}; /* Little-endian 'DbLd' */
enum {
    CXSD_DBLDR_MODREC_VERSION_MAJOR = 1,
    CXSD_DBLDR_MODREC_VERSION_MINOR = 0,
    CXSD_DBLDR_MODREC_VERSION = CX_ENCODE_VERSION(CXSD_DBLDR_MODREC_VERSION_MAJOR,
                                                  CXSD_DBLDR_MODREC_VERSION_MINOR)
};

typedef CxsdDb (*CxsdDbLdr_t)(const char *argv0,
                              const char *reference);

typedef struct _CxsdDbLdrModRec_struct
{
    cx_module_rec_t  mr;

    CxsdDbLdr_t      ldr;

    struct _CxsdDbLdrModRec_struct *next;
} CxsdDbLdrModRec;

#define CXSD_DBLDR_MODREC_NAME(name) \
    __CX_CONCATENATE(name, CXSD_DBLDR_MODREC_SUFFIX)

#define DECLARE_CXSD_DBLDR(name) \
    CxsdDbLdrModRec CXSD_DBLDR_MODREC_NAME(name)

#define DEFINE_CXSD_DBLDR(name, comment,                             \
                          init_m, term_m,                            \
                          ldr_f)                                     \
    CxsdDbLdrModRec CXSD_DBLDR_MODREC_NAME(name) =                   \
    {                                                                \
        {                                                            \
            CXSD_DBLDR_MODREC_MAGIC, CXSD_DBLDR_MODREC_VERSION,      \
            __CX_STRINGIZE(name), comment,                           \
            init_m, term_m                                           \
        },                                                           \
        ldr_f,                                                       \
        NULL                                                         \
    }


int  CxsdDbRegisterDbLdr  (CxsdDbLdrModRec *metric);
int  CxsdDbDeregisterDbLdr(CxsdDbLdrModRec *metric);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXSD_DB_H */
