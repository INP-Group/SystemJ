#include "misclib.h"
#include "timeval_utils.h"
#include "cxsd_driver.h"

#include "drv_i/u0632_drv_i.h"


enum
{
    TIMEOUT_SECONDS = 10*0,
    WAITTIME_MSECS  = TIMEOUT_SECONDS * 1000,
};

enum {POLL_PERIOD = 100*1000*0+5*1000}; // 100ms

enum
{
    U0632_USECS = 30,   // 30us delay after read/write cycle NAF
    U0632_CADDR = 070,  // Control word's address
    U0632_4TEST = 32,   // A word for tests
};

enum
{
    SETTING_P_MASK = 0x1F00,  SETTING_P_SHIFT = 8,  SETTING_P_ALWD = SETTING_P_MASK >> SETTING_P_SHIFT,
    SETTING_M_MASK = 0xC0,    SETTING_M_SHIFT = 6,  SETTING_M_ALWD = SETTING_M_MASK >> SETTING_M_SHIFT,
};

enum
{
    NUM_LINES      = 2,
    UNITS_PER_LINE = 15
};

static inline int unitn2unitcode(int n)
{
    return (n % UNITS_PER_LINE) + (1 << 4) * (n / UNITS_PER_LINE);
}

static inline int unitn2line    (int n)
{
    return n / UNITS_PER_LINE;
}

static inline int IPPvalue2int(int iv)
{
  int  ret = (iv & 16382) >> 1;

    if (iv & (1 << 14))
        ret |= (~(int)8191);

    return ret;
}

//////////////////////////////////////////////////////////////////////

static int loop_NAF(int N, int A, int F, int *data, int tries)
{
  int  t;
  int  status;
  
    for (t = 0, status = 0;
         t < tries  &&  (status & CAMAC_Q) == 0;
         t++)
        status = DO_NAF(CAMAC_REF, N, A, F, data);
  
    return status;
}

static int write_word(int N, int unit, int addr, int val)
{
  int  aw = ((addr & 0x3F) << 8) | unitn2unitcode(unit);
  int  status;
    
    status = DO_NAF(CAMAC_REF, N, 2, 16, &aw);  // Set address
    SleepBySelect(U0632_USECS);
    if ((status & (CAMAC_X|CAMAC_Q)) != (CAMAC_X|CAMAC_Q)) return status;

    status = DO_NAF(CAMAC_REF, N, 0, 16, &val); // Perform "write" cycle
    SleepBySelect(U0632_USECS);

    return status;
}

static int read_word (int N, int unit, int addr, int *v_p)
{
  int  aw = ((addr & 0x3F) << 8) | unitn2unitcode(unit);
  int  status;
  int  w;
    
    status = DO_NAF(CAMAC_REF, N, 2, 16, &aw);  // Set address
    SleepBySelect(U0632_USECS);
    if ((status & (CAMAC_X|CAMAC_Q)) != (CAMAC_X|CAMAC_Q)) return status;

    status = DO_NAF(CAMAC_REF, N, 0, 0, &w);    // Initiate "read" cycle
    SleepBySelect(U0632_USECS);
    status = DO_NAF(CAMAC_REF, N, 0, 0, &w);    // Read previously requested value from the buffer register
    SleepBySelect(U0632_USECS);

    *v_p = (~w) & 0xFFFE;                 // Convert value from "IPP format" to a normal one
    
    return status;
}

//////////////////////////////////////////////////////////////////////

typedef struct
{
    int             devid;
    int             N_DEV;
    sl_tid_t        tid;
    int             measuring_now;
    struct timeval  measurement_start;

    int             unit_online[U0632_MAXUNITS];
    int32           unit_M     [U0632_MAXUNITS]; // "32" to be suitable
    int32           unit_P     [U0632_MAXUNITS]; // for ReturnChanGroup()
    rflags_t        unit_rflags[U0632_MAXUNITS];

} privrec_t;

static void SetM(privrec_t *me, int unit, int M)
{
    if ((M | SETTING_M_ALWD) == SETTING_M_ALWD)
    {
        me->unit_online[unit] = 1;
        me->unit_M     [unit] = M;
    }
    else
    {
        me->unit_online[unit] = 0;
        me->unit_M     [unit] = 0;
        me->unit_P     [unit] = 0;
    }
}

