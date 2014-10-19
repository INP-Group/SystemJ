#ifndef __CXSD_MODULE_H
#define __CXSD_MODULE_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "cx_module.h"


int  CxsdModuleChecker_cm(const char *name, void *symptr, void *privptr);
int  CxsdModuleInit_cm   (const char *name, void *symptr);
void CxsdModuleFini_cm   (const char *name, void *symptr);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CXSD_MODULE_H */
