#include "common_elem_macros.h"

#define PHYSINFO_ARRAY_NAME linmagx_physinfo
#include "pi_linac1_31.h"


#define MAGSYS_LINE(id, label, tip, max)                                   \
    {__CX_STRINGIZE(id) "_set", label,     "A", "%6.1f", "groupable", tip, \
     LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL,                   \
     PHYS_ID(__CX_CONCATENATE(CSET_, id)),                                 \
     NULL, NULL, 0.0, max, .mindisp=0.0, .maxdisp=max},                    \
    {__CX_STRINGIZE(id) "_mes", label"_m", "",  "%6.1f", NULL,        NULL,\
     LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL,                   \
     PHYS_ID(__CX_CONCATENATE(CMES_, id)),                                 \
     MAGX_C100(__CX_CONCATENATE(CSET_, id),__CX_CONCATENATE(CMES_, id)),   \
     NULL, 0.0, 1.0, 0.0, 0.0, 2.0, 1.0, 0.0, max},                        \
    {__CX_STRINGIZE(id) "_dvn", NULL,      "",  "%6.3f", NULL,        NULL,\
     LOGT_MINMAX, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL,                   \
     PHYS_ID(__CX_CONCATENATE(CMES_, id)), NULL},                          \
    EMPTY_CELL(),                                                          \
    EMPTY_CELL(),                                                          \
    EMPTY_CELL()

#define V1000_LINE(id, label, tip, n, max)                                 \
    {__CX_STRINGIZE(id) "_set", label,     "A", "%6.1f", "groupable", tip, \
     LOGT_WRITE1, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL,                   \
     PHYS_ID(B_V1PKS+(n)), NULL, NULL, 0.0, max},                          \
    {__CX_STRINGIZE(id) "_mes", label"_m", "",  "%6.1f", NULL,        NULL,\
     LOGT_READ,   LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL,                   \
     PHYS_ID(B_V1ADC+(n)*5), MAGX_C100(B_V1PKS+(n),B_V1ADC+(n)*5),  NULL, 0.0, 1.0, 0.0, 0.0, 2.0, 1.0, 0.0, 0.0}, \
    {__CX_STRINGIZE(id) "_dvn", NULL,      "",  "%6.3f", NULL,        NULL,\
     LOGT_MINMAX, LOGD_TEXT,   LOGK_DIRECT, LOGC_NORMAL,                   \
     PHYS_ID(B_V1ADC+(n)*5), NULL},                                        \
    {"", " ", NULL, NULL, NULL, NULL,                                      \
     LOGT_READ, LOGD_HLABEL, LOGK_NOP},                                    \
    EMPTY_CELL(),                                                          \
    EMPTY_CELL()

//////////////////////////////////////////////////////////////////////

enum
{
    BEAM_PCP_PM_CHAN = B_IST_IST + IST_XCDAC20_NUMCHANS * 3 + IST_XCDAC20_CHAN_DCCT1
};


