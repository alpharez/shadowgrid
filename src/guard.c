#include "guard.h"
#include "items.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Utility                                                              */
/* ------------------------------------------------------------------ */

static int absi(int x) { return x < 0 ? -x : x; }

/* Chebyshev (8-directional) distance */
static int chebyshev(int x1, int y1, int x2, int y2)
{
    int dx = absi(x1 - x2);
    int dy = absi(y1 - y2);
    return dx > dy ? dx : dy;
}

/* ------------------------------------------------------------------ */
/* Line of sight (Bresenham)                                           */
/* ------------------------------------------------------------------ */

/* Returns true if the path from (gx,gy) to (px,py) is unobstructed.
 * The guard's own tile and the player's tile are not tested. */
static bool los_clear(const Map *map, int gx, int gy, int px, int py)
{
    int dx = absi(px - gx);
    int dy = absi(py - gy);
    int sx = gx < px ? 1 : -1;
    int sy = gy < py ? 1 : -1;
    int err = dx - dy;
    int x = gx, y = gy;

    while (x != px || y != py) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
        if (x == px && y == py) break;   /* end tile: don't test */
        if (map_blocks_sight(map, x, y)) return false;
    }
    return true;
}

bool guard_can_see(const Guard *g, const Map *map, int px, int py,
                   int detection_range)
{
    if (chebyshev(g->x, g->y, px, py) > detection_range)
        return false;
    return los_clear(map, g->x, g->y, px, py);
}

/* ------------------------------------------------------------------ */
/* Guard list queries                                                   */
/* ------------------------------------------------------------------ */

bool guard_at(const GuardList *gl, int x, int y)
{
    for (int i = 0; i < gl->count; i++) {
        if (gl->guards[i].x == x && gl->guards[i].y == y)
            return true;
    }
    return false;
}

GuardState guard_max_state(const GuardList *gl)
{
    GuardState max = GUARD_UNAWARE;
    for (int i = 0; i < gl->count; i++) {
        if (gl->guards[i].state > max)
            max = gl->guards[i].state;
    }
    return max;
}

/* ------------------------------------------------------------------ */
/* Movement                                                             */
/* ------------------------------------------------------------------ */

/* Move g one step toward (tx, ty), avoiding walls, other guards, and
 * the tile (bx, by) — pass bx=-1,by=-1 to disable the extra block.
 * Returns true if the guard moved. */
static bool move_toward(Guard *g, const Map *map, const GuardList *gl,
                        int tx, int ty, int bx, int by)
{
    if (g->x == tx && g->y == ty) return false;

    int dx = 0, dy = 0;
    if (tx > g->x) dx =  1; else if (tx < g->x) dx = -1;
    if (ty > g->y) dy =  1; else if (ty < g->y) dy = -1;

#define passable(nx, ny) \
    (!map_is_blocked(map, (nx), (ny)) && \
     !((nx) == bx && (ny) == by) && \
     !guard_at(gl, (nx), (ny)))

    /* Prefer diagonal, then x-only, then y-only */
    if (dx && dy && passable(g->x + dx, g->y + dy)) {
        g->x += dx; g->y += dy; return true;
    }
    if (dx && passable(g->x + dx, g->y)) {
        g->x += dx; return true;
    }
    if (dy && passable(g->x, g->y + dy)) {
        g->y += dy; return true;
    }

#undef passable
    return false;
}

/* Advance patrol one step: move toward the active waypoint,
 * flipping direction on arrival. */
static void patrol_step(Guard *g, const Map *map, const GuardList *gl)
{
    int tx = g->patrol[g->wp_idx][0];
    int ty = g->patrol[g->wp_idx][1];

    if (g->x == tx && g->y == ty) {
        g->wp_idx ^= 1;   /* reached waypoint — head back */
        tx = g->patrol[g->wp_idx][0];
        ty = g->patrol[g->wp_idx][1];
    }

    move_toward(g, map, gl, tx, ty, -1, -1);
}

/* ------------------------------------------------------------------ */
/* AI tick                                                              */
/* ------------------------------------------------------------------ */

