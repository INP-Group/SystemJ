#include "drv_i/karpov_monster_drv_i.h"
#include "bpmclient.h"


#define EMPTY_CELL        {"-", "",  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HLABEL, LOGK_NOP}
#define VERTLABEL_CELL(l) {"-", l,   NULL, NULL, NULL, NULL, LOGT_READ, LOGD_VLABEL, LOGK_NOP}
#define HSEPARATOR_CELL   {"-", "",  NULL, NULL, NULL, NULL, LOGT_READ, LOGD_HSEP,   LOGK_NOP}


#define DELAY_SETTING_CELL(n, l) \
    {"delay" l, "Задержка " l, "",   "%5.2f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KARPOV_DELAY_C(n))}

#define AMPL_SETTING_CELL(n, l) \
    {"ampl" l,  NULL,          NULL, "%2.0f", "#F16d,1,0,#%d", NULL, LOGT_WRITE1, LOGD_SELECTOR, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(KARPOV_AMPL_C(n))}

#define MSMNT_CELL(l, t, u, df, c) \
    {l, t, u, df, NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(c)}

#define PROFILE_LINE(n) \
    {NULL, "Разрез " n, NULL,  NULL, "n=" n, NULL, LOGT_READ, LOGD_BPM_PROFILE, LOGK_NOP}
    
#define GRAPH_LINE(l, wdi) \
    {NULL, l, NULL,  NULL, wdi, NULL, LOGT_READ, LOGD_BPM_GRAPH, LOGK_NOP}

#define RESULTS_LINE(n, l)                                       \
    MSMNT_CELL("ux1-" l, "Ux1-" l, "",   "%5.0f", KARPOV_UX1_C(n)), \
    MSMNT_CELL("ux2-" l, "Ux2-" l, "",   "%5.0f", KARPOV_UX2_C(n)), \
    MSMNT_CELL("uy1-" l, "Uy1-" l, "",   "%5.0f", KARPOV_UY1_C(n)), \
    MSMNT_CELL("uy2-" l, "Uy2-" l, "",   "%5.0f", KARPOV_UY2_C(n)), \
    {          "sum-" l, "Sum-" l, "",   "%5.0f", NULL, NULL, LOGT_READ, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_GETP_I(KARPOV_UX1_C(n)), CMD_GETP_I(KARPOV_UX2_C(n)), \
                    CMD_GETP_I(KARPOV_UY1_C(n)), CMD_GETP_I(KARPOV_UY2_C(n)), \
                    CMD_ADD, CMD_ADD, CMD_ADD, CMD_RET} \
    },                                                           \
    MSMNT_CELL("x-"   l, "X-" l,   "mm", "%4.1f", KARPOV_X_C(n)),   \
    MSMNT_CELL("y-"   l, "Y-" l,   "mm", "%4.1f", KARPOV_Y_C(n))


