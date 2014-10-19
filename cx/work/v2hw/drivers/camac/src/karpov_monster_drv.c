#include <stdlib.h>
#include <ctype.h>

#include "timeval_utils.h"
#include "cxsd_driver.h"

#include "drv_i/karpov_monster_drv_i.h"


enum {MINVALLIMIT = 10};

enum {NUMSUPPORTED = KARPOV_MONSTER_MAXUNITS};

enum
{
    TIMEOUT_SECONDS = 10,
    WAITTIME_MSECS  = TIMEOUT_SECONDS * 1000,
}; // If there's no LAM after 10s, return CXRF_IO_TIMEOUT-marked data

static int karpovfignya2int(int kk)
{
  int  code = 0;
  
    if (kk & 04000) code |= 1;
    if (kk & 02000) code |= 2;
    if (kk & 01000) code |= 4;
    if (kk & 00400) code |= 8;
    if (kk & 00200) code |= 16;
    if (kk & 00100) code |= 32;
    if (kk & 00040) code |= 64;
    if (kk & 00020) code |= 128;
    if (kk & 00010) code |= 256;
    if (kk & 00004) code |= 512;
    if (kk & 00002) code |= 1024;
    if (kk & 00001) code |= 2048;

    return code - 2048;
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int             devid;
    int             N_DEV;
    sl_tid_t        tid;
    int             measuring_now;
    struct timeval  measurement_start;

    int             unit_online[NUMSUPPORTED];
    int             unit_N_ADC [NUMSUPPORTED];
    int             unit_N_FLT [NUMSUPPORTED];

    struct
    {
        int32     data  [KARPOV_MONSTER_CHANS_PER_UNIT];
        rflags_t  rflags[KARPOV_MONSTER_CHANS_PER_UNIT];
    } measured[NUMSUPPORTED];
} privrec_t;

//////////////////////////////////////////////////////////////////////

static int  StartMeasurements(privrec_t *me)
{
  int     dummy = 0;
  int     i;

    DO_NAF(CAMAC_REF, me->N_DEV, 0, 10, &dummy);
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 25, &dummy);
  
    for (i = 0;  i < NUMSUPPORTED;  i++)
        if (me->unit_online[i])
            DO_NAF(CAMAC_REF, me->unit_N_ADC[i], 0, 25, &dummy);

    return 0;
}

static int  AbortMeasurements(privrec_t *me)
{
  int  i;
  int  j;

    for (i = 0;  i < NUMSUPPORTED;  i++)
    {
        for (j = 0;  j < 4;  j++) // This uses the fact that CHANOFS_U{X1,X2,Y1,Y2} are 0,1,2,3
        {
            me->measured[i].data  [j] = 0;
            me->measured[i].rflags[j] = CXRF_IO_TIMEOUT;
        }
    }

    return 1;
}

static void ReadMeasurements (privrec_t *me)
{
  int     dummy = 0;
  int     i;
  int     j;
  int     v;
  int     status;

  int     Ux1, Ux2, Uy1, Uy2;

    for (i = 0;  i < NUMSUPPORTED;  i++)
        if (me->unit_online[i])
        {
            /* I. Read */
            DO_NAF(CAMAC_REF, me->unit_N_ADC[i], 0, 17, &dummy);
            DO_NAF(CAMAC_REF, me->unit_N_ADC[i], 0, 26, &dummy);
            for (j = 0;  j < 4;  j++) // This uses the fact that CHANOFS_U{X1,X2,Y1,Y2} are 0,1,2,3
            {
                status = DO_NAF(CAMAC_REF, me->unit_N_ADC[i], j, 0, &v);
                me->measured[i].data  [j] = karpovfignya2int(v);
                me->measured[i].rflags[j] = status2rflags(status);
            }

            /* II. Perform calculations */
            Ux1 = me->measured[i].data[KARPOV_MONSTER_CHANOFS_UX1];
            Ux2 = me->measured[i].data[KARPOV_MONSTER_CHANOFS_UX2];
            Uy1 = me->measured[i].data[KARPOV_MONSTER_CHANOFS_UY1];
            Uy2 = me->measured[i].data[KARPOV_MONSTER_CHANOFS_UY2];
            /* 6600=K*1000, where K=6.6, 1000microns/mm */
            me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_X] =
                me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_UX1] |
                me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_UX2];
            if (Ux1 != Ux2  &&  abs(Ux1) > MINVALLIMIT  && abs(Ux2) > MINVALLIMIT)
            {
                me->measured[i].data  [KARPOV_MONSTER_CHANOFS_X]  =
                    6600 * (Ux1-Ux2) / (Ux1+Ux2);
            }
            else
            {
                me->measured[i].data  [KARPOV_MONSTER_CHANOFS_X]  = 0;
                me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_X] |= CXRF_OVERLOAD;
            }
            me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_Y] =
                me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_UY1] |
                me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_UY2];
            if (Uy1 != Uy2  &&  abs(Uy1) > MINVALLIMIT  && abs(Uy2) > MINVALLIMIT)
            {
                me->measured[i].data  [KARPOV_MONSTER_CHANOFS_Y]  =
                    6600 * (Uy1-Uy2) / (Uy1+Uy2);
            }
            else
            {
                me->measured[i].data  [KARPOV_MONSTER_CHANOFS_Y]  = 0;
                me->measured[i].rflags[KARPOV_MONSTER_CHANOFS_Y] |= CXRF_OVERLOAD;
            }
        }
}

