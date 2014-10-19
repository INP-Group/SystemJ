
#define JUST_KNOB {"just-knob", "JustKnob", NULL, NULL, NULL, "Just a knob", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}

static groupunit_t knobtest_grouping[] =
{
    {
        &(elemnet_t){
            "top-tabber", "ZZZ",
            "",
            "?\v?\v?\v?\v?\v?\v?",
            ELEM_TABBER, 4, 1,
            (logchannet_t []){
                {"ident", "HLabel", NULL, NULL, "icon=zicon16-1", NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP, LOGC_HEADING},
                {"ident", "Button", NULL, NULL, "icon=icon16-2", NULL, LOGT_READ, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL*0+LOGC_HEADING, PHYS_ID(0)},
                {"ident", "On\nOff", NULL, NULL, "icon=icon16-3", NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
                {"sel", "Selector", NULL, NULL, "#F5i,1,3,=#!=icon16-%i", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
                JUST_KNOB,
                JUST_KNOB,
                JUST_KNOB,
            },
            "bottom"
        }, 1
    },

    {NULL}
};



static physprops_t knobtest_physinfo[] =
{
};
