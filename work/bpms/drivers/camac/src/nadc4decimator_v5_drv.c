#include "cxsd_driver.h"

#include "misclib.h"
#include <string.h>

#include "timeval_utils.h"

enum
{
    A_ANY  = 0,

    A_STAT = 0,
    A_DEC  = 1,

    S_NONE       =   0,
    S_ENABLE_EXT = ( 1 << 0 ),
    S_STOP_ENOUT = ( 1 << 1 ),
    S_ENABLE_INT = ( 1 << 2 ),
    S_STOP_CLOCK = ( 1 << 3 ),
    S_READ_ENOUT = ( 1 << 4 ),
    S_ENABLE_DEC = ( 1 << 5 ),
    S_RESERVED_1 = ( 1 << 6 ),
    S_RESERVED_2 = ( 1 << 7 ),

};

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int       N;

    int       cword;

    sl_tid_t  hbt_tid;

    int       n_to_skip;
    int       skip_count;
    int       enable_operation;
} privrec_t;

static psp_paramdescr_t DEVICETYPE_HERE_params[] =
{
    PSP_P_INT ("factor", privrec_t, n_to_skip, 100, 1, 1000000),
    PSP_P_END()
};

static int CheckDeviceLAM(int devid, privrec_t *me)
{

  int  cword = 0;
  int  qx    = 0;
  int  r     = 0;

    qx = DO_NAF(CAMAC_REF, me->N, 0, 0, &cword);
    // DoDriverLog(devid, 0, "%s(), CWORD=0x%x, qx=0x%x", __FUNCTION__, cword, qx);

    /* For Memory: enum {CAMAC_X = 1, CAMAC_Q = 2}; */
    if      ( (qx & CAMAC_Q) != 0 && (cword & S_READ_ENOUT) != 0 )
    {
        // DoDriverLog(devid, 0, "SIGUSR1 in (%s)", __FUNCTION__);
        r = 1;
    }
    else if ( (qx & CAMAC_X) == 0 )
    {
        // DoDriverLog(devid, 0, "%s: WARNING: Something wrong with device: qx=0x%x", __FUNCTION__, qx);
        r = 0;
    }

    return r;
}

static int StopDevice(int devid, privrec_t *me)
{
  int  cword = 0;

    /* Stop device, drop all outputs and drop LAM */
    /* Turn off calibration signal */
    cword = S_NONE | S_STOP_ENOUT * 1 | S_STOP_CLOCK * 1;
        status2rflags(DO_NAF(camac_fd, me->N, A_STAT, 16, &cword));
    cword = S_NONE;
        status2rflags(DO_NAF(camac_fd, me->N, A_DEC,  16, &cword));

    return 1;
}

static int RestartDevice(int devid, privrec_t *me, int do_restart)
{
  int  cword, cword_dec = 0;

    /* Program the device */
    /* a. Drop LAM first... */
    cword = S_NONE | S_STOP_ENOUT * 1 | S_STOP_CLOCK * 1;
    DO_NAF(camac_fd, me->N, A_STAT, 16, &cword);

    /* b. Prepare status/control word */
    cword = S_NONE |
        S_RESERVED_1 * 0 |
        S_RESERVED_2 * 0 |
        S_STOP_ENOUT * 0 | /* enable  EN out         */
        S_READ_ENOUT * 0 | /* we want read nothing   */
        S_STOP_CLOCK * 1 | /* disable internal clock */
        S_ENABLE_INT * 0 | /* disable INT start      */
        S_ENABLE_EXT * 1 | /* enable  EXT start      */
        S_ENABLE_DEC * 0;  /* enable  DECIMATION     */

    // DoDriverLog(0, 0, "%s(), CWORD=0x%x", __FUNCTION__, cword);

    /* c. Prepare decimation control word */
    DO_NAF(camac_fd, me->N, A_DEC,  16, &cword_dec);

    /* d. Start device */
    if (do_restart == 1)
        DO_NAF(camac_fd, me->N, A_STAT, 16, &cword);

    me->cword = cword;
    return 1;
}

enum
{
    HEART_BEAT_PERIOD = 5 * 1000, // 5 ms == 200 Hz
};

static void heart_beat_hbt(int devid, void *devptr,
                           sl_tid_t tid __attribute__((unused)), void *privptr __attribute__((unused)))
{
  privrec_t *me = (privrec_t *) devptr;

  int  wait_mksek;

    me->hbt_tid = -1;

    /* 0. Increase counter */
    me->skip_count++;

    /* 1. Check if it is alowed to configure device for work with external start */
    if (me->skip_count >= me->n_to_skip  &&  me->enable_operation == 0 )
    {
        /* 2. Programm device */
        RestartDevice(devid, me, 1);
        
        me->enable_operation = 1;
        fprintf(stderr, "%s hbt: ok, lets enable laming\n", strcurtime_msc());
    }
    else
    {
        /* 3. Waiting for LAM */
        if ( CheckDeviceLAM(devid, me) )
        {
            wait_mksek = 3000; // mikroseconds
            SleepBySelect(wait_mksek);
            
            StopDevice(devid, me);
            
            me->skip_count = me->enable_operation = 0;
            fprintf(stderr, "%s hbt: there was LAM!!!\n", strcurtime_msc());
        } 
    }

//    fprintf(stderr, "hbt: count = %d\n", me->skip_count);

    me->hbt_tid = sl_enq_tout_after(devid, devptr,
                                    HEART_BEAT_PERIOD,
                                    heart_beat_hbt, NULL);
}

////
static void LAM_CB(int devid, void *devptr)
{
  privrec_t *me = (privrec_t *) devptr;

    RestartDevice(devid, me, 1);
}

static int init_b(int devid, void *devptr,
                  int businfocount, int businfo[],
                  const char *auxinfo __attribute__((unused)))
{
  privrec_t *me = (privrec_t*)devptr;

  const char      *errstr;

    if (businfocount <  1) return -CXRF_CFG_PROBL;
    me->N = businfo[0];
//    if ((errstr = WATCH_FOR_LAM(devid, devptr, me->N, LAM_CB)) != NULL)
//    {
//        DoDriverLog(devid, DRIVERLOG_ERRNO, "WATCH_FOR_LAM(): %s", errstr);
//        return -CXRF_DRV_PROBL;
//    }

    heart_beat_hbt(devid, me, -1, NULL);
    return DEVSTATE_OPERATING;
}

static void term_b(int devid, void *devptr)
{
    privrec_t *me = (privrec_t*)devptr;

    if (me->N > 0)
    {
        StopDevice(devid, me);
    }

    return;
}

////

DEFINE_DRIVER(decimatorv5, "decimator (for nadc4) driver v 5",
              NULL, NULL,
              sizeof(privrec_t), DEVICETYPE_HERE_params,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_b, term_b, NULL, NULL);
