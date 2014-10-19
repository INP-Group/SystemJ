
static groupunit_t nsukhphase_grouping[] =
{
    {
        &(elemnet_t){
            "plot", "Графики",
            "",
            "",
            ELEM_MULTICOL, 1, 1,
            (logchannet_t []){
                {"plots", "Place for plots...", NULL, NULL, "color=white width=1000,height=400 border=1 textpos=0c0m fontsize=huge fontmod=bolditalic", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP},
            },
            "noshadow,notitle,nocoltitles,norowtitles"
        }, 1
    },


    {NULL}
};

static physprops_t nsukhphase_physinfo[] =
{
};
