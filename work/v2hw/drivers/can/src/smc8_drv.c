#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/smc8_drv_i.h"


enum
{
    DEVTYPE = 31
};


enum
{
    DESC_ERROR      = 0x00,
    DESC_START_STOP = 0x70,
    DESC_WR_PARAM   = 0x71,
    DESC_RD_PARAM   = 0xF1,
};

enum
{
    PARAM_CSTATUS_REG = 0,
    PARAM_CONTROL_REG = 1,
    PARAM_START_FREQ  = 2,
    PARAM_FINAL_FREQ  = 3,
    PARAM_ACCEL       = 4,
    PARAM_NUM_STEPS   = 5,
    PARAM_POS_CNT     = 6,
};

enum
{
    ERR_NO        = 0,
    ERR_UNKCMD    = 1,
    ERR_LENGTH    = 2,
    ERR_INV_VAL   = 3,
    ERR_INV_STATE = 4,
    ERR_INV_PARAM = 9,
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

    struct
    {
        int    rcvd;
        int    pend;

        uint8  cur_val;
        uint8  req_val;
        uint8  req_msk;
    }                    ctl[SMC8_NUMLINES];
} privrec_t;

static psp_paramdescr_t smc8_params[] =
{
    PSP_P_END()
};


static void smc8_rst(int devid, void *devptr, int is_a_reset);
static void smc8_in (int devid, void *devptr,
                     int desc, int datasize, uint8 *data);

static int smc8_init_d(int devid, void *devptr, 
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
                                smc8_rst, smc8_in,
                                SMC8_NUMCHANS * 2,
                                -1);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void smc8_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void smc8_rst(int   devid    __attribute__((unused)),
                     void *devptr,
                     int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
        bzero(&(me->ctl), sizeof(me->ctl));
    }
}

static void send_wrctl_cmd(privrec_t *me, int l)
{
  canqelem_t    item;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 4;
    item.data[0]  = DESC_WR_PARAM;
    item.data[1]  = l;
    item.data[2]  = PARAM_CONTROL_REG;
    item.data[3]  = (me->ctl[l].cur_val &~ me->ctl[l].req_msk) |
                    (me->ctl[l].req_val &  me->ctl[l].req_msk);

    ////DoDriverLog(DEVID_NOT_IN_DRIVER, 0 | DRIVERLOG_C_PKTDUMP, "{71,02,01   cur_val=%02x req_val=%02x req_msk=%02x data[3]=%02x", me->ctl[l].cur_val, me->ctl[l].req_val, me->ctl[l].req_msk, item.data[3]);

    /*!!! Should use some form of enq! */
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, item.datasize, item.data);

    me->ctl[l].pend = 0;
}

static void ReturnCStatus(int devid, privrec_t *me, int l, uint8 b)
{
  int32        val;     // Value

    val = (b >> 1) & 1;
    ReturnChanGroup(devid, SMC8_CHAN_KM_base    + l, 1, &val, &zero_rflags);
    val = (b >> 2) & 1;
    ReturnChanGroup(devid, SMC8_CHAN_KP_base    + l, 1, &val, &zero_rflags);
    val = (b >> 4) & 15;
    ReturnChanGroup(devid, SMC8_CHAN_STATE_base + l, 1, &val, &zero_rflags);
    val = (b >> 7) & 1;
    ReturnChanGroup(devid, SMC8_CHAN_GOING_base + l, 1, &val, &zero_rflags);
}

static void EnqRD_PARAM(privrec_t *me, int l, int pn)
{
  canqelem_t    item;

    item.prio     = CANKOZ_PRIO_UNICAST;
    item.datasize = 3;
    item.data[0]  = DESC_RD_PARAM;
    item.data[1]  = l;
    item.data[2]  = pn;
    me->iface->q_enqueue(me->handle,
                         &item, QFE_IF_ABSENT);
}