/* Tick one guard.  Returns melee damage dealt to the player. */
static int guard_tick_one(Guard *g, const Map *map, GuardList *gl,
                          int px, int py, int detection_range,
                          int suspicion_max, int suspicion_decay,
                          bool vanish)
{
    /* Tick stun timer — guard is completely frozen while stunned */
    if (g->stun_timer > 0) {
        g->stun_timer--;
        return 0;
    }

    /* Tick disarm timer */
    if (g->disarm_timer > 0)
        g->disarm_timer--;

    bool sees = guard_can_see(g, map, px, py, detection_range + g->fov_bonus);

    switch (g->state) {

    case GUARD_UNAWARE:
        if (sees) {
            g->state       = GUARD_SUSPICIOUS;
            g->suspicion   = 1;
            g->last_seen_x = px;
            g->last_seen_y = py;
        } else {
            patrol_step(g, map, gl);
        }
        break;

    case GUARD_SUSPICIOUS:
        if (sees) {
            g->last_seen_x = px;
            g->last_seen_y = py;
            if (++g->suspicion >= suspicion_max) {
                g->state      = GUARD_ALERT;
                g->hunt_timer = GUARD_HUNT_TIMEOUT;
                g->suspicion  = 0;
            }
            /* Stand still while building suspicion */
        } else {
            g->suspicion -= suspicion_decay;
            if (g->suspicion <= 0) {
                g->state     = GUARD_UNAWARE;
                g->suspicion = 0;
            }
        }
        break;

    case GUARD_ALERT:
        if (sees) {
            g->last_seen_x = px;
            g->last_seen_y = py;
            g->state       = GUARD_HUNTING;
            g->hunt_timer  = GUARD_HUNT_TIMEOUT;
        } else {
            /* Vanish: SHADOW (0), tier 3, opt 0 — instant de-aggro on LOS break */
            if (vanish) {
                g->state     = GUARD_UNAWARE;
                g->suspicion = 0;
                g->hunt_timer = 0;
            } else {
                /* Move toward last known position; time out back to suspicious */
                move_toward(g, map, gl, g->last_seen_x, g->last_seen_y, px, py);
                if (--g->hunt_timer <= 0) {
                    g->state     = GUARD_SUSPICIOUS;
                    g->suspicion = 1;
                }
            }
        }
        break;

    case GUARD_HUNTING:
        if (sees) {
            g->last_seen_x = px;
            g->last_seen_y = py;
            g->hunt_timer  = GUARD_HUNT_TIMEOUT;
        } else {
            /* Vanish: SHADOW (0), tier 3, opt 0 — instant de-aggro on LOS break */
            if (vanish) {
                g->state      = GUARD_UNAWARE;
                g->suspicion  = 0;
                g->hunt_timer = 0;
                break;
            }
            if (--g->hunt_timer <= 0) {
                g->state      = GUARD_ALERT;
                g->hunt_timer = GUARD_HUNT_TIMEOUT;
                break;
            }
        }
        /* Chase: move toward last known position (updated above if visible) */
        move_toward(g, map, gl, g->last_seen_x, g->last_seen_y, px, py);

        /* Melee: adjacent hunting guard deals damage (suppressed while disarmed).
         * Elite guards hit harder. */
        if (chebyshev(g->x, g->y, px, py) == 1 && g->disarm_timer == 0)
            return g->elite ? 3 : 2;
        break;
    }

    return 0;
}

bool guard_player_attack(GuardList *gl, int x, int y, int dmg, int px, int py,
                         Item *drop)
{
    for (int i = 0; i < gl->count; i++) {
        Guard *g = &gl->guards[i];
        if (g->x != x || g->y != y) continue;

        /* Alert immediately */
        g->state       = GUARD_HUNTING;
        g->hunt_timer  = GUARD_HUNT_TIMEOUT;
        g->last_seen_x = px;
        g->last_seen_y = py;

        g->hp -= dmg;
        if (g->hp <= 0) {
            /* Normal guards: 50% loot chance; elite guards: 75% loot chance.
             * Loot type: 40% stim pack, 40% med kit, 20% trauma kit. */
            if (drop) {
                int threshold = g->elite ? 3 : 2;
                if (rand() % 4 < threshold) {
                    int roll = rand() % 5;
                    if      (roll < 2) *drop = item_make_stimpack();
                    else if (roll < 4) *drop = item_make_medkit();
                    else               *drop = item_make_trauma_kit();
                } else {
                    memset(drop, 0, sizeof(*drop));
                }
            }
            /* Remove by swapping with the last guard in the list */
            gl->guards[i] = gl->guards[gl->count - 1];
            gl->count--;
        }
        return true;
    }
    return false;
}

