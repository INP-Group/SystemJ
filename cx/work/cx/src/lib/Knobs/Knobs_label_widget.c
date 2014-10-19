#include "Knobs_includes.h"

#include "Knobs_label_widget.h"


typedef struct
{
    char *icon;
} labelopts_t;

static psp_paramdescr_t text2labelopts[] =
{
    PSP_P_MSTRING("icon",   labelopts_t, icon,  NULL,              100),
    PSP_P_END()
};


static XhWidget DoCreate(knobinfo_t *ki, XhWidget parent, Boolean vertical)
{
  Widget         w;
  XmString       s;
  const char    *text;
  unsigned char  halign = XmALIGNMENT_CENTER;
  int            valign = 0;
  
  labelopts_t    opts;

    /* Parse parameters from widdepinfo */
    ParseWiddepinfo(ki, &opts, sizeof(opts), text2labelopts);
    
    if (!get_knob_label(ki, &text)) text = "";//text = "???";
    
    w = XtVaCreateManagedWidget("rowlabel",
                                xmLabelWidgetClass,
                                CNCRTZE(parent),
                                XmNlabelString, s = XmStringCreateLtoR(text, xh_charset),
                                XmNalignment,   halign,
                                NULL);
    XmStringFree(s);
    MaybeAssignPixmap(w, opts.icon, 1);
    
    if (vertical)
        XhAssignVertLabel(ABSTRZE(w), NULL, valign);

    /* The HookPropsWindow() should NOT be called for decorative widgets */
  
    return ABSTRZE(w);
}

static XhWidget HCreate_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, False);
}

static XhWidget VCreate_m(knobinfo_t *ki, XhWidget parent)
{
    return DoCreate(ki, parent, True);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
    return; /* 28.04.2011: refrain from colorization, because of conns_u. */

    CommonColorize_m(ki, newstate);

    if (ki->vmtlink == &KNOBS_VLABEL_VMT) /*!!! Oh, my god, what a hack! */
        XhAssignVertLabel(ki->indicator, NULL, 0);
}

knobs_vmt_t KNOBS_HLABEL_VMT = {HCreate_m, NULL, NULL, Colorize_m, NULL};
knobs_vmt_t KNOBS_VLABEL_VMT = {VCreate_m, NULL, NULL, Colorize_m, NULL};
