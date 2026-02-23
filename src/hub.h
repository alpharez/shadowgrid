#pragma once

#include "entity.h"

/* Forward-declare GameState so hub.h doesn't need main's definition.
 * main.c must ensure the values match. */
typedef enum {
    GAME_STATE_HUB,
    GAME_STATE_PLAYING,
    GAME_STATE_DEBRIEF,
    GAME_STATE_QUIT,
} GameState;

/* Run the hub loop. Returns the next GameState. */
GameState hub_run(Entity *player);

/* Run the debrief screen. Awards mission reward and returns GAME_STATE_HUB. */
GameState debrief_run(Entity *player);

/* In-mission inventory overlay: browse backpack, equip items.
 * Draws over the current map frame; restores stdscr on close. */
void mission_inventory(Entity *player);
