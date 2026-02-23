#include "render.h"

#include <ncurses.h>

void render_init(void)
{
    start_color();
    use_default_colors();

    if (COLORS >= 256) {
        /*
         * Cyberpunk 256-color palette (xterm-256 indices):
         *   51  = #00d7d7  neon cyan         (visible floor)
         *   24  = #005f87  dark steel blue   (explored/dark floor)
         *  252  = #d0d0d0  light gray        (visible wall)
         *  237  = #3a3a3a  very dark gray    (remembered wall)
         *  226  = #ffff00  bright yellow     (player)
         *   39  = #00afff  electric blue     (status bar)
         *   46  = #00ff00  bright green      (extraction stairs)
         *  240  = #585858  medium gray       (dim menu item)
         */
        init_pair(COL_FLOOR_VIS,  51,  -1);
        init_pair(COL_FLOOR_MEM,  24,  -1);
        init_pair(COL_WALL_VIS,  252,  -1);
        init_pair(COL_WALL_MEM,  237,  -1);
        init_pair(COL_PLAYER,    226,  -1);
        init_pair(COL_STATUS,     39,  -1);
        init_pair(COL_STAIRS,     46,  -1);
        init_pair(COL_MENU_SEL,  226,  -1);
        init_pair(COL_MENU_DIM,  240,  -1);
        init_pair(COL_MENU_TITLE,       39,  -1);
        /* Rarity colours (256-color): gray / green / blue / magenta / yellow */
        init_pair(COL_RARITY_COMMON,   245,  -1);
        init_pair(COL_RARITY_UNCOMMON,  46,  -1);
        init_pair(COL_RARITY_RARE,      39,  -1);
        init_pair(COL_RARITY_EPIC,     129,  -1);
        init_pair(COL_RARITY_LEGENDARY,226,  -1);
        /* Guard colours */
        init_pair(COL_GUARD,           252,  -1);  /* light gray */
        init_pair(COL_GUARD_SUSPICIOUS,226,  -1);  /* yellow */
        init_pair(COL_GUARD_ALERT,     196,  -1);  /* bright red */
        /* Vent colour: xterm-256 #00afff teal */
        init_pair(COL_VENT,             45,  -1);
        /* Hacking colours */
        init_pair(COL_TERMINAL,        129,  -1);  /* xterm-256 magenta */
        init_pair(COL_GUARD_STUNNED,   240,  -1);  /* xterm-256 dark gray */
        init_pair(COL_DOOR,            208,  -1);  /* xterm-256 amber/orange */
        init_pair(COL_PROJECTILE,      231,  -1);  /* xterm-256 pure white (needle) */
        init_pair(COL_GUARD_ELITE,      75,  -1);  /* xterm-256 steel blue */
    } else {
        /* 8-color fallback */
        init_pair(COL_FLOOR_VIS,  COLOR_CYAN,    -1);
        init_pair(COL_FLOOR_MEM,  COLOR_BLUE,    -1);
        init_pair(COL_WALL_VIS,   COLOR_WHITE,   -1);
        init_pair(COL_WALL_MEM,   COLOR_BLACK,   -1);
        init_pair(COL_PLAYER,     COLOR_YELLOW,  -1);
        init_pair(COL_STATUS,     COLOR_CYAN,    -1);
        init_pair(COL_STAIRS,     COLOR_GREEN,   -1);
        init_pair(COL_MENU_SEL,   COLOR_YELLOW,  -1);
        init_pair(COL_MENU_DIM,   COLOR_WHITE,   -1);
        init_pair(COL_MENU_TITLE, COLOR_CYAN,    -1);
        /* Rarity colours (8-color fallback) */
        init_pair(COL_RARITY_COMMON,   COLOR_WHITE,   -1);
        init_pair(COL_RARITY_UNCOMMON, COLOR_GREEN,   -1);
        init_pair(COL_RARITY_RARE,     COLOR_CYAN,    -1);
        init_pair(COL_RARITY_EPIC,     COLOR_MAGENTA, -1);
        init_pair(COL_RARITY_LEGENDARY,COLOR_YELLOW,  -1);
        /* Guard colours (8-color fallback) */
        init_pair(COL_GUARD,            COLOR_WHITE,  -1);
        init_pair(COL_GUARD_SUSPICIOUS, COLOR_YELLOW, -1);
        init_pair(COL_GUARD_ALERT,      COLOR_RED,    -1);
        /* Vent colour (8-color fallback) */
        init_pair(COL_VENT,             COLOR_CYAN,    -1);
        /* Hacking colours (8-color fallback) */
        init_pair(COL_TERMINAL,         COLOR_MAGENTA, -1);
        init_pair(COL_GUARD_STUNNED,    COLOR_BLACK,   -1);
        init_pair(COL_DOOR,             COLOR_YELLOW,  -1);
        init_pair(COL_PROJECTILE,       COLOR_WHITE,   -1);
        init_pair(COL_GUARD_ELITE,      COLOR_CYAN,    -1);
    }
}