static void PerformReturn(privrec_t *me, rflags_t rflags)
{
  int  i;
  int  j;

    for (i = 0;  i < NUMSUPPORTED;  i++)
        if (me->unit_online[i])
        {
            if (rflags != 0)
                for (j = 0;  j < KARPOV_MONSTER_CHANS_PER_UNIT; j++)
                    me->measured[i].rflags[j] |= rflags;

            ReturnChanGroup(me->devid, i * KARPOV_MONSTER_CHANS_PER_UNIT, 6,
                            me->measured[i].data + 0, me->measured[i].rflags + 0);
        }
}

//////////////////////////////////////////////////////////////////////

static void tout_p(int devid, void *devptr __attribute__((unused)),
                   sl_tid_t tid __attribute__((unused)),
                   void *privptr);

static void ReturnMeasurements (privrec_t *me, rflags_t rflags)
{
    ReadMeasurements(me);
    PerformReturn   (me, rflags);
}

static void SetDeadline(privrec_t *me)
{
  struct timeval   msc; // timeval-representation of MilliSeConds
  struct timeval   dln; // DeadLiNe

    if (WAITTIME_MSECS > 0)
    {
        msc.tv_sec  =  WAITTIME_MSECS / 1000;
        msc.tv_usec = (WAITTIME_MSECS % 1000) * 1000;
        timeval_add(&dln, &(me->measurement_start), &msc);
        
        me->tid = sl_enq_tout_at(me->devid, me,
                                 &dln,
                                 tout_p, me);
    }
}

static void RequestMeasurements(privrec_t *me)
{
    StartMeasurements(me);
    me->measuring_now = 1;
    gettimeofday(&(me->measurement_start), NULL);
    SetDeadline(me);
}

static void PerformTimeoutActions(privrec_t *me, int do_return)
{
    me->measuring_now = 0;
    if (me->tid >= 0)
    {
        sl_deq_tout(me->tid);
        me->tid = -1;
    }
    AbortMeasurements(me);
    if (do_return)
        ReturnMeasurements (me, CXRF_IO_TIMEOUT);

    RequestMeasurements(me);
}

static void tout_p(int devid, void *devptr __attribute__((unused)),
                   sl_tid_t tid __attribute__((unused)),
                   void *privptr)
{
  privrec_t *me = (privrec_t *)privptr;

    me->tid = -1;
    if (me->measuring_now == 0  ||
        WAITTIME_MSECS <= 0)
    {
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "strange timeout: measuring_now=%d, waittime=%d",
                    me->measuring_now,
                    WAITTIME_MSECS);
        return;
    }

    PerformTimeoutActions(me, 1);
}

static void drdy_p(privrec_t *me,  int do_return, rflags_t rflags)
{
    if (me->measuring_now == 0) return;
    me->measuring_now = 0;
    if (me->tid >= 0)
    {
        sl_deq_tout(me->tid);
        me->tid = -1;
    }
    if (do_return)
        ReturnMeasurements (me, rflags);
    RequestMeasurements(me);
}

