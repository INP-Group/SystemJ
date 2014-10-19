#include "Knobs_includes.h"

typedef struct
{
    Widget  pulldown;
    Widget *items;
    int    *bgs;
    int     count;
    
    Widget  cascade;
    Pixel   csc_deffg;
    Pixel   csc_defbg;
    int     are_defcols_got;
    int     cur_bgidx;
} selector_privrec_t;

#define PulldownW(ki)   (((selector_privrec_t *)(ki->widget_private))->pulldown)
#define ItemsList(ki)   (((selector_privrec_t *)(ki->widget_private))->items)
#define ItemsBgs(ki)    (((selector_privrec_t *)(ki->widget_private))->bgs)
#define ItemsCount(ki)  (((selector_privrec_t *)(ki->widget_private))->count)
#define csc_w(ki)       (((selector_privrec_t *)(ki->widget_private))->cascade)
#define csc_deffg_p(ki) (((selector_privrec_t *)(ki->widget_private))->csc_deffg)
#define csc_defbg_p(ki) (((selector_privrec_t *)(ki->widget_private))->csc_defbg)
#define defcols_f(ki)   (((selector_privrec_t *)(ki->widget_private))->are_defcols_got)
#define cur_bgidx_v(ki) (((selector_privrec_t *)(ki->widget_private))->cur_bgidx)

static void SetValue_m(knobinfo_t *ki, double v);
static void Colorize_m(knobinfo_t *ki, colalarm_t newstate);

static void HandleBgIdxChange(knobinfo_t *ki, int newidx)
{
  selector_privrec_t *me = (selector_privrec_t *)(ki->widget_private);

  int  oldidx;
    
    oldidx = me->cur_bgidx;
    me->cur_bgidx = newidx;

    if (newidx != oldidx) Colorize_m(ki, ki->colstate);
}

static void ActivateCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data __attribute__((unused)))
{
  knobinfo_t *ki = (knobinfo_t *)closure;
  selector_privrec_t *me = (selector_privrec_t *)(ki->widget_private);
  int         y;
    
    /* Mmmm...  We have to walk through the whole list to find *which* one... */
    for (y = 0;  y < me->count; y++)
    {
        if (me->items[y] == w)
        {
            if (ki->is_rw)
            {
                SetControlValue(ki, y, 1);
                HandleBgIdxChange(ki, me->bgs[y]);
            }
            else
            {
                /* Handle readonly selectors */
                if (y != (int)(ki->curv))
                    SetValue_m(ki, ki->curv);
            }
            return;
        }
    }

    /* Hmmm...  Nothing found?!?!?! */
    fprintf(stderr, "%s::%s: no widget found!\n", __FILE__, __FUNCTION__);
}

static void Mouse1Handler(Widget     w       __attribute__((unused)),
                          XtPointer  closure __attribute__((unused)),
                          XEvent    *event,
                          Boolean   *continue_to_dispatch)
{
  XButtonPressedEvent *ev  = (XButtonPressedEvent *) event;

    if (ev->button == Button1) *continue_to_dispatch = False;
}


typedef struct
{
    int  armidx;
} itemstyle_t;

static psp_paramdescr_t text2itemstyle[] =
{
    PSP_P_LOOKUP("lit",  itemstyle_t, armidx, -1, Knobs_colors_lkp),
    PSP_P_END()
};

