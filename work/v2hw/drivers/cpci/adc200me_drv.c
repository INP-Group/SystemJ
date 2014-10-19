#include "cxsd_driver.h"
#include "pzframe_drv.h"
#include "pci4624.h"

#include "drv_i/adc200me_drv_i.h"


enum
{
    ADC200ME_REG_START_ADDR = 0x00,
    ADC200ME_REG_PAGE_LEN   = 0x04,
    ADC200ME_REG_CSR        = 0x08,
    ADC200ME_REG_CUR_ADDR   = 0x0C,
    ADC200ME_REG_PAGE_NUM   = 0x10,
    ADC200ME_REG_IRQS       = 0x14,
    ADC200ME_REG_RESERVE_18 = 0x18,
    ADC200ME_REG_SERIAL     = 0x1C,
};

enum
{
    ADC200ME_CSR_RUN_shift     = 0,  ADC200ME_CSR_RUN     = 1 << ADC200ME_CSR_RUN_shift,
    ADC200ME_CSR_HLT_shift     = 1,  ADC200ME_CSR_HLT     = 1 << ADC200ME_CSR_HLT_shift,
    ADC200ME_CSR_STM_shift     = 2,  ADC200ME_CSR_STM     = 1 << ADC200ME_CSR_STM_shift,
    ADC200ME_CSR_Fn_shift      = 3,  ADC200ME_CSR_Fn      = 1 << ADC200ME_CSR_Fn_shift,
    ADC200ME_CSR_BUSY_shift    = 4,  ADC200ME_CSR_BUSY    = 1 << ADC200ME_CSR_BUSY_shift,
    ADC200ME_CSR_PM_shift      = 5,  ADC200ME_CSR_PM      = 1 << ADC200ME_CSR_PM_shift,
    ADC200ME_CSR_PC_shift      = 6,  ADC200ME_CSR_PC      = 1 << ADC200ME_CSR_PC_shift,
    ADC200ME_CSR_REC_shift     = 7,  ADC200ME_CSR_REC     = 1 << ADC200ME_CSR_REC_shift,
    ADC200ME_CSR_A_shift       = 8,  ADC200ME_CSR_A_MASK  = 3 << ADC200ME_CSR_A_shift,
    ADC200ME_CSR_B_shift       = 10, ADC200ME_CSR_B_MASK  = 3 << ADC200ME_CSR_B_shift,
    ADC200ME_CSR_ERR_LN_shift  = 12, ADC200ME_CSR_ERR_LN  = 1 << ADC200ME_CSR_ERR_LN_shift,
    ADC200ME_CSR_ERR_PN_shift  = 13, ADC200ME_CSR_ERR_PN  = 1 << ADC200ME_CSR_ERR_PN_shift,
    ADC200ME_CSR_ERR_PC_shift  = 14, ADC200ME_CSR_ERR_PC  = 1 << ADC200ME_CSR_ERR_PC_shift,
    ADC200ME_CSR_ERR_OF_shift  = 15, ADC200ME_CSR_ERR_OF  = 1 << ADC200ME_CSR_ERR_OF_shift,
    ADC200ME_CSR_INT_ENA_shift = 16, ADC200ME_CSR_INT_ENA = 1 << ADC200ME_CSR_INT_ENA_shift,
};

enum
{
    FASTADC_VENDOR_ID      = 0x4624,
    FASTADC_DEVICE_ID      = 0xADC1,
    FASTADC_SERIAL_OFFSET  = ADC200ME_REG_SERIAL,
    
    FASTADC_IRQ_CHK_OP     = PCI4624_OP_RD,
    FASTADC_IRQ_CHK_OFS    = ADC200ME_REG_IRQS,
    FASTADC_IRQ_CHK_MSK    = -1,
    FASTADC_IRQ_CHK_VAL    = 0,
    FASTADC_IRQ_CHK_COND   = PCI4624_COND_NONZERO,
    
    FASTADC_IRQ_RST_OP     = PCI4624_OP_WR,
    FASTADC_IRQ_RST_OFS    = ADC200ME_REG_IRQS,
    FASTADC_IRQ_RST_VAL    = 0,
    
