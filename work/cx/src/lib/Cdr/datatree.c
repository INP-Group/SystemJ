#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <math.h>

#include "misclib.h"
#include "memcasecmp.h"

#include "cx.h"
#include "cxdata.h"
#include "datatree.h"
#include "Knobs_typesP.h"
//#include "Cdr.h" /*!!! Should move CDR_FLAGs somewhere to datatreeP.h */


static knob_editstate_hook_t editstate_hook = NULL;

void set_knob_editstate_hook(knob_editstate_hook_t hook)
{
    editstate_hook = hook;
}


static int flipflop = 0;

void set_datatree_flipflop(int value)
{
    flipflop = value*0;
}


rflags_t    datatree_choose_knob_rflags(Knob k, tag_t tag, rflags_t rflags, double new_v)
{
    if (k == NULL  ||  k->type == LOGT_SUBELEM) return rflags;

    //  0. Do "``changed by...'' flags" have sense?
    if (!(k->is_rw)  ||  (k->behaviour & KNOB_B_IGN_OTHEROP) != 0)
        rflags &=~ CXCF_FLAG_4WRONLY_MASK;

    //  2. ALARM
    if ((k->behaviour & KNOB_B_IS_ALARM) != 0  &&
        (((int)new_v)    & CX_VALUE_LIT_MASK) != 0)
        rflags |=  CXCF_FLAG_ALARM_ALARM;
    
    //  3. YELLOW...
    if (k->rmins[KNOBS_RANGE_NORM] < k->rmaxs[KNOBS_RANGE_NORM])
        if (new_v < k->rmins[KNOBS_RANGE_NORM]  ||  new_v > k->rmaxs[KNOBS_RANGE_NORM])
            rflags |= CXCF_FLAG_COLOR_YELLOW;
    //    ...and RED
    if (k->rmins[KNOBS_RANGE_YELW] < k->rmaxs[KNOBS_RANGE_YELW])
        if (new_v < k->rmins[KNOBS_RANGE_YELW]  ||  new_v > k->rmaxs[KNOBS_RANGE_YELW])
            rflags |= CXCF_FLAG_COLOR_RED;
    
    //  4. DEFUNCT -- is now in cda responsibility
    
    //  5. "Attention" -- in fact, now only RELAX
    if (k->attn_endtime != 0)
    {
        if (k->attn_colstate == COLALARM_RELAX)
            rflags |= CXCF_FLAG_ALARM_RELAX;
    }

    return rflags;
}

void        datatree_set_knobstate     (Knob k, colalarm_t newstate)
{
    if (newstate != k->colstate)
    {
        if (k->vmtlink != NULL  &&
            k->vmtlink->Colorize != NULL)
            k->vmtlink->Colorize(k, newstate);
        
        k->colstate = newstate;
    }
}

void        datatree_set_attn          (Knob k, colalarm_t state, int on, int period)
{
  time_t      timenow = time(NULL);
    
    if (on)
    {
        k->attn_endtime  = timenow + period;
        k->attn_colstate = state;
    }
    else
    {
        k->attn_endtime  = 0;
    }
}

colalarm_t  datatree_choose_knobstate  (Knob k, rflags_t rflags)
{
  colalarm_t  colstate = COLALARM_NONE;

  static colalarm_t sf_hw_ff[2][2][2] =
  {
      {{COLALARM_NONE,  COLALARM_NONE},  {COLALARM_HWERR, COLALARM_HWERR}},
      {{COLALARM_SFERR, COLALARM_SFERR}, {COLALARM_SFERR, COLALARM_HWERR}},
  };

    if (rflags & CXCF_FLAG_PRGLYCHG)         colstate = COLALARM_PRGLYCHG;
    if (k != NULL  &&  k->attn_endtime != 0) colstate = k->attn_colstate;
    if (rflags & CXCF_FLAG_COLOR_YELLOW)     colstate = COLALARM_YELLOW;
    if (rflags & CXCF_FLAG_COLOR_RED)        colstate = COLALARM_RED;
    if (rflags & CXCF_FLAG_COLOR_WEIRD)      colstate = COLALARM_WEIRD;
    if (rflags & CXCF_FLAG_DEFUNCT)          colstate = COLALARM_DEFUNCT;
#if 1
    if (rflags & (CXCF_FLAG_SFERR_MASK | CXCF_FLAG_HWERR_MASK))
        colstate = sf_hw_ff
            [(rflags & CXCF_FLAG_SFERR_MASK) != 0]
            [(rflags & CXCF_FLAG_HWERR_MASK) != 0]
            [flipflop];
#else
    if (rflags & CXCF_FLAG_SFERR_MASK)       colstate = COLALARM_SFERR;
    if (rflags & CXCF_FLAG_HWERR_MASK)       colstate = COLALARM_HWERR;
#endif
    if (rflags & CXCF_FLAG_NOTFOUND)         colstate = COLALARM_NOTFOUND;

    return colstate;
}

