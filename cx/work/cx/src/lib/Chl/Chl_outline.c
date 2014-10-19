#include "Chl_includes.h"

#if XM_OUTLINE_AVAILABLE

#include <Xm/Outline.h>

#include "KnobsI.h"


enum
{
    FOLD_NEVER    = 0,
    FOLD_POSSIBLE = 1,
    FOLD_INITIAL  = 2,
};

typedef struct
{
    int fold;
} outlineopts_t;

static psp_paramdescr_t text2outlineopts[] =
{
    PSP_P_FLAG("fixed",    outlineopts_t, fold, FOLD_NEVER,    1),
    PSP_P_FLAG("foldable", outlineopts_t, fold, FOLD_POSSIBLE, 0),
    PSP_P_FLAG("folded",   outlineopts_t, fold, FOLD_INITIAL,  0),

    PSP_P_END(),
};


static int ElementOfSameKind(knobinfo_t *investigated, ElemInfo mine)
{
    return
        investigated->type == LOGT_SUBELEM  &&
        investigated->subelem->type == mine->type;
}

static int PopulateTree(ElemInfo e, Widget parent_node, Widget container_and_innage)
{
  outlineopts_t  opts;
  int            y;
  knobinfo_t    *ki;

  XmHierarchyNodeState  ns;

    if (e->uplink != NULL) e->emlink = e->uplink->emlink;

    bzero(&opts, sizeof(opts));
    if (psp_parse(e->options, NULL,
                  &opts,
                  Knobs_wdi_equals_c, Knobs_wdi_separators, Knobs_wdi_terminators,
                  text2outlineopts) != PSP_R_OK)
    {
        fprintf(stderr, "Element %s/\"%s\".options: %s\n",
                e->ident, e->label, psp_error());
        bzero(&opts, sizeof(opts));
    }

    if      (opts.fold == FOLD_POSSIBLE) ns = XmOpen;
    else if (opts.fold == FOLD_INITIAL)  ns = XmClosed;
    else                                 ns = XmAlwaysOpen;

    for (y = 0;  y < e->count;  y++)
    {
        ki = e->content + y;
        if (ElementOfSameKind(ki, e))
        {
            PopulateTree(ki->subelem, e->caption, container_and_innage);
        }
        else
        {
            ChlGiveBirthToKnob(ki);
            XtVaSetValues(ki->indicator,
                          XmNparentNode, e->caption,
                          XmNnodeState,  XmAlwaysOpen,
                          NULL);
        }
    }

    return 0;
}

int ELEM_OUTLINE_Create_m(XhWidget parent, ElemInfo e)
{
  Widget  hrc;

    hrc = XtVaCreateManagedWidget("tree", xmOutlineWidgetClass, parent,
                                  XmNconnectNodes, True,
                                  XmNindentSpace,  16/*!!!*/,
                                  NULL);

    return PopulateTree(e, NULL, hrc);
}


#endif /* XM_OUTLINE_AVAILABLE */
