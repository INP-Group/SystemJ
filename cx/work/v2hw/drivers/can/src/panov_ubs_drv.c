#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/panov_ubs_drv_i.h"


enum
{
    DEVTYPE = 22
};

/*
 *  WRT  write
 *  GET  get
 *  ACT  activate //13.02.2011 -- obsolete
 *  MES  measure
 *  RST  reset
 *  ENG  engage (switch on)
 *  OFF  switch off
 *  CLB  calibrate
 */
enum
{
    DESC_ERROR             = 0x00,

    /* Starter1 */
    DESC_ST1_WRT_IHEAT     = 0x01,
    DESC_ST1_GET_IHEAT     = 0x09,
    DESC_ST1_ACT_IHEAT     = 0x11,
    DESC_ST1_MES_IHEAT     = 0x19,
    DESC_ST1_WRT_MIN_IHEAT = 0x21,
    DESC_ST1_GET_MIN_IHEAT = 0x29,
    DESC_ST1_WRT_MAX_IHEAT = 0x31,
    DESC_ST1_GET_MAX_IHEAT = 0x39,
    DESC_ST1_ENG_HEAT      = 0x41,
    DESC_ST1_OFF_HEAT      = 0x49,
    DESC_ST1_WRT_IARC      = 0x51,
    DESC_ST1_GET_IARC      = 0x59,
    DESC_ST1_ACT_IARC      = 0x61,
    DESC_ST1_MES_IARC      = 0x69,
    DESC_ST1_WRT_MIN_IARC  = 0x71,
    DESC_ST1_GET_MIN_IARC  = 0x79,
    DESC_ST1_WRT_MAX_IARC  = 0x81,
    DESC_ST1_GET_MAX_IARC  = 0x89,
    //
    DESC_ST1_GET_HEAT_TIME = 0x99,
    DESC_ST1_CLB_IHEAT     = 0xA1,
    DESC_ST1_GET_CST_IHEAT = 0xA9,
    DESC_ST1_CLB_IARC      = 0xB1,
    DESC_ST1_GET_CST_IARC  = 0xB9,
    //
    DESC_ST1_WRT_MASK      = 0xD1,
    DESC_ST1_GET_MASK      = 0xD9,
    DESC_ST1_RST_ILKS      = 0xE1,
    DESC_ST1_MES_ILKS      = 0xE9,
    DESC_ST1_REBOOT        = 0xF1,
    DESC_ST1_GET_STATE     = 0xF9,

    /* Starter2 */
    DESC_ST2_WRT_IHEAT     = 0x02,
    DESC_ST2_GET_IHEAT     = 0x0A,
    DESC_ST2_ACT_IHEAT     = 0x12,
    DESC_ST2_MES_IHEAT     = 0x1A,
    DESC_ST2_WRT_MIN_IHEAT = 0x22,
    DESC_ST2_GET_MIN_IHEAT = 0x2A,
    DESC_ST2_WRT_MAX_IHEAT = 0x32,
    DESC_ST2_GET_MAX_IHEAT = 0x3A,
    DESC_ST2_ENG_HEAT      = 0x42,
    DESC_ST2_OFF_HEAT      = 0x4A,
    DESC_ST2_WRT_IARC      = 0x52,
    DESC_ST2_GET_IARC      = 0x5A,
    DESC_ST2_ACT_IARC      = 0x62,
    DESC_ST2_MES_IARC      = 0x6A,
    DESC_ST2_WRT_MIN_IARC  = 0x72,
    DESC_ST2_GET_MIN_IARC  = 0x7A,
    DESC_ST2_WRT_MAX_IARC  = 0x82,
    DESC_ST2_GET_MAX_IARC  = 0x8A,
    //
    DESC_ST2_GET_HEAT_TIME = 0x9A,
    DESC_ST2_CLB_IHEAT     = 0xA2,
    DESC_ST2_GET_CST_IHEAT = 0xAA,
    DESC_ST2_CLB_IARC      = 0xB2,
    DESC_ST2_GET_CST_IARC  = 0xBA,
    //
    DESC_ST2_WRT_MASK      = 0xD2,
    DESC_ST2_GET_MASK      = 0xDA,
    DESC_ST2_RST_ILKS      = 0xE2,
    DESC_ST2_MES_ILKS      = 0xEA,
    DESC_ST2_REBOOT        = 0xF2,
    DESC_ST2_GET_STATE     = 0xFA,