void render_map(const Map *map)
{
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            const Tile *t = &map->tiles[y][x];

            if (t->visible) {
                switch (t->type) {
                case TILE_FLOOR:
                    attron(COLOR_PAIR(COL_FLOOR_VIS));
                    mvaddch(y, x, '.');
                    attroff(COLOR_PAIR(COL_FLOOR_VIS));
                    break;
                case TILE_STAIRS:
                    attron(COLOR_PAIR(COL_STAIRS) | A_BOLD);
                    mvaddch(y, x, '>');
                    attroff(COLOR_PAIR(COL_STAIRS) | A_BOLD);
                    break;
                case TILE_VENT:
                    attron(COLOR_PAIR(COL_VENT) | A_BOLD);
                    mvaddch(y, x, '=');
                    attroff(COLOR_PAIR(COL_VENT) | A_BOLD);
                    break;
                case TILE_TERMINAL:
                    attron(COLOR_PAIR(COL_TERMINAL) | A_BOLD);
                    mvaddch(y, x, '[');
                    attroff(COLOR_PAIR(COL_TERMINAL) | A_BOLD);
                    break;
                case TILE_DOOR:
                    attron(COLOR_PAIR(COL_DOOR) | A_BOLD);
                    mvaddch(y, x, '+');
                    attroff(COLOR_PAIR(COL_DOOR) | A_BOLD);
                    break;
                case TILE_STAIRS_LOCKED:
                    /* Invisible until unlocked — renders as plain floor */
                    attron(COLOR_PAIR(COL_FLOOR_VIS));
                    mvaddch(y, x, '.');
                    attroff(COLOR_PAIR(COL_FLOOR_VIS));
                    break;
                default: /* TILE_WALL */
                    attron(COLOR_PAIR(COL_WALL_VIS) | A_BOLD);
                    mvaddch(y, x, '#');
                    attroff(COLOR_PAIR(COL_WALL_VIS) | A_BOLD);
                    break;
                }
            } else if (t->seen) {
                switch (t->type) {
                case TILE_FLOOR:
                    attron(COLOR_PAIR(COL_FLOOR_MEM));
                    mvaddch(y, x, '.');
                    attroff(COLOR_PAIR(COL_FLOOR_MEM));
                    break;
                case TILE_STAIRS:
                    attron(COLOR_PAIR(COL_FLOOR_MEM));
                    mvaddch(y, x, '>');
                    attroff(COLOR_PAIR(COL_FLOOR_MEM));
                    break;
                case TILE_VENT:
                    attron(COLOR_PAIR(COL_FLOOR_MEM));
                    mvaddch(y, x, '=');
                    attroff(COLOR_PAIR(COL_FLOOR_MEM));
                    break;
                case TILE_TERMINAL:
                    attron(COLOR_PAIR(COL_FLOOR_MEM));
                    mvaddch(y, x, '[');
                    attroff(COLOR_PAIR(COL_FLOOR_MEM));
                    break;
                case TILE_DOOR:
                    attron(COLOR_PAIR(COL_FLOOR_MEM));
                    mvaddch(y, x, '+');
                    attroff(COLOR_PAIR(COL_FLOOR_MEM));
                    break;
                case TILE_STAIRS_LOCKED:
                    attron(COLOR_PAIR(COL_FLOOR_MEM));
                    mvaddch(y, x, '.');
                    attroff(COLOR_PAIR(COL_FLOOR_MEM));
                    break;
                default: /* TILE_WALL */
                    attron(COLOR_PAIR(COL_WALL_MEM) | A_BOLD);
                    mvaddch(y, x, '#');
                    attroff(COLOR_PAIR(COL_WALL_MEM) | A_BOLD);
                    break;
                }
            } else {
                mvaddch(y, x, ' ');
            }
        }
    }
}

