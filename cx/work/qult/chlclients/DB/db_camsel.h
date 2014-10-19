enum
{
    CAMSEL_WAIT_TIME   = 40, // 40s to wait while stepping motor moves
    CAMSEL_REG_ENDWAIT = 0
};

enum
{
    C_VIDEO1 = 0,  V_VIDEO1 = 0,
    C_VIDEO2 = 6,  V_VIDEO2 = 1,
    C_VIDEO3 = 7,  V_VIDEO3 = 1,
    C_VIDEO4 = 12, V_VIDEO4 = 1,
    //C_VIDEO5 = 7,  V_VIDEO5 = 1,
    C_CONE   = 10, V_CONE   = 1,
    C_FARCUP = 9,  V_FARCUP = 1,
};

enum
{
    C_SENSOR1 = 1,  REG_SENSOR1 = 1,
    C_SENSOR2 = 2,  REG_SENSOR2 = 2,
    C_SENSOR3 = 3,  REG_SENSOR3 = 3,
    C_SENSOR4 = 4,  REG_SENSOR4 = 4,
    C_SENSOR5 = 11, REG_SENSOR5 = 5,
    C_SENSOR6 = 5,  REG_SENSOR6 = 6,
    C_SENSOR7 = 8,  REG_SENSOR7 = 7
};

#define GET_TIME_LEFT(reg) \
    CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(reg),             /* Init reg if required */      \
    CMD_PUSH_I(0), CMD_PUSH_I(0),                          /* Zeroes for <0 and ==0 */     \
    CMD_GETLCLREG_I(reg),                                  /* Time when wait should end */ \
    CMD_GETTIME, CMD_SUB, CMD_DUP,                         /* Real value and selector */   \
    CMD_CASE                                               /* =0 if curtime>=endtime */

#define SELECT_MAX() /* max(a,b)=(a+b)/2+|a-b|/2=(a+b+|a-b])/2 */ \
    CMD_SETREG_I(1), CMD_SETREG_I(2),           /* Store values in regs */ \
    CMD_GETREG_I(1), CMD_GETREG_I(2), CMD_ADD,          /* a+b  */         \
    CMD_GETREG_I(1), CMD_GETREG_I(2), CMD_SUB, CMD_ABS, /* |a-b|*/         \
    CMD_ADD, CMD_DIV_I(2)

#define BOOLEANIZE(GET_VAL_CMDS, TRUE_VAL)                     \
    CMD_PUSH_I(TRUE_VAL), CMD_PUSH_I(0), CMD_PUSH_I(TRUE_VAL), \
    GET_VAL_CMDS, CMD_CASE

#define GET_DISABLE(reg)                                              \
    CMD_PUSH_I(0), CMD_PUSH_I(0), CMD_PUSH_I(CX_VALUE_DISABLED_MASK), \
    GET_TIME_LEFT(reg), CMD_CASE

#define CAM_CHAN(chan, on_v, ident, label, comment) \
    {ident, label, NULL, NULL, "", comment, LOGT_WRITE1, LOGD_RADIOBTN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){BOOLEANIZE(CMD_GETP_I(chan), 1),              \
                    CMD_ADD_I(1-on_v), CMD_MOD_I(2),              \
                    GET_DISABLE(CAMSEL_REG_ENDWAIT),              \
                    CMD_ADD, CMD_RET},                            \
        (excmd_t[]){CMD_PUSH_I(1-V_VIDEO1), CMD_SETP_I(C_VIDEO1), \
                    CMD_PUSH_I(1-V_CONE),   CMD_SETP_I(C_CONE),   \
                    CMD_PUSH_I(1-V_VIDEO2), CMD_SETP_I(C_VIDEO2), \
                    CMD_PUSH_I(1-V_VIDEO3), CMD_SETP_I(C_VIDEO3), \
                    CMD_PUSH_I(1-V_VIDEO4), CMD_SETP_I(C_VIDEO4), \
                    CMD_PUSH_I(1-V_FARCUP), CMD_SETP_I(C_FARCUP), \
                    CMD_QRYVAL,                                   \
                    CMD_ADD_I(1-on_v), CMD_MOD_I(2),              \
                    CMD_SETP_I(chan),                             \
                    CMD_GETTIME, CMD_ADD_I(CAMSEL_WAIT_TIME),     \
                    CMD_SETLCLREG_I(CAMSEL_REG_ENDWAIT), CMD_REFRESH_I(1), \
                    CMD_RET}                                      \
    }

