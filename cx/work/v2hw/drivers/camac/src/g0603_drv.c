
#include "cxsd_driver.h"

//!!!No g0603_drv_i.h file...


typedef struct
{
    int       N;

    int       val_cache[8];
    rflags_t  rfl_cache[8];
} g0603_privrec_t;


static int g0603_init_d(int devid, void *devptr,
                        int businfocount, int *businfo,
                        const char *auxinfo)
{
  g0603_privrec_t *me = (g0603_privrec_t*)devptr;

  int              i;
  
    me->N = businfo[0];
  
    for (i = 0;  i < 8;  i++)
    {
        me->val_cache[i] = 0;
        me->rfl_cache[i] = CXRF_CAMAC_NO_Q;
    }

    return DEVSTATE_OPERATING;
}

static void g0603_rw_p(int devid, void *devptr,
                       int first, int count, int32 *values, int action)
{
  g0603_privrec_t *me = (g0603_privrec_t*)devptr;

  int              n;
  int              x;

  int32            value;
  rflags_t         rflags;

  int              v;
  int              status;
  int              tries;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) value = values[n];
        else                      value = 0xFFFFFFFF; // That's just to make GCC happy

        if (x < 0  ||  x > 7)
        {
            value  = -1;
            rflags = CXRF_UNSUPPORTED;
        }
        else
        {
            if (action == DRVA_READ)
            {
                value  = me->val_cache[x];
                rflags = me->rfl_cache[x];
            }
            else
            {
                v = (value + 0x8000) & 0xFFFF;
                for (status = 0, tries = 0;
                     (status & CAMAC_Q) == 0  &&  tries < 7000/*~=6.7ms(PKS8_cycle)/1us(CAMAC_cycle)*/;
                     tries++)
                    status = DO_NAF(CAMAC_REF, me->N, x, 16, &v);

                rflags = status2rflags(status);
                /* Failure? */
                if (rflags & CXRF_CAMAC_NO_Q) rflags |= CXRF_IO_TIMEOUT;

                me->val_cache[x] = value;
                me->rfl_cache[x] = rflags;
            }
        }

        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}


DEFINE_DRIVER(g0603, "G0603",
              NULL, NULL,
              sizeof(g0603_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              g0603_init_d, NULL, g0603_rw_p, NULL);
