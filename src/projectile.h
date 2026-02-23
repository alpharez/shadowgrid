#pragma once

#include <stdbool.h>
#include "map.h"
#include "guard.h"
#include "items.h"

#define MAX_PROJECTILES      8
#define FLECHETTE_NOISE_RANGE 4   /* guards within this range hear the shot */

/*
 * A single projectile travelling along a Bresenham line.
 * Positions advance one grid cell per proj_step() call.
 */
typedef struct {
    int  x, y;        /* current grid position                          */
    int  sx, sy;      /* Bresenham step signs (+1 / 0 / -1)             */
    int  dx, dy;      /* absolute axis differences (Bresenham state)    */
    int  err;         /* Bresenham error accumulator                    */
    int  steps_left;  /* Chebyshev distance remaining to target         */
    int  damage;
    char symbol;      /* -, |, \, /  based on travel direction          */
    int  color_pair;
    bool active;
} Projectile;

typedef struct {
    Projectile p[MAX_PROJECTILES];
    int        count;
} ProjectileList;

/*
 * Fire a 3-dart flechette burst from (sx,sy) toward (tx,ty).
 * The centre dart aims directly; the outer two spread ±1 tile
 * perpendicular to the shot direction.
 * Returns the number of projectiles created (0 if list is full).
 */
int  proj_flechette(ProjectileList *pl,
                    int sx, int sy, int tx, int ty, int damage);

/*
 * Advance every active projectile one grid cell along its Bresenham line.
 * - Stops a projectile if it hits a wall/door or goes out of bounds.
 * - Calls guard_player_attack() on any guard occupying the landing cell;
 *   loot drops are appended to drops[0..MAX_GUARDS-1] and *ndrop updated.
 * Returns true if at least one projectile is still moving.
 */
bool proj_step(ProjectileList *pl, const Map *map, GuardList *guards,
               int px, int py, Item *drops, int *ndrop);

/* Reset the list (mark all inactive, set count = 0). */
void proj_clear(ProjectileList *pl);