    /* Degausser */
    DESC_DGS_WRT_UDGS      = 0x03,
    DESC_DGS_GET_UDGS      = 0x0B,
    DESC_DGS_ACT_UDGS      = 0x13,
    DESC_DGS_MES_UDGS      = 0x1B,
    DESC_DGS_WRT_MIN_UDGS  = 0x23,
    DESC_DGS_GET_MIN_UDGS  = 0x2B,
    DESC_DGS_WRT_MAX_UDGS  = 0x33,
    DESC_DGS_GET_MAX_UDGS  = 0x3B,
    DESC_DGS_ENG_UDGS      = 0x43,
    DESC_DGS_OFF_UDGS      = 0x4B,
    //
    DESC_DGS_CLB_UDGS      = 0xA3,
    DESC_DGS_GET_CST_UDGS  = 0xAB,
    //
    DESC_DGS_WRT_MASK      = 0xD3,
    DESC_DGS_GET_MASK      = 0xDB,
    DESC_DGS_RST_ILKS      = 0xE3,
    DESC_DGS_MES_ILKS      = 0xEB,
    DESC_DGS_REBOOT        = 0xF3,
    DESC_DGS_GET_STATE     = 0xFB,

    /* "All" -- commands for all 3 devices */
    DESC_ALL_WRT_PARAMS    = 0x06,
    DESC_ALL_GET_PARAMS    = 0x0E,
    DESC_ALL_ACT_PARAMS    = 0x16,
    DESC_ALL_MES_PARAMS    = 0x1E,
    DESC_ALL_WRT_MIN       = 0x26,
    DESC_ALL_GET_MIN       = 0x2E,
    DESC_ALL_WRT_MAX       = 0x36,
    DESC_ALL_GET_MAX       = 0x3E,
    DESC_ALL_GET_HEAT_TIME = 0x9E,
    DESC_ALL_GET_CST_PARAMS = 0xAE,
    DESC_ALL_WRT_ILKS_MASK = 0xD6,
    DESC_ALL_GET_ILKS_MASK = 0xDE,
    DESC_ALL_RST_ILKS      = 0xE6,
    DESC_ALL_MES_ILKS      = 0xEE,
    DESC_ALL_REBOOT        = 0xF6,
    DESC_ALL_GET_STATE     = 0xFE, // =CANKOZ_DESC_GETDEVSTAT

    /* UBS */
    DESC_UBS_WRT_THT_TIME  = 0x07,
    DESC_UBS_GET_THT_TIME  = 0x0F,
    DESC_UBS_WRT_MODE      = 0x17,
    DESC_UBS_GET_MODE      = 0x1F,
    DESC_UBS_REREAD_HWADDR = 0x27,
    DESC_UBS_GET_HWADDR    = 0x2F,
    //
    DESC_UBS_POWER_ON      = 0x47,
    DESC_UBS_POWER_OFF     = 0x4F,
    DESC_UBS_WRT_ARC_TIME  = 0x57,
    DESC_UBS_GET_ARC_TIME  = 0x5F,
    //
    DESC_UBS_SAVE_SETTINGS = 0xA7,
    //
    DESC_UBS_WRT_ILKS_MASK = 0xD7,
    DESC_UBS_GET_ILKS_MASK = 0xDF,
    DESC_UBS_RST_ILKS      = 0xE7,
    DESC_UBS_MES_ILKS      = 0xEF,
    DESC_UBS_REBOOT        = 0xF7,
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
    PERSISTENT_STAT_MASK =
        (1 << (PANOV_UBS_CHAN_STAT_ST1_REBOOT - PANOV_UBS_CHAN_STAT_base)) |
        (1 << (PANOV_UBS_CHAN_STAT_ST2_REBOOT - PANOV_UBS_CHAN_STAT_base)) |
        (1 << (PANOV_UBS_CHAN_STAT_DGS_REBOOT - PANOV_UBS_CHAN_STAT_base)) |
        (1 << (PANOV_UBS_CHAN_STAT_UBS_REBOOT - PANOV_UBS_CHAN_STAT_base))
};


