#ifndef BPM_ID
	#error The "BPM_ID" macro is undefined
#endif

#define BPM_LB(bpm_id) ( \
    (bpm_id == 1  ? "BPM 01" : \
     bpm_id == 2  ? "BPM 02" : \
     bpm_id == 3  ? "BPM 03" : \
     bpm_id == 4  ? "BPM 04" : \
     bpm_id == 5  ? "BPM 05" : \
     bpm_id == 6  ? "BPM 07" : \
     bpm_id == 7  ? "BPM 08" : \
     bpm_id == 8  ? "BPM 09" : \
     bpm_id == 9  ? "BPM 10" : \
     bpm_id == 10 ? "BPM 11" : \
     bpm_id == 11 ? "BPM 12" : \
     bpm_id == 12 ? "BPM 13" : \
     bpm_id == 13 ? "BPM 14" : \
     bpm_id == 14 ? "BPM 15" : \
     bpm_id == 15 ? "BPM 16" : \
     bpm_id == 16 ? "BPM 17" : \
     "BPM ??"))

SUBELEM_START("bpmd"__CX_STRINGIZE(BPM_ID), BPM_LB(BPM_ID), 1 + 1, (1 << 24) + 1)

    {
     "state", NULL, NULL, NULL,
     "#-T\v"
     "Unknown     \f\flit=red\v"
     "Determinate \f\flit=yellow\v"
     "AutomaticRun\f\flit=green\v"
     "Electron Run\f\flit=green\v"
     "Electron Inj\f\flit=green\v"
     "Electron Dls\f\flit=green\v"
     "Positron Run\f\flit=green\v"
     "Positron Inj\f\flit=green\v"
     "Positron Dls\f\flit=green\v"
     "Free Running\f\flit=green\v"
#if 0
     "ELCTRUN_SET_ON\f\f\v"
     "ELCTRUN_STOP_PREV\f\f\v"
     "ELCTRUN_WR_PARAMS\f\f\v"
     "ELCTRUN_STRT_NEXT\f\f\v"

     "ELCTINJ_SET_ON\f\f\v"
     "ELCTINJ_STOP_PREV\f\f\v"
     "ELCTINJ_WR_PARAMS\f\f\v"
     "ELCTINJ_STRT_NEXT\f\f\v"

     "PSTRRUN_SET_ON\f\f\v"
     "PSTRRUN_STOP_PREV\f\f\v"
     "PSTRRUN_WR_PARAMS\f\f\v"
     "PSTRRUN_STRT_NEXT\f\f\v"

     "PSTRINJ_SET_ON\f\f\v"
     "PSTRINJ_STOP_PREV\f\f\v"
     "PSTRINJ_WR_PARAMS\f\f\v"
     "PSTRINJ_STRT_NEXT\f\f\v"
#endif
     ""
     , "show current BPM state", LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(STATE_CHAN(BPM_ID))
    },

    // RGSWCH_LINE("onoff", NULL, "Off", "On", IS_ON_CHAN(BPM_ID), SWITCH_OFF_CHAN(BPM_ID), SWITCH_ON_CHAN(BPM_ID)),

    SUBELEM_START("bpm", NULL, 4, 1)
        /* Parameters Settings */
        SUBELEM_START("params", NULL, 3, 3)
	    SUBELEM_START("common", NULL, 1, 1)
		SUBELEM_START("set", "Settings", 6 + 1, 1 + (1 << 24))
                    {"is_ingroup", "", NULL, NULL, "", NULL,
                        LOGT_WRITE1, LOGD_ONOFF, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                        (excmd_t[]){
                            CMD_PUSH_I(1), CMD_SETLCLREGDEFVAL_I(IS_INGROUP_REGS_base + (BPM_ID)),
                            CMD_GETLCLREG_I(IS_INGROUP_REGS_base + (BPM_ID)),
                            CMD_RET
                        },
                        (excmd_t[]){
                            CMD_QRYVAL,
                            CMD_SETLCLREG_I(IS_INGROUP_REGS_base + (BPM_ID)),
                            CMD_RET
                        }},
		    {"numpts", "NumPts", "#",  "%5.0f", NULL, "tbt data array length", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(NUMPTS_CHAN(BPM_ID))},
		    {"ptsofs", "PtsOfs", "#",  "%5.0f", NULL, "tbt data array offset", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(PTSOFS_CHAN(BPM_ID))},
		    {"decmtn", "Decmtn", "#",  "%5.0f", NULL, "tbt data decimation",   LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DECMTN_CHAN(BPM_ID))},
		    {"delay",  "Delay",  "ns", "%5.3f", NULL, "delay",                 LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(DELAY_CHAN(BPM_ID)) },
		    {"atten",  "Atten",  "-",  "%5.0f", NULL, "attenuation",           LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ATTEN_CHAN(BPM_ID)) },
                    {"stat",   NULL,     "",   NULL,    "#T \vOVERLOAD\f\flit=red", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(STATUS_CHAN(BPM_ID))},
		SUBELEM_END("noshadow,nocoltitles", NULL),
		SUBELEM_START(NULL, NULL, 1, 1)
		{"frld", " Reload Params ", NULL, NULL, NULL, "force write parameters into electronics", LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(FORCE_RELOAD(BPM_ID)), .placement="horz=fill"},
		SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
	    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
	    VSEP_L(),
	    SUBELEM_START("opr", "Regime", 6, 1)
		{"auto", "AUTO",  NULL, NULL, NULL, "listen commands from automatica", LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(REGIME_AUTOMAT(BPM_ID)), .placement="horz=fill"},
		{"erun", "E_RUN", NULL, NULL, NULL, "electron orbit measurements",     LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(REGIME_ELCTRUN(BPM_ID)), .placement="horz=fill"},
		{"einj", "E_INJ", NULL, NULL, NULL, "electron injection measurements", LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(REGIME_ELCTINJ(BPM_ID)), .placement="horz=fill"},
		{"prun", "P_RUN", NULL, NULL, NULL, "positron orbit measurements",     LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(REGIME_PSTRRUN(BPM_ID)), .placement="horz=fill"},
		{"pinj", "P_INJ", NULL, NULL, NULL, "positron injection measurements", LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(REGIME_PSTRINJ(BPM_ID)), .placement="horz=fill"},
		{"free", "FREE",  NULL, NULL, NULL, "free running",                    LOGT_WRITE1, LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(REGIME_FREERUN(BPM_ID)), .placement="horz=fill"},
	    SUBELEM_END("noshadow,nocoltitles,norowtitles", NULL),
	SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),

	SEP_L(),

	/* Idividual settings */
	SUBWINV4_START("indiv", "BPM#"__CX_STRINGIZE(BPM_ID)" Individual Settings", 1)
	    SUBELEM_START("set", NULL, 3, 3)
		SUBELEM_START("izer", "Zero", 4, 1)
		    {"iz1", "Z1", "#",  "%+5.0f", NULL, "individual zero shift of line1", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IZERO_CHAN(BPM_ID, 1))},
		    {"iz2", "Z2", "#",  "%+5.0f", NULL, "individual zero shift of line2", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IZERO_CHAN(BPM_ID, 2))},
		    {"iz3", "Z3", "#",  "%+5.0f", NULL, "individual zero shift of line3", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IZERO_CHAN(BPM_ID, 3))},
		    {"iz4", "Z4", "#",  "%+5.0f", NULL, "individual zero shift of line4", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IZERO_CHAN(BPM_ID, 4))},
		SUBELEM_END("noshadow,nocoltitles", NULL),
		VSEP_L(),
		SUBELEM_START("idel", "Delay", 4, 1)
		    {"id1", "D1", "#",  "%5.0f",  NULL, "individual    delay   of line1", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IDLAY_CHAN(BPM_ID, 1))},
		    {"id2", "D2", "#",  "%5.0f",  NULL, "individual    delay   of line2", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IDLAY_CHAN(BPM_ID, 2))},
		    {"id3", "D3", "#",  "%5.0f",  NULL, "individual    delay   of line3", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IDLAY_CHAN(BPM_ID, 3))},
		    {"id4", "D4", "#",  "%5.0f",  NULL, "individual    delay   of line4", LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(IDLAY_CHAN(BPM_ID, 4))},
		SUBELEM_END("noshadow,nocoltitles", NULL),
	    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
        SUBWIN_END("Individual Settings...", "noshadow,notitle,compact", NULL),

	SEP_L(),

	/* Slow Readouts */
	SUBELEM_START(NULL, NULL, 0, 3)
	    SUBELEM_START(NULL, NULL, 2, 1)
	        {"xmean", "<X>:", "",  "%6.4f", NULL, "X  mean  value", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XMEAN(BPM_ID))  },
		{"zmean", "<Z>:", "",  "%6.4f", NULL, "Z  mean  value", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ZMEAN(BPM_ID))  },
	    SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
	    VSEP_L(),
	    SUBELEM_START(NULL, NULL, 2, 1)
	        {"xstdv", " sX:", "",  "%6.4f", NULL, "X stddev value", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(XSTDDEV(BPM_ID))},
	        {"zstdv", " sZ:", "",  "%6.4f", NULL, "Z stddev value", LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ZSTDDEV(BPM_ID))},
	    SUBELEM_END("notitle,noshadow,nocoltitles", NULL),
	SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),

    SUBELEM_END("notitle,noshadow,nocoltitles,norowtitles", NULL),
SUBELEM_END("nocoltitles,norowtitles", NULL)


#undef BPM_ID
