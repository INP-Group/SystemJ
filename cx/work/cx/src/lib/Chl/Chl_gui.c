#include "Chl_includes.h"


/*####  ############################################################*/

static void UpdateChannels(XhWindow window, int sid_n)
{
  datainfo_t         *di = DataInfoOf(window);
  cda_localreginfo_t  localreginfo;

    if (di->is_freezed) return;
  
    localreginfo.count       = countof(di->localregs);
    localreginfo.regs        = di->localregs;
    localreginfo.regsinited  = di->localregsinited;
    
    CdrProcessGrouplist(sid_n, 
                        (sid_n < CDA_R_MIN_SID_N? CDR_OPT_SYNTHETIC : 0)
                        |
                        (di->is_readonly?         CDR_OPT_READONLY  : 0),
                        NULL, &localreginfo, di->grouplist);
    UpdatePropsWindow  (window);
    UpdateBigvalsWindow(window);
    UpdatePlotWindow   (window);
    
    if (sid_n == CDA_R_MAINDATA) /*!!! What this is for? Or, maybe, >=CDA_R_MIN_SID_N?  */
    {
        leds_update(di->leds, di->srvcount, di->mainsid);
    }

    if (di->user_cb) di->user_cb(window, di->user_ptr, sid_n != CDA_R_MAINDATA);
}


/*#### Elements' methods ###########################################*/

void Chl_E_SetPhysValue_m(Knob ki, double v)
{
  int                 r;
  XhWidget            w;
  eleminfo_t         *elem;
  XhWindow            window;
  datainfo_t         *di;
  cda_localreginfo_t  localreginfo;

    if (!ki->is_rw) return; /* Or, maybe, also log such a strange attempt? */

    /* Handle invisible (widgetless) knobs */
    w = ki->indicator;
    for (elem = ki->uplink;  elem != NULL  &&  w == NULL;  elem = elem->uplink)
        w = elem->container;
    if (w == NULL) return;

    window = XhWindowOf(w);
    di = DataInfoOf(window);
    
    localreginfo.count       = countof(di->localregs);
    localreginfo.regs        = di->localregs;
    localreginfo.regsinited  = di->localregsinited;
    
    r = CdrSetKnobValue(ki, v, (di->is_readonly? CDR_OPT_READONLY : 0),
                        &localreginfo);
    if (r > 0)
        UpdateChannels(window, -1);
}

static void DisplayAlarm(eleminfo_t *ei, int onoff)
{
  XhPixel     col = onoff? XhGetColor(XH_COLOR_JUST_RED) : ei->wdci.defbg;
  WidgetList  children;
  Cardinal    numChildren;
  Cardinal    i;

    if (!XtIsComposite(CNCRTZE(ei->container))) return;

    XtVaSetValues(CNCRTZE(ei->container), XmNbackground, col, NULL);
    
    XtVaGetValues(CNCRTZE(ei->container),
                  XmNchildren,    &children,
                  XmNnumChildren, &numChildren,
                  NULL);
    
    for (i = 0;  i < numChildren;  i++)
        XtVaSetValues(CNCRTZE(children[i]), XmNbackground, col, NULL);
}

static void CycleAlarm(eleminfo_t *ei)
{
    if (!ei->alarm_on  ||  ei->alarm_acked) return;

    XBell(XtDisplay(CNCRTZE(ei->container)), 100);
    ei->alarm_flipflop = ! ei->alarm_flipflop;
    if (ei->alarm_flipflop)
        DisplayAlarm(ei, 1);
    else
        DisplayAlarm(ei, 0);
}

void Chl_E_ShowAlarm_m(ElemInfo ei, int onoff)
{
    if (ei->uplink                    != NULL  &&
        ei->uplink->emlink            != NULL  &&
        ei->uplink->emlink->ShowAlarm != NULL)
    {
        ei->uplink->emlink->ShowAlarm(ei->uplink, onoff);
        return;
    }

    if (onoff)
    {
        if (ei->alarm_on++ == 0) ei->alarm_flipflop = 0;
        ei->alarm_acked = 0;
    }
    else
    {
        if (--ei->alarm_on == 0) DisplayAlarm(ei, 0);
    }
    
}

