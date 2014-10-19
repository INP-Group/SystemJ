#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/vsdc2_drv_i.h"
#include "vsdc2_defs.h"


enum
{
    DEVTYPE = 0x80
};



#define DESC_RD_REG(n) (n)
#define DESC_WR_REG(n) ((n) | 0x80)

enum
{
    /* Global */
    DESC_RD_GCSR          = DESC_RD_REG(VSDC2_R_GCSR),
    DESC_WR_GCSR          = DESC_WR_REG(VSDC2_R_GCSR),

    /* ADC0 */
    DESC_RD_ADC0_INTEGRAL = DESC_RD_REG(VSDC2_R_ADC0_INTEGRAL),
    
    DESC_RD_ADC0_CSR      = DESC_RD_REG(VSDC2_R_ADC0_CSR),
    DESC_WR_ADC0_CSR      = DESC_WR_REG(VSDC2_R_ADC0_CSR),

    DESC_RD_ADC0_AVG_NUM  = DESC_RD_REG(VSDC2_R_ADC0_AVG_NUM),
    DESC_WR_ADC0_AVG_NUM  = DESC_WR_REG(VSDC2_R_ADC0_AVG_NUM),

    DESC_RD_ADC0_BP_MUX   = DESC_RD_REG(VSDC2_R_ADC0_BP_MUX),
    DESC_WR_ADC0_BP_MUX   = DESC_WR_REG(VSDC2_R_ADC0_BP_MUX),

    /* ADC1 */
    DESC_RD_ADC1_INTEGRAL = DESC_RD_REG(VSDC2_R_ADC1_INTEGRAL),

    DESC_RD_ADC1_CSR      = DESC_RD_REG(VSDC2_R_ADC1_CSR),
    DESC_WR_ADC1_CSR      = DESC_WR_REG(VSDC2_R_ADC1_CSR),

    DESC_RD_ADC1_AVG_NUM  = DESC_RD_REG(VSDC2_R_ADC1_AVG_NUM),
    DESC_WR_ADC1_AVG_NUM  = DESC_WR_REG(VSDC2_R_ADC1_AVG_NUM),

    DESC_RD_ADC1_BP_MUX   = DESC_RD_REG(VSDC2_R_ADC1_BP_MUX),
    DESC_WR_ADC1_BP_MUX   = DESC_WR_REG(VSDC2_R_ADC1_BP_MUX),
};


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;
static rflags_t  ovfl_rflags = CXRF_OVERLOAD;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

    int                  sw_ver;

    int                  ch0start;
    int                  ch0stop;
    int                  ch1start;
    int                  ch1stop;
} privrec_t;

static psp_lkp_t start_stop_lkp[] =
{
    {"start0", VSDC2_ADCn_BP_MUX_START0},
    {"start1", VSDC2_ADCn_BP_MUX_START1},
    {"stop0",  VSDC2_ADCn_BP_MUX_STOP0},
    {"stop1",  VSDC2_ADCn_BP_MUX_STOP1},
    {NULL, 0},
};

static psp_paramdescr_t vsdc2_params[] =
{
    PSP_P_LOOKUP("ch0start", privrec_t, ch0start, VSDC2_ADCn_BP_MUX_START0, start_stop_lkp),
    PSP_P_LOOKUP("ch0stop",  privrec_t, ch0stop,  VSDC2_ADCn_BP_MUX_STOP0,  start_stop_lkp),
    PSP_P_LOOKUP("ch1start", privrec_t, ch1start, VSDC2_ADCn_BP_MUX_START1, start_stop_lkp),
    PSP_P_LOOKUP("ch1stop",  privrec_t, ch1stop,  VSDC2_ADCn_BP_MUX_STOP1,  start_stop_lkp),

    PSP_P_END()
};


static void vsdc2_rst(int devid, void *devptr, int is_a_reset);
static void vsdc2_in(int devid, void *devptr,
                     int desc, int datasize, uint8 *data);

