#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "cx.h"
#include "misclib.h"

/* ========== future pzframe_drv.h ========================================== */
#include <sys/time.h>
#include "cx.h"

enum
{
    PZFRAME_R_READY = +1,
    PZFRAME_R_DOING =  0,
    PZFRAME_R_ERROR = -1,
    PZFRAME_R_IGNOR = -1, // For now -- the same as ERROR
};


struct _pzframe_drv_t_struct;

typedef int32 (*pzframe_validate_param_t)    (struct _pzframe_drv_t_struct *pdr,
                                              int n, int32 v);
typedef int   (*pzframe_start_measurements_t)(struct _pzframe_drv_t_struct *pdr);
typedef int   (*pzframe_trggr_measurements_t)(struct _pzframe_drv_t_struct *pdr);
typedef int   (*pzframe_abort_measurements_t)(struct _pzframe_drv_t_struct *pdr);
typedef void  (*pzframe_read_measurements_t) (struct _pzframe_drv_t_struct *pdr);

typedef struct _pzframe_drv_t_struct
{
    int                           devid;

    int                           num_params;
    int                           param_shot;
    int                           param_istart;
    int                           param_waittime;
    int                           param_stop;
    int                           param_elapsed;

    pzframe_validate_param_t      validate_param;
    pzframe_start_measurements_t  start_measurements;
    pzframe_trggr_measurements_t  trggr_measurements;
    pzframe_abort_measurements_t  abort_measurements;
    pzframe_read_measurements_t   read_measurements;

    int                           state; // Opaque -- used by pzframe_drv internally
    int                           measuring_now;
    int                           tid;
    struct timeval                measurement_start;
    int32                         cur_args[CX_MAX_BIGC_PARAMS];
    int32                         nxt_args[CX_MAX_BIGC_PARAMS];
    void                         *retdata_p;
    size_t                        retdatasize;
    size_t                        retdataunits;
} pzframe_drv_t;


void  pzframe_drv_init(pzframe_drv_t *pdr, int devid,
                       int                           num_params,
                       int                           param_shot,
                       int                           param_istart,
                       int                           param_waittime,
                       int                           param_stop,
                       int                           param_elapsed,
                       pzframe_validate_param_t      validate_param,
                       pzframe_start_measurements_t  start_measurements,
                       pzframe_trggr_measurements_t  trggr_measurements,
                       pzframe_abort_measurements_t  abort_measurements,
                       pzframe_read_measurements_t   read_measurements);
void  pzframe_drv_term(pzframe_drv_t *pdr);

void  pzframe_drv_rw_p  (pzframe_drv_t *pdr,
                         int firstchan, int count,
                         int32 *values, int action);
void  pzframe_drv_bigc_p(pzframe_drv_t *pdr,
                         int32 *args, int nargs);
void  pzframe_drv_drdy_p(pzframe_drv_t *pdr, int do_return, rflags_t rflags);
void  pzframe_set_buf_p (pzframe_drv_t *pdr, void *retdata_p);
/* ========================================================================== */

#include "drv_i/bpmd_drv_i.h"
#define BIVME2_DBODY_RCVBUFSIZE (4096)
#define BIVME2_DBODY_SNDBUFSIZE (4096 + 1 * BPMD_MAX_NUMPTS * BPMD_NUM_LINES * BPMD_DATAUNITS)

#include "cxsd_driver.h"
//#include "pzframe_drv.h"

#include "karpov_pickup_station_functions.h"

extern volatile unsigned char *__libvme_a16;

enum
{
    STATE_MEAS_ASKED,
    STATE_MEAS_DONE,
    
    STATE_SLW_ASKED,
    STATE_SLW_READED,

    STATE_TBT_ASKED,
    STATE_TBT_READED,

    STATE_FULL_DONE,
};

typedef struct
{
    /* should be first because of the macros below */
    pzframe_drv_t   pz;
    int             devid;

    uint32          base_addr;
    int             iohandle;
    uint8           irq_n;
    uint8           irq_vect;

    int             read_data_state;
    int             pts4avr;
    int             line4read;

    BPMD_DATUM_T    retdata[2 * BPMD_MAX_NUMPTS * BPMD_NUM_LINES];
    BPMD_DATUM_T    slow_rd[2 *               1 * BPMD_NUM_LINES];

    int             iz1;   // individual zerro shift for line #1
    int             iz2;   // individual zerro shift for line #2
    int             iz3;   // individual zerro shift for line #3
    int             iz4;   // individual zerro shift for line #4
    int             id1;   // individual delay       for line #1
    int             id2;   // individual delay       for line #2
    int             id3;   // individual delay       for line #3
    int             id4;   // individual delay       for line #4
    int             mgd;   // magic delay
} privrec_t;

#define PDR2ME(pdr) ((privrec_t*)pdr) //!!! Should better sufbtract offsetof(pz)