int  datatree_SetControlValue(Knob k, double v, int fromlocal)
{
  int     ret = 0;
  double  minv, maxv;

    if (k->type == LOGT_SUBELEM) return -1;

    if (fromlocal  &&  !k->is_rw) return -1;

    /* Check if it is valid. */
    if (fromlocal)
    {
        minv = maxv = 0.0;
        if (k->colformula == NULL)
        {
            if (k->rmins[KNOBS_RANGE_NORM] < k->rmaxs[KNOBS_RANGE_NORM])
            {
                minv = k->rmins[KNOBS_RANGE_NORM];
                maxv = k->rmaxs[KNOBS_RANGE_NORM];
            }
            if (k->rmins[KNOBS_RANGE_YELW] < k->rmaxs[KNOBS_RANGE_YELW])
            {
                minv = k->rmins[KNOBS_RANGE_YELW];
                maxv = k->rmaxs[KNOBS_RANGE_YELW];
            }
        }
        if (minv < maxv)
        {
            if (v < minv)
            {
                if (minv - v > k->incdec_step) ret = 1;
                v = minv;
            }
            if (v > maxv)
            {
                if (v - maxv > k->incdec_step) ret = 1;
                v = maxv;
            }
        }
    }

    /* "Quant" wizardry... */
    if (fromlocal)
    /* Save user-input value for future comparison */
    {
        k->userval       = v;
        k->userval_valid = 1;
    }
    else
    /* Check if we are still near user-input */
    {
        if (k->userval_valid)
        {
            if (fabs(v - k->userval) < k->q * 0.999999)
                v = k->userval;
            else
                k->userval_valid = 0;
        }
    }

    /* Set the value in widget */
    if ((!fromlocal  ||  (k->behaviour & KNOB_B_IS_BUTTON) == 0)  &&  // skip button-alikes
        k->vmtlink != NULL  &&
        k->vmtlink->SetValue != NULL)
        k->vmtlink->SetValue(k, v);

    if (!k->is_rw) return ret;

    /* And mark as requiring to be updated */
    if (k->usertime != 0)
    {
        k->usertime = 0;
        if (editstate_hook != NULL) editstate_hook(k, 0);
    }

    /* ...but a cycle later */
    if (fromlocal)
        if ((k->behaviour & KNOB_B_NO_WASJUSTSET) == 0) k->wasjustset = 1;

    /* Reset the "attention" (OTHEROP, RELAX) state, recolorizing accordingly */
    if (fromlocal)
    {
        datatree_set_attn     (k, COLALARM_NONE, 0, 0);
        datatree_set_knobstate(k, datatree_choose_knobstate(k, k->currflags));
    }

    /* Finally, set physical channel(s) value(s) */
    if (fromlocal)
        if (k->uplink                       != NULL  &&
            k->uplink->emlink               != NULL  &&
            k->uplink->emlink->SetPhysValue != NULL)
            k->uplink->emlink->SetPhysValue(k, v);

    return ret;
}


int  get_knob_label               (Knob k, char **label_p)
{
    *label_p = k->label;
    return *label_p != NULL  &&  **label_p != '\0'  &&  **label_p != KNOB_LABELTIP_NOLABELTIP_PFX;
}