#define PHN_CHAN(chan, on_v, ident, label, comment) \
    {ident, label, NULL, NULL, "color=red", comment, LOGT_WRITE1, LOGD_RADIOBTN, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_PUSH_I(0), CMD_PUSH_I(1), CMD_PUSH_I(0),  \
                    CMD_GETP_I(C_VIDEO1), CMD_ADD_I(1-V_VIDEO1), CMD_MOD_I(2), \
                    CMD_GETP_I(C_VIDEO2), CMD_ADD_I(1-V_VIDEO2), CMD_MOD_I(2), CMD_ADD, \
                    CMD_GETP_I(C_VIDEO3), CMD_ADD_I(1-V_VIDEO3), CMD_MOD_I(2), CMD_ADD, \
                    CMD_GETP_I(C_VIDEO4), CMD_ADD_I(1-V_VIDEO4), CMD_MOD_I(2), CMD_ADD, \
                    CMD_GETP_I(C_CONE),   CMD_ADD_I(1-V_CONE),   CMD_MOD_I(2), CMD_ADD, \
                    CMD_GETP_I(C_FARCUP), CMD_ADD_I(1-V_FARCUP), CMD_MOD_I(2), CMD_ADD, \
                    CMD_CASE, CMD_RET},                           \
        (excmd_t[]){CMD_PUSH_I(1-V_VIDEO1), CMD_SETP_I(C_VIDEO1), \
                    CMD_PUSH_I(1-V_CONE),   CMD_SETP_I(C_CONE),   \
                    CMD_PUSH_I(1-V_VIDEO2), CMD_SETP_I(C_VIDEO2), \
                    CMD_PUSH_I(1-V_VIDEO3), CMD_SETP_I(C_VIDEO3), \
                    CMD_PUSH_I(1-V_VIDEO4), CMD_SETP_I(C_VIDEO4), \
                    CMD_PUSH_I(1-V_FARCUP), CMD_SETP_I(C_FARCUP), \
                    CMD_GETTIME, CMD_ADD_I(CAMSEL_WAIT_TIME),     \
                    CMD_SETLCLREG_I(CAMSEL_REG_ENDWAIT), CMD_REFRESH_I(1), \
                    CMD_RET}                                      \
    }

#define SNS_CHAN(chan, reg, ident, label, comment) \
    {ident, label, NULL, NULL, "", comment, LOGT_WRITE1, LOGD_GREENLED, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){BOOLEANIZE(CMD_GETP_I(chan), 1),            \
                    CMD_ADD_I(1), CMD_MOD_I(2),                 \
                    GET_DISABLE(reg),                           \
                    CMD_ADD, CMD_RET},                          \
        (excmd_t[]){CMD_QRYVAL,                                 \
                    CMD_ADD_I(1), CMD_MOD_I(2),                 \
                    CMD_SETP_I(chan),                           \
                    CMD_GETTIME, CMD_ADD_I(CAMSEL_WAIT_TIME),   \
                    CMD_SETLCLREG_I(reg), CMD_REFRESH_I(1),     \
                    CMD_RET}                                    \
    }

#define TIM_CHAN(reg) \
    {"-progress", NULL, "с", "%-2.0f", "thermometer,size=50", "", LOGT_READ, LOGD_HSLIDER, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t []){GET_TIME_LEFT(reg), CMD_RET}, \
        .mindisp=0, .maxdisp=CAMSEL_WAIT_TIME}

