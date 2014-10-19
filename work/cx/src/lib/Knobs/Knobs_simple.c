#include "Knobs_includes.h"

#include "cxdata.h" /* For paramstr definitions */


enum {USERINACTIVITYLIMIT = 60};  // Seconds between "input" turns back to autoupdate


static void Simple_SetPhysValue_m(knobinfo_t *ki, double v)
{
  simpleknob_cb_t  cb = ki->privptr1;

    if (cb != NULL)
        cb(ki, v, ki->privptr2);
}

static knobs_emethods_t Simple_emethods =
{
    Simple_SetPhysValue_m,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static eleminfo_t  Simple_fakeelem;

static int LookPluginParser(const char *str, const char **endptr,
                            void *rec, size_t recsize __attribute__((unused)),
                            const char *separators __attribute__((unused)), const char *terminators __attribute__((unused)),
                            void *privptr, char **errstr)
{
  int          look = ptr2lint(privptr);
  int         *lp   = (int *)rec;
  const char  *srcp = str;
  const char  *name_b;

  char         namebuf[100];
  size_t       namelen;
  
    if (str != NULL)
    {
        /* Okay, let's extract the name... */
        name_b = srcp;
        while (isalnum(*srcp)  ||  *srcp == '_') srcp++;

        /* Empty name?! */
        if (name_b == srcp)
        {
            *errstr = "knob-type name expected";
            return PSP_R_USRERR;
        }
        
        /* Extract name... */
        namelen = srcp - name_b;
        if (namelen > sizeof(namebuf) - 1) namelen = sizeof(namebuf) - 1;
        memcpy(namebuf, name_b, namelen);
        namebuf[namelen] = '\0';

        look = KnobsName2ID(namebuf);
        
        if (endptr != NULL) *endptr = srcp;
    }
    
    *lp = look;

    return PSP_R_OK;
}

static psp_lkp_t psp_colors[] =
{
    {"normal",    LOGC_NORMAL},
    {"hilited",   LOGC_HILITED},
    {"important", LOGC_IMPORTANT},
    {"vic",       LOGC_VIC},
    {"dim",       LOGC_DIM},
    {NULL, 0}
};

static psp_paramdescr_t text2knobinfo[] =
{
    PSP_P_FLAG   ("rw",         knobinfo_t, is_rw,       1, 1),
    PSP_P_FLAG   ("ro",         knobinfo_t, is_rw,       0, 0),

    PSP_P_FLAG   ("local",      knobinfo_t, wasjustset,  1, 0),
    
    PSP_P_MSTRING("label",      knobinfo_t, label,       NULL, 100),
    PSP_P_MSTRING("units",      knobinfo_t, units,       NULL, 100),
    PSP_P_STRING ("dpyfmt",     knobinfo_t, dpyfmt,      "%8.3f"),
    PSP_P_MSTRING("widdepinfo", knobinfo_t, widdepinfo,  NULL, 0),
    PSP_P_MSTRING("comment",    knobinfo_t, comment,     NULL, 0),

    PSP_P_PLUGIN ("look",       knobinfo_t, look,        LookPluginParser, (void *)LOGD_TEXT),
    PSP_P_LOOKUP ("color",      knobinfo_t, color,       LOGC_NORMAL, psp_colors),
    
    PSP_P_REAL   ("minnorm",    knobinfo_t, rmins[KNOBS_RANGE_NORM], 0.0, 0.0, 0.0),
    PSP_P_REAL   ("maxnorm",    knobinfo_t, rmaxs[KNOBS_RANGE_NORM], 0.0, 0.0, 0.0),
    PSP_P_REAL   ("minyelw",    knobinfo_t, rmins[KNOBS_RANGE_YELW], 0.0, 0.0, 0.0),
    PSP_P_REAL   ("maxyelw",    knobinfo_t, rmaxs[KNOBS_RANGE_YELW], 0.0, 0.0, 0.0),
    PSP_P_REAL   ("mindisp",    knobinfo_t, rmins[KNOBS_RANGE_DISP], 0.0, 0.0, 0.0),
    PSP_P_REAL   ("maxdisp",    knobinfo_t, rmaxs[KNOBS_RANGE_DISP], 0.0, 0.0, 0.0),

    PSP_P_REAL   ("step",       knobinfo_t, incdec_step, 1.0, 0.0, 0.0),

    PSP_P_END()
};

Knob         CreateSimpleKnob(const char      *spec,     // Knob specification
                              int              options,
                              XhWidget         parent,   // Parent widget
                              simpleknob_cb_t  cb,       // Callback proc
                              void            *privptr)  // Pointer to pass to cb
{
  knobinfo_t *ki;
  int         r;
  
    ki = malloc(sizeof(*ki));
    if (ki == NULL) return ki;
    bzero(ki, sizeof(*ki));

    Simple_fakeelem.emlink = &Simple_emethods;

    ki->look = LOGD_TEXTND;

    r = psp_parse(spec, NULL, ki, '=', " \t", "", text2knobinfo);
    if (r != PSP_R_OK)
    {
        fprintf(stderr, "%s(): psp_parse(): %s\n", __FUNCTION__, psp_error());
    }

    if ((options & SIMPLEKNOB_OPT_READONLY) != 0) ki->is_rw = 0;

    if (ki->wasjustset) ki->behaviour |= KNOB_B_NO_WASJUSTSET;
    ki->wasjustset = 0;
    
    if (CreateKnob(ki, parent) != 0)
    {
        free(ki);
        return NULL;
    }
    
    ki->uplink = &Simple_fakeelem;
    
    ki->privptr1 = cb;
    ki->privptr2 = privptr;
    
    return ki;
}

void         SetKnobValue(Knob k, double v)
{
  time_t      timenow   = time(NULL);
  
    k->curv = v;
    
#if 1
    if (
        !k->is_rw  ||
        (
         difftime(timenow, k->usertime) > USERINACTIVITYLIMIT  &&
         !k->wasjustset
        )
       )
        SetControlValue(k, v, 0);

    k->wasjustset = 0;
#else
    if (k->vmtlink != NULL  &&
        k->vmtlink->SetValue != NULL)
        k->vmtlink->SetValue(k, v);
#endif
}

void         SetKnobState(Knob k, colalarm_t newstate)
{
    if (newstate != k->colstate)
    {
        if (k->vmtlink != NULL  &&
            k->vmtlink->Colorize != NULL)
            k->vmtlink->Colorize(k, newstate);
        
        k->colstate = newstate;
    }
}

XhWidget     GetKnobWidget(Knob k)
{
    return k->indicator;
}
