#include <stdio.h>

#include "cankoz_pre_lyr.h"
#include "remsrv.h"


remcxsd_dev_t  devinfo[1 + 200]; // An almost arbitrary limit -- >64*2

remcxsd_dev_t *remcxsd_devices = devinfo;
int            remcxsd_maxdevs = countof(devinfo);


static void disable_log_frame(int               nargs     __attribute__((unused)),
                              const char       *argp[]    __attribute__((unused)),
                              void             *context,
                              remsrv_cprintf_t  do_printf)
{
    CanKozSetLogTo(0);
    do_printf(context, "Frame logging is disabled\n");
}

static void enable_log_frame (int               nargs     __attribute__((unused)),
                              const char       *argp[]    __attribute__((unused)),
                              void             *context,
                              remsrv_cprintf_t  do_printf)
{
    CanKozSetLogTo(1);
    do_printf(context, "Frame logging is enabled\n");
}

static void exec_ff(int               nargs     __attribute__((unused)),
                    const char       *argp[]    __attribute__((unused)),
                    void             *context   __attribute__((unused)),
                    remsrv_cprintf_t  do_printf __attribute__((unused)))
{
    CanKozSend0xFF();
}

static remsrv_conscmd_descr_t canserver_commands[] =
{
    {"disable_log_frame", disable_log_frame, ""},
    {"enable_log_frame",  enable_log_frame,  ""},
    {"ff",                exec_ff,           ""},
    {NULL, NULL, NULL}
};

static const char * canserver_clninfo(int devid)
{
  int          dev_line;
  int          dev_kid;
  int          dev_code;

  char         devinfo_buf[100];
  static char  buf2return [100];

    if (CanKozGetDevInfo(devid, &dev_line, &dev_kid, &dev_code) >= 0)
        sprintf(devinfo_buf, "(%d,%-2d 0x%02X)",
                dev_line, dev_kid, dev_code);
    else
        sprintf(devinfo_buf, "-");

    sprintf(buf2return, "%12s", devinfo_buf);

    return buf2return;
}


int main(int argc, char *argv[])
{
    return remsrv_main(argc, argv,
                       "canserver", "CX CANGW canserver",
                       canserver_commands, canserver_clninfo);
}
