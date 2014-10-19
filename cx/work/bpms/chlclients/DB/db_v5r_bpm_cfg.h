#include "common_elem_macros.h"

#include "drv_i/bpmd_cfg_drv_i.h"
#include "drv_i/bpmd_drv_i.h"
#include "drv_i/bpmd_txzi_drv_i.h"


#define COMMON_BASE                   (0)

#define BPMD_DRV_OFFSET( bpm_num_id)  (COMMON_BASE + (0   ) + BPMD_NUM_PARAMS      * (bpm_num_id - 1))
#define BPMD_TXZI_OFFSET(bpm_num_id)  (COMMON_BASE + (1700) + BPMD_TXZI_CHAN_COUNT * (bpm_num_id - 1))
#define BPMD_CFG_OFFSET( bpm_num_id)  (COMMON_BASE + (3300) + BPMD_CFG_CHAN_COUNT  * (bpm_num_id - 1))

/* ========================================================================== */

#define REGIME_AUTOMAT(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_AUTOMAT)
#define REGIME_ELCTRUN(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_ELCTRUN)
#define REGIME_ELCTINJ(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_ELCTINJ)
#define REGIME_ELCTDLS(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_ELCTDLS)
#define REGIME_PSTRRUN(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_PSTRRUN)
#define REGIME_PSTRINJ(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_PSTRINJ)
#define REGIME_PSTRDLS(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_PSTRDLS)
#define REGIME_FREERUN(bpm_num_id)    (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_REGIME_FREERUN)

#define NUMPTS_CHAN(bpm_num_id)       (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_NUMPTS  )
#define PTSOFS_CHAN(bpm_num_id)       (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_PTSOFS  )
#define DECMTN_CHAN(bpm_num_id)       (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_DECMTN  )

#define DELAY_CHAN(bpm_num_id)        (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_DELAY   )
#define ATTEN_CHAN(bpm_num_id)        (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_ATTEN   )

#define FORCE_RELOAD(bpm_num_id)      (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_FORCE_RELOAD)

#define STATE_CHAN(bpm_num_id)        (BPMD_CFG_OFFSET(bpm_num_id) + BPMD_CFG_CHAN_BPMD_STATE)

/* ========================================================================== */

#define XMEAN(bpm_num_id)             (BPMD_TXZI_OFFSET(bpm_num_id) + BPMD_TXZI_CHAN_XMEAN  )
#define XSTDDEV(bpm_num_id)           (BPMD_TXZI_OFFSET(bpm_num_id) + BPMD_TXZI_CHAN_XSTDDEV)
#define ZMEAN(bpm_num_id)             (BPMD_TXZI_OFFSET(bpm_num_id) + BPMD_TXZI_CHAN_ZMEAN  )
#define ZSTDDEV(bpm_num_id)           (BPMD_TXZI_OFFSET(bpm_num_id) + BPMD_TXZI_CHAN_ZSTDDEV)

#define IZERO_CHAN(bpm_num_id, ch)    (BPMD_DRV_OFFSET(bpm_num_id)  + BPMD_PARAM_ZERO0 + ch - 1)
#define IDLAY_CHAN(bpm_num_id, ch)    (BPMD_DRV_OFFSET(bpm_num_id)  + BPMD_PARAM_DLAY0 + ch - 1)
#define MGDLY_CHAN(bpm_num_id)        (BPMD_DRV_OFFSET(bpm_num_id)  + BPMD_PARAM_MGDLY         )

#define STATUS_CHAN(bpm_num_id)       (BPMD_DRV_OFFSET(bpm_num_id)  + BPMD_PARAM_STATUS        )

/* ========================================================================== */

enum
{
    IS_INGROUP_REGS_base = 100,
    INCDEC_REGS_base     = 200,
    SETVAL_REGS_base     = 300,
};

/* ========================================================================== */

