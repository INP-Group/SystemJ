
#include "drv_i/kozdev_common_drv_i.h"
#include "common_elem_macros.h"




static groupunit_t kozdev_grouping[] =
{

#define ADC_LINE(n) \
    {"adc" __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%9.6f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_ADC_n_base+n, .minnorm=-10.0, .maxnorm=+10.0}

#define DAC_LINE(n) \
    {"dac"    __CX_STRINGIZE(n), __CX_STRINGIZE(n), NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_OUT_n_base      + n, .minnorm=-10.0, .maxnorm=+10.0}, \
    {"dacspd" __CX_STRINGIZE(n), NULL,              NULL, "%7.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_OUT_RATE_n_base + n, .minnorm=-20.0, .maxnorm=+20.0}, \
    {"daccur" __CX_STRINGIZE(n), NULL,              NULL, "%7.4f", NULL, NULL, LOGT_READ,   LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_OUT_CUR_n_base  + n, .minnorm=-10.0, .maxnorm=+10.0}


    GLOBELEM_START("adc", "ADC, V", 4 + KOZDEV_CHAN_ADC_n_maxcnt, 1 + 10*65536 + (4 << 24))
        {"adc_time",  "",   NULL, NULL, "1ms\v2ms\v5ms\v10ms\v20ms\v40ms\v80ms\v160ms", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_ADC_TIMECODE},
        {"adc_magn",  "",   NULL, NULL, "x1\vx10\v\nx100\v\nx1000",                     NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_ADC_GAIN},
        {"beg",       "Ch", NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_ADC_BEG, .minnorm=0, .maxnorm=KOZDEV_CHAN_ADC_n_maxcnt-1},
        {"end",       "-",  NULL, "%2.0f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_ADC_END, .minnorm=0, .maxnorm=KOZDEV_CHAN_ADC_n_maxcnt-1},
        ADC_LINE(0),
        ADC_LINE(1),
        ADC_LINE(2),
        ADC_LINE(3),
        ADC_LINE(4),
        ADC_LINE(5),
        ADC_LINE(6),
        ADC_LINE(7),
        ADC_LINE(8),
        ADC_LINE(9),
        ADC_LINE(10),
        ADC_LINE(11),
        ADC_LINE(12),
        ADC_LINE(13),
        ADC_LINE(14),
        ADC_LINE(15),
        ADC_LINE(16),
        ADC_LINE(17),
        ADC_LINE(18),
        ADC_LINE(19),
        ADC_LINE(20),
        ADC_LINE(21),
        ADC_LINE(22),
        ADC_LINE(23),
        ADC_LINE(24),
        ADC_LINE(25),
        ADC_LINE(26),
        ADC_LINE(27),
        ADC_LINE(28),
        ADC_LINE(29),
        ADC_LINE(30),
        ADC_LINE(31),
        ADC_LINE(32),
        ADC_LINE(33),
        ADC_LINE(34),
        ADC_LINE(35),
        ADC_LINE(36),
        ADC_LINE(37),
        ADC_LINE(38),
        ADC_LINE(39),
        ADC_LINE(40),
        ADC_LINE(41),
        ADC_LINE(42),
        ADC_LINE(43),
        ADC_LINE(44),
        ADC_LINE(45),
        ADC_LINE(46),
        ADC_LINE(47),
        ADC_LINE(48),
        ADC_LINE(49),
        ADC_LINE(50),
        ADC_LINE(51),
        ADC_LINE(52),
        ADC_LINE(53),
        ADC_LINE(54),
        ADC_LINE(55),
        ADC_LINE(56),
        ADC_LINE(57),
        ADC_LINE(58),
        ADC_LINE(59),
        ADC_LINE(60),
        ADC_LINE(61),
        ADC_LINE(62),
        ADC_LINE(63),
        ADC_LINE(64),
        ADC_LINE(65),
        ADC_LINE(66),
        ADC_LINE(67),
        ADC_LINE(68),
        ADC_LINE(69),
        ADC_LINE(70),
        ADC_LINE(71),
        ADC_LINE(72),
        ADC_LINE(73),
        ADC_LINE(74),
        ADC_LINE(75),
        ADC_LINE(76),
        ADC_LINE(77),
        ADC_LINE(78),
        ADC_LINE(79),
        ADC_LINE(80),
        ADC_LINE(81),
        ADC_LINE(82),
        ADC_LINE(83),
        ADC_LINE(84),
        ADC_LINE(85),
        ADC_LINE(86),
        ADC_LINE(87),
        ADC_LINE(88),
        ADC_LINE(89),
        ADC_LINE(90),
        ADC_LINE(91),
        ADC_LINE(92),
        ADC_LINE(93),
        ADC_LINE(94),
        ADC_LINE(95),
        ADC_LINE(96),
        ADC_LINE(97),
        ADC_LINE(98),
        ADC_LINE(99),
    GLOBELEM_END("nocoltitles", GU_FLAG_FROMNEWLINE),

    GLOBELEM_START_CRN("adc", "DAC", "Set, V\vMaxSpd, V/s\vCur, V", NULL, 2 + KOZDEV_CHAN_OUT_n_maxcnt*3, 3 + 8*65536 + (2 << 24))
        {"dac_state", "Mode:", NULL, NULL, "#+#TNorm\vTable", NULL, LOGT_READ, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, KOZDEV_CHAN_OUT_MODE},
        SUBWIN_START("dac_table", "DAC table mode control", 3, 3)
            EMPTY_CELL(),
            {NULL, NULL, NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VSEP, LOGK_NOP, .placement="vert=fill"},
            SUBELEM_START(NULL, NULL, 6, 1)
                BUTT_CELL(NULL, "Drop",     KOZDEV_CHAN_DO_TAB_DROP),
                BUTT_CELL(NULL, "Activate", KOZDEV_CHAN_DO_TAB_ACTIVATE),
                BUTT_CELL(NULL, "Start",    KOZDEV_CHAN_DO_TAB_START),
                BUTT_CELL(NULL, "Stop",     KOZDEV_CHAN_DO_TAB_STOP),
                BUTT_CELL(NULL, "Pause",    KOZDEV_CHAN_DO_TAB_PAUSE),
                BUTT_CELL(NULL, "Resume",   KOZDEV_CHAN_DO_TAB_RESUME),
            SUBELEM_END("noshadow,notitle,nocoltitles,norowtitles", NULL),
        SUBWIN_END("Table...", "noshadow,notitle,nocoltitles", NULL),

        DAC_LINE(0),
        DAC_LINE(1),
        DAC_LINE(2),
        DAC_LINE(3),
        DAC_LINE(4),
        DAC_LINE(5),
        DAC_LINE(6),
        DAC_LINE(7),
        DAC_LINE(8),
        DAC_LINE(9),
        DAC_LINE(10),
        DAC_LINE(11),
        DAC_LINE(12),
        DAC_LINE(13),
        DAC_LINE(14),
        DAC_LINE(15),
        DAC_LINE(16),
        DAC_LINE(17),
        DAC_LINE(18),
        DAC_LINE(19),
        DAC_LINE(20),
        DAC_LINE(21),
        DAC_LINE(22),
        DAC_LINE(23),
        DAC_LINE(24),
        DAC_LINE(25),
        DAC_LINE(26),
        DAC_LINE(27),
        DAC_LINE(28),
        DAC_LINE(29),
        DAC_LINE(30),
        DAC_LINE(31),
    GLOBELEM_END("", GU_FLAG_FROMNEWLINE),

    {NULL}
};


static physprops_t kozdev_physinfo[] =
{
    {KOZDEV_CHAN_ADC_n_base +  0, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  1, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  2, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  3, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  4, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  5, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  6, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  7, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  8, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  9, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  10, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  11, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  12, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  13, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  14, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  15, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  16, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  17, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  18, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  19, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  20, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  21, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  22, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  23, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  24, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  25, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  26, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  27, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  28, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  29, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  30, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  31, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  32, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  33, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  34, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  35, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  36, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  37, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  38, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  39, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  40, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  41, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  42, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  43, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  44, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  45, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  46, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  47, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  48, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  49, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  50, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  51, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  52, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  53, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  54, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  55, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  56, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  57, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  58, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  59, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  60, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  61, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  62, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  63, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  64, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  65, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  66, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  67, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  68, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  69, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  70, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  71, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  72, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  73, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  74, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  75, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  76, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  77, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  78, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  79, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  80, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  81, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  82, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  83, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  84, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  85, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  86, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  87, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  88, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  89, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  90, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  91, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  92, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  93, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  94, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  95, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  96, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  97, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  98, 1000000.0, 0, 0},
    {KOZDEV_CHAN_ADC_n_base +  99, 1000000.0, 0, 0},
};