static psp_paramdescr_t bpmd_params[] =
{
    PSP_P_INT ("iz1", privrec_t, iz1, 0, -8192, 8191 ),
    PSP_P_INT ("iz2", privrec_t, iz2, 0, -8192, 8191 ),
    PSP_P_INT ("iz3", privrec_t, iz3, 0, -8192, 8191 ),
    PSP_P_INT ("iz4", privrec_t, iz4, 0, -8192, 8191 ),

    PSP_P_INT ("id1", privrec_t, id1, 0,  0,    1023 ),
    PSP_P_INT ("id2", privrec_t, id2, 0,  0,    1023 ),
    PSP_P_INT ("id3", privrec_t, id3, 0,  0,    1023 ),
    PSP_P_INT ("id4", privrec_t, id4, 0,  0,    1023 ),

    PSP_P_INT ("mgd", privrec_t, mgd, 0,  0,    7    ),

    PSP_P_END()
};

static int        global_hack_devid;
static privrec_t *global_hack_me;

static inline rflags_t bpmd_code2uv(uint16 code, int32 zero_shift, BPMD_DATUM_T *r_p)
{
    *r_p = code - 8191 - zero_shift; /*!!!*/
    return 0;
}

static int32 ValidateParam(pzframe_drv_t *pdr __attribute__((unused)), int n, int v)
{
    if      (n == BPMD_PARAM_PTSOFS)
    {
        if      (v < 0)                 return 0;
        else if (v > BPMD_MAX_NUMPTS-1) return BPMD_MAX_NUMPTS-1;
        else                            return v;
    }
    else if (n == BPMD_PARAM_NUMPTS)
    {
        if      (v < 1)                 return 1;
        else if (v > BPMD_MAX_NUMPTS)   return BPMD_MAX_NUMPTS;
        else                            return v;
    }
    else if (n == BPMD_PARAM_ATTEN)
    {
        if      (v < 0)                 return 0;
        else if (v > 1)                 return 1;
        else                            return v;
    }
    else if (n == BPMD_PARAM_DECMTN)
    {
        if      (v < 1   )              return 1;
        else if (v > 2048)              return 2048;
        else                            return v;
    }
    else if (n >= BPMD_PARAM_ZERO0  &&  n <= BPMD_PARAM_ZERO3)
    {
        if (v < -2048)                  return -2048;
        if (v > +2047)                  return +2047;
        else                            return v;
    }
    else return v;
}

static void  InitParams(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);

    pdr->nxt_args[BPMD_PARAM_NUMPTS] = 1024;

    pdr->nxt_args[BPMD_PARAM_ZERO0]  = me->iz1;
    pdr->nxt_args[BPMD_PARAM_ZERO1]  = me->iz2;
    pdr->nxt_args[BPMD_PARAM_ZERO2]  = me->iz3;
    pdr->nxt_args[BPMD_PARAM_ZERO3]  = me->iz4;

    pdr->nxt_args[BPMD_PARAM_DLAY0]  = me->id1;
    pdr->nxt_args[BPMD_PARAM_DLAY1]  = me->id2;
    pdr->nxt_args[BPMD_PARAM_DLAY2]  = me->id3;
    pdr->nxt_args[BPMD_PARAM_DLAY3]  = me->id4;

    pdr->nxt_args[BPMD_PARAM_MGDLY]  = me->mgd;

    pdr->retdataunits = BPMD_DATAUNITS;
}

static void WriteIndividualDelay(pzframe_drv_t *pdr)
{
  uint16 w;
    /* === */
    w = pdr->cur_args[BPMD_PARAM_DLAY0];
    PS_RegWrite(PS_REG_INDV_DEL_1, w);
    w = ( 2 << 3 ) | ( 1 << 2 ) | ( 0 << 0 );
    PS_RegWrite(PS_REG_DEL_CH_SEL, w);

    /* === */
    w = pdr->cur_args[BPMD_PARAM_DLAY1];
    PS_RegWrite(PS_REG_INDV_DEL_2, w);
    w = ( 2 << 3 ) | ( 1 << 2 ) | ( 1 << 0 );
    PS_RegWrite(PS_REG_DEL_CH_SEL, w);

    /* === */
    w = pdr->cur_args[BPMD_PARAM_DLAY2];
    PS_RegWrite(PS_REG_INDV_DEL_3, w);
    w = ( 2 << 3 ) | ( 1 << 2 ) | ( 2 << 0 );
    PS_RegWrite(PS_REG_DEL_CH_SEL, w);

    /* === */
    w = pdr->cur_args[BPMD_PARAM_DLAY3];
    PS_RegWrite(PS_REG_INDV_DEL_4, w);
    w = ( 2 << 3 ) | ( 1 << 2 ) | ( 3 << 0 );
    PS_RegWrite(PS_REG_DEL_CH_SEL, w);
    return;
}

