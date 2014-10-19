#ifndef __B0641_DRV_I_H
#define __B0641_DRV_I_H


enum
{
    B0641_CHAN_NUMSTEPS = 0,     // # of steps to perform
    B0641_CHAN_F0       = 1,     // Start frequency
    B0641_CHAN_FA       = 2,     // Frequency acceleration
    B0641_CHAN_TA       = 3,     // Time of acceleration

    B0641_CHAN_GOLFT    = 4,     // Go NUMSTEPS left
    B0641_CHAN_GORGT    = 5,     // Go NUMSTEPS right
    B0641_CHAN_STOP     = 6,     // Stop
    B0641_CHAN_RSTSUM   = 7,     // Reset SUM counter
    
    B0641_CHAN_REMAIN   = 8,     // # of steps remaining to go
    B0641_CHAN_CURSUM   = 9,     // Current sum of steps (signed)
    B0641_CHAN_KLFT     = 10,    // K- state
    B0641_CHAN_KRGT     = 11,    // K+ state
    B0641_CHAN_BUSY     = 12,    // Currently going
    B0641_CHAN_MANUAL   = 13,    // In manual mode

    B0641_CHAN_RSRV1    = 14,    // Reserved 1
    B0641_CHAN_RSRV2    = 15,    // Reserved 2
    
    B0641_NUMCHANS = 16
};


#endif /**/