static void smc8_in (int devid, void *devptr,
                     int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value
  int          l;
  int          pn;
  int          bit;

    if      (desc == DESC_RD_PARAM  ||
             desc == DESC_WR_PARAM)
    {
        if (datasize < 3)
        {
            DoDriverLog(devid, 0,
                        "DESC_%s_PARAM: dlc=%d, <3",
                        desc == DESC_RD_PARAM? "RD":"WR", datasize);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL,
                        desc == DESC_RD_PARAM? "DESC_RD_PARAM packet too short":"DESC_WR_PARAM packet too short");
            return;
        }

        l  = data[1];
        pn = data[2];
        if (l >= SMC8_NUMLINES)
        {
            DoDriverLog(devid, 0,
                        "DESC_%s_PARAM: line=%d, >%d",
                        desc == DESC_RD_PARAM? "RD":"WR", l, SMC8_NUMLINES-1);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL,
                        desc == DESC_RD_PARAM? "DESC_RD_PARAM line too big":"DESC_WR_PARAM line too big");
            return;
        }

        me->iface->q_erase_and_send_next(me->handle, -3, desc, l, pn);

        if      (pn == PARAM_CSTATUS_REG)
        {
            ReturnCStatus(devid, me, l, data[3]);
        }
        else if (pn == PARAM_CONTROL_REG)
        {
            me->ctl[l].cur_val = data[3];
            ////DoDriverLog(devid, 0 | DRIVERLOG_C_PKTDUMP, "{71,02,01      cur_val=%02x", me->ctl[l].cur_val);
            me->ctl[l].rcvd    = 1;
            /* Do we have a pending write request? */
            if (me->ctl[l].pend)
                send_wrctl_cmd(me, l);

            for (bit = 0;
                 bit <= (SMC8_g_CONTROL_LAST - SMC8_g_CONTROL_FIRST);
                 bit++)
            {
                val = (data[3] >> bit) & 1;
                ReturnChanGroup(devid,
                                (SMC8_g_CONTROL_FIRST + bit) * SMC8_NUMLINES + l,
                                1, &val, &zero_rflags);
            }
        }
        else if (pn == PARAM_START_FREQ  ||
                 pn == PARAM_FINAL_FREQ  ||
                 pn == PARAM_ACCEL)
        {
            val = (data[3] << 8) | data[4];
            ReturnChanGroup(devid,
                            /*!!! Here we use the fact that "groups" and "params" are in synced order */
                            (SMC8_CHAN_START_FREQ_base +
                             (pn - PARAM_START_FREQ) * SMC8_NUMLINES
                            ) + l,
                            1, &val, &zero_rflags);
        }
        else if (pn == PARAM_NUM_STEPS)
        {
            val = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
            ReturnChanGroup(devid, SMC8_CHAN_NUM_STEPS_base + l,
                            1, &val, &zero_rflags);
        }
        else if (pn == PARAM_POS_CNT)
        {
            val = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
            ReturnChanGroup(devid, SMC8_CHAN_COUNTER_base + l,
                            1, &val, &zero_rflags);
        }
    }
    else if (desc == DESC_START_STOP)
    {
        l  = data[1];
        me->iface->q_erase_and_send_next(me->handle, -3, desc, l, data[2]);
        ReturnCStatus(devid, me, l, data[3]);
    }
    else if (desc == DESC_ERROR)
    {
        fprintf(stderr, "DESC_ERROR(dlc=%d): %08x,%08x,%08x,%08x\n", datasize, data[1], data[2], data[3], data[4]);
        if (datasize < 3)
        {
            DoDriverLog(devid, 0,
                        "DESC_ERROR: dlc=%d, <3 -- terminating",
                        datasize);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "DESC_ERROR dlc<3");
            return;
        }
        if (data[1] == ERR_UNKCMD)
        {
            DoDriverLog(devid, 0,
                        "DESC_ERROR: ERR_UNKCMD cmd=0x%02x", 
                        data[1]);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "ERR_UNKCMD");
            return;
        }
        if (data[1] == ERR_LENGTH)
        {
            DoDriverLog(devid, 0,
                        "DESC_ERROR: ERR_LENGTH of cmd=0x%02x", 
                        data[2]);
            SetDevState(devid, DEVSTATE_OFFLINE, CXRF_REM_C_PROBL, "ERR_LENGTH");
            return;
        }
        if (data[1] == ERR_INV_VAL    ||
            data[1] == ERR_INV_STATE)
        {
            if (data[2] == DESC_WR_PARAM)
                EnqRD_PARAM(me, data[3], data[4]);
            me->iface->q_erase_and_send_next(me->handle, -3, data[2], data[3], data[4]);
            return;
        }
        if (data[1] == ERR_INV_PARAM)
        {
            me->iface->q_erase_and_send_next(me->handle, -3, data[2], data[3], data[4]);
            return;
        }
    }
}

