#pragma once

#include <stdbool.h>

#define MAX_TRAPS 2

/*
 * Per-mission state for all Netrunner skill tree effects.
 * Passives are booleans set once at mission start from tree_bought.
 * Actives track availability and timers for once-per-mission abilities.
 */
typedef struct {

    /* --- Tier 1 passives ------------------------------------------- */
    bool scavenger;       /* T1 opt1: +1 hack power on all terminal attempts  */
    bool deep_scan;       /* T1 opt2: terminals/door/stairs pre-revealed      */

    /* --- Tier 2 actives -------------------------------------------- */
    bool overclock_avail; /* T2 opt2: can arm once-per-mission overclock      */
    bool overclock_armed; /* true = next hack attempt gains +2 hack power     */

    bool trap_avail;      /* T2 opt1: can place floor traps this mission      */
    int  trap_count;      /* number of traps currently placed (0..MAX_TRAPS)  */
    int  trap_x[MAX_TRAPS];
    int  trap_y[MAX_TRAPS];

    /* --- Tier 3 ---------------------------------------------------- */
    bool drone_buddy;     /* T3 opt1: passive — all guards always visible     */
    bool daemon_avail;    /* T3 opt0: lure one guard to random room           */
    bool emp_avail;       /* T3 opt2: stun all visible guards for 2 turns     */

    /* --- Tier 4 ---------------------------------------------------- */
    bool backdoor_king;   /* T4 opt0: one hack triggers both mission effects  */
    bool spoof_avail;     /* T4 opt1: reset all alert/hunting guards          */
    bool remote_det;      /* T4 opt2: hack terminals from adjacent tile       */

    /* --- Tier 5 ---------------------------------------------------- */
    bool godmode_avail;   /* T5 opt0: can activate GOD MODE                   */
    int  godmode_timer;   /* turns remaining while GOD MODE is active         */

} NetrunnerState;
