#ifndef __PSCTRLUNIT_IO_H
#define __PSCTRLUNIT_IO_H


typedef struct
{
    int  dummy;
} psctrlunit_dscr_t;

static psctrlunit_dscr_t  v2k_pd   =
{
    0
};
static psctrlunit_dscr_t  vepp5_pd =
{
    0
};


static int psctrlunit_do_io(psctrlunit_dscr_t *dd,
                            int ref,
                            int N, int chan, int action,
                            int32 *value, rflags_t *rflags,
                            int32 *ret_code)
{
  int data = *value;
  int save;

    if (chan > PSCTRLUNIT_NUM_GAIN_CHANS)
    {
        *value  = 0xFFFFFFFF;
        *rflags = CXRF_INVAL;
        return 0;
    }

    if      (action == DRVA_READ)
    {
        *value  = 0xFFFFFFFF;
        *rflags = CXRF_UNSUPPORTED;
    }
    else if (action == DRVA_WRITE)
    {
        if (data < PSCTRLUNIT_MIN_GAIN) data = PSCTRLUNIT_MIN_GAIN;
        if (data > PSCTRLUNIT_MAX_GAIN) data = PSCTRLUNIT_MAX_GAIN;

        int status = 0;

        status = DO_NAF(ref, N, chan, 16, &data);
        save = data;
        status = DO_NAF(ref, N, chan,  0, &data);
        if ((data &0xF) != save)
            DoDriverLog(DEVID_NOT_IN_DRIVER, DRIVERLOG_C_DATACONV,
                        "%s(): wr=%d != rd=%d",
                        __FUNCTION__, save, data&0xF);

        *value  = (data & 0xF);
        *rflags = status2rflags(status);
    }

    return 0;
}


#endif /* __PSCTRLUNIT_IO_H */

