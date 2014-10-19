#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/cgvi8_drv_i.h"


/* CGVI8 specifics */

enum
{
    DEVTYPE       = -1, /* Free the layer from this check, we'll do it ourselves */
    DEVTYPE_CGVI8 = 6,  /* CGVI8 is 6 */
    DEVTYPE_GVI8M = 32,
};

enum
{
    DESC_WRITE_n_BASE   = 0,
    DESC_READ_n_BASE    = 0x10,

    DESC_WRITE_MODE     = 0xF0,
    DESC_WRITE_BASEBYTE = 0xF1,
    DESC_PROGSTART      = 0xF7
};


// Note: 1quant=100ns

static inline int32 prescaler2factor(int prescaler)
{
    return 1 << (prescaler & 0x0F);
}

static inline int32  code2quants(uint16 code,   int prescaler)
{
    return code * prescaler2factor(prescaler);
}

static inline uint16 quants2code(int32  quants, int prescaler)
{
    return quants / prescaler2factor(prescaler);
}


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
    
    int                  hw_ver;
    int                  sw_ver;

    int32                cur_codes     [CGVI8_CHAN_WORK_COUNT];
    int                  cur_codes_read[CGVI8_CHAN_WORK_COUNT];
    
    struct
    {
        /* General behavioral properties */
        int    rcvd;          // Was mode ever read since start/reset
        int    pend;          // A write request is pending

        /* "Output mask" */
        uint8  cur_val;       // Current known-to-be-in-device value
        uint8  req_val;       // Required-for-write value
        uint8  req_msk;       // Mask showing which bits of req_val should be written

        /* Prescaler */
        uint8  cur_prescaler; // Current known-to-be-in-device
        uint8  req_prescaler; // Required-for-write value
        uint8  wrt_prescaler; // Was a write-to-prescaler requested?
    }                    mode;
} privrec_t;


static void cgvi8_rst(int devid, void *devptr, int is_a_reset);
static void cgvi8_in(int devid, void *devptr,
                     int desc, int datasize, uint8 *data);

static int cgvi8_init_d(int devid, void *devptr, 
                        int businfocount, int businfo[],
                        const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                cgvi8_rst, cgvi8_in,
                                CGVI8_NUMCHANS * 2, CGVI8_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void cgvi8_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void cgvi8_rst(int   devid,
                      void *devptr,
                      int   is_a_reset)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         devcode;

    me->iface->get_dev_ver(me->handle, &(me->hw_ver), &(me->sw_ver), &devcode);
    if (devcode != DEVTYPE_CGVI8  &&  devcode != DEVTYPE_GVI8M)
    {
        DoDriverLog(devid, 0,
                    "%s: DevCode=%d is neither CGVI8 (%d) nor GVI8M (%d). Terminating device.",
                    __FUNCTION__, devcode, DEVTYPE_CGVI8, DEVTYPE_GVI8M);
        SetDevState(devid, DEVSTATE_OFFLINE, CXRF_WRONG_DEV, "wrong device");
        return;
    }
    if (is_a_reset)
    {
        bzero(me->cur_codes_read, sizeof(me->cur_codes_read));
        bzero(&(me->mode), sizeof(me->mode));
    }
}

static void send_wrmode_cmd(int devid, privrec_t *me)
{
#if 1
  canqelem_t  item;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 3;
    item.data[0]  = DESC_WRITE_MODE;
    item.data[1]  = (me->mode.cur_val &~ me->mode.req_msk) |
                    (me->mode.req_val &  me->mode.req_msk);
    item.data[2]  = me->mode.wrt_prescaler? me->mode.req_prescaler
                                          : me->mode.cur_prescaler;
    me->iface->q_enq_ons(me->handle, &item, QFE_ALWAYS);
#else
  uint8       data[8];

    /* Note: as for now, we do direct-send, instead of enq_ons... :-( */
    data[0] = DESC_WRITE_MODE;
    data[1] = (me->mode.cur_val &~ me->mode.req_msk) |
              (me->mode.req_val &  me->mode.req_msk);
    data[2] = me->mode.wrt_prescaler? me->mode.req_prescaler
                                    : me->mode.cur_prescaler;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                          3, data);
#endif
    
    me->mode.pend          = 0;
//    me->mode.wrt_prescaler = 0; /* With this uncommented, prescaler stops changing upon mode loads */
}