static void LAM_CB(int devid, void *devptr)
{
  privrec_t *me = (privrec_t *)devptr;

    drdy_p(me, 1, 0);
}


//////////////////////////////////////////////////////////////////////

static int init_d(int devid, void *devptr,
                  int businfocount, int *businfo,
                  const char *auxinfo)
{
  privrec_t  *me = (privrec_t*)devptr;

  const char *errstr;
  const char *p;
  int         unit_n;
  char       *err;
  int         N_ADC;
  int         N_FLT;
  const char *errmsg;
    
    me->devid = devid;
    me->N_DEV = businfo[0];
    bzero(me->unit_online, sizeof(me->unit_online));

    p = auxinfo;
    while (p != NULL  &&  *p != '\0'  &&  isspace(*p)) p++;
    for (unit_n = 0;  p != NULL  &&  *p != '\0';  unit_n++)
    {
        /*  */
        if (unit_n >= NUMSUPPORTED)
        {
            errmsg = "too many units";
            goto DEACTIVATE;
        }
        
        /* A "skip"? */
        if (*p == '-')
        {
            p++;
            if (*p != ','  &&  *p != '\0')
            {
                errmsg = "bad char";
                goto DEACTIVATE;
            }
            goto CONTINUE_PARSE;
        }

        /* Parse "N_ADC+N_FLT" */
        N_ADC = strtol(p, &err, 10);
        if (err == p  ||  *err != '+')
        {
            errmsg = "N_ADC+N_FLT expected";
            goto DEACTIVATE;
        }
        p = err + 1;
        N_FLT = strtol(p, &err, 10);
        if (err == p)
        {
            errmsg = "N_FLT expected";
            goto DEACTIVATE;
        }

        me->unit_online[unit_n] = 1;
        me->unit_N_ADC [unit_n] = N_ADC;
        me->unit_N_FLT [unit_n] = N_FLT;

        /* A comma? */
        p = err;
        if (*p != '\0'  &&  *p != ',')
        {
            errmsg = "',' expected";
            goto DEACTIVATE;
        }
        p++;
        
 CONTINUE_PARSE:;
    }

    if ((errstr = WATCH_FOR_LAM(devid, devptr, me->N_DEV, LAM_CB)) != NULL)
    {
        DoDriverLog(devid, DRIVERLOG_ERRNO, "WATCH_FOR_LAM(): %s", errstr);
        return -CXRF_DRV_PROBL;
    }
    RequestMeasurements(me);
    
    return DEVSTATE_OPERATING;

 DEACTIVATE:
    DoDriverLog(devid, 0, "%s at auxinfo pos %d", errmsg, p-auxinfo+1);
    return -CXRF_CFG_PROBL;
}

static void rdwr_p(int devid, void *devptr,
                   int first, int count, int32 *values, int action)
{
  privrec_t  *me = (privrec_t*)devptr;

  int         n;
  int         x;
  int         unit_n;
  int         chan_n;
  
  int32       value;
  rflags_t    rflags;
  int         status;

    for (n = 0;  n < count;  n++)
    {
        x = first + n;

        unit_n = x / KARPOV_MONSTER_CHANS_PER_UNIT;
        chan_n = x % KARPOV_MONSTER_CHANS_PER_UNIT;

        if (unit_n >= NUMSUPPORTED    ||
            !me->unit_online[unit_n]  ||
            chan_n == KARPOV_MONSTER_CHANOFS_UNUSED)
        {
            value  = 0;
            rflags = CXRF_UNSUPPORTED;
            ReturnChanGroup(devid, x, 1, &value, &rflags);
        }
        else if (chan_n == KARPOV_MONSTER_CHANOFS_AMPL)
        {
            if (action == DRVA_WRITE)
            {
                value = values[n];
                status = DO_NAF(CAMAC_REF, me->unit_N_FLT[unit_n], 0, 16, &value);
            }
            else
            {
                status = DO_NAF(CAMAC_REF, me->unit_N_FLT[unit_n], 0, 0,  &value);
            }
            rflags = status2rflags(status);
            ReturnChanGroup(devid, x, 1, &value, &rflags);
        }
        /* Other channels are returned unconditionally upon LAM */
    }
}

DEFINE_DRIVER(karpov_monster, "Karpov's monster-alike BPM",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              0, NULL,
              init_d, NULL, rdwr_p, NULL);
