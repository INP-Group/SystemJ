#ifndef __KNOBS_TYPES_H
#define __KNOBS_TYPES_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/*
 *  Note: when adding/changing flags, the
 *  lib/Cdr/Cdr.c::_cdr_colalarms_list[] must be updated coherently.
 */

typedef enum {
    COLALARM_UNINITIALIZED = 0,  // Uninitialized
    COLALARM_JUSTCREATED,        // Just created
    COLALARM_NONE,               // Normal
    COLALARM_YELLOW,             // Value in yellow zone
    COLALARM_RED,                // Value in red zone
    COLALARM_WEIRD,              // Value is weird
    COLALARM_DEFUNCT,            // Defunct channel
    COLALARM_HWERR,              // Hardware error
    COLALARM_SFERR,              // Software error
    COLALARM_NOTFOUND,           // Channel not found
    COLALARM_RELAX,              // Relaxing after alarm
    COLALARM_OTHEROP,            // Other operator is active
    COLALARM_PRGLYCHG,           // Programmatically changed

    COLALARM_FIRST = COLALARM_UNINITIALIZED,
    COLALARM_LAST  = COLALARM_PRGLYCHG
} colalarm_t;

typedef struct _knobinfo_t_struct *Knob;

typedef struct _eleminfo_t_struct *ElemInfo;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __KNOBS_TYPES_H */
