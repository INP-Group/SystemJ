#include <stdio.h>

#include "cxsd_driver.h"


static int noop_init_d(int devid, void *devptr __attribute__((unused)), 
                       int businfocount, int businfo[],
                       const char *auxinfo)
{
    DoDriverLog(devid, 0, "%s(auxinfo=\"%s\")", __FUNCTION__, auxinfo);
    return DEVSTATE_OPERATING;
}

static void noop_term_d(int devid, void *devptr __attribute__((unused)))
{
    DoDriverLog(devid, 0, "%s()", __FUNCTION__);
}

static void noop_bigc_p(int devid, void *devptr __attribute__((unused)), int chan, int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{
////    fprintf(stderr, "%s(magic=%d, chan=%d, nargs=%d, datasize=%d, dataunits=%d)\n",
////            __FUNCTION__, devid, chan, nargs, datasize, dataunits);

    ReturnBigc(devid, chan, args, nargs, data, datasize, dataunits*0+4, 0);
}

DEFINE_DRIVER(noop, "NO-OP",
              NULL, NULL,
              0, NULL,
              0, 20,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              noop_init_d, noop_term_d,
              StdSimulated_rw_p, noop_bigc_p);
