#include "projectile.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static int absi(int x) { return x < 0 ? -x : x; }

/*
 * Choose a single ASCII symbol that best represents the direction
 * from (x0,y0) toward (x1,y1).
 */
static char direction_symbol(int x0, int y0, int x1, int y1)
{
    int rdx = x1 - x0;
    int rdy = y1 - y0;
    int adx = absi(rdx);
    int ady = absi(rdy);

    if (adx == 0 && ady == 0) return '*';
    if (adx >= ady * 2)       return '-';   /* mostly horizontal */
    if (ady >= adx * 2)       return '|';   /* mostly vertical   */
    /* roughly diagonal — orientation matches screen convention   */
    return ((rdx > 0) == (rdy > 0)) ? '\\' : '/';
}

/*
 * Initialise a projectile using a Bresenham line from (x0,y0) to (x1,y1).
 * The projectile sits at (x0,y0) and has not moved yet; the first
 * proj_step() call will advance it to the next cell.
 */
static void proj_init(Projectile *p,
                      int x0, int y0, int x1, int y1,
                      int damage, int color_pair)
{
    memset(p, 0, sizeof(*p));
    p->x    = x0;
    p->y    = y0;
    p->dx   = absi(x1 - x0);
    p->dy   = absi(y1 - y0);
    p->sx   = (x1 > x0) ? 1 : (x1 < x0 ? -1 : 0);
    p->sy   = (y1 > y0) ? 1 : (y1 < y0 ? -1 : 0);
    p->err  = p->dx - p->dy;
    p->steps_left = (p->dx > p->dy) ? p->dx : p->dy;  /* Chebyshev distance */
    p->damage     = damage;
    p->symbol     = direction_symbol(x0, y0, x1, y1);
    p->color_pair = color_pair;
    p->active     = (p->steps_left > 0);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int proj_single(ProjectileList *pl,
                int sx, int sy, int tx, int ty,
                int damage, int color_pair, bool hostile)
{
    if (pl->count >= MAX_PROJECTILES) return 0;
    Projectile *p = &pl->p[pl->count++];
    proj_init(p, sx, sy, tx, ty, damage, color_pair);
    p->can_hit_player = hostile;
    return 1;
}

int proj_flechette(ProjectileList *pl,
                   int sx, int sy, int tx, int ty, int damage)
{
    if (pl->count + 3 > MAX_PROJECTILES) return 0;

    int adx = absi(tx - sx);
    int ady = absi(ty - sy);

    /* Perpendicular spread: ±1 in the minor axis */
    int px, py;
    if (adx >= ady) { px = 0; py = 1; }   /* horizontal shot → spread vertically   */
    else            { px = 1; py = 0; }   /* vertical shot   → spread horizontally */

    /* Centre dart */
    proj_init(&pl->p[pl->count++], sx, sy, tx,      ty,      damage, 23);
    /* Upper spread dart */
    proj_init(&pl->p[pl->count++], sx, sy, tx - px, ty - py, damage, 23);
    /* Lower spread dart */
    proj_init(&pl->p[pl->count++], sx, sy, tx + px, ty + py, damage, 23);

    return 3;
}

bool proj_step(ProjectileList *pl, const Map *map, GuardList *guards,
               DroneList *drones,
               int px, int py, Item *drops, int *ndrop, int *player_dmg)
{
    bool any_active = false;

    for (int i = 0; i < pl->count; i++) {
        Projectile *p = &pl->p[i];
        if (!p->active) continue;
        if (p->steps_left <= 0) { p->active = false; continue; }

        /* --- Bresenham step --- */
        int e2 = 2 * p->err;
        if (e2 > -p->dy) { p->err -= p->dy; p->x += p->sx; }
        if (e2 <  p->dx) { p->err += p->dx; p->y += p->sy; }
        p->steps_left--;

        /* Bounds check */
        if (p->x < 0 || p->x >= MAP_WIDTH ||
            p->y < 0 || p->y >= MAP_HEIGHT) {
            p->active = false;
            continue;
        }

        /* Wall / door collision */
        if (map_blocks_sight(map, p->x, p->y)) {
            p->active = false;
            continue;
        }

        /* Guard collision: delegate to existing attack logic */
        if (guard_at(guards, p->x, p->y)) {
            Item drop = {0};
            guard_player_attack(guards, p->x, p->y, p->damage,
                                px, py, &drop);
            if (item_valid(&drop) && ndrop && *ndrop < MAX_GUARDS)
                drops[(*ndrop)++] = drop;
            p->active = false;
            continue;
        }

        /* Drone collision: projectiles can hit and damage drones */
        if (drone_at(drones, p->x, p->y)) {
            drone_player_attack(drones, p->x, p->y, p->damage, NULL);
            p->active = false;
            continue;
        }

        /* Player collision: only hostile projectiles (e.g. drone shots) */
        if (p->can_hit_player && p->x == px && p->y == py) {
            if (player_dmg) *player_dmg += p->damage;
            p->active = false;
            continue;
        }

        any_active = true;
    }

    return any_active;
}

void proj_clear(ProjectileList *pl)
{
    memset(pl, 0, sizeof(*pl));
}
