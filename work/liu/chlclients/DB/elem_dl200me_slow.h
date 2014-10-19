#ifndef DL200ME_BASE
  #error The "DL200ME_BASE" macro is undefined
#endif
#ifndef DL200ME_IDENT
  #error The "DL200ME_IDENT" macro is undefined
#endif
#ifndef DL200ME_LABEL
  #error The "DL200ME_LABEL" macro is undefined
#endif

#ifndef DL200ME_S_EXT_PFX
  #define DL200ME_S_EXT_PFX ""
#endif
#ifndef DL200ME_S_EXT_SFX
  #define DL200ME_S_EXT_SFX ""
#endif

#ifdef DL200ME_ATTITLE_CHAN
  #define DL200ME_ATTITLE_COUNT 1
#else
  #define DL200ME_ATTITLE_COUNT 0
#endif

#ifndef DL200ME_L_1
  #define  DL200ME_L_1  "1"
#endif
  #ifndef DL200ME_L_2
#define  DL200ME_L_2  "2"
#endif
#ifndef DL200ME_L_3
  #define  DL200ME_L_3  "3"
#endif
#ifndef DL200ME_L_4
  #define  DL200ME_L_4  "4"
#endif
#ifndef DL200ME_L_5
  #define  DL200ME_L_5  "5"
#endif
#ifndef DL200ME_L_6
  #define  DL200ME_L_6  "6"
#endif
#ifndef DL200ME_L_7
  #define  DL200ME_L_7  "7"
#endif
#ifndef DL200ME_L_8
  #define  DL200ME_L_8  "8"
#endif
#ifndef DL200ME_L_9
  #define  DL200ME_L_9  "9"
#endif
#ifndef DL200ME_L_10
  #define  DL200ME_L_10 "10"
#endif
#ifndef DL200ME_L_11
  #define  DL200ME_L_11 "11"
#endif
#ifndef DL200ME_L_12
  #define  DL200ME_L_12 "12"
#endif
#ifndef DL200ME_L_13
  #define  DL200ME_L_13 "13"
#endif
#ifndef DL200ME_L_14
  #define  DL200ME_L_14 "14"
#endif
#ifndef DL200ME_L_15
  #define  DL200ME_L_15 "15"
#endif
#ifndef DL200ME_L_16
  #define  DL200ME_L_16 "16"
#endif


#ifndef NO_GROUPUNIT
    {
#else
    (void *)
#endif
        &(elemnet_t){
            DL200ME_IDENT, DL200ME_LABEL,
            "", "",
            ELEM_MULTICOL, 3 + DL200ME_ATTITLE_COUNT, 1 + (DL200ME_ATTITLE_COUNT << 24),
            (logchannet_t []){
#ifdef DL200ME_ATTITLE_CHAN
  DL200ME_ATTITLE_CHAN ,
#endif
                SUBELEM_START("ctl", "Control", 2, 1)
                    SUBELEM_START("starts", "Starts", 5, 5)
                        {"mode", "Mode",   NULL, NULL,    "#L\v#TOff\vProg\v"DL200ME_S_EXT_PFX"Ext"DL200ME_S_EXT_SFX"\vP+E", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_RUN_MODE)},
                        {"tyk",  "",       NULL, NULL,    "size=giant",                 NULL, LOGT_WRITE1, LOGD_ARROW_RT, LOGK_CALCED, LOGC_IMPORTANT, PHYS_ID(-1),
                            (excmd_t[]){
                                CMD_GETP_I(DL200ME_BASE + DL200ME_CHAN_DO_SHOT),
#ifdef DL200ME_TYK_AUX_READ_CODE
  DL200ME_TYK_AUX_READ_CODE ,
#endif
                                CMD_RET
                            },
                            (excmd_t[]){
                                CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
                                CMD_GETTIME, CMD_SETLCLREG_I(DL200ME_REG_PREV),
                                CMD_PUSH_I(CX_VALUE_COMMAND), CMD_SETP_I(DL200ME_BASE + DL200ME_CHAN_DO_SHOT),
                                CMD_RET
                            },
                        },
#ifdef DL200ME_AUTO_AUX_READ_CODE
                        {"-auto", "каждые", NULL, NULL,    "color=green",               NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                            (excmd_t[]){
                                CMD_GETP_I(DL200ME_BASE + DL200ME_CHAN_AUTO_SHOT),
                                DL200ME_AUTO_AUX_READ_CODE ,
                                CMD_RET
                            },
                            (excmd_t[]){
                                CMD_QRYVAL,
                                CMD_SETP_I(DL200ME_BASE + DL200ME_CHAN_AUTO_SHOT),
                                CMD_RET
                            },
},
#else
                        {"-auto", "каждые", NULL, NULL,    "color=green",               NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_AUTO_SHOT)},
#endif
                        {"secs",  "secs",   "s",  "%4.1f", "withunits",                 NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_AUTO_MSEC), .minnorm=0, .maxnorm=999*1000},
                        {"-lkcont", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                            (void *)&(elemnet_t){
                                "-lkcont", NULL,
                                NULL, NULL,
                                ELEM_INVISIBLE, 1, 1,
                                (logchannet_t []){
                                    {"lock", NULL, NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_SOFT_LOCK)},
                                },
                            },
                        },
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),