void Chl_E_AckAlarm_m(ElemInfo ei)
{
    if (ei->uplink                   != NULL  &&
        ei->uplink->emlink           != NULL  &&
        ei->uplink->emlink->AckAlarm != NULL)
    {
        ei->uplink->emlink->AckAlarm(ei->uplink);
        return;
    }

    if (ei->alarm_on == 0  ||  ei->alarm_acked) return;
    ei->alarm_acked = 1;
    DisplayAlarm(ei, 0);
}

void Chl_E_NewData_m(ElemInfo ei, int synthetic)
{
    if (synthetic) return;

    CycleAlarm(ei);
}


//////////////////////////////////////////////////////////////////////

static void TitleClickHandler(Widget     w,
                              XtPointer  closure,
                              XEvent    *event,
                              Boolean   *continue_to_dispatch)
{
  eleminfo_t   *e   = (eleminfo_t *)  closure;

    if (CheckForDoubleClick(w, event, continue_to_dispatch))
        XhSwitchContentFoldedness(e->container);
}

int  ChlCreateContainer(XhWidget parent, eleminfo_t *e,
                        int flags,
                        chl_cpopulator_t populator, void *privptr)
{
  Dimension     fst;
  XmString      s;
  char          buf[1000];
  const char   *label_s;
  Widget        top, bot, lft, rgt;
  int           title_side = (flags & CHL_CF_TITLE_SIDE_MASK) >> CHL_CF_TITLE_SIDE_shift;
  int           title_vert = (title_side == CHL_TITLE_SIDE_LEFT  ||
                              title_side == CHL_TITLE_SIDE_RIGHT);
  Widget       *side_p;

    /* Sanitize parameters first */
    if (flags & (CHL_CF_NOTITLE | CHL_CF_NOFOLD)) flags &=~ CHL_CF_FOLDED;
  
    e->container = ABSTRZE(XtVaCreateManagedWidget("elementForm",
                                                   xmFormWidgetClass,
                                                   CNCRTZE(parent),
                                                   NULL));

    if (flags & CHL_CF_NOSHADOW) XtVaSetValues(CNCRTZE(e->container),
                                               XmNshadowThickness, 0,
                                               NULL);
    
    XtVaGetValues(CNCRTZE(e->container),
                  XmNbackground,      &(e->wdci.defbg),
                  XmNshadowThickness, &fst,
                  NULL);

    e->ttl_cont = e->caption = e->toolbar = e->attitle =
        e->folded = e->hline = NULL;
    label_s = e->label;
    if (label_s    == NULL) label_s = "(UNLABELED)";
    if (label_s[0] == '\0') label_s = " ";
    top = bot = lft = rgt = NULL;
    if ((flags & CHL_CF_NOTITLE) == 0)
    {
        if      (title_side == CHL_TITLE_SIDE_TOP)    side_p = &top;
        else if (title_side == CHL_TITLE_SIDE_BOTTOM) side_p = &bot;
        else if (title_side == CHL_TITLE_SIDE_LEFT)   side_p = &lft;
        else                                          side_p = &rgt;

        e->ttl_cont = ABSTRZE(XtVaCreateManagedWidget("titleForm",
                                                      xmFormWidgetClass,
                                                      CNCRTZE(e->container),
                                                      XmNshadowThickness, 0,
                                                      NULL));
        if (title_vert)
        {
            XtVaSetValues(e->ttl_cont,
                          XmNtopAttachment,        XmATTACH_FORM,
                          XmNtopOffset,            fst,
                          XmNbottomAttachment,     XmATTACH_FORM,
                          XmNbottomOffset,         fst,
                          NULL);
            if (title_side == CHL_TITLE_SIDE_LEFT)
                XtVaSetValues(e->ttl_cont,
                              XmNleftAttachment,   XmATTACH_FORM,
                              XmNleftOffset,       fst,
                              NULL);
            else
                XtVaSetValues(e->ttl_cont,
                              XmNrightAttachment,  XmATTACH_FORM,
                              XmNrightOffset,      fst,
                              NULL);
        }
        else
        {
            XtVaSetValues(e->ttl_cont,
                          XmNleftAttachment,       XmATTACH_FORM,
                          XmNleftOffset,           fst,
                          XmNrightAttachment,      XmATTACH_FORM,
                          XmNrightOffset,          fst,
                          NULL);
            if (title_side == CHL_TITLE_SIDE_TOP)
                XtVaSetValues(e->ttl_cont,
                              XmNtopAttachment,    XmATTACH_FORM,
                              XmNtopOffset,        fst,
                              NULL);
            else
                XtVaSetValues(e->ttl_cont,
                              XmNbottomAttachment, XmATTACH_FORM,
                              XmNbottomOffset,     fst,
                              NULL);
        }
        *side_p = CNCRTZE(e->ttl_cont);

        if (flags & CHL_CF_TOOLBAR)
        {
            e->toolbar = ABSTRZE(XtVaCreateManagedWidget("elemToolbar",
                                                         xmFormWidgetClass,
                                                         CNCRTZE(e->ttl_cont),
                                                         XmNshadowThickness, 0,
                                                         XmNtraversalOn,     False,
                                                         NULL));
            if (title_vert) attachtop   (e->toolbar, NULL, 0);
            else            attachleft  (e->toolbar, NULL, 0);
        }

        if (flags & CHL_CF_ATTITLE)
        {
            e->attitle = ABSTRZE(XtVaCreateManagedWidget("elemAttitle",
                                                         xmFormWidgetClass,
                                                         CNCRTZE(e->ttl_cont),
                                                         XmNshadowThickness, 0,
                                                         NULL));
            if (title_vert) attachbottom(e->attitle, NULL, 0);
            else            attachright (e->attitle, NULL, 0);
        }

        e->caption =
            ABSTRZE(XtVaCreateManagedWidget("elemCaption",
                                            xmLabelWidgetClass,
                                            CNCRTZE(e->ttl_cont),
                                            XmNlabelString,     s = XmStringCreateLtoR(label_s, xh_charset),
                                            XmNalignment,       XmALIGNMENT_CENTER,
                                            NULL));
        XmStringFree(s);
        if (title_vert) XhAssignVertLabel(e->caption, label_s, 0);
        if (title_vert)
        {
            attachtop   (e->caption, e->toolbar, 0);
            attachbottom(e->caption, e->attitle, 0);
        }
        else
        {
            attachleft  (e->caption, e->toolbar, 0);
            attachright (e->caption, e->attitle, 0);
        }

        if ((flags & CHL_CF_NOHLINE) == 0)
        {
            e->hline =
                ABSTRZE(XtVaCreateManagedWidget("hline",
                                                xmSeparatorWidgetClass,
                                                CNCRTZE(e->container),
                                                XmNorientation,     title_vert? XmVERTICAL
                                                                              : XmHORIZONTAL,
                                                NULL));
            *side_p = CNCRTZE(e->hline);

            if (title_vert)
            {
                XtVaSetValues(e->hline,
                              XmNtopAttachment,        XmATTACH_FORM,
                              XmNtopOffset,            fst,
                              XmNbottomAttachment,     XmATTACH_FORM,
                              XmNbottomOffset,         fst,
                              NULL);
                if (title_side == CHL_TITLE_SIDE_LEFT)
                    XtVaSetValues(e->hline,
                                  XmNleftAttachment,   XmATTACH_WIDGET,
                                  XmNleftWidget,       e->ttl_cont,
                                  NULL);
                else
                    XtVaSetValues(e->hline,
                                  XmNrightAttachment,  XmATTACH_WIDGET,
                                  XmNrightWidget,      e->ttl_cont,
                                  NULL);
            }
            else
            {
                XtVaSetValues(e->hline,
                              XmNleftAttachment,       XmATTACH_FORM,
                              XmNleftOffset,           fst,
                              XmNrightAttachment,      XmATTACH_FORM,
                              XmNrightOffset,          fst,
                              NULL);
                if (title_side == CHL_TITLE_SIDE_TOP)
                    XtVaSetValues(e->hline,
                                  XmNtopAttachment,    XmATTACH_WIDGET,
                                  XmNtopWidget,        e->ttl_cont,
                                  NULL);
                else
                    XtVaSetValues(e->hline,
                                  XmNbottomAttachment, XmATTACH_WIDGET,
                                  XmNbottomWidget,     e->ttl_cont,
                                  NULL);
            }

        }

        if ((flags & CHL_CF_NOFOLD) == 0)
        {
            if (flags & CHL_CF_SEPFOLD)
            {
                if ((flags & CHL_CF_FOLD_H) == 0)
                    e->folded =
                        ABSTRZE(XtVaCreateWidget("caption2",
                                                 xmSeparatorWidgetClass,
                                                 CNCRTZE(e->container),
                                                 XmNorientation,      XmVERTICAL,
                                                 XmNheight,           60,
                                                 XmNshadowThickness,  4,
                                                 XmNseparatorType,    XmSHADOW_ETCHED_IN_DASH,
                                                 NULL));
                else
                    e->folded =
                        ABSTRZE(XtVaCreateWidget("caption2",
                                                 xmSeparatorWidgetClass,
                                                 CNCRTZE(e->container),
                                                 XmNorientation,      XmHORIZONTAL,
                                                 XmNwidth,            60,
                                                 XmNshadowThickness,  4,
                                                 XmNseparatorType,    XmSHADOW_ETCHED_IN_DASH,
                                                 NULL));
            }
            else
            {
                snprintf(buf, sizeof(buf), "{%s}", label_s);
                e->folded =
                    ABSTRZE(XtVaCreateWidget("caption2",
                                             xmLabelWidgetClass,
                                             CNCRTZE(e->container),
                                             XmNlabelString,      s = XmStringCreateLtoR(buf, xh_charset),
                                             NULL));
                XmStringFree(s);

                if ((flags & CHL_CF_FOLD_H) == 0)
                    XhAssignVertLabel(e->folded, buf, 0);
            }
            XhSetBalloon(e->folded, "Double-click to expand");
            
            XtVaSetValues(CNCRTZE(e->folded),
                          XmNleftAttachment,   XmATTACH_FORM,
                          XmNleftOffset,       fst,
                          XmNrightAttachment,  XmATTACH_FORM,
                          XmNrightOffset,      fst,
                          XmNtopAttachment,    XmATTACH_FORM,
                          XmNtopOffset,        fst,
                          XmNbottomAttachment, XmATTACH_FORM,
                          XmNbottomOffset,     fst,
                          NULL);

            XtAddEventHandler(CNCRTZE(e->caption), ButtonPressMask, False,
                              TitleClickHandler, (XtPointer)e);
            XtAddEventHandler(CNCRTZE(e->folded), ButtonPressMask, False,
                              TitleClickHandler, (XtPointer)e);
        }
    }

    populator(e, privptr);
    if (e->innage != NULL) /*!!! In fact, we MUST detect the failure, destroy everything and return -1 */
    {
        attachleft  (e->innage, lft, lft == NULL? fst : 0);
        attachright (e->innage, rgt, rgt == NULL? fst : 0);
        attachtop   (e->innage, top, top == NULL? fst : 0);
        attachbottom(e->innage, bot, bot == NULL? fst : 0);
    }

    if (flags & CHL_CF_FOLDED) XhSwitchContentFoldedness(e->container);
    
    return 0;
}