static void SetP(privrec_t *me, int unit, int P)
{
    if ((P | SETTING_P_ALWD) == SETTING_P_ALWD)
    {
        me->unit_online[unit] = 1;
        me->unit_P     [unit] = P;
    }
    else
    {
        me->unit_online[unit] = 0;
        me->unit_M     [unit] = 0;
        me->unit_P     [unit] = 0;
    }
}

//////////////////////////////////////////////////////////////////////

static int  StartMeasurements(privrec_t *me)
{
  int     unit;
  int     line;
  int     c;
  int     status;
  int     line_active[NUM_LINES];

    /* Program individual boxes */
    bzero(line_active, sizeof(line_active));
    for (unit = 0;  unit < U0632_MAXUNITS;  unit++)
        if (me->unit_online[unit])
        {
            status = write_word(me->N_DEV, unit, U0632_CADDR,
                                (me->unit_M[unit] << SETTING_M_SHIFT) |
                                (me->unit_P[unit] << SETTING_P_SHIFT));
            
            line_active[unitn2line(unit)] = 1;
        }

    /* Enable start */
    for (line = 0;  line < NUM_LINES;  line++)
        if (line_active[line])
        {
            ////DoDriverLog(devid, 0, "Start %d", line);
            c = 040017 | (line << 4);
            status = DO_NAF(CAMAC_REF, me->N_DEV, 2,  16, &c);
            SleepBySelect(U0632_USECS);
            c = 0;
            status = DO_NAF(CAMAC_REF, me->N_DEV, 12, 16, &c);
            SleepBySelect(U0632_USECS);
        }

    return 0;
}

static int  AbortMeasurements(privrec_t *me)
{
    /* No real way to do this? */

    return 1;
}

static void ReadMeasurements (privrec_t *me)
{
  int       unit;
  int       c;
  int       junk;

  int       wire;
  int       cell;
  rflags_t  crflags;
  int32     wdata  [U0632_CHANS_PER_UNIT];
  rflags_t  wrflags[U0632_CHANS_PER_UNIT];
  int32     info[U0632_NUM_PARAMS];

    SleepBySelect(7000); // 7ms delay -- 3.37ms max-delay plus ~2ms

    for (unit = 0;  unit < U0632_MAXUNITS;  unit++)
        if (me->unit_online[unit])
        {
            c = 0 + unitn2unitcode(unit);
            crflags  = status2rflags(loop_NAF(me->N_DEV, 0, 8, &junk, U0632_USECS));
            crflags |= status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, 2, 16, &c));
            DO_NAF(CAMAC_REF, me->N_DEV, 1, 0, &c);

            for (cell = 0;  cell < U0632_CHANS_PER_UNIT;  cell++)
            {
                wire = (cell + U0632_CHANS_PER_UNIT-1) % U0632_CHANS_PER_UNIT;
                crflags |= status2rflags(loop_NAF(me->N_DEV, 0, 8, &junk, U0632_USECS));
                wrflags[wire] = status2rflags(DO_NAF(CAMAC_REF, me->N_DEV, 1, 0, &c));
                wdata  [wire] = IPPvalue2int(c);
                crflags |= wrflags[wire];
            }
            ReturnChanGroup(me->devid,
                            U0632_CHAN_INTDATA_BASE + unit*U0632_CHANS_PER_UNIT,
                            U0632_CHANS_PER_UNIT,
                            wdata, wrflags);
            
            info[U0632_PARAM_M] = me->unit_M[unit];
            info[U0632_PARAM_P] = me->unit_P[unit];
            ReturnBigc(me->devid, unit,
                       info, U0632_NUM_PARAMS,
                       wdata, sizeof(wdata), sizeof(wdata[0]), crflags);
        }
}