typedef struct
{
    int8   nb;      // Number of data Bytes
    uint8  rd_desc; // Read
    uint8  wr_desc; // Write
    uint8  after_w; // Additional command to send AFTER write
} chan2desc_t;

static chan2desc_t chan2desc_map[] =
{
    //
    [PANOV_UBS_CHAN_SET_ST1_IHEAT] = {1, DESC_ST1_GET_IHEAT,     DESC_ST1_WRT_IHEAT,     0},
    ////[PANOV_UBS_CHAN_SET_ST1_IARC]  = {1, DESC_ST1_GET_IARC,      DESC_ST1_WRT_IARC,      0},
    [PANOV_UBS_CHAN_SET_ST2_IHEAT] = {1, DESC_ST2_GET_IHEAT,     DESC_ST2_WRT_IHEAT,     0},
    ////[PANOV_UBS_CHAN_SET_ST2_IARC]  = {1, DESC_ST2_GET_IARC,      DESC_ST2_WRT_IARC,      0},
    [PANOV_UBS_CHAN_SET_DGS_UDGS]  = {1, DESC_DGS_GET_UDGS,      DESC_DGS_WRT_UDGS,      0},
    [PANOV_UBS_CHAN_SET_THT_TIME]  = {1, DESC_UBS_GET_THT_TIME,  DESC_UBS_WRT_THT_TIME,  0},
    ////[PANOV_UBS_CHAN_SET_ARC_TIME]  = {1, DESC_UBS_GET_ARC_TIME,  DESC_UBS_WRT_ARC_TIME,  0},
    [PANOV_UBS_CHAN_SET_MODE]      = {1, DESC_UBS_GET_MODE,      DESC_UBS_WRT_MODE,      0},
    //
    [PANOV_UBS_CHAN_CLB_ST1_IHEAT] = {0, 0,                      DESC_ST1_CLB_IHEAT,     0},
    ////[PANOV_UBS_CHAN_CLB_ST1_IARC]  = {0, 0,                      DESC_ST1_CLB_IARC,      0},
    [PANOV_UBS_CHAN_CLB_ST2_IHEAT] = {0, 0,                      DESC_ST2_CLB_IHEAT,     0},
    ////[PANOV_UBS_CHAN_CLB_ST2_IARC]  = {0, 0,                      DESC_ST2_CLB_IARC,      0},
    [PANOV_UBS_CHAN_CLB_DGS_UDGS]  = {0, 0,                      DESC_DGS_CLB_UDGS,      0},
    //
    [PANOV_UBS_CHAN_MIN_ST1_IHEAT] = {1, DESC_ST1_GET_MIN_IHEAT, DESC_ST1_WRT_MIN_IHEAT, 0},
    ////[PANOV_UBS_CHAN_MIN_ST1_IARC]  = {1, DESC_ST1_GET_MIN_IARC,  DESC_ST1_WRT_MIN_IARC,  0},
    [PANOV_UBS_CHAN_MIN_ST2_IHEAT] = {1, DESC_ST2_GET_MIN_IHEAT, DESC_ST2_WRT_MIN_IHEAT, 0},
    ////[PANOV_UBS_CHAN_MIN_ST2_IARC]  = {1, DESC_ST2_GET_MIN_IARC,  DESC_ST2_WRT_MIN_IARC,  0},
    [PANOV_UBS_CHAN_MIN_DGS_UDGS]  = {1, DESC_DGS_GET_MIN_UDGS,  DESC_DGS_WRT_MIN_UDGS,  0},
    [PANOV_UBS_CHAN_MAX_ST1_IHEAT] = {1, DESC_ST1_GET_MAX_IHEAT, DESC_ST1_WRT_MAX_IHEAT, 0},
    ////[PANOV_UBS_CHAN_MAX_ST1_IARC]  = {1, DESC_ST1_GET_MAX_IARC,  DESC_ST1_WRT_MAX_IARC,  0},
    [PANOV_UBS_CHAN_MAX_ST2_IHEAT] = {1, DESC_ST2_GET_MAX_IHEAT, DESC_ST2_WRT_MAX_IHEAT, 0},
    ////[PANOV_UBS_CHAN_MAX_ST2_IARC]  = {1, DESC_ST2_GET_MAX_IARC,  DESC_ST2_WRT_MAX_IARC,  0},
    [PANOV_UBS_CHAN_MAX_DGS_UDGS]  = {1, DESC_DGS_GET_MAX_UDGS,  DESC_DGS_WRT_MAX_UDGS,  0},
    //
    [PANOV_UBS_CHAN_MES_ST1_IHEAT] = {1, DESC_ST1_MES_IHEAT,     0,                      0},
    [PANOV_UBS_CHAN_MES_ST1_IARC]  = {1, DESC_ST1_MES_IARC,      0,                      0},
    [PANOV_UBS_CHAN_MES_ST2_IHEAT] = {1, DESC_ST2_MES_IHEAT,     0,                      0},
    [PANOV_UBS_CHAN_MES_ST2_IARC]  = {1, DESC_ST2_MES_IARC,      0,                      0},
    [PANOV_UBS_CHAN_MES_DGS_UDGS]  = {1, DESC_DGS_MES_UDGS,      0,                      0},
    [PANOV_UBS_CHAN_CST_ST1_IHEAT] = {1, DESC_ST1_GET_CST_IHEAT, 0,                      0},
    ////[PANOV_UBS_CHAN_CST_ST1_IARC]  = {1, DESC_ST1_GET_CST_IARC,  0,                      0},
    [PANOV_UBS_CHAN_CST_ST2_IHEAT] = {1, DESC_ST2_GET_CST_IHEAT, 0,                      0},
    ////[PANOV_UBS_CHAN_CST_ST2_IARC]  = {1, DESC_ST2_GET_CST_IARC,  0,                      0},
    [PANOV_UBS_CHAN_CST_DGS_UDGS]  = {1, DESC_DGS_GET_CST_UDGS,  0,                      0},
    //
    [PANOV_UBS_CHAN_POWER_ON]      = {0, 0,                      DESC_UBS_POWER_ON,      CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_POWER_OFF]     = {0, 0,                      DESC_UBS_POWER_OFF,     CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_ST1_HEAT_ON]   = {0, 0,                      DESC_ST1_ENG_HEAT,      CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_ST1_HEAT_OFF]  = {0, 0,                      DESC_ST1_OFF_HEAT,      CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_ST2_HEAT_ON]   = {0, 0,                      DESC_ST2_ENG_HEAT,      CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_ST2_HEAT_OFF]  = {0, 0,                      DESC_ST2_OFF_HEAT,      CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_DGS_UDGS_ON]   = {0, 0,                      DESC_DGS_ENG_UDGS,      CANKOZ_DESC_GETDEVSTAT},
    [PANOV_UBS_CHAN_DGS_UDGS_OFF]  = {0, 0,                      DESC_DGS_OFF_UDGS,      CANKOZ_DESC_GETDEVSTAT},
    //
    [PANOV_UBS_CHAN_REBOOT_ALL]    = {0, 0,                      DESC_ALL_REBOOT,        0},
    [PANOV_UBS_CHAN_REBOOT_ST1]    = {0, 0,                      DESC_ST1_REBOOT,        0},
    [PANOV_UBS_CHAN_REBOOT_ST2]    = {0, 0,                      DESC_ST2_REBOOT,        0},
    [PANOV_UBS_CHAN_REBOOT_DGS]    = {0, 0,                      DESC_DGS_REBOOT,        0},
    [PANOV_UBS_CHAN_REBOOT_UBS]    = {0, 0,                      DESC_UBS_REBOOT,        0},
    [PANOV_UBS_CHAN_REREAD_HWADDR] = {0, 0,                      DESC_UBS_REREAD_HWADDR, 0},
    //
    [PANOV_UBS_CHAN_SAVE_SETTINGS] = {0, 0,                      DESC_UBS_SAVE_SETTINGS, 0},
    //
    [PANOV_UBS_CHAN_RST_ALL_ILK]   = {0, 0,                      DESC_ALL_RST_ILKS,      DESC_ALL_MES_ILKS},
    [PANOV_UBS_CHAN_RST_ST1_ILK]   = {0, 0,                      DESC_ST1_RST_ILKS,      DESC_ALL_MES_ILKS},
    [PANOV_UBS_CHAN_RST_ST2_ILK]   = {0, 0,                      DESC_ST2_RST_ILKS,      DESC_ALL_MES_ILKS},
    [PANOV_UBS_CHAN_RST_DGS_ILK]   = {0, 0,                      DESC_DGS_RST_ILKS,      DESC_ALL_MES_ILKS},
    [PANOV_UBS_CHAN_RST_UBS_ILK]   = {0, 0,                      DESC_UBS_RST_ILKS,      DESC_ALL_MES_ILKS},
    //
    [PANOV_UBS_CHAN_HWADDR]        = {1, DESC_UBS_GET_HWADDR,    0,                      0},
};