static groupunit_t linbpm_grouping[] =
{
    {
        &(elemnet_t){
            "settings", "Уставки",
            "Задержка\vУсиление",
            "1-\v2-\v3-\v4-\v5-\v7+\v8+\v9+\v10+\v11+\v12+\v13+\v14+",
            ELEM_MULTICOL, 13*2, 2+5*65536,
            (logchannet_t []){
                DELAY_SETTING_CELL(0,  "1"),
                AMPL_SETTING_CELL (0,  "1"),
                DELAY_SETTING_CELL(1,  "2"),
                AMPL_SETTING_CELL (1,  "2"),
                DELAY_SETTING_CELL(2,  "3"),
                AMPL_SETTING_CELL (2,  "3"),
                DELAY_SETTING_CELL(3,  "4"),
                AMPL_SETTING_CELL (3,  "4"),
                DELAY_SETTING_CELL(4,  "5"),
                AMPL_SETTING_CELL (4,  "5"),
		DELAY_SETTING_CELL(5,  "6"),
                AMPL_SETTING_CELL (5,  "6"),
                DELAY_SETTING_CELL(6,  "7"),
                AMPL_SETTING_CELL (6,  "7"),
                DELAY_SETTING_CELL(10, "8"),
                AMPL_SETTING_CELL (10, "8"),
                DELAY_SETTING_CELL(11, "9"),
                AMPL_SETTING_CELL (11, "9"),
                DELAY_SETTING_CELL(12, "10"),
                AMPL_SETTING_CELL (12, "10"),
		DELAY_SETTING_CELL(13, "11"),
                AMPL_SETTING_CELL (13, "11"),
                DELAY_SETTING_CELL(14, "12"),
                AMPL_SETTING_CELL (14, "12"),
                DELAY_SETTING_CELL(7,  "13"),
                AMPL_SETTING_CELL (15, "13"),
                DELAY_SETTING_CELL(8,  "14"),
                AMPL_SETTING_CELL (16, "14"),
            },
            "fold_h",
        }, 0
    },
    {
        &(elemnet_t){
            "linbpm", "Всякая хрень",
            "",
            "?\v?\v?",
            ELEM_TABBER, 3, 1,
            (logchannet_t []){
                //{NULL, "График",  NULL,  NULL, NULL, NULL, LOGT_READ, LOGD_BPM_GRAPH,   LOGK_NOP},
                {NULL, "Графики", NULL,  NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "graph", "Graph",
                        "",
                        "?\v?\v",
                        ELEM_MULTICOL, 2, 1,//2 * 3, 3,
                        (logchannet_t []){
                            GRAPH_LINE("X", "x count=13"),
                            GRAPH_LINE("Y", "y count=13"),
                        },
                        "notitle,noshadow,nocoltitles"
                    }
                },
		//{NULL, "Профиль", NULL,  NULL, NULL, NULL, LOGT_READ, LOGD_BPM_PROFILE, LOGK_NOP},
		{NULL, "Разрезы", NULL,  NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "profiles", "Profiles",
                        "",
                        "1-\v2-\v3-\v4-\v5-\v7+\v8+\v9+\v10+\v11+\v12+\v13+\v14+",
                        ELEM_MULTICOL, 13, 1 + 5*65536,
                        (logchannet_t []){
                            PROFILE_LINE("0"),
                            PROFILE_LINE("1"),
                            PROFILE_LINE("2"),
			    PROFILE_LINE("3"),
                            PROFILE_LINE("4"),
                            PROFILE_LINE("5"),
			    PROFILE_LINE("6"),
                            PROFILE_LINE("7"),
                            PROFILE_LINE("8"),
			    PROFILE_LINE("9"),
                            PROFILE_LINE("10"),
                            PROFILE_LINE("11"),
			    PROFILE_LINE("12"),
                            PROFILE_LINE("13"),
			},
                        "notitle,noshadow,nocoltitles,transposed"
                    }
                },
		
                {"numbers", "Числа",   NULL,  NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "numbers", "Числа",
                        "Ux1\vUx2\vUy1\vUy2\vSum\vX\vY",
                        "1-\v2-\v3-\v4-\v5-\v-+\v7+\v8+\v9+\v10+\v11+\v12+\v13+\v14+",
                        ELEM_MULTICOL, 13 * 7, 7+7*65536,
                        (logchannet_t []){
                            RESULTS_LINE(0,  "1-"),
                            RESULTS_LINE(1,  "2-"),
                            RESULTS_LINE(2,  "3-"),
                            RESULTS_LINE(3,  "4-"),
                            RESULTS_LINE(4,  "5-"),
			    RESULTS_LINE(5,  "7+"),
                            RESULTS_LINE(6,  "8+"),
                            RESULTS_LINE(10, "9+"),
                            RESULTS_LINE(11, "10+"),
                            RESULTS_LINE(12, "11+"),
			    RESULTS_LINE(13, "12+"),
                            RESULTS_LINE(14, "13+"),
                            RESULTS_LINE(15, "14+"),
                        },
                        "notitle,noshadow,transposed"
                    }
                },
		/*
		{NULL, "Числа",   NULL,  NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                    (void *)&(elemnet_t){
                        "lotofnumbers", "Числа",
                        "",
                        "",
                        ELEM_MULTICOL, 2, 1,
                        (logchannet_t []){
			    {NULL, "Числа",   NULL,  NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                		(void *)&(elemnet_t){
                    		"numbers", "Числа",
                    		"Ux1\vUx2\vUy1\vUy2\vSum\vX\vY",
                    		"1-\v2-\v3-\v4-\v5-\v6+\v7+",
                    		ELEM_MULTICOL, 7 * 7, 7,
                    		(logchannet_t []){
                        	    RESULTS_LINE(0, "1"),
                        	    RESULTS_LINE(1, "2"),
                        	    RESULTS_LINE(2, "3"),
                        	    RESULTS_LINE(3, "4"),
                        	    RESULTS_LINE(4, "5"),
				    RESULTS_LINE(5, "6"),
                        	    RESULTS_LINE(6, "7"),
                    		},
                    		"notitle,noshadow,transposed"
                		}
            		    },
			    {NULL, "Числа2",   NULL,  NULL, NULL, NULL, LOGT_SUBELEM, -1, -1, -1, PHYS_ID(-1),
                		(void *)&(elemnet_t){
                    		"numbers2", "Числа2",
                    		"Ux1\vUx2\vUy1\vUy2\vSum\vX\vY",
                    		"8+\v9+\v10+\v11+\v12+\v13+\v14+",
                    		ELEM_MULTICOL, 7 * 7, 7,
                    		(logchannet_t []){
                        	    RESULTS_LINE(10, "11"),
                        	    RESULTS_LINE(11, "12"),
                        	    RESULTS_LINE(12, "13"),
				    RESULTS_LINE(13, "14"),
                        	    RESULTS_LINE(14, "15"),
                        	    RESULTS_LINE(15, "16"),
                        	    RESULTS_LINE(16, "17"),
                    		},
                    		"notitle,noshadow,transposed"
                		}
            		    },
                        },
                        "notitle,noshadow"
                    }
                },
		*/
            },
            "bottom",
        }, 1
    },
    {NULL}
};

