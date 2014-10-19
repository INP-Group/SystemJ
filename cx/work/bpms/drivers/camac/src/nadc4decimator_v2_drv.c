#define CM5307_SL_DBODY 1

#include <signal.h>
#include "cxsd_driver.h"

#include "misclib.h"
#include <string.h>

#include "timeval_utils.h"

/* channels enumaertion, values so on */
#include "drv_i/nadc4decimator_v2_drv_i.h"

#define log_pref    "NADC4 Decimator Unit (dbody)"
#define short_lpref "N4DU"
#define ver         "2.2.0"

#include "log_func.h"

enum
{
    NUM_CHANS = DECIMATOR_NUMC,
};

#define A_ANY ( (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) )

/* common functions i.e. SanityCheck, Val2Code & Code2Val so on */
#include "nadc4decimator_v2_common.h"

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int      N_DEV;
    int      val_cache[NUM_CHANS];
    rflags_t rfl_cache[NUM_CHANS];
    rflags_t unit_rflags;

    int      values_changed;
} privrec_t;

static void InitChannelValues(int devid  __attribute__((unused)), privrec_t *me)
{
    int i;

    for (i = 0; i < NUM_CHANS; i++)
    {
        switch ( i )
        {
            case DEC_FACT_CH:
                me->val_cache[i] = DF_0;
                break;
            case DEC_DUTY_CH:
                me->val_cache[i] = DD_1;
                break;
            case SHOT_CH:
                me->val_cache[i] = 0;
                break;
            case STOP_CH:
                me->val_cache[i] = 0;
                break;
            case CALIBR_CH:
                me->val_cache[i] = 0;
                break;
            default:
                me->val_cache[i] = 0;
                break;
        }
        me->rfl_cache[i] = CXRF_CAMAC_NO_Q;
    }

    me->values_changed = 1;

    return;
}

static int SetLAMBinding(int devid, privrec_t *me, int value)
{
    if (value < 0) value = 0;
    if (value > 1) value = 1;

    camac_setlam(camac_fd, me->N_DEV, value);
#if 0
    MDoDriverLog(devid, 0, "%s: %s", __FUNCTION__, value ? "LAM  Enabled" : "LAM Disabled");
#endif
    return 1;
}

static int RestartDevice(int devid, privrec_t *me)
{
    int      junk, f, wait_mksek;
#if 0
    MDoDriverLog(devid, 0, "%s: There was START", __FUNCTION__);
#endif
    /*
     * Delay for measurements ends.
     * f0 = 12.291MHz <- measured, 172MHZ/14 = 12.285MHz <- project value.
     *
     * 32768/12.285*FRQDIV*FDSCAL = 2667.3 \mu sek * FRQDIV*FDSCAL
     */
#if 0
    float mult;
    mult = val2mult(&me->val_cache[DEC_FACT_CH], &me->val_cache[DEC_DUTY_CH]);
    MDoDriverLog(devid, 0, "%s: Multiplication factor = %f", __FUNCTION__, mult);
    wait_mksek = 2700 * mult;
#else
    wait_mksek = 2700;
#endif

    /*
     * Do delay here (not using RegisterDevTout()), because delay value is very
     * small, maybe smaller than time needed for process context switching in
     * PPC with our CPU type
     */
    SleepBySelect(wait_mksek);

    /* Stop device, drop all outputs and drop LAM */
    status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY, 10, &junk));

    if ( me->values_changed >= 0 && me->val_cache[DEC_FACT_CH] != DF_0 )
    {
        /* real write */
        WriteValues(devid, me->N_DEV,
                    &me->val_cache[DEC_FACT_CH], &me->val_cache[DEC_DUTY_CH],
                    &me->unit_rflags);
#if 0 /* Maybe it is sutable to do control READ in WriteValues */
        /* test read */
        ReadValues (devid, me->N_DEV,
                    &me->val_cache[DEC_FACT_CH], &me->val_cache[DEC_DUTY_CH],
                    &me->unit_rflags);
#endif
        me->values_changed = 0;
    }

    /* Change device state according to user purposes */
    f = cmd2f(0 /* istart = 0 */, me->val_cache[DEC_FACT_CH]);