static int desc2chan_map[256];

static int panov_ubs_init_mod(void)
{
  int          x;
  int          chan;
  chan2desc_t *cdp;

    /* Initialize all with -1's first (since chan MAY be ==0) */
    for (x = 0;  x < countof(desc2chan_map);  x++) desc2chan_map[x] = -1;
  
    /* And than mark used cells with their client-channel numbers */
    for (chan = 0, cdp = chan2desc_map;
         chan < countof(chan2desc_map);
         chan++,   cdp++)
    {
        if (cdp->rd_desc != 0)
            desc2chan_map[cdp->rd_desc] = chan;
        if (cdp->wr_desc != 0)
            desc2chan_map[cdp->wr_desc] = chan;
    }

    return 0;
}


static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

    int32                hwaddr;

    uint32               acc_stat;

    uint32               ff_poweron_count;
    uint32               ff_watchdog_count;

    struct
    {
        int     rcvd;
        int     pend;

        uint8   cur_vals[8];
        uint8   req_vals[8];
        uint8   req_msks[8];
    }                    msks;
} privrec_t;

static psp_paramdescr_t panov_ubs_params[] =
{
    PSP_P_END()
};


static void panov_ubs_rst(int devid, void *devptr, int is_a_reset);
static void panov_ubs_in(int devid, void *devptr,
                         int desc, int datasize, uint8 *data);