static physprops_t linbpm_physinfo[] =
{
    {KARPOV_DELAY_C( 0), 1000},
    {KARPOV_DELAY_C( 1), 1000},
    {KARPOV_DELAY_C( 2), 1000},
    {KARPOV_DELAY_C( 3), 1000},
    {KARPOV_DELAY_C( 4), 1000},
    {KARPOV_DELAY_C( 5), 1000},
    {KARPOV_DELAY_C( 6), 1000},
    {KARPOV_DELAY_C( 7), 1000},
    {KARPOV_DELAY_C( 8), 1000},
    {KARPOV_DELAY_C( 9), 1000},
    {KARPOV_DELAY_C(10), 1000},
    {KARPOV_DELAY_C(11), 1000},
    {KARPOV_DELAY_C(12), 1000},
    {KARPOV_DELAY_C(13), 1000},
    {KARPOV_DELAY_C(14), 1000},

    {KARPOV_X_C( 0), 1000},
    {KARPOV_Y_C( 0), 1000},
    {KARPOV_X_C( 1), 1000},
    {KARPOV_Y_C( 1), 1000},
    {KARPOV_X_C( 2), 1000},
    {KARPOV_Y_C( 2), 1000},
    {KARPOV_X_C( 3), 1000},
    {KARPOV_Y_C( 3), 1000},
    {KARPOV_X_C( 4), 1000},
    {KARPOV_Y_C( 4), 1000},
    {KARPOV_X_C( 5), 1000},
    {KARPOV_Y_C( 5), 1000},
    {KARPOV_X_C( 6), 1000},
    {KARPOV_Y_C( 6), 1000},
    {KARPOV_X_C( 7), 1000},
    {KARPOV_Y_C( 7), 1000},
    {KARPOV_X_C( 8), 1000},
    {KARPOV_Y_C( 8), 1000},
    {KARPOV_X_C( 9), 1000},
    {KARPOV_Y_C( 9), 1000},
    {KARPOV_X_C(10), 1000},
    {KARPOV_Y_C(10), 1000},
    {KARPOV_X_C(11), 1000},
    {KARPOV_Y_C(11), 1000},
    {KARPOV_X_C(12), 1000},
    {KARPOV_Y_C(12), 1000},
    {KARPOV_X_C(13), 1000},
    {KARPOV_Y_C(13), 1000},
    {KARPOV_X_C(14), 1000},
    {KARPOV_Y_C(14), 1000},
    {KARPOV_X_C(15), 1000},
    {KARPOV_Y_C(15), 1000},
    {KARPOV_X_C(16), 1000},
    {KARPOV_Y_C(16), 1000},
    {KARPOV_X_C(17), 1000},
    {KARPOV_Y_C(17), 1000},
    {KARPOV_X_C(18), 1000},
    {KARPOV_Y_C(18), 1000},
    {KARPOV_X_C(19), 1000},
    {KARPOV_Y_C(19), 1000},
};
