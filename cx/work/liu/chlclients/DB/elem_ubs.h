#ifndef UBS_BASE
  #error The "UBS_BASE" macro is undefined
#endif
#ifndef UBS_IDENT
  #error The "UBS_IDENT" macro is undefined
#endif
#ifndef UBS_LABEL
  #error The "UBS_LABEL" macro is undefined
#endif

#ifdef UBS_MODE_INCTLS
  #define PANOV_MAINELEM_COUNT 4
  #define PANOV_MAINELEM_NCOLS 1
  #define PANOV_CTLELEM_COUNT  7
#else
  #define PANOV_MAINELEM_COUNT 5
  #define PANOV_MAINELEM_NCOLS 1 + (1 << 24)
  #define PANOV_CTLELEM_COUNT  4
#endif

#define PANOV_MODE_CELL() \
    {"mode", NULL, NULL, NULL,                                                                          \
        "#L\v#H\v#=\v#TС\fСон\flit=blue\vТ\fТест\flit=yellow\vР\fРабота\flit=green\vО\fОтладка\flit=gray\v\nП\fПрограммирование\flit=brown\vА\fАвария\flit=red", \
        NULL, LOGT_WRITE1, LOGD_CHOICEBS, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_SET_MODE), \
        (excmd_t[]){CMD_RETSEP_I(0), CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_SET_MODE), CMD_RET},          \
        .minyelw=0, .maxyelw=4,                                                                         \
        .placement="vert=center"}

#define PANOV_STAT(id, label, vict, cn) \
    {id,  label, NULL, NULL, "offcol=red,shape=default,victim="vict, NULL, LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn), .minyelw=0.5, .maxyelw=1.5}

#define PANOV_RANGE_LINE(id, dpyfmt, step) \
    {__CX_STRINGIZE(id) "_min", "[", NULL, dpyfmt, "withlabel",           NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_MIN_, id)), .minnorm=0, .maxnorm=100*0, .incdec_step=step}, \
    {__CX_STRINGIZE(id) "_max", "-", "]",  dpyfmt, "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_MAX_, id)), .minnorm=0, .maxnorm=100*0, .incdec_step=step}, \
    {__CX_STRINGIZE(id) "_clb", "Калибр.", NULL, NULL, NULL,              NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_CLB_, id))}, \
    {__CX_STRINGIZE(id) "_cst", "/", "%",  "%3.0f", NULL,                 NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_CST_, id))}

#define PANOV_ILK_LINE(l, id) \
    {NULL, l,    NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_LKM_, id))}, \
    {NULL, NULL, NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_ILK_, id))}


#define PANOV_SET_AND_MES_BEG(units, dpyfmt, id, max, step) \
    {__CX_STRINGIZE(id) "s", __CX_STRINGIZE(id) "S", units, dpyfmt,  NULL, NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_SET_, id)), .minnorm=0, .maxnorm=max, .incdec_step=step}, \
    {__CX_STRINGIZE(id) "m", __CX_STRINGIZE(id) "M", units, dpyfmt,  NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_MES_, id))
#define PANOV_SET_AND_MES_END() \
    }

#define PANOV_ARC_CELL(units, dpyfmt, id, auxparams...) \
    {__CX_STRINGIZE(id) "m", __CX_STRINGIZE(id) "M", units,  dpyfmt,  NULL, NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + __CX_CONCATENATE(PANOV_UBS_CHAN_MES_, id)) \
    , ##auxparams}


#define PANOV_HEAT_LINE(n) \
    RGSWCH_LINE("heat_" __CX_STRINGIZE(n), "Heat " __CX_STRINGIZE(n),                    \
    "Выкл", "Вкл",                                                                       \
    UBS_BASE + __CX_CONCATENATE(__CX_CONCATENATE(PANOV_UBS_CHAN_STAT_ST, n), _HEATIN),   \
    UBS_BASE + __CX_CONCATENATE(__CX_CONCATENATE(PANOV_UBS_CHAN_ST, n),      _HEAT_OFF), \
    UBS_BASE + __CX_CONCATENATE(__CX_CONCATENATE(PANOV_UBS_CHAN_ST, n),      _HEAT_ON))

