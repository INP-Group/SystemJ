#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/curvv_drv_i.h"


enum
{
    DEVTYPE = 10
};



enum
{
    DESC_READ_PWR_TTL = 0xE8,
    DESC_WRITE_PWR    = 0xE9,
};


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

    struct
    {
        int     rcvd;
        int     pend;
        
        int32   cur_val; /*!!! Note: these 3 should be uint8, and cur_val be copied into int32 for ReturnChan*() */
        int32   req_val;
        int32   req_msk;
        int32   cur_inp;
    }                    ptregs; // 'pt' stands for Pwr,Ttl
} privrec_t;

static psp_paramdescr_t curvv_params[] =
{
    PSP_P_END()
};


static void curvv_rst(int devid, void *devptr, int is_a_reset);
static void curvv_in(int devid, void *devptr,
                     int desc, int datasize, uint8 *data);

static int curvv_init_d(int devid, void *devptr, 
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
                                curvv_rst, curvv_in,
                                CURVV_NUMCHANS * 2,
                                CURVV_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void curvv_term_d(int devid, void *devptr)
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void curvv_rst(int   devid    __attribute__((unused)),
                      void *devptr,
                      int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
        //me->ptregs.rcvd = 0;
        //me->ptregs.pend = 0;
        bzero(&(me->ptregs), sizeof(me->ptregs));
    }
}

static void send_wrpwr_cmd(privrec_t *me)
{
  uint8  data[8];
    
    data[0] = DESC_WRITE_PWR;
    data[1] = (me->ptregs.cur_val &~ me->ptregs.req_msk) |
              (me->ptregs.req_val &  me->ptregs.req_msk);
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);

//    me->ptregs.req_msk = 0;
    me->ptregs.pend    = 0;
}

static void curvv_in(int devid, void *devptr,
                     int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int          n;
  int32        pwr_vals[CURVV_CHAN_PWR_Bn_COUNT];
  int32        ttl_vals[CURVV_CHAN_TTL_Bn_COUNT];
  rflags_t     vals_rflags[CURVV_CHAN_PWR_Bn_COUNT > CURVV_CHAN_TTL_Bn_COUNT?
                           CURVV_CHAN_PWR_Bn_COUNT : CURVV_CHAN_TTL_Bn_COUNT];

  canqelem_t   item;

    switch (desc)
    {
        case DESC_READ_PWR_TTL:
            /* Erase the requestor */
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 6) return;
        
            /* Extract data into a ready-for-ReturnChanGroup format */
            me->ptregs.cur_val = data[1];
            me->ptregs.cur_inp = data[3] | ((uint32)data[4] << 8) | ((uint32)data[5] << 16);
            for (n = 0;  n < CURVV_CHAN_PWR_Bn_COUNT;  n++)
                pwr_vals[n] = (me->ptregs.cur_val & (1 << n)) != 0;
            for (n = 0;  n < CURVV_CHAN_TTL_Bn_COUNT;  n++)
                ttl_vals[n] = (me->ptregs.cur_inp & (1 << n)) != 0;
            me->ptregs.rcvd = 1;
            
            /* Do we have a pending write request? */
            if (me->ptregs.pend != 0)
            {
                send_wrpwr_cmd(me);
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = DESC_READ_PWR_TTL;
                me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
            }
            
            /* And return requested data */
            bzero(vals_rflags, sizeof(vals_rflags));
            ReturnChanGroup(devid, CURVV_CHAN_PWR_Bn_BASE, CURVV_CHAN_PWR_Bn_COUNT, pwr_vals, vals_rflags);
            ReturnChanGroup(devid, CURVV_CHAN_TTL_Bn_BASE, CURVV_CHAN_TTL_Bn_COUNT, ttl_vals, vals_rflags);
            ReturnChanGroup(devid, CURVV_CHAN_PWR_8B,  1, &(me->ptregs.cur_val), &zero_rflags);
            ReturnChanGroup(devid, CURVV_CHAN_TTL_24B, 1, &(me->ptregs.cur_inp), &zero_rflags);
            break;

        default:;
    }
}

static void curvv_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int          n;       // channel N in values[] (loop ctl var)
  int          x;       // channel indeX
  int32        val;     // Value
  uint32       mask;

  canqelem_t   item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        /* Prepare the READ packet -- it will be required in all cases */
        item.prio     = CANKOZ_PRIO_UNICAST;
        item.datasize = 1;
        item.data[0]  = DESC_READ_PWR_TTL;

        if      (x >= CURVV_CHAN_REGS_BASE  &&  x <= CURVV_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, values + n);
        /* Read? */
        else if (
                 ((
                   (x >= CURVV_CHAN_PWR_Bn_BASE  &&  x < CURVV_CHAN_PWR_Bn_BASE + CURVV_CHAN_PWR_Bn_COUNT)
                   ||
                   x == CURVV_CHAN_PWR_8B
                  )  &&  action == DRVA_READ)
                 ||
                 (  x >= CURVV_CHAN_TTL_Bn_BASE  &&  x < CURVV_CHAN_TTL_Bn_BASE + CURVV_CHAN_TTL_Bn_COUNT)
                 ||
                 x   == CURVV_CHAN_TTL_24B
                )
        {
            me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
        }
        /* Some form of write? */
        else if ((x >= CURVV_CHAN_PWR_Bn_BASE  &&  x < CURVV_CHAN_PWR_Bn_BASE + CURVV_CHAN_PWR_Bn_COUNT)
                 || x == CURVV_CHAN_PWR_8B)
        {
            /* Decide, what to write... */
            if (x == CURVV_CHAN_PWR_8B)
            {
                me->ptregs.req_val = val;
                me->ptregs.req_msk = 0xFF;
            }
            else
            {
                mask = 1 << (x - CURVV_CHAN_PWR_Bn_BASE);
                if (val != 0) me->ptregs.req_val |=  mask;
                else          me->ptregs.req_val &=~ mask;
                me->ptregs.req_msk |= mask;
            }
    
            me->ptregs.pend = 1;
            /* May we perform write right now? */
            if (me->ptregs.rcvd)
            {
                send_wrpwr_cmd(me);
                me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
            }
            /* No, we should request read first... */
            else
            {
                me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
            }
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec curvv_main_info[] =
{
    {CURVV_CONFIG_CHAN_COUNT, 1}, // config
    CANKOZ_REGS_MAIN_INFO,        // standard I/O regs
    {12,                      0}, // I/O regs padding
    {CURVV_CHAN_PWR_Bn_COUNT, 1}, // PWR
    {CURVV_CHAN_TTL_Bn_COUNT, 0}, // TTL
    {CURVV_CHAN_TTL_Bn_COUNT, 1}, // TTL_IE
    {1,                       1}, // PWR_8B
    {1,                       0}, // TTL_24B
    {1,                       1}, // TTL_IE24B
    {1,                       0}, // reserved
};

DEFINE_DRIVER(curvv, NULL,
              NULL, NULL,
              sizeof(privrec_t), curvv_params,
              2, 3,
              NULL, 0,
              countof(curvv_main_info), curvv_main_info,
              0, NULL,
              curvv_init_d, curvv_term_d,
              curvv_rw_p, NULL);
