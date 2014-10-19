#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/vv2222_drv_i.h"


/*=== VV2222 specifics =============================================*/

enum
{
    DEVTYPE = 38
};

enum
{
    DESC_ERROR         = 0x00,
    DESC_STOP          = 0x00,
    DESC_MULTICHAN     = 0x01,
    DESC_OSCILL        = 0x02,
    DESC_READONECHAN   = 0x03,
    DESC_READRING      = 0x04,
    DESC_WR_DAC_n_BASE = 0x80,
    DESC_RD_DAC_n_BASE = 0x90,
};

enum
{
    ERR_NO       = 0,
    ERR_LENGTH   = 1,
    ERR_WRCMD    = 2,
    ERR_UNKCMD   = 3,
    ERR_CRASH    = 4,
    ERR_CHECKSUM = 5,
    ERR_SEQ      = 6,
    ERR_TIMEOUT  = 7,
    ERR_INTBUFF  = 8,
    ERR_ALLOCMEM = 9,
    ERR_OFFLINE  = 10,
    ERR_TSKRUN   = 11,
    ERR_BLOCK    = 12,
};

enum
{
    MAX_ALWD_VAL =   9999999,
    MIN_ALWD_VAL = -10000305
};

static inline void  decode_attr(uint8 attr, int *chan_p, int *gain_code_p)
{
    *chan_p      = attr & 63;
    *gain_code_p = (attr >> 6) & 3;
}

static rflags_t kozadc_decode_le24(uint8 *bytes,  int    gain_code,
                                   int32 *code_p, int32 *val_p)
{
  int32  code;

  static int ranges[4] =
        {
            10000000 / 1,
            10000000 / 10,
            10000000 / 100,
            10000000 / 1000
        };

    code = bytes[0] | ((uint32)(bytes[1]) << 8) | ((uint32)(bytes[2]) << 16);
    if ((code &  0x800000) != 0)
        code -= 0x1000000;
    
    *code_p = code;
    *val_p  = scale32via64(code, ranges[gain_code], 0x3fffff);
    
    return (code >= -0x3FFFFF && code <= 0x3FFFFF)? 0
                                                  : CXRF_OVERLOAD;

}

static inline int32  kozdac_c16_to_val(uint32 kozak16)
{
    return scale32via64((int32)kozak16 - 0x8000, 10000000, 0x8000);
}

static inline uint32 kozdac_val_to_c16(int32 val)
{
    return scale32via64(val, 0x8000, 10000000) + 0x8000;
}


typedef struct
{
    cankoz_liface_t *iface;
    int              devid;
    int              handle;

} privrec_t;

static psp_paramdescr_t vv2222_params[] =
{
    PSP_P_END()
};

/*=== ZZZ ===*/

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;
static rflags_t  invl_rflags = CXRF_INVAL;

static void SendWrRq(privrec_t *me, int l, int32 val)
{
  uint32      code;    // Code -- in device/Kozak encoding
  canqelem_t  item;
  
    code = kozdac_val_to_c16(val);
    DoDriverLog(me->devid, 0 | DRIVERLOG_C_DATACONV,
                "%s: w=%d => [%d]:=0x%04X", __FUNCTION__, val, l, code);
    
    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 5;
    item.data[0]  = DESC_WR_DAC_n_BASE + l;
    item.data[1]  = (code >> 8) & 0xFF;
    item.data[2]  = code & 0xFF;
    item.data[3]  = 0;
    item.data[4]  = 0;
    me->iface->q_enq_ons(me->handle,
                         &item, QFE_ALWAYS); /*!!! And should REPLACE if such packet is present...  */
}

static void SendRdRq(privrec_t *me, int l)
{
  canqelem_t  item;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 1;
    item.data[0]  = DESC_RD_DAC_n_BASE + l;
    me->iface->q_enqueue(me->handle,
                         &item, QFE_IF_NONEORFIRST);
}
/*=== ZZZ ===*/


static void vv2222_rst(int devid, void *devptr, int is_a_reset);
static void vv2222_in (int devid, void *devptr,
                       int desc, int datasize, uint8 *data);