void render_entity(const Entity *e)
{
    attron(COLOR_PAIR(COL_PLAYER) | A_BOLD);
    mvaddch(e->y, e->x, e->symbol);
    attroff(COLOR_PAIR(COL_PLAYER) | A_BOLD);
}

void render_guards(const GuardList *gl, const Map *map, bool scan_active)
{
    for (int i = 0; i < gl->count; i++) {
        const Guard *g = &gl->guards[i];
        if (!map->tiles[g->y][g->x].visible && !scan_active) continue;

        int    col;
        attr_t attr = A_BOLD;
        char   sym  = g->elite ? 'E' : 'G';

        if (g->stun_timer > 0) {
            col  = COL_GUARD_STUNNED;
            attr = 0;
        } else {
            switch (g->state) {
            case GUARD_UNAWARE:
                col  = g->elite ? COL_GUARD_ELITE : COL_GUARD;
                attr = g->elite ? A_BOLD : 0;
                break;
            case GUARD_SUSPICIOUS:
                col = COL_GUARD_SUSPICIOUS;
                break;
            default: /* ALERT and HUNTING */
                col = COL_GUARD_ALERT;
                break;
            }
        }

        attron(COLOR_PAIR(col) | attr);
        mvaddch(g->y, g->x, sym);
        attroff(COLOR_PAIR(col) | attr);
    }
}

void render_projectiles(const ProjectileList *pl)
{
    for (int i = 0; i < pl->count; i++) {
        const Projectile *p = &pl->p[i];
        if (!p->active) continue;
        attron(COLOR_PAIR(COL_PROJECTILE) | A_BOLD);
        mvaddch(p->y, p->x, p->symbol);
        attroff(COLOR_PAIR(COL_PROJECTILE) | A_BOLD);
    }
}