#define PANOV_UDGS_LINE() \
    RGSWCH_LINE("udgs_" __CX_STRINGIZE(n), "Udgs ",                    \
    "Выкл", "Вкл",                                                                       \
    UBS_BASE + PANOV_UBS_CHAN_STAT_DGS_IS_ON,   \
    UBS_BASE + PANOV_UBS_CHAN_DGS_UDGS_OFF, \
    UBS_BASE + PANOV_UBS_CHAN_DGS_UDGS_ON)

#define PANOV_TIME_LINE(n, filename, c_stat, r_time, r_wass, r_prvt)      \
    {"wktime" __CX_STRINGIZE(n), NULL, "m", "%8.0f", NULL, "Total on-time", LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t []){                                                  \
            /*CMD_RET_I(12345.67),*/                                       \
            CMD_PUSH_I(-1), CMD_SETLCLREGDEFVAL_I(r_wass),             \
            CMD_GETLCLREG_I(r_wass), CMD_PUSH_I(0),                    \
            CMD_IF_GE_TEST, CMD_GOTO_I("already_loaded"),              \
            CMD_PUSH_I(0), CMD_LOADVAL_I(filename),                    \
            CMD_SETLCLREG_I(r_time),                                   \
            CMD_PUSH_I(0), CMD_SETLCLREG_I(r_wass),                    \
            CMD_PUSH_I(0), CMD_SETLCLREG_I(r_prvt),                    \
            CMD_LABEL_I("already_loaded"),                             \
                                                                       \
            /* Is heat on? */                                          \
            CMD_GETP_I(c_stat),  CMD_PUSH_I(0.9),                      \
            CMD_IF_LT_TEST, CMD_GOTO_I("ret"),                         \
            /* Get current time */                                     \
            CMD_GETTIME, CMD_SETREG_I(0),                              \
                                                                       \
            /* Was heat previously "on"? */                            \
            CMD_GETLCLREG_I(r_wass), CMD_PUSH_I(0.0),                  \
            CMD_IF_GT_TEST, CMD_GOTO_I("was_on"),                      \
            /* No?  Set current moment as start */                     \
            CMD_GETREG_I(0), CMD_SETLCLREG_I(r_prvt),                  \
            CMD_LABEL_I("was_on"),                                     \
                                                                       \
            /* Add elapsed time to a sum... */                         \
            CMD_GETREG_I(0), CMD_GETLCLREG_I(r_prvt), CMD_SUB,         \
            CMD_GETLCLREG_I(r_time), CMD_ADD, CMD_SETLCLREG_I(r_time), \
            /* ...save sum... */                                       \
            CMD_GETLCLREG_I(r_time), CMD_SAVEVAL_I(filename),          \
            /* ...and promote "previous" time to current */            \
            CMD_GETREG_I(0), CMD_SETLCLREG_I(r_prvt),                  \
                                                                       \
            CMD_LABEL_I("ret"),                                        \
            CMD_GETP_I(c_stat), CMD_SETLCLREG_I(r_wass),               \
            CMD_GETLCLREG_I(r_time), CMD_DIV_I(60), CMD_RET            \
        }                                                              \
    }
    

