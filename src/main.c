#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "map.h"
#include "fov.h"
#include "entity.h"
#include "items.h"
#include "guard.h"
#include "drone.h"
#include "render.h"
#include "projectile.h"
#include "netrunner.h"
#include "chrome.h"
#include "hub.h"

/* Returns true if any of the 8 neighbours of (x, y) blocks sight (is a wall). */
static bool near_wall(const Map *map, int x, int y)
{
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            if (map_blocks_sight(map, x + dx, y + dy)) return true;
        }
    return false;
}

static void ncurses_init(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    render_init();
}

static void ncurses_cleanup(void)
{
    endwin();
}

/* Returns true if the action consumed a turn (guards should tick). */
static bool handle_input(int ch, Entity *player, Map *map,
                         GuardList *guards, DroneList *drones,
                         bool *quick_draw_avail,
                         bool *vent_used, bool *bt_avail, bool *bt_active,
                         bool *gp_avail, int *gp_timer,
                         bool *done, char *msg, NetrunnerState *nr,
                         ChromeState *cr)
{
    int dx = 0, dy = 0;

    switch (ch) {
    /* vi-keys */
    case 'h': case KEY_LEFT:  dx = -1; break;
    case 'l': case KEY_RIGHT: dx =  1; break;
    case 'k': case KEY_UP:    dy = -1; break;
    case 'j': case KEY_DOWN:  dy =  1; break;
    /* diagonals */
    case 'y': dx = -1; dy = -1; break;
    case 'u': dx =  1; dy = -1; break;
    case 'b': dx = -1; dy =  1; break;
    case 'n': dx =  1; dy =  1; break;

    case 'q':
        *done = true;
        return false;

    case 'm':
        entity_use_medkit(player);
        return true;  /* costs a turn */

    case 'c': {
        /* Toggle crouch.
         * Quick Crouch (SHADOW tree, tier 0, option 1) makes this free. */
        player->crouched = !player->crouched;
        bool has_quick_crouch = (player->tree_bought[0][0] & (1 << 1)) != 0;
        return !has_quick_crouch;  /* free action with the skill */
    }

    case 'v': {
        /* Vent Rat: SHADOW tree (0), tier 2, option 2.
         * Teleports the player between the two vent openings. Once per mission. */
        bool has_vent_rat = (player->tree_bought[0][2] & (1 << 2)) != 0;
        if (!has_vent_rat || *vent_used)
            return false;
        if (map->tiles[player->y][player->x].type != TILE_VENT)
            return false;
        /* Find which vent the player is on, teleport to the other */
        for (int i = 0; i < map->num_vents; i++) {
            if (map->vents[i][0] == player->x && map->vents[i][1] == player->y) {
                int other = 1 - i;
                player->x = map->vents[other][0];
                player->y = map->vents[other][1];
                *vent_used = true;
                return true;
            }
        }
        return false;
    }

    case 'x': {
        /* Hack terminal or adjacent drone.
         * Terminal: Hack power = SKILL_HACKING + deck bonus + Scavenger + Overclock.
         * Quick Hack   (NETRUNNER T1 opt0): success is a free action.
         * Ghost Signal (NETRUNNER T2 opt0): failure raises no alarm.
         * Overclock    (NETRUNNER T2 opt2): +2 power on next attempt.
         * Backdoor King(NETRUNNER T4 opt0): success triggers both effects.
         * Remote Det.  (NETRUNNER T4 opt2): hack from adjacent tile.
         * Drone: always requires adjacency; first jack = HACKED, second = disabled. */

        /* Locate the terminal to hack: on current tile, or adjacent (Remote Det.) */
        int hack_tx = -1, hack_ty = -1;
        if (map->tiles[player->y][player->x].type == TILE_TERMINAL) {
            hack_tx = player->x;
            hack_ty = player->y;
        } else if (nr->remote_det) {
            for (int dy2 = -1; dy2 <= 1 && hack_tx < 0; dy2++) {
                for (int dx2 = -1; dx2 <= 1 && hack_tx < 0; dx2++) {
                    if (dx2 == 0 && dy2 == 0) continue;
                    int nx2 = player->x + dx2, ny2 = player->y + dy2;
                    if (nx2 >= 0 && nx2 < MAP_WIDTH &&
                        ny2 >= 0 && ny2 < MAP_HEIGHT &&
                        map->tiles[ny2][nx2].type == TILE_TERMINAL) {
                        hack_tx = nx2; hack_ty = ny2;
                    }
                }
            }
        }

        /* No terminal — try hacking an adjacent drone instead */
        if (hack_tx < 0) {
            int result = drone_hack(drones, player->x, player->y);
            if (result == 1) {
                snprintf(msg, 80, "JACK IN -- drone compromised, turned against corp");
                return true;
            } else if (result == 2) {
                snprintf(msg, 80, "DRONE OFFLINE -- remote shutdown successful");
                return true;
            }
            return false;
        }

        /* Find terminal index */
        int tidx = -1;
        for (int i = 0; i < map->num_terminals; i++) {
            if (map->terminals[i][0] == hack_tx &&
                map->terminals[i][1] == hack_ty) {
                tidx = i;
                break;
            }
        }
        if (tidx < 0) return false;

        int hack_power = player->skills[SKILL_HACKING] + entity_gear_hack(player);
        if (nr->scavenger)     hack_power += 1;  /* Scavenger: +1 passive bonus */
        if (nr->overclock_armed) {               /* Overclock: +2, consume arm  */
            hack_power += 2;
            nr->overclock_armed = false;
        }
        bool has_quick_hack   = (player->tree_bought[1][0] & (1 << 0)) != 0;
        bool has_ghost_signal = (player->tree_bought[1][1] & (1 << 0)) != 0;

        if (hack_power >= map->terminal_security[tidx]) {
            /* Success: Backdoor King applies both effects; otherwise apply one */
            bool do_door   = nr->backdoor_king ||
                             map->terminal_effects[tidx] == HACK_OPEN_DOOR;
            bool do_unlock = nr->backdoor_king ||
                             map->terminal_effects[tidx] == HACK_UNLOCK;
            if (do_door && map->tiles[map->door_y][map->door_x].type == TILE_DOOR)
                map->tiles[map->door_y][map->door_x].type = TILE_FLOOR;
            if (do_unlock &&
                map->tiles[map->stairs_y][map->stairs_x].type == TILE_STAIRS_LOCKED)
                map->tiles[map->stairs_y][map->stairs_x].type = TILE_STAIRS;
            map->tiles[hack_ty][hack_tx].type = TILE_FLOOR;
            if (nr->backdoor_king)
                snprintf(msg, 80, "HACK SUCCESS -- door opened + stairway live (Backdoor King)");
            else if (map->terminal_effects[tidx] == HACK_OPEN_DOOR)
                snprintf(msg, 80, "HACK SUCCESS -- security door opened");
            else
                snprintf(msg, 80, "HACK SUCCESS -- stairway unlocked");
            return !has_quick_hack;
        } else {
            if (!has_ghost_signal) {
                guard_alarm_network(guards);
                snprintf(msg, 80, "HACK FAILED -- security %d vs hack power %d -- alarm raised",
                         map->terminal_security[tidx], hack_power);
            } else {
                snprintf(msg, 80, "HACK FAILED -- security %d vs hack power %d",
                         map->terminal_security[tidx], hack_power);
            }
            return true;
        }
    }

    case 'g': {
        /* Ghost Protocol: SHADOW tree (0), tier 4, option 0.
         * 5 turns of complete invisibility — guards' detection range set to 0. */
        bool has_gp = (player->tree_bought[0][4] & (1 << 0)) != 0;
        if (!has_gp || !*gp_avail || *gp_timer > 0)
            return false;
        *gp_avail = false;
        *gp_timer = 5;
        return false;  /* activation is free */
    }

    case 't': {
        /* Bullet Time: CHROME tree (2), tier 1, option 0.
         * Activating is a free action; the next turn-consuming action won't
         * tick guards, giving the player 2 actions before guards respond. */
        bool has_bt = (player->tree_bought[2][1] & (1 << 0)) != 0;
        if (!has_bt || !*bt_avail || *bt_active)
            return false;
        *bt_active = true;
        *bt_avail  = false;
        return false;  /* activation itself is free */
    }

    case 'i': case 'I':
        mission_inventory(player);
        return false;  /* free action — time does not pass */

    case 'o': case 'O':
        /* Overclock (NETRUNNER T2 opt2): arm +2 hack bonus for next terminal.
         * Once per mission; free action to arm. */
        if (!nr->overclock_avail || nr->overclock_armed)
            return false;
        nr->overclock_avail = false;
        nr->overclock_armed = true;
        snprintf(msg, 80, "OVERCLOCK armed -- next hack +2 power");
        return false;

    case 'p': case 'P':
        /* Trap Master (NETRUNNER T2 opt1): place a stun trap on current tile.
         * Max MAX_TRAPS per mission; guard that steps on it freezes 3 turns. */
        if (!nr->trap_avail || nr->trap_count >= MAX_TRAPS)
            return false;
        if (map->tiles[player->y][player->x].type != TILE_FLOOR)
            return false;  /* only on plain floor */
        nr->trap_x[nr->trap_count] = player->x;
        nr->trap_y[nr->trap_count] = player->y;
        nr->trap_count++;
        if (nr->trap_count >= MAX_TRAPS)
            nr->trap_avail = false;
        snprintf(msg, 80, "Trap placed -- %d/%d traps used",
                 nr->trap_count, MAX_TRAPS);
        return false;  /* free action */

    case 'd': case 'D':
        /* Daemon (NETRUNNER T3 opt0): lure one guard to a random room.
         * Targets the nearest unaware guard; once per mission. */
        if (!nr->daemon_avail) return false;
        {
            int best = -1, best_d = 9999;
            for (int i = 0; i < guards->count; i++) {
                Guard *g = &guards->guards[i];
                if (g->state != GUARD_UNAWARE) continue;
                int dd = abs(g->x - player->x) + abs(g->y - player->y);
                if (dd < best_d) { best_d = dd; best = i; }
            }
            if (best < 0) {
                snprintf(msg, 80, "DAEMON -- no unaware guards to lure");
                return false;
            }
            /* Pick a random room (not rooms[0] or last) as the lure target */
            int num = map->num_rooms;
            int lure_r = (num > 2) ? (1 + rand() % (num - 2)) : 0;
            guards->guards[best].state      = GUARD_ALERT;
            guards->guards[best].hunt_timer = 12;
            guards->guards[best].last_seen_x =
                map->rooms[lure_r].x + map->rooms[lure_r].w / 2;
            guards->guards[best].last_seen_y =
                map->rooms[lure_r].y + map->rooms[lure_r].h / 2;
            nr->daemon_avail = false;
            snprintf(msg, 80, "DAEMON deployed -- guard lured away");
        }
        return true;

    case 'e': case 'E':
        /* EMP Specialist (NETRUNNER T3 opt2): stun all visible guards/drones 2 turns.
         * Once per mission; costs a turn. */
        if (!nr->emp_avail) return false;
        {
            int hit = 0;
            for (int i = 0; i < guards->count; i++) {
                Guard *g = &guards->guards[i];
                if (map->tiles[g->y][g->x].visible) {
                    g->stun_timer = 2;
                    hit++;
                }
            }
            for (int i = 0; i < drones->count; i++) {
                Drone *d = &drones->drones[i];
                if (map->tiles[d->y][d->x].visible) {
                    d->stun_timer = 2;
                    hit++;
                }
            }
            nr->emp_avail = false;
            snprintf(msg, 80, "EMP pulse -- %d unit%s stunned",
                     hit, hit == 1 ? "" : "s");
        }
        return true;

    case 'z': case 'Z':
        /* Spoof (NETRUNNER T4 opt1): flood network with false signals,
         * resetting all alert/hunting guards to unaware. Once per mission. */
        if (!nr->spoof_avail) return false;
        guard_erase_all(guards);
        nr->spoof_avail = false;
        snprintf(msg, 80, "SPOOF broadcast -- all guards reset to unaware");
        return true;

    case 'G':
        /* GOD MODE (NETRUNNER T5 opt0): full facility network control.
         * Stuns all guards 5 turns, opens door, unlocks extraction.
         * Once per mission; free action to trigger. */
        if (!nr->godmode_avail) return false;
        guard_stun_all(guards, 5);
        drone_stun_all(drones, 5);
        if (map->tiles[map->door_y][map->door_x].type == TILE_DOOR)
            map->tiles[map->door_y][map->door_x].type = TILE_FLOOR;
        if (map->tiles[map->stairs_y][map->stairs_x].type == TILE_STAIRS_LOCKED)
            map->tiles[map->stairs_y][map->stairs_x].type = TILE_STAIRS;
        nr->godmode_avail = false;
        nr->godmode_timer = 3;
        snprintf(msg, 80, "GOD MODE -- full network control, all systems compromised");
        return false;

    case 's': case 'S': {
        /* Suppressive Fire: CHROME tree (2), tier 2, option 0.
         * Lay down covering fire — pin all currently-visible guards for 2 turns.
         * Requires a ranged weapon; once per mission; costs a turn. */
        if (!cr->suppressive_avail) return false;
        if (player->equipped[EQUIP_WEAPON].range <= 0) return false;
        {
            int hit = 0;
            for (int i = 0; i < guards->count; i++) {
                Guard *g = &guards->guards[i];
                if (map->tiles[g->y][g->x].visible) {
                    g->stun_timer = 2;
                    hit++;
                }
            }
            cr->suppressive_avail = false;
            snprintf(msg, 80, "SUPPRESSIVE FIRE -- %d guard%s pinned",
                     hit, hit == 1 ? "" : "s");
        }
        return true;
    }

    case 'r': {
        /* Chrome Rush: CHROME tree (2), tier 3, option 0.
         * Charge 3 tiles in the last movement direction, dealing ATK damage
         * to any guard in the path and knocking them 1 tile back.
         * Player advances through dead or knocked-back guards. Once/mission. */
        if (!cr->chrome_rush_avail) return false;
        if (cr->last_dx == 0 && cr->last_dy == 0) return false;
        {
            int rdx  = cr->last_dx, rdy = cr->last_dy;
            int atk  = entity_gear_atk(player) + player->skills[SKILL_COMBAT];
            int hits = 0;

            for (int step = 0; step < 3; step++) {
                int nx = player->x + rdx;
                int ny = player->y + rdy;
                if (map_is_blocked(map, nx, ny)) break;  /* stopped by wall */

                if (guard_at(guards, nx, ny)) {
                    /* Strike the guard */
                    int before = guards->count;
                    Item drop = {0};
                    guard_player_attack(guards, nx, ny, atk,
                                        player->x, player->y, &drop);
                    if (item_valid(&drop))
                        entity_inv_add(player, drop);
                    hits++;

                    bool guard_dead = (guards->count < before);
                    bool advanced   = false;

                    if (guard_dead) {
                        /* Guard dead — advance through */
                        player->x = nx;
                        player->y = ny;
                        advanced  = true;
                    } else {
                        /* Guard alive — try to knock them back */
                        for (int i = 0; i < guards->count; i++) {
                            Guard *g = &guards->guards[i];
                            if (g->x != nx || g->y != ny) continue;
                            int kx = nx + rdx, ky = ny + rdy;
                            if (!map_is_blocked(map, kx, ky) &&
                                !guard_at(guards, kx, ky)) {
                                g->x = kx;
                                g->y = ky;
                                player->x = nx;
                                player->y = ny;
                                advanced = true;
                            }
                            break;
                        }
                    }
                    if (!advanced) break;  /* guard blocked the path */
                } else {
                    player->x = nx;
                    player->y = ny;
                }
            }
            cr->chrome_rush_avail = false;
            snprintf(msg, 80, "CHROME RUSH -- %d guard%s struck",
                     hits, hits == 1 ? "" : "s");
        }
        return true;
    }

    case 'R': {
        /* REAPER: CHROME tree (2), tier 4, option 0.
         * Activate REAPER mode — every kill this mission grants a free action
         * (guard tick skipped), chainable. Once per mission to activate. */
        if (!cr->reaper_avail || cr->reaper_active) return false;
        cr->reaper_avail  = false;
        cr->reaper_active = true;
        snprintf(msg, 80, "REAPER ACTIVATED -- kills grant bonus actions");
        return false;  /* free action to activate */
    }

    case 'f': {
        /* Ranged fire: requires an equipped weapon with range > 0.
         * Auto-aims at the nearest guard visible in the player's FOV.
         * Fires a 3-dart flechette burst with slight lateral spread.
         *
         * Iron Grip   (CHROME T1 opt2): +1 ATK on ranged attacks.
         * Precise Shot (CHROME T3 opt1): surviving target also stunned+disarmed. */
        if (player->equipped[EQUIP_WEAPON].range <= 0)
            return false;  /* melee weapon or empty slot — no shot */

        /* Find nearest visible enemy (guard or non-hacked drone) within weapon range */
        int   range  = player->equipped[EQUIP_WEAPON].range;
        int   best_d = range + 1;
        int   tgt_x  = -1, tgt_y = -1;
        for (int i = 0; i < guards->count; i++) {
            const Guard *g = &guards->guards[i];
            if (!map->tiles[g->y][g->x].visible) continue;
            int dx2 = g->x - player->x, dy2 = g->y - player->y;
            int d   = (abs(dx2) > abs(dy2)) ? abs(dx2) : abs(dy2);
            if (d < best_d) { best_d = d; tgt_x = g->x; tgt_y = g->y; }
        }
        for (int i = 0; i < drones->count; i++) {
            const Drone *dr = &drones->drones[i];
            if (dr->state == DRONE_HACKED) continue;  /* don't shoot allies */
            if (!map->tiles[dr->y][dr->x].visible) continue;
            int dx2 = dr->x - player->x, dy2 = dr->y - player->y;
            int d   = (abs(dx2) > abs(dy2)) ? abs(dx2) : abs(dy2);
            if (d < best_d) { best_d = d; tgt_x = dr->x; tgt_y = dr->y; }
        }
        if (tgt_x < 0) return false;  /* no visible target in range */

        int dmg = entity_gear_atk(player) + player->skills[SKILL_COMBAT];

        /* Iron Grip: CHROME (2), tier 0, opt 2 — steady aim gives +1 ATK */
        if (player->tree_bought[2][0] & (1 << 2))
            dmg += 1;

        /* Noise: flechettes are near-silent but not truly quiet */
        for (int i = 0; i < guards->count; i++) {
            Guard *g = &guards->guards[i];
            int dx2 = g->x - player->x, dy2 = g->y - player->y;
            int d   = (abs(dx2) > abs(dy2)) ? abs(dx2) : abs(dy2);
            if (d <= FLECHETTE_NOISE_RANGE && g->state == GUARD_UNAWARE)
                g->state = GUARD_SUSPICIOUS;
        }

        /* Create and animate the burst */
        ProjectileList pl;
        proj_clear(&pl);
        proj_flechette(&pl, player->x, player->y, tgt_x, tgt_y, dmg);

        Item drops[MAX_GUARDS];
        int  ndrop = 0;

        while (proj_step(&pl, map, guards, drones, player->x, player->y,
                         drops, &ndrop, NULL)) {
            erase();
            render_map(map);
            render_guards(guards, map, false);
            render_drones(drones, map, false);
            render_entity(player);
            render_projectiles(&pl);
            refresh();
            napms(55);
        }

        /* Collect loot from killed guards */
        for (int i = 0; i < ndrop; i++)
            entity_inv_add(player, drops[i]);

        /* Precise Shot: CHROME (2), tier 3, opt 1 — stun + disarm surviving target.
         * Applied after the burst: if the guard is still alive at (tgt_x, tgt_y),
         * freeze them 2 turns and disarm them 3 turns (called shot to limbs). */
        if (player->tree_bought[2][3] & (1 << 1)) {
            for (int i = 0; i < guards->count; i++) {
                Guard *g = &guards->guards[i];
                if (g->x == tgt_x && g->y == tgt_y) {
                    g->stun_timer  = 2;
                    g->disarm_timer = 3;
                    break;
                }
            }
        }

        return true;  /* costs a turn */
    }

    default:
        return false;
    }

    /* Record direction for Chrome Rush targeting */
    if (dx != 0 || dy != 0) {
        cr->last_dx = dx;
        cr->last_dy = dy;
    }

    int nx = player->x + dx;
    int ny = player->y + dy;

    if (guard_at(guards, nx, ny)) {
        /* Silent Takedown: SHADOW (0), tier 2, opt 0 — instant kill on unaware guard */
        bool silent_td = (player->tree_bought[0][2] & (1 << 0)) != 0 &&
                         guard_state_at(guards, nx, ny) == GUARD_UNAWARE;
        int dmg = silent_td ? GUARD_MAX_HP
                            : entity_gear_atk(player) + player->skills[SKILL_COMBAT];
        Item drop = {0};
        int count_before = guards->count;
        guard_player_attack(guards, nx, ny, dmg, player->x, player->y, &drop);
        if (item_valid(&drop))
            entity_inv_add(player, drop);
        /* CQC: CHROME tree (2), tier 1, option 1 — disarms guard for 3 turns */
        if (guards->count == count_before &&
                (player->tree_bought[2][1] & (1 << 1)))
            guard_disarm(guards, nx, ny, 3);
        /* Adrenaline: CHROME tree (2), tier 1, option 2 — kills restore 2 HP */
        if (guards->count < count_before &&
                (player->tree_bought[2][1] & (1 << 2))) {
            player->hp += 2;
            if (player->hp > player->max_hp)
                player->hp = player->max_hp;
        }
        /* Quick Draw: CHROME tree (2), tier 0, option 0 — first attack free */
        if ((player->tree_bought[2][0] & (1 << 0)) && *quick_draw_avail) {
            *quick_draw_avail = false;
            return false;  /* free action */
        }
    } else if (drone_at(drones, nx, ny)) {
        /* Melee a drone: close the gap and smash it */
        int dmg = entity_gear_atk(player) + player->skills[SKILL_COMBAT];
        Item drop = {0};
        int drones_before = drones->count;
        drone_player_attack(drones, nx, ny, dmg, &drop);
        /* Adrenaline: kill restores 2 HP */
        if (drones->count < drones_before &&
                (player->tree_bought[2][1] & (1 << 2))) {
            player->hp += 2;
            if (player->hp > player->max_hp)
                player->hp = player->max_hp;
        }
        /* Quick Draw: first attack free */
        if ((player->tree_bought[2][0] & (1 << 0)) && *quick_draw_avail) {
            *quick_draw_avail = false;
            return false;
        }
    } else {
        entity_move(player, map, dx, dy);
    }
    return true;
}

