#include "cxsd_driver.h"

#include "drv_i/v06xx_drv_i.h"


typedef struct
{
    int  N;
} v06xx_privrec_t;


static int v06xx_init_d(int devid, void *devptr,
                        int businfocount, int *businfo,
                        const char *auxinfo)
{
  v06xx_privrec_t *me = (v06xx_privrec_t*)devptr;
  
    me->N = businfo[0];

    return DEVSTATE_OPERATING;
}

static void v06xx_rw_p(int devid, void *devptr,
                       int first, int count, int32 *values, int action)
{
  v06xx_privrec_t *me = (v06xx_privrec_t*)devptr;

  int              n;
  int              x;
  int              i;
  
  int              c;
  int32            value;
  rflags_t         rflags;
  
    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        switch (x)
        {
            case V06XX_CHAN_WR_N_BASE ... V06XX_CHAN_WR_N_BASE+V06XX_CHAN_WR_N_COUNT-1:
            case V06XX_CHAN_WR1:
                i = x - V06XX_CHAN_WR_N_BASE;
                if (action == DRVA_READ)
                {
                    rflags  = status2rflags(DO_NAF(CAMAC_REF, me->N, 0, 0, &c));
                    if (x == V06XX_CHAN_WR1)
                        value = c & 0x00FFFFFF;
                    else
                        value = ((c & (1 << i)) != 0);
                    ReturnChanGroup(devid, x, 1, &value, &rflags);
                }
                else
                {
                    rflags  = status2rflags(DO_NAF(CAMAC_REF, me->N, 0, 0, &c));
                    ////DoDriverLog(devid, 0, "i=%-2d c=%d", i, c);
                    if (x == V06XX_CHAN_WR1)
                        c = values[n] & 0x00FFFFFF;
                    else
                        c = (c &~ (1 << i)) | ((values[n] != 0) << i);
                    ////DoDriverLog(devid, 0, "     c:%d", c);
                    rflags |= status2rflags(DO_NAF(CAMAC_REF, me->N, 3, 16, &c));
                    {
                      int rc;
                      
                        DO_NAF(CAMAC_REF, me->N, 0, 0, &rc);
                        ////DoDriverLog(devid, 0, "     r=%d", rc);
                    }

                    if (x == V06XX_CHAN_WR1)
                    {
                        // Should notify about change of all channels
                        for (i = 0;  i < V06XX_CHAN_WR_N_COUNT;  i++)
                        {
                            value = (c >> i) & 1;
                            ReturnChanGroup(devid, V06XX_CHAN_WR_N_BASE + i, 1, &value, &rflags);
                        }
                        // ...including WR1
                        value = c;
                        ReturnChanGroup(devid, V06XX_CHAN_WR1, 1, &value, &rflags);
                    }
                    else
                    {
                        // Should notify about x's new value
                        value = (c >> i) & 1;
                        ReturnChanGroup(devid, x,              1, &value, &rflags);
                        // ...and about WR1
                        value = c;
                        ReturnChanGroup(devid, V06XX_CHAN_WR1, 1, &value, &rflags);
                    }
                }
                break;

            default:
                value = 0;
                rflags = CXRF_UNSUPPORTED;
                ReturnChanGroup(devid, x, 1, &value, &rflags);
        }
    }
}


DEFINE_DRIVER(v06xx, "V06XX",
              NULL, NULL,
              sizeof(v06xx_privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              v06xx_init_d, NULL, v06xx_rw_p, NULL);
