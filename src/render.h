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

/* Hub NPC colours */
#define COL_HUB_ARMS         27  /* arms dealer (red)     */
#define COL_HUB_RIPPERDOC    28  /* ripperdoc  (cyan)     */
#define COL_HUB_FIXER        29  /* fixer      (yellow)   */
#define COL_HUB_TECH         30  /* tech dealer (blue)    */
#define COL_HUB_FENCE        31  /* fence      (magenta)  */
#define COL_HUB_MEDIC        32  /* street doc (green)    */
#define COL_HUB_AMBIENT      33  /* ambient NPC (gray)    */

void render_init(void);
/* cam_x/cam_y: top-left world tile of the viewport (computed by game loop). */
void render_map(const Map *map, int cam_x, int cam_y);
void render_entity(const Entity *e, int cam_x, int cam_y);
/* scan_active: show all guards/drones through walls (network scan effect) */
void render_guards(const GuardList *gl, const Map *map, bool scan_active,
                   int cam_x, int cam_y);
/* Draw drones; scan_active reveals them through walls like guards. */
void render_drones(const DroneList *dl, const Map *map, bool scan_active,
                   int cam_x, int cam_y);
/* Draw all active projectiles over the current frame. */
void render_projectiles(const ProjectileList *pl, int cam_x, int cam_y);
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
