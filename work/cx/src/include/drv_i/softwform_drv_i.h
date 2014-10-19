#ifndef __SOFTWFORM_DRV_I
#define __SOFTWFORM_DRV_I


// w10,r10
enum
{
    SOFTWFORM_CHAN_START  = 0,
    SOFTWFORM_CHAN_STOP   = 1,

    SOFTWFORM_CHAN_CUR_X  = 10,
    SOFTWFORM_CHAN_NUMPTS = 11,
};

enum
{
    SOFTWFORM_BIGC_CUR = 0,
    SOFTWFORM_BIGC_NXT = 1,
};

/* General "device" specs */
typedef int32 SOFTWFORM_DATUM_T;
enum
{
    SOFTWFORM_MAX_NUMPTS = 100000,
    SOFTWFORM_DATAUNITS  = sizeof(SOFTWFORM_DATUM_T),
};


#endif /* __SOFTWFORM_DRV_I */