/* Self-contained playing loop. Returns the next GameState. */
static GameState game_run(Map *map, Entity *player)
{
    static GuardList guards;
    static DroneList drones;

    /* Difficulty 1-5 stored directly from mission selection */
    int difficulty = player->active_mission_difficulty;
    if (difficulty < 1) difficulty = 1;
    if (difficulty > 5) difficulty = 5;

    /* Number of floors scales with difficulty: diff 0-1 → 1F, 2-3 → 2F, 4-5 → 3F */
    int num_floors = 1 + difficulty / 2;
    if (num_floors > 3) num_floors = 3;

    /* Skill-derived constants, recomputed each mission start */
    /* Light Step: SHADOW tree (0), tier 0, option 0 — doubles suspicion threshold */
    /* Keen Eyes:  SHADOW tree (0), tier 0, option 2 — extends FOV radius */
#define HAS_SKILL(tree, tier, opt) \
    ((player->tree_bought[(tree)][(tier)] & (1 << (opt))) != 0)

    int fov_radius      = HAS_SKILL(0, 0, 2) ? FOV_RADIUS_KEEN : FOV_RADIUS;
    int suspicion_max   = HAS_SKILL(0, 0, 0) ? GUARD_SUSPICION_MAX * 2
                                              : GUARD_SUSPICION_MAX;
    /* Grey Man: SHADOW (0), tier 1, opt 1 — suspicion decays twice as fast */
    int suspicion_decay = HAS_SKILL(0, 1, 1) ? 2 : 1;
    /* Vanish: SHADOW (0), tier 3, opt 0 — instant de-aggro on LOS break */
    bool vanish = HAS_SKILL(0, 3, 0);

    /* Shadow Meld: SHADOW (0), tier 1, opt 0 — standing still = invisible.
     * Player position is compared before/after each turn to detect movement. */
    bool shadow_meld = HAS_SKILL(0, 1, 0);

    /* Predict Patrol: SHADOW (0), tier 2, opt 1 — see guard patrol waypoints. */
    bool predict_patrol = HAS_SKILL(0, 2, 1);

    /* Disguise Master: SHADOW (0), tier 3, opt 1 — guards take much longer to
     * notice the player (simulates blending in as maintenance crew). +4 to the
     * suspicion threshold on top of any Light Step multiplier. */
    if (HAS_SKILL(0, 3, 1))
        suspicion_max += 4;

    /* Cat Fall: SHADOW (0), tier 3, opt 2 — silent movement; crouched detection
     * range is reduced by 1 additional point (stacks with Wall Hug). */
    bool cat_fall = HAS_SKILL(0, 3, 2);

    /* Quick Draw: CHROME tree (2), tier 0, option 0.
     * Starts available; consumed on first attack; re-arms when all guards
     * return to UNAWARE (i.e. between distinct encounters). */
    bool quick_draw_avail = HAS_SKILL(2, 0, 0);

    /* Bullet Time: CHROME tree (2), tier 1, option 0. Once per mission. */
    bool bt_avail = HAS_SKILL(2, 1, 0);

    /* Second Wind: CHROME tree (2), tier 3, option 2. Once per mission.
     * Auto-revives from lethal damage with 1 HP. */
    bool sw_avail = HAS_SKILL(2, 3, 2);

    /* Ghost Protocol: SHADOW tree (0), tier 4, option 0. Once per mission. */
    bool gp_avail = HAS_SKILL(0, 4, 0);

    /* Counterattack: CHROME (2), tier 2, opt 1 — passive.
     * When a hunting guard deals melee damage, player auto-strikes them back. */
    bool counterattack = HAS_SKILL(2, 2, 1);

    /* Pain Editor: CHROME (2), tier 2, opt 2 — passive.
     * Without it: being below half HP makes the player harder to miss (+2 det).
     * Pain Editor negates this wound penalty entirely. */
    bool pain_editor = HAS_SKILL(2, 2, 2);

    /* Chrome active abilities (once per mission) */
    ChromeState cr;
    cr.suppressive_avail = HAS_SKILL(2, 2, 0);  /* Suppressive Fire */
    cr.chrome_rush_avail = HAS_SKILL(2, 3, 0);  /* Chrome Rush       */
    cr.reaper_avail      = HAS_SKILL(2, 4, 0);  /* REAPER            */
    cr.reaper_active     = false;
    cr.last_dx           = 0;
    cr.last_dy           = 0;

    /* --- Netrunner skill state (once per mission) --- */
    NetrunnerState nr;
    nr.scavenger       = HAS_SKILL(1, 0, 1);
    nr.deep_scan       = HAS_SKILL(1, 0, 2);
    nr.overclock_avail = HAS_SKILL(1, 1, 2);
    nr.overclock_armed = false;
    nr.drone_buddy     = HAS_SKILL(1, 2, 1);
    nr.daemon_avail    = HAS_SKILL(1, 2, 0);
    nr.emp_avail       = HAS_SKILL(1, 2, 2);
    nr.backdoor_king   = HAS_SKILL(1, 3, 0);
    nr.spoof_avail     = HAS_SKILL(1, 3, 1);
    nr.remote_det      = HAS_SKILL(1, 3, 2);
    nr.godmode_avail   = HAS_SKILL(1, 4, 0);
    nr.godmode_timer   = 0;
    /* Traps reset per floor (map regenerates, old positions invalid) */
    nr.trap_avail  = HAS_SKILL(1, 1, 1);
    nr.trap_count  = 0;

    bool done = false;
    int current_floor = 1;

    while (!done) {  /* floor loop: iterate until mission complete or abort */
        map_generate(map);
        entity_place(player, map);
        guard_spawn(&guards, map, difficulty);
        drone_spawn(&drones, map, difficulty);

        player->crouched = false;

        /* Per-floor resets: vent positions change with new map */
        bool vent_used = false;
        bool bt_active = false;
        int  gp_timer  = 0;
        nr.trap_count  = 0;
        nr.trap_avail  = HAS_SKILL(1, 1, 1);

        /* Deep Scan: pre-reveal terminals, security door, and stairs on map */
        if (nr.deep_scan) {
            for (int i = 0; i < map->num_terminals; i++)
                map->tiles[map->terminals[i][1]][map->terminals[i][0]].seen = true;
            if (map->door_x > 0 || map->door_y > 0)
                map->tiles[map->door_y][map->door_x].seen = true;
            map->tiles[map->stairs_y][map->stairs_x].seen = true;
        }

        fov_compute(map, player->x, player->y, fov_radius);

        char game_msg[80] = "";
        bool next_floor = false;

        while (!done && !next_floor) {
            erase();
            render_map(map);
            render_guards(&guards, map, nr.drone_buddy);
            render_drones(&drones, map, nr.drone_buddy);
            render_entity(player);

            /* Predict Patrol: SHADOW (0), tier 2, opt 1 — overlay guard patrol
             * waypoints on the map as dim '+' markers on explored tiles. */
            if (predict_patrol) {
                for (int i = 0; i < guards.count; i++) {
                    const Guard *g = &guards.guards[i];
                    for (int wp = 0; wp < 2; wp++) {
                        int wx = g->patrol[wp][0], wy = g->patrol[wp][1];
                        if (map->tiles[wy][wx].visible || map->tiles[wy][wx].seen) {
                            attron(COLOR_PAIR(COL_MENU_DIM));
                            mvaddch(wy, wx, '+');
                            attroff(COLOR_PAIR(COL_MENU_DIM));
                        }
                    }
                }
            }

            /* Trap Master: show placed traps as yellow ^ if visible */
            for (int t = 0; t < nr.trap_count; t++) {
                if (map->tiles[nr.trap_y[t]][nr.trap_x[t]].visible) {
                    attron(COLOR_PAIR(COL_GUARD_SUSPICIOUS) | A_BOLD);
                    mvaddch(nr.trap_y[t], nr.trap_x[t], '^');
                    attroff(COLOR_PAIR(COL_GUARD_SUSPICIOUS) | A_BOLD);
                }
            }

            bool door_locked    = (map->tiles[map->door_y][map->door_x].type == TILE_DOOR);
            bool stairs_locked  = (map->tiles[map->stairs_y][map->stairs_x].type == TILE_STAIRS_LOCKED);
            render_status(player, &guards, &drones, bt_avail, bt_active, gp_avail, gp_timer,
                          door_locked, stairs_locked, current_floor, num_floors,
                          &nr, &cr, game_msg);
            refresh();
            game_msg[0] = '\0';  /* clear after one displayed frame */

            /* Snapshot position and enemy counts before input for post-turn analysis */
            int old_px = player->x, old_py = player->y;
            int guards_before = guards.count;
            int drones_before = drones.count;

            int ch = getch();
            bool turn_used = handle_input(ch, player, map, &guards, &drones,
                                          &quick_draw_avail, &vent_used,
                                          &bt_avail, &bt_active,
                                          &gp_avail, &gp_timer,
                                          &done, game_msg, &nr, &cr);

            if (done)
                break;

            bool player_moved  = (player->x != old_px || player->y != old_py);
            bool killed_enemy  = (guards.count < guards_before ||
                                  drones.count < drones_before);

            /* REAPER: CHROME (2), tier 4, opt 0 — each kill grants a bonus action. */
            if (cr.reaper_active && killed_enemy)
                bt_active = true;

            /* Guards only act when the player used a turn */
            if (turn_used) {
                /* Bullet Time: CHROME (2), tier 1, opt 0 — first action after
                 * activation skips the guard tick, giving 2 actions in 1 turn. */
                if (bt_active) {
                    bt_active = false;  /* consume the free action; guards sit still */
                } else {
                    /* Ghost Protocol: SHADOW (0), tier 4, opt 0 — invisible to guards */
                    int det;
                    if (gp_timer > 0) {
                        det = 0;
                        gp_timer--;
                    } else {
                        det = guard_detection_range(player->crouched,
                                                    player->skills[SKILL_STEALTH]);
                        /* Shadow Meld: SHADOW (0), tier 1, opt 0 — standing still = 0 det */
                        if (shadow_meld && !player_moved)
                            det = 0;
                        /* Wall Hug: SHADOW (0), tier 1, opt 2 — adjacent wall -1 range */
                        if (HAS_SKILL(0, 1, 2) && near_wall(map, player->x, player->y))
                            det = det > 1 ? det - 1 : 1;
                        /* Cat Fall: SHADOW (0), tier 3, opt 2 — crouched -1 range */
                        if (cat_fall && player->crouched)
                            det = det > 1 ? det - 1 : 1;
                        /* Pain Editor: CHROME (2), tier 2, opt 2 — wound penalty.
                         * Below half HP: bleeding/laboured breathing gives +2 detection.
                         * Pain Editor negates this entirely. */
                        if (!pain_editor && player->hp > 0 &&
                                player->hp < player->max_hp / 2)
                            det += 2;
                    }
                    int dmg = guard_tick_all(&guards, map, player->x, player->y,
                                             det, suspicion_max, suspicion_decay,
                                             vanish);
                    /* Drones tick after guards: shots are queued as animated projectiles */
                    {
                        ProjectileList drone_pl;
                        proj_clear(&drone_pl);
                        drone_tick_all(&drones, map, &guards, &drone_pl,
                                       player->x, player->y);

                        if (drone_pl.count > 0) {
                            Item drone_drops[MAX_GUARDS];
                            int  drone_ndrop = 0;
                            int  drone_pdmg  = 0;
                            while (proj_step(&drone_pl, map, &guards, &drones,
                                             player->x, player->y,
                                             drone_drops, &drone_ndrop, &drone_pdmg)) {
                                erase();
                                render_map(map);
                                render_guards(&guards, map, nr.drone_buddy);
                                render_drones(&drones, map, nr.drone_buddy);
                                render_entity(player);
                                render_projectiles(&drone_pl);
                                refresh();
                                napms(55);
                            }
                            player->hp -= drone_pdmg;
                            if (drone_pdmg > 0 && game_msg[0] == '\0')
                                snprintf(game_msg, 80, "DRONE HIT -- %d damage!", drone_pdmg);
                            /* Loot from guards killed by hacked-drone shots */
                            for (int i = 0; i < drone_ndrop; i++)
                                if (item_valid(&drone_drops[i]))
                                    entity_inv_add(player, drone_drops[i]);
                        }
                    }
                    player->hp -= dmg;
                    /* Second Wind: CHROME (2), tier 3, opt 2 — survive lethal hit once */
                    if (player->hp <= 0 && sw_avail) {
                        player->hp = 1;
                        sw_avail = false;
                    }

                    /* Counterattack: CHROME (2), tier 2, opt 1 — auto-strike on hit.
                     * Find the adjacent hunting guard that dealt damage and counter them. */
                    if (dmg > 0 && counterattack) {
                        for (int i = 0; i < guards.count; i++) {
                            Guard *g = &guards.guards[i];
                            if (g->state != GUARD_HUNTING) continue;
                            int cdx = g->x - player->x;
                            int cdy = g->y - player->y;
                            if (cdx < 0) cdx = -cdx;
                            if (cdy < 0) cdy = -cdy;
                            if ((cdx > cdy ? cdx : cdy) == 1) {
                                int cnt_dmg = entity_gear_atk(player) +
                                              player->skills[SKILL_COMBAT];
                                Item cnt_drop = {0};
                                guard_player_attack(&guards, g->x, g->y, cnt_dmg,
                                                    player->x, player->y, &cnt_drop);
                                if (item_valid(&cnt_drop))
                                    entity_inv_add(player, cnt_drop);
                                snprintf(game_msg, 80, "COUNTERATTACK! (%d dmg)", cnt_dmg);
                                break;
                            }
                        }
                    }

                    /* Trap Master: stun guards that walked onto a trap tile */
                    for (int i = 0; i < guards.count; i++) {
                        for (int t = 0; t < nr.trap_count; t++) {
                            if (guards.guards[i].x == nr.trap_x[t] &&
                                guards.guards[i].y == nr.trap_y[t]) {
                                guards.guards[i].stun_timer = 3;
                                /* Remove trap (swap with last) */
                                nr.trap_x[t] = nr.trap_x[nr.trap_count - 1];
                                nr.trap_y[t] = nr.trap_y[nr.trap_count - 1];
                                nr.trap_count--;
                                t--;
                            }
                        }
                    }

                    /* GOD MODE timer countdown */
                    if (nr.godmode_timer > 0)
                        nr.godmode_timer--;
                    if (player->hp <= 0)
                        return GAME_STATE_DEAD;

                    /* Re-arm Quick Draw when the area goes quiet (all unaware/patrol) */
                    if (!quick_draw_avail && HAS_SKILL(2, 0, 0) &&
                            guard_max_state(&guards) == GUARD_UNAWARE &&
                            !drone_any_alert(&drones))
                        quick_draw_avail = true;
                }
            }

            fov_compute(map, player->x, player->y, fov_radius);

            /* Check if player is standing on an unlocked stairway */
            if (map->tiles[player->y][player->x].type == TILE_STAIRS) {
                if (current_floor >= num_floors) {
                    /* Final floor: extract — mission complete */
                    item_loot_mission(difficulty,
                                      player->pending_loot,
                                      &player->pending_loot_count);
                    return GAME_STATE_DEBRIEF;
                } else {
                    /* Intermediate floor: descend to next level */
                    snprintf(game_msg, 80, "DESCENDING -- floor %d of %d",
                             current_floor + 1, num_floors);
                    next_floor = true;
                }
            }
        }  /* end inner game loop */

        if (next_floor)
            current_floor++;
        /* if done=true, exit floor loop and return to hub */
    }  /* end floor loop */

    return GAME_STATE_HUB;
}

int main(void)
{
    srand((unsigned)time(NULL));

    static Map    map;
    static Entity player;

    ncurses_init();

    entity_init(&player);

    GameState state = GAME_STATE_HUB;
    while (state != GAME_STATE_QUIT) {
        switch (state) {
        case GAME_STATE_HUB:     state = hub_run(&player);         break;
        case GAME_STATE_PLAYING:  state = game_run(&map, &player);  break;
        case GAME_STATE_DEBRIEF:  state = debrief_run(&player);     break;
        case GAME_STATE_DEAD:     state = death_run(&player);       break;
        default:                  state = GAME_STATE_QUIT;          break;
        }
    }

    ncurses_cleanup();
    return 0;
}