#define GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, bpm_n)              \
    CMD_GETP_I(BPMD_CFG_OFFSET(bpm_n) + chan_ofs),                 \
    CMD_GETLCLREG_I(INCDEC_REGS_base + (reg_ofs)), CMD_MUL_I(dir), \
    CMD_ADD,                                                       \
    CMD_GETLCLREG_I(IS_INGROUP_REGS_base + (bpm_n)), CMD_TEST,     \
    CMD_SETP_I(BPMD_CFG_OFFSET(bpm_n) + chan_ofs)

#define GROUP_SETV_ONE(chan_ofs, reg_ofs, bpm_n)                   \
    CMD_GETLCLREG_I(SETVAL_REGS_base + (reg_ofs)),                 \
    CMD_GETLCLREG_I(IS_INGROUP_REGS_base + (bpm_n)), CMD_TEST,     \
    CMD_SETP_I(BPMD_CFG_OFFSET(bpm_n) + chan_ofs)

#define GROUP_SND1_ONE(chan_ofs, bpm_n)                            \
    CMD_PUSH_I(CX_VALUE_COMMAND),                                  \
    CMD_GETLCLREG_I(IS_INGROUP_REGS_base + (bpm_n)), CMD_TEST,     \
    CMD_SETP_I(BPMD_CFG_OFFSET(bpm_n) + chan_ofs)

#define GROUP_DIR_CELL(chan_ofs, reg_ofs, dir)          \
    {NULL, dir < 0? "-" : "+", NULL, NULL, NULL, NULL,  \
     LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_RET_I(0)} ,                     \
        (excmd_t[]){                                    \
            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,        \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 1),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 2),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 3),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 4),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 5),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 6),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 7),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 8),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 9),  \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 10), \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 11), \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 12), \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 13), \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 14), \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 15), \
            GROUP_STEP_ONE(chan_ofs, reg_ofs, dir, 16), \
            CMD_RET                                     \
        }}

#define GROUP_SET_CELL(chan_ofs, reg_ofs)          \
    {NULL, "=", NULL, NULL, NULL, NULL,            \
     LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_RET_I(0)} ,                \
        (excmd_t[]){                               \
            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,   \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 1),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 2),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 3),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 4),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 5),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 6),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 7),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 8),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 9),  \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 10), \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 11), \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 12), \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 13), \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 14), \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 15), \
            GROUP_SETV_ONE(chan_ofs, reg_ofs, 16), \
            CMD_RET                                \
        }}

#define GROUP_CMD_CELL(id, label, chan_ofs)        \
    {id, label, NULL, NULL, NULL, NULL,            \
     LOGT_WRITE1, LOGD_BUTTON, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){CMD_RET_I(0)} ,                \
        (excmd_t[]){                               \
            CMDMULTI_CHECK_IF_REAL_BUTTON_CLICK,   \
            GROUP_SND1_ONE(chan_ofs,          1),  \
            GROUP_SND1_ONE(chan_ofs,          2),  \
            GROUP_SND1_ONE(chan_ofs,          3),  \
            GROUP_SND1_ONE(chan_ofs,          4),  \
            GROUP_SND1_ONE(chan_ofs,          5),  \
            GROUP_SND1_ONE(chan_ofs,          6),  \
            GROUP_SND1_ONE(chan_ofs,          7),  \
            GROUP_SND1_ONE(chan_ofs,          8),  \
            GROUP_SND1_ONE(chan_ofs,          9),  \
            GROUP_SND1_ONE(chan_ofs,          10), \
            GROUP_SND1_ONE(chan_ofs,          11), \
            GROUP_SND1_ONE(chan_ofs,          12), \
            GROUP_SND1_ONE(chan_ofs,          13), \
            GROUP_SND1_ONE(chan_ofs,          14), \
            GROUP_SND1_ONE(chan_ofs,          15), \
            GROUP_SND1_ONE(chan_ofs,          16), \
            CMD_RET                                \
        }}

