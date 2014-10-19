#ifndef UBS_N
  #error The "UBS_N" macro is undefined
#endif
#ifndef UBS_L
  #error The "UBS_L" macro is undefined
#endif

#define UBS_FQN   __CX_STRINGIZE(RACK_N)"-"UBS_L
#define UBS_TITLE "Модулятор "UBS_FQN
#define UBS_SUBWIN_ID "subwin-ubs"

CANVAS_START("cont-ubs"UBS_L, NULL, 5)

    SUBWINV4_START(UBS_SUBWIN_ID, UBS_TITLE, 1)
    
        {"ubs"UBS_L, "subelem-ubs"UBS_L, NULL, NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
            #define  UBS_R_IH2N         UBS_HEAT_REG_NAME (RACK_N,UBS_N,IH,2,N)
            #define  UBS_R_IH1N         UBS_HEAT_REG_NAME (RACK_N,UBS_N,IH,1,N)

            #define  UBS_R_ST2T         UBS_HEAT_REG_NAME (RACK_N,UBS_N,ST,2,T)
            #define  UBS_R_ST2W         UBS_HEAT_REG_NAME (RACK_N,UBS_N,ST,2,W)
            #define  UBS_R_ST2P         UBS_HEAT_REG_NAME (RACK_N,UBS_N,ST,2,P)
            #define  UBS_R_ST1T         UBS_HEAT_REG_NAME (RACK_N,UBS_N,ST,1,T)
            #define  UBS_R_ST1W         UBS_HEAT_REG_NAME (RACK_N,UBS_N,ST,1,W)
            #define  UBS_R_ST1P         UBS_HEAT_REG_NAME (RACK_N,UBS_N,ST,1,P)
            #define  UBS_HEAT2TIME_FILE UBS_HEAT_FILE_NAME(RACK_N, UBS_L, 2)
            #define  UBS_HEAT1TIME_FILE UBS_HEAT_FILE_NAME(RACK_N, UBS_L, 1)

            #define  UBS_BASE     __CX_CONCATENATE(RACKX_43_UBS,__CX_CONCATENATE(UBS_N,_BASE))
#define ZZZ_UBS_BASE __CX_CONCATENATE(RACKX_43_UBS,__CX_CONCATENATE(UBS_N,_BASE))
            #define  UBS_IDENT    "mod"
            #define  UBS_LABEL    UBS_TITLE
            #define  UBS_OPTIONS  "notitle,noshadow"
            #define  UBS_MODE_INCTLS
            #define  NO_GROUPUNIT
            #include "elem_ubs.h"
        }
    
    SUBWIN_END("    \n"UBS_L"\n ", "noshadow,notitle,nocoltitles,norowtitles size=normal,bold,logc=vic", NULL),
    
    {NULL, "", NULL, NULL, "nomargins,offcol=red,color=green,size=tiny", NULL, LOGT_READ, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ZZZ_UBS_BASE + PANOV_UBS_CHAN_STAT_ST2_HEATIN), .placement="left=!3@" UBS_SUBWIN_ID",top=!3@"UBS_SUBWIN_ID},
    {NULL, "", NULL, NULL, "nomargins,offcol=red,color=green,size=tiny", NULL, LOGT_READ, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ZZZ_UBS_BASE + PANOV_UBS_CHAN_STAT_ST1_HEATIN), .placement="left=!3@" UBS_SUBWIN_ID",bottom=!3@"UBS_SUBWIN_ID},
    {NULL, "", NULL, NULL, "nomargins,offcol=red,color=green,size=tiny", NULL, LOGT_READ, LOGD_ONOFF, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(ZZZ_UBS_BASE + PANOV_UBS_CHAN_STAT_DGS_IS_ON),  .placement="right=!3@"UBS_SUBWIN_ID",bottom=!3@"UBS_SUBWIN_ID},

    SUBWINV4_START("subwin-adc", "ADC200ME и ADC812ME модулятора "UBS_FQN, 1)
        CANVAS_START(":", "ADCs", 3)

