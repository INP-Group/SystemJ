#include "common_elem_macros.h"

#include "drv_i/bpmd_cfg_drv_i.h"
#include "drv_i/bpmd_drv_i.h"
#include "drv_i/bpmd_txzi_drv_i.h"


#define COMMON_BASE                   (0)

#define BPMD_DRV_OFFSET( bpm_num_id)  (COMMON_BASE + (0  )  + BPMD_NUM_PARAMS      * (bpm_num_id - 1))
#define BPMD_TXZI_OFFSET(bpm_num_id)  (COMMON_BASE + (400)  + BPMD_TXZI_CHAN_COUNT * (bpm_num_id - 1))
#define BPMD_CFG_OFFSET( bpm_num_id)  (COMMON_BASE + (800)  + BPMD_CFG_CHAN_COUNT  * (bpm_num_id - 1))

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

/* ========================================================================== */

static groupunit_t v2k_bpm_cfg_grouping[] =
{
    GLOBELEM_START_CRN("v2k", "VEPP2000 BPM system cofiguration", "", "", 4, 4)
        #define BPM_ID 1
        #include "v2k_bpm_cfg_elem.h"
        ,
        #define BPM_ID 2
        #include "v2k_bpm_cfg_elem.h"
        ,
        #define BPM_ID 3
        #include "v2k_bpm_cfg_elem.h"
        ,
        #define BPM_ID 4
        #include "v2k_bpm_cfg_elem.h"
    GLOBELEM_END("notitle,noshadow", 1),

    {NULL}
};

static physprops_t v2k_bpm_cfg_physinfo[] =
{
    {DELAY_CHAN(1), 1.e+3, 0, 0},
    {DELAY_CHAN(2), 1.e+3, 0, 0},
    {DELAY_CHAN(3), 1.e+3, 0, 0},
    {DELAY_CHAN(4), 1.e+3, 0, 0},

#define coord_scale 1.e+4

    {XMEAN(1),    coord_scale, 0, 0},
    {ZMEAN(1),    coord_scale, 0, 0},
    {XSTDDEV(1),  coord_scale, 0, 0},
    {ZSTDDEV(1),  coord_scale, 0, 0},

    {XMEAN(2),    coord_scale, 0, 0},
    {ZMEAN(2),    coord_scale, 0, 0},
    {XSTDDEV(2),  coord_scale, 0, 0},
    {ZSTDDEV(2),  coord_scale, 0, 0},

    {XMEAN(3),    coord_scale, 0, 0},
    {ZMEAN(3),    coord_scale, 0, 0},
    {XSTDDEV(3),  coord_scale, 0, 0},
    {ZSTDDEV(3),  coord_scale, 0, 0},

    {XMEAN(4),    coord_scale, 0, 0},
    {ZMEAN(4),    coord_scale, 0, 0},
    {XSTDDEV(4),  coord_scale, 0, 0},
    {ZSTDDEV(4),  coord_scale, 0, 0},
};

/* ========================================================================== */

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
