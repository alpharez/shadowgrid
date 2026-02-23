#include "map.h"

#include <stdlib.h>
#include <string.h>

static void fill_rect(Map *map, int x, int y, int w, int h, TileType type)
{
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            if (col >= 0 && col < MAP_WIDTH && row >= 0 && row < MAP_HEIGHT)
                map->tiles[row][col].type = type;
        }
    }
}

static void carve_h_tunnel(Map *map, int x1, int x2, int y)
{
    int start = x1 < x2 ? x1 : x2;
    int end   = x1 < x2 ? x2 : x1;
    for (int x = start; x <= end; x++)
        fill_rect(map, x, y, 1, 1, TILE_FLOOR);
}

static void carve_v_tunnel(Map *map, int y1, int y2, int x)
{
    int start = y1 < y2 ? y1 : y2;
    int end   = y1 < y2 ? y2 : y1;
    for (int y = start; y <= end; y++)
        fill_rect(map, x, y, 1, 1, TILE_FLOOR);
}

static bool rooms_overlap(const Room *a, const Room *b)
{
    return a->x <= b->x + b->w && a->x + a->w >= b->x &&
           a->y <= b->y + b->h && a->y + a->h >= b->y;
}

static int room_center_x(const Room *r) { return r->x + r->w / 2; }
static int room_center_y(const Room *r) { return r->y + r->h / 2; }

void map_generate(Map *map)
{
    /* Start with all walls */
    memset(map, 0, sizeof(*map));

    int last_cx = 0, last_cy = 0;  /* corner of last corridor placed */

    int attempts = 0;
    while (map->num_rooms < MAX_ROOMS && attempts < 200) {
        attempts++;

        int w = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE + 1);
        int h = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE + 1);
        int x = 1 + rand() % (MAP_WIDTH  - w - 2);
        int y = 1 + rand() % (MAP_HEIGHT - h - 2);

        Room candidate = { x, y, w, h };

        bool overlaps = false;
        for (int i = 0; i < map->num_rooms; i++) {
            /* Add a 1-tile buffer so rooms don't share walls */
            Room expanded = {
                map->rooms[i].x - 1,
                map->rooms[i].y - 1,
                map->rooms[i].w + 2,
                map->rooms[i].h + 2
            };
            if (rooms_overlap(&candidate, &expanded)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps)
            continue;

        fill_rect(map, x, y, w, h, TILE_FLOOR);

        if (map->num_rooms > 0) {
            /* Connect to previous room with L-shaped corridor */
            const Room *prev = &map->rooms[map->num_rooms - 1];
            int cx = room_center_x(&candidate);
            int cy = room_center_y(&candidate);
            int px = room_center_x(prev);
            int py = room_center_y(prev);

            if (rand() % 2) {
                carve_h_tunnel(map, px, cx, py);
                carve_v_tunnel(map, py, cy, cx);
                last_cx = cx; last_cy = py;
            } else {
                carve_v_tunnel(map, py, cy, px);
                carve_h_tunnel(map, px, cx, cy);
                last_cx = px; last_cy = cy;
            }
        }

        map->rooms[map->num_rooms++] = candidate;
    }

    /* Place extraction stairs (locked) in the center of the last room */
    if (map->num_rooms > 0) {
        const Room *last = &map->rooms[map->num_rooms - 1];
        int sx = room_center_x(last);
        int sy = room_center_y(last);
        map->tiles[sy][sx].type = TILE_STAIRS_LOCKED;
        map->stairs_x = sx;
        map->stairs_y = sy;
    }

    /* Place two hackable terminals at ~1/4 and ~1/2 through the room list.
     * Terminal 0 opens the security door; terminal 1 unlocks extraction.
     * Offset left of center (vents sit at center+1) to avoid overlap. */
    if (map->num_rooms >= 4) {
        int r1 = map->num_rooms / 4;
        int r2 = map->num_rooms / 2;
        const Room *ra = &map->rooms[r1];
        const Room *rb = &map->rooms[r2];
        int tx1 = room_center_x(ra) - 1;
        int ty1 = room_center_y(ra);
        int tx2 = room_center_x(rb) - 1;
        int ty2 = room_center_y(rb);
        map->tiles[ty1][tx1].type = TILE_TERMINAL;
        map->tiles[ty2][tx2].type = TILE_TERMINAL;
        map->terminals[0][0] = tx1;  map->terminals[0][1] = ty1;
        map->terminals[1][0] = tx2;  map->terminals[1][1] = ty2;
        map->terminal_security[0] = 1 + rand() % 2;  /* 1-2 */
        map->terminal_security[1] = 1 + rand() % 2;
        map->terminal_effects[0]  = HACK_OPEN_DOOR;
        map->terminal_effects[1]  = HACK_UNLOCK;
        map->num_terminals = 2;

        /* Place security door at the last corridor corner, guarding the
         * path to the extraction room.  Only place on a plain floor tile
         * that isn't the stairs position. */
        if (map->tiles[last_cy][last_cx].type == TILE_FLOOR &&
                !(last_cx == map->stairs_x && last_cy == map->stairs_y)) {
            map->tiles[last_cy][last_cx].type = TILE_DOOR;
            map->door_x = last_cx;
            map->door_y = last_cy;
        }
    }

    /* Place two vent openings at ~1/3 and ~2/3 through the room list,
     * skipping rooms[0] (player start) and rooms[num_rooms-1] (stairs). */
    if (map->num_rooms >= 4) {
        int r1 = map->num_rooms / 3;
        int r2 = (map->num_rooms * 2) / 3;
        /* Offset one tile from center so they don't overlap stairs/guard */
        const Room *ra = &map->rooms[r1];
        const Room *rb = &map->rooms[r2];
        int vx1 = room_center_x(ra) + 1;
        int vy1 = room_center_y(ra);
        int vx2 = room_center_x(rb) + 1;
        int vy2 = room_center_y(rb);
        map->tiles[vy1][vx1].type = TILE_VENT;
        map->tiles[vy2][vx2].type = TILE_VENT;
        map->vents[0][0] = vx1;  map->vents[0][1] = vy1;
        map->vents[1][0] = vx2;  map->vents[1][1] = vy2;
        map->num_vents = 2;
    }
}

bool map_is_blocked(const Map *map, int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return true;
    TileType t = map->tiles[y][x].type;
    return t == TILE_WALL || t == TILE_DOOR;
}

bool map_blocks_sight(const Map *map, int x, int y)
{
    return map_is_blocked(map, x, y);
}