static groupunit_t camsel_grouping[] =
{
    {
        &(elemnet_t){
            "Camerae", "Камеры",
            "", "",
            ELEM_SINGLECOL, 9, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=right"},
                CAM_CHAN(C_VIDEO1, V_VIDEO1, "Video1", "Камера 1",   "->Люминофор в кольцах"),
                CAM_CHAN(C_CONE,   V_CONE,   "Cone",   "Конус",      "->Кварцевый конус в техпромежутке"),
                CAM_CHAN(C_VIDEO2, V_VIDEO2, "Video2", "Камера 2",   "->Люминофор в повороте"),
                CAM_CHAN(C_VIDEO3, V_VIDEO3, "Video3", "Камера 3",   "->Синхротронное излучение"),
                CAM_CHAN(C_VIDEO4, V_VIDEO4, "Video4", "Камера 4",   "->Люминофор после 10-й секции"),
                //CAM_CHAN(C_VIDEO5, V_VIDEO5, "Video5", "Камера 5",   ""),
                CAM_CHAN(C_FARCUP, V_FARCUP, "Farcup", "Ц. Фарадея", "Цилиндр Фарадея перед конвертером"),
                PHN_CHAN(-1,       -1,       "-Conv",  "Камера Ф",   "->Конвертер\n(включается по умолчанию,\nкогда остальные выведены)"),
                TIM_CHAN(CAMSEL_REG_ENDWAIT),
            },
            "nocoltitles,norowtitles"
        }, 1
    },
    {
        &(elemnet_t){
            "Sensors", "Датчики",
            "", "",
            ELEM_SINGLECOL, 9, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP, .placement="horz=left"},
                SNS_CHAN(C_SENSOR1, REG_SENSOR1, "Sensor1", "Датчик 1", "Перед спектрометром"),
                SNS_CHAN(C_SENSOR2, REG_SENSOR2, "Sensor2", "Датчик 2", "Сразу после пучкового датчика"),
                SNS_CHAN(C_SENSOR3, REG_SENSOR3, "Sensor3", "Датчик 3", "После 5й секции"),
                SNS_CHAN(C_SENSOR4, REG_SENSOR4, "Sensor4", "Датчик 4", "Второй датчик в техническом промежутке"),
                SNS_CHAN(C_SENSOR5, REG_SENSOR5, "Sensor5", "Датчик 5", "Третий датчик в техпромежутке"),
                SNS_CHAN(C_SENSOR6, REG_SENSOR6, "Sensor6", "Датчик 6", "Перед поворотом"),
                SNS_CHAN(C_SENSOR7, REG_SENSOR7, "Sensor7", "Датчик 7", "В триплете после поворота"),
                {"-progress", NULL, "с", "%-2.0f", "thermometer,size=50", "", LOGT_READ, LOGD_HSLIDER, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
                    (excmd_t []){GET_TIME_LEFT(REG_SENSOR1),
                                 GET_TIME_LEFT(REG_SENSOR2), SELECT_MAX(),
                                 GET_TIME_LEFT(REG_SENSOR3), SELECT_MAX(),
                                 GET_TIME_LEFT(REG_SENSOR4), SELECT_MAX(),
                                 GET_TIME_LEFT(REG_SENSOR5), SELECT_MAX(),
                                 GET_TIME_LEFT(REG_SENSOR6), SELECT_MAX(),
                                 GET_TIME_LEFT(REG_SENSOR7), SELECT_MAX(),
                                 CMD_RET},
                    .mindisp=0, .maxdisp=CAMSEL_WAIT_TIME}
            },
            "nocoltitles,norowtitles"
        }
    },
    {NULL}
};

static physprops_t camsel_physinfo[] =
{
    {0, 1.0, 0},
};


#if 0
#define WAIT_TIME 40

#define REG_TIMER_CAM 1
#define REG_TIMER_SNS 2

#define BASE_CAM 0
#define BASE_SNS 8

#define GET_TIME(reg) \
    CMD_PUSH_I(0), CMD_SETLCLREGDEFVAL_I(reg),             /* Init reg if required */      \
    CMD_PUSH_I(0), CMD_PUSH_I(0),                          /* Zeroes for <0 and ==0 */     \
    CMD_GETLCLREG_I(reg), CMD_ADD_I(WAIT_TIME),            /* Time when wait should end */ \
    CMD_GETTIME, CMD_SUB, CMD_DUP,                         /* Real value and selector */   \
    CMD_CASE

#define BOOLEANIZE(VAL, TRUE_VAL) \
    CMD_PUSH_I(TRUE_VAL), CMD_PUSH_I(0), CMD_PUSH_I(TRUE_VAL), \
    VAL, CMD_CASE

#define GET_DISABLE(reg)                         \
    CMD_PUSH_I(0), CMD_PUSH_I(0), CMD_PUSH_I(2), \
    GET_TIME(reg), CMD_CASE

