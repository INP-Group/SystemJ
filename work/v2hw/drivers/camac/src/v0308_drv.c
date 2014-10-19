#include "cxsd_driver.h"

#include "drv_i/v0308_drv_i.h"


typedef struct
{
    int  N;
} v0308_privrec_t;


static int v0308_init_d(int devid, void *devptr,
                        int businfocount, int *businfo,
                        const char *auxinfo)
{
  v0308_privrec_t *me = (v0308_privrec_t*)devptr;
  
    me->N = businfo[0];

    return DEVSTATE_OPERATING;
}

static void v0308_rw_p(int devid, void *devptr,
                       int first, int count, int32 *values, int action)
{
  v0308_privrec_t *me = (v0308_privrec_t*)devptr;

  int              n;
  int              x;
  int              A;

  int              c;
  int32            value;
  rflags_t         rflags;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        if (action == DRVA_WRITE) value = values[n];
        else                      value = 0xFFFFFFFF; // That's just to make GCC happy

        c      = 0;
        rflags = 0;

        A = x & 1;
        switch (x &~ 1)
        {
            case V0308_CHAN_VOLTS_base:
                if (action == DRVA_WRITE)
                {
                    if (value < 0)     value = 0;
                    if (value > 0xFFF) value = 0xFFF;
                    c = value;
                    rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, A, 16, &c));
                }
                rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, A, 0, &c));
                value   = c & 0xFFF;
                break;

            case V0308_CHAN_RESET_base:
                if (action == DRVA_WRITE  &&  value == CX_VALUE_COMMAND)
                    DO_NAF(CAMAC_REF, me->N, A, 10, &c);
                value = 0;
                break;

            case V0308_CHAN_SW_ON_base:
                if (action == DRVA_WRITE  &&  value == CX_VALUE_COMMAND)
                    DO_NAF(CAMAC_REF, me->N, A, 26, &c);
                break;

            case V0308_CHAN_SW_OFF_base:
                if (action == DRVA_WRITE  &&  value == CX_VALUE_COMMAND)
                    DO_NAF(CAMAC_REF, me->N, A, 24, &c);
                break;

            case V0308_CHAN_IS_ON_base:
                rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, A, 0, &c));
                value = (c >> 12) & 1;
                break;

            case V0308_CHAN_OVL_U_base:
                rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, A, 0, &c));
                value = (c >> 13) & 1;
                break;

            case V0308_CHAN_OVL_I_base:
                rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, A, 0, &c));
                value = (c >> 14) & 1;
                break;

            case V0308_CHAN_PLRTY_base:
                rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, A, 0, &c));
                value = (c >> 15) & 1;
                break;

            default:
                value = 0;
                rflags = CXRF_UNSUPPORTED;
        }
        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}

DEFINE_DRIVER(v0308, "V0308",
              NULL, NULL,
              sizeof(v0308_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              v0308_init_d, NULL, v0308_rw_p, NULL);
