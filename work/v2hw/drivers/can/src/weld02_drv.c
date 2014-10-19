#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/weld02_drv_i.h"


enum
{
    DEVTYPE = 0x0F,
};

enum
{
    DESC_GETMES      = 0x03,
    DESC_SET_AA_BASE = 0x70, // "with AutoAnswer"
    DESC_SET_BASE    = 0x80,
    DESC_QRY_BASE    = 0x90,
};

enum {HEARTBEAT_USECS = 1*1000*1000};

static int max_set_values[WELD02_SET_n_COUNT] =
{
    4095,
    4095,
    4095,
    4095,
    4095, //???
    4095, //???
    3,
    1,
};


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

    struct
    {
        int32  vals[WELD02_NUMCHANS];
        int    read[WELD02_NUMCHANS];
    } known;
} privrec_t;


static psp_paramdescr_t weld02_params[] =
{
    PSP_P_END()
};


static void weld02_rst(int devid, void *devptr, int is_a_reset);
static void weld02_in (int devid, void *devptr,
                       int desc, int datasize, uint8 *data);
static void weld02_heartbeat(int devid, void *devptr,
                             sl_tid_t tid,
                             void *privptr);
static void weld02_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action);

static int weld02_init_d(int devid, void *devptr, 
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
                                weld02_rst, weld02_in,
                                WELD02_NUMCHANS * 2, -1);
    if (me->handle < 0) return me->handle;

    ////sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, weld02_heartbeat, NULL);

    return DEVSTATE_OPERATING;
}

static void weld02_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static inline void ReSendSetting(int devid, privrec_t *me, int cn)
{
    if (me->known.read[cn])
        weld02_rw_p(devid, me, cn, 1, me->known.vals + cn, DRVA_WRITE),
            DoDriverLog(devid, 0, "%s(%d:=%d)", __FUNCTION__, cn, me->known.vals[cn]);
}

static void weld02_rst(int devid, void *devptr, int is_a_reset)
{
  privrec_t  *me    = (privrec_t *) devptr;

    if (is_a_reset)
    {
        ReSendSetting(devid, me, WELD02_CHAN_SET_UH);
        ReSendSetting(devid, me, WELD02_CHAN_SET_UL);
        ReSendSetting(devid, me, WELD02_CHAN_SET_UN);
    }
}

static void weld02_in (int devid, void *devptr __attribute__((unused)),
                       int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         attr;
  int         code;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_GETMES:
            if (datasize < 4) return;
            attr = data[1] & 0x1F;
            me->iface->q_erase_and_send_next(me->handle, 2, desc, attr);
            
            if (attr < WELD02_MES_n_COUNT)
            {
                val    = data[2] + data[3] * 256;
                if (
                    (attr == WELD02_SET_n_IH  ||  attr == WELD02_SET_n_IL)
                    &&
                    ((data[1] & 0x40) == 0)
                   )
                    val *= 10;
                rflags = 0;
                ReturnChanGroup(devid,
                                WELD02_CHAN_MES_BASE + attr,
                                1, &val, &rflags);
                me->known.vals[WELD02_CHAN_MES_BASE + attr] = val;
                me->known.read[WELD02_CHAN_MES_BASE + attr] = 1;
            }
            else
            {
                DoDriverLog(devid, 0, "%s(): DESC_GETMES attr=%d, >max=%d",
                            __FUNCTION__, attr, WELD02_MES_n_COUNT-1);
            }
            break;

        case DESC_QRY_BASE ... DESC_QRY_BASE + WELD02_SET_n_COUNT - 1:
            if (datasize < 4) return;
            attr = desc - DESC_QRY_BASE;
            me->iface->q_erase_and_send_next(me->handle, 1, desc);

            val    = (data[2] << 8) | data[3];
            rflags = 0;
            ReturnChanGroup(devid,
                            WELD02_CHAN_SET_BASE + attr,
                            1, &val, &rflags);
            break;

        case DESC_SET_AA_BASE ... DESC_SET_AA_BASE + WELD02_SET_n_COUNT - 1:
            if (datasize < 4) return;
            attr = desc - DESC_SET_AA_BASE;
            me->iface->q_erase_and_send_next(me->handle, -1, desc);

            val    = (data[2] << 8) | data[3];
            rflags = 0;
            ReturnChanGroup(devid,
                            WELD02_CHAN_SET_BASE + attr,
                            1, &val, &rflags);
            break;
    }
}

static void weld02_heartbeat(int devid, void *devptr,
                             sl_tid_t tid,
                             void *privptr)
{
  privrec_t  *me    = (privrec_t *) devptr;

    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, weld02_heartbeat, NULL);

    ReSendSetting(devid, me, WELD02_CHAN_SET_UH);
    ReSendSetting(devid, me, WELD02_CHAN_SET_UL);
    ReSendSetting(devid, me, WELD02_CHAN_SET_UN);
}

static void weld02_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

  int         attr;
  int         code;
  uint8       data[8];
  canqelem_t  item;

  int         zero     = 0;
  rflags_t    rflags;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

        if      (x >= WELD02_CHAN_SET_BASE  &&
                 x <= WELD02_CHAN_SET_BASE + WELD02_SET_n_COUNT-1)
        {
            attr = x - WELD02_CHAN_SET_BASE;
#define USE_AA 1
#if USE_AA
            if (action == DRVA_WRITE)
            {
                code = values[n];
                if (code < 0)                    code = 0;
                if (code > max_set_values[attr]) code = max_set_values[attr];

                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 4;
                item.data[0]  = DESC_SET_AA_BASE + attr;
                item.data[1]  = 0;
                item.data[2]  = (code >> 8) & 0x0F;
                item.data[3]  = code & 0xFF;
                me->iface->q_enqueue(me->handle,
                                     &item, QFE_IF_NONEORFIRST);
            }
            else
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = DESC_QRY_BASE + attr;
                me->iface->q_enqueue(me->handle,
                                     &item, QFE_IF_NONEORFIRST);
            }
#else
            if (action == DRVA_WRITE)
            {
                code = values[n];
                if (code < 0)                    code = 0;
                if (code > max_set_values[attr]) code = max_set_values[attr];

                data[0] = DESC_SET_BASE + attr;
                data[1] = 0;
                data[2] = (code >> 8) & 0x0F;
                data[3] = code & 0xFF;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      4, data);
            }

            /* Perform read anyway */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_QRY_BASE + attr;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
#endif
        }
        else if (x >= WELD02_CHAN_MES_BASE  &&
                 x <= WELD02_CHAN_MES_BASE + WELD02_MES_n_COUNT-1  &&
                 x != WELD02_CHAN_MES_RSRVD)
        {
            attr = x - WELD02_CHAN_MES_BASE;
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 2;
            item.data[0]  = DESC_GETMES;
            item.data[1]  = attr;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_ABSENT);
        }
        else
        {
            rflags = CXRF_UNSUPPORTED;
            ReturnChanGroup(devid, x, 1, &zero, &rflags);
        }
    }
}

/* Metric */

static CxsdMainInfoRec weld02_main_info[] =
{
    {WELD02_SET_n_COUNT,     1},
    {WELD02_MES_n_COUNT,     0},
};

DEFINE_DRIVER(weld02, "Zharikov-Repkov WELD02 welding controller",
              NULL, NULL,
              sizeof(privrec_t), weld02_params,
              2, 3,
              NULL, 0,
              countof(weld02_main_info), weld02_main_info,
              0, NULL,
              weld02_init_d, weld02_term_d,
              weld02_rw_p, NULL);
