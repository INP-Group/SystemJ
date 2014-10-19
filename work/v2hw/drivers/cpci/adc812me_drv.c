#include "cxsd_driver.h"
#include "pzframe_drv.h"
#include "pci4624.h"

#include "drv_i/adc812me_drv_i.h"


enum
{
    ADC812ME_REG_RESERVE_00 = 0x00,
    ADC812ME_REG_PAGE_LEN   = 0x04,
    ADC812ME_REG_CSR        = 0x08,
    ADC812ME_REG_CUR_ADDR   = 0x0C,
    ADC812ME_REG_RESERVE_10 = 0x10,
    ADC812ME_REG_IRQS       = 0x14,
    ADC812ME_REG_RANGES     = 0x18,
    ADC812ME_REG_SERIAL     = 0x1C,
};

enum
{
    ADC812ME_CSR_RUN_shift     = 0,  ADC812ME_CSR_RUN       = 1 << ADC812ME_CSR_RUN_shift,
    ADC812ME_CSR_HLT_shift     = 1,  ADC812ME_CSR_HLT       = 1 << ADC812ME_CSR_HLT_shift,
    ADC812ME_CSR_STM_shift     = 2,  ADC812ME_CSR_STM       = 1 << ADC812ME_CSR_STM_shift,
    ADC812ME_CSR_Fn_shift      = 3,  ADC812ME_CSR_Fn        = 1 << ADC812ME_CSR_Fn_shift,
    ADC812ME_CSR_BUSY_shift    = 4,  ADC812ME_CSR_BUSY      = 1 << ADC812ME_CSR_BUSY_shift,
    ADC812ME_CSR_R5_SHIFT      = 5,
    ADC812ME_CSR_PC_shift      = 6,  ADC812ME_CSR_PC        = 1 << ADC812ME_CSR_PC_shift,
    ADC812ME_CSR_REC_shift     = 7,  ADC812ME_CSR_REC       = 1 << ADC812ME_CSR_REC_shift,
    ADC812ME_CSR_Fdiv_shift    = 8,  ADC812ME_CSR_Fdiv_MASK = 3 << ADC812ME_CSR_Fdiv_shift,
    ADC812ME_CSR_INT_ENA_shift = 16, ADC812ME_CSR_INT_ENA   = 1 << ADC812ME_CSR_INT_ENA_shift,
    ADC812ME_CSR_ZC_shift      = 17, ADC812ME_CSR_ZC        = 1 << ADC812ME_CSR_ZC_shift,
};

enum
{
    FASTADC_VENDOR_ID      = 0x4624,
    FASTADC_DEVICE_ID      = 0xADC2,
    FASTADC_SERIAL_OFFSET  = ADC812ME_REG_SERIAL,
    
    FASTADC_IRQ_CHK_OP     = PCI4624_OP_RD,
    FASTADC_IRQ_CHK_OFS    = ADC812ME_REG_IRQS,
    FASTADC_IRQ_CHK_MSK    = -1,
    FASTADC_IRQ_CHK_VAL    = 0,
    FASTADC_IRQ_CHK_COND   = PCI4624_COND_NONZERO,
    
    FASTADC_IRQ_RST_OP     = PCI4624_OP_WR,
    FASTADC_IRQ_RST_OFS    = ADC812ME_REG_IRQS,
    FASTADC_IRQ_RST_VAL    = 0,
    
    FASTADC_ON_CLS1_OP     = PCI4624_OP_WR,
    FASTADC_ON_CLS1_OFS    = ADC812ME_REG_CSR,
    FASTADC_ON_CLS1_VAL    = ADC812ME_CSR_HLT | ADC812ME_CSR_INT_ENA,
    
    FASTADC_ON_CLS2_OP     = PCI4624_OP_WR,
    FASTADC_ON_CLS2_OFS    = ADC812ME_REG_IRQS,
    FASTADC_ON_CLS2_VAL    = 0
};

/* We allow only as much measurements, not ADC812ME_MAX_NUMPTS  */
enum { ADC812ME_MAX_ALLOWED_NUMPTS = 50000*0 + ADC812ME_MAX_NUMPTS };