#ifndef NO_GROUPUNIT
    {
#else
    (void *)
#endif
        &(elemnet_t){
            UBS_IDENT, UBS_LABEL,
            "", "",
            ELEM_MULTICOL, PANOV_MAINELEM_COUNT, PANOV_MAINELEM_NCOLS,
            (logchannet_t []){

#ifndef UBS_MODE_INCTLS
                PANOV_MODE_CELL(),
#endif

                SUBELEM_START("ctl", "Controls", PANOV_CTLELEM_COUNT, 7)
                    SUBELEM_START("hwaddr", NULL, 2, 2)
                        {"hwaddr-hi", NULL, NULL, "%1.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                            (excmd_t[]){
                                CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_HWADDR),
                                CMD_DIV_I(16), CMD_DUP, CMD_MOD_I(1), CMD_SUB,
                                CMD_RET
                            }},
                        {"hwaddr-lo", "", NULL, "%1.0f", "#T0\v1\v2\v3\v4\v5\v6\v7\v8\v9\vA\vb\vC\vd\vE\vF", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                            (excmd_t[]){
                                CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_HWADDR),
                                CMD_MOD_I(16),
                                CMD_RET
                            }},
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
                    RGSWCH_LINE_RED("power", "Power", "Выкл", "Вкл", UBS_BASE + PANOV_UBS_CHAN_STAT_UBS_POWER, UBS_BASE + PANOV_UBS_CHAN_POWER_OFF, UBS_BASE + PANOV_UBS_CHAN_POWER_ON),
#ifdef UBS_MODE_INCTLS
                    VSEP_L(),
                    PANOV_MODE_CELL(),
                    VSEP_L(),
#endif
                    BUTT_CELL("reboot_ubs", "Перезагрузка",     UBS_BASE + PANOV_UBS_CHAN_REBOOT_UBS),
                    BUTT_CELL("-rst_ilks",  "Сбр.блк",          UBS_BASE + PANOV_UBS_CHAN_RST_UBS_ILK),

                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", ""),

                SEP_L(),

#define PANOV_HEAT_CFLA(stat_cn, mes_cn, reg_n)        \
    (excmd_t []){                                      \
        CMD_RETSEP_I(0),                               \
        /* return 1 if reg<=0 */                       \
        CMD_GETLCLREG_I(reg_n), CMD_PUSH_I(0),         \
        CMD_IF_LE_TEST, CMD_BREAK_I(1),                \
        /* return 1 if heat is off */                  \
        CMD_GETP_I(stat_cn), CMD_PUSH_I(0.9),          \
        CMD_IF_LT_TEST, CMD_BREAK_I(1),                \
        /* Okay, let's return MES/GOOD */              \
        CMD_GETP_I(mes_cn), CMD_GETLCLREG_I(reg_n),    \
        CMD_DIV, CMD_RET                               \
    }
#define PANOV_ARC_MINNORM 12.0
#define PANOV_ARC_MAXNORM 33.0
#define PANOV_ARC_CFLA( stat_cn, mes_cn)               \
    (excmd_t []){                                      \
        CMD_RETSEP_I(0),                               \
        /* return 1 if heat is off */                  \
        CMD_GETP_I(stat_cn), CMD_PUSH_I(0.9),          \
        CMD_IF_LT_TEST, CMD_BREAK_I(PANOV_ARC_MINNORM),\
        /* Okay, let's return measured value */        \
        CMD_GETP_I(mes_cn), CMD_RET                    \
    }
#define PANOV_UDGS_CFLA(stat_cn, mes_cn, good_val)     \
    (excmd_t []){                                      \
        CMD_RETSEP_I(0),                               \
        /* return 1 if degausser is off */             \
        CMD_GETP_I(stat_cn), CMD_PUSH_I(0.9),          \
        CMD_IF_LT_TEST, CMD_BREAK_I(good_val),         \
        /* Okay, let's return measured value */        \
        CMD_GETP_I(mes_cn), CMD_RET                    \
    }


                SUBELEM_START("sam", "SAM", 8*3, 8)
                    /* Starter 2 */
                    PANOV_STAT("st2_s", "БЗ2", "ST2_IHEATs", UBS_BASE + PANOV_UBS_CHAN_STAT_ST2_ONLINE),
                    PANOV_HEAT_LINE(2),
                    RHLABEL_CELL("Iнак2"),
                    PANOV_SET_AND_MES_BEG("A",   "%4.2f", ST2_IHEAT, 2.91, 0.01)
#ifdef UBS_R_IH2N
                        ,
                        PANOV_HEAT_CFLA(UBS_BASE + PANOV_UBS_CHAN_STAT_ST2_HEATIN,
                                        UBS_BASE + PANOV_UBS_CHAN_MES_ST2_IHEAT,
                                        UBS_R_IH2N),
                        .minnorm=1.-UBS_HEAT_ALWD_DELTA, .maxnorm=1.+UBS_HEAT_ALWD_DELTA