typedef struct
{
    _Chl_containeropts_t  elem;
    char                 *title;
} simplecontaineropts_t;

static psp_paramdescr_t text2simplecontaineropts[] =
{
    PSP_P_INCLUDE("elem",  text2_Chl_multicolopts,
                  PSP_OFFSET_OF(simplecontaineropts_t, elem)),
    PSP_P_MSTRING("title", simplecontaineropts_t, title, NULL, 0),
    PSP_P_END()
};

ElemInfo  ChlCreateSimpleContainer(XhWidget parent,
                                   const char *spec,
                                   chl_cpopulator_t populator, void *privptr)
{
  eleminfo_t            *e;
  simplecontaineropts_t  opts;
  
    e = malloc(sizeof(*e));
    if (e == NULL) return e;
    bzero(e, sizeof(*e));

    bzero(&opts, sizeof(opts)); // Is required, since none of the flags have defaults specified
    if (psp_parse(spec, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2simplecontaineropts) != PSP_R_OK)
    {
        fprintf(stderr, "%s(spec=\"%s\"): %s\n",
                __FUNCTION__, spec, psp_error());
        psp_free(&opts, text2simplecontaineropts);
        bzero(&opts, sizeof(opts));
        opts.title = strdup("ERROR-IN-spec");
    }

    if (opts.title != NULL) e->label = strdup(opts.title);

    if (ChlCreateContainer(parent, e,
                           (opts.elem.noshadow? CHL_CF_NOSHADOW : 0) |
                           (opts.elem.notitle?  CHL_CF_NOTITLE  : 0) |
                           (opts.elem.nohline?  CHL_CF_NOHLINE  : 0) |
                           (opts.elem.nofold?   CHL_CF_NOFOLD   : 0) |
                           (opts.elem.folded?   CHL_CF_FOLDED   : 0) |
                           (opts.elem.fold_h?   CHL_CF_FOLD_H   : 0) |
                           (opts.elem.sepfold?  CHL_CF_SEPFOLD  : 0) |
                           (opts.elem.title_side << CHL_CF_TITLE_SIDE_shift),
                           populator, privptr) != 0)
    {
        CdrDestroyEleminfo(e);
        free(e);
        e = NULL;
    }
    
    psp_free(&opts, text2simplecontaineropts);
    return e;
}