static int  StartMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
  uint16 w;

    /* Additional check of PTSOFS against NUMPTS */
    if (me->pz.cur_args[BPMD_PARAM_PTSOFS] > BPMD_MAX_NUMPTS - me->pz.cur_args[BPMD_PARAM_NUMPTS])
        me->pz.cur_args[BPMD_PARAM_PTSOFS] = BPMD_MAX_NUMPTS - me->pz.cur_args[BPMD_PARAM_NUMPTS];

    /* Store datasize for future use */
    me->pz.retdatasize = me->pz.cur_args[BPMD_PARAM_NUMPTS] *
                                         BPMD_NUM_LINES     *
                                         BPMD_DATAUNITS;
    
    /* Program the device */
    /* 1.  Stop */
    /* 1.a stop previous measurements, if any */
    PS_MeasStop();
    /* 1.b drop IRQ previously rised IRQ */
    DropIRQ();

    /* 2.  Configure measurements .... */
    /* 2.a enable external start */
    w = PS_REG_STAT_FLAG_EXT_START * 1 |
        PS_REG_STAT_FLAG_CALIBR_ON * 0 |
        PS_REG_STAT_FLAG_ATTENN_ON * (1 - me->pz.cur_args[BPMD_PARAM_ATTEN]);
    PS_RegWrite(PS_REG_STAT_CONFG, w);

    /* 2.b calculate decimation factor */
    w  = (me->pz.cur_args[BPMD_PARAM_DECMTN] - 1);
    PS_RegWrite(PS_REG_DECIMATION, w);

    /* 2.c calculation of numpts for averaging */
    int pts4avr = (BPMD_MAX_NUMPTS * 0 + 10000) * me->pz.cur_args[BPMD_PARAM_DECMTN];
    PS_Pts2AvrWr(pts4avr);
    me->pts4avr = pts4avr;

    /* 2.d warite individual delayjs for each line */
    WriteIndividualDelay(pdr);

    /* 2.e calculate delay, program delay, write delay in real registers */
    int del_set_for_usege = 1;      // use the 1 set of delay registers
    me->pz.cur_args[BPMD_PARAM_DELAY] = PS_DelayWr(me->pz.cur_args[BPMD_PARAM_DELAY], del_set_for_usege);
    int i = del_set_for_usege - 1;  // in Karpov's notation
    w = ( i << 3 ) | ( 0 << 2 ) | ( i << 0 );
    PS_RegWrite(PS_REG_DEL_CH_SEL, w);

    /* 2.f set delay between RF and F_0_SEP */
    w = me->pz.cur_args[BPMD_PARAM_MGDLY];
    PS_RegWrite(PS_REG_SYNC_DELAY, w);

    /* 3. ....... */

    /* 4.  Prepare infrastructure for reading measurements in future */
    /* 4.a Clear? incomeing buffer and alow write to it */
    LinkD_PrepareInbuf();

    /* 5.  Start measurements */
    /* 5.a Force Pickup Station to start real data asquisition  */
    PS_MeasStart();
    me->read_data_state = STATE_MEAS_ASKED;
#if 1
    DoDriverLog(me->pz.devid, 0, "numpts/ptsofs/del/atten/int = %d/%d/%d/%s/%s",
        me->pz.cur_args[BPMD_PARAM_NUMPTS], me->pz.cur_args[BPMD_PARAM_PTSOFS],
        me->pz.cur_args[BPMD_PARAM_DELAY],
        me->pz.cur_args[BPMD_PARAM_ATTEN]  ? "on" : "off",
        me->pz.cur_args[BPMD_PARAM_ISTART] ? "on" : "off");
#endif
    SleepBySelect(10 * 1000); // 10 ms
#endif
    return PZFRAME_R_DOING;
}

static int  TrggrMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
    uint16 w;
    /* 1. Stop previously startet measurements */
    PS_MeasStop();

    /* 2. Rewrite type of start */
    w = PS_REG_STAT_FLAG_EXT_START * 0 |
        PS_REG_STAT_FLAG_CALIBR_ON * 0 |
        PS_REG_STAT_FLAG_ATTENN_ON * (1 - me->pz.cur_args[BPMD_PARAM_ATTEN]);
    PS_RegWrite(PS_REG_STAT_CONFG, w);

    /* 3. Start device again */
    PS_MeasStart();
    me->read_data_state = STATE_MEAS_ASKED;

    SleepBySelect(10 * 1000); // 10 ms
#endif
    return PZFRAME_R_DOING;
}

static int  AbortMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
    /* 1. Stop device */
    PS_MeasStop();
    
    /* 2. Drop IRQ */
    DropIRQ();

    if (me->pz.retdatasize != 0) bzero(me->retdata, me->pz.retdatasize);
#endif
    return PZFRAME_R_READY;
}

