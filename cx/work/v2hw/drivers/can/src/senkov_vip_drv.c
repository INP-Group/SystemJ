#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/senkov_vip_drv_i.h"


enum
{
    DEVTYPE = 0x30*0+-1,
};

enum
{
    DESC_GETMES   = 0x04,
    DESC_WR_DAC   = 0x05,
    DESC_RD_DAC   = 0x06,
    DESC_READSTAT = CANKOZ_DESC_READREGS,
    DESC_WRITECTL = CANKOZ_DESC_WRITEOUTREG,
};

static int max_dac_values[SENKOV_VIP_SET_n_COUNT] =
{
    0,
    700,
    255,
    255,
    255,
    1000,
    12000,
    20
};

enum {HEARTBEAT_USECS = 500*1000};


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

} privrec_t;

static psp_paramdescr_t senkov_vip_params[] =
{
    PSP_P_END()
};


static void senkov_vip_in(int devid, void *devptr,
                          int desc, int datasize, uint8 *data);
static void senkov_vip_heartbeat(int devid, void *devptr,
                                 sl_tid_t tid,
                                 void *privptr);

static int senkov_vip_init_d(int devid, void *devptr, 
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
                                NULL, senkov_vip_in,
                                SENKOV_VIP_NUMCHANS * 2, -1);
    if (me->handle < 0) return me->handle;
    ////me->iface->q_erase_and_send_next(me->handle, 1, CANKOZ_DESC_GETDEVATTRS);

    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, senkov_vip_heartbeat, NULL);

    return DEVSTATE_OPERATING;
}

static void senkov_vip_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}


static void senkov_vip_in(int devid, void *devptr __attribute__((unused)),
                          int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         attr;
  int         code;
  int         b;
  int32       val;
  rflags_t    rflags;

  enum {GRPSIZE = (SENKOV_VIP_MODE_sh_COUNT > SENKOV_VIP_STAT_sh_COUNT)?
                   SENKOV_VIP_MODE_sh_COUNT : SENKOV_VIP_STAT_sh_COUNT};
  int32       valgrp   [GRPSIZE];
  rflags_t    rflagsgrp[GRPSIZE];

    switch (desc)
    {
        case DESC_GETMES:
            attr = data[1];
            me->iface->q_erase_and_send_next(me->handle, 2, desc, attr);
            if (datasize < 4) return;
            
            if (attr < SENKOV_VIP_MES_n_COUNT)
            {
                val    = data[2] + data[3] * 256;
                rflags = 0;
                ReturnChanGroup(devid,
                                SENKOV_VIP_CHAN_MES_BASE + attr,
                                1, &val, &rflags);
            }
            else
            {
                DoDriverLog(devid, 0, "%s(): DESC_GETMES attr=%d, >max=%d",
                            __FUNCTION__, attr, SENKOV_VIP_MES_n_COUNT-1);
            }
            break;

        case DESC_RD_DAC:
            attr = data[1];
            me->iface->q_erase_and_send_next(me->handle, 2, desc, attr);
            if (datasize < 4) return;
            
            if (attr < SENKOV_VIP_SET_n_COUNT)
            {
                val    = data[2] + data[3] * 256;
                rflags = 0;
                ReturnChanGroup(devid,
                                SENKOV_VIP_CHAN_SET_BASE + attr,
                                1, &val, &rflags);
            }
            else
            {
                DoDriverLog(devid, 0, "%s(): DESC_RD_DAC attr=%d, >max=%d",
                            __FUNCTION__, attr, SENKOV_VIP_SET_n_COUNT-1);
            }
            break;

        case DESC_READSTAT:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 5) return;

            // Mode
            code = data[1];
            for (b = 0;  b < SENKOV_VIP_MODE_sh_COUNT;  b++)
            {
                valgrp   [b] = code & 1;
                rflagsgrp[b] = 0;
                code >>= 1;
            }
            ReturnChanGroup(devid,
                            SENKOV_VIP_CHAN_MODE_BASE,
                            SENKOV_VIP_MODE_sh_COUNT, valgrp, rflagsgrp);

            // Status
            code = data[2] + data[3] * 256;
            for (b = 0;  b < SENKOV_VIP_STAT_sh_COUNT;  b++)
            {
                valgrp   [b] = code & 1;
                rflagsgrp[b] = 0;
                code >>= 1;
            }
            ReturnChanGroup(devid,
                            SENKOV_VIP_CHAN_STAT_BASE,
                            SENKOV_VIP_STAT_sh_COUNT, valgrp, rflagsgrp);
            
            // Count
            val    = data[4];
            rflags = 0;
            ReturnChanGroup(devid,
                            SENKOV_VIP_CHAN_CUR_BRKDN_COUNT,
                            1, &val, &rflags);
            break;
    }
}