void ChlPopulateContainerAttitle(ElemInfo e, int nattl, int title_side)
{
  int         n;
  XhWidget    prev;
  knobinfo_t *ki;

    for (n = 0,      prev = NULL,           ki = e->content;
         n < nattl;
         n++,        prev = ki->indicator,  ki++)
    {
        if (ChlGiveBirthToKnobAt(ki, e->attitle) != 0) /*!!!*/;
        if (title_side == CHL_TITLE_SIDE_TOP  ||
            title_side == CHL_TITLE_SIDE_BOTTOM)
            attachleft(ki->indicator, prev, prev == NULL? 0 : CHL_INTERKNOB_H_SPACING);
        else
            attachtop (ki->indicator, prev, prev == NULL? 0 : CHL_INTERKNOB_V_SPACING);
    }
}

int ChlGiveBirthToKnob  (knobinfo_t *k)
{
    return ChlGiveBirthToKnobAt(k, k->uplink->innage);
}

int ChlGiveBirthToKnobAt(knobinfo_t *k, XhWidget parent)
{
  int         r;
    
    if (k->type == LOGT_SUBELEM)
    {
        r = ChlMakeElement(parent, k->subelem);
        if (r == 0) k->indicator = k->subelem->container;
        return r;
    }
    else
    {
        return CreateKnob(k, parent);
    }
}