static void smc8_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t    *me       = (privrec_t *) devptr;
  int           n;       // channel N in values[] (loop ctl var)
  int           x;       // channel indeX
  int32         val;     // Value
  int           g;       // Group #
  int           l;       // Line #
  qfe_status_t  r;
  canqelem_t    item;

  uint8         mask;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        g = x / SMC8_NUMLINES;
        l = x % SMC8_NUMLINES;
        
        if (0)
        {
        }
        else if (g == SMC8_g_GOING  ||  g == SMC8_g_STATE  ||
                 g == SMC8_g_KM     ||  g == SMC8_g_KP)
        {
            EnqRD_PARAM(me, l, PARAM_CSTATUS_REG);
        }
        else if (g == SMC8_g_COUNTER)
        {
            EnqRD_PARAM(me, l, PARAM_POS_CNT);
        }
        else if (g == SMC8_g_STOP)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                /* Prepare packet sans "Action" byte */
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 3;
                item.data[0]  = DESC_START_STOP;
                item.data[1]  = l;
                /* Erase queued "GO" commands, if any */
                item.data[2]  = 1;
                me->iface->q_foreach(me->handle,
                                     NULL, &item, QFE_ERASE_ALL);
                /* Send "STOP" -- both directly and enqueue (if direct-send fails) */
                item.data[2]  = 0;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, item.datasize, item.data);
                me->iface->q_enqueue (me->handle,
                                      &item, QFE_IF_NONEORFIRST);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (g == SMC8_g_GO)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                /*!!! That's weird!  Should check if there's any other "GO"
                      command in queue, and if so -- remove it.
                      Or, maybe, even more -- check last-known CStatus value
                      for "RUNNING" bit.
                 */
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 3;
                item.data[0]  = DESC_START_STOP;
                item.data[1]  = l;
                item.data[2]  = 1;
                me->iface->q_enqueue (me->handle,
                                      &item, QFE_IF_NONEORFIRST);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (g == SMC8_g_GO_N_STEPS)
        {
            if (action == DRVA_WRITE  &&  val != 0)
            {
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (g == SMC8_g_RST_COUNTER)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 7;
                item.data[0]  = DESC_WR_PARAM;
                item.data[1]  = l;
                item.data[2]  = PARAM_POS_CNT;
                item.data[3]  = 0;
                item.data[4]  = 0;
                item.data[5]  = 0;
                item.data[6]  = 0;
                r = me->iface->q_enqueue(me->handle,
                                         &item, QFE_IF_NONEORFIRST);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (g == SMC8_g_START_FREQ    ||
                 g == SMC8_g_FINAL_FREQ    ||
                 g == SMC8_g_ACCELERATION  ||
                 g == SMC8_g_NUM_STEPS)
        {
            if (action == DRVA_WRITE)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = (g == SMC8_g_NUM_STEPS)? 7 : 5;
                item.data[0]  = DESC_WR_PARAM;
                item.data[1]  = l;
                /*!!! Here we use the fact that "groups" and "params" are in synced order */
                item.data[2]  = PARAM_START_FREQ + (g - SMC8_g_START_FREQ);
                if (g == SMC8_g_NUM_STEPS)
                {
                    item.data[3]  = (val >> 24) & 0xFF;
                    item.data[4]  = (val >> 16) & 0xFF;
                    item.data[5]  = (val >>  8) & 0xFF;
                    item.data[6]  =  val        & 0xFF;
                }
                else
                {
                    if (val < 0)     val = 0;
                    if (val > 65535) val = 65535;
                    item.data[3]  = (val >>  8) & 0xFF;
                    item.data[4]  =  val        & 0xFF;
                }
                r = me->iface->q_enqueue(me->handle,
                                         &item, QFE_ALWAYS); /*!!! And should REPLACE if such packet is present...  */
            }
            else
            {
                EnqRD_PARAM(me, l,
                            /*!!! Here we use the fact that "groups" and "params" are in synced order */
                            PARAM_START_FREQ + (g - SMC8_g_START_FREQ));
            }
        }
        else if (g >= SMC8_g_CONTROL_FIRST  &&
                 g <= SMC8_g_CONTROL_LAST)
        {
            if (action == DRVA_READ)
                EnqRD_PARAM(me, l, PARAM_CONTROL_REG);
            else
            {
                ////DoDriverLog(devid, 0 | DRIVERLOG_C_PKTDUMP, "{71,02,01   PRE   cur_val=%02x", me->ctl[l].cur_val);
                mask = 1 << (g - SMC8_g_CONTROL_FIRST);
                if (val != 0) me->ctl[l].req_val |=  mask;
                else          me->ctl[l].req_val &=~ mask;
                me->ctl[l].req_msk |= mask;
                ////DoDriverLog(devid, 0 | DRIVERLOG_C_PKTDUMP, "{71,02,01   PST   cur_val=%02x", me->ctl[l].cur_val);

                me->ctl[l].pend = 1;
                /* May we perform write right now? */
                if (me->ctl[l].rcvd)
                    send_wrctl_cmd(me, l);
                /* No, we should request read first... */
                else
                    EnqRD_PARAM(me, l, PARAM_CONTROL_REG);
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

static CxsdMainInfoRec smc8_main_info[] =
{
    {64,  0},  // Measurements and interlocks
    {136, 1},  // ..., settings, commands
};

DEFINE_DRIVER(smc8, "Yaminov KShD8",
              NULL, NULL,
              sizeof(privrec_t), smc8_params,
              2, 3,
              NULL, 0,
              countof(smc8_main_info), smc8_main_info,
              0, NULL,
              smc8_init_d, smc8_term_d,
              smc8_rw_p, NULL);