static int panov_ubs_init_d(int devid, void *devptr, 
                            int businfocount, int businfo[],
                            const char *auxinfo)
{
  privrec_t *me      = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo, DEVTYPE,
                                panov_ubs_rst, panov_ubs_in,
                                PANOV_UBS_NUMCHANS * 2, -1);
    if (me->handle < 0) return me->handle;

    me->hwaddr = -1;
    
    return DEVSTATE_OPERATING;
}

static void panov_ubs_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void panov_ubs_rst(int   devid    __attribute__((unused)),
                          void *devptr,
                          int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
        me->hwaddr         = -1;
        bzero(&(me->msks), sizeof(me->msks));
//        me->msks.rcvd = 0;
//        me->msks.pend = 0;
    }
}

static void send_wrmsks_cmd(privrec_t *me)
{
  uint8  data[8];
  uint8  vals[7];
  int    x;
  
    for (x = 0;  x <= 7;  x++)
        vals[x] = (me->msks.cur_vals[x] &~ me->msks.req_msks[x]) |
                  (me->msks.req_vals[x] &  me->msks.req_msks[x]);
  
    data[0] = DESC_ALL_WRT_ILKS_MASK;
    data[1] = vals[0];
    data[2] = vals[1];
    data[3] = vals[2];
    data[4] = vals[3];
    data[5] = vals[4];
    data[6] = vals[5];
    data[7] = vals[6];
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 8, data);

    me->msks.pend = 0;
}

