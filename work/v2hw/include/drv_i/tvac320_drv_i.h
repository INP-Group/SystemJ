#ifndef __TVAC320_DRV_I_H
#define __TVAC320_DRV_I_H


/* Status (ST) bits' numbers */
enum
{
    TVAC320_ST_CHARGER_ON  = 0,
    TVAC320_ST_TVR_ON      = 1,
    TVAC320_ST_TVR_PH_STAB = 2,
    TVAC320_ST_TVR_U_STAB  = 3,
    TVAC320_ST_DISCHARGE   = 4,
    TVAC320_ST_RES5        = 5,
    TVAC320_ST_RES6        = 6,
    TVAC320_ST_CHARGER_FLT = 7,
};

/* Interlock (IL) bits' numbers */
enum
{
    TVAC320_IL_U_WORK_EXCESS  = 0,
    TVAC320_IL_I_CHRG_EXCESS  = 1,
    TVAC320_IL_FULL_DISCHARGE = 2,
    TVAC320_IL_RES3           = 3,
    TVAC320_IL_RES4           = 4,
    TVAC320_IL_RES5           = 5,
    TVAC320_IL_RES6           = 6,
    TVAC320_IL_CHARGER_FLT    = 7,
};

// w10,r10,w10,r10,r10,w10,r40
enum
{
    TVAC320_CHAN_SET_base  = 0,
    TVAC320_CHAN_SET_U     = TVAC320_CHAN_SET_base + 0,
    TVAC320_CHAN_SET_U_MIN = TVAC320_CHAN_SET_base + 1,
    TVAC320_CHAN_SET_U_MAX = TVAC320_CHAN_SET_base + 2,

    TVAC320_CHAN_MES_base  = 10,
    TVAC320_CHAN_MES_U     = TVAC320_CHAN_MES_base + 0,

    TVAC320_CHAN_CMD_base  = 20,
    TVAC320_CHAN_TVR_ON    = TVAC320_CHAN_CMD_base + 0,
    TVAC320_CHAN_TVR_OFF   = TVAC320_CHAN_CMD_base + 1,
    TVAC320_CHAN_RST_ILK   = TVAC320_CHAN_CMD_base + 2,
    TVAC320_CHAN_REBOOT    = TVAC320_CHAN_CMD_base + 3,

    TVAC320_CHAN_STAT_base = 30,

    TVAC320_CHAN_ILK_base  = 40,

    TVAC320_CHAN_LKM_base  = 50,
    
    TVAC320_NUMCHANS = 100
};


#endif /* __TVAC320_DRV_I_H */