static void senkov_vip_heartbeat(int devid, void *devptr,
                                 sl_tid_t tid,
                                 void *privptr)
{
  privrec_t  *me    = (privrec_t *) devptr;
  canqelem_t  item;

    sl_enq_tout_after(devid, devptr, HEARTBEAT_USECS, senkov_vip_heartbeat, NULL);
  
    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 2;
    item.data[0]  = DESC_GETMES;
    item.data[1]  = SENKOV_VIP_MES_n_VIP_UOUT;
    me->iface->q_enqueue(me->handle,
                         &item, QFE_IF_ABSENT);
}

static void senkov_vip_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
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

        if      (x >= SENKOV_VIP_CHAN_SET_BASE  &&
                 x <= SENKOV_VIP_CHAN_SET_BASE + SENKOV_VIP_SET_n_COUNT-1  &&
                 x != SENKOV_VIP_CHAN_SET_DAC_RESERVED0)
        {
            attr = x - SENKOV_VIP_CHAN_SET_BASE;
            if (action == DRVA_WRITE)
            {
                code = values[n];
                if (code < 0)                    code = 0;
                if (code > max_dac_values[attr]) code = max_dac_values[attr];

                data[0] = DESC_WR_DAC;
                data[1] = attr;
                data[2] = code & 0xFF;
                data[3] = (code >> 8) & 0xFF;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      4, data);
            }

            /* Perform read anyway */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 2;
            item.data[0]  = DESC_RD_DAC;
            item.data[1]  = attr;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
        else if (x >= SENKOV_VIP_CHAN_CMD_BASE  &&
                 x <= SENKOV_VIP_CHAN_CMD_BASE + SENKOV_VIP_CMD_sh_COUNT-1)
        {
            if (action == DRVA_WRITE  &&  values[n] == CX_VALUE_COMMAND)
            {
                data[0] = DESC_WRITECTL;
                data[1] = 1 << (x - SENKOV_VIP_CHAN_CMD_BASE);
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      2, data);
            }

            // Return 0 for button-type channel
            rflags = 0;
            ReturnChanGroup(devid, x, 1, &zero, &rflags);
        }
        else if (x >= SENKOV_VIP_CHAN_MES_BASE  &&
                 x <= SENKOV_VIP_CHAN_MES_BASE + SENKOV_VIP_MES_n_COUNT-1  &&
                 x != SENKOV_VIP_CHAN_MES_RESERVED0)
        {
            attr = x - SENKOV_VIP_CHAN_MES_BASE;
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 2;
            item.data[0]  = DESC_GETMES;
            item.data[1]  = attr;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_ABSENT);
        }
        else if ((x >= SENKOV_VIP_CHAN_MODE_BASE  &&
                  x <= SENKOV_VIP_CHAN_MODE_BASE + SENKOV_VIP_MODE_sh_COUNT-1)
                 ||
                 (x >= SENKOV_VIP_CHAN_STAT_BASE  &&
                  x <= SENKOV_VIP_CHAN_STAT_BASE + SENKOV_VIP_STAT_sh_COUNT-1)
                 ||
                 x == SENKOV_VIP_CHAN_CUR_BRKDN_COUNT)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_READSTAT;
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

static CxsdMainInfoRec senkov_vip_main_info[] =
{
    {SENKOV_VIP_SET_n_COUNT,   1},
    {SENKOV_VIP_CMD_sh_COUNT,  1},
    {SENKOV_VIP_MES_n_COUNT,   0},
    {SENKOV_VIP_MODE_sh_COUNT, 0},
    {SENKOV_VIP_STAT_sh_COUNT, 0},
    {2,                        0},
};

DEFINE_DRIVER(senkov_vip, "Senkov VIP (high-voltage power supply)",
              NULL, NULL,
              sizeof(privrec_t), senkov_vip_params,
              2, 3,
              NULL, 0,
              countof(senkov_vip_main_info), senkov_vip_main_info,
              0, NULL,
              senkov_vip_init_d, senkov_vip_term_d,
              senkov_vip_rw_p, NULL);