int  get_knob_tip                 (Knob k, char **tip_p)
{
    *tip_p   = k->comment;
    return *tip_p   != NULL  &&  **tip_p   != '\0'  &&  **tip_p   != KNOB_LABELTIP_NOLABELTIP_PFX;
}

void get_knob_rowcol_label_and_tip(const char *ms, int n,
                                   Knob        model_k,
                                   char       *lbuf, size_t lbufsize,
                                   char       *tbuf, size_t tbufsize,
                                   char      **caption_p,
                                   char      **tip_p)
{
  char *p;
    
    if (ms != NULL)
        extractstring(ms, n, lbuf, lbufsize);
    else
        strzcpy(lbuf, "?\f?", lbufsize);
    if ((p = strchr(lbuf, MULTISTRING_OPTION_SEPARATOR)) != NULL)
    {
        *p = '\0';
        strzcpy(tbuf, p + 1, tbufsize);
    }
    else
    {
        tbuf[0] = '\0';
    }

    *caption_p = (strcmp(lbuf, "?") != 0) ? lbuf : model_k->label;
    *tip_p     = (strcmp(tbuf, "?") != 0) ? tbuf : model_k->comment;

    if (*caption_p == NULL) *caption_p = "";
}


static Knob RecFindGlobalKnob(eleminfo_t *ei, const char *sn)
{
  const char  *knn;    // Knob's normalized name
  knobinfo_t  *ki;
  int          y;
  knobinfo_t  *rv;
  
    for (y = 0, ki = ei->content;  y < ei->count;  y++, ki++)
    {
        knn = ki->ident;
        if (knn != NULL)
        {
            if (*knn == DATATREE_DELIM) knn++;

            if (strcasecmp(knn, sn) == 0) return ki;
        }

        if (ki->type == LOGT_SUBELEM  &&
            (rv = RecFindGlobalKnob(ki->subelem, sn)) != NULL)
            return rv;
    }

    return NULL;
}

static Knob FindKnobInElem(eleminfo_t *ei, const char *name)
{
  const char  *sn = name;
  size_t       snlen;
  const char  *dot;
  int          y;
  knobinfo_t  *ki;

    if (ei == NULL  ||  ei->content == NULL)
    {
        errno = ENOENT;
        return NULL;
    }

    while (1)
    {
        dot = strchr(sn, DATATREE_DELIM);
        if (dot == NULL) snlen = strlen(sn);
        else             snlen = dot - sn;
        ki = datatree_FindNodeInside(ei, sn, snlen);
        if (ki == NULL)
        {
            errno = ENOENT;
            return NULL;
        }

        /* If it was the last component, return it */
        if (dot == NULL) return ki;

        sn = dot + 1;
        
        /* Oh-ka-a-ay, what did we found -- a knob or a subelement? */
        if (ki->type == LOGT_SUBELEM)
        {
            /* Dive deeper into hierarchy */
            ei = ki->subelem;
            goto CONTINUE_DIVE;
        }
        else
        {
            /* Sorry... */
            errno = ENOTDIR;
            return NULL;
        }

        errno = ENOENT;
        return NULL;

 CONTINUE_DIVE:;
    }
}

static knobinfo_t * FindKnobInList(groupelem_t *list, const char *name)
{
  const char  *dot;
  const char  *sn;     // Sub-name
  size_t       snlen;

  groupelem_t *ge;
  eleminfo_t  *ei;
  knobinfo_t  *ki;

    /* A global name? */
    if (name[0] == DATATREE_DELIM  &&
        strchr((sn = name + 1), DATATREE_DELIM) == NULL)
    {
        /* Upper layer loop */
        for (ge = list;  (ei = ge->ei) != NULL;  ge++)
        {
            if ((ki = RecFindGlobalKnob(ei, sn)) != NULL)
                return ki;
        }
    }
    /* No, a regular fully-qualified knob name */
    else
    {
        if (name[0] == DATATREE_DELIM) name++;

        /* FQKN *must* contain at least one dot */
        if ((dot = strchr(name, DATATREE_DELIM)) == NULL) goto NOTFOUND;
        snlen = dot - name;

        /* Root-level search */
        for (ge = list;  (ei = ge->ei) != NULL;  ge++)
        {
            if (ei->ident != NULL  &&
                cx_strmemcasecmp(ei->ident, name, snlen) == 0) break;
        }

        if (ei != NULL)
            return FindKnobInElem(ei, dot + 1);
    }

 NOTFOUND:
    errno = ENOENT;
    return NULL;
}


