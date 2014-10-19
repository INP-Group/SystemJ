#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/yamkshd8_drv_i.h"


enum
{
    DEVTYPE = 31
};


enum
{
    DESC_WR_PARAM = 0x10,
    DESC_RD_PARAM = 0x11,
    DESC_GO       = 0x12,
    DESC_STOP     = 0x13,
};

enum
{
    PARAM_STATUS       = 0,
    PARAM_CONTROL      = 1,
    PARAM_MIN_VELOCITY = 2,
    PARAM_MAX_VELOCITY = 3,
    PARAM_ACCELERATION = 4,
    PARAM_T_ACCEL      = 5,
    PARAM_NUM_STEPS    = 6,
    PARAM_COUNTER      = 7,
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

} privrec_t;

static psp_paramdescr_t yamkshd8_params[] =
{
    PSP_P_END()
};


static void yamkshd8_rst(int devid, void *devptr, int is_a_reset);
static void yamkshd8_in(int devid, void *devptr,
                        int desc, int datasize, uint8 *data);

static int yamkshd8_init_d(int devid, void *devptr, 
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
                                yamkshd8_rst, yamkshd8_in,
                                YAMKSHD8_NUMCHANS * 2,
                                -1);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void yamkshd8_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void yamkshd8_rst(int   devid    __attribute__((unused)),
                         void *devptr,
                         int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
    }
}

#define IN_RW2(name)                                       \
    case PARAM_##name:                                     \
       val = (data[3] << 8) | data[4];                     \
       ReturnChanGroup(devid,                              \
                       YAMKSHD8_CHAN_##name##_base + l, 1, \
                       &val, &zero_rflags);                \
       break;
        
#define IN_RW4(name)                                       \
    case PARAM_##name:                                     \
       val = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6]; \
       ReturnChanGroup(devid,                              \
                       YAMKSHD8_CHAN_##name##_base + l, 1, \
                       &val, &zero_rflags);                \
       break;

static void yamkshd8_in(int devid, void *devptr __attribute__((unused)),
                        int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value
  int          l;
  int          pn;

    switch (desc)
    {
        case CANKOZ_DESC_GETDEVSTAT:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            for (l = 0;  l < YAMKSHD8_NUMLINES;  l++)
            {
                val = (data[1] >> l) & 1;
                ReturnChanGroup(devid,
                                YAMKSHD8_CHAN_GOING_base + l, 1,
                                &val, &zero_rflags);
            }
            break;

        case DESC_WR_PARAM:
        case DESC_RD_PARAM:
            l  = data[1];
            pn = data[2];
            me->iface->q_erase_and_send_next(me->handle,
                                             -3, desc, l, pn);
            if (l < 0  ||  l >= YAMKSHD8_NUMLINES)
            {
                /*!!! Ouch!!! */
                break;
            }
            switch (pn)
            {
                IN_RW2(MIN_VELOCITY)
                IN_RW2(MAX_VELOCITY)
                IN_RW2(ACCELERATION)
                IN_RW2(T_ACCEL)
                IN_RW4(NUM_STEPS)
                IN_RW4(COUNTER)
                default:
                    /*!!! Ouch... */
                    ;
            }
            break;
    }
}

