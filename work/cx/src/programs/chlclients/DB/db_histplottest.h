
#define JUST_CHAN(n) \
    {"chan" __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}

static groupunit_t histplottest_grouping[] =
{
    {
        &(elemnet_t){
            "channels", "Channels", NULL, NULL,
            ELEM_SINGLECOL, 8, 1,
            (logchannet_t []){
                JUST_CHAN(0),
                JUST_CHAN(1),
                JUST_CHAN(2),
                JUST_CHAN(3),
                JUST_CHAN(4),
                JUST_CHAN(5),
                JUST_CHAN(6),
                JUST_CHAN(7),
            },
            ""
        }, 1
    },

    {
        &(elemnet_t){
            "histplot", "History Plot", NULL, NULL,
            ELEM_SINGLECOL, 1, 1,
            (logchannet_t []){
                {"histplot", "HistPlot", NULL, NULL, "width=800,height=200,plot1=.chan1,plot2=.chan3,plot3=.chan4,fixed", NULL, LOGT_READ, LOGD_HISTPLOT, LOGK_NOP},
            },
            "nocoltitles,norowtitles"
        }, 0
    },

    {NULL}
};

static physprops_t histplottest_physinfo[] =
{
};