Knob datatree_FindNode    (groupelem_t *list, const char *name)
{
  knobinfo_t *ki;

    /* Empty names are never found */
    if (name == NULL  ||  name[0] == '\0')
    {
        errno = ENOENT;
        return NULL;
    }
  
    ki = FindKnobInList(list, name);

#if 0
    /* Don't return SUBELEMs */
    if (ki != NULL  &&  ki->type == LOGT_SUBELEM)
    {
        errno = EISDIR;
        return NULL;
    }
#endif

    ////fprintf(stderr, "%s(\"%s\"): %s\n", __FUNCTION__, name, ki==NULL?"absent":"found");
    return ki;
}

Knob datatree_FindNodeFrom(groupelem_t *list, const char *name,
                           ElemInfo     start_point)
{
  knobinfo_t *ki;

    /* Empty names are never found */
    if (name == NULL  ||  name[0] == '\0')
    {
        errno = ENOENT;
        return NULL;
    }
  
    /* Determine type of reference... */
    if (name[0] == '.')
    {
        /* A global one */
        if (list == NULL)
        {
            errno = ENOENT; /* Or what? */
            return NULL;
        }

        ki =  FindKnobInList(list, name);
    }
    else
    {
         /* Skip ":"-transparent elements upwards */
         while (start_point           != NULL  &&
                start_point->ident    != NULL  &&
                start_point->ident[0] == ':')
             start_point = start_point->uplink;
        /* A relative one */
        if (start_point == NULL)
        {
            errno = ENOENT; /* Or what? */
            return NULL;
        }

        /* Skip '.'-shadowing ':' */
        if (*name == ':') name++;

        /* Walk upwards */
        while (*name == '.')
        {
            name++;
            if (start_point->uplink != NULL)
            {
                start_point = start_point->uplink;
                while (start_point           != NULL  &&
                       start_point->ident    != NULL  &&
                       start_point->ident[0] == ':')
                    start_point = start_point->uplink;
            }
        }

        ki = FindKnobInElem(start_point, name);
    }

#if 0
    /* Don't return SUBELEMs */
    if (ki != NULL  &&  ki->type == LOGT_SUBELEM)
    {
        errno = EISDIR;
        return NULL;
    }
#endif

    return ki;
}

Knob datatree_FindNodeInside(ElemInfo container,                   
                             const void *subname, size_t subnamelen)
{
  int   y;
  Knob  ki;
  Knob  kf;

    if (container == NULL  ||  subnamelen == 0) return NULL;

    for (y = 0, ki = container->content;
         y < container->count;
         y++,   ki++)
    {
        /* Either a matching name... */
        if (/* Match either knob's name... */
            datatree_NameMatches(ki->ident, subname, subnamelen)  ||
            /* ...or subelement's name */
            (ki->type == LOGT_SUBELEM    &&
             datatree_NameMatches(ki->subelem->ident, subname, subnamelen))
           ) return ki;
        /* ...or a transparent container */
        if (ki->type == LOGT_SUBELEM  &&
            (/* Match either knob's name... */
             (ki->ident != NULL  &&  ki->ident[0] == ':')  ||
             /* ...or subelement's name */
             (ki->subelem           != NULL  &&
              ki->subelem->ident    != NULL  &&
              ki->subelem->ident[0] == ':')
            )
           )
        {
            kf = datatree_FindNodeInside(ki->subelem, subname, subnamelen);
            if (kf != NULL) return kf;
        }
    }

    return NULL;
}