typedef struct
{
    pzframe_drv_t     pz;
    int               devid;
    int               handle;
    pci4624_vmt_t    *lvmt;

    int               serial;

    int32             prv_args[ADC812ME_NUM_PARAMS];
    ADC812ME_DATUM_T  retdata [ADC812ME_MAX_ALLOWED_NUMPTS*ADC812ME_NUM_LINES];
    int               do_return;
    int               force_read;

    // This holds CSR value WITHOUT RUN/PC bits, i.e. -- effective SETTINGS only
    int32             cword;
    // This holds either ADC812ME_CSR_RUN or ADC812ME_CSR_PC
    int32             start_mask;

    int               clb_valid;
    int32             clb_Offs[ADC812ME_NUM_LINES];
    int32             clb_DatS[ADC812ME_NUM_LINES];
} adc812me_privrec_t;

#define PDR2ME(pdr) ((adc812me_privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

//--------------------------------------------------------------------

static psp_lkp_t adc812me_timing_lkp[] =
{
    {"4",   ADC812ME_T_4MHZ},
    {"int", ADC812ME_T_4MHZ},
    {"ext", ADC812ME_T_EXT},
    {NULL, 0}
};

static psp_lkp_t adc812me_frqdiv_lkp[] =
{
    {"1",    ADC812ME_FRQD_250NS},
    {"250",  ADC812ME_FRQD_250NS},
    {"2",    ADC812ME_FRQD_500NS},
    {"500",  ADC812ME_FRQD_500NS},
    {"4",    ADC812ME_FRQD_1000NS},
    {"1000", ADC812ME_FRQD_1000NS},
    {"8",    ADC812ME_FRQD_2000NS},
    {"2000", ADC812ME_FRQD_2000NS},
    {NULL, 0}
};

static psp_lkp_t adc812me_range_lkp [] =
{
    {"8192", ADC812ME_R_8V},
    {"4096", ADC812ME_R_4V},
    {"2048", ADC812ME_R_2V},
    {"1024", ADC812ME_R_1V},
    {NULL, 0}
};

static psp_paramdescr_t adc812me_params[] =
{
    PSP_P_INT    ("ptsofs",   adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_PTSOFS],   -1, 0, ADC812ME_MAX_ALLOWED_NUMPTS-1),
    PSP_P_INT    ("numpts",   adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_NUMPTS],   -1, 1, ADC812ME_MAX_ALLOWED_NUMPTS),
    PSP_P_LOOKUP ("timing",   adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_TIMING],   -1, adc812me_timing_lkp),
    PSP_P_LOOKUP ("frqdiv",   adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_FRQDIV],   -1, adc812me_frqdiv_lkp),
    PSP_P_LOOKUP ("range0A",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE0],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range0B",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE1],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range1A",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE2],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range1B",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE3],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range2A",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE4],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range2B",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE5],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range3A",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE6],   -1, adc812me_range_lkp),
    PSP_P_LOOKUP ("range3B",  adc812me_privrec_t, pz.nxt_args[ADC812ME_PARAM_RANGE7],   -1, adc812me_range_lkp),
    PSP_P_END()
};

//--------------------------------------------------------------------

static int32 saved_data_signature[] =
{
    0x00FFFFFF,
    0x00000000,
    0x00123456,
    0x00654321,
};

enum
{
    SAVED_DATA_COUNT  = countof(saved_data_signature) + ADC812ME_NUM_PARAMS,
    SAVED_DATA_OFFSET = (1 << 22) - SAVED_DATA_COUNT * sizeof(int32),
};

static int32 MakeCWord(int TIMING, int FRQDIV)
{
    return
        0 /* RUN=0 */
        |
        0 /* HLT=0 -- sure! */
        |
        ADC812ME_CSR_STM
        |
        ((TIMING == ADC812ME_T_EXT)? ADC812ME_CSR_Fn : 0)
        |
        0 /* PC=0 */
        |
        0 /* REC=0 */
        |
        (FRQDIV << ADC812ME_CSR_Fdiv_shift)
        |
        0 /* INT_ENA=0 */
        ;
}