static XhWidget Create_m(knobinfo_t *ki, XhWidget parent)
{
  selector_privrec_t *me;

  Arg       al[20];
  int       ac;
  XmString  s;
  
  Widget    selector;
  Widget    label_w;
  Widget    b;

  char      l_buf[1000];
  char     *l_buf_p;
  char     *tip_p;
  char     *style_p;
  int       disabled;
  char      left_buf[200];
  Bool      has_label;
  char     *ls;
  int       y;
  
  itemstyle_t  istyle;
  int  r;

  int       is_compact = 0;
  
  typedef enum {LIST_NONE, LIST_LIST, LIST_FORMAT} list_type_t;
  
  list_type_t  list_type;        /* How this list should be generated */
  char        *list_list_p;      /* Pointer to multistring in case of list-type list */
  double       list_multiplier;
  double       list_offset;
  char         list_units;       /* Either 'i' (integer) or 'f' (double float) */
  int          list_count;       /* # of items in format-type list */
  char         list_format[200]; /* Format for sprintf() in format-type list */
  int          list_alignment;   /* XmALIGNMENT_BEGINNING/XmALIGNMENT_END */

  char        *wdi_end_p;        /* Points after the end of ki->widdepinfo */
  char        *strend_p;         /* Points to the terminator of currently parsed string segment */
  char        *p;                /* Parser's pointer */
  char        *err;              /* endptr for strtoX() */
  int          len;              /* length of strings to copy to separate buffers */

  char         cmd;              /* The 'P' character after '#' */

  Dimension    shd_thk;
  Dimension    mrg_hgt;
  
  
    /*
     widdepinfo format is:
         "item1\vitem2\v...itemN"
     or
         "#Pdata1[\v#Pdata2[\v#Pdata3...]]"
     where "P" is one of
         L (label), data is the label text
         T (texts), data is the same as in "item1\vitem2\v..." (eats the rest)
         F (format), data has a format "NNc,MULTIPLIER,OFFSET,FORMAT", where
             NN           is number of items
             c            is a format character (either "d" or "f")
             MULTIPLIER \ these form the value:
             OFFSET     / value = y * MULTIPLIER + OFFSET
             FORMAT       is a format for printf(), %c must match "c"
     */

    ki->behaviour |= KNOB_B_INCDECSTEP_FXD;
  
    /* Allocate the private structure... */
    if ((ki->widget_private = XtMalloc(sizeof(selector_privrec_t))) == NULL)
        return NULL;
    me = (selector_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));

    //ki->behaviour |= KNOB_B_HALIGN_RIGHT;
    
    /* Parse the "widdepinfo" string first */
    left_buf[0] = '\0';
    if (get_knob_label(ki, &ls)) strzcpy(left_buf, ls, sizeof(left_buf));
    p = ki->widdepinfo;
    if (ki->widdepinfo == NULL)
    {
        fprintf(stderr, "%s::%s: [%s/\"%s\"].widdepinfo==NULL!\n",
                __FILE__, __FUNCTION__, ki->ident, ki->label);
        p = "";
    }
    for (wdi_end_p = p + strlen(p),
         list_type = LIST_NONE, list_alignment = XmALIGNMENT_END, list_count = 0;
         *p != '\0';)
    {
        if (*p == '#')
        {
            p++;
            cmd = toupper(*p++);
            
            strend_p = strchr(p, MULTISTRING_SEPARATOR);
            if (strend_p == NULL) strend_p = wdi_end_p;
            
            switch (cmd)
            {
                case '\0':
                    goto END_PARSE;

                case '!':
                    ki->behaviour |= KNOB_B_IGN_OTHEROP;
                    if (*p == '#') strend_p = p - 1;
                    break;

                case '+':
                    list_alignment = XmALIGNMENT_END;
                    if (*p == '#') strend_p = p - 1; // For '\v' to be optional
                    break;
                    
                case '-':
                    list_alignment = XmALIGNMENT_BEGINNING;
                    if (*p == '#') strend_p = p - 1;
                    break;
                    
                case '=':
                    list_alignment = XmALIGNMENT_CENTER;
                    if (*p == '#') strend_p = p - 1;
                    break;
                    
                case 'H':
                    is_compact = 0;
                    if (*p == '#') strend_p = p - 1;
                    break;

                case 'C':
                    is_compact = 1;
                    if (*p == '#') strend_p = p - 1;
                    break;

                case 'L':
                    len = strend_p - p;
                    if (len > sizeof(left_buf) - 1)
                        len = sizeof(left_buf) - 1;
                    
                    memcpy(left_buf, p, len);
                    left_buf[len] = '\0';
                    break;
                    
                case 'T':
                    goto NON_HASH_PARAMETER;
                    
                case 'F':
#define CHECK_FOR_ERROR(condition, format_str, args...)  \
    if (condition)                                       \
    {                                                    \
        fprintf(stderr, "%s::%s: " format_str, __FILE__, __FUNCTION__, ## args); \
        goto CONTINUE_TO_NEXT_PARAMETER;                 \
    }
                    
                    /* Get the list size */
                    list_count = strtoul(p, &err, 10);
                    CHECK_FOR_ERROR(err == p  ||  list_count == 0,
                                    "missing list size at #F\n")

                    /* Get the list format */
                    switch (tolower(*err))
                    {
                        case 'd': case 'i': case 'o': case 'u': case 'x':
                            list_units = 'i';
                            break;

                        case 'e': case 'f': case 'g': case 'a':
                            list_units = 'f';
                            break;

                        default:
                            CHECK_FOR_ERROR(1, "invalid list format char '%c' at #F\n", *err);
                    }
                    p = err + 1;

                    /* Get the MULTIPLIER */
                    CHECK_FOR_ERROR(*p++ != ',', "missing ',' after number\n");

                    list_multiplier = strtod(p, &err);
                    CHECK_FOR_ERROR(err == p, "missing MULTIPLIER\n");
                    p = err;

                    /* Get the OFFSET */
                    CHECK_FOR_ERROR(*p++ != ',', "missing ',' after MULTIPLIER\n");
                    
                    list_offset = strtod(p, &err);
                    CHECK_FOR_ERROR(err == p, "missing OFFSET\n");
                    p = err;

                    /* Okay, the rest must be format... */
                    CHECK_FOR_ERROR(*p++ != ',', "missing ',' after OFFSET\n");

                    len = strend_p - p;
                    if (len > sizeof(list_format) - 1)
                        len = sizeof(list_format) - 1;

                    memcpy(list_format, p, len);
                    list_format[len] = '\0';

                    list_type = LIST_FORMAT;
                    
                    break;

                default:
                    fprintf(stderr, "%s::%s: invalid #-command '#%c' in selector description\n",
                            __FILE__, __FUNCTION__, cmd);
            }
        }
        else
        {
 NON_HASH_PARAMETER:
            list_type   = LIST_LIST;
            list_list_p = p;
            list_count  = countstrings(list_list_p);
            
            goto END_PARSE;
        }

 CONTINUE_TO_NEXT_PARAMETER:
        p = strend_p;
        if (*p == '\0')
            goto END_PARSE;
        else
            p++;
    }
 END_PARSE:

    /* Create a pulldown menu... */
    ac = 0;
    XtSetArg(al[ac], XmNentryAlignment, list_alignment); ac++;
    me->pulldown = 
        XmCreatePulldownMenu(CNCRTZE(parent), "selectorPullDown", al, ac);

    /* ...and populate it */
    if (list_count <= 0)
    {
        list_count = 1;
        list_type  = LIST_NONE;
    }

    /* Fill in the item info... */
    me->items = (void *)(XtCalloc(list_count, sizeof(Widget)));
    me->bgs   = (void *)(XtCalloc(list_count, sizeof(int)));
    if (me->items == NULL  ||  me->bgs == NULL)
    {
        XtFree(me->bgs);
        XtFree(me->items);
        return NULL;
    }
    bzero(me->bgs, sizeof(me->bgs[0]) * list_count);
    me->count = list_count;
    
    for (y = 0;  y < list_count;  y++)
    {
        switch (list_type)
        {
            case LIST_LIST:
                extractstring(list_list_p, y, l_buf, sizeof(l_buf));
                break;

            case LIST_FORMAT:
                if (list_units == 'i')
                    snprintf(l_buf, sizeof(l_buf), list_format,
                             (int)(y * list_multiplier + list_offset));
                else
                    snprintf(l_buf, sizeof(l_buf), list_format,
                             (y * list_multiplier + list_offset));
                break;

            default:
                sprintf(l_buf, "***ITEM#%d***", y+1);
        }

        if (l_buf[0] == '\0')
            snprintf(l_buf, sizeof(l_buf), "*EMPTY#%d*", y + 1);

        l_buf_p  = l_buf;
        disabled = 0;
        if (*l_buf_p == '\n')
        {
            l_buf_p++;
            disabled = 1;
        }
        
        tip_p = NULL;
        if (l_buf_p != NULL) tip_p = strchr(l_buf_p, MULTISTRING_OPTION_SEPARATOR);
        if (tip_p != NULL)
        {
            *tip_p = '\0';
            tip_p++;

        }

        style_p = NULL;
        if (tip_p != NULL) style_p = strchr(tip_p, MULTISTRING_OPTION_SEPARATOR);
        if (style_p != NULL)
        {
            *style_p = '\0';
            style_p++;
        }
        bzero(&istyle, sizeof(istyle));
        r = psp_parse(style_p, NULL,
                      &istyle,
                      Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                      text2itemstyle);
        if (r != PSP_R_OK)
        {
            fprintf(stderr, "%s: [%s/\"%s\"].item[%d:\"%s\"].style: %s\n",
                    __FUNCTION__, ki->ident, ki->label, y, l_buf_p, psp_error());
            if (r == PSP_R_USRERR)
                psp_parse("", NULL,
                          &istyle,
                          Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                          text2itemstyle);
            else
                bzero(&istyle, sizeof(istyle));
        }
        me->bgs[y] = istyle.armidx;
        if (y == 0) me->cur_bgidx = istyle.armidx;
        
        me->items[y] = b =
            XtVaCreateManagedWidget("selectorItem", xmPushButtonWidgetClass, me->pulldown,
                                    XmNlabelString, s = XmStringCreateLtoR(l_buf_p, xh_charset),
                                    NULL);
        XmStringFree(s);
        MaybeAssignPixmap(b, l_buf_p, 0);
        if (tip_p != NULL  &&  *tip_p != '\0')
            XhSetBalloon(ABSTRZE(b), tip_p);
        if (istyle.armidx > 0)
            XtVaSetValues(b, XmNbackground, XhGetColor(istyle.armidx), NULL);
        if (disabled) XtSetSensitive(b, False);
        XtAddCallback(b, XmNactivateCallback,
                      ActivateCB, (XtPointer)ki);
    }
    
    has_label = (left_buf[0] != '\0');
    
    ac = 0;
    XtSetArg(al[ac], XmNsubMenuId, me->pulldown);    ac++;
    XtSetArg(al[ac], XmNtraversalOn, ki->is_rw); ac++;
    if (has_label)
    {
        XtSetArg(al[ac], XmNlabelString, s = XmStringCreateLtoR(left_buf, xh_charset)); ac++;
    }
    if (is_compact)
    {
        XtSetArg(al[ac], XmNorientation, XmVERTICAL);   ac++;
        XtSetArg(al[ac], XmNspacing,     0);            ac++;
    }
    else
    {
        XtSetArg(al[ac], XmNorientation, XmHORIZONTAL); ac++;
    }
    selector = XmCreateOptionMenu(CNCRTZE(parent),
                                  ki->is_rw? "selector_i" : "selector_o",
                                  al, ac);

    label_w = XtNameToWidget(selector, "OptionLabel");
    if (label_w != NULL  &&  !has_label)
        XtUnmanageChild(label_w);
    if (label_w != NULL)
        XtVaSetValues(label_w, XmNalignment, XmALIGNMENT_BEGINNING, NULL);
    
    me->cascade = XtNameToWidget(selector, "OptionButton");
    if (me->cascade != NULL)
        XtVaSetValues(me->cascade, XmNalignment, list_alignment, NULL);

    XtManageChild(selector);

    if (0){
      Dimension h;
      
        XtVaGetValues(CNCRTZE(ki->indicator), XmNheight, &h, NULL);
        fprintf(stderr, "h=%d\n", h);
    }

    /*!!!Note: we use the fact that only "selector" is a true widget
     with XWindow, while "label_w" and "cascade_w" are gadgets.  This has two
     consequences: 1) hooking to selector is enough; 2) attempt to hook to
     {label,cascade}_w leads to SIGSEGV. */
    HookPropsWindow(ki, selector);

    if (me->cascade != NULL  &&  !ki->is_rw)
    {
        XtVaGetValues(me->cascade,
                      XmNshadowThickness, &shd_thk,
                      XmNmarginHeight,    &mrg_hgt,
                      NULL);
        XtVaSetValues(me->cascade,
                      XmNshadowThickness, 0,
                      XmNmarginHeight,    mrg_hgt + shd_thk,
                      NULL);

        XtInsertEventHandler(selector, ButtonPressMask, False, Mouse1Handler, (XtPointer)ki, XtListHead);
    }
    
    return ABSTRZE(selector);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
  selector_privrec_t *me = (selector_privrec_t *)(ki->widget_private);

  int  item = (int)v;
  
    if (item < 0)
        item = 0;
    if (item >= me->count)
        item = me->count - 1;

    XtVaSetValues(CNCRTZE(ki->indicator), XmNmenuHistory, me->items[item], NULL);
    HandleBgIdxChange(ki, me->bgs[item]);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
  selector_privrec_t *me = (selector_privrec_t *)(ki->widget_private);
  Pixel         fg;
  Pixel         bg;

    if (!me->are_defcols_got)
    {
        XtVaGetValues(me->cascade,
                      XmNforeground, &(me->csc_deffg),
                      XmNbackground, &(me->csc_defbg),
                      NULL);
        me->are_defcols_got = 1;
    }
  
    CommonColorize_m(ki, newstate);

    if (me->cascade != NULL)
    {
        ChooseKnobColors(ki->color, newstate,
                         me->csc_deffg, me->csc_defbg,
                         &fg, &bg);
        if (me->cur_bgidx > 0) bg = XhGetColor(me->cur_bgidx);
        
        XtVaSetValues(me->cascade,
                      XmNforeground, fg,
                      XmNbackground, bg,
                      NULL);
    }
}

knobs_vmt_t KNOBS_SELECTOR_VMT = {Create_m, NULL, SetValue_m, Colorize_m, NULL};
