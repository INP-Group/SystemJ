#ifndef __DATATREE_H
#define __DATATREE_H


#include "memcasecmp.h"

#include "Knobs.h"


enum {DATATREE_DELIM = '.'};               // Knob-name-components delimiter


typedef struct
{
    ElemInfo    ei;
    int         fromnewline;
} groupelem_t;  


typedef void (*knob_editstate_hook_t)(Knob k, int state);
void set_knob_editstate_hook(knob_editstate_hook_t hook);

void set_datatree_flipflop(int value);

rflags_t    datatree_choose_knob_rflags(Knob k, tag_t tag, rflags_t rflags, double new_v);
void        datatree_set_knobstate     (Knob k, colalarm_t newstate);
void        datatree_set_attn          (Knob k, colalarm_t state, int on, int period);
colalarm_t  datatree_choose_knobstate  (Knob k, rflags_t rflags);

int  datatree_SetControlValue(Knob k, double v, int fromlocal);

int  get_knob_label               (Knob k, char **label_p);
int  get_knob_tip                 (Knob k, char **tip_p);
void get_knob_rowcol_label_and_tip(const char *ms, int n,
                                   Knob        model_k,
                                   char       *lbuf, size_t lbufsize,
                                   char       *tbuf, size_t tbufsize,
                                   char      **caption_p,
                                   char      **tip_p);


Knob datatree_FindNode    (groupelem_t *list, const char *name);
Knob datatree_FindNodeFrom(groupelem_t *list, const char *name,
                           ElemInfo     start_point);
Knob datatree_FindNodeInside(ElemInfo container,
                             const void *subname, size_t subnamelen);

static inline int datatree_NameMatches(const char *ident,
                                       const void *subname, size_t subnamelen)
{
    if (ident == NULL) return 0;
    if (*ident == DATATREE_DELIM) ident++;

    return cx_strmemcasecmp(ident, subname, subnamelen) == 0;
}


#endif /* __DATATREE_H */