    FASTADC_ON_CLS1_OP     = PCI4624_OP_WR,
    FASTADC_ON_CLS1_OFS    = ADC200ME_REG_CSR,
    FASTADC_ON_CLS1_VAL    = ADC200ME_CSR_HLT | ADC200ME_CSR_INT_ENA,
    
    FASTADC_ON_CLS2_OP     = PCI4624_OP_WR,
    FASTADC_ON_CLS2_OFS    = ADC200ME_REG_IRQS,
    FASTADC_ON_CLS2_VAL    = 0
};

/* We allow only as much measurements, not ADC200ME_MAX_NUMPTS  */
enum { ADC200ME_MAX_ALLOWED_NUMPTS = 200000*0 + ADC200ME_MAX_NUMPTS };

typedef struct
{
    pzframe_drv_t     pz;
    int               devid;
    int               handle;
    pci4624_vmt_t    *lvmt;

    int               timing_alwd;

    int               serial;

    int32             prv_args[ADC200ME_NUM_PARAMS];
    ADC200ME_DATUM_T  retdata [ADC200ME_MAX_ALLOWED_NUMPTS*ADC200ME_NUM_LINES];
    int               do_return;
    int               force_read;

    // This holds CSR value WITHOUT RUN/PC bits, i.e. -- effective SETTINGS only
    int32             cword;
    // This holds either ADC200ME_CSR_RUN or ADC200ME_CSR_PC
    int32             start_mask;

    int               clb_valid;
    int32             clb_Offs[2];
    int32             clb_DatS[2];
} adc200me_privrec_t;

#define PDR2ME(pdr) ((adc200me_privrec_t*)pdr) //!!! Should better subtract offsetof(pz)

//--------------------------------------------------------------------

static psp_lkp_t adc200me_timing_lkp[] =
{
    {"200", ADC200ME_T_200MHZ},
    {"int", ADC200ME_T_200MHZ},
    {"ext", ADC200ME_T_EXT},
    {NULL, 0}
};

static psp_lkp_t adc200me_range_lkp [] =
{
    {"4096", ADC200ME_R_4V},
    {"2048", ADC200ME_R_2V},
    {"1024", ADC200ME_R_1V},
    {"512",  ADC200ME_R_05V},
    {NULL, 0}
};

static psp_paramdescr_t adc200me_params[] =
{
    PSP_P_INT   ("ptsofs",   adc200me_privrec_t, pz.nxt_args[ADC200ME_PARAM_PTSOFS], -1, 0, ADC200ME_MAX_ALLOWED_NUMPTS-1),
    PSP_P_INT   ("numpts",   adc200me_privrec_t, pz.nxt_args[ADC200ME_PARAM_NUMPTS], -1, 1, ADC200ME_MAX_ALLOWED_NUMPTS),
    PSP_P_LOOKUP("timing",   adc200me_privrec_t, pz.nxt_args[ADC200ME_PARAM_TIMING], -1, adc200me_timing_lkp),
    PSP_P_LOOKUP("rangeA",   adc200me_privrec_t, pz.nxt_args[ADC200ME_PARAM_RANGE1], -1, adc200me_range_lkp),
    PSP_P_LOOKUP("rangeB",   adc200me_privrec_t, pz.nxt_args[ADC200ME_PARAM_RANGE2], -1, adc200me_range_lkp),

    PSP_P_FLAG("notiming", adc200me_privrec_t, timing_alwd, 0, 1),
    PSP_P_FLAG("timing",   adc200me_privrec_t, timing_alwd, 1, 0),
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
    SAVED_DATA_COUNT  = countof(saved_data_signature) + ADC200ME_NUM_PARAMS,
    SAVED_DATA_OFFSET = (1 << 22) - SAVED_DATA_COUNT * sizeof(int32),
};