static groupunit_t linmagx_grouping[] =
{
    GLOBELEM_START_CRN("magsys", "Ì¡«Œ…‘Œ¡— ”…”‘≈Õ¡",
                       "˙¡ƒ¡Œœ, A\vÈ⁄Õ., A\vd/dT\v\v", NULL,
                       1 + 23*6+22*6+6+6+6, 6+24*65536 + (1 << 24))

#define ZERO_FILENAME "mode_linmag_zero.dat"
        {"-",    " !000! ", NULL, NULL, "color=red shift file='"ZERO_FILENAME"'", "Û¬“œ”…‘ÿ ◊”≈ ’”‘¡◊À… ◊ ŒœÃÿ, Œ¡»!\n(˙¡«“’⁄…‘ÿ "ZERO_FILENAME")\n“… Œ¡÷¡‘…… ƒ≈“÷¡‘ÿ Shift.", LOGT_WRITE1, LOGD_MODE_BTN, LOGK_NOP, LOGC_NORMAL},

        V1000_LINE (Rings,        "Rings",      "\n˜1000: ˜1000-3",            7,      0.0),
        MAGX_IST_XCDAC20_LINE(Solenoid1, "Solenoid 1", "\nÈ2: ÈÛÙ-3", B_IST_ICD20 + XCDAC20_NUMCHANS * 0, B_IST_IST + IST_XCDAC20_NUMCHANS * 0, 1800.0, 1.0, 1, 0),
        MAGSYS_LINE(Lens1,        "Lens 1",     "\nÈ2: ˜˛300-1",                    240.0),
        MAGSYS_LINE(Lens2,        "Lens 2",     "\nÈ2: ˜˛300-2",                    240.0),
        MAGSYS_LINE(Lens3,        "Lens 3",     "\nÈ2: ˜˛300-3",                    240.0),
        MAGSYS_LINE(Lens4,        "Lens 4",     "\nÈ2: ıÌ-15-1",                     15.0),
        V1000_LINE (Spectrometer, "Spectrom.",  "\n˜1000: ˜2000",              6,      0.0),
        MAGSYS_LINE(Lens5,        "Lens 5",     "\nÈ2: ıÌ-15-2",                     15.0),
        MAGSYS_LINE(Lens6n7,      "Lens 6/7",   "\nÈ2: ˜˛300-4",                    240.0),
        MAGSYS_LINE(Lens8n9,      "Lens 8/9",   "\nÈ2: ˜˛300-5",                    240.0),
        MAGSYS_LINE(Lens10,       "Lens 10",    "\nÈ2: ˜˛300-6",                    240.0),
        MAGSYS_LINE(Lens11,       "Lens 11",    "\nÈ2: ˜˛300-7",                    240.0),
        MAGSYS_LINE(Lens12,       "Lens 12",    "\nÈ2: ˜300-1 (˜≈Ã…À¡Œœ◊)",          90.0),
        MAGSYS_LINE(Lens13,       "Lens 13",    "\nÈ2: ˜300-2 (˜≈Ã…À¡Œœ◊)",          90.0),
        MAGSYS_LINE(Lens14,       "Lens 14",    "\nÈ2: ˜300-3 (˜≈Ã…À¡Œœ◊)",          90.0),
        MAGSYS_LINE(LensB1,       "LensB 1",    "\nÈ2: ·300-1",                     250.0),
        MAGSYS_LINE(LensB3n4,     "LensB 3/4",  "\nÈ2: ·300-2",                     200.0),
        MAGSYS_LINE(LensB2n5,     "LensB 2/5",  "\nÈ2: ·300-3",                     216.0),
        MAGX_IST_XCDAC20_LINE(Bend,    "Bend", "\nÈÛÙÚ: ÈÛÙ-1",        B_IST_ICD20 + XCDAC20_NUMCHANS * 3, B_IST_IST + IST_XCDAC20_NUMCHANS * 3, 1800.0, 1.0, 1, 0),
        V1000_LINE (TrLens1,      "TrLens1",    "\n˜1000: ˜300-1, Ù“…–Ã≈‘",    0,    0.0),
        V1000_LINE (TrLens2,      "TrLens2",    "\n˜1000: ˜300-2, Ù“…–Ã≈‘",    1,    0.0),
        V1000_LINE (TrLens3,      "TrLens3",    "\n˜1000: ˜300-3, Ù“…–Ã≈‘",    2,    0.0),
//#if 1
        MAGX_IST_XCDAC20_LINE(Coil1, "Coil1",   "\n˜1000: ˜1000-1",    B_IST_ICD20 + XCDAC20_NUMCHANS * 4, B_IST_IST + IST_XCDAC20_NUMCHANS * 4, 1500.0, 1.0, 1, 0),
//#else
        V1000_LINE (Coil1o,        "Coil1-old", "\n˜1000: ˜1000-1",            4,    0.0),
//#endif
        V1000_LINE (Coil2,        "Coil2",      "\n˜1000: ˜1000-2",            5,    0.0),
        MAGX_IST_XCDAC20_LINE(Solenoid2,    "Solenoid 2", "\nÈÛÙÚ: ÈÛÙ-2", B_IST_ICD20 + XCDAC20_NUMCHANS * 2, B_IST_IST + IST_XCDAC20_NUMCHANS * 2, 1800.0, 1.0, 1, 0),
        V1000_LINE (TrPosL1n3,    "Lens 15",    "\n˜1000: ˜300-4",             3,    0.0),

        MAGX_XCH300_LINE(lens16, "Lens 16", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 0, 0, 1000.0),
        MAGX_XCH300_LINE(lens17, "Lens 17", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 1, 1, 1000.0),
        MAGX_XCH300_LINE(lens18, "Lens 18", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 2, 2, 1000.0),
        MAGX_XCH300_LINE(lens19, "Lens 19", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 3, 3, 1000.0),
        MAGX_XCH300_LINE(lens20, "Lens 20", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 4, 4, 1000.0),
        MAGX_XCH300_LINE(lens21, "Lens 21", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 5, 5, 1000.0),
        MAGX_XCH300_LINE(lens22, "Lens 22", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 6, 6, 1000.0),
        MAGX_XCH300_LINE(lens23, "Lens 23", "", B_NEWVCH300_ADC1, B_NEWVCH300_DAC1, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 7, 7, 1000.0),
        MAGX_XCH300_LINE(lens24, "Lens 24", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 8, 0, 1000.0),
        MAGX_XCH300_LINE(lens25, "Lens 25", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS* 9, 1, 1000.0),
        MAGX_XCH300_LINE(lens26, "Lens 26", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS*10, 2, 1000.0),
        MAGX_XCH300_LINE(lens27, "Lens 27", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS*11, 3, 1000.0),
        MAGX_XCH300_LINE(lens28, "Lens 28", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS*12, 4, 1000.0),
        MAGX_XCH300_LINE(lens29, "Lens 29", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS*13, 5, 1000.0),
        MAGX_XCH300_LINE(lens30, "Lens 30", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS*14, 6, 1000.0),
        MAGX_XCH300_LINE(lens31, "Lens 31", "", B_NEWVCH300_ADC2, B_NEWVCH300_DAC2, B_NEWVCH300_V3HS + V3H_XA40D16_NUMCHANS*15, 7, 1000.0),

        MAGSYS_LINE(Separator,    "Corr1",      "\nÈ2: ·300-0",                     300.0),
        MAGSYS_LINE(TrPosL2,      "Corr 2/3",   "\nÈ2: ˜˛300-8",                    300.0),
        
        {"pcp", "Beam PC+", "MeV", "%5.1f", NULL, "", LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t []){
                CMD_GETP_I(BEAM_PCP_PM_CHAN),
                CMD_LAPPROX_FROM_I("bend", 1*65536+2),
                CMD_MUL_I(60.0*300.0/1e06),
                CMD_RET
            }},
        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),

        {"pcm", "Beam PC-", "MeV", "%5.1f", NULL, "", LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
            (excmd_t []){
                CMD_GETP_I(BEAM_PCP_PM_CHAN),
                CMD_LAPPROX_FROM_I("bend", 1*65536+3),
                CMD_MUL_I(60.0*300.0/1e06),
                CMD_RET
            }},

        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),
        EMPTY_CELL(),
        MAGX_IST_XCDAC20_LINE(1M1_1M4, "1M1-1M4", "\nÈ2: ÈÛÙ-?", B_IST_ICD20 + XCDAC20_NUMCHANS * 1, B_IST_IST + IST_XCDAC20_NUMCHANS * 1, 1800.0, 1.0, 1, 0),
    GLOBELEM_END("", 0),

    GLOBELEM_START_CRN("magcor", "Ì¡«Œ…‘Œ¡— Àœ““≈À√…—",
                       "˙¡ƒ¡Œœ, mA\vÈ⁄Õ.\vU, V", NULL,
                       24*3+24*3, 3 + 24*65536)
        MAGX_XOR4016_LINE(B_XOR4016, 0,  LENS1,  "LENS1"),
        MAGX_XOR4016_LINE(B_XOR4016, 1,  LENS2,  "LENS2"),
        MAGX_XOR4016_LINE(B_XOR4016, 2,  GUN_V,  "GUN_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 3,  GUN_H,  "GUN_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 4,  CAV_V,  "CAV_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 5,  CAV_H,  "CAV_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 6,  SCR_V,  "SCR_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 7,  SCR_H,  "SCR_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 8,  BUN_H,  "BUN_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 9,  BUN_V,  "BUN_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 10, BUN1_H, "BUN1_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 11, BUN2_H, "BUN2_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 12, S1_V,   "S1_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 13, S1_H,   "S1_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 14, S2_V,   "S2_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 15, S2_H,   "S2_H"),

        MAGX_XOR4016_LINE(B_XOR4016, 16, S3_V,   "S3_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 17, S3_H,   "S3_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 18, S4_V,   "S4_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 19, S4_H,   "S4_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 20, S5_V,   "S5_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 21, S5_H,   "S5_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 22, B1,     "B1"),
        MAGX_XOR4016_LINE(B_XOR4016, 23, B2,     "B2"),
        MAGX_XOR4016_LINE(B_XOR4016, 24, B_V,    "B_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 25, B3,     "B3"),
        MAGX_XOR4016_LINE(B_XOR4016, 26, TR_V,   "TR_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 27, TR_H,   "TR_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 28, CNV_V,  "CNV_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 29, CNV_H,  "CNV_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 30, S6_V,   "S6_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 31, S6_H,   "S6_H"),

        MAGX_XOR4016_LINE(B_XOR4016, 32, S7_V,   "S7_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 33, S7_H,   "S7_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 34, S8_V,    "S8_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 35, S8_H,    "S8_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 36, S9_V,    "S9_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 37, S9_H,    "S9_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 38, S10_V,   "S10_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 39, S10_H,   "S10_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 40, S11_V,   "S11_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 41, S11_H,   "S11_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 42, S12_V,   "S12_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 43, S12_H,   "S12_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 44, S13_V,   "S13_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 45, S13_H,   "S13_H"),
        MAGX_XOR4016_LINE(B_XOR4016, 46, S14_V,   "S14_V"),
        MAGX_XOR4016_LINE(B_XOR4016, 47, S14_H,   "S14_H"),
    GLOBELEM_END("", 0),

    GLOBELEM_START_CRN("mc-c208", "Îœ“208",
                       "Iset, mA\vImes\vUmes, V", NULL,
                       2 + 6*3, 3 + (2 << 24))
        {"-switch", NULL, NULL, NULL, "#T0\f\flit=red\v1\f\flit=green", NULL,
         LOGT_WRITE1, LOGD_CHOICEBS, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1),
         (excmd_t[]){
             CMD_GETP_I(B_FED208 + XCAC208_CHAN_REGS_RD8_BASE + 0),
             CMD_RET
         },
         (excmd_t[]){
             CMD_PUSH_I(1), CMD_QRYVAL, CMD_SUB,
             CMD_SETP_I(B_FED208 + XCAC208_CHAN_REGS_WR8_BASE + 0),
             CMD_RET
         },
        },
        {"opora",  "Ô–œ“¡", "V", "%6.2f", "withlabel", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(B_FED208 + XCAC208_CHAN_ADC_n_BASE + 12)},
        MAGX_COR208_LINE(B_FED208, 0, tech_v, "tech_v"),
        MAGX_COR208_LINE(B_FED208, 1, tech_h, "tech_h"),
        MAGX_COR208_LINE(B_FED208, 2, tr2_h,  "tr2_h"),
        MAGX_COR208_LINE(B_FED208, 3, tr2_v,  "tr2_v"),
        MAGX_COR208_LINE(B_FED208, 4, n45,    "#4/N5"),
        MAGX_COR208_LINE(B_FED208, 5, n56,    "#5/N6"),
    GLOBELEM_END("", 0),

    {NULL}
};
