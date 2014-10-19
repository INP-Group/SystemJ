#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/tvac320_drv_i.h"


enum
{
    DEVTYPE = 36
};



enum
{
    DESC_WRT_U         = 0x07,
    DESC_GET_U         = 0x0F,
    DESC_MES_U         = 0x1F,

    DESC_WRT_U_MIN     = 0x27,
    DESC_GET_U_MIN     = 0x2F,

    DESC_WRT_U_MAX     = 0x37,
    DESC_GET_U_MAX     = 0x3F,

    DESC_ENG_TVR       = 0x47,
    DESC_OFF_TVR       = 0x4F,

    DESC_WRT_ILKS_MASK = 0xD7,
    DESC_GET_ILKS_MASK = 0xDF,
    DESC_RST_ILKS      = 0xE7,
    DESC_MES_ILKS      = 0xEF,

    DESC_REBOOT        = 0xF7,
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
    }                    msks;
} privrec_t;

static psp_paramdescr_t tvac320_params[] =
{
    PSP_P_END()
};


static void tvac320_rst(int devid, void *devptr, int is_a_reset);
static void tvac320_in(int devid, void *devptr,
                       int desc, int datasize, uint8 *data);

static int tvac320_init_d(int devid, void *devptr, 
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
                                tvac320_rst, tvac320_in,
                                TVAC320_NUMCHANS * 2,
                                -1);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void tvac320_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void tvac320_rst(int   devid    __attribute__((unused)),
                        void *devptr,
                        int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
        bzero(&(me->msks), sizeof(me->msks));
//        me->msks.rcvd = 0;
//        me->msks.pend = 0;
    }
}

static void send_wrmsk_cmd(privrec_t *me)
{
  uint8  data[8];

    data[0] = DESC_WRT_ILKS_MASK;
    data[1] = (me->msks.cur_val &~ me->msks.req_msk) |
              (me->msks.req_val &  me->msks.req_msk);

    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
    
    me->msks.pend = 0;
}

static void tvac320_in(int devid, void *devptr __attribute__((unused)),
                       int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value

  int          x;
  int32        bits_vals[8];
  rflags_t     bits_flgs[8];

  canqelem_t   item;

#define ON_RW_IN(CHAN_N, DESC_WRT, DESC_GET)                    \
    case DESC_WRT:                                              \
    case DESC_GET:                                              \
        me->iface->q_erase_and_send_next(me->handle, -1, desc); \
        val = data[1] + ((data[2] & 0x0F) << 8);                \
        ReturnChanGroup(devid, CHAN_N, 1, &val, &zero_rflags);  \
        break;

#define ON_RDS_IN(CHAN_N)                                    \
    me->iface->q_erase_and_send_next(me->handle, -1, desc);  \
    for (x = 0;  x < 8;  x++)                                \
    {                                                        \
        bits_vals[x] = (data[1] >> x) & 1;                   \
        bits_flgs[x] = 0;                                    \
    }                                                        \
    ReturnChanGroup(devid, CHAN_N, 8, bits_vals, bits_flgs); \
    break;

    switch (desc)
    {
        ON_RW_IN(TVAC320_CHAN_SET_U,     DESC_GET_U,     DESC_WRT_U);
        ON_RW_IN(TVAC320_CHAN_SET_U_MIN, DESC_GET_U_MIN, DESC_WRT_U_MIN);
        ON_RW_IN(TVAC320_CHAN_SET_U_MAX, DESC_GET_U_MAX, DESC_WRT_U_MAX);

        case DESC_MES_U:
            me->iface->q_erase_and_send_next(me->handle, -1, desc);
            val = data[1] + (data[2] << 8);
            ReturnChanGroup(devid, TVAC320_CHAN_MES_U, 1, &val, &zero_rflags);
            break;

        case DESC_ENG_TVR:
        case DESC_OFF_TVR:
        case CANKOZ_DESC_GETDEVSTAT:
            ON_RDS_IN(TVAC320_CHAN_STAT_base);
            
        case DESC_MES_ILKS:
            ON_RDS_IN(TVAC320_CHAN_ILK_base);

        case DESC_WRT_ILKS_MASK:
        case DESC_GET_ILKS_MASK:
            /* Store */
            me->msks.cur_val = data[1];
            me->msks.rcvd    = 1;
            /* Do we have a pending write request? */
            if (me->msks.pend)
            {
                send_wrmsk_cmd(me);
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 1;
                item.data[0]  = DESC_GET_ILKS_MASK;
            }
            /* And perform standard mask actions */
            ON_RDS_IN(TVAC320_CHAN_LKM_base);

        case DESC_RST_ILKS:
        case DESC_REBOOT:
            me->iface->q_erase_and_send_next(me->handle, -1, desc);
            break;

        default:;
    }
}

