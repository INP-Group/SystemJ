#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "misclib.h"

#include "cda.h"
#include "Xh.h"
#include "Xh_globals.h"
#include "Knobs.h"
#include "KnobsI.h"
#include "Knobs_typesP.h"
#include "Chl.h"
#include "Cdr_script.h"

#include "key_offon_knobplugin.h"


enum
{
    KV_NONE   = -1,
    KV_ABSENT = 0,
    KV_OFF    = 1,
    KV_ON     = 2,
};


typedef struct
{
    char      *devpath;
    char      *on_off;
    char      *on_on;

    Knob       k;
    int        prev_val;

    int        fd;
    XtInputId  rd_id;

    lse_context_t  off_ctx;
    lse_context_t  on_ctx;
} KEY_OFFON_privrec_t;

static psp_paramdescr_t text2KEY_OFFON_opts[] =
{
    PSP_P_MSTRING("dev",    KEY_OFFON_privrec_t, devpath, NULL, 1000),
    PSP_P_MSTRING("on_off", KEY_OFFON_privrec_t, on_off,  "",   10000),
    PSP_P_MSTRING("on_on",  KEY_OFFON_privrec_t, on_on,   "",   10000),
    PSP_P_END()
};

static void SetKeyVal(KEY_OFFON_privrec_t *me, int val, int reflect_in_hw)
{
    if (val == me->prev_val) return;
    SetKnobValue(me->k, me->prev_val = val);

    if (reflect_in_hw)
    {
        if (val == KV_ON)
        {
            lse_run (&(me->on_ctx), 0);
            lse_stop(&(me->on_ctx));
        }
        else
        {
            lse_run (&(me->off_ctx), 0);
            lse_stop(&(me->off_ctx));
        }
    }
}

static int OpenRS232(const char *devpath)
{
  int             fd;
  int             r;
  struct termios  newtio;

    /* Open a descriptor to COM port */
    fd = open(devpath, O_RDONLY | O_NOCTTY | O_NONBLOCK | O_EXCL);
    if (fd < 0)
        return fd;

    /* Perform serial setup wizardry */
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag     = B9600 | CRTSCTS*1 | CS8 | CLOCAL | CREAD;
    newtio.c_iflag     = IGNPAR;
    newtio.c_oflag     = 0;
    newtio.c_lflag     = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN]  = 1;
    tcflush(fd, TCIFLUSH);
    errno = 0;
    r = tcsetattr(fd,TCSANOW,&newtio);
    if (r < 0)
        fprintf(stderr, "%s %s::%s: WARNING: tcsetattr()=%d/%s",
                strcurtime(), __FILE__, __FUNCTION__, r, strerror(errno));

    return fd;
}

static void ScheduleReOpen(KEY_OFFON_privrec_t *me);

static void HandleRD(XtPointer     closure,
                     int          *source  __attribute__((unused)),
                     XtInputId    *id      __attribute__((unused)))
{
  KEY_OFFON_privrec_t *me = (KEY_OFFON_privrec_t *) closure;
  int                  r;
  unsigned char        c;

    r = read(me->fd, &c, 1);
    if (r <= 0)
    {
        SetKeyVal(me, KV_ABSENT, 1);
        ScheduleReOpen(me);
    }
    else if (c == 0xaa)
    {
        SetKeyVal(me, KV_OFF, 1);
    }
    else if (c == 0xbb)
    {
        SetKeyVal(me, KV_ON,  1);
    }
}

static void OpenKey(KEY_OFFON_privrec_t *me)
{
    me->fd = OpenRS232(me->devpath);
    ////fprintf(stderr, "fd=%d\n", me->fd);
    if (me->fd < 0)
    {
        ScheduleReOpen(me);
        return;
    }

    me->rd_id = XtAppAddInput(xh_context, me->fd,
                              (XtPointer)XtInputReadMask,
                              HandleRD, (XtPointer)me);
}

static void TimeoutProc(XtPointer     closure,
                        XtIntervalId *id      __attribute__((unused)))
{
  KEY_OFFON_privrec_t *me = (KEY_OFFON_privrec_t *) closure;

    OpenKey(me);
}
static void ScheduleReOpen(KEY_OFFON_privrec_t *me)
{
    if (me->fd >= 0)
    {
        close(me->fd);
        XtRemoveInput(me->rd_id);
    }

    XtAppAddTimeOut(xh_context, 1000, TimeoutProc, (XtPointer)me);
}

static XhWidget KEY_OFFON_Create_m(knobinfo_t *ki, XhWidget parent)
{
  XhWidget             ret = NULL;
  KEY_OFFON_privrec_t *me;

    if ((me = ki->widget_private = XtMalloc(sizeof(*me))) == NULL)
        goto FINISH;

    bzero(me, sizeof(*me));
    if (psp_parse(ki->widdepinfo, NULL,
                  me,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2KEY_OFFON_opts) != PSP_R_OK)
    {
        fprintf(stderr, "%s: [%s/\"%s\"].widdepinfo: %s\n",
                __FUNCTION__, ki->ident, ki->label, psp_error());
        goto FINISH;
    }

    OpenKey(me);
    Cdr_script_init(&(me->off_ctx),
                    "on_off", me->on_off,
                    ChlGetGrouplist(XhWindowOf(parent)));
    Cdr_script_init(&(me->on_ctx),
                    "on_on",  me->on_on,
                    ChlGetGrouplist(XhWindowOf(parent)));

    me->k = CreateSimpleKnob(" ro look=selector"
                             " widdepinfo='#Tîåôõ\f\flit=violet\v÷ùëì\f\flit=red\v÷ëì\f\flit=green'",
                             0, parent,
                             NULL, NULL);
    ret = GetKnobWidget(me->k);

    me->prev_val = KV_NONE;
    SetKeyVal(me, KV_ABSENT, 0);

 FINISH:
    if (ret == NULL)
        return ABSTRZE(XtVaCreateManagedWidget("", widgetClass, CNCRTZE(parent),
                                               XmNwidth,      20,
                                               XmNheight,     10,
                                               XmNbackground, XhGetColor(XH_COLOR_JUST_RED),
                                               NULL));

    return ret;    
}

static void NoColorize_m(knobinfo_t *ki  __attribute__((unused)), colalarm_t newstate  __attribute__((unused)))
{
    /* This method is intentionally empty,
       to prevent any attempts to change colors */
}

static knobs_vmt_t KNOB_KEY_OFFON_VMT  = {KEY_OFFON_Create_m,  NULL, NULL, NoColorize_m, NULL};
static knobs_widgetinfo_t key_offon_widgetinfo[] =
{
    {0, "key_offon", &KNOB_KEY_OFFON_VMT},
    {0, NULL, NULL}
};
static knobs_widgetset_t  key_offon_widgetset = {key_offon_widgetinfo, NULL};


void  RegisterKEY_OFFON_widgetset(int look_id)
{
    key_offon_widgetinfo[0].look_id = look_id;
    KnobsRegisterWidgetset(&key_offon_widgetset);
}
