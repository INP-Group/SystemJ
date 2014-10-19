#ifndef RACK_N
  #error The "RACK_N" macro is undefined
#endif


#define RACK_L      __CX_STRINGIZE(RACK_N)
#define RACK_SERVER "rack"RACK_L":43"

#ifndef RACK_ADC812_0_SPECIFIC_PARAMS
  #define RACK_ADC812_0_SPECIFIC_PARAMS ""
#endif
#ifndef RACK_ADC812_1_SPECIFIC_PARAMS
  #define RACK_ADC812_1_SPECIFIC_PARAMS ""
#endif


#if 1

#define RACK_SCENARIO(target, val) \
    "set modulators.rack"RACK_L".top-cd"".cont-ubs{C,D}"  ".subwin-ubs.mod."target"="__CX_STRINGIZE(val) \
    "set modulators.rack"RACK_L".bot-abef.cont-ubs{A,B,E,F}.subwin-ubs.mod."target"="__CX_STRINGIZE(val)

SUBELEM_START("rack"RACK_L, "", 3+1+1*1, 1 + (1<<24))
    SUBMENU_START("menu", "Ст. "RACK_L, 3*4, 4)
                HLABEL_CELL("УБС"),
                SCENARIO_CELL("Выкл", "", RACK_SCENARIO("ctl.power", 0)),
                SCENARIO_CELL("Вкл",  "", RACK_SCENARIO("ctl.power", 1)),
                HLABEL_CELL("БЗ2"),
                SCENARIO_CELL("Выкл", "", RACK_SCENARIO("sam.heat_2", 0)),
                SCENARIO_CELL("Вкл",  "", RACK_SCENARIO("sam.heat_2", 1)),
                HLABEL_CELL("БЗ1"),
                SCENARIO_CELL("Выкл", "", RACK_SCENARIO("sam.heat_1", 0)),
                SCENARIO_CELL("Вкл",  "", RACK_SCENARIO("sam.heat_1", 1)),
                HLABEL_CELL("БРМ"),
                SCENARIO_CELL("Выкл", "", RACK_SCENARIO("sam.udgs_n", 0)),
                SCENARIO_CELL("Вкл",  "", RACK_SCENARIO("sam.udgs_n", 1)),
    SUBMENU_END("'Управление стойкой "RACK_L"'", "", NULL),

#undef RACK_SCENARIO

#else
SUBELEM_START("rack"RACK_L, "Стойка "RACK_L, 3+, 1)
#endif

#if 1
#define ADC812_AUX_PARAMS(m1, m2, m3) \
    " foldctls  height=200" \
    " save_button subsys=\"rack" RACK_L "-" m1 m2 m3 "\"" \
    " descr0A=\"U50"   m1 "\"" \
    " descr0B=\"Iразм" m1 "\"" \
    " descr1A=\"U50"   m2 "\"" \
    " descr1B=\"Iразм" m2 "\"" \
    " descr2A=\"U50"   m3 "\"" \
    " descr2B=\"Iразм" m3 "\"" \
    " descr3A=ch3A descr3B=ch3B "//"nochan3A nochan3B"

#define ADC812_SUBWIN(lr, m1, m2, m3, SPECIFIC_PARAMS) \
    SUBWINV4_START(":"/*"subwin-812me-" __CX_STRINGIZE(lr)*/,                    \
                   "ADC812ME стойки "RACK_L" модуляторов "m1","m2","m3"", \
                   1)                                                   \
        {"adc812me-" __CX_STRINGIZE(lr), RACK_L"-"m1""m2""m3, NULL, NULL, \
         ADC812_AUX_PARAMS(m1, m2, m3) " " SPECIFIC_PARAMS,             \
         NULL, LOGT_READ, LOGD_LIU_ADC812ME_ONE, LOGK_DIRECT,           \
         RACKX_43_ADC812ME_N_BIGC_BASE+lr /*!!! A hack: we use 'color' field to pass integer (bigc_n) */, \
         PHYS_ID(RACKX_43_ADC812ME_N_BASE+lr*ADC812ME_NUM_PARAMS),      \
         .placement="horz=fill"},                                       \
    SUBWIN_END  ("812", "one,noshadow,notitle,nocoltitles,norowtitles size=tiny,resizable,compact,noclsbtn", NULL)

    SUBELEM_START("812s", NULL, 2, 2)
        ADC812_SUBWIN(0, "C", "B", "A", RACK_ADC812_0_SPECIFIC_PARAMS),
        ADC812_SUBWIN(1, "D", "E", "F", RACK_ADC812_1_SPECIFIC_PARAMS),
    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