static chl_elemdescr_t *custom_elements_table = NULL;

void ChlSetCustomElementsTable(chl_elemdescr_t *table)
{
    custom_elements_table = table;
}

int  ChlMakeElement(XhWidget parent, eleminfo_t *e)
{
  chl_elemdescr_t *tblitem;
  
    /* I. Look in custom table */
    for (tblitem = custom_elements_table;
         tblitem != NULL  &&  tblitem->look_id > 0;
         tblitem++)
        if (tblitem->look_id == e->type)
            return tblitem->create        (parent, e);
  
    /* II. Select builtin-type */
    switch (e->type)
    {
        case ELEM_SINGLECOL:
        case ELEM_MULTICOL:
            return ELEM_MULTICOL_Create_m (parent, e);
        
        case ELEM_CANVAS:
            return ELEM_CANVAS_Create_m   (parent, e);

#if XM_TABBER_AVAILABLE
        case ELEM_TABBER:
            return ELEM_TABBER_Create_m   (parent, e);
#endif

        case ELEM_SUBWIN:
            return ELEM_SUBWIN_Create_m   (parent, e);

        case ELEM_INVISIBLE:
            return ELEM_INVISIBLE_Create_m(parent, e);
            
        case ELEM_SUBMENU:
            return ELEM_SUBMENU_Create_m  (parent, e);

        case ELEM_SUBWINV4:
            return ELEM_SUBWINV4_Create_m (parent, e);

        case ELEM_ROWCOL:
            return ELEM_ROWCOL_Create_m   (parent, e);

#if XM_OUTLINE_AVAILABLE  &&  0
        case ELEM_OUTLINE:
            return ELEM_OUTLINE_Create_m   (parent, e);
#endif
        default:
            fprintf(stderr, "%s::%s: no element type %d (%s/\"%s\")\n",
                    __FILE__, __FUNCTION__, e->type, e->ident, e->label);
            e->type = ELEM_SINGLECOL;
            return ELEM_MULTICOL_Create_m(parent, e);
    }
}


