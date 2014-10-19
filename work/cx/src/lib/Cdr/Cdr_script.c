#include <stdio.h>

#include "Knobs_typesP.h"
#include "Cdr.h"
#include "Cdr_script.h"
#include "fqkn.h"


typedef struct
{
    groupelem_t *grouplist;

    int          options;

    int          val_parsed;
    double       val;

    char        *endptr;
} set_cmd_privrec_t;


static int set_exp(const char *name, const char *p, void *privptr)
{
  set_cmd_privrec_t *rp = privptr;
  knobinfo_t        *ki;

////fprintf(stderr, "\t[%s]\n", name);
    if (!(rp->val_parsed))
    {
        if (*p != '=')
        {
            /*!!!*/
            fprintf(stderr, "'=' expected, not <%s>\n", p);
            return -1;
        }
        p++;

        rp->val = strtod(p, &(rp->endptr));
        if (rp->endptr == p)
        {
            /*!!!*/
            fprintf(stderr, "empty value\n");
            return -1;
        }

        rp->val_parsed = 1;
    }

    ki = CdrFindKnob(rp->grouplist, name);
    if (ki == NULL)
    {
        /*!!!*/
        fprintf(stderr, "Cdr_script::set: target knob <%s> not found\n", name);
        return -1;
    }
    
    if ((rp->options & LSE_PARSE_ONLY) == 0)
        datatree_SetControlValue(ki, rp->val, 1);

    return 0;
}

static int script_set_cmd(lse_context_t *ctx,
                          const char    *p, const char **after_p,
                          int            options)
{
  set_cmd_privrec_t   rec;
  int                 r;

////if ((options & LSE_PARSE_ONLY) == 0)fprintf(stderr, "SET-PRE: <%s>\n", p);
    bzero(&rec, sizeof(rec));
    rec.grouplist = ctx->privptr;
    rec.options   = options;
    r = fqkn_expand(p, set_exp, &rec);
    ////fprintf(stderr, "%s r=%d\n", __FUNCTION__, r);
    if (r < 0) return LSE_R_ERR;

    *after_p = rec.endptr;
////if ((options & LSE_PARSE_ONLY) == 0)fprintf(stderr, "SET-POST: <%s>\n", *after_p);
    
    return LSE_R_OK;
}


static lse_cmd_descr_t Cdr_script_cmdset[] =
{
    {"set", script_set_cmd},
    {NULL, NULL}
};

int Cdr_script_init(lse_context_t *ctx,
                    const char *scriptname, const char *script,
                    groupelem_t *grouplist)
{
    return lse_init(ctx, scriptname, script, Cdr_script_cmdset, grouplist);
}