static int32 MakeCWord(int TIMING, int RANGE1, int RANGE2)
{
    return
        0 /* RUN=0 */
        |
        0 /* HLT=0 -- sure! */
        |
        ADC200ME_CSR_STM
        |
        ((TIMING == ADC200ME_T_EXT)? ADC200ME_CSR_Fn : 0)
        |
        0 /* PM=0 */
        |
        0 /* PC=0 */
        |
        0 /* REC=0 */
        |
        (RANGE1 << ADC200ME_CSR_A_shift)
        |
        (RANGE2 << ADC200ME_CSR_B_shift)
        |
        0 /* INT_ENA=0 */
        ;
}

static void InvalidateClb(adc200me_privrec_t *me)
{
  int  nl;

    me->pz.cur_args[ADC200ME_PARAM_CLB_STATE] = 
    me->pz.nxt_args[ADC200ME_PARAM_CLB_STATE] = 
        me->clb_valid   = 0;
    for (nl = 0;  nl < ADC200ME_NUM_LINES;  nl++)
    {
        me->pz.cur_args[ADC200ME_PARAM_CLB_OFS1+nl] =
        me->pz.nxt_args[ADC200ME_PARAM_CLB_OFS1+nl] =
        me->clb_Offs   [                        nl] = 0;

        me->pz.cur_args[ADC200ME_PARAM_CLB_COR1+nl] =
        me->pz.nxt_args[ADC200ME_PARAM_CLB_COR1+nl] =
        me->clb_DatS   [                        nl] = 3900;
    }
}

static int32 ValidateParam(pzframe_drv_t *pdr, int n, int32 v)
{
  adc200me_privrec_t *me = PDR2ME(pdr);

    if      (n == ADC200ME_PARAM_TIMING)
    {
        if (v < ADC200ME_T_200MHZ) v = ADC200ME_T_200MHZ;
        if (v > ADC200ME_T_EXT)    v = ADC200ME_T_EXT;
        if (!(me->timing_alwd))    v = ADC200ME_T_200MHZ;
    }
    else if (n == ADC200ME_PARAM_RANGE1  ||  n == ADC200ME_PARAM_RANGE2)
    {
        if (v < ADC200ME_R_4V)  v = ADC200ME_R_4V;
        if (v > ADC200ME_R_05V) v = ADC200ME_R_05V;
    }
    else if (n == ADC200ME_PARAM_PTSOFS)
    {
        if (v < 0)                             v = 0;
        if (v > ADC200ME_MAX_ALLOWED_NUMPTS-1) v = ADC200ME_MAX_ALLOWED_NUMPTS-1;
    }
    else if (n == ADC200ME_PARAM_NUMPTS)
    {
        if (v < 1)                             v = 1;
        if (v > ADC200ME_MAX_ALLOWED_NUMPTS)   v = ADC200ME_MAX_ALLOWED_NUMPTS;
    }
    else if (n == ADC200ME_PARAM_FGT_CLB)
    {
        if (v == CX_VALUE_COMMAND)
            InvalidateClb(me);
        v = 0;
    }
    else if (n >= ADC200ME_PARAM_CLB_OFS1  &&  n <= ADC200ME_PARAM_CLB_COR2)
        v = me->pz.cur_args[n];
    else if (n == ADC200ME_PARAM_CLB_STATE)
        v = me->clb_valid;
    else if (n == ADC200ME_PARAM_SERIAL)
        v = me->serial;

    return v;
}

