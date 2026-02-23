#pragma once

#include "map.h"
#include "items.h"
#include <stdbool.h>

#define MAX_GUARDS          8
#define GUARD_FOV_RANGE     7   /* detection range, standing */
#define GUARD_CROUCH_RANGE  3   /* detection range vs. crouched player */
#define GUARD_SUSPICION_MAX 3   /* ticks of sight before suspicious→alert */
#define GUARD_HUNT_TIMEOUT  8   /* ticks without sight before hunting→alert */
#define GUARD_MAX_HP        6
#define GUARD_ELITE_HP      10  /* elite guard hit points */
#define GUARD_ELITE_FOV     2   /* extra detection tiles for elite guards */

typedef enum {
    GUARD_UNAWARE,
    GUARD_SUSPICIOUS,
    GUARD_ALERT,
    GUARD_HUNTING,
} GuardState;

typedef struct {
    int x, y;
    int hp, max_hp;
    GuardState state;
    int suspicion;           /* 1..GUARD_SUSPICION_MAX while suspicious */
    int hunt_timer;          /* counts down when sight is lost */
    int patrol[2][2];        /* two waypoints: patrol[i] = {x, y} */
    int wp_idx;              /* 0 or 1: which waypoint we're heading toward */
    int last_seen_x, last_seen_y;
    int disarm_timer;        /* turns remaining where this guard can't deal melee damage */
    int stun_timer;          /* turns remaining where this guard is completely frozen */
    bool elite;              /* elite guard: higher HP, wider FOV, harder hits */
    int  fov_bonus;          /* extra detection tiles (0 for normal, +2 for elite) */
} Guard;

typedef struct {
    Guard guards[MAX_GUARDS];
    int   count;
} GuardList;

/* Place guards in rooms (skips player start and stair room).
 * difficulty: 0-5 scale — higher values spawn more elite guards. */
void guard_spawn(GuardList *gl, const Map *map, int difficulty);

/* Advance all guards one turn.
 * detection_range : max tiles a guard can spot the player.
 * suspicion_max   : ticks of sight before suspicious→alert (Light Step doubles this).
 * suspicion_decay : suspicion lost per turn out of sight (Grey Man sets this to 2).
 * vanish          : if true, guards instantly de-aggro when they lose LOS (Vanish skill).
 * Returns total melee damage dealt to the player this turn. */
int guard_tick_all(GuardList *gl, const Map *map, int px, int py,
                   int detection_range, int suspicion_max, int suspicion_decay,
                   bool vanish);

/* Returns true if any guard occupies (x, y). */
bool guard_at(const GuardList *gl, int x, int y);

/* Returns true if this guard has clear line of sight to (px, py)
 * within detection_range tiles. */
bool guard_can_see(const Guard *g, const Map *map, int px, int py,
                   int detection_range);

/* Helper: compute the effective detection range from player state.
 * Each stealth point above 1 reduces the range by 1 (minimum 1). */
static inline int guard_detection_range(bool crouched, int stealth_skill)
{
    int base = crouched ? GUARD_CROUCH_RANGE : GUARD_FOV_RANGE;
    int range = base - (stealth_skill - 1);
    return range < 1 ? 1 : range;
}

/* Helper: return the state of the guard at (x, y), or GUARD_UNAWARE if none. */
static inline GuardState guard_state_at(const GuardList *gl, int x, int y)
{
    for (int i = 0; i < gl->count; i++)
        if (gl->guards[i].x == x && gl->guards[i].y == y)
            return gl->guards[i].state;
    return GUARD_UNAWARE;
}

/* Returns the highest alert state across all guards (for status display). */
GuardState guard_max_state(const GuardList *gl);

/* Player attacks the guard at (x, y) for dmg points.
 * Immediately sets the guard to HUNTING toward (px, py).
 * Removes the guard if it dies.  Returns true if a guard was hit.
 * If the guard dies and drops loot, *drop is filled (type != ITEM_NONE);
 * pass drop=NULL to ignore. */
bool guard_player_attack(GuardList *gl, int x, int y, int dmg, int px, int py,
                         Item *drop);

/* Disarm the guard at (x, y) for the given number of turns.
 * While disarmed the guard cannot deal melee damage. */
void guard_disarm(GuardList *gl, int x, int y, int turns);

/* Stun all guards for the given number of turns (completely frozen). */
void guard_stun_all(GuardList *gl, int turns);

/* Reset all ALERT/HUNTING guards to UNAWARE (network erase effect). */
void guard_erase_all(GuardList *gl);

/* Raise all UNAWARE guards to SUSPICIOUS (network alarm on hack failure). */
void guard_alarm_network(GuardList *gl);
