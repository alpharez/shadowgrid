#pragma once

#include "entity.h"

/* Forward-declare GameState so hub.h doesn't need main's definition.
 * main.c must ensure the values match. */
typedef enum {
    GAME_STATE_HUB,
    GAME_STATE_PLAYING,
    GAME_STATE_DEBRIEF,
    GAME_STATE_DEAD,
    GAME_STATE_QUIT,
} GameState;

/* Run the hub loop. Returns the next GameState. */
GameState hub_run(Entity *player);

/* Run the debrief screen. Awards mission reward and returns GAME_STATE_HUB. */
GameState debrief_run(Entity *player);

/* Run the death screen. Returns GAME_STATE_HUB (new game) or GAME_STATE_QUIT. */
GameState death_run(Entity *player);

/* Reset shop sold-flags for a new game. */
void hub_shop_reset(void);

/* Open the skill tree overlay (full 3-tree picker) mid-mission or at hub. */
void hub_levelup_menu(Entity *player);

/* In-mission inventory overlay: browse backpack, equip items.
 * Draws over the current map frame; restores stdscr on close. */
void mission_inventory(Entity *player);