static void Init1Param(adc200me_privrec_t *me, int n, int32 v)
{
    if (me->pz.nxt_args[n] < 0) me->pz.nxt_args[n] = v;
}
static int   InitParams(pzframe_drv_t *pdr)
{
  adc200me_privrec_t *me = PDR2ME(pdr);

  int                 n;
  int32               saved_data[SAVED_DATA_COUNT];

    /* HLT device first */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR,  ADC200ME_CSR_HLT);
    /* Drop IRQ */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_IRQS, 0);
    /* Read previous-settings-storage area */
    me->lvmt->ReadBar1 (me->handle, SAVED_DATA_OFFSET, saved_data, SAVED_DATA_COUNT);

    /* Try to use previous settings, otherwise use defaults */
    if (memcmp(saved_data, saved_data_signature, sizeof(saved_data_signature)) == 0)
    {
        memcpy(me->pz.nxt_args,
               saved_data + countof(saved_data_signature),
               sizeof(me->pz.nxt_args[0]) * ADC200ME_NUM_PARAMS);
        for (n = 0;  n < ADC200ME_NUM_PARAMS;  n++)
            me->pz.nxt_args[n] = ValidateParam(pdr, n, me->pz.nxt_args[n]);
    }
    else
    {
        Init1Param(me, ADC200ME_PARAM_PTSOFS, 0);
        Init1Param(me, ADC200ME_PARAM_NUMPTS, 1024);
        Init1Param(me, ADC200ME_PARAM_TIMING, ADC200ME_T_200MHZ);
        Init1Param(me, ADC200ME_PARAM_RANGE1, ADC200ME_R_4V);
        Init1Param(me, ADC200ME_PARAM_RANGE2, ADC200ME_R_4V);
        me->pz.nxt_args[ADC200ME_PARAM_CALIBRATE] = 0;
    }

    InvalidateClb(me);

    me->pz.retdataunits = sizeof(me->retdata[0]);

    me->cword = MakeCWord(me->pz.nxt_args[ADC200ME_PARAM_TIMING],
                          me->pz.nxt_args[ADC200ME_PARAM_RANGE1],
                          me->pz.nxt_args[ADC200ME_PARAM_RANGE2]);
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword);

    me->serial = me->pz.nxt_args[ADC200ME_PARAM_SERIAL] =
        me->lvmt->ReadBar0(me->handle, ADC200ME_REG_SERIAL);

    DoDriverLog(me->devid, 0, "serial=%d", me->serial);
    
    return DEVSTATE_OPERATING;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  adc200me_privrec_t *me = PDR2ME(pdr);

    /* Zeroth step: check PTSOFS against NUMPTS */
    if (me->pz.cur_args[ADC200ME_PARAM_PTSOFS] > ADC200ME_MAX_ALLOWED_NUMPTS - me->pz.cur_args[ADC200ME_PARAM_NUMPTS])
        me->pz.cur_args[ADC200ME_PARAM_PTSOFS] = ADC200ME_MAX_ALLOWED_NUMPTS - me->pz.cur_args[ADC200ME_PARAM_NUMPTS];

    /* Check if calibration-affecting parameters had changed */
    if (me->pz.cur_args[ADC200ME_PARAM_TIMING] != me->prv_args[ADC200ME_PARAM_TIMING]  ||
        me->pz.cur_args[ADC200ME_PARAM_RANGE1] != me->prv_args[ADC200ME_PARAM_RANGE1]  ||
        me->pz.cur_args[ADC200ME_PARAM_RANGE2] != me->prv_args[ADC200ME_PARAM_RANGE2])
        InvalidateClb(me);
    memcpy(me->prv_args, me->pz.cur_args, sizeof(me->prv_args)); /* Note: countof(prv_args)<countof(pz.cur_args), so sizeof(prv_args) MUST be used */

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[ADC200ME_PARAM_NUMPTS] *
                                         ADC200ME_NUM_LINES     *
                                         ADC200ME_DATAUNITS;

    /* Stop/init the device first */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, ADC200ME_CSR_HLT);
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, 0);

    /* Now program various parameters */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_START_ADDR, 0);
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_PAGE_LEN,
                        me->pz.cur_args[ADC200ME_PARAM_NUMPTS] + me->pz.cur_args[ADC200ME_PARAM_PTSOFS]);
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_PAGE_NUM,   1);

    /* And, finally, the CSR */
    me->cword = MakeCWord(me->pz.cur_args[ADC200ME_PARAM_TIMING],
                          me->pz.cur_args[ADC200ME_PARAM_RANGE1],
                          me->pz.cur_args[ADC200ME_PARAM_RANGE2]);
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword);

    if ((me->pz.cur_args[ADC200ME_PARAM_CALIBRATE] & CX_VALUE_LIT_MASK) != 0)
        me->start_mask = ADC200ME_CSR_PC;
    else
        me->start_mask = ADC200ME_CSR_RUN;
    me->pz.nxt_args[ADC200ME_PARAM_CALIBRATE] = 0;

    /* Save settings in a previous-settings-storage area */
    me->lvmt->WriteBar1(me->handle,
                        SAVED_DATA_OFFSET + sizeof(saved_data_signature),
                        me->pz.cur_args,
                        ADC200ME_NUM_PARAMS);
    me->lvmt->WriteBar1(me->handle,
                        SAVED_DATA_OFFSET,
                        saved_data_signature,
                        countof(saved_data_signature));

    me->do_return =
        (me->start_mask != ADC200ME_CSR_PC  ||
         (me->pz.cur_args[ADC200ME_PARAM_VISIBLE_CLB] & CX_VALUE_LIT_MASK) != 0);
    me->force_read = me->start_mask == ADC200ME_CSR_PC;
    
    /* Let's go! */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword | me->start_mask);

    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  adc200me_privrec_t *me = PDR2ME(pdr);

    /* Don't even try anything in CALIBRATE mode */
    if (me->start_mask == ADC200ME_CSR_PC) return PZFRAME_R_DOING;

    /* 0. HLT device first (that is safe and don't touch any other registers) */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, ADC200ME_CSR_HLT);

    /* 1. Allow prog-start */
    me->cword &=~ ADC200ME_CSR_STM;
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword);
    /* 2. Do prog-start */
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword | ADC200ME_CSR_RUN);

    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  adc200me_privrec_t *me = PDR2ME(pdr);

    me->cword &=~ ADC200ME_CSR_STM;
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword |  ADC200ME_CSR_HLT);
    me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, me->cword);

    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);

    return PZFRAME_R_READY;
}

