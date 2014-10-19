#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/senkov_ebc_drv_i.h"


enum
{
    DEVTYPE = 0x20,
};

enum
{
    DESC_GETMES        = 0x02,
    DESC_WR_DAC_n_BASE = 0x80,
    DESC_RD_DAC_n_BASE = 0x90,
};


/*  */

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
    
} privrec_t;

static psp_paramdescr_t senkov_ebc_params[] =
{
    PSP_P_END()
};



static void senkov_ebc_in(int devid, void *devptr,
                          int desc, int datasize, uint8 *data);

static int senkov_ebc_init_d(int devid, void *devptr, 
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
                                NULL, senkov_ebc_in,
                                SENKOV_EBC_NUMCHANS * 2, SENKOV_EBC_CHAN_REGS_BASE);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void senkov_ebc_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}


static void senkov_ebc_in(int devid, void *devptr __attribute__((unused)),
                          int desc, int datasize, uint8 *data)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         chan;
  int32       val;
  rflags_t    rflags;

    switch (desc)
    {
        case DESC_GETMES:
            me->iface->q_erase_and_send_next(me->handle, 2, desc, data[1]);
            if (datasize < 4) return;
            
            chan   = data[1];
            val    = data[2] + data[3] * 256;
            rflags = 0;
            if (chan < SENKOV_EBC_CHAN_ADC_N_COUNT)
            {
                ReturnChanGroup(devid,
                                SENKOV_EBC_CHAN_ADC_N_BASE + chan,
                                1, &val, &rflags);
            }
            else
            {
                DoDriverLog(devid, 0, "in: GETMES chan=%d, >%d",
                            chan, SENKOV_EBC_CHAN_ADC_N_COUNT-1);
            }
            break;

        case DESC_RD_DAC_n_BASE ... DESC_RD_DAC_n_BASE + SENKOV_EBC_CHAN_DAC_N_COUNT-1:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 3) return;
            
            chan   = desc - DESC_RD_DAC_n_BASE;
            val    = data[1] + data[2] * 256;
            rflags = 0;
            ReturnChanGroup(devid,
                            SENKOV_EBC_CHAN_DAC_N_BASE + chan,
                            1, &val, &rflags);
            break;
    }
}

static void senkov_ebc_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

  int         code;
  uint8       data[8];
  canqelem_t  item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        
        if      (x >= SENKOV_EBC_CHAN_REGS_BASE  &&  x <= SENKOV_EBC_CHAN_REGS_LAST)
            me->iface->regs_rw(me->handle, action, x, values + n);
        else if (x >= SENKOV_EBC_CHAN_ADC_N_BASE  &&
                 x <= SENKOV_EBC_CHAN_ADC_N_BASE + SENKOV_EBC_CHAN_ADC_N_COUNT-1)
        {
            /* ADC -- always read, don't care about "action" */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 2;
            item.data[0]  = DESC_GETMES;
            item.data[1]  = x - SENKOV_EBC_CHAN_ADC_N_BASE;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
        else if (x >= SENKOV_EBC_CHAN_DAC_N_BASE  &&
                 x <= SENKOV_EBC_CHAN_DAC_N_BASE + SENKOV_EBC_CHAN_DAC_N_COUNT-1)
        {
            if (x > SENKOV_EBC_CHAN_DAC_N_BASE) continue;
            if (action == DRVA_WRITE)
            {
                code = values[n];
                if (code < 0)    code = 0;
                if (code > 5000) code = 5000;

                data[0] = DESC_WR_DAC_n_BASE + (x - SENKOV_EBC_CHAN_DAC_N_BASE);
                data[1] = code & 0xFF;
                data[2] = (code >> 8) & 0xFF;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST,
                                      3, data);
            }
            
            /* Perform read anyway */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_RD_DAC_n_BASE + (x - SENKOV_EBC_CHAN_DAC_N_BASE);
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
    }
}

/* Metric */

static CxsdMainInfoRec senkov_ebc_main_info[] =
{
    {SENKOV_EBC_CHAN_ADC_N_COUNT,  0},
    {SENKOV_EBC_CHAN_DAC_N_COUNT,  1},
    CANKOZ_REGS_MAIN_INFO
};

DEFINE_DRIVER(senkov_ebc, "Senkov/Pureskin EBC (Electron-Beam Controller)",
              NULL, NULL,
              sizeof(privrec_t), senkov_ebc_params,
              2, 3,
              NULL, 0,
              countof(senkov_ebc_main_info), senkov_ebc_main_info,
              0, NULL,
              senkov_ebc_init_d, senkov_ebc_term_d,
              senkov_ebc_rw_p, NULL);