static void cgvi8_in(int devid, void *devptr __attribute__((unused)),
                     int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         x;
  
  int         chan;
  int32       code;
  int32       val;

  int32       status_b;
  int32       mask;
  int32       prescaler;
  int32       basebyte;
  
  rflags_t    rflags;

  canqelem_t  item;

    switch (desc)
    {
        case DESC_READ_n_BASE ... DESC_READ_n_BASE+CGVI8_CHAN_WORK_COUNT-1:
            if (datasize < 3) return;
            chan   = desc - DESC_READ_n_BASE;
            code   = data[1] + (data[2] << 8);
            val    = code2quants(code, me->mode.cur_prescaler);
            rflags = 0;

            me->cur_codes     [chan] = code;
            me->cur_codes_read[chan] = 1;
            
            me->iface->q_erase_and_send_next(me->handle, 1, desc);

            ReturnChanGroup(devid,
                            CGVI8_CHAN_16BIT_N_BASE + chan,
                            1, &code, &rflags);
            ReturnChanGroup(devid,
                            CGVI8_CHAN_QUANT_N_BASE + chan,
                            1, &val,  &rflags);
            break;

        case CANKOZ_DESC_GETDEVSTAT:
            /* Erase the requestor */
            me->iface->q_erase_and_send_next(me->handle, 1, desc);

            if (datasize < 4) return;

            /* Extract data... */
            rflags    = 0;
            status_b  = data[1];
            mask      = data[2];
            prescaler = data[3];
            if (me->hw_ver >= 1  &&  me->sw_ver >= 5)
                basebyte  = data[4];
            else
                basebyte  = 0;
            DoDriverLog(devid, 0 | DRIVERLOG_C_PKTINFO,
                        "GETDEVSTAT: status=0x%02x mask=0x%02X prescaler=%d basebyte=%d",
                        status_b, mask, prescaler, basebyte);

            /* ...and store it */
            me->mode.cur_val       = mask;
            me->mode.cur_prescaler = prescaler;

            me->mode.rcvd = 1;

            /* Do we have a pending write request? */
            if (me->mode.pend)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = CANKOZ_DESC_GETDEVSTAT;
                send_wrmode_cmd(devid, me);
                me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);
            }

            /* Finally, return received data */
            ReturnChanGroup    (devid, CGVI8_CHAN_MASK8,     1, &mask,      &rflags);
            ReturnChanGroup    (devid, CGVI8_CHAN_PRESCALER, 1, &prescaler, &rflags);
            if (me->hw_ver >= 1  &&  me->sw_ver >= 5)
                ReturnChanGroup(devid, CGVI8_CHAN_BASEBYTE,  1, &basebyte,  &rflags);
            for (x = 0;  x < CGVI8_CHAN_WORK_COUNT;  x++)
            {
                val = (mask & (1 << x)) != 0;
                ReturnChanGroup(devid,
                                CGVI8_CHAN_MASK1_N_BASE + x,
                                1, &val, &rflags);
                if (me->cur_codes_read[x])
                {
                    val = code2quants(me->cur_codes[x], me->mode.cur_prescaler);
                    ReturnChanGroup(devid,
                                    CGVI8_CHAN_QUANT_N_BASE + x,
                                    1, &val, &rflags);
                }
            }

            break;
    }
}

