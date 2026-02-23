#pragma once

#include <stdbool.h>

#define MAP_WIDTH  80
#define MAP_HEIGHT 40

#define MAX_ROOMS     15
#define ROOM_MIN_SIZE  4
#define ROOM_MAX_SIZE 10

typedef enum {
    TILE_WALL,
    TILE_FLOOR,
    TILE_STAIRS,
    TILE_STAIRS_LOCKED, /* extraction: hidden as floor until terminal unlocked */
    TILE_VENT,
    TILE_TERMINAL,      /* hackable network terminal */
    TILE_DOOR,          /* security door: blocks movement and sight */
} TileType;

typedef enum {
    HACK_OPEN_DOOR = 0, /* opens the security door blocking access */
    HACK_UNLOCK    = 1, /* reveals and activates the extraction point */
} HackEffect;

#define MAX_TERMINALS 2

typedef struct {
    TileType type;
    bool seen;     /* ever revealed to player */
    bool visible;  /* currently in FOV */
} Tile;

typedef struct {
    int x, y, w, h;
} Room;

typedef struct {
    Tile  tiles[MAP_HEIGHT][MAP_WIDTH];
    Room  rooms[MAX_ROOMS];
    int   num_rooms;
    int   vents[2][2];  /* vents[i] = {x, y}; two connected vent openings */
    int   num_vents;    /* 0 or 2 */
    int   terminals[MAX_TERMINALS][2]; /* terminals[i] = {x, y} */
    int   terminal_security[MAX_TERMINALS]; /* difficulty 1-2 */
    int   terminal_effects[MAX_TERMINALS];  /* HackEffect value */
    int   num_terminals;
    int   door_x, door_y;     /* location of TILE_DOOR (0,0 if none placed) */
    int   stairs_x, stairs_y; /* location of extraction (locked or active) */
} Map;

void map_generate(Map *map);

/* Returns true if the tile blocks movement */
bool map_is_blocked(const Map *map, int x, int y);

/* Returns true if the tile blocks line-of-sight */
bool map_blocks_sight(const Map *map, int x, int y);