#endif
                    PANOV_SET_AND_MES_END(),
                    PANOV_ARC_CELL("mA", "%3.0f", ST2_IARC,
                                   PANOV_ARC_CFLA(UBS_BASE + PANOV_UBS_CHAN_STAT_ST2_HEATIN,
                                                  UBS_BASE + PANOV_UBS_CHAN_MES_ST2_IARC),
                                   .minnorm=PANOV_ARC_MINNORM, .maxnorm=PANOV_ARC_MAXNORM),
#ifdef UBS_HEAT2TIME_FILE
                    PANOV_TIME_LINE(2, UBS_HEAT2TIME_FILE,
                                    UBS_BASE + PANOV_UBS_CHAN_STAT_ST2_HEATIN,
                                    UBS_R_ST2T,
                                    UBS_R_ST2W,
                                    UBS_R_ST2P
                                   ),
#else
                    EMPTY_CELL(),
#endif
                    {NULL, "",    NULL, NULL, "shape=default", "Блокировка БЗ2", LOGT_READ,   LOGD_REDLED,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                     (excmd_t []){
                         CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_ILK_UBS_ST2),
                         CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_LKM_UBS_ST2),
                         CMD_MUL, CMD_RET
                     },
                     .minyelw=-0.5, .maxyelw=+0.5
                    },
                    /* Starter 1 */
                    PANOV_STAT("st1_s", "БЗ1", "ST1_IHEATs", UBS_BASE + PANOV_UBS_CHAN_STAT_ST1_ONLINE),
                    PANOV_HEAT_LINE(1),
                    RHLABEL_CELL("Iнак1"),
                    PANOV_SET_AND_MES_BEG("A",   "%4.2f", ST1_IHEAT, 2.91, 0.01)
#ifdef UBS_R_IH1N
                        ,
                        PANOV_HEAT_CFLA(UBS_BASE + PANOV_UBS_CHAN_STAT_ST1_HEATIN,
                                        UBS_BASE + PANOV_UBS_CHAN_MES_ST1_IHEAT,
                                        UBS_R_IH1N),
                        .minnorm=1.-UBS_HEAT_ALWD_DELTA, .maxnorm=1.+UBS_HEAT_ALWD_DELTA
#endif
                    PANOV_SET_AND_MES_END(),
                    PANOV_ARC_CELL("mA", "%3.0f", ST1_IARC,
                                   PANOV_ARC_CFLA(UBS_BASE + PANOV_UBS_CHAN_STAT_ST1_HEATIN,
                                                  UBS_BASE + PANOV_UBS_CHAN_MES_ST1_IARC),
                                   .minnorm=PANOV_ARC_MINNORM, .maxnorm=PANOV_ARC_MAXNORM),
#ifdef UBS_HEAT1TIME_FILE
                    PANOV_TIME_LINE(1, UBS_HEAT1TIME_FILE,
                                    UBS_BASE + PANOV_UBS_CHAN_STAT_ST1_HEATIN,
                                    UBS_R_ST1T,
                                    UBS_R_ST1W,
                                    UBS_R_ST1P
                                   ),
#else
                    EMPTY_CELL(),
