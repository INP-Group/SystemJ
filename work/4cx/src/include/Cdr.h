#ifndef __CDR_H
#define __CDR_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "datatree.h"


enum
{
    CDR_OPT_SYNTHETIC = 1 << 0,
    CDR_OPT_READONLY  = 1 << 1,
};


DataSubsys  CdrLoadSubsystem(const char *def_scheme, const char *reference);

void     *CdrFindSection    (DataSubsys subsys, const char *type, const char *name);
DataKnob  CdrGetMainGrouping(DataSubsys subsys);

void  CdrDestroySubsystem(DataSubsys subsys);
void  CdrDestroyKnobs(DataKnob list, int count);

void  CdrSetSubsystemRO  (DataSubsys  subsys, int ro);

int   CdrRealizeSubsystem(DataSubsys  subsys, const char *defserver);
int   CdrRealizeKnobs    (DataSubsys  subsys,
                          const char *baseref,
                          DataKnob list, int count);

void  CdrProcessSubsystem(DataSubsys subsys, int cause_conn_n,  int options,
                                                    rflags_t *rflags_p);
void  CdrProcessKnobs    (DataSubsys subsys, int cause_conn_n,  int options,
                          DataKnob list, int count, rflags_t *rflags_p);

int   CdrSetKnobValue(DataSubsys subsys, DataKnob  k, double v, int options);

int   CdrBroadcastCmd(DataKnob  k, const char *cmd);


/* Error descriptions */
char *CdrLastErr(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __CDR_H */