#endif

    SUBELEM_START("top-cd", NULL, 2, 2)
        #define UBS_N 3
        #define UBS_L "C"
        #include "liu_elem_subwin_ubs.h"
        ,
        #define UBS_N 4
        #define UBS_L "D"
        #include "liu_elem_subwin_ubs.h"
    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
    
    SUBWINV4_START("dl200", "Таймер стойки "RACK_L, 1)
        {"dl200-"RACK_L, "dl200-"RACK_L, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
            #define  DL200ME_BASE     RACKX_43_DL200ME_FAST_BASE
            #define  DL200ME_IDENT    "dl200me_f"
            #define  DL200ME_LABEL    "Таймер стойки "RACK_L
            #define  DL200ME_L_1  "Запуск1 A"
            #define  DL200ME_L_2  "Запуск1 B"
            #define  DL200ME_L_3  "Запуск1 C"
            #define  DL200ME_L_4  "Запуск1 D"
            #define  DL200ME_L_5  "Запуск1 E"
            #define  DL200ME_L_6  "Запуск1 F"
            #define  DL200ME_L_7  "Триггер ABC"
            #define  DL200ME_L_8  "Предзапуск 1"
            #define  DL200ME_L_9  "Запуск2 A"
            #define  DL200ME_L_10 "Запуск2 B"
            #define  DL200ME_L_11 "Запуск2 C"
            #define  DL200ME_L_12 "Запуск2 D"
            #define  DL200ME_L_13 "Запуск2 E"
            #define  DL200ME_L_14 "Запуск2 F"
            #define  DL200ME_L_15 "Триггер DEF"
            #define  DL200ME_L_16 "Предзапуск 2"
            #define  DL200ME_EDITABLE_LABELS
            #define  DL200ME_REG_AUTO __CX_CONCATENATE(REG_R, __CX_CONCATENATE(RACK_N,_T16_FAST_AUTO))
            #define  DL200ME_REG_SECS __CX_CONCATENATE(REG_R, __CX_CONCATENATE(RACK_N,_T16_FAST_SECS))
            #define  DL200ME_REG_PREV __CX_CONCATENATE(REG_R, __CX_CONCATENATE(RACK_N,_T16_FAST_PREV))
            #define  NO_GROUPUNIT

#define ADD_ONE_DELAY(n) \
    CMD_GETP_I(DL200ME_BASE + DL200ME_CHAN_T_BASE + n), CMD_ABS, CMD_ADD
#define DL200ME_COUNT_CHAN \
    {"weirdness", NULL, NULL, NULL, "#T!\v ", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){           \
            CMD_PUSH_I(0),     \
            ADD_ONE_DELAY(0),  \
            ADD_ONE_DELAY(1),  \
            ADD_ONE_DELAY(2),  \
            ADD_ONE_DELAY(3),  \
            ADD_ONE_DELAY(4),  \
            ADD_ONE_DELAY(5),  \
            ADD_ONE_DELAY(6),  \
            ADD_ONE_DELAY(7),  \
            ADD_ONE_DELAY(8),  \
            ADD_ONE_DELAY(9),  \
            ADD_ONE_DELAY(10), \
            ADD_ONE_DELAY(11), \
            ADD_ONE_DELAY(12), \
            ADD_ONE_DELAY(13), \
            ADD_ONE_DELAY(14), \
            ADD_ONE_DELAY(15), \
            CMD_DUP,           \
            CMD_PUSH_I(0.1), CMD_IF_GT_TEST, CMD_GOTO_I("ret"), \
            CMD_WEIRD_I(1.0),         \
            CMD_LABEL_I("ret"), CMD_RET            \
        }, \
    }

            #include "elem_dl200me_fast.h"
        }
    SUBWIN_END("dl200...", "noshadow,notitle,nocoltitles,norowtitles size=normal,logc=normal", "horz=fill"),
    
    SUBELEM_START("bot-abef", NULL, 4, 2)
        #define UBS_N 2
        #define UBS_L "B"
        #include "liu_elem_subwin_ubs.h"
        ,
        #define UBS_N 5
        #define UBS_L "E"
        #include "liu_elem_subwin_ubs.h"
        ,
        #define UBS_N 1
        #define UBS_L "A"
        #include "liu_elem_subwin_ubs.h"
        ,
        #define UBS_N 6
        #define UBS_L "F"
        #include "liu_elem_subwin_ubs.h"
    SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),

SUBELEM_END("defserver="RACK_SERVER",nocoltitles,norowtitles", NULL)


#undef RACK_ADC812_0_SPECIFIC_PARAMS
#undef RACK_ADC812_1_SPECIFIC_PARAMS

#undef RACK_N

#undef RACK_L
#undef RACK_SERVER