static int vv2222_init_d(int devid, void *devptr, 
                         int businfocount, int businfo[],
                         const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->devid  = devid;
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                vv2222_rst, vv2222_in,
                                VV2222_NUMCHANS * 2,
                                VV2222_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void vv2222_in (int devid, void *devptr __attribute__((unused)),
                       int desc, int datasize, uint8 *data)
{
  privrec_t  *me       = (privrec_t *) devptr;
  int         l;       // Line #
  int32       code;    // Code -- in device/Kozak encoding
  int32       val;     // Value -- in volts
  rflags_t    rflags;
  int         gain_code;

    switch (desc)
    {
        case DESC_MULTICHAN:
        case DESC_OSCILL:
        case DESC_READONECHAN:
            me->iface->q_erase_and_send_next(me->handle, -1, desc);
            if (datasize < 5) return;
            decode_attr(data[1], &l, &gain_code);
            rflags = kozadc_decode_le24(data + 2, gain_code, &code, &val);
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "in: RCHAN=%-2d code=0x%08X gain=%d val=%duV",
                        l, code, gain_code, val);
            if (l < 0  ||  l >= VV2222_CHAN_ADC_n_COUNT) return;
            ReturnChanGroup(devid,
                            VV2222_CHAN_ADC_n_BASE + l,
                            1, &val, &rflags);
            break;

        case DESC_RD_DAC_n_BASE ... DESC_RD_DAC_n_BASE + VV2222_CHAN_OUT_n_COUNT-1:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 3) return;

            l = desc - DESC_RD_DAC_n_BASE;
            code   = data[1]*256 + data[2];
            val    = kozdac_c16_to_val(code);
            rflags = 0;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "in: WCHAN=%d code=0x%04X val=%duv",
                        l, code, val);
            ReturnChanGroup(devid,
                            VV2222_CHAN_OUT_n_BASE + l,
                            1, &val, &rflags);

            break;

        case DESC_ERROR:
            if (datasize < 3)
            {
                DoDriverLog(devid, 0,
                            "DESC_ERROR: dlc=%d, <3 -- terminating", 
                            datasize);
                SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "DESC_ERROR dlc<3");
                return;
            }
            if (data[2] == ERR_LENGTH)
            {
                DoDriverLog(devid, 0,
                            "DESC_ERROR: ERR_LENGTH of cmd=0x%02x", 
                            data[1]);
                if (data[1] != 0) SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "ERR_LENGTH");
                return;
            }
            if (data[2] == ERR_WRCMD)
            {
                me->iface->q_erase_and_send_next(me->handle, -1, data[1]);
                return;
            }
            if (data[2] == ERR_UNKCMD)
            {
#if 1
                me->iface->q_erase_and_send_next(me->handle, -1, data[1]);
#else
                DoDriverLog(devid, 0,
                            "DESC_ERROR: ERR_UNKCMD cmd=0x%02x", 
                            data[1]);
                SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "ERR_UNKCMD");
#endif
                return;
            }
            break;
    }
}


static void vv2222_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_STOP;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
  
    me->iface->disconnect(devid);
}

static void vv2222_rst(int   devid    __attribute__((unused)),
                       void *devptr,
                       int   is_a_reset)
{
  privrec_t  *me     = (privrec_t *) devptr;

}

static void vv2222_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;     // channel N in values[] (loop ctl var)
  int         x;     // channel indeX
  int         l;     // Line #
  int32       val;   // Value -- in volts
  uint8       data[8];
  canqelem_t  item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x >= VV2222_CHAN_REGS_BASE  &&  x <= VV2222_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else if (x >= VV2222_CHAN_REGS_RSVD_B  &&  x <= VV2222_CHAN_REGS_RSVD_E)
        {
            // Non-existent register channels...
        }
        else if(x >= VV2222_CHAN_ADC_n_BASE  &&
                x <  VV2222_CHAN_ADC_n_BASE + VV2222_CHAN_ADC_n_COUNT)
        {
            l = x - VV2222_CHAN_ADC_n_BASE;
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 2;
            item.data[0]  = DESC_READONECHAN;
            item.data[1]  = l;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_ABSENT);
        }
        else if (x >= VV2222_CHAN_OUT_n_BASE  &&
                 x <  VV2222_CHAN_OUT_n_BASE + VV2222_CHAN_OUT_n_COUNT)
        {
            l = x - VV2222_CHAN_OUT_n_BASE;
            
            if (action == DRVA_WRITE)
            {
                if (val > MAX_ALWD_VAL) val = MAX_ALWD_VAL;
                if (val < MIN_ALWD_VAL) val = MIN_ALWD_VAL;
                SendWrRq(me, l, val);
            }

            /* Perform read anyway */
            SendRdRq(me, l);
        }
    }
}


DEFINE_DRIVER(vv2222, "Panov VV2222 (R,W,In,Out)x2",
              NULL, NULL,
              sizeof(privrec_t), vv2222_params,
              2, 3,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              vv2222_init_d, vv2222_term_d,
              vv2222_rw_p, NULL);
