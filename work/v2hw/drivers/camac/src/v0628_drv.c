#include "cxsd_driver.h"

#include "drv_i/v0628_drv_i.h"
enum {V0628_CHAN_WR_TRUE_COUNT = 16};
enum {V0628_CHAN_WR1 = V0628_CHAN_FAKE_WR1};


typedef struct
{
    int  N;
} v0628_privrec_t;


static int v0628_init_d(int devid, void *devptr,
                        int businfocount, int *businfo,
                        const char *auxinfo)
{
  v0628_privrec_t *me = (v0628_privrec_t*)devptr;
  
    me->N = businfo[0];

    return DEVSTATE_OPERATING;
}

static void v0628_rw_p(int devid, void *devptr,
                       int first, int count, int32 *values, int action)
{
  v0628_privrec_t *me = (v0628_privrec_t*)devptr;

  int              n;
  int              x;
  int              i;
  
  int              c;
  int              A;
  int              shift;
  int32            value;
  rflags_t         rflags;
  
    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        switch (x)
        {
            case V0628_CHAN_WR_N_BASE ... V0628_CHAN_WR_N_BASE+V0628_CHAN_WR_TRUE_COUNT-1:
            case V0628_CHAN_WR1:
                i = x - V0628_CHAN_WR_N_BASE;
                if (action == DRVA_READ)
                {
                    rflags  = status2rflags(DO_NAF(CAMAC_REF, me->N, 0, 0, &c));
                    if (x == V0628_CHAN_WR1)
                        value = c & 0x00FFFFFF;
                    else
                        value = ((c & (1 << i)) != 0);
                    ReturnChanGroup(devid, x, 1, &value, &rflags);
                }
                else
                {
                    rflags  = status2rflags(DO_NAF(CAMAC_REF, me->N, 0, 0, &c));
                    if (x == V0628_CHAN_WR1)
                        c = values[n] & 0x00FFFFFF;
                    else
                        c = (c &~ (1 << i)) | ((values[n] != 0) << i);
                    rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, 3, 16, &c));

                    if (x == V0628_CHAN_WR1)
                    {
                        // Should notify about change of all channels
                        for (i = 0;  i < V0628_CHAN_WR_TRUE_COUNT;  i++)
                        {
                            value = (c >> i) & 1;
                            ReturnChanGroup(devid, V0628_CHAN_WR_N_BASE + i, 1, &value, &rflags);
                        }
                        // ...including WR1
                        value = c;
                        ReturnChanGroup(devid, V0628_CHAN_WR1, 1, &value, &rflags);
                    }
                    else
                    {
                        // Should notify about x's new value
                        value = (c >> i) & 1;
                        ReturnChanGroup(devid, x,              1, &value, &rflags);
                        // ...and about WR1
                        value = c;
                        ReturnChanGroup(devid, V0628_CHAN_WR1, 1, &value, &rflags);
                    }
                }
                break;

            case V0628_CHAN_RD_N_BASE ... V0628_CHAN_RD_N_BASE+V0628_CHAN_RD_N_COUNT-1:
                A     = (x - V0628_CHAN_RD_N_BASE) / 16 + 1;
                shift = (x - V0628_CHAN_RD_N_BASE) % 16;
                rflags  = status2rflags(DO_NAF(CAMAC_REF, me->N, A, 0, &c));
                value   = (c >> shift) & 1;
                ReturnChanGroup(devid, x, 1, &value, &rflags);
                break;

            case V0628_CHAN_RD1:
                rflags  = status2rflags(DO_NAF(CAMAC_REF, me->N, 1, 0, &c));
                value   =  c & 0x0000FFFF;
                rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, 2, 0, &c));
                value  |= (c & 0x0000FFFF) << 16;
                ReturnChanGroup(devid, x, 1, &value, &rflags);
                break;
                
            /* Channels 16...23 go here */
            default:
                value = 0;
                rflags = CXRF_UNSUPPORTED;
                ReturnChanGroup(devid, x, 1, &value, &rflags);
        }
    }
}


DEFINE_DRIVER(v0628, "V0628",
              NULL, NULL,
              sizeof(v0628_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              v0628_init_d, NULL, v0628_rw_p, NULL);