void render_status(const Entity *e, const GuardList *gl,
                   bool bt_avail, bool bt_active,
                   bool gp_avail, int gp_timer,
                   bool door_locked, bool extract_locked,
                   const NetrunnerState *nr,
                   const char *msg)
{
    move(MAP_HEIGHT, 0);
    clrtoeol();

    attron(COLOR_PAIR(COL_STATUS) | A_BOLD);
    mvprintw(MAP_HEIGHT, 0, "HP:%d/%d  ¢%d  LVL:%d",
             e->hp, e->max_hp, e->credits, e->level);
    attroff(COLOR_PAIR(COL_STATUS) | A_BOLD);

    /* Mission objective indicators */
    if (door_locked) {
        attron(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
        printw("  [DOOR:LOCKED]");
        attroff(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
    } else {
        attron(COLOR_PAIR(COL_STAIRS) | A_BOLD);
        printw("  [DOOR:OPEN]");
        attroff(COLOR_PAIR(COL_STAIRS) | A_BOLD);
    }
    if (extract_locked) {
        attron(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
        printw("  [EXIT:LOCKED]");
        attroff(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
    } else {
        attron(COLOR_PAIR(COL_STAIRS) | A_BOLD);
        printw("  [EXIT:LIVE]");
        attroff(COLOR_PAIR(COL_STAIRS) | A_BOLD);
    }

    /* Crouch indicator */
    if (e->crouched) {
        attron(COLOR_PAIR(COL_STAIRS) | A_BOLD);
        printw("  [CROUCH]");
        attroff(COLOR_PAIR(COL_STAIRS) | A_BOLD);
    }

    /* Bullet Time indicator */
    if (bt_active) {
        attron(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        printw("  [BULLET TIME]");
        attroff(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    } else if (bt_avail) {
        attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        printw("  [BTIME RDY]");
        attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
    }

    /* Ghost Protocol indicator */
    if (gp_timer > 0) {
        attron(COLOR_PAIR(COL_FLOOR_VIS) | A_BOLD);
        printw("  [GHOST:%d]", gp_timer);
        attroff(COLOR_PAIR(COL_FLOOR_VIS) | A_BOLD);
    } else if (gp_avail) {
        attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        printw("  [GHOST RDY]");
        attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
    }

    /* Guard alert indicator */
    GuardState max = guard_max_state(gl);
    if (max >= GUARD_HUNTING) {
        attron(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
        printw("  !! HUNTING");
        attroff(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
    } else if (max >= GUARD_ALERT) {
        attron(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
        printw("  ! ALERT");
        attroff(COLOR_PAIR(COL_GUARD_ALERT) | A_BOLD);
    } else if (max >= GUARD_SUSPICIOUS) {
        attron(COLOR_PAIR(COL_GUARD_SUSPICIOUS) | A_BOLD);
        printw("  ? SUSPICIOUS");
        attroff(COLOR_PAIR(COL_GUARD_SUSPICIOUS) | A_BOLD);
    }

    /* Netrunner ability indicators */
    if (nr) {
        if (nr->godmode_timer > 0) {
            attron(COLOR_PAIR(COL_TERMINAL) | A_BOLD);
            printw("  [GOD:%d]", nr->godmode_timer);
            attroff(COLOR_PAIR(COL_TERMINAL) | A_BOLD);
        } else if (nr->godmode_avail) {
            attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
            printw("  [GOD RDY]");
            attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        }
        if (nr->overclock_armed) {
            attron(COLOR_PAIR(COL_TERMINAL) | A_BOLD);
            printw("  [OC!]");
            attroff(COLOR_PAIR(COL_TERMINAL) | A_BOLD);
        } else if (nr->overclock_avail) {
            attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
            printw("  [OC RDY]");
            attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        }
        if (nr->trap_avail) {
            attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
            printw("  [TRAP:%d]", MAX_TRAPS - nr->trap_count);
            attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        }
        if (nr->daemon_avail) {
            attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
            printw("  [DMN RDY]");
            attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        }
        if (nr->emp_avail) {
            attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
            printw("  [EMP RDY]");
            attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        }
        if (nr->spoof_avail) {
            attron(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
            printw("  [SPOOF RDY]");
            attroff(COLOR_PAIR(COL_MENU_DIM) | A_BOLD);
        }
    }

    attron(COLOR_PAIR(COL_STATUS) | A_BOLD);
    printw("  [hjkl] move  [i] inv  [f] fire  [m] medkit  [x] hack  [t] btime  [g] ghost  [v] vent  [>] extract  [q] hub");
    attroff(COLOR_PAIR(COL_STATUS) | A_BOLD);

    /* One-turn feedback message on the line below the status bar */
    move(MAP_HEIGHT + 1, 0);
    clrtoeol();
    if (msg && msg[0] != '\0') {
        bool success = (msg[0] == 'H' && msg[5] == 'S');  /* "HACK S..." */
        int col = success ? COL_STAIRS : COL_GUARD_ALERT;
        attron(COLOR_PAIR(col) | A_BOLD);
        mvprintw(MAP_HEIGHT + 1, 0, "%s", msg);
        attroff(COLOR_PAIR(col) | A_BOLD);
    }
}
