#pragma once

#include <stdbool.h>

/*
 * Per-mission state for Chrome skill tree active abilities.
 * Passives (Iron Grip, Counterattack, Pain Editor, Precise Shot) are handled
 * inline in game_run using HAS_SKILL checks on player->tree_bought.
 */
typedef struct {

    /* --- Tier 3 active ---------------------------------------------- */
    bool suppressive_avail; /* T3 opt0: Suppressive Fire — stun visible guards */

    /* --- Tier 4 active ---------------------------------------------- */
    bool chrome_rush_avail; /* T4 opt0: Chrome Rush — charge through guards    */

    /* --- Tier 5 active ---------------------------------------------- */
    bool reaper_avail;      /* T5 opt0: REAPER — once/mission activation       */
    bool reaper_active;     /* true once REAPER has been activated this mission */

    /* Directional tracking for Chrome Rush */
    int  last_dx, last_dy;  /* direction of last player movement/attack        */

} ChromeState;