static void panov_ubs_in(int devid, void *devptr __attribute__((unused)),
                         int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value
  int          chan;
  int          err_desc;

  uint32       bits;
  int          x;
  int32        bits_vals[32];
  rflags_t     bits_flgs[32];

  int32        msks_vals[64];
  rflags_t     msks_flgs[64];
  canqelem_t   item;


    if (desc == DESC_UBS_GET_HWADDR)
    {
        me->iface->q_erase_and_send_next(me->handle, -1, desc);
        if (datasize < 2) return;

        me->hwaddr = val = data[1];
        ReturnChanGroup(devid, PANOV_UBS_CHAN_HWADDR,        1, &val, &zero_rflags);
        return;
    }
    if (desc == DESC_ALL_MES_PARAMS)
    {
        me->iface->q_erase_and_send_next(me->handle, -1, desc);
        if (datasize < 6) return;

        val = data[1];
        ReturnChanGroup(devid, PANOV_UBS_CHAN_MES_ST1_IHEAT, 1, &val, &zero_rflags);
        val = data[2];
        ReturnChanGroup(devid, PANOV_UBS_CHAN_MES_ST1_IARC,  1, &val, &zero_rflags);
        val = data[3];
        ReturnChanGroup(devid, PANOV_UBS_CHAN_MES_ST2_IHEAT, 1, &val, &zero_rflags);
        val = data[4];
        ReturnChanGroup(devid, PANOV_UBS_CHAN_MES_ST2_IARC,  1, &val, &zero_rflags);
        val = data[5];
        ReturnChanGroup(devid, PANOV_UBS_CHAN_MES_DGS_UDGS,  1, &val, &zero_rflags);
        return;
    }
    if (desc > 0  &&  desc < countof(desc2chan_map)  &&
        (chan = desc2chan_map[desc]) >= 0)
    {
        me->iface->q_erase_and_send_next(me->handle, -1, desc);

        if (
            chan >= 0  &&  chan < countof(chan2desc_map)  &&  // Guard: channel IS present in table
            desc == chan2desc_map[chan].rd_desc           &&  // It is a READ command
            chan2desc_map[chan].nb != 0                       // It is a DATA, not a COMMAND channel
           )
        {
#if 1
            if (datasize < chan2desc_map[chan].nb + 1) return;
#else
            if (datasize != chan2desc_map[chan].nb + 1  &&  0)
            {
                DoDriverLog(devid, 0,
                            "DESC=0x%02x: packet size mismatch - %d bytes (!=%d)",
                            desc, datasize, chan2desc_map[chan].nb + 1);
                return;
            }
#endif
            
            val  = data[1];
            ReturnChanGroup(devid, chan, 1, &val, &zero_rflags);
        }
        return;
    }
    switch (desc)
    {
        case CANKOZ_DESC_GETDEVATTRS:
            if (datasize > 4)
            {
                if      (data[4] == CANKOZ_IAMR_POWERON)
                {
                    me->ff_poweron_count++;
                    ReturnChanGroup(devid, PANOV_UBS_CHAN_POWERON_CTR, 1,
                                    &(me->ff_poweron_count), &zero_rflags);
                }
                else if (data[4] == CANKOZ_IAMR_WATCHDOG)
                {
                    me->ff_watchdog_count++;
                    ReturnChanGroup(devid, PANOV_UBS_CHAN_WATCHDOG_CTR, 1,
                                    &(me->ff_watchdog_count), &zero_rflags);
                }
            }
            break;

        case CANKOZ_DESC_GETDEVSTAT:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 6) return;

            bits = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
            me->acc_stat |= (bits & PERSISTENT_STAT_MASK);
            bits |= me->acc_stat;

            for (x = 0;  x < 32;  x++)
            {
                bits_vals[x] = (bits >> x) & 1;
                bits_flgs[x] = 0;
            }

            ReturnChanGroup(devid, PANOV_UBS_CHAN_STAT_base, 32,
                            bits_vals, bits_flgs);

            val = data[5];
            ReturnChanGroup(devid, PANOV_UBS_CHAN_SET_MODE, 1,
                            &val, &zero_rflags);
            break;

        case DESC_ALL_MES_ILKS:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 5) return;

            bits = data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);

            for (x = 0;  x < 32;  x++)
            {
                bits_vals[x] = (bits >> x) & 1;
                bits_flgs[x] = 0;
            }

            ReturnChanGroup(devid, PANOV_UBS_CHAN_ILK_base, 32,
                            bits_vals, bits_flgs);
            break;

        case DESC_ALL_GET_ILKS_MASK:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            if (datasize < 5) return;

            /* Move data to storage... */
            me->msks.cur_vals[0] = data[1];
            me->msks.cur_vals[1] = data[2];
            me->msks.cur_vals[2] = data[3];
            me->msks.cur_vals[3] = data[4];
            me->msks.cur_vals[4] = data[5];
            me->msks.cur_vals[5] = data[6];
            me->msks.cur_vals[6] = data[7];
            me->msks.cur_vals[7] = 0;
            me->msks.rcvd = 1;

            /* Do we have a pending write request? */
            if (me->msks.pend != 0)
            {
                send_wrmsks_cmd(me);
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = DESC_ALL_GET_ILKS_MASK;
                me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
            }

            /* And return requested data */
            for (x = 0;  x < 64;  x++)
                msks_vals[x] = (me->msks.cur_vals[x / 8] & (1 << (x & 7))) != 0;
            bzero(msks_flgs, sizeof(msks_flgs));
            ReturnChanGroup(devid, PANOV_UBS_CHAN_LKM_base, 64, msks_vals, msks_flgs);
            
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
                err_desc = data[1];
                if (err_desc > 0  &&  err_desc < countof(desc2chan_map)  &&
                    (chan = desc2chan_map[err_desc]) >= 0)
                {
                    val = 0;
                    ReturnChanGroup(devid, chan, 1, &val, &unsp_rflags);
                }
                else
                {
                    DoDriverLog(devid, 0,
                                "DESC_ERROR: ERR_UNKCMD cmd=0x%02x", 
                                data[1]);
                    SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "ERR_UNKCMD");
                }
                return;
            }
            break;
    }
}