#define RW_RW2(name, minval, maxval)                               \
    else if (g == YAMKSHD8_G_##name)                               \
    {                                                              \
        if (action == DRVA_WRITE)                                  \
        {                                                          \
            if (val < minval) val = minval;                        \
            if (val > maxval) val = maxval;                        \
            item.prio     = CANKOZ_PRIO_UNICAST;                   \
            item.datasize = 5;                                     \
            item.data[0]  = DESC_WR_PARAM;                         \
            item.data[1]  = l;                                     \
            item.data[2]  = PARAM_##name;                          \
            item.data[3]  = (val >>  8) & 0xFF;                    \
            item.data[4]  = val         & 0xFF;                    \
            me->iface->q_enqueue(me->handle,                       \
                                 &item, QFE_IF_NONEORFIRST);       \
        }                                                          \
        else                                                       \
        {                                                          \
            item.prio     = CANKOZ_PRIO_UNICAST;                   \
            item.datasize = 3;                                     \
            item.data[0]  = DESC_RD_PARAM;                         \
            item.data[1]  = l;                                     \
            item.data[2]  = PARAM_##name;                          \
            me->iface->q_enqueue(me->handle,                       \
                                 &item, QFE_IF_ABSENT);            \
        }                                                          \
    }

#define RW_RW4(name, minval, maxval)                               \
    else if (g == YAMKSHD8_G_##name)                               \
    {                                                              \
        if (action == DRVA_WRITE)                                  \
        {                                                          \
            if (val < minval) val = minval;                        \
            if (val > maxval) val = maxval;                        \
            item.prio     = CANKOZ_PRIO_UNICAST;                   \
            item.datasize = 7;                                     \
            item.data[0]  = DESC_WR_PARAM;                         \
            item.data[1]  = l;                                     \
            item.data[2]  = PARAM_##name;                          \
            item.data[3]  = (val >> 24) & 0xFF;                    \
            item.data[4]  = (val >> 16) & 0xFF;                    \
            item.data[5]  = (val >>  8) & 0xFF;                    \
            item.data[6]  =  val &        0xFF;                    \
            me->iface->q_enqueue(me->handle,                       \
                                 &item, QFE_IF_NONEORFIRST);       \
        }                                                          \
        else                                                       \
        {                                                          \
            item.prio     = CANKOZ_PRIO_UNICAST;                   \
            item.datasize = 3;                                     \
            item.data[0]  = DESC_RD_PARAM;                         \
            item.data[1]  = l;                                     \
            item.data[2]  = PARAM_##name;                          \
            me->iface->q_enqueue(me->handle,                       \
                                 &item, QFE_IF_ABSENT);            \
        }                                                          \
    }

static void yamkshd8_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t    *me       = (privrec_t *) devptr;
  int           n;       // channel N in values[] (loop ctl var)
  int           x;       // channel indeX
  int32         val;     // Value
  int           g;       // Group #
  int           l;       // Line #
  qfe_status_t  r;
  canqelem_t    item;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) val = values[n];
        else                      val = 0xFFFFFFFF; // That's just to make GCC happy

        g = x / YAMKSHD8_NUMLINES;
        l = x % YAMKSHD8_NUMLINES;
        
        if (0)
        {
        }
        else if (g == YAMKSHD8_G_GOING)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = CANKOZ_DESC_GETDEVSTAT;
            me->iface->q_enq_ons(me->handle,
                                 &item, QFE_IF_ABSENT);
        }
        else if (g == YAMKSHD8_G_COUNTER  &&  1)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 3;
            item.data[0]  = DESC_RD_PARAM;
            item.data[1]  = l;
            item.data[2]  = PARAM_COUNTER;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_ABSENT);
        }
        RW_RW2(MIN_VELOCITY, 0, 65535)
        RW_RW2(MAX_VELOCITY, 0, 65535)
        RW_RW2(ACCELERATION, 0, 65535)
        RW_RW2(T_ACCEL,      0, 65535)
        RW_RW4(NUM_STEPS,    -0x7FFFFFFF, +0x7FFFFFFF)
        else if (g == YAMKSHD8_G_GO)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 2;
                item.data[0]  = DESC_GO;
                item.data[1]  = l;
                r = me->iface->q_enq_ons(me->handle,
                                         &item, QFE_IF_NONEORFIRST);
                if (r == QFE_NOTFOUND)
                {
                    item.prio     = CANKOZ_PRIO_UNICAST;
                    item.datasize = 1;
                    item.data[0]  = CANKOZ_DESC_GETDEVSTAT;
                    me->iface->q_enq_ons(me->handle,
                                         &item, QFE_IF_ABSENT);
                }
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (g == YAMKSHD8_G_STOP)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 2;
                item.data[0]  = DESC_STOP;
                item.data[1]  = l;
                r = me->iface->q_enq_ons(me->handle,
                                         &item, QFE_IF_NONEORFIRST);
                if (r == QFE_NOTFOUND)
                {
                    item.prio     = CANKOZ_PRIO_UNICAST;
                    item.datasize = 1;
                    item.data[0]  = CANKOZ_DESC_GETDEVSTAT;
                    me->iface->q_enq_ons(me->handle,
                                         &item, QFE_IF_ABSENT);
                }
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (g == YAMKSHD8_G_RST_COUNTER  &&  0)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                item.prio     = CANKOZ_PRIO_UNICAST;
                item.datasize = 7;
                item.data[0]  = DESC_WR_PARAM;
                item.data[1]  = l;
                item.data[2]  = PARAM_COUNTER;
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
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec yamkshd8_main_info[] =
{
    {100, 0},  // Measurements and interlocks
    {100, 1},  // ..., settings, commands
};

DEFINE_DRIVER(yamkshd8, "Yaminov KShD8",
              NULL, NULL,
              sizeof(privrec_t), yamkshd8_params,
              2, 3,
              NULL, 0,
              countof(yamkshd8_main_info), yamkshd8_main_info,
              0, NULL,
              yamkshd8_init_d, yamkshd8_term_d,
              yamkshd8_rw_p, NULL);
