#include "common_elem_macros.h"


#define MAILBOX(id, n) \
    {id, id, NULL, "%8.4f", NULL, NULL, LOGT_WRITE1, LOGD_TEXT, LOGK_DIRECT, LOGC_NORMAL, PHYS_ID(n)}

enum
{
IC_STATUS,
LINAC_STATUS,
RING_STATUS,

LINAC_F_RF,
RING_F_RF,
LINAC_F_SUB,
D16_F_CLOCK,

F_TACT_LINRF,
F_TACT_LINBEAM,
TACTMODE_LINBEAM,

DCCT_U2I,
DCCT_ADC_ZERO,
DCCT_BCURRENT,
DCCT_STOREGE_RATE,
DCCT_LIFETIME,
DCCT_MEADDLE_RUN,

INJEXT_FIREMODE,
INJEXT_OPERATION,
INJEXT_REQEST_CURRENT

};


static groupunit_t ic_grouping[] =
{

    GLOBTABBER_START("x", 6)
        SUBELEM_START("status", "status", 3, 1)
            MAILBOX("complex",  IC_STATUS),
            MAILBOX("linac",  LINAC_STATUS),
            MAILBOX("ring",  RING_STATUS),
        SUBELEM_END("nocoltitles", NULL),

        SUBELEM_START("LLRF", "LLRF", 4, 1)
            MAILBOX("f_lin",  LINAC_F_RF),
            MAILBOX("f_ring",  RING_F_RF),
            MAILBOX("f_sub",  LINAC_F_SUB),
            MAILBOX("f_clockd16",  D16_F_CLOCK),
        SUBELEM_END("nocoltitles", NULL),

        SUBELEM_START("syn", "syn", 3, 1)
            MAILBOX("ftact_linrf",  F_TACT_LINRF),
            MAILBOX("ftact_linbeam",  F_TACT_LINBEAM),
            MAILBOX("tactmode_linbeam",  TACTMODE_LINBEAM),
        SUBELEM_END("nocoltitles", NULL),

        SUBELEM_START("linac", "linac", 0, 1)

        SUBELEM_END("nocoltitles", NULL),

        TABBER_START("ring", "ring", 3)
            SUBELEM_START("dcct", "DCCT", 6, 1)
                MAILBOX("u2i", DCCT_U2I ),
                MAILBOX("ADCzero",  DCCT_ADC_ZERO),
                MAILBOX("beam_current", DCCT_BCURRENT),
                MAILBOX("storage_rate", DCCT_STOREGE_RATE),
                MAILBOX("lifetime", DCCT_LIFETIME),
                MAILBOX("middle_run", DCCT_MEADDLE_RUN),
            SUBELEM_END("nocoltitles", NULL),

            SUBELEM_START("magnets", "magnets", 0, 1)
            SUBELEM_END("nocoltitles", NULL),

            SUBELEM_START("RF", "RF", 0, 1)
            SUBELEM_END("nocoltitles", NULL),
        SUBELEM_END("left", NULL),


        TABBER_START("injext", "inj/ext", 3)
            SUBELEM_START("gen", "general", 3, 1)
                MAILBOX("firemode", INJEXT_FIREMODE ),
                MAILBOX("operation", INJEXT_OPERATION  ),
                MAILBOX("reqestcurrent", INJEXT_REQEST_CURRENT ),
            SUBELEM_END("nocoltitles", NULL),

            SUBELEM_START("injsyn", "injection syn", 0, 1)
            SUBELEM_END("nocoltitles", NULL),

            SUBELEM_START("extsyn", "RF", 0, 1)
            SUBELEM_END("nocoltitles", NULL),

        SUBELEM_END("left", NULL),

    GLOBELEM_END("", 0),


    {NULL}
};

static physprops_t ic_physinfo[] =
{
    //{0,  1000000.0, 0},
    //{98,       1.0, 0, -1},
    //{99, 1000000.0, 0},
    {DCCT_U2I, 1000000.0, 0},
    {DCCT_ADC_ZERO, 1000000.0, 0},
    {DCCT_BCURRENT, 1000000.0, 0},
    {DCCT_STOREGE_RATE, 1000000.0, 0},
    {DCCT_LIFETIME, 1000000.0, 0},



};
