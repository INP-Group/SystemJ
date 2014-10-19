#ifndef __BPMD_CFG_DRV_TABLE_H
#define __BPMD_CFG_DRV_TABLE_H


typedef struct
{
    const char  *strID;
    const int    numID;

    int32  tbt_def_params[BPMD_STATE_count][SUBORD_NUMCHANS];
} bpmd_info_t;

static bpmd_info_t bpmd_info_db[] =
{
    {
        "v5r_bpm_01", 1,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 86600, [BCPC_ATTEN] = 13 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 86600, [BCPC_ATTEN] = 13 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 76800, [BCPC_ATTEN] = 13 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 76800, [BCPC_ATTEN] = 13 },
        },
    },

    {
        "v5r_bpm_02", 2,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  4200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  4200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 69200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 69200, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_03", 3,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 18200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 18200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 75600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 75600, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_04", 4,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 32400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 32400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 77800, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 77800, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_05", 5,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 49800, [BCPC_ATTEN] = 11 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 49800, [BCPC_ATTEN] = 11 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 81800, [BCPC_ATTEN] = 11 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 81800, [BCPC_ATTEN] = 11 },
        },
    },

    {
        "v5r_bpm_06", 6,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 46600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 46600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 81800, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 81800, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_07", 7,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 46600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 46600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 67600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 67600, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_08", 8,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 48400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 48400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 53400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 53400, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_09", 9,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 38000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 38000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 38400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 38400, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_10", 10,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 30600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 30600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 20800, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 20800, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_11", 11,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 00000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 00000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 14200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 14200, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_12", 12,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 56200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 56200, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 19000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 19000, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_13", 13,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 71400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 71400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 22600, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 22600, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_14", 14,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  3400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  3400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 39800, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 39800, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_15", 15,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  6400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  6400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 29000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 29000, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_16", 16,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 00000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 00000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 16000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 16000, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v5r_bpm_17", 17,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 10000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 10000, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  4400, [BCPC_ATTEN] = 14 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  4400, [BCPC_ATTEN] = 14 },
        },
    },

    {
        "v2k_bpm_1", 1,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 69100, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] = 50, [BCPC_DELAY] = 69100, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 45600, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] = 50, [BCPC_DELAY] = 45600, [BCPC_ATTEN] = 0 },
        },
    },

    {
        "v2k_bpm_2", 2,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  7100, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] = 50, [BCPC_DELAY] =  7100, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 30650, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] = 50, [BCPC_DELAY] = 30650, [BCPC_ATTEN] = 0 },
	    },
    },

    {
        "v2k_bpm_3", 3,
        {
            [BPMD_STATE_ELCTRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  7750, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_ELCTINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] =  7750, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_PSTRRUN] =
                { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 65550, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_PSTRINJ] =
                { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 65550, [BCPC_ATTEN] = 0 },
	    },
    },

    {
        "v2k_bpm_4", 4,
        {
            [BPMD_STATE_ELCTRUN] =
		{ [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 24350, [BCPC_ATTEN] = 0 },
            [BPMD_STATE_ELCTINJ] =
	        { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 24350, [BCPC_ATTEN] = 0 },
	    [BPMD_STATE_PSTRRUN] =
	        { [BCPC_NUMPTS] = 1024, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 48100, [BCPC_ATTEN] = 0 },
	    [BPMD_STATE_PSTRINJ] =
	        { [BCPC_NUMPTS] = 8189, [BCPC_PTSOFS] = 0, [BCPC_DECMTN] =  0, [BCPC_DELAY] = 48100, [BCPC_ATTEN] = 0 },
	    },
    },

    { NULL }
};


#endif /* __BPMD_CFG_DRV_TABLE_H */