int  ChlLayOutGrouping(XhWidget parent_form, groupelem_t *grouplist,
                       int is_vertical, int hspc, int vspc)
{
  groupelem_t *gu;
  XhWidget     ew;
  XhWidget     prev;
  XhWidget     line, prevline;
  groupelem_t *warn_about;

    /*!!!Hack -- here is the best place*/
    RegisterCanvasWidgetset     ();
    RegisterLedsKnob            ();
    RegisterActionknobsWidgetset();
    RegisterHistplotKnob        ();
    RegisterScenarioWidgetset   ();

    if (hspc < 0) hspc = CHL_INTERELEM_H_SPACING;
    if (vspc < 0) vspc = CHL_INTERELEM_V_SPACING;

    for (prev = NULL, line = NULL, prevline = NULL, warn_about = NULL,
         gu = grouplist;  gu->ei != NULL;  gu++)
    {
        /* Linefeed? */
        if ((gu->fromnewline & GU_FLAG_FROMNEWLINE) != 0)
        {
            if (warn_about != NULL)
            {
                fprintf(stderr, "%s(): the \"%s\" element requests a newline, while \"%s\" previously had employed a %s flag. Such behaviour is unsupported and isn't guaranteed to work.\n",
                        __FUNCTION__, gu->ei->ident, warn_about->ei->ident,
                        is_vertical? "FILLHORZ" : "FILLVERT");
                warn_about = NULL;
            }
            if (line != NULL)  // Guard against previously forced-newline
            {
                prevline = line;
                line = NULL;
            }
        }

        /* Should we create a new row-container? */
        if (line == NULL)
        {
            line =
                ABSTRZE(XtVaCreateManagedWidget("container",
                                                xmFormWidgetClass,
                                                CNCRTZE(parent_form),
                                                XmNshadowThickness, 0,
                                                NULL));

            if (prevline == NULL)
                is_vertical? attachleft(line, NULL,     0)
                           : attachtop (line, NULL,     0);
            else
                is_vertical? attachleft(line, prevline, hspc)
                           : attachtop (line, prevline, vspc);
            is_vertical? attachtop (line, NULL, 0)
                       : attachleft(line, NULL, 0);
                           
            prev = NULL;
        }
        
        /* Create and attach the element */
        if (ChlMakeElement(line, gu->ei) < 0) goto ERREXIT;
        
        ew = gu->ei->container;
        if (prev == NULL)
            is_vertical? attachtop (ew, NULL, 0)
                       : attachleft(ew, NULL, 0);
        else
            is_vertical? attachtop (ew, prev, vspc)
                       : attachleft(ew, prev, hspc);
        is_vertical? attachleft(ew, NULL, 0)
                   : attachtop (ew, NULL, 0);

        /* Remember it for future reference... */
        prev = ew;
        
        /* Strech element+container, if required... */
        /*   a. Horizontally */
        if ((gu->fromnewline & GU_FLAG_FILLHORZ) != 0)
        {
            attachright(ew,   NULL, 0);
            attachright(line, NULL, 0);
        }
        /*   b. Vertically */
        if ((gu->fromnewline & GU_FLAG_FILLVERT) != 0)
        {
            attachbottom(ew,   NULL, 0);
            attachbottom(line, NULL, 0);
        }
        /* ...and perform additional "actions" if so */
        if ((gu->fromnewline & GU_FLAG_FILLHORZ) != 0)
        {
            if (is_vertical)
            {
                warn_about = gu;
            }
            else
            {
                /* Force newline afterwards */
                prevline = line;
                line = NULL;
            }
        }
        /*   b. Vertically */
        if ((gu->fromnewline & GU_FLAG_FILLVERT) != 0)
        {
            if (is_vertical)
            {
                /* Force new-column afterwards */
                prevline = line;
                line = NULL;
            }
            else
            {
                warn_about = gu;
            }
        }
    }

    return 0;
    
 ERREXIT:
    /*!!! Need some cleanup...*/
    return -1;
}