#define GROUP_DIR_LINE(id, label, chan_ofs, reg_ofs, defval, dpyfmt)   \
    {id "_step", NULL, NULL, dpyfmt, "", NULL,                         \
        LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){                                                   \
            CMD_PUSH_I(defval), CMD_SETLCLREGDEFVAL_I(INCDEC_REGS_base + (reg_ofs)), \
            CMD_GETLCLREG_I(INCDEC_REGS_base + (reg_ofs)),             \
            CMD_RET                                                    \
        },                                                             \
        (excmd_t[]){                                                   \
            CMD_QRYVAL,                                                \
            CMD_SETLCLREG_I(INCDEC_REGS_base + (reg_ofs)),             \
            CMD_RET                                                    \
        }},                                                            \
    GROUP_DIR_CELL(chan_ofs, reg_ofs, -1),                             \
    GROUP_DIR_CELL(chan_ofs, reg_ofs, +1)
    
#define GROUP_SET_LINE(id, label, chan_ofs, reg_ofs, defval, dpyfmt)   \
    {id "_setv", NULL, NULL, dpyfmt, "", NULL,                         \
        LOGT_WRITE1, LOGD_TEXT, LOGK_CALCED, LOGC_NORMAL, PHYS_ID(-1), \
        (excmd_t[]){                                                   \
            CMD_PUSH_I(defval), CMD_SETLCLREGDEFVAL_I(SETVAL_REGS_base + (reg_ofs)), \
            CMD_GETLCLREG_I(SETVAL_REGS_base + (reg_ofs)),             \
            CMD_RET                                                    \
        },                                                             \
        (excmd_t[]){                                                   \
            CMD_QRYVAL,                                                \
            CMD_SETLCLREG_I(SETVAL_REGS_base + (reg_ofs)),             \
            CMD_RET                                                    \
        }},                                                            \
    GROUP_SET_CELL(chan_ofs, reg_ofs),                                 \
    EMPTY_CELL()
    


/* ========================================================================== */

static groupunit_t v5r_bpm_cfg_grouping[] =
{
    GLOBELEM_START_CRN("v5r", "VEPP5 BPM system cofiguration", "", "", 16, 4)
        #define BPM_ID 1
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 2
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 3
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 4
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 5
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 6
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 7
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 8
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 9
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 10
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 11
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 12
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 13
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 14
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 15
        #include "v5r_bpm_cfg_elem.h"
        ,
        #define BPM_ID 16
        #include "v5r_bpm_cfg_elem.h"
    GLOBELEM_END("notitle,noshadow", 1),

    GLOBELEM_START_CRN("common", "Group",
                       "NumPts\v\v\vPtsOfs\v\v\vDecmtn\v\v\vDelay\v\v\vAtten\v\v", NULL,
                       2*5*3, 5*3)
        GROUP_DIR_LINE("numpts", "NumPts", BPMD_CFG_CHAN_NUMPTS, 0, 1,    "%5.0f"),
        GROUP_DIR_LINE("ptsofs", "PtsOfs", BPMD_CFG_CHAN_PTSOFS, 1, 1,    "%5.0f"),
        GROUP_DIR_LINE("decmtn", "Decmtn", BPMD_CFG_CHAN_DECMTN, 2, 1,    "%5.0f"),
        GROUP_DIR_LINE("delay",  "Delay",  BPMD_CFG_CHAN_DELAY,  3, 1,    "%5.3f"),
        GROUP_DIR_LINE("atten",  "Atten",  BPMD_CFG_CHAN_ATTEN,  4, 1,    "%5.0f"),
        GROUP_SET_LINE("numpts", "NumPts", BPMD_CFG_CHAN_NUMPTS, 0, 1024, "%5.0f"),
        GROUP_SET_LINE("ptsofs", "PtsOfs", BPMD_CFG_CHAN_PTSOFS, 1, 0,    "%5.0f"),
        GROUP_SET_LINE("decmtn", "Decmtn", BPMD_CFG_CHAN_DECMTN, 2, 1,    "%5.0f"),
        GROUP_SET_LINE("delay",  "Delay",  BPMD_CFG_CHAN_DELAY,  3, 10,   "%5.3f"),
        GROUP_SET_LINE("atten",  "Atten",  BPMD_CFG_CHAN_ATTEN,  4, 7,    "%5.0f"),
    GLOBELEM_END("noshadow,norowtitles", 1),

    GLOBELEM_START("commmode", "Modes",
                   4*3, 4)
        GROUP_CMD_CELL("e_run", "E_RUN", BPMD_CFG_CHAN_REGIME_ELCTRUN),
        GROUP_CMD_CELL("e_inj", "E_INJ", BPMD_CFG_CHAN_REGIME_ELCTINJ),
        GROUP_CMD_CELL("p_run", "P_RUN", BPMD_CFG_CHAN_REGIME_PSTRRUN),
        GROUP_CMD_CELL("p_inj", "P_INJ", BPMD_CFG_CHAN_REGIME_PSTRINJ),
        GROUP_CMD_CELL("free",  "FREE",  BPMD_CFG_CHAN_REGIME_FREERUN),
        EMPTY_CELL    (),
        EMPTY_CELL    (),
        EMPTY_CELL    (),
        GROUP_CMD_CELL("save",  "Save",  BPMD_CFG_CHAN_REGIME_MSAVE),
        EMPTY_CELL    (),
        EMPTY_CELL    (),
        EMPTY_CELL    (),
    GLOBELEM_END("noshadow,nocoltitles,norowtitles", 0),

    {NULL}
};