static void InvalidateClb(adc812me_privrec_t *me)
{
  int  nl;

    me->pz.cur_args[ADC812ME_PARAM_CLB_STATE] =
    me->pz.nxt_args[ADC812ME_PARAM_CLB_STATE] =
        me->clb_valid   = 0;
    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        me->pz.cur_args[ADC812ME_PARAM_CLB_OFS0+nl] =
        me->pz.nxt_args[ADC812ME_PARAM_CLB_OFS0+nl] =
        me->clb_Offs   [                        nl] = 0;

        me->pz.cur_args[ADC812ME_PARAM_CLB_COR0+nl] =
        me->pz.nxt_args[ADC812ME_PARAM_CLB_COR0+nl] =
        me->clb_DatS   [                        nl] = 1775;
    }
}

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int32 v)
{
  adc812me_privrec_t *me = PDR2ME(pdr);

    if      (n == ADC812ME_PARAM_TIMING)
    {
        if (v < ADC812ME_T_4MHZ) v = ADC812ME_T_4MHZ;
        if (v > ADC812ME_T_EXT)  v = ADC812ME_T_EXT;
    }
    else if (n == ADC812ME_PARAM_FRQDIV)
    {
        if (v < ADC812ME_FRQD_250NS)  v = ADC812ME_FRQD_250NS;
        if (v > ADC812ME_FRQD_2000NS) v = ADC812ME_FRQD_2000NS;
    }
    else if (n >= ADC812ME_PARAM_RANGE0  &&  n <= ADC812ME_PARAM_RANGE7)
    {
        if (v < ADC812ME_R_8V) v = ADC812ME_R_8V;
        if (v > ADC812ME_R_1V) v = ADC812ME_R_1V;
    }
    else if (n == ADC812ME_PARAM_PTSOFS)
    {
        if (v < 0)                             v = 0;
        if (v > ADC812ME_MAX_ALLOWED_NUMPTS-1) v = ADC812ME_MAX_ALLOWED_NUMPTS-1;
    }
    else if (n == ADC812ME_PARAM_NUMPTS)
    {
        if (v < 1)                             v = 1;
        if (v > ADC812ME_MAX_ALLOWED_NUMPTS)   v = ADC812ME_MAX_ALLOWED_NUMPTS;
    }
    else if (n == ADC812ME_PARAM_FGT_CLB)
    {
        if (v == CX_VALUE_COMMAND)
            InvalidateClb(me);
        v = 0;
    }
    else if (n >= ADC812ME_PARAM_CLB_OFS0  &&  n <= ADC812ME_PARAM_CLB_COR7)
        v = me->pz.cur_args[n];
    else if (n == ADC812ME_PARAM_CLB_STATE)
        v = me->clb_valid;
    else if (n == ADC812ME_PARAM_SERIAL)
        v = me->serial;

    return v;
}