static int vsdc2_init_d(int devid, void *devptr, 
                        int businfocount, int businfo[],
                        const char *auxinfo)
{
  privrec_t *me      = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                vsdc2_rst, vsdc2_in,
                                VSDC2_NUMCHANS * 2,
                                -1);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void vsdc2_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void vsdc2_rst(int   devid    __attribute__((unused)),
                      void *devptr,
                      int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;
  canqelem_t  item;
  uint32      rv;

    if (is_a_reset)
    {
    }

    me->iface->get_dev_ver(me->handle, NULL, &(me->sw_ver), NULL);

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 5;
    
    item.data[0]  = DESC_WR_GCSR;
    rv            = VSDC2_GCSR_AUTOCAL;
    item.data[1]  =  rv        & 0xFF;
    item.data[2]  = (rv >>  8) & 0xFF;
    item.data[3]  = (rv >> 16) & 0xFF;
    item.data[4]  = (rv >> 24) & 0xFF;
    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
    
    item.data[1]  = 0xFC;
    item.data[2]  = 0;
    item.data[3]  = 0x04;
    item.data[4]  = 0;
    item.data[0]  = DESC_WR_ADC0_CSR;
    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
    item.data[0]  = DESC_WR_ADC1_CSR;
    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);

    item.data[1]  = item.data[2] = item.data[3] = item.data[4] = 0;
    item.data[0]  = DESC_WR_ADC0_AVG_NUM;
    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
    item.data[0]  = DESC_WR_ADC1_AVG_NUM;
    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);

    if (me->sw_ver >= 2)
    {
        item.data[0] = DESC_WR_ADC0_BP_MUX;
        item.data[1] = (me->ch0start & 3) | ((me->ch0stop  & 3) << 4);
        item.data[2] = item.data[3] = item.data[4] = 0;
        me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);

        item.data[0] = DESC_WR_ADC1_BP_MUX;
        item.data[1] = (me->ch1start & 3) | ((me->ch1stop  & 3) << 4);
        item.data[2] = item.data[3] = item.data[4] = 0;
        me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
    }
}

static void vsdc2_in(int devid, void *devptr __attribute__((unused)),
                     int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value
  rflags_t     rflags;
  float32      fv;
  int          l;
  canqelem_t   item;
  
  union
  {
      int32   i32;
      float32 f32;
  } intflt;

    switch (desc)
    {
        case DESC_RD_ADC0_CSR:
        case DESC_RD_ADC1_CSR:
            l = (desc == DESC_RD_ADC0_CSR)? 0 : 1;
            
            val = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
            if (val & ((1 << 11) | (1 << 14)))
            {
                val = 0;
                ReturnChanGroup(devid, VSDC2_CHAN_INT0 + l, 1,
                                &val, &ovfl_rflags);
                return;
            }
            
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = (l == 0)? DESC_RD_ADC0_INTEGRAL : DESC_RD_ADC1_INTEGRAL;
            me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
            break;

        case DESC_RD_ADC0_INTEGRAL:
        case DESC_RD_ADC1_INTEGRAL:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);

            l = (desc == DESC_RD_ADC0_INTEGRAL)? 0 : 1;
            
            intflt.i32 = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
            if (intflt.i32 == 0x7f800000) // +NaN => integration isn't finished yet
            {
                val = 0;
                ReturnChanGroup(devid, VSDC2_CHAN_INT0 + l, 1,
                                &val, &ovfl_rflags);
            }
            else
            {
                fv = intflt.f32 * 1e9;
                if      (fv > (float32)(+0x7fffffff))
                {
                    val    = +0x7fffffff;
                    rflags = CXRF_OVERLOAD;
                }
                else if (fv < (float32)(-0x7fffffff))
                {
                    val    = -0x7fffffff;
                    rflags = CXRF_OVERLOAD;
                }
                else
                {
                    val    = (int32)fv; /*!!!trunc(fv);*/
                    rflags = 0;
                }

                ReturnChanGroup(devid, VSDC2_CHAN_INT0 + l, 1,
                                &val, &rflags);
            }
            break;
            
        default:;
    }
}

static void vsdc2_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int          n;       // channel N in values[] (loop ctl var)
  int          x;       // channel indeX
  int32        val;     // Value
  uint8        data[8];
  int          nc;

  canqelem_t   item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x == VSDC2_CHAN_INT0  ||
                 x == VSDC2_CHAN_INT1)
        {
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec vsdc2_main_info[] =
{
};

DEFINE_DRIVER(vsdc2, NULL,
              NULL, NULL,
              sizeof(privrec_t), vsdc2_params,
              2, 3,
              NULL, 0,
              countof(vsdc2_main_info)*0-1, vsdc2_main_info,
              0, NULL,
              vsdc2_init_d, vsdc2_term_d,
              vsdc2_rw_p, NULL);
