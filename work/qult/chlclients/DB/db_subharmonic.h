#define S_C 0

#define ADC_BASE 0
#define DAC_BASE 58
#define ADC_UR   (ADC_BASE+40)
#define DAC_UR   (DAC_BASE+16)

#define URRN(subsys, c) (S_C + (subsys==0?DAC_UR:ADC_UR) + c)


#define U2D_S(subsys, c, m) CMD_GETP_I(URRN(subsys,c)), CMD_MUL_I(m)

#define URR2DEG(subsys) (excmd_t []){   \
        U2D_S(subsys, 7, 160),          \
        U2D_S(subsys, 6, 80),  CMD_ADD, \
        U2D_S(subsys, 5, 80),  CMD_ADD, \
        U2D_S(subsys, 4, 40),  CMD_ADD, \
        U2D_S(subsys, 3, 20),  CMD_ADD, \
        U2D_S(subsys, 2, 10),  CMD_ADD, \
        U2D_S(subsys, 1, 5),   CMD_ADD, \
        U2D_S(subsys, 0, 5),   CMD_ADD, \
        CMD_RET                         \
    }


/* r1=(r0>=m)?m:0; r0-=r1; c:=r1/m; */
#define D2U_S(subsys, c, m)                                     \
    /* r1=(r0>=m)?m:0 */                                        \
    CMD_PUSH_I(0), CMD_DUP_I(m), CMD_GETREG_I(0), CMD_SUB_I(m), \
    CMD_CASE, CMD_SETREG_I(1),                                  \
    /* r0-=r1 */                                                \
    CMD_GETREG_I(0), CMD_GETREG_I(1), CMD_SUB, CMD_SETREG_I(0), \
    /* c:=r1/m */                                               \
    CMD_GETREG_I(1), CMD_DIV_I(m), CMD_SETP_I(URRN(subsys,c))

#define DEG2URR(subsys) (excmd_t []){  \
        /* r0=val%360 */               \
        CMD_QRYVAL,                    \
        CMD_MOD_I(360.0),              \
        CMD_SETREG_I(0),               \
        /* r0=val==360?360:r0*/        \
        CMD_GETREG_I(0),               \
        CMD_PUSH_I(360.0),             \
        CMD_GETREG_I(0),               \
        CMD_QRYVAL,                    \
        CMD_SUB_I(360.0),              \
        CMD_CASE,                      \
        CMD_SETREG_I(0),               \
                                       \
        /* if(r0<0) r0+=360 */         \
        CMD_GETREG_I(0),               \
        CMD_ADD_I(360.0), /* <0?  */   \
        CMD_GETREG_I(0),  /* ==0? */   \
        CMD_GETREG_I(0),  /* >0?  */   \
        CMD_GETREG_I(0),  /* sel-r */  \
        CMD_CASE,                      \
                                       \
        /* Loop... */                  \
        D2U_S(subsys, 7, 160),         \
        D2U_S(subsys, 6, 80),          \
        D2U_S(subsys, 5, 80),          \
        D2U_S(subsys, 4, 40),          \
        D2U_S(subsys, 3, 20),          \
        D2U_S(subsys, 2, 10),          \
        D2U_S(subsys, 1, 5),           \
        D2U_S(subsys, 0, 5),           \
        CMD_RET                        \
    }


#define ROUGH_CHAN(n) {"Rough"  #n, "\0Грубо "  #n, "╟", "%3.0f", "size=100,value", "Грубая настройка, по 5╟, #" #n, LOGT_WRITE1, LOGD_DIAL, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(S_C+n), URR2DEG(n), DEG2URR(n),    0.0,   360.0, 0.0, 0.0, 0.0, 5.0, 0.0, 360.0, .placement="horz=center"}
#define FINE_CHAN(n,c){"Phase"  #n, "Фаза "     #n, "",  NULL, NULL, "Тонкая настройка #"         #n, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(S_C+c), NULL, NULL,              -60.0,   +60.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}
#define FIN2_CHAN(n,c){"Urez"   #n, "U рез. "   #n, "╟", NULL, NULL, "Напряжение в резонаторе #"  #n, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(S_C+c), NULL, NULL,                0.0,   +10.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}
#define LUB1_CHAN(n,c){"Lub1"   #n, "Криво "    #n, "╟", NULL, NULL, "Кривое любопытство #"       #n, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(S_C+c), NULL, NULL,            -1000.0, +1000.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}
#define LUB2_CHAN(n,c){"Lub2"   #n, "Косо "     #n, "╟", NULL, NULL, "Косое любопытство #"        #n, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(S_C+c), NULL, NULL,            -1000.0, +1000.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}

#define CHANS2CHAN(subsys, c, m) {"LED_" #subsys "_" #c, #m, "", NULL, NULL, "LED_" #subsys "_" #c ", " #m "╟", LOGT_WRITE1, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(URRN(subsys,c))}
#define CHANS2LINE(c, m) CHANS2CHAN(0, c, m), CHANS2CHAN(1, c, m)

static logchannet_t subharmonic_chans [] =
{
    ROUGH_CHAN(0),              ROUGH_CHAN(1),
    FINE_CHAN (0, DAC_BASE+2),  FINE_CHAN (1, DAC_BASE+3),
    LUB1_CHAN (0, ADC_BASE+3),  LUB1_CHAN (1, ADC_BASE+4),
    FIN2_CHAN (0, DAC_BASE+0),  FIN2_CHAN (1, DAC_BASE+1),
    LUB2_CHAN (0, ADC_BASE+1),  LUB2_CHAN (1, ADC_BASE+2),
};

