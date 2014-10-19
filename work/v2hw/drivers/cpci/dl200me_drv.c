#include "timeval_utils.h"
#include "cxsd_driver.h"
#include "pci4624.h"
#include "drv_i/dl200me_drv_i.h"


enum
{
    DL200ME_REG_T_BASE = 0x00,
    DL200ME_REG_AILK   = 0x40,
    DL200ME_REG_CSR    = 0x44,
    DL200ME_REG_OFF    = 0x48,
    DL200ME_REG_IINH   = 0x4C,
    DL200ME_REG_ILAS   = 0x50,
    DL200ME_REG_IRQS   = 0x54,
    DL200ME_REG_SERIAL = 0x58,
};

enum
{
    DL200ME_CSR_RUN      = 0,
    DL200ME_CSR_RUN_MODE = 1,
    DL200ME_CSR_BLK      = 2,
    DL200ME_CSR_BUSY     = 3,
    DL200ME_CSR_INTA     = 4,
    DL200ME_CSR_INTB     = 5,
};

enum
{
    DL200ME_IRQS_FIN = 0,
    DL200ME_IRQS_ILK = 1,
};

enum {AUTOSHOT_HBT_USECS = 100000}; // 100ms is enough to support one-second-granularity shot periods


typedef struct
{
    pci4624_vmt_t  *lvmt;
    int             handle;

    int             slow;
    uint16          lkmsk;
    int32           ilas16;  // signed int32, because <0 means "don't init"

    int32           auto_shot;
    int32           auto_msec;
    struct timeval  last_auto_shot;

    struct timeval  last_shot;
    int32           last_ablk;
    int32           last_was_fin;
    int32           last_was_ilk;
    int32           serial;

    int             is_locked;
} dl200me_privrec_t;

static psp_paramdescr_t dl200me_params[] =
{
    PSP_P_FLAG("fast",   dl200me_privrec_t, slow,     0, 1),
    PSP_P_FLAG("slow",   dl200me_privrec_t, slow,     1, 0),
    PSP_P_INT ("lkmsk",  dl200me_privrec_t, lkmsk,    0, 0, 0),
    PSP_P_INT ("ilas16", dl200me_privrec_t, ilas16,  -1, 0, 0),
    PSP_P_END()
};


static void dl200me_irq_p(int devid, void *devptr,
                          int num_irqs, int got_mask);
static void dl200me_autoshot_hbt(int devid, void *devptr,
                                 sl_tid_t tid,
                                 void *privptr);

static int  dl200me_init_d(int devid, void *devptr, 
                           int businfocount, int businfo[],
                           const char *auxinfo)
{
  dl200me_privrec_t   *me = (dl200me_privrec_t *)devptr;

    me->auto_shot = 0;
    me->auto_msec = 5000;

    me->lvmt   = pci4624_get_vmt();
    me->handle = me->lvmt->Open(devid, me,
                                0x4624, me->slow? 0xDE02 : 0xDE01, businfo[0],
                                DL200ME_REG_SERIAL,

                                PCI4624_OP_RD,
                                0, DL200ME_REG_IRQS, sizeof(int32),
                                -1, 0, PCI4624_COND_NONZERO,

                                PCI4624_OP_WR,
                                0, DL200ME_REG_IRQS, sizeof(int32),
                                0,

                                dl200me_irq_p,

                                PCI4624_OP_WR,
                                0, DL200ME_REG_CSR, sizeof(int32),
                                0,

                                PCI4624_OP_WR,
                                0, DL200ME_REG_IRQS, sizeof(int32),
                                0);
    if (me->handle < 0) return me->handle;

    me->last_ablk = me->lvmt->ReadBar0(me->handle, DL200ME_REG_AILK) & 0xFFFF;
    me->serial    = me->lvmt->ReadBar0(me->handle, DL200ME_REG_SERIAL);

    /* Start heartbeat */
    sl_enq_tout_after(devid, devptr, AUTOSHOT_HBT_USECS, dl200me_autoshot_hbt, NULL);

    /* Enable interrupts */
    me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,
                        me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) | (1 << DL200ME_CSR_INTA) | (1 << DL200ME_CSR_INTB));

    /* Write ILAS16/"Non-interlockable outputs", if requested */
    if (me->ilas16 >= 0)
        me->lvmt->WriteBar0(me->handle, DL200ME_REG_ILAS, me->ilas16 & 0xFFFF);

    return DEVSTATE_OPERATING;
}

static void dl200me_term_d(int devid, void *devptr)
{
  dl200me_privrec_t   *me = (dl200me_privrec_t *)devptr;

    me->lvmt->Disconnect(devid);
}

