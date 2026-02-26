#include "hub_map.h"
#include "hub.h"
#include "render.h"
#include "map.h"
#include "entity.h"

#include <ncurses.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Camera (mirrors the static in main.c)                               */
/* ------------------------------------------------------------------ */

static void compute_camera(int px, int py, int *cam_x, int *cam_y)
{
    int view_w = COLS;
    int view_h = LINES - 2;   /* two status rows at bottom */
    *cam_x = px - view_w / 2;
    *cam_y = py - view_h / 2;
    if (*cam_x < 0) *cam_x = 0;
    if (*cam_y < 0) *cam_y = 0;
    if (*cam_x + view_w > MAP_WIDTH)  *cam_x = MAP_WIDTH  - view_w;
    if (*cam_y + view_h > MAP_HEIGHT) *cam_y = MAP_HEIGHT - view_h;
    if (*cam_x < 0) *cam_x = 0;
    if (*cam_y < 0) *cam_y = 0;
}

/* ------------------------------------------------------------------ */
/* Quit confirm dialog                                                  */
/* ------------------------------------------------------------------ */

static bool hub_confirm_quit(void)
{
    int dw = 36, dh = 7;
    int dy = (LINES - dh) / 2;
    int dx = (COLS  - dw) / 2;
    WINDOW *dlg = newwin(dh, dw, dy, dx);
    keypad(dlg, TRUE);
    box(dlg, 0, 0);

    wattron(dlg, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
    mvwprintw(dlg, 0, (dw - 10) / 2, " QUIT GAME ");
    wattroff(dlg, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

    wattron(dlg, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(dlg, 2, 4, "Abandon the run and quit?");
    mvwprintw(dlg, 4, 6, "[ Y ] Quit");
    mvwprintw(dlg, 5, 6, "[ N ] Stay");
    wattroff(dlg, COLOR_PAIR(COL_MENU_DIM));

    wrefresh(dlg);

    bool result = false;
    while (1) {
        int ch = wgetch(dlg);
        if (ch == 'y' || ch == 'Y') { result = true;  break; }
        if (ch == 'n' || ch == 'N') { result = false; break; }
        if (ch == 27 /* Esc */)     { result = false; break; }
    }
    delwin(dlg);
    return result;
}

/* ------------------------------------------------------------------ */
/* Hub city layout — organic cyberpunk district                        */
/* ------------------------------------------------------------------ */
/*
 *  Street grid (all TILE_FLOOR):
 *    V1 vert:  x=3-5    y=3-68  (west edge)
 *    V2 vert:  x=33-35  y=3-68  (left-center)
 *    V3 vert:  x=70-72  y=3-68  (center)
 *    V4 vert:  x=111-113 y=3-68 (right-center)
 *    V5 vert:  x=150-152 y=3-68 (east edge)
 *    North alley:  y=19-21  x=3-155
 *    Main street:  y=37-40  x=3-155  (4 wide)
 *    South alley:  y=54-56  x=3-155
 *
 *  Buildings (interior + doorway through wall row):
 *
 *  TOP ROW (interior y=5-17, south wall y=18 → doorway → north alley y=19-21):
 *    ARMS DEALER   x=6-32    center x=18  NPC VIVID
 *    RIPPERDOC     x=36-69   center x=52  NPC DOC WIRE  (SW notch)
 *    SAFE HOUSE    x=73-110  center x=91  terminal x=91,y=10  (NE notch)
 *    TECH DEALER   x=114-149 center x=130 NPC MOTH
 *
 *  MID ROW (interior y=23-35, north wall y=22, south wall y=36):
 *    STREET DOC    x=6-32    center x=18  NPC REY
 *    OPEN PLAZA    x=36-69   (fully open, pillars at y=26,32 x=47,58)
 *    FIXER         x=73-110  center x=91  NPC J-BONE
 *    FENCE         x=114-140 + back room x=143-149,y=27-34  NPC FRIX
 *
 *  SOUTH ROW (interior y=42-52, north wall y=41):
 *    HANGOUT A     x=6-32    ambient NPC GHOST
 *    HANGOUT B     x=36-69   ambient NPC KIRA
 *    HANGOUT C     x=73-100  + side alcove x=102-110,y=46-51  ambient NPC V
 *
 *  DEEP SOUTH dead-ends (interior y=58-65, north wall y=57):
 *    Dead-end W    x=6-32
 *    Dead-end E    x=114-149
 *
 *  Player spawn: x=91, y=38  (main street in front of FIXER entrance)
 */

/* Player spawn — main street, in front of the FIXER building entrance */
#define HUB_START_X   91
#define HUB_START_Y   38

/* ------------------------------------------------------------------ */
/* Map generation                                                       */
/* ------------------------------------------------------------------ */

static void fill_rect(Map *map, int x1, int y1, int x2, int y2, TileType t)
{
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
                map->tiles[y][x].type = t;
}

static void map_generate_hub(Map *map)
{
    /* Fill everything with walls */
    fill_rect(map, 0, 0, MAP_WIDTH - 1, MAP_HEIGHT - 1, TILE_WALL);

    /* ==== Vertical streets (run full height of active zone) ==== */
    fill_rect(map,   3,  3,   5, 68, TILE_FLOOR);  /* V1: west edge   */
    fill_rect(map,  33,  3,  35, 68, TILE_FLOOR);  /* V2: left-center */
    fill_rect(map,  70,  3,  72, 68, TILE_FLOOR);  /* V3: center      */
    fill_rect(map, 111,  3, 113, 68, TILE_FLOOR);  /* V4: right-center*/
    fill_rect(map, 150,  3, 152, 68, TILE_FLOOR);  /* V5: east edge   */

    /* ==== Horizontal streets ==== */
    fill_rect(map,   3, 19, 155, 21, TILE_FLOOR);  /* North alley  y=19-21 */
    fill_rect(map,   3, 37, 155, 40, TILE_FLOOR);  /* Main street  y=37-40 */
    fill_rect(map,   3, 54, 155, 56, TILE_FLOOR);  /* South alley  y=54-56 */

    /* ==== TOP ROW BUILDINGS (interior y=5-17; south wall y=18) ==== */

    /* ARMS DEALER — x=6-32 */
    fill_rect(map,  6,  5, 32, 17, TILE_FLOOR);
    fill_rect(map, 17, 18, 21, 18, TILE_FLOOR);  /* doorway → north alley */

    /* RIPPERDOC — x=36-69, SW notch for organic shape */
    fill_rect(map, 36,  5, 69, 17, TILE_FLOOR);
    fill_rect(map, 36,  5, 43, 10, TILE_WALL);   /* SW notch carved out  */
    fill_rect(map, 50, 18, 54, 18, TILE_FLOOR);  /* doorway → north alley */

    /* SAFE HOUSE / player apartment — x=73-110, NE notch */
    fill_rect(map,  73,  5, 110, 17, TILE_FLOOR);
    fill_rect(map, 103,  5, 110,  9, TILE_WALL);  /* NE notch carved out  */
    fill_rect(map,  89, 18,  93, 18, TILE_FLOOR); /* doorway → north alley */

    /* TECH DEALER — x=114-149 */
    fill_rect(map, 114,  5, 149, 17, TILE_FLOOR);
    fill_rect(map, 129, 18, 133, 18, TILE_FLOOR); /* doorway → north alley */

    /* ==== MID ROW BUILDINGS (interior y=23-35; walls y=22 and y=36) ==== */

    /* STREET DOC — x=6-32 */
    fill_rect(map,  6, 23, 32, 35, TILE_FLOOR);
    fill_rect(map, 17, 22, 21, 22, TILE_FLOOR);  /* north doorway → alley  */
    fill_rect(map, 17, 36, 21, 36, TILE_FLOOR);  /* south doorway → street */

    /* OPEN PLAZA — x=36-69, fully open on all four sides */
    fill_rect(map, 36, 23, 69, 35, TILE_FLOOR);
    fill_rect(map, 36, 22, 69, 22, TILE_FLOOR);  /* open north → alley    */
    fill_rect(map, 36, 36, 69, 36, TILE_FLOOR);  /* open south → street   */
    /* Decorative pillars */
    map->tiles[26][47].type = TILE_WALL;
    map->tiles[26][58].type = TILE_WALL;
    map->tiles[32][47].type = TILE_WALL;
    map->tiles[32][58].type = TILE_WALL;

    /* FIXER — x=73-110 */
    fill_rect(map,  73, 23, 110, 35, TILE_FLOOR);
    fill_rect(map,  89, 22,  93, 22, TILE_FLOOR); /* north doorway */
    fill_rect(map,  89, 36,  93, 36, TILE_FLOOR); /* south doorway */

    /* FENCE — x=114-140 main area + back room x=143-149 */
    fill_rect(map, 114, 23, 140, 35, TILE_FLOOR);
    fill_rect(map, 143, 27, 149, 34, TILE_FLOOR); /* back room           */
    fill_rect(map, 141, 30, 142, 32, TILE_FLOOR); /* connecting corridor  */
    fill_rect(map, 129, 22, 133, 22, TILE_FLOOR); /* north doorway        */
    fill_rect(map, 129, 36, 133, 36, TILE_FLOOR); /* south doorway        */

    /* ==== SOUTH ROW BUILDINGS (interior y=42-52; north wall y=41) ==== */

    /* HANGOUT A — x=6-32 */
    fill_rect(map,  6, 42, 32, 52, TILE_FLOOR);
    fill_rect(map, 17, 41, 21, 41, TILE_FLOOR);  /* doorway ← main street */

    /* HANGOUT B — x=36-69 */
    fill_rect(map, 36, 42, 69, 52, TILE_FLOOR);
    fill_rect(map, 51, 41, 55, 41, TILE_FLOOR);  /* doorway */

    /* HANGOUT C — x=73-100 + side alcove x=102-110,y=46-51 */
    fill_rect(map,  73, 42, 100, 52, TILE_FLOOR);
    fill_rect(map,  89, 41,  92, 41, TILE_FLOOR); /* doorway */
    fill_rect(map, 102, 46, 110, 51, TILE_FLOOR); /* side alcove          */
    fill_rect(map, 101, 47, 101, 50, TILE_FLOOR); /* alcove doorway       */

    /* ==== DEEP SOUTH dead-ends (interior y=58-65, north wall y=57) ==== */
    fill_rect(map,   6, 58,  32, 65, TILE_FLOOR);
    fill_rect(map,  17, 57,  21, 57, TILE_FLOOR); /* connects to south alley */

    fill_rect(map, 114, 58, 149, 65, TILE_FLOOR);
    fill_rect(map, 129, 57, 133, 57, TILE_FLOOR); /* connects to south alley */

    /* ---- Skill terminal in player's safe house ---- */
    map->tiles[10][91].type = TILE_TERMINAL;

    map->num_rooms     = 0;
    map->num_vents     = 0;
    map->num_terminals = 0;
    map->stairs_x      = -1;
    map->stairs_y      = -1;
    map->door_x        = -1;
    map->door_y        = -1;
}

/* ------------------------------------------------------------------ */
/* NPC initialisation                                                   */
/* ------------------------------------------------------------------ */

static int hub_npcs_init(HubNpc *npcs)
{
    int n = 0;

#define MAKE_NPC(x_, y_, sym_, col_, name_, type_) \
    do { \
        HubNpc *np = &npcs[n]; \
        np->x = (x_); np->y = (y_); \
        np->symbol = (sym_); np->color_pair = (col_); \
        strncpy(np->name, (name_), sizeof(np->name) - 1); \
        np->name[sizeof(np->name) - 1] = '\0'; \
        np->dialog_count = 0; \
        np->type = (type_); \
        n++; \
    } while (0)

#define ADD_DLG(line) \
    do { \
        HubNpc *np = &npcs[n - 1]; \
        if (np->dialog_count < HUB_DIALOG_LINES) { \
            strncpy(np->dialog[np->dialog_count], (line), HUB_DIALOG_LEN - 1); \
            np->dialog[np->dialog_count][HUB_DIALOG_LEN - 1] = '\0'; \
            np->dialog_count++; \
        } \
    } while (0)

    /* VIVID — arms dealer (top row, ARMS building x=6-32) */
    MAKE_NPC(18, 12, 'V', COL_HUB_ARMS, "VIVID", HUB_NPC_ARMS);
    ADD_DLG("Heat's running high in the Sprawl tonight.");
    ADD_DLG("Off-registry hardware, clean and ready to go.");
    ADD_DLG("Take a look at the inventory.");

    /* DOC WIRE — ripperdoc (top row, RIPPERDOC building x=36-69) */
    MAKE_NPC(52, 12, 'D', COL_HUB_RIPPERDOC, "DOC WIRE", HUB_NPC_RIPPERDOC);
    ADD_DLG("Welcome to the Clinic. Cash only, no questions.");
    ADD_DLG("Latest chrome arrived this morning. Top-shelf implants.");
    ADD_DLG("Let me show you what's on the table.");

    /* MOTH — tech dealer (top row, TECH building x=114-149) */
    MAKE_NPC(130, 12, 'M', COL_HUB_TECH, "MOTH", HUB_NPC_TECH);
    ADD_DLG("Corp-grade armor and decks at runner prices.");
    ADD_DLG("Don't ask where it came from. Just enjoy the discount.");
    ADD_DLG("Browse the racks.");

    /* REY — street doc (mid row, STREET DOC building x=6-32) */
    MAKE_NPC(18, 29, 'R', COL_HUB_MEDIC, "REY", HUB_NPC_MEDIC);
    ADD_DLG("You look like you could use a patch-up.");
    ADD_DLG("Stim packs, med kits, full trauma rigs. I keep runners alive.");
    ADD_DLG("What do you need?");

    /* J-BONE — fixer (mid row, FIXER building x=73-110) */
    MAKE_NPC(91, 29, 'J', COL_HUB_FIXER, "J-BONE", HUB_NPC_FIXER);
    ADD_DLG("J-Bone here. Got three contracts, all paying clean credits.");
    ADD_DLG("Don't get dead — hard to collect my cut from a corpse.");
    ADD_DLG("See what's on the board.");

    /* FRIX — fence (mid row, FENCE building x=114-140) */
    MAKE_NPC(126, 29, 'F', COL_HUB_FENCE, "FRIX", HUB_NPC_FENCE);
    ADD_DLG("Got gear burning a hole in your bag? I pay fair.");
    ADD_DLG("Corp-stamped items especially welcome. No serial numbers.");
    ADD_DLG("Lay it out on the table.");

    /* Ambient NPCs */
    MAKE_NPC(52, 29, '@', COL_HUB_AMBIENT, "SAL", HUB_NPC_AMBIENT);
    ADD_DLG("Heard Arasaka locked down the whole Eastside.");
    ADD_DLG("ICE everywhere. Don't run that district tonight.");

    MAKE_NPC(18, 47, '@', COL_HUB_AMBIENT, "GHOST", HUB_NPC_AMBIENT);
    ADD_DLG("Someone's been watching the Arms District real close.");
    ADD_DLG("Corps or a rival crew — either way, watch your back.");

    MAKE_NPC(52, 47, '@', COL_HUB_AMBIENT, "KIRA", HUB_NPC_AMBIENT);
    ADD_DLG("Three jobs this week. I'm basically a walking stim pack.");
    ADD_DLG("The Sprawl doesn't slow down, so neither do I.");

    MAKE_NPC(86, 47, '@', COL_HUB_AMBIENT, "V", HUB_NPC_AMBIENT);
    ADD_DLG("I know a guy who knows a guy.");
    ADD_DLG("Off-grid transport, no corp tags. If you ever need it.");

#undef MAKE_NPC
#undef ADD_DLG

    return n;
}

/* ------------------------------------------------------------------ */
/* Hub rendering helpers                                                */
/* ------------------------------------------------------------------ */

static void render_hub_npcs(const HubNpc *npcs, int count, int cam_x, int cam_y)
{
    int view_h = LINES - 2;
    for (int i = 0; i < count; i++) {
        const HubNpc *npc = &npcs[i];
        int sx = npc->x - cam_x;
        int sy = npc->y - cam_y;
        if (sx < 0 || sx >= COLS || sy < 0 || sy >= view_h)
            continue;
        attron(COLOR_PAIR(npc->color_pair) | A_BOLD);
        mvaddch(sy, sx, (unsigned char)npc->symbol);
        attroff(COLOR_PAIR(npc->color_pair) | A_BOLD);
    }
}

/* Location labels drawn as dim text inside each district */
static void render_hub_labels(int cam_x, int cam_y)
{
    static const struct { int wx, wy; const char *text; } labels[] = {
        /* Top row building names (just inside north wall) */
        {  7,  5,  "ARMS DEALER"  },
        { 37,  5,  "RIPPERDOC"    },
        { 74,  5,  "SAFE HOUSE"   },
        {115,  5,  "TECH DEALER"  },
        /* Mid row labels */
        {  7, 23,  "STREET DOC"   },
        { 40, 23,  ":: PLAZA ::"  },
        { 74, 23,  "FIXER"        },
        {115, 23,  "FENCE"        },
        /* South row labels */
        {  7, 42,  "HANGOUT"      },
        { 37, 42,  "HANGOUT"      },
        { 74, 42,  "HANGOUT"      },
        /* Street / alley name signs */
        { 37, 20,  "NEON ALLEY"            },
        { 55, 38,  "CHROME STRIP"          },
        { 37, 55,  "BACK ALLEY"            },
    };
    int view_h = LINES - 2;
    attron(COLOR_PAIR(COL_MENU_DIM));
    for (int i = 0; i < (int)(sizeof(labels) / sizeof(labels[0])); i++) {
        int sx = labels[i].wx - cam_x;
        int sy = labels[i].wy - cam_y;
        if (sy < 0 || sy >= view_h || sx >= COLS) continue;
        if (sx < 0) sx = 0;
        mvprintw(sy, sx, "%s", labels[i].text);
    }
    attroff(COLOR_PAIR(COL_MENU_DIM));
}

static void render_hub_status(const Entity *player, const HubNpc *nearby,
                               bool near_term, const char *msg)
{
    /* Line 1: character stats */
    attron(COLOR_PAIR(COL_STATUS) | A_BOLD);
    mvprintw(LINES - 2, 0, "HP:%d/%d  \xc2\xa2%d  LVL:%d  XP:%d/%d  SP:%d",
             player->hp, player->max_hp,
             player->credits,
             player->level,
             player->xp, player->xp_next,
             player->skill_points);
    clrtoeol();
    attroff(COLOR_PAIR(COL_STATUS) | A_BOLD);

    /* Line 2: interaction prompt or help */
    move(LINES - 1, 0);
    clrtoeol();
    if (nearby) {
        attron(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        mvprintw(LINES - 1, 0, "[E] Talk to %-10s  [I] Inventory   [Q] Quit",
                 nearby->name);
        attroff(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    } else if (near_term) {
        attron(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        mvprintw(LINES - 1, 0,
                 "[E] Access terminal  [I] Inventory   [Q] Quit");
        attroff(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    } else if (msg && msg[0]) {
        attron(COLOR_PAIR(COL_MENU_DIM));
        mvprintw(LINES - 1, 0, "%s", msg);
        attroff(COLOR_PAIR(COL_MENU_DIM));
    } else {
        attron(COLOR_PAIR(COL_MENU_DIM));
        mvprintw(LINES - 1, 0,
                 "hjkl: move  [E] interact  [C] equipped  [I] inventory  [Q] quit");
        attroff(COLOR_PAIR(COL_MENU_DIM));
    }
}

/* ------------------------------------------------------------------ */
/* NPC interaction helpers                                              */
/* ------------------------------------------------------------------ */

static HubNpc *hub_adjacent_npc(HubNpc *npcs, int count, int px, int py)
{
    for (int i = 0; i < count; i++) {
        int dx = npcs[i].x - px;
        int dy = npcs[i].y - py;
        if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1
                && (dx != 0 || dy != 0))
            return &npcs[i];
    }
    return NULL;
}

static bool hub_npc_at(const HubNpc *npcs, int count, int x, int y)
{
    for (int i = 0; i < count; i++)
        if (npcs[i].x == x && npcs[i].y == y) return true;
    return false;
}

static bool hub_adjacent_terminal(const Map *map, int px, int py)
{
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int x = px + dx, y = py + dy;
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
                if (map->tiles[y][x].type == TILE_TERMINAL)
                    return true;
        }
    return false;
}

/* ------------------------------------------------------------------ */
/* Dialog popup                                                         */
/* ------------------------------------------------------------------ */

static void hub_show_dialog(const HubNpc *npc)
{
    int dw = 52;
    int dh = 2 + npc->dialog_count + 2;
    if (dh < 6) dh = 6;
    int dy = (LINES - dh) / 2;
    int dx = (COLS  - dw) / 2;
    if (dx < 0) dx = 0;

    WINDOW *win = newwin(dh, dw, dy, dx);
    keypad(win, TRUE);
    box(win, 0, 0);

    /* NPC name as header */
    char hdr[36];
    snprintf(hdr, sizeof(hdr), " %s ", npc->name);
    wattron(win, COLOR_PAIR(npc->color_pair) | A_BOLD);
    mvwprintw(win, 0, (dw - (int)strlen(hdr)) / 2, "%s", hdr);
    wattroff(win, COLOR_PAIR(npc->color_pair) | A_BOLD);

    /* Dialog lines */
    for (int i = 0; i < npc->dialog_count; i++) {
        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, 1 + i, 2, "%-*.*s", dw - 4, dw - 4, npc->dialog[i]);
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));
    }

    /* Footer */
    wattron(win, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(win, dh - 1, (dw - 22) / 2, "-- press any key --");
    wattroff(win, COLOR_PAIR(COL_MENU_DIM));

    wrefresh(win);
    wgetch(win);
    delwin(win);
}

/* ------------------------------------------------------------------ */
/* Terminal mini-menu (player's safe house terminal)                   */
/* ------------------------------------------------------------------ */

static void hub_terminal_interact(Entity *player)
{
    static const char *opts[] = { "Neural Skills", "Equipment", "Back" };
    enum { OPT_SKILLS, OPT_EQUIPMENT, OPT_BACK };
    int nitems = 3;
    int sel    = 0;

    int dw = 42, dh = 12;
    int dy = (LINES - dh) / 2;
    int dx = (COLS  - dw) / 2;
    WINDOW *win = newwin(dh, dw, dy, dx);
    keypad(win, TRUE);

    bool dirty = true;
    while (1) {
        if (dirty) {
            werase(win);
            box(win, 0, 0);
            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, 0, (dw - 21) / 2, " PERSONAL TERMINAL ");
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, 2, 3, "LVL:%-2d  SP:%-2d  HP:%d/%d",
                      player->level, player->skill_points,
                      player->hp, player->max_hp);
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            for (int i = 0; i < nitems; i++) {
                bool is_sel = (i == sel);
                attr_t attr = is_sel ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                                     : COLOR_PAIR(COL_MENU_DIM);
                wattron(win, attr);
                mvwprintw(win, 4 + i * 2, 4, "%s%-22s",
                          is_sel ? "> " : "  ", opts[i]);
                wattroff(win, attr);
            }

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, dh - 2, 3, "[j/k] nav  [Enter] select  [Esc] close");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            wrefresh(win);
            dirty = false;
        }

        int ch = wgetch(win);
        switch (ch) {
        case KEY_UP: case 'k':
            if (sel > 0) sel--;
            dirty = true;
            break;
        case KEY_DOWN: case 'j':
            if (sel < nitems - 1) sel++;
            dirty = true;
            break;
        case '\n': case KEY_ENTER:
            delwin(win);
            if (sel == OPT_SKILLS)    hub_levelup_menu(player);
            else if (sel == OPT_EQUIPMENT) hub_screen_equipment(player);
            /* OPT_BACK falls through */
            return;
        case 'q': case 27:
            delwin(win);
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* NPC interaction dispatcher                                           */
/* ------------------------------------------------------------------ */

static GameState hub_interact_npc(Entity *player, HubNpc *npc)
{
    hub_show_dialog(npc);

    switch (npc->type) {
    case HUB_NPC_ARMS:      hub_shop_weapons(player);     break;
    case HUB_NPC_RIPPERDOC: hub_shop_cybernetics(player); break;
    case HUB_NPC_FIXER:     return hub_screen_missions(player);
    case HUB_NPC_TECH:      hub_shop_tech(player);        break;
    case HUB_NPC_FENCE:     hub_shop_fence(player);       break;
    case HUB_NPC_MEDIC:     hub_shop_consumables(player); break;
    case HUB_NPC_AMBIENT:   break;  /* dialog only */
    }
    return GAME_STATE_HUB;
}

/* ------------------------------------------------------------------ */
/* Main hub map loop                                                    */
/* ------------------------------------------------------------------ */

GameState hub_map_run(Entity *player)
{
    static Map      map;
    static HubNpc   npcs[HUB_NPC_MAX];

    map_generate_hub(&map);

    /* Hub is always fully visible — no fog of war */
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++) {
            map.tiles[y][x].visible = true;
            map.tiles[y][x].seen    = true;
        }

    int npc_count = hub_npcs_init(npcs);

    /* Place player at hub spawn point */
    player->x = HUB_START_X;
    player->y = HUB_START_Y;

    char msg[128] = "";
    int  cam_x, cam_y;

    while (1) {
        compute_camera(player->x, player->y, &cam_x, &cam_y);

        clear();
        render_map(&map, cam_x, cam_y);
        render_hub_labels(cam_x, cam_y);
        render_hub_npcs(npcs, npc_count, cam_x, cam_y);
        render_entity(player, cam_x, cam_y);

        HubNpc *nearby    = hub_adjacent_npc(npcs, npc_count, player->x, player->y);
        bool    near_term = hub_adjacent_terminal(&map, player->x, player->y);

        render_hub_status(player, nearby, near_term, msg);
        refresh();

        msg[0] = '\0';
        int ch = getch();

        /* Movement */
        int dx = 0, dy = 0;
        switch (ch) {
        case 'h': case KEY_LEFT:  dx = -1;          break;
        case 'l': case KEY_RIGHT: dx =  1;          break;
        case 'k': case KEY_UP:    dy = -1;          break;
        case 'j': case KEY_DOWN:  dy =  1;          break;
        case 'y': dx = -1; dy = -1;                 break;
        case 'u': dx =  1; dy = -1;                 break;
        case 'b': dx = -1; dy =  1;                 break;
        case 'n': dx =  1; dy =  1;                 break;
        default:  break;
        }

        if (dx != 0 || dy != 0) {
            int nx = player->x + dx;
            int ny = player->y + dy;
            if (!map_is_blocked(&map, nx, ny)
                    && !hub_npc_at(npcs, npc_count, nx, ny)) {
                player->x = nx;
                player->y = ny;
            }
            continue;
        }

        /* Interaction */
        if (ch == 'e' || ch == '\n' || ch == KEY_ENTER) {
            if (nearby) {
                GameState next = hub_interact_npc(player, nearby);
                if (next != GAME_STATE_HUB) return next;
            } else if (near_term) {
                hub_terminal_interact(player);
            }
        } else if (ch == 'c' || ch == 'C') {
            hub_screen_equipment(player);
        } else if (ch == 'i' || ch == 'I') {
            hub_screen_inventory(player);
        } else if (ch == 'q' || ch == 'Q') {
            if (hub_confirm_quit()) return GAME_STATE_QUIT;
        }
    }
}
