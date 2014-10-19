#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cxsd_driver.h"
#include "drv_i/vadc16_drv_i.h"


enum {ADDRESS_MODIFIER = 0x29};

enum
{
    KREG_IO  = 0,
    KREG_IRQ = 2,
};

enum
{
    KCMD_STOP       = 0,
    KCMD_START      = 1,
    KCMD_SET_ADTIME = 2,
    KCMD_SET_CHBEG  = 3,
    KCMD_SET_CHEND  = 4,
    KCMD_READ_MEM   = 5,
};

static inline uint16 KCMD(uint8 cmd, uint8 modifier)
{
    return (((uint16)cmd) << 8) | modifier;
}

enum
{
    KCMD_START_FLAG_MULTICHAN = 1 << 0,
    KCMD_START_FLAG_INFINITE  = 1 << 1,
    KCMD_START_FLAG_EACH_IRQ  = 1 << 2,
};

enum
{
    ADDR_FLAG0     = 33,
    ADDR_FLAG1     = 34,
    
    ADDR_CHBEG     = 37,
    ADDR_CHEND     = 38,
    ADDR_CHCUR     = 39,
    ADDR_ADTIME    = 40,

    ADDR_SWversion = 113,
    ADDR_HWversion = 114,
    
    ADDR_CHNNDATA_BASE = 128,
};


typedef struct
{
    int  iohandle;

    /*
     The irq_n, ch_beg, ch_end triplet is used for a weird purpose:
     in case of IRQ-less operation (i.e., irq_n==0), ADC data is returned
     immediately in rdwr_p(), but only if requested channel is in the
     range [ch_beg,ch_end].
     Thus we emulate CAN behaviour in a most correct way.
     */
    uint8  irq_n;
    uint8  ch_beg;
    uint8  ch_end;

    uint8  irq_vect;
} privrec_t;


static uint8 KVRdByte(privrec_t *me, uint8 addr)
{
  uint16  w;

    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(KCMD_READ_MEM, addr));
    bivme2_io_a16rd16(me->iohandle, KREG_IO, &w);

    return w & 0xFF;
}


static void ReturnAdcChan(int devid, privrec_t *me, int l)
{
  int32       code;
  int32       cod2;
  int32       val;
  rflags_t    rflags;

    if (l < 0  ||  l > VADC16_CHAN_ADC_n_COUNT) return;

    /* Read twice, ... */
    code =
        (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 0)     ) |
        (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 1) << 8) |
        (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 2) << 16) ;
    cod2 =
        (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 0)     ) |
        (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 1) << 8) |
        (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 2) << 16) ;

    /* ...and if results differ (an extremely rare probability --
     only if we managed to read when device updates value), then ... */
    if (cod2 != code)
        /* ...repeat read */
        code =
            (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 0)     ) |
            (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 1) << 8) |
            (KVRdByte(me, ADDR_CHNNDATA_BASE + l * 4 + 2) << 16) ;

    if ((code &  0x800000) != 0)
        code -= 0x1000000;
    
    val = scale32via64(code, 10000000, 0x3fffff);
    rflags = (code >= -0x3FFFFF && code <= 0x3FFFFF)? 0 : CXRF_OVERLOAD;
        
    ReturnChanGroup(devid, VADC16_CHAN_ADC_n_BASE + l, 1, &val, &rflags);
}

static void irq_p(int devid, void *devptr,
                  int irq_n, int vector)
{
  privrec_t  *me = (privrec_t*)devptr;

  int         l;

  int         ch_beg;
  int         ch_end;

    if (vector != me->irq_vect) return;

    ch_beg = KVRdByte(me, ADDR_CHBEG);
    ch_end = KVRdByte(me, ADDR_CHEND);

    for (l = ch_beg;  l <= ch_end;  l++)
        ReturnAdcChan(devid, me, l);
}

