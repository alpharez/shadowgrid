#pragma once

#include "map.h"
#include "entity.h"
#include "guard.h"
#include "drone.h"
#include "projectile.h"
#include "netrunner.h"
#include "chrome.h"

/* Colour pair indices (defined in render.c, initialised by render_init) */
#define COL_FLOOR_VIS   1  /* bright floor */
#define COL_FLOOR_MEM   2  /* remembered floor (dim) */
#define COL_WALL_VIS    3  /* bright wall */
#define COL_WALL_MEM    4  /* remembered wall (dim) */
#define COL_PLAYER      5  /* player @ */
#define COL_STATUS      6  /* status bar */
#define COL_STAIRS      7  /* extraction point > */
#define COL_MENU_SEL    8  /* selected menu item */
#define COL_MENU_DIM    9  /* unselected menu item */
#define COL_MENU_TITLE  10 /* box titles / headers */

/* Item rarity colours (used in inventory / equipment screens) */
#define COL_RARITY_COMMON    11  /* gray    */
#define COL_RARITY_UNCOMMON  12  /* green   */
#define COL_RARITY_RARE      13  /* blue    */
#define COL_RARITY_EPIC      14  /* magenta */
#define COL_RARITY_LEGENDARY 15  /* yellow  */

/* Guard colours */
#define COL_GUARD            16  /* unaware guard (gray) */
#define COL_GUARD_SUSPICIOUS 17  /* suspicious guard (yellow) */
#define COL_GUARD_ALERT      18  /* alert / hunting guard (red) */

/* Vent colour */
#define COL_VENT             19  /* vent opening (teal) */

/* Hacking colours */
#define COL_TERMINAL         20  /* hackable terminal (magenta) */
#define COL_GUARD_STUNNED    21  /* stunned guard (dark gray) */
#define COL_DOOR             22  /* security door (amber) */

/* Projectile colour */
#define COL_PROJECTILE       23  /* flying dart / needle (hot white) */

/* Elite guard colour */
#define COL_GUARD_ELITE      24  /* unaware elite guard (steel blue) */

/* Drone colours */
#define COL_DRONE            25  /* patrolling drone (violet) */
#define COL_DRONE_HACKED     26  /* hacked drone (green, friendly) */

void render_init(void);
void render_map(const Map *map);
void render_entity(const Entity *e);
/* scan_active: show all guards/drones through walls (network scan effect) */
void render_guards(const GuardList *gl, const Map *map, bool scan_active);
/* Draw drones; scan_active reveals them through walls like guards. */
void render_drones(const DroneList *dl, const Map *map, bool scan_active);
/* Draw all active projectiles over the current frame. */
void render_projectiles(const ProjectileList *pl);
/* bt_avail: Bullet Time ready to use; bt_active: extra action in progress.
 * gp_timer: turns remaining on Ghost Protocol (0 = inactive).
 * door_locked / stairs_locked: mission objective state for status display.
 * current_floor / num_floors: floor progress (num_floors>1 shows FL indicator).
 * msg: one-turn feedback message (empty string = none). */
void render_status(const Entity *e, const GuardList *gl,
                   const DroneList *dl,
                   bool bt_avail, bool bt_active,
                   bool gp_avail, int gp_timer,
                   bool door_locked, bool stairs_locked,
                   int current_floor, int num_floors,
                   const NetrunnerState *nr,
                   const ChromeState *cr,
                   const char *msg);
