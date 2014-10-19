#define S_C 0

enum {chn = 0, r0 = 0};


static excmd_t w_proggie[] =
{
    CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,
//    CMD_DEBUGP_I("Kukareku!\n"),
    CMD_PUSH_I(1), CMD_PRGLY, CMD_SETP_I(chn),
//    CMD_REFRESH_I(1.0), CMD_RET,
    
    CMD_GETTIME, CMD_SETLCLREG_I(r0), CMD_REFRESH_I(1.0), CMD_RET
};

static excmd_t r_proggie[] =
{
    /* Is r0!=0? */
    CMD_PUSH_I(0.0), CMD_SETLCLREGDEFVAL_I(r0),
    CMD_PUSH_I(1.0), CMD_PUSH_I(1.0), CMD_PUSH_I(0.0), CMD_GETLCLREG_I(r0), CMD_CASE,
    CMD_TEST, CMD_BREAK_I(0.0),

    CMD_PUSH_I(0.0), CMD_PUSH_I(0.0), CMD_PUSH_I(1.0),
    CMD_GETLCLREG_I(r0), CMD_ADD_I(4.0), CMD_GETTIME, CMD_SUB, //
    CMD_CASE,
    CMD_TEST, CMD_BREAK_I(CX_VALUE_DISABLED_MASK),
    
    CMD_PUSH_I(0), CMD_ALLOW_W, CMD_PRGLY, CMD_SETP_I(chn),
    CMD_PUSH_I(0), CMD_SETLCLREG_I(r0),
    CMD_RET_I(0.0)
};