static void cgvi8_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;
  int32       val;

  uint8       data[8];
  canqelem_t  item;
  uint8       mask;

  int         zero     = 0;
  rflags_t    rflags;

  int         c_n;
  int         c_c;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC h

        if      (x >= CGVI8_CHAN_REGS_BASE  &&  x <= CGVI8_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, &val);
        else if (x >= CGVI8_CHAN_16BIT_N_BASE  &&
                 x <= CGVI8_CHAN_QUANT_N_BASE+CGVI8_CHAN_WORK_COUNT-1)
        {
            c_c = 0;
            if (x < CGVI8_CHAN_QUANT_N_BASE)
            {
                c_n = x - CGVI8_CHAN_16BIT_N_BASE;
                if (action == DRVA_WRITE) c_c = val;
            }
            else
            {
                c_n = x - CGVI8_CHAN_QUANT_N_BASE;
                if (action == DRVA_WRITE) c_c = quants2code(val, me->mode.cur_prescaler);
            }

            if (action == DRVA_WRITE)
            {
                if (c_c < 0)     c_c = 0;
                if (c_c > 65535) c_c = 65535;

#if 1
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 3;
                item.data[0] = DESC_WRITE_n_BASE + c_n;
                item.data[1] =  c_c       & 0xFF;
                item.data[2] = (c_c >> 8) & 0xFF;
                me->iface->q_enq_ons(me->handle,
                                     &item, QFE_ALWAYS);
#else
                data[0] = DESC_WRITE_n_BASE + c_n;
                data[1] =  c_c       & 0xFF;
                data[2] = (c_c >> 8) & 0xFF;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      3, data);
#endif
            }

            /*  */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_READ_n_BASE + c_n;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
        else if ((x >= CGVI8_CHAN_MASK1_N_BASE  &&
                  x <= CGVI8_CHAN_MASK1_N_BASE+CGVI8_CHAN_WORK_COUNT-1)
                 ||
                 x == CGVI8_CHAN_MASK8
                 ||
                 x == CGVI8_CHAN_PRESCALER)
        {
            /* Prepare the GETDEVSTAT packet -- it will be required in all cases */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = CANKOZ_DESC_GETDEVSTAT;

            /* Read? */
            if (action == DRVA_READ)
            {
                me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
            }
            /* No, some form of write */
            else
            {
                /* Decide what to modify... */
                if      (x == CGVI8_CHAN_MASK8)
                {
                    me->mode.req_val = val & 0xFF;
                    me->mode.req_msk =       0xFF;
                }
                else if (x == CGVI8_CHAN_PRESCALER)
                {
                    if (val < 0)    val = 0;
                    if (val > 0x0F) val = 0x0F;
                    me->mode.req_prescaler = val;
                    me->mode.wrt_prescaler = 1;
                }
                else /* CGVI8_CHAN_MASK1_N_BASE...+8-1 */
                {
                    mask = 1 << (x - CGVI8_CHAN_MASK1_N_BASE);
                    if (val != 0) me->mode.req_val |=  mask;
                    else          me->mode.req_val &=~ mask;
                    me->mode.req_msk |= mask;
                }

                me->mode.pend = 1;
                /* May we perform write right now? */
                if (me->mode.rcvd)
                {
                    send_wrmode_cmd(devid, me);
                    me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);
                }
                /* No, we should request read first... */
                else
                {
                    me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
                }
            }

        }
        else if (x == CGVI8_CHAN_PROGSTART)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                data[0] = DESC_PROGSTART;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      1, data);
            }

            rflags = 0;
            ReturnChanGroup(devid, x, 1, &zero, &rflags);
        }
        else if (x == CGVI8_CHAN_BASEBYTE)
        {
            if (me->hw_ver >= 1  &&  me->sw_ver >= 5)
            {
                if (action == DRVA_WRITE)
                {
                    c_c = val;
                    if (c_c < 0)   c_c = 0;
                    if (c_c > 255) c_c = 255;
                    data[0] = DESC_WRITE_BASEBYTE;
                    data[1] = c_c;
                    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                          2, data);
                }
                
                /* Perform read anyway */
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = CANKOZ_DESC_GETDEVSTAT;
                me->iface->q_enqueue(me->handle,
                                     &item, QFE_IF_NONEORFIRST);
            }
            else
            {
                rflags = CXRF_UNSUPPORTED;
                ReturnChanGroup(devid, x, 1, &zero, &rflags);
            }
        }
    }
}


/* Metric */

static CxsdMainInfoRec cgvi8_main_info[] =
{
    {CGVI8_CHAN_WORK_COUNT, 1},  // 16bit-codes
    {CGVI8_CHAN_WORK_COUNT, 1},  // Quant-sized (100ns)
    {CGVI8_CHAN_WORK_COUNT, 1},  // 1-bit masks
    {1,                     1},  // 8-bit mask
    {1,                     1},  // Programmatic start
    {1,                     1},  // Prescaler
    {1,                     1},  // Base byte
    CANKOZ_REGS_MAIN_INFO
};

DEFINE_DRIVER(cgvi8, NULL,
              NULL, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              countof(cgvi8_main_info), cgvi8_main_info,
              0, NULL,
              cgvi8_init_d, cgvi8_term_d,
              cgvi8_rw_p, NULL);