static void ReadTBTMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
  uint16         w;
  BPMD_DATUM_T   *dp;
  int            nc;
  int32          zero_shift;
  enum          {COUNT_MAX = 256, COUNT_JUNK = BPMD_JUNK_MSMTS};
  int            count;
  int            x;
  // int            rdata[COUNT_MAX];
  int            v;
  BPMD_DATUM_T   vd;
  int32          min_v, max_v, sum_v;

// ==========
  int            page;
  const int      line4read = me->line4read;

  const int skip_p = me->pz.cur_args[BPMD_PARAM_PTSOFS] / COUNT_MAX;
  const int skip_x = me->pz.cur_args[BPMD_PARAM_PTSOFS] % COUNT_MAX + COUNT_JUNK; 

    zero_shift = line4read == 0 ? me->pz.cur_args[BPMD_PARAM_ZERO0] :
                 line4read == 1 ? me->pz.cur_args[BPMD_PARAM_ZERO1] :
                 line4read == 2 ? me->pz.cur_args[BPMD_PARAM_ZERO2] :
                 line4read == 3 ? me->pz.cur_args[BPMD_PARAM_ZERO3] : 0;

    if (me->pz.cur_args[BPMD_PARAM_CALC_STATS] == 0)
    {
        for (
             nc   = me->pz.cur_args[BPMD_PARAM_PTSOFS] + me->pz.cur_args[BPMD_PARAM_NUMPTS] + COUNT_JUNK,
             dp   = me->retdata + line4read * me->pz.cur_args[BPMD_PARAM_NUMPTS],
             page = skip_p
             ;
             nc   > 0
             ;
             nc  -= count,
             page++
            )
        {
            count = nc;
            if (count > COUNT_MAX) count = COUNT_MAX;

            LinkD_RegWrite(LD_REG_PAGE_SEL, page);

            for ( x = 0; x < count; x++ )
            {
                if      (page == skip_p && x < skip_x)
                {
                    /* do nothing */
                }
                else
                {
                    // LinkD_InbufCellRead(x, &w);
                    w = *(unsigned short *)(__libvme_a16 + me->base_addr + inbuf_cell_addr(x));
                    v = w;
                    bpmd_code2uv(w, zero_shift, dp);
                    dp++;
                }
            }

            me->pz.cur_args[BPMD_PARAM_MIN0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_MIN0 + nc] =
            me->pz.cur_args[BPMD_PARAM_MAX0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_MAX0 + nc] =
            me->pz.cur_args[BPMD_PARAM_AVG0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_AVG0 + nc] =
            me->pz.cur_args[BPMD_PARAM_INT0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_INT0 + nc] = 0;
        }
    }
    else
    {
        min_v = 0xFFFF; max_v = 0; sum_v = 0;
        for (
             nc   = me->pz.cur_args[BPMD_PARAM_PTSOFS] + me->pz.cur_args[BPMD_PARAM_NUMPTS] + COUNT_JUNK,
             dp   = me->retdata + line4read * me->pz.cur_args[BPMD_PARAM_NUMPTS],
             page = skip_p
             ;
             nc   > 0
             ;
             nc  -= count,
             page++
            )
        {
            count = nc;
            if (count > COUNT_MAX) count = COUNT_MAX;

            LinkD_RegWrite(LD_REG_PAGE_SEL, page);

            for ( x = 0; x < count; x++ )
            {
                if      (page == skip_p && x < skip_x)
                {
                    /* do nothing */
                }
                else
                {
                    // LinkD_InbufCellRead(x, &w);
                    w = *(unsigned short *)(__libvme_a16 + me->base_addr + inbuf_cell_addr(x));
                    v = w;
                    bpmd_code2uv(w, zero_shift, dp);
                    dp++;
                    if (min_v > v) min_v = v;
                    if (max_v < v) max_v = v;
                    sum_v += v;
                }
            }

            bpmd_code2uv(min_v, zero_shift, &vd);
            me->pz.cur_args[BPMD_PARAM_MIN0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_MIN0 + nc] = vd;

            bpmd_code2uv(max_v, zero_shift, &vd);
            me->pz.cur_args[BPMD_PARAM_MAX0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_MAX0 + nc] = vd;

            bpmd_code2uv(sum_v / me->pz.cur_args[BPMD_PARAM_NUMPTS], zero_shift, &vd);
            me->pz.cur_args[BPMD_PARAM_AVG0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_AVG0 + nc] = vd;

            vd = sum_v - (2048 + zero_shift) * me->pz.cur_args[BPMD_PARAM_NUMPTS];
            me->pz.cur_args[BPMD_PARAM_INT0 + nc] =
            me->pz.nxt_args[BPMD_PARAM_INT0 + nc] = vd;
        }
    }
#endif
}

