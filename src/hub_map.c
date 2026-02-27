#include "hub_map.h"
#include "hub.h"
#include "render.h"
#include "map.h"
#include "entity.h"

#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Message inbox                                                        */
/* ------------------------------------------------------------------ */

#define MAX_HUB_MESSAGES  8
#define HUB_MSG_BODY_LEN 120

typedef struct {
    char sender[32];
    char body[HUB_MSG_BODY_LEN];
    bool read;
} HubMsg;

static HubMsg hub_msgs[MAX_HUB_MESSAGES];
static int    hub_msg_count = 0;

void hub_message_add(const char *sender, const char *body)
{
    if (hub_msg_count >= MAX_HUB_MESSAGES) {
        memmove(&hub_msgs[0], &hub_msgs[1],
                sizeof(HubMsg) * (MAX_HUB_MESSAGES - 1));
        hub_msg_count = MAX_HUB_MESSAGES - 1;
    }
    HubMsg *m = &hub_msgs[hub_msg_count++];
    strncpy(m->sender, sender, sizeof(m->sender) - 1);
    m->sender[sizeof(m->sender) - 1] = '\0';
    strncpy(m->body, body, sizeof(m->body) - 1);
    m->body[sizeof(m->body) - 1] = '\0';
    m->read = false;
}

/* ------------------------------------------------------------------ */
/* News ticker                                                          */
/* ------------------------------------------------------------------ */

static const char *ticker_msgs[] = {
    ">> Corp drone sweeps reported across Sector 7. Avoid the district. >>",
    ">> Arasaka stock up 14% following Eastside lockdown. Market stable. >>",
    ">> NovaCorp denies involvement in Undercity data breach. Sources doubt it. >>",
    ">> Orbital Relay Station Alpha goes dark. Corp comms unresponsive. >>",
    ">> Seawall Compound radio silence — jammers active, runners warned. >>",
    ">> Ghost protocol breach logged at three facilities this week alone. >>",
    ">> Runner on corp surveillance feeds. Bounty set at 50k credits. >>",
    ">> Citywide curfew extended past midnight in the Sprawl. Patrols up. >>",
    ">> Power grid fluctuations in mid-city blamed on 'weather'. Sure. >>",
    ">> Street doc arrested in midtown sting. Clinic closed until further notice. >>",
    ">> Three runners went dark last week. No recovery teams dispatched. >>",
    ">> Militech arms shipment hijacked en route to the eastern port district. >>",
    ">> Corporate tribunal convenes. Retaliatory sweeps expected this cycle. >>",
    ">> New ICE variant detected in the Gridspace. Netrunners advised to patch. >>",
    ">> Credits flowing south — the corps are acquiring something big. >>",
    ">> Anonymous tip: server farm-7 appears understaffed on night cycle. >>",
    ">> Two senior fixers found dead. Their outstanding contracts are up. >>",
    ">> Stay off the Eastside grid tonight. Repeat: stay off the grid. >>",
    ">> Undercity black market raided. Some vendors relocated. Shop carefully. >>",
    ">> NovaCorp announces AI-assisted security rollout across all facilities. >>",
};
#define TICKER_COUNT ((int)(sizeof(ticker_msgs) / sizeof(ticker_msgs[0])))

/* ------------------------------------------------------------------ */
/* Safe-house bed position                                              */
/* ------------------------------------------------------------------ */

#define HUB_BED_X  80
#define HUB_BED_Y  14

/* ------------------------------------------------------------------ */
/* Camera (mirrors the static in main.c)                               */
/* ------------------------------------------------------------------ */