static void Init1Param(adc812me_privrec_t *me, int n, int32 v)
{
    if (me->pz.nxt_args[n] < 0) me->pz.nxt_args[n] = v;
}
static int   InitParams(pzframe_drv_t *pdr)
{
  adc812me_privrec_t *me = PDR2ME(pdr);

  int                 n;
  int32               saved_data[SAVED_DATA_COUNT];

    /* HLT device first */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR,  ADC812ME_CSR_HLT);
    /* Drop IRQ */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_IRQS, 0);
    /* Read previous-settings-storage area */
    me->lvmt->ReadBar1 (me->handle, SAVED_DATA_OFFSET, saved_data, SAVED_DATA_COUNT);

    /* Try to use previous settings, otherwise use defaults */
    if (memcmp(saved_data, saved_data_signature, sizeof(saved_data_signature)) == 0)
    {
        memcpy(me->pz.nxt_args,
               saved_data + countof(saved_data_signature),
               sizeof(me->pz.nxt_args[0]) * ADC812ME_NUM_PARAMS);
        for (n = 0;  n < ADC812ME_NUM_PARAMS;  n++)
            me->pz.nxt_args[n] = ValidateParam(pdr, n, me->pz.nxt_args[n]);
    }
    else
    {
        Init1Param(me, ADC812ME_PARAM_PTSOFS, 0);
        Init1Param(me, ADC812ME_PARAM_NUMPTS, 1024);
        Init1Param(me, ADC812ME_PARAM_TIMING, ADC812ME_T_4MHZ);
        Init1Param(me, ADC812ME_PARAM_FRQDIV, ADC812ME_FRQD_250NS);
        for (n = 0;  n < ADC812ME_NUM_LINES; n++)
            Init1Param(me, ADC812ME_PARAM_RANGE0 + n, ADC812ME_R_8V);
        me->pz.nxt_args[ADC812ME_PARAM_CALIBRATE] = 0;
    }

    InvalidateClb(me);

    me->pz.retdataunits = sizeof(me->retdata[0]);

    me->cword = MakeCWord(me->pz.nxt_args[ADC812ME_PARAM_TIMING],
                          me->pz.nxt_args[ADC812ME_PARAM_FRQDIV]);
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword);

    me->serial = me->pz.nxt_args[ADC812ME_PARAM_SERIAL] =
        me->lvmt->ReadBar0(me->handle, ADC812ME_REG_SERIAL);

    DoDriverLog(me->devid, 0, "serial=%d", me->serial);
    
    return DEVSTATE_OPERATING;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  adc812me_privrec_t *me = PDR2ME(pdr);

  int                 nl;
  int32               ranges;

    /* Zeroth step: check PTSOFS against NUMPTS */
    if (me->pz.cur_args[ADC812ME_PARAM_PTSOFS] > ADC812ME_MAX_ALLOWED_NUMPTS - me->pz.cur_args[ADC812ME_PARAM_NUMPTS])
        me->pz.cur_args[ADC812ME_PARAM_PTSOFS] = ADC812ME_MAX_ALLOWED_NUMPTS - me->pz.cur_args[ADC812ME_PARAM_NUMPTS];

    /* Check if calibration-affecting parameters had changed */
    if (me->pz.cur_args[ADC812ME_PARAM_TIMING] != me->prv_args[ADC812ME_PARAM_TIMING])
        InvalidateClb(me);
    memcpy(me->prv_args, me->pz.cur_args, sizeof(me->prv_args)); /* Note: countof(prv_args)<countof(pz.cur_args), so sizeof(prv_args) MUST be used */

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[ADC812ME_PARAM_NUMPTS] *
                                         ADC812ME_NUM_LINES     *
                                         ADC812ME_DATAUNITS;

    /* Stop/init the device first */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, ADC812ME_CSR_HLT);
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, 0);

    /* Now program various parameters */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_PAGE_LEN,
                        me->pz.cur_args[ADC812ME_PARAM_NUMPTS] + me->pz.cur_args[ADC812ME_PARAM_PTSOFS]);
    for (nl = 0, ranges = 0;
         nl < ADC812ME_NUM_LINES;
         nl++)
        ranges |= ((me->pz.cur_args[ADC812ME_PARAM_RANGE0 + nl] & 3) << (nl * 2));
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_RANGES, ranges);

    /* And, finally, the CSR */
    me->cword = MakeCWord(me->pz.cur_args[ADC812ME_PARAM_TIMING],
                          me->pz.cur_args[ADC812ME_PARAM_FRQDIV]);
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword);

    if ((me->pz.cur_args[ADC812ME_PARAM_CALIBRATE] & CX_VALUE_LIT_MASK) != 0)
        me->start_mask = ADC812ME_CSR_PC;
    else
        me->start_mask = ADC812ME_CSR_RUN;
    me->pz.nxt_args[ADC812ME_PARAM_CALIBRATE] = 0;

    /* Save settings in a previous-settings-storage area */
    me->lvmt->WriteBar1(me->handle,
                        SAVED_DATA_OFFSET + sizeof(saved_data_signature),
                        me->pz.cur_args,
                        ADC812ME_NUM_PARAMS);
    me->lvmt->WriteBar1(me->handle,
                        SAVED_DATA_OFFSET,
                        saved_data_signature,
                        countof(saved_data_signature));
    
    me->do_return =
        (me->start_mask != ADC812ME_CSR_PC  ||
         (me->pz.cur_args[ADC812ME_PARAM_VISIBLE_CLB] & CX_VALUE_LIT_MASK) != 0);
    me->force_read = me->start_mask == ADC812ME_CSR_PC;

    /* Let's go! */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword | me->start_mask);

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  adc812me_privrec_t *me = PDR2ME(pdr);

    /* Don't even try anything in CALIBRATE mode */
    if (me->start_mask == ADC812ME_CSR_PC) return PZFRAME_R_DOING;

    /* 0. HLT device first (that is safe and don't touch any other registers) */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, ADC812ME_CSR_HLT);

    /* 1. Allow prog-start */
    me->cword &=~ ADC812ME_CSR_STM;
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword);
    /* 2. Do prog-start */
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword | ADC812ME_CSR_RUN);

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  adc812me_privrec_t *me = PDR2ME(pdr);

    me->cword &=~ ADC812ME_CSR_STM;
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword |  ADC812ME_CSR_HLT);
    me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, me->cword);

    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);

    return PZFRAME_R_READY;
}