static void ReadSLWMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
  uint16     w1, w2, w;
  int32      line4read;
  int32      avg;
  int32      zero_shift;

  union {int32 i32; float32 f32;} v;

    LinkD_RegWrite(LD_REG_PAGE_SEL, 0);

    for (line4read = 0; line4read < BPMD_NUM_LINES; line4read++)
    {
        w = 3 + 2*line4read + 0;
        LinkD_InbufCellRead(w, &w1);
        w = 3 + 2*line4read + 1;
        LinkD_InbufCellRead(w, &w2);

        v.i32 = ( (w1 << 16) | (w2 << 0) );
        avg   = rint( me->pts4avr != 0 ? v.f32 / me->pts4avr : v.f32 / 1 );

        zero_shift = line4read == 0 ? me->pz.cur_args[BPMD_PARAM_ZERO0] :
                     line4read == 1 ? me->pz.cur_args[BPMD_PARAM_ZERO1] :
                     line4read == 2 ? me->pz.cur_args[BPMD_PARAM_ZERO2] :
                     line4read == 3 ? me->pz.cur_args[BPMD_PARAM_ZERO3] : 0;

        me->pz.cur_args[BPMD_PARAM_AVG0 + line4read] = avg - zero_shift;
    }
#endif
    return;
}

static void ReadMeasurements(pzframe_drv_t *pdr)
{
  privrec_t *me = PDR2ME(pdr);
#if 1
    int      do_return = 1;
    rflags_t rflags    = 0;

    switch(me->read_data_state)
    {
        case STATE_MEAS_ASKED:
            me->read_data_state = STATE_MEAS_DONE;
            ReadMeasurements(pdr);
            break;

        case STATE_MEAS_DONE:
            PS_MeasAskSLW();
            me->read_data_state = STATE_SLW_ASKED;            
            /* ReadMeasurements will be asked after feature irq */
            break;

        case STATE_SLW_ASKED:
            ReadSLWMeasurements(pdr);
            me->read_data_state = STATE_SLW_READED;

            ReadMeasurements(pdr);
            break;
        
        case STATE_SLW_READED:
            me->line4read = 0;
            PS_MeasAskTBT(me->line4read);
            me->read_data_state = STATE_TBT_ASKED;
            /* ReadMeasurements will be asked after feature irq */
            break;

        case STATE_TBT_ASKED:
            ReadTBTMeasurements(pdr);
            if (me->line4read < BPMD_NUM_LINES)
            {
                me->line4read += 1;
                PS_MeasAskTBT(me->line4read);
                me->read_data_state = STATE_TBT_ASKED;
                /* ReadMeasurements will be asked after feature irq */
            }
            else
            {
                me->read_data_state = STATE_TBT_READED;
                me->line4read = 0;
                ReadMeasurements(pdr);
            }
            break;

    case STATE_TBT_READED:
            me->read_data_state = STATE_FULL_DONE;
            ReadMeasurements(pdr);
        break;

        case STATE_FULL_DONE:
            pzframe_drv_drdy_p(pdr, do_return, rflags);
            break;

        default:
            break;
    }
#endif
    return;
}

static void PickIRQSourceAndPerfomAction (int devid, void *devptr)
{
  privrec_t  *me = (privrec_t*)devptr;
#if 1
  uint16      w = 0, cmd, cmd_mod, blk_addr;
  float       freq;

    /* Try to understand the source of IRQ */
    /*
       1. Read first 3 cells of inbuf:
            #0 cell -> addres, should be 0x1224
            #1 cell -> command in standart notation:
                    bit 7-5 -> command for pickup station
                    bit 4-0 -> reg #, or command modifier
            #2 cell -> data (depend on command value)
     */
    
    /* You MUST!!!! olways select zero page */
    LinkD_RegWrite(LD_REG_PAGE_SEL, 0);

    LinkD_InbufCellRead(0, &w);
    blk_addr = w;
    // DoDriverLog(devid, 0, "inbuf cell #0 = 0x%04x", w);

    LinkD_InbufCellRead(1, &w);
    cmd     = (w >> 5) & 0x07;
    cmd_mod = (w >> 0) & 0x1f;
    // DoDriverLog(devid, 0, "inbuf cell #1 = 0x%04x, command = %d, modifier = %d", w, cmd, cmd_mod);

    LinkD_InbufCellRead(2, &w);
    // DoDriverLog(devid, 0, "inbuf cell #2 = 0x%04x", w);

    if ( blk_addr != 0x1224 )
    {
        DoDriverLog(devid, 0, "WARNING!!!! Wrong block addres = 0x%04x", blk_addr);
        goto FATAL;
    }

    DropIRQ();
    // DoDriverLog(devid, 0, "%s:%d:%s() ME HERE!!!!!!", __FILE__, __LINE__, __FUNCTION__);

    /* Try to do the real work */
    switch ( cmd )
    {
        case PS_CMD_REG_WR:    /* don't raise IRQ */
        case PS_CMD_REG_RD:    /* don't raise IRQ */
            break;

        case PS_CMD_CYCLESTRT:
            freq = 25.0 * w / 8192;
            // DoDriverLog(devid, 0, "sep_0_freq = %f [0x%04x]", freq, w);
            LinkD_PrepareInbuf();
            DropIRQ();
            ReadMeasurements(&(me->pz));
            break;
        case PS_CMD_CYCLESTOP: /* don't raise IRQ */
            break;

        case PS_CMD_SLW_RD:
        case PS_CMD_TBT_RD:
            LinkD_PrepareInbuf();
            DropIRQ();
            ReadMeasurements(&(me->pz));
            break;

        default:
            DoDriverLog(devid, 0, "Attention: cmd = %d, cmd_mod = %d", cmd, cmd_mod);
            goto FATAL;
    }
    return;

FATAL:
    AbortMeasurements(&(me->pz));
    me->read_data_state = STATE_FULL_DONE;
    ReadMeasurements(&(me->pz));
#endif
    return;
}

