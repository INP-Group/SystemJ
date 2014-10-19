#ifndef SMC8_BASE
  #error The "SMC8_BASE" macro is undefined
#endif
#ifndef SMC8_NAME
  #error The "SMC8_NAME" macro is undefined
#endif
#ifndef SMC8_IDENT
  #error The "SMC8_IDENT" macro is undefined
#endif

#ifndef SMC8_COEFF
  #define SMC8_COEFF 1
#endif
#ifndef SMC8_PRC
  #define SMC8_PRC "0"
#endif


SUBWINV4_START(SMC8_IDENT "-smc8-subwin", SMC8_NAME ": SMC8", 1)
    SUBELEM_START_CRN("controls", "",
                      NULL, "# steps\v \v...\v \v-\v+\vState\vStartFrq, /s\vFinalFrq, /s\vAccel, /s^2\vCounter\v \vOut Mode\vM-\vM+\vT-\vT+",
                      17, 1)
        {"steps"    , "#",    NULL, "%8."SMC8_PRC"f", NULL,                   NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_NUM_STEPS_base    + 0)}, \
        {"go"       , "GO",   NULL, NULL,    NULL,                   NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_GO_base           + 0)}, \
        {"going"    , NULL,   NULL, NULL,    "shape=circle",         NULL, LOGT_READ,   LOGD_AMBERLED, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_GOING_base        + 0)}, \
        {"stop"     , "Stop", NULL, NULL,    NULL,                   NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_STOP_base         + 0)}, \
        {"km"       , NULL,   NULL, NULL,    "shape=circle",         NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_KM_base           + 0)}, \
        {"kp"       , NULL,   NULL, NULL,    "shape=circle",         NULL, LOGT_READ,   LOGD_REDLED,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_KP_base           + 0)}, \
        {"state"    , NULL,   NULL, NULL,    "#T"SMC8_STATE_MSTRING, NULL, LOGT_READ,   LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_STATE_base        + 0)}, \
        {"start_frq", "[",    NULL, "%5."SMC8_PRC"f", "withlabel",            NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_START_FREQ_base   + 0)}, \
        {"final_frq", "-",    "]",  "%5."SMC8_PRC"f", "withlabel,withunits",  NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_FINAL_FREQ_base   + 0)}, \
        {"accel"    , NULL,   NULL, "%5."SMC8_PRC"f", NULL,                   NULL, LOGT_WRITE1, LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_ACCELERATION_base + 0)}, \
        {"counter"  , NULL,   NULL, "%8."SMC8_PRC"f", NULL,                   NULL, LOGT_READ,   LOGD_TEXT,     LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_COUNTER_base      + 0)}, \
        {"rst_ctr"  , "=0",   NULL, NULL,    NULL,                   NULL, LOGT_WRITE1, LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_RST_COUNTER_base  + 0)}, \
        {"out_md"   , NULL,   NULL, NULL,    "#TS+/S-\vS/Dir",       NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_OUT_MD_base       + 0)}, \
        {"maskm"    , NULL,   NULL, NULL,    "#TIgn\vStop",          NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_MASKM_base        + 0)}, \
        {"maskp"    , NULL,   NULL, NULL,    "#TIgn\vStop",          NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_MASKP_base        + 0)}, \
        {"typem"    , NULL,   NULL, NULL,    "#TNorm=Cls\vNorm=Opn", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_TYPEM_base        + 0)}, \
        {"typep"    , NULL,   NULL, NULL,    "#TNorm=Cls\vNorm=Opn", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(SMC8_BASE + SMC8_CHAN_TYPEP_base        + 0)}
    SUBELEM_END("noshadow,notitle,nocoltitles", NULL)
SUBWIN_END    ("...", "", "")



#undef SMC8_BASE
#undef SMC8_NAME
#undef SMC8_IDENT

#undef SMC8_COEFF
#undef SMC8_PRC