int  ChlPopulateWorkspace(XhWindow window, groupunit_t *grouping,
                          int is_vertical, int hspc, int vspc)
{
  datainfo_t  *di = DataInfoOf(window);
  groupelem_t *grouplist;

    CLEAR_ERR();
  
    grouplist = CdrCvtGroupunits2Grouplist(di->mainsid, grouping);
    if (grouplist == NULL)
    {
        check_snprintf(_Chl_lasterr_str, sizeof(_Chl_lasterr_str),
                       "CdrCvtGroupunits2Grouplist(): %s", CdrLastErr());
        return -1;
    }
    if (ChlSetGrouplist(window, grouplist) != 0)
        return -1;

    if (ChlLayOutGrouping(XhGetWindowWorkspace(window), grouplist,
                          is_vertical, hspc, vspc) != 0)
        return -1;

    ChlAddLeds(window);
    ChlStart(window);
    
    return 0;
}


/**** LEDs per-window API *******************************************/

int  ChlAddLeds(XhWindow window)
{
  datainfo_t  *di = DataInfoOf(window);
  XhWidget     leds_button = XhGetWindowAlarmleds(window);
  XhWidget     leds_grid;
  
    if (leds_button == NULL) return -1;
  
    /* Populate the "leds" widget */
    leds_grid = XhCreateGrid(leds_button, "alarmLeds");
    attachtop   (leds_grid, NULL, 0);
    attachbottom(leds_grid, NULL, 0);
    XhGridSetGrid(leds_grid, 0, 0);
    di->srvcount = cda_status_lastn(di->mainsid) + 1;
    if ((di->leds = XtCalloc(di->srvcount, sizeof(*(di->leds)))) != NULL)
        leds_create(leds_grid, 20, di->leds, di->srvcount, di->mainsid);
    
    return 0;
}

//// Internal thingies ///////////////////////////////////////////////

static void EventProc(cda_serverid_t sid __attribute__((unused)), int reason, void *privptr)
{
  XhWindow  window = (XhWindow)privptr;

    if (reason < CDA_R_MIN_SID_N) return;

    UpdateChannels(window, reason);
    XhProcessPendingXEvents();
}


static void KeepaliveProc(XtPointer     closure,
                          XtIntervalId *id      __attribute__((unused)))
{
  datainfo_t *di = (datainfo_t *)closure;
    
    XtAppAddTimeOut(xh_context, 1000, KeepaliveProc, closure);

    leds_update(di->leds, di->srvcount, di->mainsid);
}

/*#### Public data API #############################################*/

void ChlSetSysname   (XhWindow window, const char *sysname)
{
  char **snp = &(Priv2Of(window)->sysname);
  
    if (*snp != NULL) XtFree(*snp);
    *snp = XtNewString(sysname);
}

int  ChlSetServer    (XhWindow window, const char *spec)
{
  cda_serverid_t  sid;
  datainfo_t     *di = DataInfoOf(window);
    
    sid = cda_new_server(spec, EventProc, (void *)window, CDA_REGULAR);

    if (sid == CDA_SERVERID_ERROR) return -1;
    
    di->mainsid = sid;
    
    return 0;
}

