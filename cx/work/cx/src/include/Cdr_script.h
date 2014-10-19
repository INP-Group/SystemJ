#ifndef __CDR_SCRIPT_H
#define __CDR_SCRIPT_H

#include "Cdr.h"
#include "lightscript_engine.h"


int Cdr_script_init(lse_context_t *ctx,
                    const char *scriptname, const char *script,
                    groupelem_t *grouplist);


#endif /* __CDR_SCRIPT_H */
