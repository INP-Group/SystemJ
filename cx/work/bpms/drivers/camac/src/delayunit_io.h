#ifndef __DELAYUNIT_IO_H
#define __DELAYUNIT_IO_H


typedef struct
{
    int  dq;
    int  aq;
    int  dp_bits;
    int  ap_bits;
    int  dp_max;
    int  ap_max;
    int  min_val;
    int  max_val;
} delayunit_dscr_t;

static delayunit_dscr_t  v2k_dd   =
{
    DELAYUNIT_V2_DIGITAL_QUANT,
    DELAYUNIT_V2_ANALOG_QUANT,
    DELAYUNIT_V2_DP_BITS,
    DELAYUNIT_V2_AP_BITS,
    DELAYUNIT_V2_DP_MAX,
    DELAYUNIT_V2_AP_MAX,
    DELAYUNIT_V2_MIN_VAL,
    DELAYUNIT_V2_MAX_VAL
};
static delayunit_dscr_t  vepp5_dd =
{
    DELAYUNIT_V5_DIGITAL_QUANT,
    DELAYUNIT_V5_ANALOG_QUANT,
    DELAYUNIT_V5_DP_BITS,
    DELAYUNIT_V5_AP_BITS,
    DELAYUNIT_V5_DP_MAX,
    DELAYUNIT_V5_AP_MAX,
    DELAYUNIT_V5_MIN_VAL,
    DELAYUNIT_V5_MAX_VAL
};

/////////////////////////////

/*
 *  Note:
 *      Delay unit consist of 2 microschemes: 1-st for 5ns delay quant (8 bit), and 2-nd
 *      for 0.25ns (250ps) delay quant (8 bit). But for VEPP-2000 delay unit there is only
 *      5 bit from 0.25ns delay mikroscheme avaliable, because of small number of registers
 *      in ALTERA (acording to private comunications with author - E.A.Bekhtenev)
 *
 *
 *  Note:
 *      Delays are specified in picoseconds, in order to be able
 *      to represent required precision (quant is 0.25ns = 250ps).
 *      So, the factor 1000 is frequently used in the code
 *      to convert between picoseconds and nanoseconds.
 */

static inline int32 delayunit_check_val(delayunit_dscr_t *dd, int32 value)
{
    if (value < dd->min_val) value = dd->min_val;
    if (value > dd->max_val) value = dd->max_val;

    value -= value % dd->dq % dd->aq;

    return value;
}

static inline int32 delayunit_delay2code(delayunit_dscr_t *dd, int32 picos)
{
    int analog_picos;
    int digital_part;
    int analog_part;
    int code;

    picos = delayunit_check_val(dd, picos);

    digital_part = picos / dd->dq;
    if (digital_part > dd->dp_max) digital_part = dd->dp_max;

    analog_picos = picos - digital_part * dd->dq;

    analog_part  = analog_picos / dd->aq;
    if (analog_part  > dd->ap_max) analog_part  = dd->ap_max;

    //code = ( (digital_part & 0x00ff) << 5 ) + ( (analog_part & 0x001f ) << 0 );
    code =
        ( (digital_part & ((1 << dd->dp_bits)-1)) << dd->ap_bits )
        +
        ( (analog_part  & ((1 << dd->ap_bits)-1)) << 0 );
#if 0
    fprintf(stderr, "%s) digital_part = %02d, analog_part = %03d, code = 0x%x, picos = %05d\n",
            __FUNCTION__, digital_part, analog_part, code, picos);
#endif
    return code;
}

static inline int delayunit_code2delay(delayunit_dscr_t *dd, int code)
{
    int picos;
    int digital_part;
    int analog_part;

    code = code & 0x1fff;

    digital_part = (code >> dd->ap_bits) & ((1 << dd->dp_bits)-1);
    analog_part  = (code >> 0)           & ((1 << dd->ap_bits)-1);

    picos = digital_part * dd->dq + analog_part * dd->aq;
#if 0
    fprintf(stderr, "%s) digital_part = %02d, analog_part = %03d, code = 0x%x, picos = %05d\n",
            __FUNCTION__, digital_part, analog_part, code, picos);
#endif
    return picos;
}

static int delayunit_do_io(delayunit_dscr_t *dd,
                           int ref,
                           int N, int chan, int action,
                           int32 *value, rflags_t *rflags,
                           int32 *ret_code)
{
  int32    val = -1;
  rflags_t rfl =  0;
  int      qx, code;
  int32    val_svd;

    if (value == NULL) return -1;

    val = *value;

    if (action == DRVA_WRITE)
    {
        /* Check data */
        val = delayunit_check_val(dd, val);
        /* Encode input data */
        code = delayunit_delay2code(dd, val);
        /* Write data */
        qx  =  DO_NAF(ref, N, chan, 16, &code);
        /* Control read data */
        qx &=~ DO_NAF(ref, N, chan,  0, &code);
    }
    else /* action == DRVA_READ */
    {
        /* Read data */
        qx  =  DO_NAF(ref, N, chan,  0, &code);
    }
    /* Decode data */
    val_svd = val;
    val = delayunit_code2delay(dd, code);
    if (action == DRVA_WRITE  &&  val != val_svd)
        DoDriverLog(DEVID_NOT_IN_DRIVER, DRIVERLOG_C_DATACONV,
                    "%s(): wr=%d rd=%d, NOTEQUAL",
                    __FUNCTION__, val_svd, val);
    rfl = status2rflags(qx);

    if ( value    != NULL ) *value    = val;
    if ( rflags   != NULL ) *rflags   = rfl;
    if ( ret_code != NULL ) *ret_code = code;

    return 1;
}


#endif /* __DELAYUNIT_IO_H */