//////////////////////////////////////////////////////////////////////

enum
{
    PARAM_SHOT     = BPMD_PARAM_SHOT,
    PARAM_STOP     = BPMD_PARAM_STOP,
    PARAM_ISTART   = BPMD_PARAM_ISTART,
    PARAM_WAITTIME = BPMD_PARAM_WAITTIME,
    PARAM_ELAPSED  = BPMD_PARAM_ELAPSED,

    NUM_PARAMS     = BPMD_NUM_PARAMS,
};

static void irq_p(int devid, void *devptr, int irq_n, int vector)
{
  privrec_t  *me = (privrec_t*)devptr;

    if (irq_n != me->irq_n)
    {
        DoDriverLog(devid, 0, "strange irq_num: %d",   irq_n);
        return;
    }

    if (vector != me->irq_vect)
    {
        DoDriverLog(devid, 0, "strange irq_vect: %d", vector);
        return;
    }

    // DoDriverLog(devid, 0, "There was interruption on irq/vector = %d/%d.", irq_n, vector);
    // DropIRQ();
    PickIRQSourceAndPerfomAction(devid, devptr);
    
    return;
}

static int  init_b(int devid, void *devptr, 
                   int businfocount, int businfo[],
                   const char *auxinfo __attribute__((unused)))
{
  privrec_t   *me = (privrec_t *)devptr;

    me->devid  = devid;
    
    global_hack_devid = devid;
    global_hack_me    = me;

// ==========
    int jumpers;
    if (businfocount < 1) return -CXRF_CFG_PROBL;
    jumpers       = businfo[0];
    me->irq_n     = businfocount > 1 ? businfo[1] &  0x7 : 0;
    me->irq_vect  = businfocount > 2 ? businfo[2] & 0xFF : 0;
    
    me->base_addr = jumpers;
    me->irq_vect  = 255;
    
    me->iohandle = bivme2_io_open(devid, devptr,
                                  me->base_addr, 0, ADDRESS_MODIFIER,
                                  me->irq_n, irq_p);

    fprintf(stderr, "businfo[0]=%08x jumpers=0x%x irq=%d\n", businfo[0], jumpers, me->irq_n);
    fprintf(stderr, "open=%d\n", me->iohandle);

    if (me->iohandle < 0)
    {
        DoDriverLog(devid, 0, "open: %d/%s", me->iohandle, strerror(errno));
        return -CXRF_DRV_PROBL;
    }

    InitKarpovLibrary(me->iohandle, me->irq_n, me->irq_vect);
// ==========

    pzframe_drv_init(&(me->pz), devid,
                     NUM_PARAMS,
                     PARAM_SHOT, PARAM_ISTART, PARAM_WAITTIME, PARAM_STOP, PARAM_ELAPSED,
                     ValidateParam,
                     StartMeasurements, TrggrMeasurements,
                     AbortMeasurements, ReadMeasurements);
    pzframe_set_buf_p(&(me->pz), me->retdata);

    InitParams(&(me->pz));
    DropIRQ();
    LinkD_EnableIRQ(1);

    return DEVSTATE_OPERATING;
}

static void term_b(int devid __attribute__((unused)), void *devptr)
{
  privrec_t   *me = (privrec_t *)devptr;

    pzframe_drv_term(&(me->pz));
}

static void rdwr_p(int devid __attribute__((unused)), void *devptr,
                   int firstchan, int count, int32 *values, int action)
{
  privrec_t   *me = (privrec_t *)devptr;

    pzframe_drv_rw_p(&(me->pz), firstchan, count, values, action);
}

static void bigc_p(int devid __attribute__((unused)), void *devptr,
                   int chan __attribute__((unused)),
                   int32 *args, int nargs,
                   void *data __attribute__((unused)), size_t datasize __attribute__((unused)), size_t dataunits __attribute__((unused)))
{
  privrec_t   *me = (privrec_t *)devptr;

    pzframe_drv_bigc_p(&(me->pz), args, nargs);
}

