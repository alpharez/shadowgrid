/* Recursive shadowcasting FOV
 * Based on the algorithm by Björn Bergström:
 * http://www.roguebasin.com/index.php/FOV_using_recursive_shadowcasting
 */
#include "fov.h"

#include <stdlib.h>

/* Transformation matrices for each of the 8 octants */
static const int mult[4][8] = {
    { 1,  0,  0, -1, -1,  0,  0,  1 },
    { 0,  1, -1,  0,  0, -1,  1,  0 },
    { 0,  1,  1,  0,  0, -1, -1,  0 },
    { 1,  0,  0,  1, -1,  0,  0, -1 },
};

static void cast_light(Map *map, int cx, int cy,
                        int row, float start, float end,
                        int radius, int xx, int xy, int yx, int yy)
{
    if (start < end)
        return;

    float new_start = 0.0f;
    bool blocked = false;

    for (int dist = row; dist <= radius && !blocked; dist++) {
        int dy = -dist;
        for (int dx = -dist; dx <= 0; dx++) {
            int mx = cx + dx * xx + dy * xy;
            int my = cy + dx * yx + dy * yy;

            float l_slope = (dx - 0.5f) / (dy + 0.5f);
            float r_slope = (dx + 0.5f) / (dy - 0.5f);

            if (start < r_slope)
                continue;
            if (end > l_slope)
                break;

            if (dx * dx + dy * dy <= radius * radius) {
                if (mx >= 0 && mx < MAP_WIDTH && my >= 0 && my < MAP_HEIGHT) {
                    map->tiles[my][mx].visible = true;
                    map->tiles[my][mx].seen    = true;
                }
            }

            if (blocked) {
                if (map_blocks_sight(map, mx, my)) {
                    new_start = r_slope;
                } else {
                    blocked = false;
                    start   = new_start;
                }
            } else {
                if (map_blocks_sight(map, mx, my) && dist < radius) {
                    blocked = true;
                    cast_light(map, cx, cy, dist + 1, start, l_slope,
                               radius, xx, xy, yx, yy);
                    new_start = r_slope;
                }
            }
        }
    }
}

void fov_compute(Map *map, int ox, int oy, int radius)
{
    /* Clear all visible flags */
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++)
            map->tiles[y][x].visible = false;

    /* Mark origin visible */
    if (ox >= 0 && ox < MAP_WIDTH && oy >= 0 && oy < MAP_HEIGHT) {
        map->tiles[oy][ox].visible = true;
        map->tiles[oy][ox].seen    = true;
    }

    for (int oct = 0; oct < 8; oct++) {
        cast_light(map, ox, oy, 1, 1.0f, 0.0f, radius,
                   mult[0][oct], mult[1][oct],
                   mult[2][oct], mult[3][oct]);
    }
}