static physprops_t v5r_bpm_cfg_physinfo[] =
{
    {DELAY_CHAN(1),  1.e+3, 0, 0},
    {DELAY_CHAN(2),  1.e+3, 0, 0},
    {DELAY_CHAN(3),  1.e+3, 0, 0},
    {DELAY_CHAN(4),  1.e+3, 0, 0},
    {DELAY_CHAN(5),  1.e+3, 0, 0},
    {DELAY_CHAN(6),  1.e+3, 0, 0},
    {DELAY_CHAN(7),  1.e+3, 0, 0},
    {DELAY_CHAN(8),  1.e+3, 0, 0},
    {DELAY_CHAN(9),  1.e+3, 0, 0},
    {DELAY_CHAN(10), 1.e+3, 0, 0},
    {DELAY_CHAN(11), 1.e+3, 0, 0},
    {DELAY_CHAN(12), 1.e+3, 0, 0},
    {DELAY_CHAN(13), 1.e+3, 0, 0},
    {DELAY_CHAN(14), 1.e+3, 0, 0},
    {DELAY_CHAN(15), 1.e+3, 0, 0},
    {DELAY_CHAN(16), 1.e+3, 0, 0},
};

/* ========================================================================== */

#if 0
enum
{
    COMMON_REGS_BASE = 0,
    REGS_NUM_FOR_BPM = 20,

#define DEFINE_LREGS_FOR_BPM(n) \
    __CX_CONCATENATE(REG_BPM, __CX_CONCATENATE(n, _REGIME)) = COMMON_REGS_BASE + REGS_NUM_FOR_BPM * n + 0, \
    __CX_CONCATENATE(REG_BPM, __CX_CONCATENATE(n, _NUMPTS)) = COMMON_REGS_BASE + REGS_NUM_FOR_BPM * n + 1, \
    __CX_CONCATENATE(REG_BPM, __CX_CONCATENATE(n, _PTSOFS)) = COMMON_REGS_BASE + REGS_NUM_FOR_BPM * n + 2, \
    __CX_CONCATENATE(REG_BPM, __CX_CONCATENATE(n, _DELAY )) = COMMON_REGS_BASE + REGS_NUM_FOR_BPM * n + 3, \
    __CX_CONCATENATE(REG_BPM, __CX_CONCATENATE(n, _ATTEN )) = COMMON_REGS_BASE + REGS_NUM_FOR_BPM * n + 4

    DEFINE_LREGS_FOR_BPM(1),
    DEFINE_LREGS_FOR_BPM(2),
    DEFINE_LREGS_FOR_BPM(3),
    DEFINE_LREGS_FOR_BPM(4),
};
#endif