#ifdef DL200ME_COUNT_CHAN
  #define DL200ME_LINE2_NUM 6
#else
  #define DL200ME_LINE2_NUM 5
#endif
                    SUBELEM_START("status", "Status", DL200ME_LINE2_NUM, DL200ME_LINE2_NUM)
                        {"serial", "#",  NULL, "%-5.0f", "withlabel",  "Serial #", LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_SERIAL)},
                        {NULL, "  !  ",  NULL, NULL,     "panel",      NULL,       LOGT_READ,   LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_WAS_SHOT)},
#ifdef DL200ME_COUNT_CHAN
                        DL200ME_COUNT_CHAN,
#endif
                        {"fin", "", NULL, NULL, "shape=circle", "Было срабатывание", LOGT_READ, LOGD_GREENLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_WAS_FIN)},
                        {"ilk", "", NULL, NULL, "shape=circle", "Была блокировка",   LOGT_READ, LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_WAS_ILK)},
                        {"left",  "",    NULL, "%6.0f", "", "Осталось до авто-тука", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_AUTO_LEFT), .placement="horz=right"},
                    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", "horz=center"),
                
                SEP_L(),

#ifdef DL200ME_EDITABLE_LABELS
  #define DL200ME_SLOW_COLS 6
  #define DL200ME_SLOW_LCTTL " \v"
  #define DL200ME_LINE_LABEL(n, l) RHLABEL_CELL(__CX_STRINGIZE(n)), USRTXT_CELL("l" __CX_STRINGIZE(n), l)
#else
  #define DL200ME_SLOW_COLS 5
  #define DL200ME_SLOW_LCTTL ""
  #define DL200ME_LINE_LABEL(n, l) HLABEL_CELL(l)
#endif

