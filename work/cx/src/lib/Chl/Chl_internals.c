#define __CHL_INTERNALS_C
#include "Chl_includes.h"


psp_paramdescr_t text2_Chl_containeropts[] =
{
    PSP_P_FLAG   ("titleattop",    _Chl_containeropts_t, title_side, CHL_TITLE_SIDE_TOP,    0),
    PSP_P_FLAG   ("titleatbottom", _Chl_containeropts_t, title_side, CHL_TITLE_SIDE_BOTTOM, 0),
    PSP_P_FLAG   ("titleatleft",   _Chl_containeropts_t, title_side, CHL_TITLE_SIDE_LEFT,   0),
    PSP_P_FLAG   ("titleatright",  _Chl_containeropts_t, title_side, CHL_TITLE_SIDE_RIGHT,  0),

    PSP_P_FLAG   ("notitle",       _Chl_containeropts_t, notitle,  1,    0),
    PSP_P_FLAG   ("nohline",       _Chl_containeropts_t, nohline,  1,    0),
    PSP_P_FLAG   ("noshadow",      _Chl_containeropts_t, noshadow, 1,    0),
    PSP_P_FLAG   ("nofold",        _Chl_containeropts_t, nofold,   1,    0),
    PSP_P_FLAG   ("folded",        _Chl_containeropts_t, folded,   1,    0),
    PSP_P_FLAG   ("fold_h",        _Chl_containeropts_t, fold_h,   1,    0),
    PSP_P_FLAG   ("fold_v",        _Chl_containeropts_t, fold_h,   0,    1),
    PSP_P_FLAG   ("sepfold",       _Chl_containeropts_t, sepfold,  1,    0),
    PSP_P_END()
};