static int32 ReadOneCalibr(adc200me_privrec_t *me, int block, int ab)
{
  int32             buf[1024];
  int               x;
  int32             v;
  
    me->lvmt->ReadBar1(me->handle, sizeof(int32)*1024*block, buf, 1024);

    for (x = 0, v = 0;  x < 1024;  x++)
    {
        if (ab == 0)
            v +=  buf[x]        & 0x0FFF;
        else
            v += (buf[x] >> 12) & 0x0FFF;
    }

    return v - 2048*1024;
}

static void ReadCalibrations (adc200me_privrec_t *me)
{
  int       nl;

  int32     DatM[2];
  int32     DatP[2];

  rflags_t  rflags = 0;

    me->clb_Offs[0] = ReadOneCalibr(me, 0, 0) / 1024;
    me->clb_Offs[1] = ReadOneCalibr(me, 0, 1) / 1024;

    DatM[0]         = ReadOneCalibr(me, 1, 0);
    DatP[0]         = ReadOneCalibr(me, 2, 0);
    DatM[1]         = ReadOneCalibr(me, 3, 1);
    DatP[1]         = ReadOneCalibr(me, 4, 1);
    me->clb_DatS[0] = (DatP[0] - DatM[0]) / 1024;
    me->clb_DatS[1] = (DatP[1] - DatM[1]) / 1024;

    if (me->clb_DatS[0] == 0  ||  me->clb_DatS[1] == 0)
        DoDriverLog(me->devid, DRIVERLOG_ERR,
                    "BAD CALIBRATION sums: %d,%d",
                    me->clb_DatS[0], me->clb_DatS[1]);

    me->pz.cur_args[ADC200ME_PARAM_CLB_STATE] = me->pz.nxt_args[ADC200ME_PARAM_CLB_STATE] = 
        me->clb_valid = 1;
    ReturnChanGroup(me->devid,        ADC200ME_PARAM_CLB_STATE, 1,
                    me->pz.cur_args + ADC200ME_PARAM_CLB_STATE, &rflags);
    for (nl = 0;  nl < ADC200ME_NUM_LINES;  nl++)
    {
        me->pz.cur_args[ADC200ME_PARAM_CLB_OFS1+nl] =
        me->pz.nxt_args[ADC200ME_PARAM_CLB_OFS1+nl] =
        me->clb_Offs   [                        nl];
        ReturnChanGroup(me->devid,        ADC200ME_PARAM_CLB_OFS1+nl, 1,
                        me->pz.cur_args + ADC200ME_PARAM_CLB_OFS1+nl, &rflags);

        me->pz.cur_args[ADC200ME_PARAM_CLB_COR1+nl] =
        me->pz.nxt_args[ADC200ME_PARAM_CLB_COR1+nl] =
        me->clb_DatS   [                        nl];
        ReturnChanGroup(me->devid,        ADC200ME_PARAM_CLB_COR1+nl, 1,
                        me->pz.cur_args + ADC200ME_PARAM_CLB_COR1+nl, &rflags);
    }

#if 0
    DoDriverLog(DEVID_NOT_IN_DRIVER, 0,
                "NEW CALB: oA=%-6d oB=%-6d sA=%-6d sB=%-6d",
                me->clb_Offs[0], me->clb_Offs[1],
                me->clb_DatS[0], me->clb_DatS[1]);
#endif
}