static void compute_camera(int px, int py, int *cam_x, int *cam_y)
{
    int view_w = COLS;
    int view_h = LINES - 2;
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
/* Hub city layout                                                      */
/* ------------------------------------------------------------------ */
/*
 *  Street grid (TILE_FLOOR):
 *    V1 x=3-5    V2 x=33-35   V3 x=70-72   V4 x=111-113  V5 x=150-152
 *    All vertical streets run y=3-68.
 *
 *    North alley:  y=19-21  x=3-155
 *    Main street:  y=37-40  x=3-155  (4 wide)
 *    South alley:  y=54-56  x=3-155
 *
 *  TOP ROW  (interior y=5-17, south wall y=18, door → north alley y=19-21):
 *    ARMS DEALER   x=6-32    NPC VIVID  (V, red)
 *    RIPPERDOC     x=36-69   NPC DOC WIRE (D, cyan)   SW notch
 *    SAFE HOUSE    x=73-110  NPC terminal (91,10)     NE notch  BED (80,14)
 *    TECH DEALER   x=114-149 NPC MOTH   (M, blue)
 *
 *  MID ROW  (interior y=23-35, walls y=22 and y=36):
 *    STREET DOC    x=6-32    NPC REY    (R, green)
 *    OPEN PLAZA    x=36-69   NPC SAL    (@, gray) wanders
 *    FIXER         x=73-110  NPC J-BONE (J, yellow)
 *    FENCE         x=114-149 NPC FRIX   (F, magenta)  back room x=143-149
 *
 *  SOUTH ROW  (interior y=42-52, north wall y=41):
 *    HANGOUT A     x=6-32    NPC GHOST  (@, gray) wanders
 *    HANGOUT B     x=36-69   NPC KIRA   (@, gray) wanders
 *    HANGOUT C     x=73-100  NPC V      (@, gray) wanders  side alcove x=102-110
 *
 *  DEEP SOUTH  (interior y=58-68, north wall y=57):
 *    WEST ANNEX    x=6-32    (no NPC — open space)
 *    CHROME LOUNGE x=36-69   NPC RICO   (R, orange) bartender
 *    ZERO'S DEN    x=73-108  NPC ZERO   (Z, purple) intel broker
 *    EAST ANNEX    x=114-149 NPC CIPHER (@, gray) wanders
 *
 *  Player spawn: x=91, y=38  (main street, FIXER entrance)
 */

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
    fill_rect(map, 0, 0, MAP_WIDTH - 1, MAP_HEIGHT - 1, TILE_WALL);

    /* ==== Vertical streets (y=3-68) ==== */
    fill_rect(map,   3,  3,   5, 68, TILE_FLOOR);
    fill_rect(map,  33,  3,  35, 68, TILE_FLOOR);
    fill_rect(map,  70,  3,  72, 68, TILE_FLOOR);
    fill_rect(map, 111,  3, 113, 68, TILE_FLOOR);
    fill_rect(map, 150,  3, 152, 68, TILE_FLOOR);

    /* ==== Horizontal streets ==== */
    fill_rect(map,   3, 19, 155, 21, TILE_FLOOR);  /* North alley  y=19-21 */
    fill_rect(map,   3, 37, 155, 40, TILE_FLOOR);  /* Main street  y=37-40 */
    fill_rect(map,   3, 54, 155, 56, TILE_FLOOR);  /* South alley  y=54-56 */

    /* ==== TOP ROW (interior y=5-17; south wall y=18) ==== */

    /* ARMS DEALER */
    fill_rect(map,  6,  5, 32, 17, TILE_FLOOR);
    fill_rect(map, 17, 18, 21, 18, TILE_FLOOR);

    /* RIPPERDOC (SW notch) */
    fill_rect(map, 36,  5, 69, 17, TILE_FLOOR);
    fill_rect(map, 36,  5, 43, 10, TILE_WALL);
    fill_rect(map, 50, 18, 54, 18, TILE_FLOOR);

    /* SAFE HOUSE (NE notch) */
    fill_rect(map,  73,  5, 110, 17, TILE_FLOOR);
    fill_rect(map, 103,  5, 110,  9, TILE_WALL);
    fill_rect(map,  89, 18,  93, 18, TILE_FLOOR);

    /* TECH DEALER */
    fill_rect(map, 114,  5, 149, 17, TILE_FLOOR);
    fill_rect(map, 129, 18, 133, 18, TILE_FLOOR);

    /* ==== MID ROW (interior y=23-35; walls y=22 and y=36) ==== */

    /* STREET DOC */
    fill_rect(map,  6, 23, 32, 35, TILE_FLOOR);
    fill_rect(map, 17, 22, 21, 22, TILE_FLOOR);
    fill_rect(map, 17, 36, 21, 36, TILE_FLOOR);

    /* OPEN PLAZA (fully open north + south; decorative pillars) */
    fill_rect(map, 36, 23, 69, 35, TILE_FLOOR);
    fill_rect(map, 36, 22, 69, 22, TILE_FLOOR);
    fill_rect(map, 36, 36, 69, 36, TILE_FLOOR);
    map->tiles[26][47].type = TILE_WALL;
    map->tiles[26][58].type = TILE_WALL;
    map->tiles[32][47].type = TILE_WALL;
    map->tiles[32][58].type = TILE_WALL;

    /* FIXER */
    fill_rect(map,  73, 23, 110, 35, TILE_FLOOR);
    fill_rect(map,  89, 22,  93, 22, TILE_FLOOR);
    fill_rect(map,  89, 36,  93, 36, TILE_FLOOR);

    /* FENCE + back room */
    fill_rect(map, 114, 23, 140, 35, TILE_FLOOR);
    fill_rect(map, 143, 27, 149, 34, TILE_FLOOR);
    fill_rect(map, 141, 30, 142, 32, TILE_FLOOR);
    fill_rect(map, 129, 22, 133, 22, TILE_FLOOR);
    fill_rect(map, 129, 36, 133, 36, TILE_FLOOR);

    /* ==== SOUTH ROW (interior y=42-52; north wall y=41) ==== */

    /* HANGOUT A */
    fill_rect(map,  6, 42, 32, 52, TILE_FLOOR);
    fill_rect(map, 17, 41, 21, 41, TILE_FLOOR);

    /* HANGOUT B */
    fill_rect(map, 36, 42, 69, 52, TILE_FLOOR);
    fill_rect(map, 51, 41, 55, 41, TILE_FLOOR);

    /* HANGOUT C + side alcove */
    fill_rect(map,  73, 42, 100, 52, TILE_FLOOR);
    fill_rect(map,  89, 41,  92, 41, TILE_FLOOR);
    fill_rect(map, 102, 46, 110, 51, TILE_FLOOR);
    fill_rect(map, 101, 47, 101, 50, TILE_FLOOR);

    /* ==== DEEP SOUTH (interior y=58-68; north wall y=57) ==== */

    /* WEST ANNEX — open space */
    fill_rect(map,   6, 58,  32, 68, TILE_FLOOR);
    fill_rect(map,  17, 57,  21, 57, TILE_FLOOR);

    /* CHROME LOUNGE (bar) */
    fill_rect(map,  36, 58,  69, 68, TILE_FLOOR);
    fill_rect(map,  50, 57,  54, 57, TILE_FLOOR);

    /* ZERO'S DEN (intel broker) */
    fill_rect(map,  73, 58, 108, 68, TILE_FLOOR);
    fill_rect(map,  89, 57,  93, 57, TILE_FLOOR);

    /* EAST ANNEX — back-alley space */
    fill_rect(map, 114, 58, 149, 68, TILE_FLOOR);
    fill_rect(map, 129, 57, 133, 57, TILE_FLOOR);

    /* ---- Fixtures ---- */
    /* Skill terminal in safe house */
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
        np->wx1 = np->wy1 = np->wx2 = np->wy2 = 0; \
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

#define SET_WANDER(x1_, y1_, x2_, y2_) \
    do { \
        npcs[n - 1].wx1 = (x1_); npcs[n - 1].wy1 = (y1_); \
        npcs[n - 1].wx2 = (x2_); npcs[n - 1].wy2 = (y2_); \
    } while (0)

    /* ----- Functional NPCs (stationary) ----- */

    /* VIVID — arms dealer */
    MAKE_NPC(18, 12, 'V', COL_HUB_ARMS, "VIVID", HUB_NPC_ARMS);
    ADD_DLG("Heat's running high in the Sprawl tonight.");
    ADD_DLG("Off-registry hardware, clean and ready to go.");
    ADD_DLG("Take a look at the inventory.");

    /* DOC WIRE — ripperdoc */
    MAKE_NPC(52, 12, 'D', COL_HUB_RIPPERDOC, "DOC WIRE", HUB_NPC_RIPPERDOC);
    ADD_DLG("Welcome to the Clinic. Cash only, no questions.");
    ADD_DLG("Latest chrome arrived this morning. Top-shelf implants.");
    ADD_DLG("Let me show you what's on the table.");

    /* MOTH — tech dealer */
    MAKE_NPC(130, 12, 'M', COL_HUB_TECH, "MOTH", HUB_NPC_TECH);
    ADD_DLG("Corp-grade armor and decks at runner prices.");
    ADD_DLG("Don't ask where it came from. Just enjoy the discount.");
    ADD_DLG("Browse the racks.");

    /* REY — street doc */
    MAKE_NPC(18, 29, 'R', COL_HUB_MEDIC, "REY", HUB_NPC_MEDIC);
    ADD_DLG("You look like you could use a patch-up.");
    ADD_DLG("Stim packs, med kits, full trauma rigs. I keep runners alive.");
    ADD_DLG("What do you need?");

    /* J-BONE — fixer */
    MAKE_NPC(91, 29, 'J', COL_HUB_FIXER, "J-BONE", HUB_NPC_FIXER);
    ADD_DLG("J-Bone here. Got three contracts, all paying clean credits.");
    ADD_DLG("Don't get dead — hard to collect my cut from a corpse.");
    ADD_DLG("See what's on the board.");

    /* FRIX — fence */
    MAKE_NPC(126, 29, 'F', COL_HUB_FENCE, "FRIX", HUB_NPC_FENCE);
    ADD_DLG("Got gear burning a hole in your bag? I pay fair.");
    ADD_DLG("Corp-stamped items especially welcome. No serial numbers.");
    ADD_DLG("Lay it out on the table.");

    /* RICO — bartender (Chrome Lounge) */
    MAKE_NPC(52, 63, 'B', COL_HUB_BAR, "RICO", HUB_NPC_BAR);
    ADD_DLG("Welcome to the Chrome Lounge. Best drinks in the Sprawl.");
    ADD_DLG("Don't ask for a menu. Just tell me how wrecked you are.");
    ADD_DLG("Something from the shelf?");

    /* ZERO — info broker (Zero's Den) */
    MAKE_NPC(91, 63, 'Z', COL_HUB_INTEL, "ZERO", HUB_NPC_INTEL);
    ADD_DLG("Data's my currency. I don't sell rumors — I sell truth.");
    ADD_DLG("For the right price I can tell you exactly what's waiting.");
    ADD_DLG("What do you need to know?");

    /* ----- Ambient NPCs (wander within their home zone) ----- */

    /* SAL — Open Plaza */
    MAKE_NPC(52, 29, '@', COL_HUB_AMBIENT, "SAL", HUB_NPC_AMBIENT);
    ADD_DLG("Heard Arasaka locked down the whole Eastside.");
    ADD_DLG("ICE everywhere. Don't run that district tonight.");
    SET_WANDER(37, 24, 68, 34);

    /* GHOST — Hangout A */
    MAKE_NPC(18, 47, '@', COL_HUB_AMBIENT, "GHOST", HUB_NPC_AMBIENT);
    ADD_DLG("Someone's been watching the Arms District real close.");
    ADD_DLG("Corps or a rival crew — either way, watch your back.");
    SET_WANDER(7, 43, 31, 51);

    /* KIRA — Hangout B */
    MAKE_NPC(52, 47, '@', COL_HUB_AMBIENT, "KIRA", HUB_NPC_AMBIENT);
    ADD_DLG("Three jobs this week. I'm basically a walking stim pack.");
    ADD_DLG("The Sprawl doesn't slow down, so neither do I.");
    SET_WANDER(37, 43, 68, 51);

    /* V — Hangout C */
    MAKE_NPC(86, 47, '@', COL_HUB_AMBIENT, "V", HUB_NPC_AMBIENT);
    ADD_DLG("I know a guy who knows a guy.");
    ADD_DLG("Off-grid transport, no corp tags. If you ever need it.");
    SET_WANDER(74, 43, 99, 51);

    /* CIPHER — East Annex */
    MAKE_NPC(130, 63, '@', COL_HUB_AMBIENT, "CIPHER", HUB_NPC_AMBIENT);
    ADD_DLG("I don't exist. You never saw me.");
    ADD_DLG("The east grid's compromised. Don't jack in there.");
    SET_WANDER(115, 59, 148, 67);

#undef MAKE_NPC
#undef ADD_DLG
#undef SET_WANDER

    return n;
}