#if 1
    /*
     * Without whis section, driver becaome a ZOMBI, and must be killed manually.
     * I.e. never set LAM binding, without droping lam first.
     */
    SetLAMBinding(devid, me, 1);
#endif

    /* Start Device */
    status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY,  f, &junk));

    return 1;
}

////

static int        global_hack_devid;
static privrec_t *global_hack_me;

static void sigusr_handler(int sig __attribute__((unused)))
{
#if 0
    MDoDriverLog(global_hack_devid, 0, "SIGUSR1 in (%s)", __FUNCTION__);
#endif
    signal(SIGUSR1, sigusr_handler);

    /* Here we must as far as possible drop LAM */
    RestartDevice(global_hack_devid, global_hack_me);
}

static void EnableSIGUSR1(void)
{
  sigset_t  set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_UNBLOCK,   &set, NULL);
}

static void DisableSIGUSR1(void)
{
  sigset_t  set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK,   &set, NULL);
}

static int  init_m(void)
{
    sl_set_select_behaviour(EnableSIGUSR1, DisableSIGUSR1, 1);

    signal(SIGUSR1, sigusr_handler);
    DisableSIGUSR1();

    return 0;
}

static void term_m(void)
{
}

static int init_b(int devid, void *devptr,
                  int businfocount, int businfo[],
                  const char *auxinfo)
{
    privrec_t *me = (privrec_t*)devptr;
    int        junk=100;

    global_hack_devid = devid;
    global_hack_me    = me;

    /*
     * in early tyme of CX protocol INIT_B function can be used for device reinitialization,
     * so this line have the clear evidence.
     */
    if (me->N_DEV > 0) SetLAMBinding(devid, me, 0); // Remove LAM binding for previous N

    if (businfocount <  1) return -CXRF_CFG_PROBL;
    if (businfocount >= 1) me->N_DEV = businfo[0];

    if (me->N_DEV > 0)
    {
        /* Stop device, drop all outputs and drop LAM */
        status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY, 10, &junk));

        /* Turn off calibration signal */
        status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY, 26, &junk));

        InitChannelValues(devid, me);
	RestartDevice(devid, me);

	MDoDriverLog(devid, 0, "%s by Bekhtenev, dbody driver ver. %s (%s)", log_pref, ver, __DATE__);
    }
    return DEVSTATE_OPERATING;
}

static int term_b(int devid, void *devptr)
{
    privrec_t *me = (privrec_t*)devptr;
    int        junk;

    if (me->N_DEV > 0)
    {
        /* Stop device, drop all outputs and drop LAM */
        status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY, 10, &junk));

	MDoDriverLog(devid, 0, "%s: Driver termination DONE! (ver. %s)", log_pref, ver);
    }
    return 1;
}

