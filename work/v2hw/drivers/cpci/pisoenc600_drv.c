#include "cxsd_driver.h"
#include "pci4624.h"

#include "drv_i/pisoenc600_drv_i.h"


enum
{
    PISOENC600_REG_ENABLE   = 0x00,
        PISOENC600_VAL_ENABLE     = 1,
        PISOENC600_VAL_DISABLE    = 0,

    PISOENC600_REG_SERIAL   = 0x04,

    PISOENC600_REG_CTL_base = 0xC0, PISOENC600_REG_CTL_inc = 8,
        PISOENC600_CTL_MODE_SHIFT = 0, PISOENC600_CTL_MODE_MASK = 3 << PISOENC600_CTL_MODE_SHIFT,
        PISOENC600_CTL_RST        = 1 << 2,
        PISOENC600_CTL_LATCH      = 1 << 3,
        PISOENC600_CTL_BSEL_SHIFT = 4, PISOENC600_CTL_BSEL_MASK = 3 << PISOENC600_CTL_BSEL_SHIFT,
        PISOENC600_CTL_HRRST      = 1 << 6,
        PISOENC600_CTL_CRST       = 1 << 7,

    PISOENC600_REG_VAL_base = 0xC0, PISOENC600_REG_VAL_inc = 8,

    PISOENC600_REG_DO       = 0xC4,

    PISOENC600_REG_DI_base  = 0xC4, PISOENC600_REG_DI_inc  = 8,
    
};

static inline int CTLR(int l)
{
    return PISOENC600_REG_CTL_base + l * PISOENC600_REG_CTL_inc;
}

static inline int VALR(int l)
{
    return PISOENC600_REG_VAL_base + l * PISOENC600_REG_VAL_inc;
}


typedef struct
{
    pci4624_vmt_t *lvmt;
    int            handle;

    int32          cword[PISOENC600_NUM_LINES];
    int8           outr;
} pisoenc600_privrec_t;


static inline int32 RB0(pisoenc600_privrec_t *me, int ofs)
{
    return me->lvmt->ReadBar0(me->handle, ofs);
}

static inline void  WB0(pisoenc600_privrec_t *me, int ofs, int32 value)
{
    me->lvmt->WriteBar0(me->handle, ofs, value);
}


static int  pisoenc600_init_d(int devid, void *devptr, 
                              int businfocount, int businfo[],
                              const char *auxinfo)
{
  pisoenc600_privrec_t   *me = (pisoenc600_privrec_t *)devptr;
  int                     l;

    me->lvmt   = pci4624_get_vmt();
    me->handle = me->lvmt->Open(devid, me,
                                0xe159, 0x0002, businfo[0],
                                PISOENC600_REG_SERIAL,

                                PCI4624_OP_NONE,
                                0, 0, 0,
                                0, 0, 0,

                                PCI4624_OP_NONE,
                                0, 0, 0,
                                   0,

                                NULL,

                                PCI4624_OP_WR,
                                0, PISOENC600_REG_ENABLE, sizeof(int32),
                                PISOENC600_VAL_DISABLE,

                                PCI4624_OP_NONE,
                                0, 0, 0,
                                0);
    if (me->handle < 0) return me->handle;

    WB0(me, PISOENC600_REG_ENABLE, PISOENC600_VAL_ENABLE);
    for (l = 0;  l < PISOENC600_NUM_LINES;  l++)
    {
        me->cword[l] = PISOENC600_CTL_RST | PISOENC600_CTL_LATCH;
        WB0(me, CTLR(l), me->cword[l]);
    }

    return DEVSTATE_OPERATING;
}

static void pisoenc600_term_d(int devid, void *devptr)
{
  pisoenc600_privrec_t   *me = (pisoenc600_privrec_t *)devptr;

    me->lvmt->Disconnect(devid);
}