/* ------------------------------------------------------------------ */
/* Ambient NPC wandering                                                */
/* ------------------------------------------------------------------ */

static void hub_wander_npcs(HubNpc *npcs, int count, const Map *map)
{
    static const int dx[] = { 0, 0, -1, 1, -1, 1, -1, 1 };
    static const int dy[] = { -1, 1, 0, 0, -1, -1, 1, 1 };

    for (int i = 0; i < count; i++) {
        HubNpc *npc = &npcs[i];
        if (npc->wx1 == 0 && npc->wx2 == 0) continue;  /* stationary */

        int dir = rand() % 8;
        for (int t = 0; t < 8; t++) {
            int d  = (dir + t) % 8;
            int nx = npc->x + dx[d];
            int ny = npc->y + dy[d];
            if (nx < npc->wx1 || nx > npc->wx2) continue;
            if (ny < npc->wy1 || ny > npc->wy2) continue;
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (map->tiles[ny][nx].type != TILE_FLOOR) continue;
            bool occupied = false;
            for (int j = 0; j < count; j++) {
                if (j != i && npcs[j].x == nx && npcs[j].y == ny) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) {
                npc->x = nx;
                npc->y = ny;
                break;
            }
        }
    }
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

/* Bed marker in safe house */
static void render_hub_fixtures(int cam_x, int cam_y)
{
    int view_h = LINES - 2;
    int bsx = HUB_BED_X - cam_x;
    int bsy = HUB_BED_Y - cam_y;
    if (bsx >= 0 && bsx < COLS && bsy >= 0 && bsy < view_h) {
        attron(COLOR_PAIR(COL_MENU_DIM));
        mvaddch(bsy, bsx, '~');
        attroff(COLOR_PAIR(COL_MENU_DIM));
    }
}

/* Location labels */
static void render_hub_labels(int cam_x, int cam_y)
{
    static const struct { int wx, wy; const char *text; } labels[] = {
        /* Top row */
        {  7,  5,  "ARMS DEALER"   },
        { 37,  5,  "RIPPERDOC"     },
        { 74,  5,  "SAFE HOUSE"    },
        {115,  5,  "TECH DEALER"   },
        /* Mid row */
        {  7, 23,  "STREET DOC"    },
        { 40, 23,  ":: PLAZA ::"   },
        { 74, 23,  "FIXER"         },
        {115, 23,  "FENCE"         },
        /* South row */
        {  7, 42,  "HANGOUT"       },
        { 37, 42,  "HANGOUT"       },
        { 74, 42,  "HANGOUT"       },
        /* Deep south */
        {  7, 58,  "WEST ANNEX"    },
        { 37, 58,  "CHROME LOUNGE" },
        { 74, 58,  "ZERO'S DEN"    },
        {115, 58,  "EAST ANNEX"    },
        /* Street / alley signs */
        { 55, 20,  "NEON ALLEY"    },
        { 65, 38,  "CHROME STRIP"  },
        { 55, 55,  "BACK ALLEY"    },
        { 55, 66,  "LOWER DISTRICT"},
        /* Safe house interior label for bed */
        { 77, 15,  "[BED]"         },
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
                               bool near_term, bool near_bed,
                               int ticker_idx, const char *msg)
{
    /* Count unread messages */
    int unread = 0;
    for (int i = 0; i < hub_msg_count; i++)
        if (!hub_msgs[i].read) unread++;

    /* Line 1: stats + message indicator */
    attron(COLOR_PAIR(COL_STATUS) | A_BOLD);
    mvprintw(LINES - 2, 0, "HP:%d/%d  \xc2\xa2%d  LVL:%d  XP:%d/%d  SP:%d",
             player->hp, player->max_hp,
             player->credits,
             player->level,
             player->xp, player->xp_next,
             player->skill_points);
    attroff(COLOR_PAIR(COL_STATUS) | A_BOLD);
    if (unread > 0) {
        attron(COLOR_PAIR(COL_GUARD_SUSPICIOUS) | A_BOLD);
        printw("  [MSG:%d]", unread);
        attroff(COLOR_PAIR(COL_GUARD_SUSPICIOUS) | A_BOLD);
    }
    clrtoeol();

    /* Line 2: interaction prompt, feedback, or ticker */
    move(LINES - 1, 0);
    clrtoeol();
    if (nearby) {
        attron(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        mvprintw(LINES - 1, 0,
                 "[E] Talk to %-10s  [C] Equip  [I] Inv  [M] Msgs  [Q] Quit",
                 nearby->name);
        attroff(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    } else if (near_term) {
        attron(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        mvprintw(LINES - 1, 0,
                 "[E] Access terminal  [C] Equip  [I] Inv  [M] Msgs  [Q] Quit");
        attroff(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    } else if (near_bed) {
        attron(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
        mvprintw(LINES - 1, 0,
                 "[E] Rest (restore HP)  [C] Equip  [I] Inv  [M] Msgs  [Q] Quit");
        attroff(COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    } else if (msg && msg[0]) {
        attron(COLOR_PAIR(COL_MENU_DIM));
        mvprintw(LINES - 1, 0, "%-.*s", COLS - 1, msg);
        attroff(COLOR_PAIR(COL_MENU_DIM));
    } else {
        /* News ticker */
        attron(COLOR_PAIR(COL_MENU_DIM));
        mvprintw(LINES - 1, 0, "%-.*s", COLS - 1, ticker_msgs[ticker_idx]);
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

static bool hub_adjacent_bed(int px, int py)
{
    int ddx = px - HUB_BED_X;
    int ddy = py - HUB_BED_Y;
    return ddx >= -1 && ddx <= 1 && ddy >= -1 && ddy <= 1
           && (ddx != 0 || ddy != 0);
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

    char hdr[36];
    snprintf(hdr, sizeof(hdr), " %s ", npc->name);
    wattron(win, COLOR_PAIR(npc->color_pair) | A_BOLD);
    mvwprintw(win, 0, (dw - (int)strlen(hdr)) / 2, "%s", hdr);
    wattroff(win, COLOR_PAIR(npc->color_pair) | A_BOLD);

    for (int i = 0; i < npc->dialog_count; i++) {
        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, 1 + i, 2, "%-*.*s", dw - 4, dw - 4, npc->dialog[i]);
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));
    }

    wattron(win, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(win, dh - 1, (dw - 22) / 2, "-- press any key --");
    wattroff(win, COLOR_PAIR(COL_MENU_DIM));

    wrefresh(win);
    wgetch(win);
    delwin(win);
}

/* ------------------------------------------------------------------ */
/* Bar interaction (RICO)                                              */
/* ------------------------------------------------------------------ */

static void hub_bar_interact(Entity *player, char *msg)
{
    int dw = 50, dh = 11;
    int dy = (LINES - dh) / 2;
    int dx = (COLS  - dw) / 2;
    if (dx < 0) dx = 0;

    WINDOW *win = newwin(dh, dw, dy, dx);
    keypad(win, TRUE);
    box(win, 0, 0);

    wattron(win, COLOR_PAIR(COL_HUB_BAR) | A_BOLD);
    mvwprintw(win, 0, (dw - 16) / 2, " CHROME LOUNGE ");
    wattroff(win, COLOR_PAIR(COL_HUB_BAR) | A_BOLD);

    wattron(win, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(win, 2, 2, "RICO: \"What'll it be, runner?\"");
    mvwprintw(win, 4, 4, "[ A ]  Shot of Chrome  (10 HP)  \xc2\xa2" "50");
    mvwprintw(win, 5, 4, "[ B ]  Street Special  (20 HP)  \xc2\xa2" "100");
    mvwprintw(win, 6, 4, "[ C ]  Full Round      (full HP) \xc2\xa2" "200");
    mvwprintw(win, 7, 4, "[ X ]  Nothing, thanks");
    mvwprintw(win, dh - 2, 2, "HP: %d/%d", player->hp, player->max_hp);
    wattroff(win, COLOR_PAIR(COL_MENU_DIM));

    wrefresh(win);

    typedef struct { int heal; int cost; const char *label; } DrinkOpt;
    static const DrinkOpt drinks[] = {
        { 10, 50,  "Shot of Chrome" },
        { 20, 100, "Street Special" },
        { 0,  200, "Full Round"     },  /* heal = 0 means full */
    };

    while (1) {
        int ch = wgetch(win);
        int idx = -1;
        if (ch == 'a' || ch == 'A') idx = 0;
        if (ch == 'b' || ch == 'B') idx = 1;
        if (ch == 'c' || ch == 'C') idx = 2;
        if (ch == 'x' || ch == 'X' || ch == 'q' || ch == 27) break;
        if (idx < 0) continue;

        const DrinkOpt *d = &drinks[idx];
        if (player->credits < d->cost) {
            snprintf(msg, 128, "Not enough credits for the %s.", d->label);
        } else if (player->hp >= player->max_hp) {
            snprintf(msg, 128, "Already at full health, runner.");
        } else {
            player->credits -= d->cost;
            if (d->heal == 0)
                player->hp = player->max_hp;
            else {
                player->hp += d->heal;
                if (player->hp > player->max_hp)
                    player->hp = player->max_hp;
            }
            snprintf(msg, 128, "Rico slides over the %s.  HP restored.", d->label);
        }
        break;
    }
    delwin(win);
}

/* ------------------------------------------------------------------ */
/* Intel interaction (ZERO)                                            */
/* ------------------------------------------------------------------ */

static void hub_intel_interact(Entity *player, char *msg)
{
    int dw = 54, dh = 14;
    int dy = (LINES - dh) / 2;
    int dx = (COLS  - dw) / 2;
    if (dx < 0) dx = 0;

    WINDOW *win = newwin(dh, dw, dy, dx);
    keypad(win, TRUE);
    box(win, 0, 0);

    wattron(win, COLOR_PAIR(COL_HUB_INTEL) | A_BOLD);
    mvwprintw(win, 0, (dw - 12) / 2, " ZERO'S DEN ");
    wattroff(win, COLOR_PAIR(COL_HUB_INTEL) | A_BOLD);

    wattron(win, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(win, 2, 2, "ZERO: \"Data's my currency. What do you need?\"");
    wattroff(win, COLOR_PAIR(COL_MENU_DIM));

    int diff = player->active_mission_difficulty;

    if (diff == 0) {
        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, 4, 2, "\"No active contract on your tag. Talk to");
        mvwprintw(win, 5, 2, " J-Bone first, then come back to me.\"");
        mvwprintw(win, dh - 2, 2, "[any key] close");
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));
        wrefresh(win);
        wgetch(win);
    } else {
        /* Estimates based on difficulty */
        int est_guards = 2 + diff;
        int est_floors = 1 + diff / 2;
        static const char *ftypes[] = {
            "low-security office block",
            "mid-tier data center",
            "corporate server farm",
            "hardened research compound",
            "black-site facility",
        };
        const char *ftype = ftypes[(diff - 1) % 5];
        bool has_drones = (diff >= 3);

        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, 4, 2, "[ A ]  Scouting report  \xc2\xa2" "150");
        mvwprintw(win, 5, 2, "[ X ]  Nothing, thanks");
        mvwprintw(win, dh - 2, 2, "[A/X] choose");
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));
        wrefresh(win);

        while (1) {
            int ch = wgetch(win);
            if (ch == 'a' || ch == 'A') {
                if (player->credits < 150) {
                    snprintf(msg, 128, "Not enough credits for the scouting report.");
                } else {
                    player->credits -= 150;
                    werase(win);
                    box(win, 0, 0);
                    wattron(win, COLOR_PAIR(COL_HUB_INTEL) | A_BOLD);
                    mvwprintw(win, 0, (dw - 12) / 2, " ZERO'S DEN ");
                    wattroff(win, COLOR_PAIR(COL_HUB_INTEL) | A_BOLD);
                    wattron(win, COLOR_PAIR(COL_MENU_DIM));
                    mvwprintw(win, 2, 2, "ZERO: \"Here's what I pulled.\"");
                    mvwprintw(win, 4, 2, "Target type : %s", ftype);
                    mvwprintw(win, 5, 2, "Est. patrols: ~%d guards", est_guards);
                    mvwprintw(win, 6, 2, "Est. floors : %d", est_floors);
                    if (has_drones)
                        mvwprintw(win, 7, 2, "WARNING      : corp drones detected on-site");
                    mvwprintw(win, dh - 3, 2, "\"Good luck out there, runner.\"");
                    mvwprintw(win, dh - 2, 2, "[any key] close");
                    wattroff(win, COLOR_PAIR(COL_MENU_DIM));
                    wrefresh(win);
                    wgetch(win);
                    snprintf(msg, 128, "Intel purchased. Credits deducted.");
                }
                break;
            } else if (ch == 'x' || ch == 'X' || ch == 'q' || ch == 27) {
                break;
            }
        }
    }
    delwin(win);
}

/* ------------------------------------------------------------------ */
/* Message inbox screen                                                 */
/* ------------------------------------------------------------------ */

static void hub_show_messages(void)
{
    int dw = 62, dh = 16;
    int dy = (LINES - dh) / 2;
    int dx = (COLS  - dw) / 2;
    if (dx < 0) dx = 0;

    WINDOW *win = newwin(dh, dw, dy, dx);
    keypad(win, TRUE);

    /* Mark all read */
    for (int i = 0; i < hub_msg_count; i++)
        hub_msgs[i].read = true;

    int sel   = 0;
    bool dirty = true;

    while (1) {
        if (dirty) {
            werase(win);
            box(win, 0, 0);
            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, 0, (dw - 11) / 2, " MESSAGES ");
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            if (hub_msg_count == 0) {
                wattron(win, COLOR_PAIR(COL_MENU_DIM));
                mvwprintw(win, dh / 2, (dw - 18) / 2, "No messages yet.");
                wattroff(win, COLOR_PAIR(COL_MENU_DIM));
            } else {
                int list_rows = dh - 7;
                for (int i = 0; i < hub_msg_count && i < list_rows; i++) {
                    bool is_sel = (i == sel);
                    attr_t attr = is_sel
                        ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                        : COLOR_PAIR(COL_MENU_DIM);
                    wattron(win, attr);
                    mvwprintw(win, 2 + i, 2, "%-9.9s  %-*.*s",
                              hub_msgs[i].sender,
                              dw - 14, dw - 14, hub_msgs[i].body);
                    wattroff(win, attr);
                }
                /* Selected message detail */
                if (sel < hub_msg_count) {
                    wattron(win, COLOR_PAIR(COL_MENU_DIM));
                    mvwhline(win, dh - 5, 1, ACS_HLINE, dw - 2);
                    mvwprintw(win, dh - 4, 2, "From: %s", hub_msgs[sel].sender);
                    mvwprintw(win, dh - 3, 2, "%-*.*s",
                              dw - 4, dw - 4, hub_msgs[sel].body);
                    wattroff(win, COLOR_PAIR(COL_MENU_DIM));
                }
            }

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, dh - 1, (dw - 30) / 2, "[j/k] scroll  [Esc/q] close");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));
            wrefresh(win);
            dirty = false;
        }

        int ch = wgetch(win);
        switch (ch) {
        case KEY_UP:   case 'k':
            if (sel > 0) { sel--; dirty = true; }
            break;
        case KEY_DOWN: case 'j':
            if (sel < hub_msg_count - 1) { sel++; dirty = true; }
            break;
        case 'q': case 27: case 'm': case 'M':
            delwin(win);
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Terminal mini-menu (safe house terminal)                            */
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
            if (sel == OPT_SKILLS)         hub_levelup_menu(player);
            else if (sel == OPT_EQUIPMENT) hub_screen_equipment(player);
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

static GameState hub_interact_npc(Entity *player, HubNpc *npc, char *msg)
{
    hub_show_dialog(npc);

    switch (npc->type) {
    case HUB_NPC_ARMS:      hub_shop_weapons(player);     break;
    case HUB_NPC_RIPPERDOC: hub_shop_cybernetics(player); break;
    case HUB_NPC_FIXER:     return hub_screen_missions(player);
    case HUB_NPC_TECH:      hub_shop_tech(player);        break;
    case HUB_NPC_FENCE:     hub_shop_fence(player);       break;
    case HUB_NPC_MEDIC:     hub_shop_consumables(player); break;
    case HUB_NPC_BAR:       hub_bar_interact(player, msg); break;
    case HUB_NPC_INTEL:     hub_intel_interact(player, msg); break;
    case HUB_NPC_AMBIENT:   break;
    }
    return GAME_STATE_HUB;
}

/* ------------------------------------------------------------------ */
/* Main hub map loop                                                    */
/* ------------------------------------------------------------------ */

GameState hub_map_run(Entity *player)
{
    static Map    map;
    static HubNpc npcs[HUB_NPC_MAX];

    map_generate_hub(&map);

    /* Hub: always fully visible */
    for (int y = 0; y < MAP_HEIGHT; y++)
        for (int x = 0; x < MAP_WIDTH; x++) {
            map.tiles[y][x].visible = true;
            map.tiles[y][x].seen    = true;
        }

    int npc_count = hub_npcs_init(npcs);

    player->x = HUB_START_X;
    player->y = HUB_START_Y;

    char msg[128] = "";
    int  cam_x, cam_y;
    int  hub_turn   = 0;
    int  ticker_idx = rand() % TICKER_COUNT;  /* start on random ticker */

    /* Show message notification on entry if unread messages exist */
    {
        int unread = 0;
        for (int i = 0; i < hub_msg_count; i++)
            if (!hub_msgs[i].read) unread++;
        if (unread > 0)
            snprintf(msg, sizeof(msg),
                     "You have %d new message%s. Press [M] to read.",
                     unread, unread == 1 ? "" : "s");
    }

    while (1) {
        compute_camera(player->x, player->y, &cam_x, &cam_y);

        clear();
        render_map(&map, cam_x, cam_y);
        render_hub_labels(cam_x, cam_y);
        render_hub_fixtures(cam_x, cam_y);
        render_hub_npcs(npcs, npc_count, cam_x, cam_y);
        render_entity(player, cam_x, cam_y);

        HubNpc *nearby    = hub_adjacent_npc(npcs, npc_count, player->x, player->y);
        bool    near_term = hub_adjacent_terminal(&map, player->x, player->y);
        bool    near_bed  = hub_adjacent_bed(player->x, player->y);

        render_hub_status(player, nearby, near_term, near_bed, ticker_idx, msg);
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
            hub_turn++;
            if (hub_turn % 5  == 0) hub_wander_npcs(npcs, npc_count, &map);
            if (hub_turn % 10 == 0) ticker_idx = (ticker_idx + 1) % TICKER_COUNT;
            continue;
        }

        /* Interaction */
        if (ch == 'e' || ch == '\n' || ch == KEY_ENTER) {
            if (nearby) {
                GameState next = hub_interact_npc(player, nearby, msg);
                if (next != GAME_STATE_HUB) return next;
            } else if (near_term) {
                hub_terminal_interact(player);
            } else if (near_bed) {
                player->hp = player->max_hp;
                snprintf(msg, sizeof(msg),
                         "You rest on the cot. HP fully restored.");
            }
        } else if (ch == 'c' || ch == 'C') {
            hub_screen_equipment(player);
        } else if (ch == 'i' || ch == 'I') {
            hub_screen_inventory(player);
        } else if (ch == 'm' || ch == 'M') {
            hub_show_messages();
        } else if (ch == 'q' || ch == 'Q') {
            if (hub_confirm_quit()) return GAME_STATE_QUIT;
        }
    }
}
