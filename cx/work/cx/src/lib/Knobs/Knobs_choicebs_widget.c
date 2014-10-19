#include "Knobs_includes.h"


typedef struct
{
    Widget *items;
    int     count;
    int     cur_active;
    int     dlyd_clrz;
    
    Pixel   b_deffg;
    Pixel   b_defbg;
    int     are_defcols_got;
} choicebs_privrec_t;


static void SetActiveItem(knobinfo_t *ki, int v)
{
  choicebs_privrec_t *me = (choicebs_privrec_t *)(ki->widget_private);

  Pixel         fg;
  Pixel         bg;

    if (v < 0  ||  v >= me->count) v = -1;
    if (me->cur_active == v) return;
    if (me->cur_active >= 0)
    {
        XhInvertButton(ABSTRZE(me->items[me->cur_active]));
        if (me->dlyd_clrz)
        {
            me->dlyd_clrz = 0;
            ChooseKnobColors(ki->color, ki->colstate,
                             me->b_deffg, me->b_defbg,
                             &fg, &bg);
            XtVaSetValues(me->items[me->cur_active],
                          XmNforeground, fg,
                          XmNbackground, bg,
                          NULL);
        }
    }
    me->cur_active = v;
    if (me->cur_active >= 0) XhInvertButton(ABSTRZE(me->items[me->cur_active]));
}

static void SetValue_m(knobinfo_t *ki, double v);

