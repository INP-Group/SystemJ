
#define TC(id, plc) \
    {"id", "", "╟C", "%4.1f", "", NULL, LOGT_READ, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement=plc}

static groupunit_t canvastest_grouping[] =
{
    {
        &(elemnet_t){
            "canvas", "Термо-давло-контроль на комплексе ВЭПП-5",
            "", "",
            ELEM_CANVAS, 12, 0+(2<<24),
            (logchannet_t []){
#if 1
                {"knob3", "Text3", NULL, "%5.2f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="left=!10@knob2,top=-10@rect1"},
                {"rect1", "Rect1", NULL, NULL,    "width=40, height=30, border=1, color=#bbe0e3", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP, .placement="left=40,top=30"},
                {"knob1", "Text1", NULL, "%5.2f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0)},
                {"knob2", "Text2", NULL, "%5.2f", "withlabel", NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement="left=@knob1,top=50@knob1"},
#endif
                {"hall1", "Зал 1", NULL, NULL, "width=400,height=100 border=1 textpos=2r2t fontsize=huge fontmod=bolditalic", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP, .placement="right=0"},
                {"hall2", "Зал 2", NULL, NULL, "width=200,height=150 border=1 textpos=2r2t", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP, .placement="right=10@hall1"},
                {"hall3", "Зал 3", NULL, NULL, "width=200,height=150 border=1 textpos=2r2t", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP, .placement="right=!@hall2,top=10@hall2"},

                {"therm", "Лебединое\nозеро", NULL, NULL, "width=120,height=100 border=2 textpos=2r2b textalign=right fontsize=small", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP, .placement="right=100!@hall1,top=10@hall1"},
                {"tank",  "", NULL, NULL, "width=80,height=70 border=1 color=blue", NULL, LOGT_READ, LOGD_FRECTANGLE, LOGK_NOP, .placement="left=!5@therm,top=!5@therm"},

                TC("", "left=!20@hall1,bottom=!2@hall1"),
                TC("", "left=!2@tank,top=!2@tank"),
                
                {"-", "Ку-ква-ре-ку\nчленистоногое!", NULL, NULL, "fontsize=huge fontmod=bolditalic textcolor=#ff00ff", NULL, LOGT_READ, LOGD_STRING, LOGK_NOP, .placement="left=@hall3,top=!10@hall3"},

                {"bar", "", NULL, NULL, "width=80,height=60            border=1 bordercolor=green thickness=10", NULL, LOGT_READ, LOGD_RECTANGLE, LOGK_NOP, .placement="right=5,bottom=5"},
                {"ell", "", NULL, NULL, "width=80,height=60 color=blue border=1 bordercolor=red",   NULL, LOGT_READ, LOGD_FELLIPSE,   LOGK_NOP, .placement="right=5,bottom=5"},

                {"poly", "", NULL, NULL, "color=violet rounded border=5 thickness=10 points=+10+10/.+50+50/.-40-0", NULL, LOGT_READ, LOGD_POLYGON,   LOGK_NOP, .placement="left=!5@therm,top=5@therm"},

                {"alm", "", "", "", "", NULL, LOGT_READ, LOGD_ALARM, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(0), .placement=""}
            },
            /*"notitle,noshadow"*/
        }
    },
    
    {NULL}
};

static physprops_t canvastest_physinfo[] =
{
};
