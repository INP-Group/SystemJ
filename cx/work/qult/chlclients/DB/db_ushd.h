
#define USHD_LINE(base, title) \
    {"-go_lft", title,  NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_ARROW_LT, LOGK_DIRECT, LOGC_NORMAL,    PHYS_ID(base)}, \
    {"value",   NULL,   NULL, "%7.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXTND,   LOGK_DIRECT, LOGC_NORMAL,    PHYS_ID(base)}, \
    {"-go_rgt", NULL,   NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_ARROW_RT, LOGK_DIRECT, LOGC_NORMAL,    PHYS_ID(base)}, \
    {"-stop",   "STOP", NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL,    PHYS_ID(base)}, \
    {"manu",    "",     NULL, NULL, "#T \vM", NULL, LOGT_READ,  LOGD_SELECTOR, LOGK_DIRECT, LOGC_IMPORTANT, PHYS_ID(base)}, \
    {"acc",     NULL,   NULL, "%8.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL,    PHYS_ID(base)}, \
    {"-zsum",   "0",    NULL, NULL,    NULL, NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL,    PHYS_ID(base)}

static groupunit_t ushd_grouping[] =
{
    {
        &(elemnet_t){
            "-ushd", "Управление шаговиками",
            "<\vШаги\v>\v\v\vСумма", "?\v?\v?\v?\v?\v?\v?\v?\v",
            ELEM_MULTICOL, 7*8, 7,
            (logchannet_t []){
                USHD_LINE(0, "УШД 1"),
                USHD_LINE(0, "УШД 2"),
                USHD_LINE(0, "УШД 3"),
                USHD_LINE(0, "УШД 4"),
                USHD_LINE(0, "УШД 5"),
                USHD_LINE(0, "УШД 6"),
                USHD_LINE(0, "УШД 7"),
                USHD_LINE(0, "УШД 8"),
            },
            "notitle,noshadow"
        }, 1
    },

    {NULL}
};

static physprops_t ushd_physinfo[] =
{
    {0, 1.0, 0},
};