static void dl200me_rw_p  (int devid, void *devptr, int firstchan, int count, int32 *values, int action)
{
  dl200me_privrec_t   *me = (dl200me_privrec_t *)devptr;
    
  int                  n;  // channel N in values[] (loop ctl var)
  int                  x;  // channel indeX
  int                  l;  // Line #

  int32                c;
  int32                value;
  rflags_t             rflags;
  int32                maxv;
  
  struct timeval       now;
  struct timeval       msc; // timeval-representation of MilliSeConds
  struct timeval       dln; // DeadLiNe

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;
        if (action == DRVA_WRITE) value = values[n];
        rflags = 0;
        switch (x)
        {
            case DL200ME_CHAN_T_BASE    ... DL200ME_CHAN_T_BASE+DL200ME_NUMOUTPUTS-1:
                l = x - DL200ME_CHAN_T_BASE;
                if (action == DRVA_WRITE)
                {
                    maxv = (me->slow? 0xEFFFFF : 0xEFFF);
                    if (value < 0)    value = 0;
                    if (value > maxv) value = maxv;
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_T_BASE + l*4, value);
                }
                value = me->lvmt->ReadBar0(me->handle, DL200ME_REG_T_BASE + l*4);
                break;
            
            case DL200ME_CHAN_OFF_BASE  ... DL200ME_CHAN_OFF_BASE+DL200ME_NUMOUTPUTS-1:
                l = x - DL200ME_CHAN_OFF_BASE;
                if (action == DRVA_WRITE)
                {
                    value = value != 0;
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_OFF,
                                        (
                                         (me->lvmt->ReadBar0(me->handle, DL200ME_REG_OFF) &~ (1 << l)) | (value << l)
                                        )
                                        |
                                        (me->is_locked? me->lkmsk : 0)
                                       );
                    dl200me_rw_p(devid, me,
                                 DL200ME_CHAN_OFF16, 1, NULL, DRVA_READ);
                }
                value = (me->lvmt->ReadBar0(me->handle, DL200ME_REG_OFF) >> l) & 1;
                break;
            
            case DL200ME_CHAN_IINH_BASE ... DL200ME_CHAN_IINH_BASE+DL200ME_NUMOUTPUTS-1:
                l = x - DL200ME_CHAN_IINH_BASE;
                if (action == DRVA_WRITE)
                {
                    value = value != 0;
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_IINH,
                                        (me->lvmt->ReadBar0(me->handle, DL200ME_REG_IINH) &~ (1 << l)) | (value << l));
                    dl200me_rw_p(devid, me,
                                 DL200ME_CHAN_IINH16, 1, NULL, DRVA_READ);
                }
                value = (me->lvmt->ReadBar0(me->handle, DL200ME_REG_IINH) >> l) & 1;
                if (l == 14  &&  0) DoDriverLog(devid, 0, "IINH#14: %c value=%d", action==DRVA_READ?'r':'w', value);
                break;
            
            case DL200ME_CHAN_ILAS_BASE ... DL200ME_CHAN_ILAS_BASE+DL200ME_NUMOUTPUTS-1:
                l = x - DL200ME_CHAN_ILAS_BASE;
                if (action == DRVA_WRITE)
                {
                    value = value != 0;
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_ILAS,
                                        (me->lvmt->ReadBar0(me->handle, DL200ME_REG_ILAS) &~ (1 << l)) | (value << l));
                    dl200me_rw_p(devid, me,
                                 DL200ME_CHAN_ILAS16, 1, NULL, DRVA_READ);
                }
                value = (me->lvmt->ReadBar0(me->handle, DL200ME_REG_ILAS) >> l) & 1;
                break;
            
            case DL200ME_CHAN_AILK_BASE ... DL200ME_CHAN_AILK_BASE+DL200ME_NUMOUTPUTS-1:
                l = x - DL200ME_CHAN_AILK_BASE;
                value = (me->last_ablk >> l) & 1;
                break;

            case DL200ME_CHAN_AILK16:
                value = me->last_ablk & 0xFFFF;
                break;

            case DL200ME_CHAN_OFF16:
                if (action == DRVA_WRITE)
                {
                    if (me->is_locked) value |= me->lkmsk;
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_OFF, value & 0xFFFF);
                    for (l = 0;  l < DL200ME_NUMOUTPUTS;  l++)
                        dl200me_rw_p(devid, me,
                                     DL200ME_CHAN_OFF_BASE+l, 1, NULL, DRVA_READ);
                }
                value = me->lvmt->ReadBar0(me->handle, DL200ME_REG_OFF) & 0xFFFF;
                break;

            case DL200ME_CHAN_IINH16:
                if (action == DRVA_WRITE)
                {
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_IINH, value & 0xFFFF);
                    for (l = 0;  l < DL200ME_NUMOUTPUTS;  l++)
                        dl200me_rw_p(devid, me,
                                     DL200ME_CHAN_IINH_BASE+l, 1, NULL, DRVA_READ);
                }
                value = me->lvmt->ReadBar0(me->handle, DL200ME_REG_IINH) & 0xFFFF;
                break;

            case DL200ME_CHAN_ILAS16:
                if (action == DRVA_WRITE)
                {
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_ILAS, value & 0xFFFF);
                    for (l = 0;  l < DL200ME_NUMOUTPUTS;  l++)
                        dl200me_rw_p(devid, me,
                                     DL200ME_CHAN_ILAS_BASE+l, 1, NULL, DRVA_READ);
                }
                value = me->lvmt->ReadBar0(me->handle, DL200ME_REG_ILAS) & 0xFFFF;
                break;

            case DL200ME_CHAN_DO_SHOT:
                if (action == DRVA_WRITE  &&  value == CX_VALUE_COMMAND)
                {
                    c = me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR);
                    if (me->slow)
                    {
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,
                                            c | (1 << DL200ME_CSR_RUN));
