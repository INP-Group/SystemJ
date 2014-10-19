
#define BLK_LINE(l, b) \
    {"-", l,   NULL, NULL, "",  NULL, LOGT_READ,   LOGD_ALARM,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}, \
    {"-", "R", NULL, NULL, "",  NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}, \
    {"-", "",  NULL, "%6.0f", "",  NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}

#define GVI8_LINE(n) \
    /*{"-",    "", NULL, NULL, "",  NULL, LOGT_WRITE1, LOGD_GREENLED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},*/ \
    {"chan" #n, "Канал " #n, NULL, NULL, "",  NULL, LOGT_WRITE1, LOGD_TEXT,      LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}

#define KLS_ELEM(l, b) \
    {                                                    \
        &(elemnet_t){                                    \
            "kls"l, "Клистрон "l,                        \
            "", "",                                      \
            ELEM_MULTICOL, 7*1, 1,                       \
            (logchannet_t []){                           \
                {"mode", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
                    (void *)&(elemnet_t){                \
                        "mode", "",             \
                        "", "",                          \
                        ELEM_MULTICOL, 3*1, 3,           \
                        (logchannet_t []){               \
                            {"-",      " Вкл ",  NULL, NULL,     "panel",  NULL, LOGT_WRITE1, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)/*, .placement="horz=center"*/}, \
                            {"gvi", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
                                (void *)&(elemnet_t){                \
                                    "gvi", "ГВИ8 клистрона "l,               \
                                    "", "?\v?\v?\v?\v?\v?\v?\v?\v?",                          \
                                    ELEM_SUBWIN, 9, 1,           \
                                    (logchannet_t []){               \
                                        {"Scale", "Квант", NULL, NULL, "#L\v#T12.8мкс\v6.4мкс\v3.2мкс\v1.6мкс\v0.8мкс\v0.4мкс\v0.2мкс\v0.1мкс", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}, \
    {"-", l,   NULL, NULL, "",  NULL, LOGT_READ,   LOGD_ALARM,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}, \
                                        GVI8_LINE(0),                 \
                                        GVI8_LINE(1),                 \
                                        GVI8_LINE(2),                 \
                                        GVI8_LINE(3),                 \
                                        GVI8_LINE(4),                 \
                                        GVI8_LINE(5),                 \
                                        GVI8_LINE(6),                 \
                                        GVI8_LINE(7),                 \
                                    },                               \
                                    "noshadow,notitle,nocoltitles,label='ГВИ...'" \
                                },                                   \
                            },                                       \
                            {"uptime", "",       NULL, "%11.0f", "",       NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="horz=right"}, \
                        },                               \
                        "noshadow,notitle,nocoltitles,norowtitles" \
                    }, .placement="horz=fill"          \
                },                                       \
                {"vals", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
                    (void *)&(elemnet_t){                \
                        "vals", "Уставки",               \
                        "", "?\v?",                          \
                        ELEM_MULTICOL, 2*1, 1,           \
                        (logchannet_t []){               \
                            {"pks", "PKS", NULL, "%6.0f", "", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}, \
                            {"dac", "DAC", NULL, "%6.4f", "", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}  \
                        },                               \
                        "noshadow,notitle,nocoltitles" \
                    },                                   \
                },                                       \
                {"-", " ", NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HSEP, LOGK_NOP}, \
                {"-", " Авто ", NULL, NULL,    "panel",     NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="horz=center"}, \
                {"auto", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
                    (void *)&(elemnet_t){                \
                        "auto", "Тренировка",            \
                        "", "",                          \
                        ELEM_MULTICOL, 2*2, 2,           \
                        (logchannet_t []){               \
                            {"max",  "Макс",   NULL, "%6.4f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)}, \
                            {"up",   "^",      NULL, "%6.4f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="horz=right"}, \
                            {"drop", " Сброс! ", NULL, NULL,    "",        NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="horz=right"}, \
                            {"dn",   "v",      NULL, "%6.4f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="horz=right"}, \
                        },                               \
                        "noshadow,notitle,nocoltitles,norowtitles" \
                    },                                   \
                },                                       \
                {"-", " ", NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HSEP, LOGK_NOP}, \
                {"blk", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1), \
                    (void *)&(elemnet_t){                \
                        "blk", "Блокировки",             \
                        "", "",                          \
                        ELEM_MULTICOL, 3*3, 3,           \
                        (logchannet_t []){               \
                            BLK_LINE("Блокировка",  0),  \
                            BLK_LINE("Быстр. блок", 0),  \
                            BLK_LINE("Отраженная",  0),  \
                        },                               \
                        "noshadow,notitle,nocoltitles,norowtitles" \
                    },                                   \
                },                                       \
            },                                           \
            "nocoltitles,norowtitles"                    \
        }, 0                                             \
    }


static groupunit_t kls_grouping[] =
{
    KLS_ELEM("1", 0),
    KLS_ELEM("2", 0),
    KLS_ELEM("3", 0),
    KLS_ELEM("4", 0),
    
    {NULL}
};

static physprops_t kls_physinfo[] =
{
    {0, 1.0, 0},
};