int guard_tick_all(GuardList *gl, const Map *map, int px, int py,
                   int detection_range, int suspicion_max, int suspicion_decay,
                   bool vanish)
{
    int total_dmg = 0;
    for (int i = 0; i < gl->count; i++)
        total_dmg += guard_tick_one(&gl->guards[i], map, gl, px, py,
                                    detection_range, suspicion_max,
                                    suspicion_decay, vanish);
    return total_dmg;
}

void guard_disarm(GuardList *gl, int x, int y, int turns)
{
    for (int i = 0; i < gl->count; i++) {
        if (gl->guards[i].x == x && gl->guards[i].y == y) {
            gl->guards[i].disarm_timer = turns;
            return;
        }
    }
}

void guard_stun_all(GuardList *gl, int turns)
{
    for (int i = 0; i < gl->count; i++)
        gl->guards[i].stun_timer = turns;
}

void guard_erase_all(GuardList *gl)
{
    for (int i = 0; i < gl->count; i++) {
        Guard *g = &gl->guards[i];
        if (g->state >= GUARD_ALERT) {
            g->state      = GUARD_UNAWARE;
            g->suspicion  = 0;
            g->hunt_timer = 0;
        }
    }
}

void guard_alarm_network(GuardList *gl)
{
    for (int i = 0; i < gl->count; i++) {
        Guard *g = &gl->guards[i];
        if (g->state == GUARD_UNAWARE) {
            g->state     = GUARD_SUSPICIOUS;
            g->suspicion = 1;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Spawning                                                             */
/* ------------------------------------------------------------------ */

void guard_spawn(GuardList *gl, const Map *map, int difficulty)
{
    memset(gl, 0, sizeof(*gl));
    if (map->num_rooms < 3) return;

    /* Skip rooms[0] (player start) and rooms[num_rooms-1] (stairs).
     * Each guard patrols between its room and the next. */
    for (int r = 1; r < map->num_rooms - 1 && gl->count < MAX_GUARDS; r++) {
        Guard *g = &gl->guards[gl->count++];
        memset(g, 0, sizeof(*g));

        const Room *room = &map->rooms[r];
        g->x      = room->x + room->w / 2;
        g->y      = room->y + room->h / 2;
        g->hp     = GUARD_MAX_HP;
        g->max_hp = GUARD_MAX_HP;
        g->state  = GUARD_UNAWARE;

        /* Waypoint 0: this room's center */
        g->patrol[0][0] = g->x;
        g->patrol[0][1] = g->y;

        /* Waypoint 1: next room's center (or previous if at the end) */
        int next_r = (r + 1 < map->num_rooms) ? r + 1 : r - 1;
        const Room *next = &map->rooms[next_r];
        g->patrol[1][0] = next->x + next->w / 2;
        g->patrol[1][1] = next->y + next->h / 2;

        g->wp_idx = 0;
    }

    /* Promote the last N guards to elite based on mission difficulty.
     * diff 0-2: 0 elites, diff 3: 1, diff 4: 2, diff 5+: 3 */
    int elite_count = 0;
    if (difficulty >= 5)      elite_count = 3;
    else if (difficulty >= 4) elite_count = 2;
    else if (difficulty >= 3) elite_count = 1;

    for (int i = gl->count - 1; i >= 0 && elite_count > 0; i--, elite_count--) {
        Guard *g = &gl->guards[i];
        g->elite     = true;
        g->fov_bonus = GUARD_ELITE_FOV;
        g->hp        = GUARD_ELITE_HP;
        g->max_hp    = GUARD_ELITE_HP;
    }
}
