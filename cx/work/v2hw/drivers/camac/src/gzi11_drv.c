
#include "cxsd_driver.h"

#include "drv_i/gzi11_drv_i.h"


enum
{
    TYPE_GZI11NEW = 0,
    TYPE_GZI      = 1,
    TYPE_GZI11OLD = 2,
};

typedef struct
{
    int       N;

    int       type_code;
    int       type_value_mask;
    int       type_onoff_shift;
    int       type_onoff_inv;
} gzi11_privrec_t;

static psp_paramdescr_t gzi11_params[] =
{
    PSP_P_FLAG("gzi11new", gzi11_privrec_t, type_code, TYPE_GZI11NEW, 1),
    PSP_P_FLAG("gzi",      gzi11_privrec_t, type_code, TYPE_GZI,      0),
    PSP_P_FLAG("gzi11old", gzi11_privrec_t, type_code, TYPE_GZI11OLD, 0),
    PSP_P_END()
};


static int gzi11_init_d(int devid, void *devptr,
                        int businfocount, int *businfo,
                        const char *auxinfo)
{
  gzi11_privrec_t *me = (gzi11_privrec_t*)devptr;

    me->N = businfo[0];

    if      (me->type_code == TYPE_GZI)
    {
        me->type_value_mask  = 0x0FFFF;
        me->type_onoff_shift = 16;
        me->type_onoff_inv   = 0;
    }
    else if (me->type_code == TYPE_GZI11OLD)
    {
        me->type_value_mask  = 0x1FFFF;
        me->type_onoff_shift = 17;
        me->type_onoff_inv   = 1;
    }
    else /* (me->type_code == TYPE_GZI11NEW) */
    {
        me->type_value_mask  = 0x1FFFF;
        me->type_onoff_shift = 17;
        me->type_onoff_inv   = 0;
    }

    return DEVSTATE_OPERATING;
}

static void gzi11_rw_p(int devid, void *devptr,
                       int first, int count, int32 *values, int action)
{
  gzi11_privrec_t *me = (gzi11_privrec_t*)devptr;

  int              n;
  int              x;

  int32            value;
  rflags_t         rflags;

  int              A;
  int              c;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) value = values[n];
        else                      value = 0xFFFFFFFF; // That's just to make GCC happy

        if      (x == GZI11_CHAN_OUT1     ||  x == GZI11_CHAN_OUT2)
        {
            A = x - GZI11_CHAN_OUT1;
            if (action == DRVA_WRITE)
            {
                if (value < 0)                   value = 0;
                if (value > me->type_value_mask) value = me->type_value_mask;
                rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A, 0,  &c));
                c = (c & (1 << me->type_onoff_shift)) | (value);
                rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A, 16, &c));
            }
            else
            {
                rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A, 0,  &c));
                value   = c & me->type_value_mask;
            }
        }
        else if (x == GZI11_CHAN_ENABLE1  ||  x == GZI11_CHAN_ENABLE2)
        {
            A = x - GZI11_CHAN_ENABLE1;
            if (action == DRVA_WRITE)
            {
                value   = (value != 0);
                rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A, 0,  &c));
                c = (c &~ (1 << me->type_onoff_shift)) | ((value ^ me->type_onoff_inv) << me->type_onoff_shift);
                rflags |= x2rflags(DO_NAF(CAMAC_REF, me->N, A, 16, &c));
            }
            else
            {
                rflags  = x2rflags(DO_NAF(CAMAC_REF, me->N, A, 0,  &c));
                value   = ((c & (1 << me->type_onoff_shift)) != 0) ^ me->type_onoff_inv;
            }
        }
        else
        {
            rflags = CXRF_UNSUPPORTED;
            value  = 0;
        }

        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}


DEFINE_DRIVER(gzi11, "GZI11",
              NULL, NULL,
              sizeof(gzi11_privrec_t), gzi11_params,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              gzi11_init_d, NULL, gzi11_rw_p, NULL);