#define CAM_CHAN(chan, ident, label, comment) \
    {ident, label, NULL, NULL, "", comment, LOGT_WRITE1, LOGD_GREENLED, LOGK_CALCED, LOGC_NORMAL, -1, \
    (excmd_t []){BOOLEANIZE(CMD_GETP_I(BASE_CAM+chan), 1), \
                 GET_DISABLE(REG_TIMER_CAM), \
                 CMD_ADD, CMD_RET}, \
    (excmd_t []){CMD_PUSH_I(0), CMD_SETP_I(BASE_CAM+0), \
                 CMD_PUSH_I(0), CMD_SETP_I(BASE_CAM+1), \
                 CMD_PUSH_I(0), CMD_SETP_I(BASE_CAM+2), \
                 CMD_PUSH_I(0), CMD_SETP_I(BASE_CAM+3), \
                 CMD_PUSH_I(0), CMD_SETP_I(BASE_CAM+4), \
                 CMD_QRYVAL, CMD_SETP_I(BASE_CAM+chan), \
                 CMD_GETTIME, CMD_SETLCLREG_I(REG_TIMER_CAM), CMD_REFRESH_I(1), \
                 CMD_RET}}

#define SNS_CHAN(chan, ident, label, comment) \
    {ident, label, NULL, NULL, "", comment, LOGT_WRITE1, LOGD_BLUELED,  LOGK_CALCED, LOGC_NORMAL, -1, \
    (excmd_t []){BOOLEANIZE(CMD_GETP_I(BASE_SNS+chan), 1), \
                 GET_DISABLE(REG_TIMER_SNS), \
                 CMD_ADD, CMD_RET}, \
    (excmd_t []){CMD_QRYVAL, CMD_SETP_I(BASE_SNS+chan), \
                 CMD_GETTIME, CMD_SETLCLREG_I(REG_TIMER_SNS), CMD_REFRESH_I(1), \
                 CMD_RET}}

#define TIM_CHAN(reg) \
    {"progress", NULL, "с", "%2.0f", "thermometer,size=50", "", LOGT_READ, LOGD_HSLIDER, LOGK_CALCED, LOGC_NORMAL, -1, \
    (excmd_t []){GET_TIME(reg), CMD_RET}, \
    NULL, 0, 0, 0, 0, 0, 0, 0, WAIT_TIME}

#define SBT_CHAN(v) \
    {"set" __CX_STRINGIZE(v), v? "+Все":"-Все", NULL, NULL, NULL, v?"Ввести все датчики":"Вывести все датчики", LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, -1, \
    (excmd_t []){CMD_RET_I(0)}, \
    (excmd_t []){CMD_RET}}

static groupunit_t camsel_grouping[] =
{
    {
        &(elemnet_t){
            "Camerae", "Камеры",
            "", "",
            ELEM_SINGLECOL, 8, 1,
            (logchannet_t []){
                {"-leds", NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_SRV_LEDS, LOGK_NOP},
                CAM_CHAN(0, "Video1", "Камера 1", ""),
                CAM_CHAN(1, "Video2", "Камера 2", ""),
                CAM_CHAN(2, "Video3", "Камера 3", ""),
                CAM_CHAN(3, "Video4", "Камера 4", ""),
                CAM_CHAN(4, "Video5", "Камера 5", ""),
                CAM_CHAN(5, "Cone",   "Конус",    ""),
                TIM_CHAN(REG_TIMER_CAM),
            },
            "nocoltitles,norowtitles"
        }, 1
    },
#if 0
    {
        &(elemnet_t){
            "Sensors", "Датчики",
            "", "",
            ELEM_SINGLECOL, 8, 1,
            (logchannet_t []){
                SNS_CHAN(0, "Sensor1", "Датчик 1", ""),
                SNS_CHAN(1, "Sensor2", "Датчик 2", ""),
                SNS_CHAN(2, "Sensor3", "Датчик 3", ""),
                SNS_CHAN(3, "Sensor4", "Датчик 4", ""),
                SNS_CHAN(4, "Sensor5", "Датчик 5", ""),
                SNS_CHAN(5, "Sensor6", "Датчик 6", ""),
                TIM_CHAN(REG_TIMER_SNS),
                {".all_sensors", NULL, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, -1,
                    (void *)&(elemnet_t){
                        ".all_sensors", "Усё",
                        "", "",
                        ELEM_MULTICOL, 2, 2,
                        (logchannet_t []){
                            SBT_CHAN(1),
                            SBT_CHAN(0),
                            {"zz", "!", NULL, NULL, NULL, NULL, LOGT_WRITE1, LOGD_ALARM, LOGK_DIRECT, LOGC_NORMAL, 0}
                        },
                        "notitle,noshadow,nocoltitles,norowtitles"
                    }
                },
            },
            "nocoltitles,norowtitles"
        }
    },
#endif
    {NULL}
};

static physprops_t camsel_physinfo[] =
{
    {0, 1.0, 0},
};
#endif