static void tvac320_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int          n;       // channel N in values[] (loop ctl var)
  int          x;       // channel indeX
  int32        val;     // Value
  uint8        data[8];
  int          nc;

  canqelem_t   item;
  
  uint8        mask;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

#define ON_RW_RW(CHAN_N, DESC_WRT, DESC_GET)                             \
    else if (x == CHAN_N)                                                \
    {                                                                    \
        if (action == DRVA_WRITE)                                        \
        {                                                                \
            if (val < 0)    val = 0;                                     \
            if (val > 3000) val = 3000;                                  \
            item.datasize = 3;                                           \
            item.data[0]  = DESC_WRT;                                    \
            item.data[1]  = val & 0xFF;                                  \
            item.data[2]  = (val >> 8) & 0x0F;                           \
            me->iface->q_enqueue(me->handle, &item, QFE_ALWAYS);         \
        }                                                                \
        else                                                             \
        {                                                                \
            item.datasize = 1;                                           \
            item.data[0] = DESC_GET;                                     \
            me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST); \
        }                                                                \
    }

#define ON_CMD_RW(CHAN_N, DESC_CMD)                                      \
    else if (x == CHAN_N)                                                \
    {                                                                    \
        if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)           \
        {                                                                \
            item.datasize = 1;                                           \
            item.data[0] = DESC_CMD;                                     \
            me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST); \
        }                                                                \
                                                                         \
        val = 0;                                                         \
        ReturnChanGroup(devid, x, 1, &val, &zero_rflags);                \
    }

#define ON_RDS_RW(CHAN_N, DESC_CMD)                                  \
    else if (x >= CHAN_N  &&  x < CHAN_N + 8)                        \
    {                                                                \
        item.datasize = 1;                                           \
        item.data[0] = DESC_CMD;                                     \
        me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST); \
    }

        item.prio = CANKOZ_PRIO_UNICAST;
        if      (0);
        ON_RW_RW (TVAC320_CHAN_SET_U,     DESC_WRT_U,     DESC_GET_U)
        ON_RW_RW (TVAC320_CHAN_SET_U_MIN, DESC_WRT_U_MIN, DESC_GET_U_MIN)
        ON_RW_RW (TVAC320_CHAN_SET_U_MAX, DESC_WRT_U_MAX, DESC_GET_U_MAX)
        ON_CMD_RW(TVAC320_CHAN_TVR_ON,    DESC_ENG_TVR)
        ON_CMD_RW(TVAC320_CHAN_TVR_OFF,   DESC_OFF_TVR)
        ON_CMD_RW(TVAC320_CHAN_RST_ILK,   DESC_RST_ILKS)
        ON_CMD_RW(TVAC320_CHAN_REBOOT,    DESC_REBOOT)
        ON_RDS_RW(TVAC320_CHAN_STAT_base, CANKOZ_DESC_GETDEVSTAT)
        ON_RDS_RW(TVAC320_CHAN_ILK_base,  DESC_MES_ILKS)
        else if (x == TVAC320_CHAN_MES_U)
        {
            item.datasize = 1;
            item.data[0] = DESC_MES_U;
            me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
        }
        else if (x >= TVAC320_CHAN_LKM_base  &&  x < TVAC320_CHAN_LKM_base + 8)
        {
            /* Prepare the GET packet -- it will be required in all cases */
            item.datasize = 1;
            item.data[0]  = DESC_GET_ILKS_MASK;

            /* Read? */
            if (action == DRVA_READ)
            {
                me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
            }
            /* No, write */
            else
            {
                mask = 1 << (x - TVAC320_CHAN_LKM_base);
                /* Decide, what to write... */
                if (val != 0) me->msks.req_val |=  mask;
                else          me->msks.req_val &=~ mask;
                me->msks.req_msk |= mask;

                me->msks.pend = 1;
                /* May we perform write right now? */
                if (me->msks.rcvd)
                {
                    send_wrmsk_cmd(me);
                    me->iface->q_enqueue(me->handle, &item, QFE_IF_NONEORFIRST);
                }
                /* No, we should request read first... */
                else
                {
                    me->iface->q_enqueue(me->handle, &item, QFE_IF_ABSENT);
                }
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

static CxsdMainInfoRec tvac320_main_info[] =
{
};

DEFINE_DRIVER(tvac320, "Pachkov TVAC320 high-voltage controller",
              NULL, NULL,
              sizeof(privrec_t), tvac320_params,
              2, 3,
              NULL, 0,
              countof(tvac320_main_info)*0-1, tvac320_main_info,
              0, NULL,
              tvac320_init_d, tvac320_term_d,
              tvac320_rw_p, NULL);