static void pisoenc600_rw_p  (int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  pisoenc600_privrec_t   *me = (pisoenc600_privrec_t *)devptr;
    
  int                     n;  // channel N in values[] (loop ctl var)
  int                     x;  // channel indeX
  int                     l;  // Line #
  
  int                     cr;
  int                     vr;

  int32                   value;
  rflags_t                rflags;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) value = values[n];
        //action = DRVA_READ;
        ////DoDriverLog(devid, 0, "%s %c[%d] %d", __FUNCTION__, action==DRVA_WRITE?'w':'r', x, value);
        rflags = 0;
        switch (x)
        {
            case PISOENC600_CHAN_COUNTER_base    ... PISOENC600_CHAN_COUNTER_base        + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_COUNTER_base;
                cr = CTLR(l);
                vr = VALR(l);
                
                WB0(me, cr,
                    (me->cword[l] &~ PISOENC600_CTL_LATCH) |
                    (0 << PISOENC600_CTL_BSEL_SHIFT));
                value  = (RB0(me, vr) & 0xFF);
                
                WB0(me, cr,
                    (me->cword[l] &~ PISOENC600_CTL_LATCH) |
                    (1 << PISOENC600_CTL_BSEL_SHIFT));
                value |= (RB0(me, vr) & 0xFF) << 8;
                
                WB0(me, cr,
                    (me->cword[l] &~ PISOENC600_CTL_LATCH) |
                    (2 << PISOENC600_CTL_BSEL_SHIFT));
                value |= (RB0(me, vr) & 0xFF) << 16;
                
                WB0(me, cr,
                    (me->cword[l] &~ PISOENC600_CTL_LATCH) |
                    (3 << PISOENC600_CTL_BSEL_SHIFT));
                value |= (RB0(me, vr) & 0xFF) << 24;
                
                WB0(me, CTLR(l), me->cword[l]);
                break;

            case PISOENC600_CHAN_C_base            ... PISOENC600_CHAN_C_base            + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_C_base;
                value = (RB0(me, PISOENC600_REG_DI_base + l * PISOENC600_REG_DI_inc) >> 1) & 1;
                break;

            case PISOENC600_CHAN_HR_base           ... PISOENC600_CHAN_HR_base           + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_HR_base;
                value = (RB0(me, PISOENC600_REG_DI_base + l * PISOENC600_REG_DI_inc) >> 0) & 1;
                break;

            case PISOENC600_CHAN_MODE_base         ... PISOENC600_CHAN_MODE_base         + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_MODE_base;
                if (action == DRVA_WRITE)
                {
                    if (value < PISOENC600_MODE_QUADRANT) value = PISOENC600_MODE_QUADRANT;
                    if (value > PISOENC600_MODE_UNUSED)   value = PISOENC600_MODE_UNUSED;
                    me->cword[l] =
                        (me->cword[l] &~ PISOENC600_CTL_MODE_MASK) |
                        (value << PISOENC600_CTL_MODE_SHIFT);
                    WB0(me, CTLR(l), me->cword[l]);
                }
                value = ((me->cword[l]) & PISOENC600_CTL_MODE_MASK) >> PISOENC600_CTL_MODE_SHIFT;
                break;
                
            case PISOENC600_CHAN_DO_RST_base       ... PISOENC600_CHAN_DO_RST_base       + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_DO_RST_base;
                if (action == DRVA_WRITE  &&  value == CX_VALUE_COMMAND)
                {
                    WB0(me, CTLR(l), me->cword[l] &~ PISOENC600_CTL_RST);
                    WB0(me, CTLR(l), me->cword[l]);
                }
                value = 0;
                break;

            case PISOENC600_CHAN_ALLOW_HR_RST_base ... PISOENC600_CHAN_ALLOW_HR_RST_base + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_ALLOW_HR_RST_base;
                if (action == DRVA_WRITE)
                {
                    me->cword[l] =
                        (me->cword[l] &~ PISOENC600_CTL_HRRST) |
                        ((value & CX_VALUE_LIT_MASK)? PISOENC600_CTL_HRRST : 0);
                    WB0(me, CTLR(l), me->cword[l]);
                }
                value = (me->cword[l] & PISOENC600_CTL_HRRST)? CX_VALUE_LIT_MASK : 0;
                break;

            case PISOENC600_CHAN_ALLOW_CR_RST_base ... PISOENC600_CHAN_ALLOW_CR_RST_base + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_ALLOW_CR_RST_base;
                if (action == DRVA_WRITE)
                {
                    me->cword[l] =
                        (me->cword[l] &~ PISOENC600_CTL_CRST) |
                        ((value & CX_VALUE_LIT_MASK)? PISOENC600_CTL_CRST : 0);
                    WB0(me, CTLR(l), me->cword[l]);
                }
                value = (me->cword[l] & PISOENC600_CTL_CRST)? CX_VALUE_LIT_MASK : 0;
                break;

            case PISOENC600_CHAN_OUTRBx_base       ... PISOENC600_CHAN_OUTRBx_base       + PISOENC600_NUM_LINES-1:
                l = x - PISOENC600_CHAN_OUTRBx_base;
                if (action == DRVA_WRITE)
                {
                    me->outr =
                        (me->outr &~ (1 << l)) |
                        ((value & CX_VALUE_LIT_MASK)? (1 << l) : 0);
                    pisoenc600_rw_p(devid, me,
                                    PISOENC600_CHAN_OUTR8B, 1, NULL, DRVA_READ);
                }
                value = (me->outr >> l) & 1;
                break;

            case PISOENC600_CHAN_OUTR8B:
                if (action == DRVA_WRITE)
                {
                    me->outr = value;
                    for (l = 0;  l < 8;  l++)
                        pisoenc600_rw_p(devid, me,
                                        PISOENC600_CHAN_OUTRBx_base+l, 1, NULL, DRVA_READ);
                }
                value = me->outr;
                break;

            case PISOENC600_CHAN_HWADDR:
                value = RB0(me, PISOENC600_REG_SERIAL);
                break;
                
            default:
                value  = 0;
                rflags = CXRF_UNSUPPORTED;
        }
        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}
DEFINE_DRIVER(pisoenc600, "PISO-Encoder-600",
              NULL, NULL,
              sizeof(pisoenc600_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              pisoenc600_init_d, pisoenc600_term_d,
              pisoenc600_rw_p, NULL);
