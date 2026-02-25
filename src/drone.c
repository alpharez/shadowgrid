#include "drone.h"
#include "projectile.h"
#include "render.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Utility                                                              */
/* ------------------------------------------------------------------ */

static int absi(int x) { return x < 0 ? -x : x; }

static int chebyshev(int x1, int y1, int x2, int y2)
{
    int dx = absi(x1 - x2);
    int dy = absi(y1 - y2);
    return dx > dy ? dx : dy;
}

/* Bresenham LOS — same logic as guard.c */
static bool los_clear(const Map *map, int x0, int y0, int x1, int y1)
{
    int dx  = absi(x1 - x0);
    int dy  = absi(y1 - y0);
    int sx  = x0 < x1 ?  1 : -1;
    int sy  = y0 < y1 ?  1 : -1;
    int err = dx - dy;
    int x   = x0, y = y0;

    while (x != x1 || y != y1) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
        if (x == x1 && y == y1) break;
        if (map_blocks_sight(map, x, y)) return false;
    }
    return true;
}

static bool drone_can_see(const Drone *d, const Map *map, int px, int py)
{
    return chebyshev(d->x, d->y, px, py) <= DRONE_RANGE &&
           los_clear(map, d->x, d->y, px, py);
}

/* ------------------------------------------------------------------ */
/* Movement                                                             */
/* ------------------------------------------------------------------ */

static void drone_move_toward(Drone *d, const Map *map, int tx, int ty)
{
    if (d->x == tx && d->y == ty) return;

    int dx = 0, dy = 0;
    if (tx > d->x) dx =  1; else if (tx < d->x) dx = -1;
    if (ty > d->y) dy =  1; else if (ty < d->y) dy = -1;

#define passable(nx, ny) (!map_is_blocked(map, (nx), (ny)))
    if (dx && dy && passable(d->x + dx, d->y + dy)) { d->x += dx; d->y += dy; return; }
    if (dx       && passable(d->x + dx, d->y))       { d->x += dx;             return; }
    if (dy       && passable(d->x,      d->y + dy))  {             d->y += dy; return; }
#undef passable
}

static void patrol_step(Drone *d, const Map *map)
{
    int tx = d->patrol[d->wp_idx][0];
    int ty = d->patrol[d->wp_idx][1];

    if (d->x == tx && d->y == ty) {
        d->wp_idx ^= 1;
        tx = d->patrol[d->wp_idx][0];
        ty = d->patrol[d->wp_idx][1];
    }
    drone_move_toward(d, map, tx, ty);
}

/* ------------------------------------------------------------------ */
/* Shooting                                                             */
/* ------------------------------------------------------------------ */