static rflags_t ReadMeasurements(pzframe_drv_t *pdr)
{
  adc200me_privrec_t *me = PDR2ME(pdr);

  ADC200ME_DATUM_T   *w1;
  ADC200ME_DATUM_T   *w2;
  int32               q1;
  int32               q2;
  int                 ofs;
  int                 n;
  int                 count;
  int                 x;
  
  int32               csr;
  
  int32               buf[1024];

  static int32        quants[4] = {2000, 1000, 500, 250};

    ////me->lvmt->WriteBar0(me->handle, ADC200ME_REG_CSR, 0);
    csr = me->lvmt->ReadBar0(me->handle, ADC200ME_REG_CSR);

    /* Was it a calibration? */
    if (me->start_mask == ADC200ME_CSR_PC)
        ReadCalibrations(me);

    w1  = me->retdata;
    w2  = w1 + me->pz.cur_args[ADC200ME_PARAM_NUMPTS];
    q1  = quants[me->pz.cur_args[ADC200ME_PARAM_RANGE1]];
    q2  = quants[me->pz.cur_args[ADC200ME_PARAM_RANGE2]];
    ofs = me->pz.cur_args[ADC200ME_PARAM_PTSOFS] * sizeof(int32);

    for (n = me->pz.cur_args[ADC200ME_PARAM_NUMPTS];
         n > 0;
         n -= count, ofs += count * sizeof(int32))
    {
        count = n;
        if (count > countof(buf))
            count = countof(buf);

        me->lvmt->ReadBar1(me->handle, ofs, buf, count);
        /* Do NOT try to apply calibration-correction to calibration data */
        if (me->clb_valid  &&  me->start_mask != ADC200ME_CSR_PC  &&
            me->clb_DatS[0] != 0  &&  me->clb_DatS[1] != 0)
            for (x = 0;  x < count;  x++)
            {
                *w1++ = (( buf[x]        & 0x0FFF) - 2048 - me->clb_Offs[0])
                        * 3900 / me->clb_DatS[0]
                        * q1;
                *w2++ = (((buf[x] >> 12) & 0x0FFF) - 2048 - me->clb_Offs[1])
                        * 3900 / me->clb_DatS[1]
                        * q2;
            }
        else
            for (x = 0;  x < count;  x++)
            {
                *w1++ = (( buf[x]        & 0x0FFF) - 2048) * q1;
                *w2++ = (((buf[x] >> 12) & 0x0FFF) - 2048) * q2;
            }
    }

    ////fprintf(stderr, "CSR=%d\n", csr);

    return 0;
}


enum
{
    PARAM_SHOT     = ADC200ME_PARAM_SHOT,
    PARAM_ISTART   = ADC200ME_PARAM_ISTART,
    PARAM_WAITTIME = ADC200ME_PARAM_WAITTIME,
    PARAM_STOP     = ADC200ME_PARAM_STOP,
    PARAM_ELAPSED  = ADC200ME_PARAM_ELAPSED,

    NUM_PARAMS     = ADC200ME_NUM_PARAMS,
};

#define FASTADC_NAME adc200me
#include "cpci_fastadc_common.h"