static logchannet_t subharmonic_chans2 [] =
{
    CHANS2LINE(7, 160),
    CHANS2LINE(6, 80),
    CHANS2LINE(5, 80),
    CHANS2LINE(4, 40),
    CHANS2LINE(3, 20),
    CHANS2LINE(2, 10),
    CHANS2LINE(1, 5),
    CHANS2LINE(0, 5),
};

static elemnet_t subharmonic_elem = {
    "SubHarmonic", "\0Субгармоника",
    "1\v2",
    "Грубо\vФаза\v1111\vU рез.\v2222",
    ELEM_MULTICOL, 5*2, 2, subharmonic_chans,
    "notitle,noshadow"
};

static elemnet_t subharmonic_elem2 = {
    "SubHarmonic_leds", "LEDs",
    "1\v2",
    "160\v80\v80\v40\v20\v10\v5\v5",
    ELEM_MULTICOL, 8*2, 2, subharmonic_chans2
};

static groupunit_t subharmonic_grouping[] =
{
    {&subharmonic_elem},
//    {&subharmonic_elem2, 1},
    {NULL}
};

/* static physprops_t subharmonic_physinfo[] = */
/* { */
/*     {0+S_C,   1, 0}, */
/*     {1+S_C,   1, 0} */
/* }; */
#if 1 /* marcankoz */
  #define SUBH_ADC_R (10000000/10.0)
  #define SUBH_DAC_R (10000000/10.0)
#else /* otokarev's */
  #define SUBH_ADC_R (0x3fffff/10.0)
  #define SUBH_DAC_R (0x7fff/10.0)
#endif
static physprops_t subharmonic_physinfo[] =
{
    {  0,    SUBH_ADC_R, 0},
    {  1,    SUBH_ADC_R, 0},
    {  2,    SUBH_ADC_R, 0},
    {  3,    SUBH_ADC_R, 0},
    {  4,    SUBH_ADC_R, 0},
    {  5,    SUBH_ADC_R, 0},
    {  6,    SUBH_ADC_R, 0},
    {  7,    SUBH_ADC_R, 0},
    {  8,    SUBH_ADC_R, 0},
    {  9,    SUBH_ADC_R, 0},
    { 10,    SUBH_ADC_R, 0},
    { 11,    SUBH_ADC_R, 0},
    { 12,    SUBH_ADC_R, 0},
    { 13,    SUBH_ADC_R, 0},
    { 14,    SUBH_ADC_R, 0},
    { 15,    SUBH_ADC_R, 0},
    { 16,    SUBH_ADC_R, 0},
    { 17,    SUBH_ADC_R, 0},
    { 18,    SUBH_ADC_R, 0},
    { 19,    SUBH_ADC_R, 0},
    { 20,    SUBH_ADC_R, 0},
    { 21,    SUBH_ADC_R, 0},
    { 22,    SUBH_ADC_R, 0},
    { 23,    SUBH_ADC_R, 0},
    { 24,    SUBH_ADC_R, 0},
    { 25,    SUBH_ADC_R, 0},
    { 26,    SUBH_ADC_R, 0},
    { 27,    SUBH_ADC_R, 0},
    { 28,    SUBH_ADC_R, 0},
    { 29,    SUBH_ADC_R, 0},
    { 30,    SUBH_ADC_R, 0},
    { 31,    SUBH_ADC_R, 0},
    { 32,    SUBH_ADC_R, 0},
    { 33,    SUBH_ADC_R, 0},
    { 34,    SUBH_ADC_R, 0},
    { 35,    SUBH_ADC_R, 0},
    { 36,    SUBH_ADC_R, 0},
    { 37,    SUBH_ADC_R, 0},
    { 38,    SUBH_ADC_R, 0},
    { 39,    SUBH_ADC_R, 0},
    { 40,    1, 0},
    { 41,    1, 0},
    { 42,    1, 0},
    { 43,    1, 0},
    { 44,    1, 0},
    { 45,    1, 0},
    { 46,    1, 0},
    { 47,    1, 0},
    { 48,    1, 0},
    { 49,    1, 0},
    { 50,    1, 0},
    { 51,    1, 0},
    { 52,    1, 0},
    { 53,    1, 0},
    { 54,    1, 0},
    { 55,    1, 0},
    { 56,    1, 0},
    { 57,    1, 0},
    { 58,    -SUBH_DAC_R, 0, 305},
    { 59,    -SUBH_DAC_R, 0, 305},
    { 60,    SUBH_DAC_R/20.0, 0, 305},
    { 61,    SUBH_DAC_R/20.0, 0, 305},
    { 62,    SUBH_DAC_R, 0, 305},
    { 63,    SUBH_DAC_R, 0, 305},
    { 64,    SUBH_DAC_R, 0, 305},
    { 65,    SUBH_DAC_R, 0, 305},
    { 66,    SUBH_DAC_R, 0, 305},
    { 67,    SUBH_DAC_R, 0, 305},
    { 68,    SUBH_DAC_R, 0, 305},
    { 69,    SUBH_DAC_R, 0, 305},
    { 70,    SUBH_DAC_R, 0, 305},
    { 71,    SUBH_DAC_R, 0, 305},
    { 72,    SUBH_DAC_R, 0, 305},
    { 73,    SUBH_DAC_R, 0, 305},
    { 74,    1, 0},
    { 75,    1, 0},
    { 76,    1, 0},
    { 77,    1, 0},
    { 78,    1, 0},
    { 79,    1, 0},
    { 80,    1, 0},
    { 81,    1, 0},
    { 82,    1, 0},
    { 83,    1, 0},
    { 84,    1, 0},
    { 85,    1, 0},
    { 86,    1, 0},
    { 87,    1, 0},
    { 88,    1, 0},
    { 89,    1, 0},
    { 90,    1, 0},
    { 91,    1, 0},
};

#undef S_C