DEFINE_DRIVER(bivme2_bpmd, "driver for VME Pickup Station",
              NULL, NULL,
              sizeof(privrec_t), bpmd_params,
              0, 20, /*!!! CXSD_DB_MAX_BUSINFOCOUNT */
              NULL, 0,
              -1, NULL,
              -1, NULL,
              init_b, term_b, rdwr_p, bigc_p);


/* ========== realization of future pzframe_drv.c =========================== */
#include "timeval_utils.h"

static int32     zero_value = 0;
static rflags_t  rf_bad     = {CXRF_UNSUPPORTED};
static rflags_t  zero_rflags[CX_MAX_BIGC_PARAMS] = {[0 ... CX_MAX_BIGC_PARAMS-1] = 0};


static void pzframe_tout_p(int devid, void *devptr,
                           int tid,
                           void *privptr);

static void ReturnMeasurements (pzframe_drv_t *pdr, rflags_t rflags)
{
    pdr->read_measurements(pdr);
    ReturnBigc(pdr->devid, 0, pdr->cur_args, pdr->num_params,
               pdr->retdata_p, pdr->retdatasize, pdr->retdataunits,
               rflags);
    ReturnChanGroup(pdr->devid, 0, pdr->num_params, pdr->cur_args, zero_rflags);
}

static void SetDeadline(pzframe_drv_t *pdr)
{
  struct timeval   msc; // timeval-representation of MilliSeConds
  struct timeval   dln; // DeadLiNe

    if (pdr->param_waittime >=  0  &&
        pdr->cur_args[pdr->param_waittime] > 0)
    {
        msc.tv_sec  =  pdr->cur_args[pdr->param_waittime] / 1000;
        msc.tv_usec = (pdr->cur_args[pdr->param_waittime] % 1000) * 1000;
        timeval_add(&dln, &(pdr->measurement_start), &msc);
        
        pdr->tid = sl_enq_tout_at(pdr->devid, NULL, &dln,
                                  pzframe_tout_p, pdr);
    }
}

static void RequestMeasurements(pzframe_drv_t *pdr)
{
  int  r;

    memcpy(pdr->cur_args, pdr->nxt_args, sizeof(int32) * pdr->num_params);

    r = pdr->start_measurements(pdr);
    if (r == PZFRAME_R_READY)
        ReturnMeasurements(pdr, 0);
    else
    {
        pdr->measuring_now = 1;
        gettimeofday(&(pdr->measurement_start), NULL);
        SetDeadline(pdr);

        if (pdr->param_istart >= 0  &&
            (pdr->cur_args[pdr->param_istart] & CX_VALUE_LIT_MASK) != 0)
            pdr->trggr_measurements(pdr);
    }
}

static void PerformTimeoutActions(pzframe_drv_t *pdr, int do_return)
{
    pdr->measuring_now = 0;
    if (pdr->tid >= 0)
    {
        sl_deq_tout(pdr->tid);
        pdr->tid = -1;
    }
    pdr->abort_measurements(pdr);
    if (do_return)
        ReturnMeasurements (pdr, CXRF_IO_TIMEOUT);
    else
        RequestMeasurements(pdr);
}

static void pzframe_tout_p(int devid, void *devptr __attribute__((unused)),
                           int tid __attribute__((unused)),
                           void *privptr)
{
  pzframe_drv_t *pdr = (pzframe_drv_t *)privptr;

    pdr->tid = -1;
    if (pdr->measuring_now == 0  ||
        pdr->param_waittime < 0  ||
        pdr->cur_args[pdr->param_waittime] <= 0)
    {
        DoDriverLog(devid, DRIVERLOG_WARNING,
                    "strange timeout: measuring_now=%d, waittime=%d",
                    pdr->measuring_now,
                    pdr->param_waittime < 0 ? -123456789 : pdr->cur_args[pdr->param_waittime]);
        return;
    }

    PerformTimeoutActions(pdr, 1);
}

//////////////////////////////////////////////////////////////////////


void  pzframe_drv_init(pzframe_drv_t *pdr, int devid,
                       int                           num_params,
                       int                           param_shot,
                       int                           param_istart,
                       int                           param_waittime,
                       int                           param_stop,
                       int                           param_elapsed,
                       pzframe_validate_param_t      validate_param,
                       pzframe_start_measurements_t  start_measurements,
                       pzframe_trggr_measurements_t  trggr_measurements,
                       pzframe_abort_measurements_t  abort_measurements,
                       pzframe_read_measurements_t   read_measurements)
{
    bzero(pdr, sizeof(*pdr));
    pdr->devid              = devid;

    pdr->num_params         = num_params;
    pdr->param_shot         = param_shot;
    pdr->param_istart       = param_istart;
    pdr->param_waittime     = param_waittime;
    pdr->param_stop         = param_stop;
    pdr->param_elapsed      = param_elapsed;
    pdr->validate_param     = validate_param;
    pdr->start_measurements = start_measurements;
    pdr->trggr_measurements = trggr_measurements;
    pdr->abort_measurements = abort_measurements;
    pdr->read_measurements  = read_measurements;

    pdr->tid                = -1;
}

