#ifndef __CXSD_LOADER_H
#define __CXSD_LOADER_H


typedef int (*cxsd_ld_vchk_t)(const char *fname, void *symptr, void *privinfo);


int LoadModule(const char *name, const char *fdir, const char *fsuffix,
               const char *symname,                const char *symsuffix,
               int do_global,
               cxsd_ld_vchk_t checker, void *checker_privinfo,
               void **metric_p);


#endif /* __CXSD_LOADER_H */