#define VAC_LINE(line) \
    {"Imes"   #line, "Iизм" #line,        "",  "%.10f"/*NULL*/, "groupable", "Mama!!1 Papa!!!", LOGT_WRITE1,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL*0+LOGC_IMPORTANT, PHYS_ID((68+line)*0+1*line*7+0), NULL, NULL, 0.0, 0.1*0, 0.0, 0.0, 2.0*0}, \
    {"Umes"   #line, "Uизм" #line,        "",  NULL, NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_HILITED, PHYS_ID(line*7+1*0), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 2.0}, \
    /*{"Ilim"   #line, "Предел I" #line,    "ZZ",  "%.1f", "size=100", "У попа\nбыла собака", LOGT_WRITE1, (line&1?LOGD_DIAL:LOGD_KNOB)*1+LOGD_HSLIDER*0+(line&1?LOGD_HLABEL:LOGD_VLABEL)*0,   LOGK_DIRECT, LOGC_IMPORTANT, PHYS_ID(line*7+2*0), NULL, NULL, 0.0, 3.0, 0.0, -1.0, 4.0, 0.5, -2.0, +5.0},*/ \
    {"Ilim"   #line, "K" #line,    "",  "%4.1f", "size=100,value,compact", "У попа\nбыла собака", LOGT_WRITE1, (line&1?LOGD_DIAL:LOGD_KNOB)*0+LOGD_HSLIDER*1+(line&1?LOGD_HLABEL:LOGD_VLABEL)*0,   LOGK_DIRECT, LOGC_IMPORTANT, PHYS_ID(line*7+2*0), NULL, NULL, 0.0, 3.0, 0.0, -1.0, 4.0, 0.5, -2.0, +5.0}, \
    {"Ulim"   #line, "???",               "",  NULL, "#C#LMama!\v#F25f,6.5,3.1,Zhao='%.1f'", "А роза упала на лапу азора", LOGT_WRITE1*1+LOGT_READ*0, LOGD_SELECTOR*0+LOGD_CHOICEBS,   LOGK_CALCED, LOGC_NORMAL*0+LOGC_VIC, PHYS_ID(line*0+0), \
    (excmd_t []){CMD_GETP_I(0),CMD_RET}, \
    (excmd_t []){CMD_QRYVAL,CMD_SETP_I(0),CMD_QRYVAL,CMD_DIV_I(3.0),CMD_SETP_I(line*7),CMD_RET_I(0.0)}, 0.0, 0.1, 0.0, 0.0, 2.0}, \
    {"IlimHW" #line, "ПределHW" #line,    "",  NULL, NULL, NULL, LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(line*7+4)}, \
    /*{"IlimHW" #line, " z ",                "",  NULL, NULL, NULL, LOGT_READ,   LOGD_LIGHT*0+LOGD_BUTTON,   LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(line*7+4*0), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 2.0},*/ \
    {"UlimHW" #line, /*" z "*/"=#!=btn_histplot",                "",  NULL, NULL, NULL, LOGT_WRITE1,   LOGD_LIGHT*0+LOGD_BUTTON,   LOGK_CALCED, LOGC_NORMAL, PHYS_ID(line*7+4*0), r_proggie, w_proggie, 0.0, 0.1, 0.0, 0.0, 2.0}, \
    {"Ialm"   #line, "01",                "",  NULL, "color=green,bg=amber", NULL, LOGT_WRITE1*1+LOGT_READ*0,   (LOGD_ONOFF)*0+(LOGD_ARROW_LT+(line&3))*1,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(line*7+5*0), NULL, NULL, 0.0, 0.1, 0.0, 0.0, 2.0}, \
    {"Ualm"   #line, "02",                "",  NULL, line&1?"panel" : "color=blue", NULL, LOGT_READ*1+LOGT_WRITE1*0,   LOGD_ALARM*1+LOGD_ONOFF*0+LOGD_TEXT*0,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(line*7+6*0)} \
    /*{"Ualm"   #line, "Падающая",                "",  NULL, line&1?"panel" : "color=blue", NULL, LOGT_READ*0+LOGT_WRITE1,   LOGD_RADIOBTN*1+LOGD_ONOFF*0+LOGD_TEXT*0,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(line*7+6*0), .placement="vert=bottom,horz=right"}*/

static logchannet_t widgettest_chans [] =
{
    VAC_LINE(0),
    VAC_LINE(1),
    VAC_LINE(2),
    VAC_LINE(3),
};

static elemnet_t widgettest_elem = {
    "WidgetTest", "Разные виджеты",
    "I (A)\vU (?)\vПредел I\f?\vПредел U\fА у меня ж опыта больше!\vIlimHW\v!!!\vI!\vU!",
    "1\fBaba!Yaga!\v2\f?\v3\v4",
    ELEM_MULTICOL, 4*8*0+8, 8+(2<<24), widgettest_chans,
    "defserver=:45,fold_v,sepfold,titleatbottom"
};

static groupunit_t widgettest_grouping[] =
{
    {&widgettest_elem},

    {
        &(elemnet_t){
            "subwin", "SUBWIN",
            "", "",
            ELEM_SUBWIN, 3, 1,
            (logchannet_t [])
            {
                {"set", "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
                {"mes", "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_TEXT,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .minnorm=0, .maxnorm=10, .minyelw=0, .maxyelw=20},
                {"alm", "", NULL, NULL, "", NULL, LOGT_READ,   LOGD_ALARM, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
            },
            "norowtitles logc=vic"
        }
    },

    {
        &(elemnet_t){
            "submenu", "v",
            "", "",
            ELEM_SUBMENU, 2, 2,
            (logchannet_t [])
            {
                {"alm", "Zzz",    NULL, NULL, "", NULL, LOGT_READ,   LOGD_ALARM,  LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
                {"btn", "Button", NULL, NULL, "", NULL, LOGT_READ,   LOGD_BUTTON, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
            },
            "label=A label=SubMenu"
        }
    },

    {
        &(elemnet_t){
            "recaller", "Recaller",
            "", "",
            ELEM_MULTICOL, 3, 1,
            (logchannet_t [])
            {
#define R_VAL 90
#define R_GRD 91
#define FNAME "testreg.val"
                {"set", "", NULL, NULL, "", NULL, LOGT_WRITE1, LOGD_TEXT,  LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){
                        CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(R_GRD),
                        CMD_GETLCLREG_I(R_GRD), CMD_PUSH_I(0),
                        CMD_IF_GT_TEST, CMD_GOTO_I("ret"),
                        
                        CMD_PUSH_I(1), CMD_SETLCLREG_I(R_GRD),
                        CMD_PUSH_I(123), CMD_LOADVAL_I(FNAME),
                        CMD_SETLCLREG_I(R_VAL),
                        
                        CMD_LABEL_I("ret"), CMD_GETLCLREG_I(R_VAL), CMD_RET
                    },
                    (excmd_t []){
                        CMD_QRYVAL, CMD_SETLCLREG_I(R_VAL),
                        CMD_QRYVAL, CMD_SAVEVAL_I(FNAME),
                        CMD_RET
                    },
                },
                {
                    "script", "Script", NULL, NULL,
                    "stop,size=large,bg=green,color=red"
                    " scenario=echo 1234567890\n"
                    " set recaller.tf=234 "
                    "sleep 2 "
                    "echo 0987654321",
                    NULL, LOGT_READ, LOGD_SCENARIO, LOGK_NOP, LOGC_NORMAL
                },
                {"tf", "TextField", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
            },
            "nocoltitles,norowtitles"
        }
    },

    {NULL}
};

static physprops_t widgettest_physinfo[] =
{
    {0+S_C,   1, 0},
    {68+S_C,  (0x7fff/10.0)*(8.0/300), 0, 1},
    {69+S_C,  (0x7fff/10.0)*(8.0/300), 0, 0},
};

#undef S_C
#undef VAC_LINE
