#include "cxsd_driver.h"
#include "cankoz_lyr.h"


typedef struct
{
} privrec_t;


static int  cac208_init_d(int devid, void *devptr,
                          int businfocount, int businfo[],
                          const char *auxinfo)
{
    
    DoDriverLog(devid, 0, "%s([%d]:\"%s\")",
                __FUNCTION__, businfocount, auxinfo);
    return DEVSTATE_OPERATING;
}

static void cac208_term_d(int devid, void *devptr)
{
    DoDriverLog(devid, 0, "%s()", __FUNCTION__);
}

static void cac208_rw_p(int devid, void *devptr,
                        int action,
                        int count, int *addrs,
                        cxdtype_t *dtypes, int *nelems, void **values)
{
  privrec_t *me = (privrec_t *)devptr;
}


DEFINE_CXSD_DRIVER(cac208, "CAC208/CEAC208 CAN-DAC/ADC driver",
                   NULL, NULL,
                   sizeof(privrec_t), NULL,
                   2, 2,
                   CANKOZ_LYR_NAME, CANKOZ_LYR_VERSION,
                   NULL,
                   -1, NULL, NULL,
                   cac208_init_d, cac208_term_d, cac208_rw_p);