static void PerformReturn(privrec_t *me, rflags_t rflags)
{
    /* That is done in ReadMeasurements() as for now... */
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

//////////////////////////////////////////////////////////////////////

static void poll_tout_p(int devid, void *devptr,
                        sl_tid_t tid,
                        void *privptr)
{
  privrec_t *me = (privrec_t *)devptr;

  int  junk;

    sl_enq_tout_after(me->devid, me, POLL_PERIOD, poll_tout_p, me);

    if (!me->measuring_now) return;
    if ((DO_NAF(CAMAC_REF, me->N_DEV, 3, 8, &junk) & CAMAC_Q) != 0)
        drdy_p(me, 1, 0);
} 

static int init_d(int devid, void *devptr,
                  int businfocount, int *businfo,
                  const char *auxinfo)
{
  privrec_t  *me = (privrec_t*)devptr;

  int     online_count;
  int     c;
  int     unit;
  int     s_cw, cword;
  int     s_w1, s_r1, r1;
  int     s_w2, s_r2, r2;
  
  enum {NUM_REPKOV_US = 26};
  
  enum {TEST_VAL_1 = 0xFF00, TEST_VAL_2 = 0xa5e1 & 0xFFFE};

    me->devid = devid;
    me->N_DEV = businfo[0];

    /* Determine which units are online and obtain their settings */
    /*!!! Maybe should broadcast a "stop" command? */
    c = 0;
    DO_NAF(CAMAC_REF, me->N_DEV, 2, 16, &c);
    DO_NAF(CAMAC_REF, me->N_DEV, 0, 16, &c);
    SleepBySelect(10000);

    online_count = 0;
    for (unit = 0; unit < U0632_MAXUNITS;  unit++)
    {
        me->unit_online[unit] = 0;
        
        s_cw = read_word(me->N_DEV, unit, U0632_CADDR, &cword);

        s_w1 = s_r1 = r1 = s_w2 = s_r2 = r2 = 0;
        
        if (s_cw & CAMAC_Q)
        {
            s_w1 = write_word(me->N_DEV, unit, U0632_4TEST, TEST_VAL_1);
            s_r1 = read_word (me->N_DEV, unit, U0632_4TEST, &r1);
            s_w2 = write_word(me->N_DEV, unit, U0632_4TEST, TEST_VAL_2);
            s_r2 = read_word (me->N_DEV, unit, U0632_4TEST, &r2);

            me->unit_rflags[unit] = status2rflags(s_cw & s_w1 & s_r1 & s_w2 & s_r2);
            me->unit_online[unit] =
                ((s_cw & s_w1 & s_r1 & s_w2 & s_r2) == (CAMAC_X | CAMAC_Q)  &&
                 r1 == TEST_VAL_1  &&  r2 == TEST_VAL_2);
        }

        if (me->unit_online[unit])
        {
            online_count++;
            me->unit_M     [unit] = ((cword & SETTING_M_MASK) >> SETTING_M_SHIFT);
            me->unit_P     [unit] = ((cword & SETTING_P_MASK) >> SETTING_P_SHIFT);
        }
        else
        {
            me->unit_M     [unit] = 0;
            me->unit_P     [unit] = 0;
        };
        
        ////DoDriverLog(devid, 0, "Scan [%2d]: %d", unit, me->unit_online[unit]);
        ////DoDriverLog(devid, 0, "Scan [%2d]: %d: %d; %d,%d->%04x; %d,%d->%04x", unit, me->unit_online[unit], s_cw, s_w1, s_r1, r1, s_w2, s_r2, r2);
        DoDriverLog(devid, 0,
                    "Scan: %2d%c  cw=%04x/%c%c  w1=%c%c/%04x/%c%c  w2=%c%c/%04x/%c%c",
                    unit, me->unit_online[unit]?'+':' ',
                    cword,
                    (s_cw & CAMAC_X)? 'X':'-', (s_cw & CAMAC_Q)? 'Q':'-',
                    (s_w1 & CAMAC_X)? 'X':'-', (s_w1 & CAMAC_Q)? 'Q':'-',
                    r1,
                    (s_r1 & CAMAC_X)? 'X':'-', (s_r1 & CAMAC_Q)? 'Q':'-',
                    (s_w2 & CAMAC_X)? 'X':'-', (s_w2 & CAMAC_Q)? 'Q':'-',
                    r2,
                    (s_r2 & CAMAC_X)? 'X':'-', (s_r2 & CAMAC_Q)? 'Q':'-');
    }

    if (online_count == 0)
    {
        DoDriverLog(devid, 0, "No boxes found. Deactivating driver.");
        return DEVSTATE_OFFLINE;
    }

    sl_enq_tout_after(me->devid, me, POLL_PERIOD, poll_tout_p, me);
    
    RequestMeasurements(me);

    return DEVSTATE_OPERATING;
}

static void rdwr_p(int devid, void *devptr,
                   int first, int count, int32 *values, int action)
{
  privrec_t  *me = (privrec_t*)devptr;

  int         n;
  int         x;
  int         unit;

  rflags_t    rflags;
  
    for (n = 0;  n < count;  n++)
    {
        x = first + n;
        switch (x)
        {
            case U0632_CHAN_M_BASE ... U0632_CHAN_M_BASE+U0632_MAXUNITS-1:
                unit = x - U0632_CHAN_M_BASE;
                if (action == DRVA_WRITE) SetM(me, unit, values[n]);
                rflags = me->unit_rflags[unit] |
                    (me->unit_online [unit]? 0 : CXRF_CAMAC_NO_Q);
                ReturnChanGroup(devid, x, 1, me->unit_M + unit, &rflags);
                break;
                
            case U0632_CHAN_P_BASE ... U0632_CHAN_P_BASE+U0632_MAXUNITS-1:
                unit = x - U0632_CHAN_P_BASE;
                if (action == DRVA_WRITE) SetP(me, unit, values[n]);
                rflags = me->unit_rflags[unit] |
                    (me->unit_online [unit]? 0 : CXRF_CAMAC_NO_Q);
                ReturnChanGroup(devid, x, 1, me->unit_P + unit, &rflags);
                break;
        }
    }
}

static void bigc_p(int devid, void *devptr,
                   int bigc,
                   int32 *args, int nargs,
                   void   *data, size_t datasize, size_t dataunits)
{
  privrec_t  *me = (privrec_t*)devptr;

    if (nargs > U0632_PARAM_M  &&  args[U0632_PARAM_M] >= 0)
        SetM(me, bigc, args[U0632_PARAM_M]);
    if (nargs > U0632_PARAM_P  &&  args[U0632_PARAM_P] >= 0)
        SetP(me, bigc, args[U0632_PARAM_P]);
}

DEFINE_DRIVER(u0632, "Repkov's KIPP",
              NULL, NULL,
              sizeof(privrec_t), NULL,
              1, 1,
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_d, NULL, rdwr_p, bigc_p);

/*********************************************************************

  About U0632 programming

  I. Data model

  U0632 "IPP controller" supports up to 30 IPP units -- 2 lines of
  15 units each (4-bit addressing, with "15" used as broadcast address).
  Each unit measures 32 words, which are accessible at addresses 0-31.
  *Exact* use of that 32 measurements depends on particular application.
  (E.g., VEPP-5 uses 30 words -- 15 'X' wires and 15 'Y' wires.)
  

  II. NAF command set


  III. Notes

  1. When the line isn't terminated, the controller can erroneously "see"
  a reply from a missing unit.  So, just trusting Q in NA(0)F(0) isn't
  sufficient.  Besides mandatory terminators (on both ends of line!),
  for "who is there" scan V.Repkov recommends to write+read-back+compare
  some value in the unit's memory.

  2. V.Repkov also highly recommends to use A(0/1)F(8) to check for
  receiver/transmitter readyness (instead of testing Q after A(*)F(0/16)
  and retrying if Q=0).

  3. EVERYTHING read from "boxes" is read in complementary code, with bit 0
  replaced with parity bit.  I.e., after writing 0xFFFF one would read 0x0001,
  0x0000 returns as 0xFFFF.

  4. Memory map:
      - 0-31 hold measurements;
        wires 0-30 are put into words #1..31, while wire 32 is put into word #0.
        (That's because connector's wires are numbered from 1.)
      - 56 (070, 0x38) - status word.
      - All others are free for use.
  
  
*********************************************************************/