#define DL200ME_SLOW_LINE(n, l) \
    DL200ME_LINE_LABEL(n, l), \
    {"o" __CX_STRINGIZE(n), "",                NULL, NULL,    "color=black",  NULL, LOGT_WRITE1, LOGD_ONOFF,    LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_OFF_BASE  + n-1)}, \
    {"s" __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%8.2f", NULL,           NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_T_BASE    + n-1), .minnorm=0, .maxnorm=0xEFFFFF*80, .incdec_step=5}, \
    {"b" __CX_STRINGIZE(n), "",                NULL, NULL,    NULL,           NULL, LOGT_WRITE1, LOGD_BLUELED,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_ILAS_BASE + n-1)}, \
    {"a" __CX_STRINGIZE(n), "",                NULL, NULL,    "shape=circle", NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_AILK_BASE + n-1)}

#define DL200ME_SLOW_LINE16(n) \
    {"i" __CX_STRINGIZE(n), "зб-"__CX_STRINGIZE(n), NULL, NULL,    NULL,           NULL, LOGT_WRITE1, LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DL200ME_BASE + DL200ME_CHAN_IINH_BASE + n-1)}

                SUBELEM_START("lines", NULL, 3, 3)
                    SUBELEM_START_CRN(":", "Data", DL200ME_SLOW_LCTTL" \v-\fОтключение\vT, us\v#\fНеблокирующесть\v? \fБлокировка?", NULL, 16*(DL200ME_SLOW_COLS), (DL200ME_SLOW_COLS))
                        DL200ME_SLOW_LINE(1,  DL200ME_L_1),
                        DL200ME_SLOW_LINE(2,  DL200ME_L_2),
                        DL200ME_SLOW_LINE(3,  DL200ME_L_3),
                        DL200ME_SLOW_LINE(4,  DL200ME_L_4),
                        DL200ME_SLOW_LINE(5,  DL200ME_L_5),
                        DL200ME_SLOW_LINE(6,  DL200ME_L_6),
                        DL200ME_SLOW_LINE(7,  DL200ME_L_7),
                        DL200ME_SLOW_LINE(8,  DL200ME_L_8),
                        DL200ME_SLOW_LINE(9,  DL200ME_L_9),
                        DL200ME_SLOW_LINE(10, DL200ME_L_10),
                        DL200ME_SLOW_LINE(11, DL200ME_L_11),
                        DL200ME_SLOW_LINE(12, DL200ME_L_12),
                        DL200ME_SLOW_LINE(13, DL200ME_L_13),
                        DL200ME_SLOW_LINE(14, DL200ME_L_14),
                        DL200ME_SLOW_LINE(15, DL200ME_L_15),
                        DL200ME_SLOW_LINE(16, DL200ME_L_16),
                    SUBELEM_END("notitle,noshadow,norowtitles", ""),
                    VSEP_L(),
                    SUBELEM_START_CRN(":", "16", " ", NULL, 16*1, 1)
                        DL200ME_SLOW_LINE16(1),
                        DL200ME_SLOW_LINE16(2),
                        DL200ME_SLOW_LINE16(3),
                        DL200ME_SLOW_LINE16(4),
                        DL200ME_SLOW_LINE16(5),
                        DL200ME_SLOW_LINE16(6),
                        DL200ME_SLOW_LINE16(7),
                        DL200ME_SLOW_LINE16(8),
                        DL200ME_SLOW_LINE16(9),
                        DL200ME_SLOW_LINE16(10),
                        DL200ME_SLOW_LINE16(11),
                        DL200ME_SLOW_LINE16(12),
                        DL200ME_SLOW_LINE16(13),
                        DL200ME_SLOW_LINE16(14),
                        DL200ME_SLOW_LINE16(15),
                        DL200ME_SLOW_LINE16(16),
                    SUBELEM_END("notitle,noshadow,norowtitles", ""),
                SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
            },
#ifdef DEFSERVER
            "defserver=" DEFSERVER ","
#endif /* DEFSERVER */
            "nocoltitles,norowtitles"
        }
#ifndef NO_GROUPUNIT
#ifdef FROMNEWLINE
        ,GU_FLAG_FROMNEWLINE
#endif /* FROMNEWLINE */
    }
#endif /* NO_GROUPUNIT */

#undef DL200ME_SLOW_COLS
#undef DL200ME_SLOW_LCTTL
#undef DL200ME_SLOW_LINE
#undef DL200ME_LINE_LABEL
#undef DL200ME_LINE2_NUM
#undef DL200ME_ATTITLE_COUNT

#undef DL200ME_BASE
#undef DL200ME_IDENT
#undef DL200ME_LABEL
#undef DL200ME_S_EXT_PFX
#undef DL200ME_S_EXT_SFX
#undef DL200ME_EDITABLE_LABELS
#undef NO_GROUPUNIT
#undef FROMNEWLINE
#undef DEFSERVER
#undef DL200ME_REG_AUTO
#undef DL200ME_REG_SECS
#undef DL200ME_REG_PREV
#undef DL200ME_COUNT_CHAN
#undef DL200ME_AUTO_AUX_READ_CODE
#undef DL200ME_TYK_AUX_READ_CODE
#undef DL200ME_ATTITLE_CHAN

#undef DL200ME_L_1
#undef DL200ME_L_2
#undef DL200ME_L_3
#undef DL200ME_L_4
#undef DL200ME_L_5
#undef DL200ME_L_6
#undef DL200ME_L_7
#undef DL200ME_L_8
#undef DL200ME_L_9
#undef DL200ME_L_10
#undef DL200ME_L_11
#undef DL200ME_L_12
#undef DL200ME_L_13
#undef DL200ME_L_14
#undef DL200ME_L_15
#undef DL200ME_L_16
