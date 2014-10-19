#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "misc_macros.h"
#include "cxsd_driver.h"

#include "cankoz_pre_lyr.h"
#include "drv_i/canpcsc8_drv_i.h"


/* PCSC8 specifics */

enum
{
    DEVTYPE   = 19, /* PCSC8 is 6 */
};

enum
{
    DESC_WRITE_SETTINGS = 0xE0,
    DESC_READ_SETTINGS  = 0xE1,
    DESC_READ_DATA      = 0xE2,
    DESC_WRITE_LATCHES  = 0xE3
};

enum
{
    SETTING_MASK_SEND_ON_1ST = 1,
    SETTING_MASK_SEND_ON_ANY = 2,
};


typedef struct
{
    cankoz_liface_t     *iface;
    int                  handle;
} privrec_t;


static void canpcsc8_rst(int devid, void *devptr, int is_a_reset);
static void canpcsc8_in(int devid, void *devptr,
                        int desc, int datasize, uint8 *data);

static int canpcsc8_init_d(int devid, void *devptr, 
                           int businfocount, int businfo[],
                           const char *auxinfo)
{
  privrec_t *me    = (privrec_t *) devptr;
    
    DoDriverLog(devid, 0 | DRIVERLOG_C_ENTRYPOINT, "ENTRY %s(%s)", __FUNCTION__, auxinfo);

    /* Initialize interface */
    me->iface  = CanKozGetIface();
    me->handle = me->iface->add(devid, devptr,
                                businfocount, businfo,
                                DEVTYPE,
                                canpcsc8_rst, canpcsc8_in,
                                CANPCSC8_NUMCHANS * 2, -1);
    if (me->handle < 0) return me->handle;

    return DEVSTATE_OPERATING;
}

static void canpcsc8_term_d(int devid, void *devptr __attribute__((unused)))
{
  privrec_t *me    = (privrec_t *) devptr;
  uint8      data[8];

    data[0] = DESC_WRITE_SETTINGS;
    data[1] = 0;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
  
    me->iface->disconnect(devid);
}

static void canpcsc8_rst(int   devid    __attribute__((unused)),
                         void *devptr,
                         int   is_a_reset __attribute__((unused)))
{
  privrec_t  *me     = (privrec_t *) devptr;
  uint8       data[8];

    data[0] = DESC_WRITE_SETTINGS;
    data[1] = SETTING_MASK_SEND_ON_1ST | SETTING_MASK_SEND_ON_ANY;
    me->iface->send_frame(me->handle, CANKOZ_PRIO_UNICAST, 2, data);
}

static void canpcsc8_in(int devid, void *devptr __attribute__((unused)),
                        int desc, int datasize, uint8 *data)
{
}

static void canpcsc8_rw_p(int devid, void *devptr, int firstchan, int count, int *values, int action)
{
  privrec_t  *me     = (privrec_t *) devptr;
  int         n;
  int         x;

    for (n = 0;  n < count;  n++)
    {
        x = firstchan + n;

    }
}


/* Metric */

static CxsdMainInfoRec canpcsc8_main_info[] =
{
    {CANPCSC8_NUMLINES, 0}, // Status
    {CANPCSC8_NUMLINES, 0}, // Latches
    {CANPCSC8_NUMLINES, 0}, // "1st"
    {CANPCSC8_NUMLINES, 1}, // Write
    {3,                 0}, // {Status,Latch,1st}1
    {1,                 1}, // Write1
};

DEFINE_DRIVER(canpcsc8, NULL,
              NULL, NULL,
              sizeof(privrec_t), NULL,
              2, 3,
              NULL, 0,
              countof(canpcsc8_main_info), canpcsc8_main_info,
              0, NULL,
              canpcsc8_init_d, canpcsc8_term_d,
              canpcsc8_rw_p, NULL);
