#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/yamctr_drv_i.h"


enum
{
    DEVTYPE = 0x10
};

#define ED(t,w,g,n) ((t << 6) | (w << 5) | (g << 4) | n)

enum
{
    DESC_RD_SET_V            = ED(0, 0, 0, 1),
    DESC_RD_SET_I_MAX        = ED(0, 0, 0, 2),
    DESC_WR_SET_V            = ED(0, 1, 0, 1),
    DESC_WR_SET_I_MAX        = ED(0, 1, 0, 2),
    
    DESC_RD_CUR_COORD        = ED(1, 0, 0, 0),
    DESC_RD_CUR_V            = ED(1, 0, 0, 1),
    DESC_RD_CUR_I            = ED(1, 0, 0, 2),

    DESC_RD_ILK              = ED(2, 0, 0, 0),
    DESC_RD_LKM              = ED(2, 0, 0, 1),
    DESC_WR_LKM              = ED(2, 1, 0, 1),

    DESC_RD_SET_PERIOD       = ED(3, 0, 0, 0),
    DESC_RD_SET_V_PROP_COEFF = ED(3, 0, 0, 3),
    DESC_RD_SET_V_TIME_COEFF = ED(3, 0, 0, 4),
    DESC_RD_SET_I_PROP_COEFF = ED(3, 0, 0, 5),
    DESC_RD_SET_I_TIME_COEFF = ED(3, 0, 0, 6),
    DESC_WR_SET_PERIOD       = ED(3, 1, 0, 0),
    DESC_WR_SET_V_PROP_COEFF = ED(3, 1, 0, 3),
    DESC_WR_SET_V_TIME_COEFF = ED(3, 1, 0, 4),
    DESC_WR_SET_I_PROP_COEFF = ED(3, 1, 0, 5),
    DESC_WR_SET_I_TIME_COEFF = ED(3, 1, 0, 6),

    DESC_GO   = 0x0D,
    DESC_STOP = 0x0E,
};

static rflags_t  zero_rflags = 0;
static rflags_t  unsp_rflags = CXRF_UNSUPPORTED;

typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;

} privrec_t;

static psp_paramdescr_t yamctr_params[] =
{
    PSP_P_END()
};


static void yamctr_rst(int devid, void *devptr, int is_a_reset);
static void yamctr_in(int devid, void *devptr,
                      int desc, int datasize, uint8 *data);

static int yamctr_init_d(int devid, void *devptr, 
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
                                yamctr_rst, yamctr_in,
                                YAMCTR_NUMCHANS * 2,
                                -1);
    if (me->handle < 0) return me->handle;
    
    return DEVSTATE_OPERATING;
}

static void yamctr_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;

    me->iface->disconnect(devid);
}

static void yamctr_rst(int   devid    __attribute__((unused)),
                       void *devptr,
                       int   is_a_reset)
{
  privrec_t  *me       = (privrec_t *) devptr;

    if (is_a_reset)
    {
    }
}

#define IN_RD(name)                                           \
   case __CX_CONCATENATE(DESC_RD_,name):                      \
       me->iface->q_erase_and_send_next(me->handle, 1, desc); \
       val = (int8)data[1];                                   \
       ReturnChanGroup(devid, __CX_CONCATENATE(YAMCTR_CHAN_,name), 1, &val, &zero_rflags); \
       break;
       
#define IN_WR(name)                                           \
   case __CX_CONCATENATE(DESC_WR_,name):                      \
   case __CX_CONCATENATE(DESC_RD_,name):                      \
       me->iface->q_erase_and_send_next(me->handle, -1, desc); \
       val = (int8)data[1];                                   \
       ReturnChanGroup(devid, __CX_CONCATENATE(YAMCTR_CHAN_,name), 1, &val, &zero_rflags); \
       break;