static int init_d(int devid, void *devptr,
                  int businfocount, int *businfo,
                  const char *auxinfo)
{
  privrec_t  *me = (privrec_t*)devptr;

  int         jumpers;
  uint8       adtime;

    if (businfocount < 1) return -CXRF_CFG_PROBL;
    jumpers      = businfo[0];
    me->irq_n    = businfocount > 1? businfo[1] &  0x7 : 0;
    me->irq_vect = businfocount > 2? businfo[2] & 0xFF : 0;
fprintf(stderr, "businfo[0]=%08x jumpers=0x%x irq=%d\n", businfo[0], jumpers, me->irq_n);

    me->iohandle = bivme2_io_open(devid, devptr,
                                  jumpers << 4, 4, ADDRESS_MODIFIER,
                                  me->irq_n, irq_p);
    fprintf(stderr, "open=%d\n", me->iohandle);
    if (me->iohandle < 0)
    {
        DoDriverLog(devid, 0, "open: %d/%s", me->iohandle, strerror(errno));
        return -CXRF_DRV_PROBL;
    }

    me->ch_beg = KVRdByte(me, ADDR_CHBEG);
    me->ch_end = KVRdByte(me, ADDR_CHEND);
    adtime     = KVRdByte(me, ADDR_ADTIME);
fprintf(stderr, "[%d-%d]@%d\n", me->ch_beg, me->ch_end, adtime);
    {
        int  a;
        for (a = 0;  a <= 255; a++)
            fprintf(stderr, "%3d%c", KVRdByte(me, a), (a&15)==15? '\n' : ' ');
    }

    if (me->ch_beg > VADC16_CHAN_ADC_n_COUNT-1  ||
        me->ch_end > VADC16_CHAN_ADC_n_COUNT-1  ||
        me->ch_beg > me->ch_end)
    {
        me->ch_beg = 0;
        me->ch_end = VADC16_CHAN_ADC_n_COUNT-1;
    }
    if (adtime > 7) adtime = 4;
fprintf(stderr, "[%d-%d]@%d\n", me->ch_beg, me->ch_end, adtime);

    bivme2_io_a16wr16(me->iohandle, KREG_IRQ,
                      (((uint16)(me->irq_n)) << 8) | me->irq_vect);

    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(KCMD_STOP, 0));
    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(KCMD_SET_CHBEG,  me->ch_beg));
    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(KCMD_SET_CHEND,  me->ch_end));
    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(KCMD_SET_ADTIME, adtime));
    bivme2_io_a16wr16(me->iohandle, KREG_IO,
                      KCMD(KCMD_START,
                           KCMD_START_FLAG_MULTICHAN |
                           KCMD_START_FLAG_INFINITE  |
                           KCMD_START_FLAG_EACH_IRQ * 1 /*!!! Bug in vadc16r.pdf */));

    return DEVSTATE_OPERATING;
}

static void term_d(int devid, void *devptr)
{
  privrec_t  *me = (privrec_t*)devptr;

    bivme2_io_a16wr16(me->iohandle, KREG_IRQ, 0);
    bivme2_io_a16wr16(me->iohandle, KREG_IO,  KCMD(KCMD_STOP, 0));
    bivme2_io_close  (me->iohandle);
}

static void ReRun(privrec_t *me, uint8 set_cmd, uint8 value)
{
    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(KCMD_STOP, 0));
    bivme2_io_a16wr16(me->iohandle, KREG_IO, KCMD(set_cmd,   value));
    bivme2_io_a16wr16(me->iohandle, KREG_IO,
                      KCMD(KCMD_START,
                           KCMD_START_FLAG_MULTICHAN |
                           KCMD_START_FLAG_INFINITE));
}

static void rdwr_p(int devid, void *devptr,
                   int first, int count, int32 *values, int action)
{
  privrec_t  *me = (privrec_t*)devptr;

  int         n;
  int         x;
  int         l;

  int32       val;
  rflags_t    rflags;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) val = values[n];
        ////fprintf(stderr, "%c(%d)\n", action==DRVA_READ?'r':'w', x);

        switch (x)
        {
            case VADC16_CHAN_ADC_n_BASE ... VADC16_CHAN_ADC_n_BASE + VADC16_CHAN_ADC_n_COUNT-1:
                l = x - VADC16_CHAN_ADC_n_BASE;
                if (me->irq_n == 0  &&
                    l >= me->ch_beg  &&  l <= me->ch_end)
                    ReturnAdcChan(devid, me, l);
                else
                    ; /* Do-nothing -- those channels are returned upon IRQ */
                break;

            case VADC16_CHAN_ADC_BEG:
            case VADC16_CHAN_ADC_END:
                if (action == DRVA_WRITE)
                {
                    if (val < 0)
                        val = 0;
                    if (val > VADC16_CHAN_ADC_n_COUNT - 1)
                        val = VADC16_CHAN_ADC_n_COUNT - 1;
                    ReRun(me,
                          x == VADC16_CHAN_ADC_BEG? KCMD_SET_CHBEG : KCMD_SET_CHEND,
                          val);
                }
                val    = KVRdByte(me, x == VADC16_CHAN_ADC_BEG? ADDR_CHBEG : ADDR_CHEND);
                rflags = 0;
                ReturnChanGroup(devid, x, 1, &val, &rflags);

                if (x == VADC16_CHAN_ADC_BEG) me->ch_beg = val;
                else                          me->ch_end = val;

                if (me->irq_n == 0)
                    for (l = me->ch_beg;  l <= me->ch_end;  l++)
                        ReturnAdcChan(devid, me, l);
                break;

            case VADC16_CHAN_ADC_TIMECODE:
                if (action == DRVA_WRITE)
                {
                    if (val < 0)
                        val = 0;
                    if (val > 7)
                        val = 7;
                    ReRun(me, KCMD_SET_ADTIME, val);
                }
                val    = KVRdByte(me, ADDR_ADTIME);
                rflags = 0;
                ReturnChanGroup(devid, x, 1, &val, &rflags);
                break;
                
            default:
                val  = 0;
                rflags = CXRF_UNSUPPORTED;
                ReturnChanGroup(devid, x, 1, &val, &rflags);
        }
    }
}


DEFINE_DRIVER(vadc16, "VADC16 VME ADC driver",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_d, term_d, rdwr_p, NULL);