void ChlSetPhysInfo  (XhWindow window, physprops_t *info, int count)
{
  datainfo_t *di = DataInfoOf(window);

    cda_set_physinfo(di->mainsid, info, count);
}

static void _ChlKnobUnhiliter(Knob k)
{
    if (k != NULL  &&  k->indicator != NULL) XhHideHilite(k->indicator);
}

int  ChlSetGrouplist (XhWindow window, groupelem_t *grouplist)
{
  datainfo_t  *di = DataInfoOf(window);
  
    if (di->grouplist != NULL) return -1;
    
    di->grouplist = grouplist;

    if (cdr_unhilite_knob_hook == NULL) cdr_unhilite_knob_hook = _ChlKnobUnhiliter;
    
    return 0;
}

int  ChlStart        (XhWindow window)
{
  datainfo_t  *di = DataInfoOf(window);
  
    KeepaliveProc((XtPointer)di, NULL);
    
    cda_run_server(di->mainsid); /*! Should it be called from here? */

    return 0;
}

void ChlFreeze       (XhWindow window, int freeze)
{
  datainfo_t *di = DataInfoOf(window);

    if (freeze < 0) freeze = !(di->is_freezed);
    di->is_freezed = (freeze != 0);
}

int  ChlSetDataArrivedCallback(XhWindow window, ChlDataCallback_t proc, void *privptr)
{
  datainfo_t  *di = DataInfoOf(window);

    di->user_cb  = proc;
    di->user_ptr = privptr;
    
    return 0;
}

cda_serverid_t ChlGetMainsid(XhWindow window)
{
  datainfo_t  *di = DataInfoOf(window);
  
    return di->mainsid;
}

int  ChlGetChanVal(XhWindow window, const char *chanspec, double *v_p)
{
  datainfo_t  *di = DataInfoOf(window);
  knobinfo_t  *ki;
  
    ki = CdrFindKnob(di->grouplist, chanspec);
    if (ki == NULL) return -1;
    
    *v_p = ki->curv;

    return 0;
}

int  ChlSetChanVal(XhWindow window, const char *chanspec, double  v)
{
  datainfo_t  *di = DataInfoOf(window);
  knobinfo_t  *ki;
  
    ki = CdrFindKnob(di->grouplist, chanspec);
    if (ki == NULL) return -1;
    
    if (ki->uplink                       != NULL  &&
        ki->uplink->emlink               != NULL  &&
        ki->uplink->emlink->SetPhysValue != NULL)
        ki->uplink->emlink->SetPhysValue(ki, v);

    return 0;
}

Knob ChlFindKnob  (XhWindow window, const char *chanspec)
{
  datainfo_t  *di = DataInfoOf(window);

    return CdrFindKnob(di->grouplist, chanspec);
}

Knob ChlFindKnobFrom(XhWindow window, const char *chanspec,
                     ElemInfo start_point)
{
  datainfo_t  *di = DataInfoOf(window);

    return CdrFindKnobFrom(di->grouplist, chanspec, start_point);
}

int  ChlSetLclReg (XhWindow window, int n, double  v)
{
  datainfo_t         *di = DataInfoOf(window);
  cda_localreginfo_t  localreginfo;

    localreginfo.count       = countof(di->localregs);
    localreginfo.regs        = di->localregs;
    localreginfo.regsinited  = di->localregsinited;

    return cda_setlclreg(&localreginfo, n, v);
}

int  ChlGetLclReg (XhWindow window, int n, double *v_p)
{
  datainfo_t         *di = DataInfoOf(window);
  cda_localreginfo_t  localreginfo;

    localreginfo.count       = countof(di->localregs);
    localreginfo.regs        = di->localregs;
    localreginfo.regsinited  = di->localregsinited;

    return cda_getlclreg(&localreginfo, n, v_p);
}


const char  *ChlGetSysname  (XhWindow window)
{
    return Priv2Of(window)->sysname;
}

groupelem_t *ChlGetGrouplist(XhWindow window)
{
    return DataInfoOf(window)->grouplist;
}


int          ChlIsReadonly  (XhWindow window)
{
    return DataInfoOf(window)->is_readonly;
}
