#pragma once

#include "map.h"

#define FOV_RADIUS      10   /* default player vision radius */
#define FOV_RADIUS_KEEN 14   /* radius with Keen Eyes (SHADOW T1) */

/* Recompute which tiles are visible from (ox, oy) within radius.
 * Clears all visible flags first, then marks newly visible tiles.
 * Also sets seen=true on any tile that becomes visible. */
void fov_compute(Map *map, int ox, int oy, int radius);
