#ifndef ABSTRACT_NAME
  #error "The ABSTRACT_NAME (abstract driver's name) isn't defined"
#endif

#include "cxsd_driver.h"


/*
  Note:
      As compared to old src/programs/server/drivers/std_abstract_iface.h,
      this implementation omits N2, set1, set2.
      BIGC functionality is removed too.
 */


/* Define our internal names */
#define A_INIT_D __CX_CONCATENATE(ABSTRACT_NAME, _init_d)
#define A_RW_P   __CX_CONCATENATE(ABSTRACT_NAME, _rw_p)

/* Define abstract names */
#define A_INIT_FUNC_NAME __CX_CONCATENATE(ABSTRACT_NAME,_adrv_init)
#define A_READ_FUNC_NAME __CX_CONCATENATE(ABSTRACT_NAME,_adrv_read)
#define A_FEED_FUNC_NAME __CX_CONCATENATE(ABSTRACT_NAME,_adrv_feed)

/* Define functions' parameters */
#define ABSTRACT_INIT_FUNC() static int A_INIT_FUNC_NAME(int ref, int N1, const char *infostr __attribute__((__unused__)))
#define ABSTRACT_READ_FUNC() static int A_READ_FUNC_NAME(int ref, int N1, int chan, int *value, rflags_t *rflags)
#define ABSTRACT_FEED_FUNC() static int A_FEED_FUNC_NAME(int ref, int N1, int chan, int *value, rflags_t *rflags)

#define ABSENT_INIT_FUNC static int A_INIT_FUNC_NAME(int ref __attribute__((__unused__)), int N1 __attribute__((__unused__)), const char *infostr __attribute__((__unused__))) {return 0;}
#define ABSENT_READ_FUNC static int A_READ_FUNC_NAME(int ref __attribute__((__unused__)), int N1 __attribute__((__unused__)), int chan __attribute__((__unused__)), int *value __attribute__((__unused__)), rflags_t *rflags __attribute__((__unused__))) {return 0;}
#define ABSENT_FEED_FUNC static int A_FEED_FUNC_NAME(int ref __attribute__((__unused__)), int N1 __attribute__((__unused__)), int chan __attribute__((__unused__)), int *value __attribute__((__unused__)), rflags_t *rflags __attribute__((__unused__))) {return 0;}

/* Forward declarations */
ABSTRACT_INIT_FUNC();
ABSTRACT_READ_FUNC();
ABSTRACT_FEED_FUNC();


typedef struct
{
    int  N;
} privrec_t;


static int  A_INIT_D(int devid, void *devptr,
                     int businfocount, int *businfo,
                     const char *auxinfo)
{
  privrec_t *me = (privrec_t*)devptr;

  int        r;
  
    me->N = businfo[0];

    r = A_INIT_FUNC_NAME(CAMAC_REF, me->N, auxinfo);
    if (r < 0) return r;

    return DEVSTATE_OPERATING;
}

static void A_RW_P(int devid, void *devptr,
                   int first, int count, int32 *values, int action)
{
  privrec_t *me = (privrec_t*)devptr;

  int        n;
  int        x;

  int32      value;
  rflags_t   rflags;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        rflags = 0;
        if (action == DRVA_WRITE)
        {
            value = values[n];
            A_FEED_FUNC_NAME(CAMAC_REF, me->N, x, &value, &rflags);
        }
        else
        {
            value = 0xFFFFFFFF;
            A_READ_FUNC_NAME(CAMAC_REF, me->N, x, &value, &rflags);
        }

        ReturnChanGroup(devid, x, 1, &value, &rflags);
    }
}

DEFINE_DRIVER(ABSTRACT_NAME, __CX_STRINGIZE(ABSTRACT_NAME) "abstract driver",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              A_INIT_D, NULL, A_RW_P, NULL);

static void DO_LOG(const char *format, ...) __attribute__((__unused__));
static void DO_LOG(const char *format, ...)
{
  va_list        ap;
  
    va_start(ap, format);
    vDoDriverLog(DEVID_NOT_IN_DRIVER, 0, format, ap);
    va_end(ap);
    
}