#if 0
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,/*!!!*/
                                            /*Setting mode:=PROG isn't what we need, setting up PROG bit is enough*/
                                            (c /*&~ (3 << DL200ME_CSR_RUN_MODE)*/)
                                            |
                                            (DL200ME_RUN_MODE_SLOW_PROG << DL200ME_CSR_RUN_MODE)
                                            |
                                            (1 << DL200ME_CSR_RUN));
                        c |= (1 << DL200ME_CSR_RUN);
#endif
                    }
                    else
                    {
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,
                                            (c &~ (1 << DL200ME_CSR_RUN_MODE))
                                            |
                                            (1 << DL200ME_CSR_RUN));
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR, c);
                    }
                }
                value = 0;
                break;

            case DL200ME_CHAN_RUN_MODE:
                if (me->slow)
                {
                    if (action == DRVA_WRITE)
                    {
                        if (value < 0) value = 0;
                        if (value > 3) value = 3;
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,
                                            (me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) &~ (3 << DL200ME_CSR_RUN_MODE)) | (value << DL200ME_CSR_RUN_MODE));
                    }
                    value = (me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) >> DL200ME_CSR_RUN_MODE) & 3;
                }
                else
                {
                    if (action == DRVA_WRITE)
                    {
                        value = !(value != 0);
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,
                                            (me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) &~ (1 << DL200ME_CSR_RUN_MODE)) | (value << DL200ME_CSR_RUN_MODE));
                    }
                    value = !((me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) >> DL200ME_CSR_RUN_MODE) & 1);
                }
                break;

            case DL200ME_CHAN_ALLOFF:
                if (me->slow)
                {
                    value  = 0;
                    rflags = CXRF_UNSUPPORTED;
                }
                else
                {
                    if (action == DRVA_WRITE)
                    {
                        value = !(value != 0);
                        me->lvmt->WriteBar0(me->handle, DL200ME_REG_CSR,
                                            (me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) &~ (1 << DL200ME_CSR_BLK)) | (value << DL200ME_CSR_BLK));
                    }
                    value = !((me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR) >> DL200ME_CSR_BLK) & 1);
                }
                break;

            case DL200ME_CHAN_AUTO_SHOT:
                if (action == DRVA_WRITE)
                {
                    value = value != 0;
                    me->auto_shot = value;
                }
                value = me->auto_shot;
                break;

            case DL200ME_CHAN_AUTO_MSEC:
                if (action == DRVA_WRITE)
                {
                    if (value < 0)            value = 0;
                    if (value > 1000*86400*7) value = 1000*86400*7; // Max 1wk period
                    me->auto_msec = value;
                }
                value = me->auto_msec;
                break;

            case DL200ME_CHAN_SOFT_LOCK:
                if (action == DRVA_WRITE)
                {
                    me->is_locked = value != 0;

                    c = me->lvmt->ReadBar0(me->handle, DL200ME_REG_OFF) & 0xFFFF;
                    if (me->is_locked)
                        c  |= me->lkmsk;
                    else
                        c &=~ me->lkmsk;
                    me->lvmt->WriteBar0(me->handle, DL200ME_REG_OFF, c);

                    for (l = 0;  l < DL200ME_NUMOUTPUTS;  l++)
                        dl200me_rw_p(devid, me,
                                     DL200ME_CHAN_OFF_BASE+l, 1, NULL, DRVA_READ);
                    dl200me_rw_p(devid, me,
                                 DL200ME_CHAN_OFF16, 1, NULL, DRVA_READ);
                }
                value = me->is_locked;
                break;

            case DL200ME_CHAN_WAS_SHOT:
                gettimeofday(&now, NULL);
                /*
                 Note: instead of checking for AFTER(now,last+1sec), which
                 would require a copy of me->last_shot, we employ
                 a mathematically equivalent AFTER(now-1sec,last).
                 */
                now.tv_sec -= 1;
                value = TV_IS_AFTER(now, me->last_shot)? 0 : 1;
                break;

            case DL200ME_CHAN_WAS_FIN:
                value = me->last_was_fin;
                break;

            case DL200ME_CHAN_WAS_ILK:
                value = me->last_was_ilk;
                break;

            case DL200ME_CHAN_AUTO_LEFT:
                value = -1000; // -1s
                if (me->auto_shot  &&  me->auto_msec > 0)
                {
                    gettimeofday(&now, NULL);
                    msc.tv_sec   =  me->auto_msec / 1000;
                    msc.tv_usec  = (me->auto_msec % 1000) * 1000;
                    timeval_add(&dln, &(me->last_auto_shot), &msc);
                    if (timeval_subtract(&msc, &dln, &now) == 0)
                        value = msc.tv_sec * 1000 + msc.tv_usec / 1000;
                }
                break;

            case DL200ME_CHAN_SERIAL:
                value = me->serial;
                break;
                
            default:
                value  = 0;
                rflags = CXRF_UNSUPPORTED;
        }
        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}