static void ActivateCB(Widget     w,
                       XtPointer  closure,
                       XtPointer  call_data __attribute__((unused)))
{
  knobinfo_t *ki = (knobinfo_t *)closure;
  choicebs_privrec_t *me = (choicebs_privrec_t *)(ki->widget_private);

  int         y;
    
    /* Mmmm...  We have to walk through the whole list to find *which* one... */
    for (y = 0;  y < me->count; y++)
    {
        if (me->items[y] == w)
        {
            if (ki->is_rw)
                SetControlValue(ki, y, 1);
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

typedef struct
{
    int  fgcidx;
    int  armidx;
    int  size;
    int  bold;
} itemstyle_t;

static psp_paramdescr_t text2itemstyle[] =
{
    PSP_P_LOOKUP("color",  itemstyle_t, fgcidx, -1, Knobs_colors_lkp),
    PSP_P_LOOKUP("lit",    itemstyle_t, armidx, -1, Knobs_colors_lkp),
    PSP_P_LOOKUP("size",   itemstyle_t, size,   -1, Knobs_sizes_lkp),
    PSP_P_FLAG  ("bold",   itemstyle_t, bold,    1, 0),
    PSP_P_FLAG  ("medium", itemstyle_t, bold,    0, 1),
    PSP_P_END()
};

static XhWidget Create_m(knobinfo_t *ki, XhWidget parent)
{
  choicebs_privrec_t *me;

  XmString  s;

  Widget    form;
  Widget    prev;
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

  Dimension    hth;
  Dimension    mrw;
  Dimension    mrh;
  
  char            fontspec[100];
  
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
    if ((ki->widget_private = XtMalloc(sizeof(choicebs_privrec_t))) == NULL)
        return NULL;
    me = (choicebs_privrec_t *)(ki->widget_private);
    bzero(me, sizeof(*me));
    me->cur_active = -1;

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
                    fprintf(stderr, "%s::%s: invalid #-command '#%c' in choicebs description\n",
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

    /* Create a form... */
    form = XtVaCreateManagedWidget("choiceForm", xmFormWidgetClass, CNCRTZE(parent), 
                                   XmNshadowThickness, 0,
                                   XmNtraversalOn, ki->is_rw,
                                   NULL);

    /* ...and populate it */
    if (list_count <= 0)
    {
        list_count = 1;
        list_type  = LIST_NONE;
    }

    prev = NULL;
    has_label = (left_buf[0] != '\0');
    if (has_label)
    {
        label_w = XtVaCreateManagedWidget("rowlabel", xmLabelWidgetClass, form,
                                          XmNlabelString, s = XmStringCreateLtoR(left_buf, xh_charset),
                                          NULL);
        XmStringFree(s);
        MaybeAssignPixmap(label_w, left_buf, 0);
        attachleft(label_w, NULL, 0);
        attachtop (label_w, NULL, 0);
        
        prev = label_w;
    }
    
    /* Fill in the item info... */
    if ((me->items = (void *)(XtCalloc(list_count, sizeof(Widget)))) == NULL)
        return NULL;
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
        
        me->items[y] = b =
            XtVaCreateManagedWidget(ki->is_rw? "push_i" : "push_o",
                                    xmPushButtonWidgetClass,
                                    form,
                                    XmNlabelString, s = XmStringCreateLtoR(l_buf_p, xh_charset),
                                    XmNalignment,   list_alignment,
                                    XmNtraversalOn, False,
                                    XmNarmColor,    XhGetColor(XH_COLOR_BG_TOOL_ARMED),
                                    NULL);
        XmStringFree(s);
        MaybeAssignPixmap(me->items[y], l_buf_p, 0);
        if (tip_p != NULL  &&  *tip_p != '\0')
            XhSetBalloon(ABSTRZE(b), tip_p);
        if (istyle.fgcidx > 0)
            XtVaSetValues(b, XmNforeground, XhGetColor(istyle.fgcidx), NULL)/*, fprintf(stderr, "fgcidx=%d\n", istyle.fgcidx)*/;
        if (istyle.armidx > 0)
            XtVaSetValues(b, XmNarmColor,   XhGetColor(istyle.armidx), NULL);
        if (istyle.size   > 0)
        {
            check_snprintf(fontspec, sizeof(fontspec),
                           "-*-" XH_FONT_PROPORTIONAL_FMLY "-%s-r-*-*-%d-*-*-*-*-*-*-*",
                           istyle.bold? XH_FONT_BOLD_WGHT : XH_FONT_MEDIUM_WGHT,
                           istyle.size);
            XtVaSetValues(b,
                          XtVaTypedArg, XmNfontList, XmRString, fontspec, strlen(fontspec)+1,
                          NULL);
        }
        if (disabled) XtSetSensitive(b, False);
        if (ki->is_rw)
        {
            XtAddCallback(b, XmNactivateCallback,
                          ActivateCB, (XtPointer)ki);
        }
        else
        {
            XtVaGetValues(b,
                          XmNshadowThickness, &shd_thk,
                          XmNmarginHeight,    &mrg_hgt,
                          NULL);
            XtVaSetValues(b,
                          XmNshadowThickness, 0,
                          XmNmarginHeight,    mrg_hgt + shd_thk,
                          NULL);
        }

        XtVaGetValues(b,
                      XmNhighlightThickness, &hth,
                      XmNmarginWidth,        &mrw,
                      XmNmarginHeight,       &mrh,
                      NULL);
        XtVaSetValues(b,
                      XmNhighlightThickness, 0,
                      XmNmarginWidth,        mrw + hth,
                      XmNmarginHeight,       mrh + hth,
                      NULL);
        
        if (!is_compact)
        {
            attachtop     (b, NULL, 0);
            attachbottom  (b, NULL, 0);
            if (prev != NULL)
                attachleft(b, prev, 0);
        }
        else
        {
            attachleft    (b, NULL, 0);
            attachright   (b, NULL, 0);
            if (prev != NULL)
                attachtop (b, prev, 0);
        }

        HookPropsWindow(ki, b);

        prev = b;
    }
    
    return ABSTRZE(form);
}

static void SetValue_m(knobinfo_t *ki, double v)
{
    SetActiveItem(ki, (int)v);
}

static void Colorize_m(knobinfo_t *ki, colalarm_t newstate)
{
  choicebs_privrec_t *me = (choicebs_privrec_t *)(ki->widget_private);

  Pixel         fg;
  Pixel         bg;
  int           n;

    if (!me->are_defcols_got)
    {
        XtVaGetValues(me->items[0],
                      XmNforeground, &(me->b_deffg),
                      XmNbackground, &(me->b_defbg),
                      NULL);
        me->are_defcols_got = 1;
    }
  
    CommonColorize_m(ki, newstate);

    ChooseKnobColors(ki->color, newstate,
                     me->b_deffg, me->b_defbg,
                     &fg, &bg);
    for (n = 0;  n < me->count;  n++)
        if (n != me->cur_active)
            XtVaSetValues(me->items[n],
                          XmNforeground, fg,
                          XmNbackground, bg,
                          NULL);
        else
            me->dlyd_clrz = 1;
}


knobs_vmt_t KNOBS_CHOICEBS_VMT = {Create_m, NULL, SetValue_m, Colorize_m, NULL};
