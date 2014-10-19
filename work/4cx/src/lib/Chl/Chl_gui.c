#include "Chl_includes.h"


static void _ChlSetPhys_m(DataKnob k, double v)
{
    CdrSetKnobValue(get_knob_subsys(k), k, v, 0);
}

static dataknob_cont_vmt_t ChlTopVMT =
{
    {.type = DATAKNOB_CONT},
    _ChlSetPhys_m,
    _ChlShowProps_m,
    NULL, // _ChlShowBigVal_m,
    NULL, // _ChlToHistPlot_m,
    NULL,
    NULL,
    NULL,
    NULL,
};

static data_knob_t ChlTopCont =
{
    .type    = DATAKNOB_CONT,
    .vmtlink = (dataknob_unif_vmt_t *) &ChlTopVMT
};


void _ChlBindMethods(DataKnob k)
{
    if (k->uplink != NULL)
        fprintf(stderr, "%s(): '%s'.uplink!=NULL, ->'%s'\n",
                __FUNCTION__, k->ident, k->uplink->ident);

    k->uplink = &ChlTopCont;
}