static void yamctr_in(int devid, void *devptr __attribute__((unused)),
                      int desc, int datasize, uint8 *data)
{
  privrec_t   *me       = (privrec_t *) devptr;
  int32        val;     // Value
  int          n;

    switch (desc)
    {
        case DESC_RD_CUR_COORD:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            val = (data[1] << 16) | (data[2] << 8) | (data[3]);
            if ((val & (1 << 23)) != 0) val |= 0xFF000000;
            ReturnChanGroup(devid, YAMCTR_CHAN_CUR_COORD, 1, &val, &zero_rflags);
            break;
        case DESC_GO:
        case DESC_STOP:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            break;
        case DESC_RD_ILK:
            me->iface->q_erase_and_send_next(me->handle, 1, desc);
            for (n = 0;  n < YAMCTR_CHAN_ILK_count;  n++)
            {
                val = (data[1] >> n) & 1;
                ReturnChanGroup(devid, YAMCTR_CHAN_ILK_base + n, 1, &val, &zero_rflags);
            }
            break;
        IN_RD(CUR_V)
        IN_RD(CUR_I)
        IN_WR(SET_V)
        IN_WR(SET_I_MAX)
        IN_WR(SET_V_PROP_COEFF)
        IN_WR(SET_V_TIME_COEFF)
        IN_WR(SET_I_PROP_COEFF)
        IN_WR(SET_I_TIME_COEFF)
    }
}

#define RW_RD(name)                                          \
    else if (x == __CX_CONCATENATE(YAMCTR_CHAN_,name))       \
    {                                                        \
        item.prio     = CANKOZ_PRIO_UNICAST;                 \
        item.datasize = 1;                                   \
        item.data[0]  = __CX_CONCATENATE(DESC_RD_,name);     \
        me->iface->q_enqueue(me->handle,                     \
                             &item, QFE_IF_NONEORFIRST);     \
    }

#define RW_RW(name)                                          \
    else if (x == __CX_CONCATENATE(YAMCTR_CHAN_,name))       \
    {                                                        \
        if (action == DRVA_WRITE)                            \
        {                                                    \
            item.prio     = CANKOZ_PRIO_UNICAST;             \
            item.datasize = 2;                               \
            item.data[0]  = __CX_CONCATENATE(DESC_WR_,name); \
            item.data[1]  = (int8)val;                       \
            me->iface->q_enqueue(me->handle,                 \
                                 &item, QFE_IF_NONEORFIRST); \
        }                                                    \
        else                                                 \
        {                                                    \
            item.prio     = CANKOZ_PRIO_UNICAST;             \
            item.datasize = 1;                               \
            item.data[0]  = __CX_CONCATENATE(DESC_RD_,name); \
            me->iface->q_enqueue(me->handle,                 \
                                 &item, QFE_IF_NONEORFIRST); \
        }                                                    \
    }


static void yamctr_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
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

        if      (x == YAMCTR_CHAN_GO)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                    item.prio     = CANKOZ_PRIO_UNICAST;
                    item.datasize = 1;
                    item.data[0]  = DESC_GO;
                    me->iface->q_enqueue(me->handle,
                                         &item, QFE_IF_NONEORFIRST);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x == YAMCTR_CHAN_STOP)
        {
            if (action == DRVA_WRITE  &&  val == CX_VALUE_COMMAND)
            {
                me->iface->q_clear   (me->handle);
                data[0] = DESC_STOP;
                me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 1, data);
                for (nc = 0;  nc < YAMCTR_NUMCHANS;  nc++)
                    yamctr_rw_p(devid, devptr, nc, 1, NULL, DRVA_READ);
            }
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &zero_rflags);
        }
        else if (x >= YAMCTR_CHAN_ILK_base  &&  x <= YAMCTR_CHAN_ILK_base + YAMCTR_CHAN_ILK_count - 1)
        {
            item.prio     = CANKOZ_PRIO_UNICAST;
            item.datasize = 1;
            item.data[0]  = DESC_RD_ILK;
            me->iface->q_enqueue(me->handle,
                                 &item, QFE_IF_NONEORFIRST);
        }
        RW_RD(CUR_COORD)
        RW_RD(CUR_V)
        RW_RD(CUR_I)
        RW_RW(SET_V)
        RW_RW(SET_I_MAX)
        RW_RW(SET_V_PROP_COEFF)
        RW_RW(SET_V_TIME_COEFF)
        RW_RW(SET_I_PROP_COEFF)
        RW_RW(SET_I_TIME_COEFF)
        else
        {
            val = 0;
            ReturnChanGroup(devid, x, 1, &val, &unsp_rflags);
        }
    }
}


/* Metric */

static CxsdMainInfoRec yamctr_main_info[] =
{
    {20, 0},  // Measurements and interlocks
    {30, 1},  // interlock-masks, settings, commands
};

DEFINE_DRIVER(yamctr, NULL,
              NULL, NULL,
              sizeof(privrec_t), yamctr_params,
              2, 3,
              NULL, 0,
              countof(yamctr_main_info), yamctr_main_info,
              0, NULL,
              yamctr_init_d, yamctr_term_d,
              yamctr_rw_p, NULL);