#endif
                    {NULL, "",    NULL, NULL, "shape=default", "Блокировка БЗ1", LOGT_READ,   LOGD_REDLED,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                     (excmd_t []){
                         CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_ILK_UBS_ST1),
                         CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_LKM_UBS_ST1),
                         CMD_MUL, CMD_RET
                     },
                     .minyelw=-0.5, .maxyelw=+0.5
                    },
                    /* Degausser */
                    PANOV_STAT("dgs_s", "БРМ", "DGS_UDGSs", UBS_BASE + PANOV_UBS_CHAN_STAT_DGS_ONLINE),
                    PANOV_UDGS_LINE(),
                    RHLABEL_CELL("Uрзм"),
                    PANOV_SET_AND_MES_BEG("V",  "%4.0f", DGS_UDGS, 1200, 10),
                        PANOV_UDGS_CFLA(UBS_BASE + PANOV_UBS_CHAN_STAT_DGS_IS_ON,
                                        UBS_BASE + PANOV_UBS_CHAN_MES_DGS_UDGS,
                                        500),
                        .minnorm=200, .maxnorm=900,
                        .minyelw=100, .maxyelw=1000
                    PANOV_SET_AND_MES_END(),
                    EMPTY_CELL(),
                    EMPTY_CELL(),
                    {NULL, "",    NULL, NULL, "shape=default", "Блокировка БРМ", LOGT_READ,   LOGD_REDLED,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                     (excmd_t []){
                         CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_ILK_UBS_DGS),
                         CMD_GETP_I(UBS_BASE + PANOV_UBS_CHAN_LKM_UBS_DGS),
                         CMD_MUL, CMD_RET
                     },
                     .minyelw=-0.5, .maxyelw=+0.5
                    },
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),

                SUBELEM_START("service", "Служебные каналы", 2, 1)
                    SUBELEM_START("ctl", "Controls", 2, 2)
                        SUBELEM_START_CRN("limits", "Диапазоны und калибровка", NULL, "Iнак2\vIнак1\vUразм", 3*4, 4)
                            PANOV_RANGE_LINE(ST2_IHEAT, "%4.2f", 0.01),
                            PANOV_RANGE_LINE(ST1_IHEAT, "%4.2f", 0.01),
                            PANOV_RANGE_LINE(DGS_UDGS,  "%4.0f", 10),
                        SUBELEM_END("nofold,nocoltitles", NULL),
                        SUBELEM_START("misc", "Управление", 3, 1)
                            {"tht_time", "T прогр. нак.", "s",  "%3.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXTND, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_SET_THT_TIME), .minnorm=0, .maxnorm=255},
                            BUTT_CELL(NULL, "Перечитать HWADDR",    UBS_BASE + PANOV_UBS_CHAN_REREAD_HWADDR),
                            BUTT_CELL(NULL, "Перезагрузить всех",   UBS_BASE + PANOV_UBS_CHAN_REBOOT_ALL),
                        SUBELEM_END("nofold,nocoltitles,norowtitles", NULL),
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),

                    SUBELEM_START("ilk", "Блокировки", 5, 5)
                        SUBELEM_START("st1", "БЗ1", 3, 1)
                            SUBELEM_START("ilk_ctl", "Упр.блок.", 3, 3)
                                {NULL, "БЗ1", NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_LKM_UBS_ST1)},
                                BUTT_CELL(NULL, "Сброс", UBS_BASE + PANOV_UBS_CHAN_RST_ST1_ILK),
                                {NULL, "",    NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_ILK_UBS_ST1), .placement="horz=right"}
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
                            SEP_L(),
                            SUBELEM_START("ilocks", "Блокировки", 16, 2)
                                PANOV_ILK_LINE("0 Нет блока",        ST1_CONNECT),
                                PANOV_ILK_LINE("1 Накал выкл.",      ST1_HEAT_OFF),
                                PANOV_ILK_LINE("2 ...не прогрет",    ST1_HEAT_COLD),
                                PANOV_ILK_LINE("3 Iнак вне диап.",   ST1_HEAT_RANGE),
                                PANOV_ILK_LINE("4 Iдуги вне диап.",  ST1_ARC_RANGE),
                                PANOV_ILK_LINE("5 Нет дуги",         ST1_ARC_FAST),
                                PANOV_ILK_LINE("6 Нет высокого",     ST1_HIGH_FAST),
                                PANOV_ILK_LINE("7 -резерв-",         ST1_RES7),
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                        VSEP_L(),
                        SUBELEM_START("st2", "БЗ2", 3, 1)
                            SUBELEM_START("ilk_ctl", "Упр.блок.", 3, 3)
                                {NULL, "БЗ2", NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_LKM_UBS_ST2)},
                                BUTT_CELL(NULL, "Сброс", UBS_BASE + PANOV_UBS_CHAN_RST_ST2_ILK),
                                {NULL, "",    NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_ILK_UBS_ST2), .placement="horz=right"}
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
                            SEP_L(),
                            SUBELEM_START("ilocks", "Блокировки", 16, 2)
                                PANOV_ILK_LINE("0 Нет блока",        ST2_CONNECT),
                                PANOV_ILK_LINE("1 Накал выкл.",      ST2_HEAT_OFF),
                                PANOV_ILK_LINE("2 ...не прогрет",    ST2_HEAT_COLD),
                                PANOV_ILK_LINE("3 Iнак вне диап.",   ST2_HEAT_RANGE),
                                PANOV_ILK_LINE("4 Iдуги вне диап.",  ST2_ARC_RANGE),
                                PANOV_ILK_LINE("5 Нет дуги",         ST2_ARC_FAST),
                                PANOV_ILK_LINE("6 Нет высокого",     ST2_HIGH_FAST),
                                PANOV_ILK_LINE("7 -резерв-",         ST2_RES7),
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                        VSEP_L(),
                        SUBELEM_START("dgs", "БРМ", 3, 1)
                            SUBELEM_START("ilk_ctl", "Упр.блок.", 3, 3)
                                {NULL, "БРМ", NULL, NULL, "",             NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_LKM_UBS_DGS)},
                                BUTT_CELL(NULL, "Сброс", UBS_BASE + PANOV_UBS_CHAN_RST_DGS_ILK),
                                {NULL, "",    NULL, NULL, "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(UBS_BASE + PANOV_UBS_CHAN_ILK_UBS_DGS), .placement="horz=right"}
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", "horz=fill"),
                            SEP_L(),
                            SUBELEM_START("ilocks", "Блокировки", 16, 2)
                                PANOV_ILK_LINE("0 Нет блока",        DGS_CONNECT),
                                PANOV_ILK_LINE("1 Uрзм выкл.",       DGS_UDGS_OFF),
                                PANOV_ILK_LINE("2 -резерв-",         DGS_RES2),
                                PANOV_ILK_LINE("3 Uрзм вне диап.",   DGS_UDGS_RANGE),
                                PANOV_ILK_LINE("4 -резерв-",         DGS_RES4),
                                PANOV_ILK_LINE("5 Нет Iрзм",         DGS_DGS_FAST),
                                PANOV_ILK_LINE("6 -резерв-",         DGS_RES6),
                                PANOV_ILK_LINE("7 -резерв-",         DGS_RES7),
                            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
                    SUBELEM_END("nofold,nocoltitles,norowtitles", NULL),
                SUBELEM_END("noshadow,nocoltitles,norowtitles, folded,fold_h", "horz=center"),
                
            }
            ,
#ifdef DEFSERVER
            "defserver=" DEFSERVER ","
#endif /* DEFSERVER */
#ifdef UBS_OPTIONS
            UBS_OPTIONS ","
#endif
            "nocoltitles,norowtitles"
        }
#ifndef NO_GROUPUNIT
#ifdef FROMNEWLINE
        ,GU_FLAG_FROMNEWLINE
#endif /* FROMNEWLINE */
    }
#endif /* NO_GROUPUNIT */

#undef PANOV_STAT
#undef PANOV_RANGE_LINE
#undef PANOV_ILK_LINE
#undef PANOV_HEAT_LINE
#undef PANOV_HEAT_CFLA
#undef PANOV_ARC_CFLA
#undef PANOV_ARC_MINNORM
#undef PANOV_ARC_MAXNORM

#undef UBS_BASE
#undef UBS_IDENT
#undef UBS_LABEL
#undef UBS_OPTIONS
#undef UBS_MODE_INCTLS
#undef NO_GROUPUNIT
#undef FROMNEWLINE
#undef DEFSERVER

#undef UBS_R_IH2N
#undef UBS_R_IH1N
