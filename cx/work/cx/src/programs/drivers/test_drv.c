#include <stdio.h>

#include "cxsd_driver.h"

static void tcb(int devid, void *devptr,
                sl_tid_t tid,
                void *privptr)
{
    DoDriverLog(devid, 0, "%s::%s(devid=%d, devptr=%p, tid=%d, privptr=%p",
                __FILE__, __FUNCTION__, devid, devptr, tid, privptr);
    sl_enq_tout_after(devid, devptr, 1500*1000, tcb, privptr);
}

static int test_init_d(int devid, void *devptr __attribute__((unused)), 
                       int businfocount, int businfo[],
                       const char *auxinfo)
{
    DoDriverLog(devid, 0, "%s(businfocount=%d, auxinfo=\"%s\")",
                __FUNCTION__, businfocount, auxinfo);
    sl_enq_tout_after(devid, devptr, 1000*1000, tcb, lint2ptr(0x12345678));
    sl_enq_tout_after(devid, devptr, 1000*1000, tcb, lint2ptr(0x87654321));
    return DEVSTATE_OPERATING;
}

static void test_term_d(int devid, void *devptr __attribute__((unused)))
{
    DoDriverLog(devid, 0, "%s()", __FUNCTION__);
}

static void test_bigc_p(int devid, void *devptr __attribute__((unused)), int chan, int32 *args, int nargs, void *data, size_t datasize, size_t dataunits)
{

    ReturnBigc(devid, chan, args, nargs, data, datasize, dataunits*0+4, 0);
}

DEFINE_DRIVER(test, "A testbed driver",
              NULL, NULL,
              0, NULL,
              0, 20,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              test_init_d, test_term_d,
              StdSimulated_rw_p, test_bigc_p);
