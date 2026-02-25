#pragma once

#include "map.h"
#include "guard.h"
#include "items.h"
#include <stdbool.h>

/* Forward-declared so projectile.h and drone.h don't include each other. */
typedef struct ProjectileList ProjectileList;

#define MAX_DRONES          4
#define DRONE_HP            4   /* fragile — 4 hits and it's scrap */
#define DRONE_RANGE         6   /* tiles before it opens fire */
#define DRONE_DAMAGE        2   /* damage per shot */
#define DRONE_SHOT_COOLDOWN 2   /* turns between shots */
#define DRONE_NOISE_RANGE   5   /* guards within this range hear the shot */
#define DRONE_MOVE_INTERVAL 2   /* patrol steps every N turns (slow hover) */

typedef enum {
    DRONE_PATROL,   /* slow patrol, scanning for targets */
    DRONE_ALERT,    /* spotted player — hovering in place, shooting */
    DRONE_HACKED,   /* compromised — now targets guards */
} DroneState;

typedef struct {
    int x, y;
    int hp;
    DroneState state;
    int patrol[2][2];    /* two waypoints: patrol[i] = {x, y} */
    int wp_idx;
    int shot_timer;      /* turns until next shot (0 = ready) */
    int move_timer;      /* ticks until next patrol step */
    int stun_timer;      /* completely frozen while > 0 */
} Drone;

typedef struct {
    Drone drones[MAX_DRONES];
    int   count;
} DroneList;

/* Spawn drones based on mission difficulty (0-5). */
void drone_spawn(DroneList *dl, const Map *map, int difficulty);

/* Advance all drones one turn.
 * Hostile shots toward the player and hacked-drone shots toward guards
 * are both queued into pl as animated projectiles — the caller drives the
 * animation loop and applies player damage via proj_step's player_dmg out. */
void drone_tick_all(DroneList *dl, const Map *map, GuardList *guards,
                    ProjectileList *pl, int px, int py);

/* Returns true if any drone occupies (x, y). */
bool drone_at(const DroneList *dl, int x, int y);

/* Player attacks the drone at (x, y) for dmg points.
 * Returns true if a drone was hit. drop is filled on kill (may be empty). */
bool drone_player_attack(DroneList *dl, int x, int y, int dmg, Item *drop);

/* Stun all drones for turns turns (EMP / GOD MODE). */
void drone_stun_all(DroneList *dl, int turns);

/* Hack the drone at or adjacent to (x, y).
 * PATROL/ALERT → HACKED; HACKED → removed (disabled).
 * Returns: 0=not found, 1=compromised, 2=disabled. */
int drone_hack(DroneList *dl, int x, int y);

/* Returns true if any drone is in DRONE_ALERT state. */
bool drone_any_alert(const DroneList *dl);