#define  ADC200_AUX_PARAMS \
    " foldctls  height=200" \
    " save_button subsys=\""UBS_FQN"\"" \
    " magnA=4 descrA=I unitsA=kA coeffA=6 " \
    " magnB=4 descrB=U unitsB=kV coeffB=37.758"
#define TWO812_AUX_PARAMS(lr_s) \
    " foldctls height=200"       \
    " source=modulators.rack" RACK_L ".812s.adc812me-"lr_s

#if 1
            SUBELEM_START("diffs", NULL, 2, 2)
                {"avgdiff_I", "avgIdiff", "kA", NULL,
                 "src=:.adc nl=0",
                 NULL, LOGT_READ, LOGD_LIU_AVGDIFF, LOGK_NOP, LOGC_NORMAL, PHYS_ID(-1)},
                {"avgdiff_U", "avgUdiff", "kV", NULL,
                 "src=:.adc nl=1",
                 NULL, LOGT_READ, LOGD_LIU_AVGDIFF, LOGK_NOP, LOGC_NORMAL, PHYS_ID(-1)},
            SUBELEM_END("noshadow,notitle,nocoltitles", "left=0,top=0"),
#else
            SUBELEM_START("diffs", NULL, 4, 1 + (2<<16))
                {"adc_diff1", "Idiff", NULL, "%8.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(RACKX_43_200_DIFFER_BASE + (UBS_N-1)*2 + 0)},
                {"812_diff1", "1diff", NULL, "%8.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(RACKX_43_812_DIFFER_BASE + (UBS_N-1)*2 + 0)},
                {"adc_diff2", "Udiff", NULL, "%8.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(RACKX_43_200_DIFFER_BASE + (UBS_N-1)*2 + 1)},
                {"812_diff2", "2diff", NULL, "%8.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(RACKX_43_812_DIFFER_BASE + (UBS_N-1)*2 + 1)},
            SUBELEM_END("noshadow,notitle,nocoltitles", "left=0,top=0"),
#endif
            {"adc", UBS_FQN, NULL, NULL, ADC200_AUX_PARAMS, NULL, LOGT_READ, LOGD_LIU_ADC200ME_ONE,     LOGK_DIRECT, RACKX_43_ADC200ME_N_BIGC_BASE+UBS_N-1 /*!!! A hack: we use 'color' field to pass integer (bigc_n) */, PHYS_ID(RACKX_43_ADC200ME_N_BASE + (UBS_N-1)   * ADC200ME_NUM_PARAMS),
             .placement="left=0,right=0,top=@diffs"},
            {"812", NULL,    NULL, NULL, UBS_N<=3? TWO812_AUX_PARAMS("0") : TWO812_AUX_PARAMS("1"),
             NULL, LOGT_READ, LOGD_LIU_TWO812CH, LOGK_DIRECT,
             UBS_N<=3? 2-(UBS_N-1) : (UBS_N-1)-3 /*!!! A hack: we use 'color' field to pass integer (seg_n)  */, 
             PHYS_ID(RACKX_43_ADC812ME_N_BASE + (UBS_N-1)/3 * ADC812ME_NUM_PARAMS),
             .placement="left=0,right=0,top=@adc,bottom=0"},
        SUBELEM_END("noshadow,notitle", ""),
    SUBWIN_END  ("~~", "one,noshadow,notitle,nocoltitles,norowtitles size=small,logc=important,resizable,compact,noclsbtn", "top=@"UBS_SUBWIN_ID",left=@,right=@"),

SUBELEM_END("noshadow,notitle", NULL)


#undef UBS_N
#undef UBS_L

#undef UBS_FQN
#undef UBS_TITLE
#undef UBS_SUBWIN_ID
#undef SUBWIN_UBS_LINES