static void rdwr_p(int devid, void *devptr,
                   int first, int count, int32 *values, int action)
{
    privrec_t *me = (privrec_t*)devptr;

    int              n;
    int              x;

    int              junk, f = 0;
    char             str[4];

    static int32     zero_value              = 0;
    static rflags_t  zero_rflag              = 0;
//    static rflags_t  rf_bad                  = {CXRF_UNSUPPORTED};
//    static rflags_t  zero_rflags[NUM_CHANS]  = {[0 ... NUM_CHANS-1] = 0};

    int32            value;
    rflags_t         rflags;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;

        if (action == DRVA_READ)
        {

	    switch (x)
            {
                case SHOT_CH:
                case STOP_CH:
                case CALIBR_CH:
                    ReturnChanGroup(devid, x,           1, &(me->val_cache[x]),            &(me->rfl_cache[x]));
                    break;
                case DEC_FACT_CH:
                case DEC_DUTY_CH:
		    value = me->val_cache[DEC_FACT_CH];

		    ReadValues(devid, me->N_DEV,
                               &(me->val_cache[DEC_FACT_CH]),
                               &(me->val_cache[DEC_DUTY_CH]),
                               &rflags);

		    if ( value == DF_0 ) me->val_cache[DEC_FACT_CH] = value;

		    ReturnChanGroup(devid, DEC_FACT_CH, 1, &(me->val_cache[DEC_FACT_CH]),  &rflags            );
                    ReturnChanGroup(devid, DEC_DUTY_CH, 1, &(me->val_cache[DEC_DUTY_CH]),  &rflags            );
                    break;
            }
	}

        if (action == DRVA_WRITE)
        {
            value = values[n];
            MDoDriverLog(devid, 0, "%s: chan = %d, incoming value = %d", __FUNCTION__, x, value);
            rflags = zero_rflag;

            switch (x)
            {
                case SHOT_CH:
                case STOP_CH:
                    if      (value > 1) value = 1;
                    else if (value < 0) value = 0;

                    if (value == CX_VALUE_COMMAND)
                    {
                        if      (x == SHOT_CH)
                        {
                            f = cmd2f(1 /* istart = 1 */, me->val_cache[DEC_FACT_CH]);
                            sprintf(str, "SHOT");
                        }
                        else if (x == STOP_CH)
                        {
                            f = 10;
                            sprintf(str, "STOP");
                        }
                        MDoDriverLog(devid, 0, "%s: Master want's %s me", __FUNCTION__, str);

                        me->val_cache[x] = zero_value;
                        me->rfl_cache[x] = rflags = status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY, f, &junk));
                    }
                    else
                    {
                        /*
                         * nothing because after each writing into hardware,
                         * command, command value flushes to zero (see above).
                         */
                    }
                    ReturnChanGroup(devid, x, 1, &(me->val_cache[x]), &(me->rfl_cache[x]));
                    break;

               case CALIBR_CH:
                    if      (value >= 1) { value = 1; f = 27; }
                    else if (value <= 0) { value = 0; f = 26; }
                    me->val_cache[x] = value;
                    me->rfl_cache[x] = rflags = status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, A_ANY, f, &junk));

                    MDoDriverLog(devid, 0, "%s: Calibration signal is: %s", __FUNCTION__, value ? "ON" : "OFF");

                    ReturnChanGroup(devid, x, 1, &(me->val_cache[x]), &(me->rfl_cache[x]));
                    break;

                case DEC_FACT_CH:
                case DEC_DUTY_CH:
                    if ( x == DEC_FACT_CH )
                    {
                        value = DF_SanityCheck(value);
                    }
                    if ( x == DEC_DUTY_CH )
                    {
                        value = DD_SanityCheck(value);
                    }

                    if (me->val_cache[x] != value)
                    {
                        me->values_changed = 1;

                        me->val_cache[x] = value;
                        me->rfl_cache[x] = rflags = zero_rflag;

                        MDoDriverLog(devid, 0,
                                    "%s: dec_fact = %d (%s), dec_duty = %d (%s)", __FUNCTION__,
                                                me->val_cache[DEC_FACT_CH],
                                     dec_fact_c[me->val_cache[DEC_FACT_CH]+1],
                                                me->val_cache[DEC_DUTY_CH],
                                     dec_duty_c[me->val_cache[DEC_DUTY_CH]  ]);

                        RestartDevice(devid, me);
                    }
                    ReturnChanGroup(devid, x, 1, &(me->val_cache[x]), &(me->rfl_cache[x]));
                    break;

                default:
                    value  = zero_value;
                    rflags = CXRF_UNSUPPORTED;
                    ReturnChanGroup(devid, x, 1, &value, &rflags);
                    break;
            } // switch
	} // if
    } // for
}

////

DEFINE_DRIVER(decimatorv2, "decimator (for nadc4) driver v 2.2",
              init_m, term_m,
              sizeof(privrec_t), NULL,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_b, term_b, rdwr_p, NULL);
