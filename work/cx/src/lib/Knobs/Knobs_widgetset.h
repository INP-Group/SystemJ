#ifndef __KNOBS_WIDGETSET_H
#define __KNOBS_WIDGETSET_H


knobs_vmt_t   *KnobsGetVmtByID  (int id);
knobs_vmt_t   *KnobsGetVmtByName(const char *name);
int            KnobsName2ID     (const char *name);


#endif /* __KNOBS_WIDGETSET_H */