static void panov_ubs_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t    *me       = (privrec_t *) devptr;
  int           n;       // channel N in values[] (loop ctl var)
  int           x;       // channel indeX
  int32         val;     // Value

  canqelem_t    item;
  qfe_status_t  sr;

  chan2desc_t *cdp;
  
    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        cdp = chan2desc_map + x;
        if      (x == PANOV_UBS_CHAN_HWADDR)
        {
            if (me->hwaddr >= 0)
            {
                ReturnChanGroup(devid, x, 1, &(me->hwaddr), &zero_rflags);
            }
            else
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = DESC_UBS_GET_HWADDR;
                me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
            }
        }
        else if (x == PANOV_UBS_CHAN_MES_ST1_IHEAT  ||
                 x == PANOV_UBS_CHAN_MES_ST1_IARC   ||
                 x == PANOV_UBS_CHAN_MES_ST2_IHEAT  ||
                 x == PANOV_UBS_CHAN_MES_ST2_IARC   ||
                 x == PANOV_UBS_CHAN_MES_DGS_UDGS)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_ALL_MES_PARAMS;
            me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
        }
        else if (x == PANOV_UBS_CHAN_REBOOT_UBS  &&  action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
        {
            item.data[0] = DESC_UBS_REBOOT;
            me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, item.data);
            
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x < countof(chan2desc_map)  &&
                 (cdp->rd_desc != 0  ||  cdp->wr_desc != 0))
        {
            /* A command channel? */
            if (cdp->nb == 0)
            {
                if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                {
                    item.prio     = CANKOZ_PRIO_UNICAST;
                    item.datasize = 1;
                    item.data[0]  = cdp->wr_desc;
                    sr = me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
                    
                    if (sr == QFE_NOTFOUND  &&  cdp->after_w != 0)
                    {
                        item.prio     = CANKOZ_PRIO_UNICAST;
                        item.datasize = 1;
                        item.data[0]  = cdp->after_w;
                        me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);
                    }
                }

                val = 0;
                ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
            }
            /* No, a regular data one */
            else
            {
                if (action == DRVA_WRITE  &&  cdp->wr_desc != 0)
                {
                    if (val < 0)   val = 0;
                    if (val > 255) val = 255;
                    
                    item.prio     = CANKOZ_PRIO_UNICAST;
                    item.datasize = cdp->nb + 1;
                    item.data[0]  = cdp->wr_desc;
                    item.data[1]  = val;
                    me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);
                }

                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = cdp->rd_desc;
                me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);

                /* A hack for auto-activation */
                if (0 &&
                    action == DRVA_WRITE               &&
                    x >= PANOV_UBS_CHAN_SET_ST1_IHEAT  &&
                    x <= PANOV_UBS_CHAN_SET_DGS_UDGS)
                {
                    val = CX_VALUE_COMMAND;
                    panov_ubs_rw_p(devid, devptr,
                                   (x + 10), 1, &val, DRVA_WRITE);
                }
            }
        }
        else if (x >= PANOV_UBS_CHAN_STAT_base  &&
                 x <= PANOV_UBS_CHAN_STAT_base + 32-1)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = CANKOZ_DESC_GETDEVSTAT;
            me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
        }
        else if (x >= PANOV_UBS_CHAN_ILK_base  &&
                 x <= PANOV_UBS_CHAN_ILK_base + 32-1)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_ALL_MES_ILKS;
            me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
        }
        else if (x >= PANOV_UBS_CHAN_LKM_base  &&
                 x <= PANOV_UBS_CHAN_LKM_base + 64-1)
        {
          int  offs = (x - PANOV_UBS_CHAN_LKM_base) / 8;
          int  mask = 1 << ((x - PANOV_UBS_CHAN_LKM_base) & 7);

            /* Prepare the GET packet -- it will be required in all cases */
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_ALL_GET_ILKS_MASK;

            /* Read? */
            if (action == DRVA_READ)
            {
                me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
            }
            /* No, write */
            else
            {
                /* Decide, what to write... */
                if (val != 0) me->msks.req_vals[offs] |=  mask;
                else          me->msks.req_vals[offs] &=~ mask;
                me->msks.req_msks[offs] |= mask;
                
                me->msks.pend = 1;
                /* May we perform write right now? */
                if (me->msks.rcvd)
                {
                    send_wrmsks_cmd(me);
                    me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
                }
                /* No, we should request read first... */
                else
                {
                    me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
                }
            }
        }
        else if (x == PANOV_UBS_CHAN_RST_ACCSTAT)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
                me->acc_stat = 0;
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == PANOV_UBS_CHAN_POWERON_CTR)
        {
            if (action == DRVA_WRITE) me->ff_poweron_count = val;
            ReturnChanGroup(devid, PANOV_UBS_CHAN_POWERON_CTR, 1,
                            &(me->ff_poweron_count), &zero_rflags);
        }
        else if (x == PANOV_UBS_CHAN_WATCHDOG_CTR)
        {
            if (action == DRVA_WRITE) me->ff_watchdog_count = val;
            ReturnChanGroup(devid, PANOV_UBS_CHAN_WATCHDOG_CTR, 1,
                            &(me->ff_watchdog_count), &zero_rflags);
        }
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec panov_ubs_main_info[] =
{
    {30, 1},  // SET,ACT,LIM
    {10, 0},  // MES
    {30, 1},  // commands
    {64, 0},  // statuses,interlocks
    {64, 1},  // interlock_masks,critical_masks
    {2,  0},  // padding
};

DEFINE_DRIVER(panov_ubs, "Panov UBS (modulator controller) for LIU",
              panov_ubs_init_mod, NULL,
              sizeof(privrec_t), panov_ubs_params,
              2, 3,
              NULL, 0,
              countof(panov_ubs_main_info), panov_ubs_main_info,
              0, NULL,
              panov_ubs_init_d, panov_ubs_term_d,
              panov_ubs_rw_p, NULL);