enum
{
    INT32S_PER_POINT = 4,
    BYTES_PER_POINT  = sizeof(int32) * INT32S_PER_POINT
};

static int32 CalcOneSum(int32 *buf, int block, int nl)
{
  int    cofs = nl / 2;
  int    ab   = nl % 2;
  int32 *rp   = buf + 
                (100 + block * 500) * INT32S_PER_POINT
                + cofs;
  int    x;
  int32  v;

    for (x = 0, v = 0; 
         x < 300;
         x++)
    {
        if (ab == 0)
            v +=  rp[x * INT32S_PER_POINT]        & 0x0FFF;
        else
            v += (rp[x * INT32S_PER_POINT] >> 12) & 0x0FFF;
    }

    return v - 2048*300;
}

static void ReadCalibrations (adc812me_privrec_t *me)
{
  int       nl;

  rflags_t  rflags = 0;

  int32     buf[INT32S_PER_POINT * 1000];

    me->lvmt->ReadBar1(me->handle, 0, buf, countof(buf));

    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        me->clb_Offs[nl] = CalcOneSum(buf, 0, nl) / 300;
        me->clb_DatS[nl] = CalcOneSum(buf, 1, nl) / 300;
        //fprintf(stderr, "%d: %d,%d\n", nl, me->clb_Offs[nl], me->clb_DatS[nl]);
        if (me->clb_DatS[nl] == 0)
            DoDriverLog(me->devid, DRIVERLOG_ERR,
                        "BAD CALIBRATION sum[%d]=%d",
                        nl, me->clb_DatS[nl]);
    }

    me->pz.cur_args[ADC812ME_PARAM_CLB_STATE] = me->pz.nxt_args[ADC812ME_PARAM_CLB_STATE] =
        me->clb_valid = 1;
    ReturnChanGroup(me->devid,        ADC812ME_PARAM_CLB_STATE, 1,
                    me->pz.cur_args + ADC812ME_PARAM_CLB_STATE, &rflags);
    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        me->pz.cur_args[ADC812ME_PARAM_CLB_OFS0+nl] =
        me->pz.nxt_args[ADC812ME_PARAM_CLB_OFS0+nl] =
        me->clb_Offs   [                        nl];
        ReturnChanGroup(me->devid,        ADC812ME_PARAM_CLB_OFS0+nl, 1,
                        me->pz.cur_args + ADC812ME_PARAM_CLB_OFS0+nl, &rflags);

        me->pz.cur_args[ADC812ME_PARAM_CLB_COR0+nl] =
        me->pz.nxt_args[ADC812ME_PARAM_CLB_COR0+nl] =
        me->clb_DatS   [                        nl];
        ReturnChanGroup(me->devid,        ADC812ME_PARAM_CLB_COR0+nl, 1,
                        me->pz.cur_args + ADC812ME_PARAM_CLB_COR0+nl, &rflags);
    }
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  adc812me_privrec_t *me = PDR2ME(pdr);

  int                 nl;
  int                 clb_nz;
  ADC812ME_DATUM_T   *wp[ADC812ME_NUM_LINES];
  int32               q [ADC812ME_NUM_LINES];
  int                 ofs;
  int                 n;
  int                 count;
  int                 x;
  int32              *rp;
  int32               rd;
  
  int32               csr;

  int32               buf[INT32S_PER_POINT * 1024]; // Array count must be multiple of 4; 1000%4==0

  static int32        quants[4] = {4000, 2000, 1000, 500};

    ////me->lvmt->WriteBar0(me->handle, ADC812ME_REG_CSR, 0);
    csr = me->lvmt->ReadBar0(me->handle, ADC812ME_REG_CSR);

    /* Was it a calibration? */
    if (me->start_mask == ADC812ME_CSR_PC)
        ReadCalibrations(me);

    for (nl = 0;  nl < ADC812ME_NUM_LINES;  nl++)
    {
        wp[nl] = me->retdata + me->pz.cur_args[ADC812ME_PARAM_NUMPTS] * nl;
        q [nl] = quants[me->pz.cur_args[ADC812ME_PARAM_RANGE0 + nl] & 3];
    }
    ofs = me->pz.cur_args[ADC812ME_PARAM_PTSOFS] * BYTES_PER_POINT;

    /* Ensure that none of sums ==0 */
    for (nl = 0, clb_nz = 1;  nl < ADC812ME_NUM_LINES;  nl++)
        if (me->clb_DatS[nl] == 0) clb_nz = 0;

    for (n = me->pz.cur_args[ADC812ME_PARAM_NUMPTS];
         n > 0;
         n -= count, ofs += count * BYTES_PER_POINT)
    {
        count = n;
        if (count > countof(buf) / INT32S_PER_POINT)
            count = countof(buf) / INT32S_PER_POINT;

        me->lvmt->ReadBar1(me->handle, ofs, buf, count * INT32S_PER_POINT);
        /* Do NOT try to apply calibration-correction to calibration data */
        if (me->clb_valid  &&  me->start_mask != ADC812ME_CSR_PC  &&
            clb_nz)
            for (x = 0, rp = buf;  x < count;  x++)
            {
                rd = *rp++;
                *(wp[0]++) = (( rd        & 0x0FFF) - 2048 - me->clb_Offs[0])
                             * 1775 / me->clb_DatS[0]
                             * q[0];
                *(wp[1]++) = (((rd >> 12) & 0x0FFF) - 2048 - me->clb_Offs[1])
                             * 1775 / me->clb_DatS[1]
                             * q[1];
                rd = *rp++;
                *(wp[2]++) = (( rd        & 0x0FFF) - 2048 - me->clb_Offs[2])
                             * 1775 / me->clb_DatS[2]
                             * q[2];
                *(wp[3]++) = (((rd >> 12) & 0x0FFF) - 2048 - me->clb_Offs[3])
                             * 1775 / me->clb_DatS[3]
                             * q[3];
                rd = *rp++;
                *(wp[4]++) = (( rd        & 0x0FFF) - 2048 - me->clb_Offs[4])
                             * 1775 / me->clb_DatS[4]
                             * q[4];
                *(wp[5]++) = (((rd >> 12) & 0x0FFF) - 2048 - me->clb_Offs[5])
                             * 1775 / me->clb_DatS[5]
                             * q[5];
                rd = *rp++;
                *(wp[6]++) = (( rd        & 0x0FFF) - 2048 - me->clb_Offs[6])
                             * 1775 / me->clb_DatS[6]
                             * q[6];
                *(wp[7]++) = (((rd >> 12) & 0x0FFF) - 2048 - me->clb_Offs[7])
                             * 1775 / me->clb_DatS[7]
                             * q[7];
            }
        else
            for (x = 0, rp = buf;  x < count;  x++)
            {
                rd = *rp++;
                *(wp[0]++) = (( rd        & 0x0FFF) - 2048) * q[0];
                *(wp[1]++) = (((rd >> 12) & 0x0FFF) - 2048) * q[1];
                rd = *rp++;
                *(wp[2]++) = (( rd        & 0x0FFF) - 2048) * q[2];
                *(wp[3]++) = (((rd >> 12) & 0x0FFF) - 2048) * q[3];
                rd = *rp++;
                *(wp[4]++) = (( rd        & 0x0FFF) - 2048) * q[4];
                *(wp[5]++) = (((rd >> 12) & 0x0FFF) - 2048) * q[5];
                rd = *rp++;
                *(wp[6]++) = (( rd        & 0x0FFF) - 2048) * q[6];
                *(wp[7]++) = (((rd >> 12) & 0x0FFF) - 2048) * q[7];
            }
    }
    
    ////fprintf(stderr, "CSR=%d\n", csr);

    return 0;
}


enum
{
    PARAM_SHOT     = ADC812ME_PARAM_SHOT,
    PARAM_ISTART   = ADC812ME_PARAM_ISTART,
    PARAM_WAITTIME = ADC812ME_PARAM_WAITTIME,
    PARAM_STOP     = ADC812ME_PARAM_STOP,
    PARAM_ELAPSED  = ADC812ME_PARAM_ELAPSED,

    NUM_PARAMS     = ADC812ME_NUM_PARAMS,
};

#define FASTADC_NAME adc812me
#include "cpci_fastadc_common.h"