/* Queue a hostile shot toward the player and alert nearby guards. */
static void drone_fire_player(Drone *d, GuardList *guards,
                              ProjectileList *pl, int px, int py)
{
    if (d->shot_timer > 0) return;
    d->shot_timer = DRONE_SHOT_COOLDOWN;

    proj_single(pl, d->x, d->y, px, py, DRONE_DAMAGE, COL_GUARD_ALERT, true);

    /* Noise: nearby unaware guards become suspicious */
    for (int j = 0; j < guards->count; j++) {
        Guard *g = &guards->guards[j];
        if (g->state == GUARD_UNAWARE &&
            chebyshev(g->x, g->y, d->x, d->y) <= DRONE_NOISE_RANGE) {
            g->state     = GUARD_SUSPICIOUS;
            g->suspicion = 1;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

bool drone_at(const DroneList *dl, int x, int y)
{
    for (int i = 0; i < dl->count; i++)
        if (dl->drones[i].x == x && dl->drones[i].y == y)
            return true;
    return false;
}

bool drone_player_attack(DroneList *dl, int x, int y, int dmg, Item *drop)
{
    for (int i = 0; i < dl->count; i++) {
        Drone *d = &dl->drones[i];
        if (d->x != x || d->y != y) continue;

        d->hp -= dmg;
        if (d->hp <= 0) {
            if (drop) memset(drop, 0, sizeof(*drop));  /* drones don't drop loot */
            dl->drones[i] = dl->drones[dl->count - 1];
            dl->count--;
        }
        return true;
    }
    return false;
}

void drone_stun_all(DroneList *dl, int turns)
{
    for (int i = 0; i < dl->count; i++)
        dl->drones[i].stun_timer = turns;
}

int drone_hack(DroneList *dl, int x, int y)
{
    for (int i = 0; i < dl->count; i++) {
        Drone *d = &dl->drones[i];
        if (chebyshev(d->x, d->y, x, y) > 1) continue;  /* must be adjacent */

        if (d->state == DRONE_HACKED) {
            /* Second hack: full shutdown — remove drone */
            dl->drones[i] = dl->drones[dl->count - 1];
            dl->count--;
            return 2;
        } else {
            /* First hack: compromise — drone now targets guards */
            d->state      = DRONE_HACKED;
            d->shot_timer = 0;  /* ready to fire immediately */
            return 1;
        }
    }
    return 0;
}

bool drone_any_alert(const DroneList *dl)
{
    for (int i = 0; i < dl->count; i++)
        if (dl->drones[i].state == DRONE_ALERT)
            return true;
    return false;
}

void drone_tick_all(DroneList *dl, const Map *map, GuardList *guards,
                    ProjectileList *pl, int px, int py)
{
    for (int i = 0; i < dl->count; i++) {
        Drone *d = &dl->drones[i];

        if (d->stun_timer > 0) {
            d->stun_timer--;
            continue;
        }

        if (d->shot_timer > 0)
            d->shot_timer--;

        bool sees_player = drone_can_see(d, map, px, py);

        switch (d->state) {

        case DRONE_PATROL:
            if (sees_player) {
                d->state = DRONE_ALERT;
                /* Fire immediately on first sight if ready */
                drone_fire_player(d, guards, pl, px, py);
            } else {
                /* Slow patrol: move every DRONE_MOVE_INTERVAL turns */
                if (++d->move_timer >= DRONE_MOVE_INTERVAL) {
                    d->move_timer = 0;
                    patrol_step(d, map);
                }
            }
            break;

        case DRONE_ALERT:
            if (!sees_player) {
                d->state = DRONE_PATROL;
                break;
            }
            drone_fire_player(d, guards, pl, px, py);
            break;

        case DRONE_HACKED: {
            /* Find nearest guard within range with clear LOS */
            int best_idx = -1, best_dist = DRONE_RANGE + 1;
            for (int j = 0; j < guards->count; j++) {
                Guard *g = &guards->guards[j];
                int dist = chebyshev(d->x, d->y, g->x, g->y);
                if (dist < best_dist &&
                    los_clear(map, d->x, d->y, g->x, g->y)) {
                    best_dist = dist;
                    best_idx  = j;
                }
            }
            if (best_idx >= 0 && d->shot_timer == 0) {
                Guard *tgt = &guards->guards[best_idx];
                /* Hacked shot: not hostile to player, green colour */
                proj_single(pl, d->x, d->y, tgt->x, tgt->y,
                            DRONE_DAMAGE, COL_DRONE_HACKED, false);
                d->shot_timer = DRONE_SHOT_COOLDOWN;
            }
            break;
        }

        }
    }
}

/* ------------------------------------------------------------------ */
/* Spawning                                                             */
/* ------------------------------------------------------------------ */

void drone_spawn(DroneList *dl, const Map *map, int difficulty)
{
    memset(dl, 0, sizeof(*dl));
    if (map->num_rooms < 4) return;

    int num_drones = 0;
    if      (difficulty >= 4) num_drones = 2;
    else if (difficulty >= 2) num_drones = 1;

    /* Place drones evenly through the middle section of rooms,
     * skipping rooms[0] (player start) and rooms[num_rooms-1] (stairs). */
    for (int n = 0; n < num_drones && dl->count < MAX_DRONES; n++) {
        int r = 1 + (n + 1) * (map->num_rooms - 2) / (num_drones + 1);
        if (r < 1)                    r = 1;
        if (r >= map->num_rooms - 1)  r = map->num_rooms - 2;

        const Room *room = &map->rooms[r];
        Drone *d = &dl->drones[dl->count++];
        memset(d, 0, sizeof(*d));

        d->x     = room->x + room->w / 2;
        d->y     = room->y + room->h / 2;
        d->hp    = DRONE_HP;
        d->state = DRONE_PATROL;

        /* Waypoint 0: this room's center */
        d->patrol[0][0] = d->x;
        d->patrol[0][1] = d->y;

        /* Waypoint 1: adjacent room */
        int next_r = (r + 1 < map->num_rooms - 1) ? r + 1 : r - 1;
        if (next_r < 1) next_r = 1;
        const Room *next = &map->rooms[next_r];
        d->patrol[1][0] = next->x + next->w / 2;
        d->patrol[1][1] = next->y + next->h / 2;

        d->wp_idx = 0;
    }
}
