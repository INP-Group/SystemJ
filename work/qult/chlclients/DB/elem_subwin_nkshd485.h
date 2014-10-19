#ifndef NKSHD485_BASE
  #error The "NKSHD485_BASE" macro is undefined
#endif
#ifndef NKSHD485_NAME
  #error The "NKSHD485_NAME" macro is undefined
#endif
#ifndef NKSHD485_IDENT
  #error The "NKSHD485_IDENT" macro is undefined
#endif


#define COLORLED_LINE(name, l, comnt, dtype, cn) \
    {name, "\n"l, NULL, NULL, NULL, comnt, LOGT_READ,   dtype, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define INPUTLED_LINE(name, l, comnt, dtype, cn) \
    {name, "\n"l, NULL, NULL, NULL, comnt, LOGT_WRITE1, dtype, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(cn)}

#define BUT_LINE(name, title, n) \
    {name, title, "", NULL, NULL, NULL, LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}

SUBWINV4_START(NKSHD485_IDENT "-485-subwin", NKSHD485_NAME ": 485", 1)
    SUBELEM_START("controls", "", 3, 1)
        SUBELEM_START("info", "Info", 2, 2)
            {"devver", "Dev.ver.", NULL, "%3.1f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_DEV_VERSION)},
            {"devsrl", "serial#",  NULL, "%3.0f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_DEV_SERIAL)},
        SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
        SUBELEM_START("config", "Config", 7, 1 + (1 << 24))
            {NULL, "Save", NULL, NULL, NULL, "Save config to NVRAM", LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_DIM, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_SAVE_CONFIG)},
            SUBELEM_START("curr", "Current", 2, 2)
                {"work", "Work", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_WORK_CURRENT)},
                {"hold", "Hold", "", "", "#T0A\v0.2A\v0.3A\v0.5A\v0.6A\v1.0A\v2.0A\v3.5A", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_HOLD_CURRENT)},
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            {"hold_delay", "Hold dly", "1/30s", "%3.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_HOLD_DELAY), .minnorm=0, .maxnorm=255, .placement="horz=right"},
            SUBELEM_START("cfg_byte", "CfgByte", 9, 9)
                {"cfg_byte", "Byte", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BYTE)},
                INPUTLED_LINE("accleave", "b", "Acc.Leave",            LOGD_AMBERLED, NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 7),
                INPUTLED_LINE("leavek",   "k", "Leave Ks",             LOGD_AMBERLED, NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 6),
                INPUTLED_LINE("softk",    "p", "Soft Ks",              LOGD_AMBERLED, NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 5),
                INPUTLED_LINE("k3",       "s", "Sensor",               LOGD_REDLED,   NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 4),
                INPUTLED_LINE("k2",       "+", "K+",                   LOGD_REDLED,   NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 3),
                INPUTLED_LINE("k1",       "-", "K-",                   LOGD_REDLED,   NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 2),
                INPUTLED_LINE("bit1",     "1", "Unused",               LOGD_ONOFF,    NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 1),
                INPUTLED_LINE("half",     "8", "Half-speed (8-phase)", LOGD_BLUELED,  NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BIT_base + 0),
            SUBELEM_END("notitle,noshadow,norowtitles", "horz=right"),
            SUBELEM_START("stop_cond", "StopCond", 9, 9)
                {"stop_cond", "Byte", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BYTE)},
                INPUTLED_LINE("accleave", "7", "Unused",   LOGD_ONOFF,  NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 7),
                INPUTLED_LINE("leavek",   "6", "Unused",   LOGD_ONOFF,  NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 6),
                INPUTLED_LINE("softk",    "S", "Sensor=1", LOGD_REDLED, NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 5),
                INPUTLED_LINE("k3",       "s", "Sensor=0", LOGD_REDLED, NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 4),
                INPUTLED_LINE("k2",       "+", "K+",       LOGD_REDLED, NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 3),
                INPUTLED_LINE("k1",       "-", "K-",       LOGD_REDLED, NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 2),
                INPUTLED_LINE("bit1",     "1", "Unused",   LOGD_ONOFF,  NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 1),
                INPUTLED_LINE("half",     "0", "Unused",   LOGD_ONOFF,  NKSHD485_BASE + NKSHD485_CHAN_STOPCOND_BIT_base + 0),
            SUBELEM_END("notitle,noshadow,norowtitles", "horz=right"),
//                    {"cfg_byte", "Cfg byte", NULL, "%3.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_CONFIG_BYTE)},
            SUBELEM_START("vel", "Velocity", 2, 2)
                {"min_vel", "[",   "/s",  "%5.0f", "withlabel",           NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_MIN_VELOCITY), .minnorm=32, .maxnorm=12000},
                {"max_vel", "-",   "]/s", "%5.0f", "withlabel,withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_MAX_VELOCITY), .minnorm=32, .maxnorm=12000},
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
            {"accel",   "Accel", "/s^2", "%5.0f", "withunits", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_ACCELERATION), .minnorm=32, .maxnorm=65535, .placement="horz=center"},
        SUBELEM_END("noshadow,nocoltitles,fold_h", NULL),
        SUBELEM_START("operate", "Operation", 2, 1)
            SUBELEM_START("flags", "Flags", 8 , 8)
                COLORLED_LINE("s_b7",  "b", "BC",          LOGD_AMBERLED, NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 7),
                COLORLED_LINE("s_b6",  "k", "Was K",       LOGD_AMBERLED, NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 6),
                COLORLED_LINE("s_b5",  "p", "Prec. speed", LOGD_AMBERLED, NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 5),
                COLORLED_LINE("k3",    "s", "Sensor",      LOGD_REDLED,   NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 4),
                COLORLED_LINE("k2",    "+", "K+",          LOGD_REDLED,   NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 3),
                COLORLED_LINE("k1",    "-", "K-",          LOGD_REDLED,   NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 2),
                COLORLED_LINE("going", "g", "Going",       LOGD_GREENLED, NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 1),
                COLORLED_LINE("ready", "R", "Ready",       LOGD_BLUELED,  NKSHD485_BASE + NKSHD485_CHAN_STATUS_base + 0),
            SUBELEM_END("notitle,noshadow,norowtitles", "horz=right"),
            SUBELEM_START("steps", "Steps", 6, 3)
                {"numsteps", NULL,  NULL, "%6.0f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_NUM_STEPS)},
                BUT_LINE("go",         "GO",          NKSHD485_BASE + NKSHD485_CHAN_GO),
                BUT_LINE("go_unaccel", "GO unaccel.", NKSHD485_BASE + NKSHD485_CHAN_GO_WO_ACCEL),
                EMPTY_CELL(),
                //{"left", "...left", NULL, "%6.0f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NKSHD485_BASE + NKSHD485_CHAN_STEPS_LEFT)},
                BUT_LINE("stop",       "STOP",             NKSHD485_BASE + NKSHD485_CHAN_STOP),
                BUT_LINE("switchoff",  "Switch OFF",       NKSHD485_BASE + NKSHD485_CHAN_SWITCHOFF),
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
        SUBELEM_END("noshadow,nocoltitles", NULL),
    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL)
SUBWIN_END    ("...", "", "")



#undef NKSHD485_BASE
#undef NKSHD485_NAME
#undef NKSHD485_IDENT