static void dl200me_irq_p(int devid, void *devptr,
                          int num_irqs, int got_mask)
{
  dl200me_privrec_t   *me = (dl200me_privrec_t *)devptr;

    //fprintf(stderr, "got_mask=%d\n", got_mask);
    gettimeofday(&(me->last_shot), NULL);
    me->last_ablk = me->lvmt->ReadBar0(me->handle, DL200ME_REG_AILK);
    me->last_was_fin = (got_mask & (1 << DL200ME_IRQS_FIN)) != 0;
    me->last_was_ilk = (got_mask & (1 << DL200ME_IRQS_ILK)) != 0;
    dl200me_rw_p(devid, me,
                 DL200ME_CHAN_AILK16,    1,                  NULL, DRVA_READ);
    dl200me_rw_p(devid, me,
                 DL200ME_CHAN_AILK_BASE, DL200ME_NUMOUTPUTS, NULL, DRVA_READ);
    dl200me_rw_p(devid, me,
                 DL200ME_CHAN_WAS_FIN,   1,                  NULL, DRVA_READ);
    dl200me_rw_p(devid, me,
                 DL200ME_CHAN_WAS_ILK,   1,                  NULL, DRVA_READ);
}

static void dl200me_autoshot_hbt(int devid, void *devptr,
                                 sl_tid_t tid,
                                 void *privptr)
{
  dl200me_privrec_t   *me = (dl200me_privrec_t *)devptr;
  struct timeval       now;
  struct timeval       msc; // timeval-representation of MilliSeConds
  int32                csr;
  int32                on = CX_VALUE_COMMAND;

    sl_enq_tout_after(devid, devptr, AUTOSHOT_HBT_USECS, dl200me_autoshot_hbt, NULL);

    /* Is auto-shot on? */
    if (me->auto_shot  &&  me->auto_msec > 0)
    {
        gettimeofday(&now, NULL);
        msc.tv_sec   =  me->auto_msec / 1000;
        msc.tv_usec  = (me->auto_msec % 1000) * 1000;
        timeval_subtract(&now, &now, &msc);
        /* If the period elapsed? */
        if (TV_IS_AFTER(now, me->last_auto_shot))
        {
            csr = me->lvmt->ReadBar0(me->handle, DL200ME_REG_CSR);
            /* Is mode suitable for auto-shot?  */
            if (
                (me->slow   &&
                 (csr >> DL200ME_CSR_RUN_MODE) & 1 // Here we use the fact that both MODE_SLOW_PROG and MODE_SLOW_P_E have bit 1 on
                )
                ||
                (!me->slow  &&
                 /* Note that we use PROG=1/EXT=0, while in device it is inverted (PROG=0/EXT=1) */
                 (!((csr >> DL200ME_CSR_RUN_MODE) & 1)) == DL200ME_RUN_MODE_FAST_PROG
                )
               )
            {
//            fprintf(stderr, "zz %ld\n", (long int)time(NULL));
                gettimeofday(&(me->last_auto_shot), NULL);
                dl200me_rw_p(devid, me,
                             DL200ME_CHAN_DO_SHOT, 1, &on, DRVA_WRITE);
            }
        }
    }
}


DEFINE_DRIVER(dl200me, "DL200ME delay line",
              NULL, NULL,
              sizeof(dl200me_privrec_t), dl200me_params,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              dl200me_init_d, dl200me_term_d,
              dl200me_rw_p, NULL);
