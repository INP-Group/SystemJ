
#include "paramstr_parser.h"

#include "Xh.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"

#include "pzframe_data.h"
#include "pzframe_gui.h"
#include "pzframe_knobplugin.h"


typedef struct
{
    char  srvrspec[200];
} pzframe_knobplugin_opts_t;

static psp_paramdescr_t text2pzframe_knobplugin_opts[] =
{
    PSP_P_STRING("server", pzframe_knobplugin_opts_t, srvrspec, ""),
    PSP_P_END()
};


XhWidget PzframeKnobpluginDoCreate(knobinfo_t *ki, XhWidget parent,
                                   pzframe_knobplugin_t         *kpn,
                                   pzframe_knobplugin_realize_t  kp_realize,
                                   pzframe_gui_t *gui, pzframe_gui_dscr_t *gkd,
                                   psp_paramdescr_t *table2, void *rec2,
                                   psp_paramdescr_t *table4, void *rec4,
                                   psp_paramdescr_t *table7, void *rec7)
{
  pzframe_knobplugin_opts_t  opts;
  int                        bigc_n;

  const char                *ph_srvref;
  int                        ph_chan_n;
  cda_serverid_t             ph_defsid;

  const char                *srvrspec;

  void             *recs  [] =
  {
      &(gui->pfr->cfg.param_iv),
      rec2,
      &(gui->pfr->cfg),
      rec4,
      &(gui->look),
      &(opts),
      rec7,
  };
  psp_paramdescr_t *tables[] =
  {
      gkd->ftd->specific_params,
      table2,
      pzframe_data_text2cfg,
      table4,
      pzframe_gui_text2look,
      text2pzframe_knobplugin_opts,
      table7,
  };

    /* Note: gui should be already inited by caller */

    bzero(kpn, sizeof(*kpn));
    kpn->g = gui;

    /* Parse parameters */
    bzero(&opts, sizeof(opts));
    if (psp_parse_v(ki->widdepinfo, NULL,
                    recs, tables, countof(tables),
                    Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                    0) != PSP_R_OK)
    {
        fprintf(stderr, "%s: [%s/\"%s\"].widdepinfo: %s\n",
                __FUNCTION__, ki->ident, ki->label, psp_error());
        return NULL;
    }


    /* Obey readonlyness */
    if (ChlIsReadonly(XhWindowOf(parent))) gui->pfr->cfg.readonly = 1;

    /* bigc_n passing hack */
    bigc_n    = ki->color;
    ki->color = LOGC_NORMAL;

    /*!!! Try to obtain a main-sid-name if srvrspec is "".
          Or, even better -- name of CURRENT default sid (in current sub-hierarchy) */
    if (ki->kind == LOGK_DIRECT  &&
        cda_srcof_physchan(ki->physhandle, &ph_srvref, &ph_chan_n) == 1)
    {
        ph_defsid = cda_sidof_physchan(ki->physhandle);
    }
    else
    {
        ph_srvref = NULL;
        ph_chan_n = -1;
        ph_defsid = CDA_SERVERID_ERROR;
    }

    ////fprintf(stderr, "opts.srvrspec=<%s> ph_srvref=<%s>\n", opts.srvrspec, ph_srvref);
    srvrspec = opts.srvrspec;
    if (srvrspec[0] == '\0') srvrspec = ph_srvref;

    /*!!! No "constr" -- rely on realize()! */
    if (kp_realize != NULL  &&
        kp_realize(kpn, ki) < 0) ;

    /* Create widgets */
    if (gui->d->vmt.realize != NULL  &&
        gui->d->vmt.realize(gui,
                            CNCRTZE(parent),
                            srvrspec, bigc_n,
                            ph_defsid, ph_chan_n) < 0) return NULL;

    return ABSTRZE(gui->top);
}


void     PzframeKnobpluginNoColorize_m(knobinfo_t *ki  __attribute__((unused)),
                                       colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

XhWidget PzframeKnobpluginRedRectWidget(XhWidget parent)
{
    return ABSTRZE(XtVaCreateManagedWidget("", widgetClass, CNCRTZE(parent),
                                           XmNwidth,      20,
                                           XmNheight,     10,
                                           XmNbackground, XhGetColor(XH_COLOR_JUST_RED),
                                           NULL));
}

//////////////////////////////////////////////////////////////////////

enum {ALLOC_INC = 1};

// GetCblistSlot()
GENERIC_SLOTARRAY_DEFINE_GROWING(static, Cblist, pzframe_knobplugin_cb_info_t,
                                 cb, cb, NULL, (void*)1,
                                 0, ALLOC_INC, 0,
                                 kpn->, kpn,
                                 pzframe_knobplugin_t *kpn, pzframe_knobplugin_t *kpn);

static void PzframeKnobpluginEventProc(pzframe_data_t *pfr,
                                       int             reason,
                                       int             info_changed,
                                       void           *privptr)
{
  pzframe_knobplugin_t *kpn = privptr;
  int                         i;

    for (i = 0;  i < kpn->cb_list_allocd;  i++)
        if (kpn->cb_list[i].cb != NULL)
            kpn->cb_list[i].cb(kpn->g->pfr, reason,
                               info_changed, kpn->cb_list[i].privptr);
}


/*
    Note:
        Here we suppose a proper "inheritance", so that
        k->widget_private will point to pzframe_knobplugin_t object
        (which is, usually, at the beginning of an offspring-type object).
 */
pzframe_data_t
    *PzframeKnobpluginRegisterCB(Knob                         k,
                                 const char                  *rqd_type_name,
                                 pzframe_knobplugin_evproc_t  cb,
                                 void                        *privptr,
                                 const char                 **name_p)
{
  pzframe_knobplugin_t         *kpn;
  pzframe_data_t               *pfr;
  int                           id;
  pzframe_knobplugin_cb_info_t *slot;

    if (k == NULL) return NULL;

    /* Access knobplugin... */
    kpn = k->widget_private;
    if (kpn == NULL) return NULL;

    /* ...and pzframe */
    pfr = kpn->g->pfr;

    /* Check type */
    if (strcasecmp(pfr->ftd->type_name, rqd_type_name) != 0)
        return NULL;

    /* "cb==NULL" means "just return pointer to pzframe" */
    if (cb == NULL) return pfr;

    /* Perform registration */

    /* First check if we already imposed a callback to data */
    if (kpn->low_level_registered == 0)
    {
        if (PzframeDataAddEvproc(pfr, PzframeKnobpluginEventProc, kpn) < 0)
            return NULL;
        kpn->low_level_registered = 1;
    }

    id = GetCblistSlot(kpn);
    if (id < 0) return NULL;
    slot = AccessCblistSlot(id, kpn);

    slot->cb      = cb;
    slot->privptr = privptr;

    if (name_p != NULL) *name_p = k->label;

    return pfr;
}

pzframe_gui_t
    *PzframeKnobpluginGetGui    (Knob                         k,
                                 const char                  *rqd_type_name)
{
  pzframe_knobplugin_t         *kpn;
  pzframe_data_t               *pfr;

    if (k == NULL) return NULL;

    /* Access knobplugin... */
    kpn = k->widget_private;
    if (kpn == NULL) return NULL;

    /* ...and pzframe */
    pfr = kpn->g->pfr;

    /* Check type */
    if (strcasecmp(pfr->ftd->type_name, rqd_type_name) != 0)
        return NULL;

    return kpn->g;
}
