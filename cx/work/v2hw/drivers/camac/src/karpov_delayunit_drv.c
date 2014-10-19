
#include "cxsd_driver.h"

//!!!No karpov_delayunit_drv_i.h file...


enum {KARPOV_DELAY_CAP = 5};

typedef struct
{
    int       N;

    int       val_cache[KARPOV_DELAY_CAP];
    int       rfl_cache[KARPOV_DELAY_CAP];
} karpov_delayunit_privrec_t;


static int karpov_delayunit_init_d(int devid, void *devptr,
                                   int businfocount, int *businfo,
                                   const char *auxinfo)
{
  karpov_delayunit_privrec_t *me = (karpov_delayunit_privrec_t*)devptr;

  int              i;

    me->N = businfo[0];
  
    for (i = 0;  i < KARPOV_DELAY_CAP;  i++)
    {
        me->val_cache[i] = 0;
        me->rfl_cache[i] = CXRF_CAMAC_NO_Q;
    }

    return DEVSTATE_OPERATING;
}

static void karpov_delayunit_rw_p(int devid, void *devptr,
                                  int first, int count, int32 *values, int action)
{
  karpov_delayunit_privrec_t *me = (karpov_delayunit_privrec_t*)devptr;

  int              n;
  int              x;

  int32            value;
  rflags_t         rflags;

  enum {
      DIGITAL_QUANT = 50,
      MAX_PS        = (15 * DIGITAL_QUANT + 255 / 4) * 1000
  };
    
  int       picos;
  int       analog_picos;
  int       digital_part;
  int       analog_part;
  int       code;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) value = values[n];
        else                      value = 0xFFFFFFFF; // That's just to make GCC happy

        if (x >= KARPOV_DELAY_CAP)
        {
            value  = 0xFFFFFFFF;
            rflags = CXRF_UNSUPPORTED;
        }
        else
        {
            if (action == DRVA_WRITE)
            {
                picos = value;
                if (picos < 0)      picos = 0;
                if (picos > MAX_PS) picos = MAX_PS;
                picos -= picos % 250; // Quant is 0.25ns==250ps
                
                digital_part = picos / 1000 / DIGITAL_QUANT;
                if (digital_part > 15) digital_part = 15;
                analog_picos = picos - digital_part * DIGITAL_QUANT * 1000;
                analog_part  = analog_picos * 4 / 1000; // x*4 == x/0.25
                if (analog_part > 255) analog_part = 255;
                code = (digital_part << 8) + (255 - analog_part);

                me->val_cache[x] = picos;
                me->rfl_cache[x] = status2rflags(DO_NAF(CAMAC_REF, me->N, x, 16, &code));
            }

            value  = me->val_cache[x];
            rflags = me->rfl_cache[x];
        }

        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}


DEFINE_DRIVER(karpov_delayunit, "Karpov's DelayUnit",
              NULL, NULL,
              sizeof(karpov_delayunit_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              karpov_delayunit_init_d, NULL, karpov_delayunit_rw_p, NULL);