void  pzframe_drv_term(pzframe_drv_t *pdr)
{
    if (pdr->tid >= 0)
    {
        sl_deq_tout(pdr->tid);
        pdr->tid = -1;
    }
}

void  pzframe_drv_rw_p  (pzframe_drv_t *pdr,
                         int firstchan, int count,
                         int32 *values, int action)
{
  int         n;
  int         x;
  int32       v;

  struct timeval   now;

    if (action == DRVA_WRITE)
    {
        for (n = 0;  n < count;  n++)
        {
            x = firstchan + n;
            if      (x >= pdr->num_params)
            {
                ReturnChanGroup(pdr->devid, x, 1, &zero_value, &rf_bad);
                goto NEXT_PARAM;
            }
            
            v = pdr->validate_param(pdr, x, values[n]);
            if      (x == pdr->param_shot)
            {
                ////DoDriverLog(devid, 0, "SHOT!");
                if (pdr->measuring_now  &&  v == CX_VALUE_COMMAND)
                    pdr->trggr_measurements(pdr);
                ReturnChanGroup(pdr->devid, x, 1, &zero_value, &(zero_rflags[0]));
            }
            else if (x == pdr->param_waittime)
            {
                if (v < 0) v = 0;
                pdr->cur_args[x] = pdr->nxt_args[x] = v;
                ReturnChanGroup(pdr->devid, x, 1, pdr->nxt_args + x, &(zero_rflags[0]));
                /* Adapt to new waittime */
                if (pdr->measuring_now)
                {
                    if (pdr->tid >= 0)
                    {
                        sl_deq_tout(pdr->tid);
                        pdr->tid = -1;
                    }
                    SetDeadline(pdr);
                }
            }
            else if (x == pdr->param_stop)
            {
                if (pdr->measuring_now  &&  
                    (v &~ CX_VALUE_DISABLED_MASK) == CX_VALUE_COMMAND)
                    PerformTimeoutActions(pdr,
                                          (v & CX_VALUE_DISABLED_MASK) == 0);
                ReturnChanGroup(pdr->devid, x, 1, &zero_value, &(zero_rflags[0]));
            }
            else
            {
                if (x == pdr->param_elapsed) v = 0;
                pdr->nxt_args[x] = v;
                ReturnChanGroup(pdr->devid, x, 1, pdr->nxt_args + x, &(zero_rflags[0]));
            }
        NEXT_PARAM:;
        }
    }
    else /* DRVA_READ */
    {
        if (firstchan + count <= pdr->num_params)
            ReturnChanGroup(pdr->devid, firstchan, count, pdr->nxt_args + firstchan, zero_rflags);
        else
        {
            for (n = 0;  n < count;  n++)
            {
                x = firstchan + n;
                if (x == pdr->param_elapsed  ||  x >= pdr->num_params)
                {
                    /* Treat all channels above NUMCHANS as time-since-measurement-start */
                    if (!pdr->measuring_now)
                        v = 0;
                    else
                    {
                        gettimeofday(&now, NULL);
                        timeval_subtract(&now, &now, &(pdr->measurement_start));
                        v = now.tv_sec * 1000 + now.tv_usec / 1000;
                    }
                    ReturnChanGroup(pdr->devid, x, 1, &v, &(zero_rflags[0]));
                }
                else
                    ReturnChanGroup(pdr->devid, x, 1, pdr->nxt_args + x, &(zero_rflags[0]));
            }
        }
    }
}

void  pzframe_drv_bigc_p(pzframe_drv_t *pdr,
                         int32 *args, int nargs)
{
  int                  n;
  int32                v;
  
    if (pdr->measuring_now) return;
    
    DoDriverLog(pdr->devid, DRIVERLOG_DEBUG, "%s(): nargs=%d", __FUNCTION__ , nargs);
    for (n = 0;  n < pdr->num_params  &&  n < nargs;  n++)
    {
        v = pdr->validate_param(pdr, n, args[n]);
        if (n == pdr->param_shot  ||  n == pdr->param_stop) v = 0; // SHOT and STOP are regular_channel-only
        if (n == pdr->param_elapsed) v = 0;
        pdr->nxt_args[n] = v;
    }
    RequestMeasurements(pdr);
}

void  pzframe_drv_drdy_p(pzframe_drv_t *pdr, int do_return, rflags_t rflags)
{
    if (pdr->measuring_now == 0) return;
    pdr->measuring_now = 0;
    if (pdr->tid >= 0)
    {
        sl_deq_tout(pdr->tid);
        pdr->tid = -1;
    }
    if (do_return)
        ReturnMeasurements (pdr, rflags);
    else
        RequestMeasurements(pdr);
}

void  pzframe_set_buf_p (pzframe_drv_t *pdr, void *retdata_p)
{
    pdr->retdata_p = retdata_p;
}
/* ========================================================================== */
