#include "hub.h"
#include "menu.h"
#include "render.h"
#include "skilltree.h"

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Shop items                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    const char *desc;
    int         cost;
    SkillType   skill;
    int         bonus;
    bool        sold;
} ShopItem;

static ShopItem shop_items[] = {
    { "Neural Booster",   "+1 Hacking",     300, SKILL_HACKING,     1, false },
    { "Ghost Wrap",       "+1 Stealth",     350, SKILL_STEALTH,     1, false },
    { "Reflex Splice",    "+1 Athletics",   400, SKILL_ATHLETICS,   1, false },
    { "Combat Stims",     "+1 Combat",      450, SKILL_COMBAT,      1, false },
    { "Eng. Toolkit",     "+1 Engineering", 250, SKILL_ENGINEERING, 1, false },
};
#define SHOP_COUNT ((int)(sizeof(shop_items) / sizeof(shop_items[0])))

/* ------------------------------------------------------------------ */
/* Gear shop catalog                                                    */
/* ------------------------------------------------------------------ */

/* Fields: name, type, rarity, value, stat_bonus, range, mod_count, mods[] */
static Item shop_gear[] = {
    /* ---- WEAPONS --------------------------------------------------- */
    /* Melee / sidearms */
    { "Knuckle Dusters",  ITEM_WEAPON, RARITY_COMMON,     150, 1,  0, 0, {{0}} },
    { "Cheap Pistol",     ITEM_WEAPON, RARITY_COMMON,     200, 1,  0, 0, {{0}} },
    { "Ceramic Knife",    ITEM_WEAPON, RARITY_COMMON,     250, 1,  0, 1, {{ 0, 1, "+1 STL" }} },
    { "Baton",            ITEM_WEAPON, RARITY_COMMON,     300, 2,  0, 0, {{0}} },
    { "Combat Knife",     ITEM_WEAPON, RARITY_COMMON,     300, 2,  0, 0, {{0}} },
    { "Revolver",         ITEM_WEAPON, RARITY_COMMON,     350, 2,  0, 0, {{0}} },
    { "Silenced Pistol",  ITEM_WEAPON, RARITY_UNCOMMON,   375, 1,  0, 1, {{ 0, 1, "+1 STL" }} },
    { "Combat Pistol",    ITEM_WEAPON, RARITY_COMMON,     400, 2,  0, 0, {{0}} },
    { "Heavy Pistol",     ITEM_WEAPON, RARITY_UNCOMMON,   525, 3,  0, 0, {{0}} },
    { "Micro SMG",        ITEM_WEAPON, RARITY_UNCOMMON,   600, 3,  0, 0, {{0}} },
    { "SMG",              ITEM_WEAPON, RARITY_UNCOMMON,   700, 3,  0, 0, {{0}} },
    { "Katana",           ITEM_WEAPON, RARITY_RARE,       900, 4,  0, 0, {{0}} },
    { "Shotgun",          ITEM_WEAPON, RARITY_UNCOMMON,  1000, 4,  0, 0, {{0}} },
    { "Monowire",         ITEM_WEAPON, RARITY_RARE,      1300, 5,  0, 0, {{0}} },
    /* Ranged */
    { "Needler",          ITEM_WEAPON, RARITY_COMMON,     300, 1,  6, 1, {{ 0, 1, "+1 STL" }} },
    { "Flechette Gun",    ITEM_WEAPON, RARITY_UNCOMMON,   550, 2, 10, 0, {{0}} },
    { "Rail Pistol",      ITEM_WEAPON, RARITY_UNCOMMON,   700, 2,  8, 0, {{0}} },
    { "Smart Pistol",     ITEM_WEAPON, RARITY_RARE,       800, 3,  8, 1, {{ 1, 1, "+1 HCK" }} },
    { "Needle Rifle",     ITEM_WEAPON, RARITY_RARE,       950, 3, 14, 0, {{0}} },
    { "Sniper Rifle",     ITEM_WEAPON, RARITY_EPIC,      1600, 5, 18, 0, {{0}} },
    /* ---- HEAD ARMOUR ----------------------------------------------- */
    { "Street Helmet",    ITEM_ARMOR_HEAD, RARITY_COMMON,     150, 1, 0, 0, {{0}} },
    { "Urban Balaclava",  ITEM_ARMOR_HEAD, RARITY_COMMON,     200, 1, 0, 0, {{0}} },
    { "Infiltrator Hood", ITEM_ARMOR_HEAD, RARITY_COMMON,     320, 1, 0, 1, {{ 0, 1, "+1 STL" }} },
    { "Tactical Mask",    ITEM_ARMOR_HEAD, RARITY_COMMON,     300, 1, 0, 0, {{0}} },
    { "Combat Helm",      ITEM_ARMOR_HEAD, RARITY_UNCOMMON,   350, 2, 0, 0, {{0}} },
    { "Ballistic Visor",  ITEM_ARMOR_HEAD, RARITY_UNCOMMON,   450, 2, 0, 0, {{0}} },
    { "Mil-Spec Helmet",  ITEM_ARMOR_HEAD, RARITY_RARE,       600, 3, 0, 0, {{0}} },
    { "Cybernetic Crown", ITEM_ARMOR_HEAD, RARITY_RARE,       800, 3, 0, 1, {{ 1, 1, "+1 HCK" }} },
    /* ---- BODY ARMOUR ----------------------------------------------- */
    { "Runner's Vest",    ITEM_ARMOR_BODY, RARITY_COMMON,     200, 1, 0, 0, {{0}} },
    { "Leather Jacket",   ITEM_ARMOR_BODY, RARITY_COMMON,     200, 1, 0, 0, {{0}} },
    { "Corp Uniform",     ITEM_ARMOR_BODY, RARITY_COMMON,     250, 1, 0, 1, {{ 0, 1, "+1 STL" }} },
    { "Combat Jacket",    ITEM_ARMOR_BODY, RARITY_UNCOMMON,   450, 2, 0, 0, {{0}} },
    { "Tac Weave",        ITEM_ARMOR_BODY, RARITY_UNCOMMON,   500, 2, 0, 0, {{0}} },
    { "Ballistic Vest",   ITEM_ARMOR_BODY, RARITY_RARE,       750, 3, 0, 0, {{0}} },
    { "Exo-Partial",      ITEM_ARMOR_BODY, RARITY_RARE,       850, 3, 0, 1, {{ 2, 1, "+1 CMB" }} },
    { "Heavy Plate",      ITEM_ARMOR_BODY, RARITY_EPIC,      1200, 4, 0, 0, {{0}} },
    /* ---- LEG ARMOUR ------------------------------------------------ */
    { "Cargo Pants",      ITEM_ARMOR_LEGS, RARITY_COMMON,     150, 1, 0, 0, {{0}} },
    { "Urban Leggings",   ITEM_ARMOR_LEGS, RARITY_COMMON,     200, 1, 0, 0, {{0}} },
    { "Combat Pants",     ITEM_ARMOR_LEGS, RARITY_UNCOMMON,   350, 2, 0, 0, {{0}} },
    { "Stealth Runners",  ITEM_ARMOR_LEGS, RARITY_UNCOMMON,   450, 2, 0, 1, {{ 0, 1, "+1 STL" }} },
    { "Grav Boots",       ITEM_ARMOR_LEGS, RARITY_UNCOMMON,   500, 2, 0, 0, {{0}} },
    { "Tac. Greaves",     ITEM_ARMOR_LEGS, RARITY_RARE,       700, 3, 0, 0, {{0}} },
    { "Exo-Legs",         ITEM_ARMOR_LEGS, RARITY_RARE,       800, 3, 0, 0, {{0}} },
    /* ---- DECKS ----------------------------------------------------- */
    { "Junk Rig",         ITEM_DECK, RARITY_COMMON,     180, 1, 0, 0, {{0}} },
    { "Budget Deck",      ITEM_DECK, RARITY_COMMON,     250, 1, 0, 0, {{0}} },
    { "Standard Deck",    ITEM_DECK, RARITY_UNCOMMON,   500, 2, 0, 0, {{0}} },
    { "Street Deck",      ITEM_DECK, RARITY_UNCOMMON,   450, 2, 0, 0, {{0}} },
    { "Ghost Rig",        ITEM_DECK, RARITY_UNCOMMON,   700, 2, 0, 1, {{ 1, 1, "+1 HCK" }} },
    { "Assault Rig",      ITEM_DECK, RARITY_UNCOMMON,   750, 2, 0, 1, {{ 2, 1, "+1 CMB" }} },
    { "Pro Deck",         ITEM_DECK, RARITY_RARE,       900, 3, 0, 0, {{0}} },
    { "Phantom Deck",     ITEM_DECK, RARITY_RARE,      1100, 3, 0, 1, {{ 0, 1, "+1 STL" }} },
    { "ICE Breaker",      ITEM_DECK, RARITY_RARE,      1200, 3, 0, 1, {{ 1, 1, "+1 HCK" }} },
    { "Quantum Rig",      ITEM_DECK, RARITY_EPIC,      1600, 4, 0, 0, {{0}} },
    { "Neural Interface", ITEM_DECK, RARITY_EPIC,      1800, 4, 0, 1, {{ 0, 2, "+2 STL" }} },
    /* ---- CONSUMABLES (unlimited stock) ----------------------------- */
    { "Stim Pack",        ITEM_CONSUMABLE, RARITY_COMMON,    60, 2, 0, 0, {{0}} },
    { "Med Kit",          ITEM_CONSUMABLE, RARITY_COMMON,   150, 5, 0, 0, {{0}} },
    { "Trauma Kit",       ITEM_CONSUMABLE, RARITY_UNCOMMON, 280, 8, 0, 0, {{0}} },
};
#define SHOP_GEAR_COUNT ((int)(sizeof(shop_gear) / sizeof(shop_gear[0])))
static bool gear_sold[SHOP_GEAR_COUNT]; /* zeroed at startup; persists until restart */

/* ------------------------------------------------------------------ */
/* Mission board                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    const char *target;
    const char *location;
    int         difficulty; /* 1-5 */
    int         reward;
} Mission;

static const char *mission_verbs[]     = { "Extract", "Neutralise", "Steal data from", "Sabotage", "Surveil" };
static const char *mission_targets[]   = { "Director Krane", "Server Farm-7", "Biotech Lab", "Power Node Alpha", "Exec Kowalski" };
static const char *mission_locations[] = { "NovaCorp HQ", "Undercity Grid", "Seawall Compound", "Orbital Relay", "The Sprawl" };

#define MISSION_COUNT 3
static Mission missions[MISSION_COUNT];

static void missions_generate(void)
{
    for (int i = 0; i < MISSION_COUNT; i++) {
        missions[i].name     = mission_verbs[rand() % 5];
        missions[i].target   = mission_targets[rand() % 5];
        missions[i].location = mission_locations[rand() % 5];
        missions[i].difficulty = 1 + rand() % 5;
        missions[i].reward   = 200 + (missions[i].difficulty * 150) + rand() % 100;
    }
}

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

#define HUB_W  60
#define HUB_H  30
#define HUB_Y  ((MAP_HEIGHT - HUB_H) / 2)
#define HUB_X  ((MAP_WIDTH  - HUB_W) / 2)

static WINDOW *make_panel(const char *title)
{
    WINDOW *win = newwin(HUB_H, HUB_W, HUB_Y, HUB_X);
    keypad(win, TRUE);
    menu_draw_box(win, title);
    return win;
}

static bool confirm_quit(void)
{
    /* Small centered dialog */
    int dw = 36, dh = 7;
    int dy = (MAP_HEIGHT - dh) / 2;
    int dx = (MAP_WIDTH  - dw) / 2;
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

static void stars(WINDOW *win, int y, int x, int n)
{
    wattron(win, COLOR_PAIR(COL_MENU_SEL));
    for (int i = 0; i < 5; i++)
        mvwaddch(win, y, x + i, i < n ? '*' : '-');
    wattroff(win, COLOR_PAIR(COL_MENU_SEL));
}

static void hub_title(void)
{
    clear();
    attron(COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
    /* Simple banner centred on the background */
    int cx = MAP_WIDTH / 2;
    mvprintw(HUB_Y - 3, cx - 12, "S H A D O W G R I D");
    mvprintw(HUB_Y - 2, cx - 12, "// SAFE HOUSE //");
    attroff(COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
    refresh();
}

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static const char *stat_label(ItemType t)
{
    switch (t) {
    case ITEM_WEAPON:     return "ATK";
    case ITEM_ARMOR_HEAD: /* fall through */
    case ITEM_ARMOR_BODY: /* fall through */
    case ITEM_ARMOR_LEGS: return "DEF";
    case ITEM_DECK:       return "HCK";
    case ITEM_CONSUMABLE: return "HP ";
    default:              return "---";
    }
}

/* ------------------------------------------------------------------ */
/* Sub-screens                                                          */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Skill tree character screen                                          */
/* ------------------------------------------------------------------ */

/* Layout constants for the full-screen tree view (80 cols × 33 rows) */
#define ST_ROWS 33
#define ST_COLS 80

/* Column layout: col 0 at x=1 w=26, col 1 at x=28 w=26, col 2 at x=55 w=24 */
static const int col_x[TREE_COUNT] = { 1, 28, 55 };
static const int col_w[TREE_COUNT] = { 26, 26, 24 };
#define SEP_X0 27   /* vertical separator between col 0 and 1 */
#define SEP_X1 54   /* vertical separator between col 1 and 2 */

/* Row mapping:
 *   0         top border
 *   1         stats header
 *   2         hline
 *   3         tree name tabs
 *   4         hline with col dividers
 *   5+t*5     tier t header row  (t = 0..4)
 *   6+t*5+o   option o in tier t (o = 0..2)
 *   9+t*5     hline between tiers (after t = 0..3; after t=4: row 29 empty)
 *   29        hline (desc separator)
 *   30        description row
 *   31        hline
 *   32        controls / bottom border
 */
#define TIER_HDR_ROW(t)  (5 + (t) * 5)
#define OPT_ROW(t, o)    (6 + (t) * 5 + (o))
#define TIER_SEP_ROW(t)  (9 + (t) * 5)   /* separator AFTER tier t */
#define DESC_SEP_ROW     29
#define DESC_ROW         30
#define CTRL_SEP_ROW     31
#define CTRL_ROW         32

/* Max cursor value: tier 4 has 1 opt → cur_max = 4*3+0 = 12 */
#define CUR_MAX 12

static void st_hline(WINDOW *win, int row)
{
    /* Draw a full-width horizontal line with column dividers */
    mvwhline(win, row, 0, ACS_LTEE, 1);
    whline(win, ACS_HLINE, SEP_X0 - 1);
    mvwaddch(win, row, SEP_X0, ACS_PLUS);
    mvwhline(win, row, SEP_X0 + 1, ACS_HLINE, SEP_X1 - SEP_X0 - 1);
    mvwaddch(win, row, SEP_X1, ACS_PLUS);
    mvwhline(win, row, SEP_X1 + 1, ACS_HLINE, ST_COLS - SEP_X1 - 2);
    mvwaddch(win, row, ST_COLS - 1, ACS_RTEE);
}

static void st_hline_plain(WINDOW *win, int row)
{
    /* Full-width horizontal line, no dividers (for desc/ctrl separators) */
    mvwhline(win, row, 0, ACS_LTEE, 1);
    whline(win, ACS_HLINE, ST_COLS - 2);
    mvwaddch(win, row, ST_COLS - 1, ACS_RTEE);
}

/* Draw one column's content for a single tier */
static void draw_tier_col(WINDOW *win, int tree, int tier,
                           int cx, int cw,
                           const Entity *player,
                           bool is_active, int active_cur)
{
    bool unlocked  = skilltree_tier_unlocked(player->tree_points[tree], tier);
    int  bitmask   = player->tree_bought[tree][tier];
    const TreeTier *tt = &TREES[tree].tiers[tier];

    /* Tier header: show requirement */
    {
        attr_t attr = unlocked
            ? (COLOR_PAIR(COL_MENU_TITLE) | A_BOLD)
            : COLOR_PAIR(COL_MENU_DIM);
        wattron(win, attr);
        char hdr[32];
        snprintf(hdr, sizeof(hdr), " TIER %d  %s(need %d pts)",
                 tier + 1,
                 unlocked ? "" : "-- ",
                 TIER_REQ[tier]);
        mvwprintw(win, TIER_HDR_ROW(tier), cx, "%-*.*s", cw, cw, hdr);
        wattroff(win, attr);
    }

    /* Option rows */
    for (int o = 0; o < TIER_MAX_OPTS; o++) {
        int row = OPT_ROW(tier, o);
        bool has_opt   = (o < tt->opt_count);
        bool is_bought = (bitmask >> o) & 1;
        int  this_cur  = tier * 3 + o;
        bool is_cursor = is_active && (active_cur == this_cur);

        if (!has_opt) {
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, row, cx, "%-*s", cw, "");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));
            continue;
        }

        const char *name = tt->opts[o].name;
        char bracket = is_bought ? '*' : ' ';
        char prefix  = is_cursor ? '>' : ' ';

        char line[64];
        int  namew = cw - 6;
        snprintf(line, sizeof(line), "%c[%c] %-*.*s",
                 prefix, bracket, namew, namew, name);

        attr_t attr;
        if (is_cursor && unlocked)
            attr = COLOR_PAIR(COL_MENU_SEL) | A_BOLD;
        else if (!unlocked)
            attr = COLOR_PAIR(COL_MENU_DIM);
        else if (is_bought)
            attr = COLOR_PAIR(COL_MENU_TITLE) | A_BOLD;
        else
            attr = COLOR_PAIR(COL_MENU_DIM) | A_BOLD;

        wattron(win, attr);
        mvwprintw(win, row, cx, "%-*.*s", cw, cw, line);
        wattroff(win, attr);
    }
}

static void draw_char_screen(WINDOW *win, const Entity *player,
                              int tree_sel, int cur)
{
    werase(win);
    box(win, 0, 0);

    /* Row 1: stats */
    wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2,
              "LVL %-2d   HP %d/%d   Credits: %-6d   Skill Points: %d",
              player->level, player->hp, player->max_hp,
              player->credits, player->skill_points);
    wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

    /* Row 2: plain separator */
    st_hline_plain(win, 2);

    /* Row 3: tree name tabs */
    for (int t = 0; t < TREE_COUNT; t++) {
        bool active = (t == tree_sel);
        attr_t attr = active
            ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
            : COLOR_PAIR(COL_MENU_DIM);
        wattron(win, attr);
        /* Centre the name in the column */
        int namelen = (int)strlen(TREES[t].name);
        int pad     = (col_w[t] - namelen - 4) / 2;
        if (pad < 0) pad = 0;
        mvwprintw(win, 3, col_x[t] + pad, "%s%s%s",
                  active ? "==" : "  ",
                  TREES[t].name,
                  active ? "==" : "  ");
        /* Points in tree vs next tier threshold */
        int pts  = player->tree_points[t];
        mvwprintw(win, 3, col_x[t] + col_w[t] - 5, "(%2d)", pts);
        wattroff(win, attr);
    }

    /* Row 4: column separator */
    st_hline(win, 4);

    /* Draw each tier */
    for (int tier = 0; tier < TREE_TIERS; tier++) {
        for (int t = 0; t < TREE_COUNT; t++) {
            draw_tier_col(win, t, tier,
                          col_x[t], col_w[t],
                          player,
                          t == tree_sel, cur);
        }
        /* Separator after tiers 0-3 */
        if (tier < TREE_TIERS - 1)
            st_hline(win, TIER_SEP_ROW(tier));
    }

    /* Redraw vertical separators on tier rows (box() only draws the border) */
    for (int row = 4; row <= TIER_SEP_ROW(TREE_TIERS - 2); row++) {
        if (row == 4 ||
            (row >= TIER_HDR_ROW(0) && row <= OPT_ROW(TREE_TIERS - 1, TIER_MAX_OPTS - 1))) {
            mvwaddch(win, row, SEP_X0, ACS_VLINE);
            mvwaddch(win, row, SEP_X1, ACS_VLINE);
        }
    }
    /* Separator rows already have ACS_PLUS from st_hline; vert seps on remaining rows */
    for (int tier = 0; tier < TREE_TIERS; tier++) {
        for (int o = 0; o < TIER_MAX_OPTS; o++) {
            int row = OPT_ROW(tier, o);
            mvwaddch(win, row, SEP_X0, ACS_VLINE);
            mvwaddch(win, row, SEP_X1, ACS_VLINE);
        }
        mvwaddch(win, TIER_HDR_ROW(tier), SEP_X0, ACS_VLINE);
        mvwaddch(win, TIER_HDR_ROW(tier), SEP_X1, ACS_VLINE);
    }

    /* Description panel */
    st_hline_plain(win, DESC_SEP_ROW);
    {
        int tier = cur / 3;
        int opt  = cur % 3;
        const TreeTier *tt = &TREES[tree_sel].tiers[tier];
        const char *desc = (opt < tt->opt_count)
            ? tt->opts[opt].desc
            : "";
        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, DESC_ROW, 2, "%-*.*s", ST_COLS - 4, ST_COLS - 4, desc);
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));
    }

    /* Controls */
    st_hline_plain(win, CTRL_SEP_ROW);
    {
        int respec_cost = player->level * 100;
        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, CTRL_ROW, 2,
                  "[</> arrows] tree  [j/k] option  [Enter] buy  "
                  "[R] respec (%d¢)  [Esc] back",
                  respec_cost);
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));
    }
}

static void clamp_cur(int *cur, int tree_sel)
{
    /* Ensure cur doesn't point past the last valid option in its tier */
    if (*cur > CUR_MAX) { *cur = CUR_MAX; return; }
    int tier = *cur / 3;
    int opt  = *cur % 3;
    int max_opt = TREES[tree_sel].tiers[tier].opt_count - 1;
    if (opt > max_opt)
        *cur = tier * 3 + max_opt;
}

static void buy_talent(Entity *player, int tree, int cur)
{
    int tier = cur / 3;
    int opt  = cur % 3;

    if (opt >= TREES[tree].tiers[tier].opt_count) return;
    if (!skilltree_tier_unlocked(player->tree_points[tree], tier)) return;
    if (player->skill_points < 1) return;
    if ((player->tree_bought[tree][tier] >> opt) & 1) return; /* already have it */

    player->tree_bought[tree][tier] |= (1 << opt);
    player->tree_points[tree]++;
    player->skill_points--;
    entity_recalc_stats(player);
}

static void do_respec(Entity *player)
{
    int cost = player->level * 100;
    if (player->credits < cost) return;

    player->credits -= cost;
    for (int t = 0; t < TREE_COUNT; t++) {
        player->skill_points  += player->tree_points[t];
        player->tree_points[t] = 0;
        for (int r = 0; r < TREE_TIERS; r++)
            player->tree_bought[t][r] = 0;
    }
    entity_recalc_stats(player);
}

static void screen_character(Entity *player)
{
    WINDOW *win = newwin(ST_ROWS, ST_COLS, 0, 0);
    keypad(win, TRUE);

    int tree_sel = 0;
    int cur      = 0;
    bool dirty   = true;

    while (1) {
        if (dirty) {
            draw_char_screen(win, player, tree_sel, cur);
            wrefresh(win);
            dirty = false;
        }

        int ch = wgetch(win);
        switch (ch) {
        /* Switch active tree column */
        case KEY_LEFT:  case '<':
            tree_sel = (tree_sel - 1 + TREE_COUNT) % TREE_COUNT;
            clamp_cur(&cur, tree_sel);
            dirty = true;
            break;
        case KEY_RIGHT: case '>':
            tree_sel = (tree_sel + 1) % TREE_COUNT;
            clamp_cur(&cur, tree_sel);
            dirty = true;
            break;

        /* Navigate options within the active tree */
        case KEY_UP: case 'k':
            if (cur > 0) cur--;
            clamp_cur(&cur, tree_sel);
            dirty = true;
            break;
        case KEY_DOWN: case 'j':
            if (cur < CUR_MAX) cur++;
            else cur = 0;
            clamp_cur(&cur, tree_sel);
            dirty = true;
            break;

        case '\n': case KEY_ENTER:
            buy_talent(player, tree_sel, cur);
            dirty = true;
            break;

        case 'r': case 'R':
            do_respec(player);
            dirty = true;
            break;

        case 'q': case 27: /* Esc */
            delwin(win);
            return;
        }
    }
}

static void screen_shop(Entity *player)
{
    /* Combined list: indices 0..SHOP_COUNT-1 → augmentations,
     *                SHOP_COUNT..SHOP_COUNT+SHOP_GEAR_COUNT-1 → gear */
    int total = SHOP_COUNT + SHOP_GEAR_COUNT;
    int sel   = 0;
    int gear_scroll = 0;  /* first visible unsold gear item */

    /* Start on first available item */
    for (int i = 0; i < total; i++) {
        bool sold = (i < SHOP_COUNT) ? shop_items[i].sold : gear_sold[i - SHOP_COUNT];
        if (!sold) { sel = i; break; }
    }

    WINDOW *win = make_panel("BLACK MARKET");
    bool dirty = true;

    /* Fixed layout rows */
#define AUG_HDR_ROW  4
#define AUG_TOP_ROW  5
#define AUG_MAX_ROWS 5   /* max augmentation rows before gear section */
#define GEAR_HDR_ROW 11  /* gear section header (minimum) */
#define GEAR_BOT_ROW (HUB_H - 4)   /* last row usable for gear items */

    while (1) {
        if (dirty) {
            werase(win);
            menu_draw_box(win, "BLACK MARKET");

            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, 2, 3, "Credits: %d¢", player->credits);
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            /* ---- Augmentations section (fixed, no scroll) ---------- */
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, AUG_HDR_ROW, 3, "-- AUGMENTATIONS --");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            int row = AUG_TOP_ROW;
            for (int i = 0; i < SHOP_COUNT; i++) {
                ShopItem *it = &shop_items[i];
                if (it->sold) continue;
                bool affordable = player->credits >= it->cost;
                bool is_sel = (i == sel);

                attr_t attr = is_sel ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                            : (!affordable ? COLOR_PAIR(COL_MENU_DIM)
                                           : (COLOR_PAIR(COL_MENU_DIM) | A_BOLD));
                wattron(win, attr);
                mvwprintw(win, row, 3, "%s%-22s %-14s %d¢",
                          is_sel ? ">" : " ",
                          it->name, it->desc, it->cost);
                wattroff(win, attr);
                row++;
            }

            /* ---- Gear section (scrollable) ------------------------- */
            /* Build ordered list of unsold gear indices */
            int gear_uns[SHOP_GEAR_COUNT];
            int gear_n = 0;
            for (int i = 0; i < SHOP_GEAR_COUNT; i++) {
                if (!gear_sold[i])
                    gear_uns[gear_n++] = i;
            }

            /* Find sel's position in gear_uns (if sel is in gear range) */
            int sel_gear_pos = -1;
            if (sel >= SHOP_COUNT) {
                for (int g = 0; g < gear_n; g++) {
                    if (SHOP_COUNT + gear_uns[g] == sel) {
                        sel_gear_pos = g;
                        break;
                    }
                }
            }

            /* Adjust gear_scroll to keep the selected gear item in view */
            int gear_hdr = (row < GEAR_HDR_ROW - 1) ? GEAR_HDR_ROW - 1 : row + 1;
            int gear_top = gear_hdr + 1;
            int gear_vis = GEAR_BOT_ROW - gear_top;  /* visible rows */
            if (gear_vis < 1) gear_vis = 1;
            if (sel_gear_pos >= 0) {
                if (sel_gear_pos < gear_scroll)
                    gear_scroll = sel_gear_pos;
                if (sel_gear_pos >= gear_scroll + gear_vis)
                    gear_scroll = sel_gear_pos - gear_vis + 1;
            }
            if (gear_scroll < 0) gear_scroll = 0;

            /* Section header with count and scroll hints */
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, gear_hdr, 3, "-- GEAR (%d avail) --", gear_n);
            if (gear_scroll > 0)
                mvwprintw(win, gear_hdr, HUB_W - 7, " [/\\]");
            if (gear_scroll + gear_vis < gear_n)
                mvwprintw(win, GEAR_BOT_ROW, HUB_W - 7, " [\\/]");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            /* Draw visible gear items */
            row = gear_top;
            for (int g = gear_scroll; g < gear_n && row < GEAR_BOT_ROW; g++) {
                int   i   = gear_uns[g];
                int   idx = SHOP_COUNT + i;
                Item *it  = &shop_gear[i];
                bool  affordable = player->credits >= it->value;
                bool  is_sel     = (idx == sel);

                attr_t base = is_sel ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                            : (!affordable ? COLOR_PAIR(COL_MENU_DIM)
                                           : (COLOR_PAIR(COL_MENU_DIM) | A_BOLD));
                wattron(win, base);
                mvwprintw(win, row, 3, "%s[%s] %-16s",
                          is_sel ? ">" : " ",
                          item_type_label(it->type),
                          it->name);
                wattroff(win, base);

                /* Rarity in its colour */
                int rcol = item_rarity_color(it->rarity);
                wattron(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));
                mvwprintw(win, row, 30, "%-8s", item_rarity_name(it->rarity));
                wattroff(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));

                /* Stat + range (for ranged weapons) + price */
                wattron(win, base);
                if (it->type == ITEM_WEAPON && it->range > 0)
                    mvwprintw(win, row, 39, "%+d%-3s rng%-2d %4d¢",
                              it->stat_bonus, stat_label(it->type),
                              it->range, it->value);
                else
                    mvwprintw(win, row, 39, "%+d%-3s        %4d¢",
                              it->stat_bonus, stat_label(it->type), it->value);
                wattroff(win, base);

                row++;
            }

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, HUB_H - 3, 3,
                      "[j/k] navigate  [Enter] buy  [Esc/q] back");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            wrefresh(win);
            dirty = false;
        }

        int ch = wgetch(win);
        switch (ch) {

        case KEY_UP: case 'k': {
            int orig = sel;
            do {
                sel = (sel - 1 + total) % total;
                bool sold = (sel < SHOP_COUNT) ? shop_items[sel].sold
                                               : gear_sold[sel - SHOP_COUNT];
                if (!sold) break;
            } while (sel != orig);
            dirty = true;
            break;
        }

        case KEY_DOWN: case 'j': {
            int orig = sel;
            do {
                sel = (sel + 1) % total;
                bool sold = (sel < SHOP_COUNT) ? shop_items[sel].sold
                                               : gear_sold[sel - SHOP_COUNT];
                if (!sold) break;
            } while (sel != orig);
            dirty = true;
            break;
        }

        case '\n': case KEY_ENTER: {
            if (sel < SHOP_COUNT) {
                /* Buy augmentation */
                ShopItem *it = &shop_items[sel];
                if (!it->sold && player->credits >= it->cost) {
                    player->credits -= it->cost;
                    player->skills[it->skill] += it->bonus;
                    it->sold = true;
                    /* Advance sel to next available */
                    for (int i = 0; i < total; i++) {
                        int s = (sel + 1 + i) % total;
                        bool sold = (s < SHOP_COUNT) ? shop_items[s].sold
                                                     : gear_sold[s - SHOP_COUNT];
                        if (!sold) { sel = s; break; }
                    }
                    dirty = true;
                }
            } else {
                /* Buy gear */
                int gi = sel - SHOP_COUNT;
                Item *it = &shop_gear[gi];
                if (!gear_sold[gi] && player->credits >= it->value) {
                    if (entity_inv_add(player, *it)) {
                        player->credits -= it->value;
                        /* Consumables have unlimited stock; equipment sells out */
                        if (it->type != ITEM_CONSUMABLE)
                            gear_sold[gi] = true;
                        /* Advance sel to next available */
                        for (int i = 0; i < total; i++) {
                            int s = (sel + 1 + i) % total;
                            bool sold = (s < SHOP_COUNT) ? shop_items[s].sold
                                                         : gear_sold[s - SHOP_COUNT];
                            if (!sold) { sel = s; break; }
                        }
                        dirty = true;
                    }
                }
            }
            break;
        }

        case 'q': case 27:
            delwin(win);
            return;
        }
    }
#undef AUG_HDR_ROW
#undef AUG_TOP_ROW
#undef AUG_MAX_ROWS
#undef GEAR_HDR_ROW
#undef GEAR_BOT_ROW
}

/* ------------------------------------------------------------------ */
/* Inventory screen                                                     */
/* ------------------------------------------------------------------ */

static void screen_inventory(Entity *player, ItemType filter)
{
    WINDOW *win = make_panel("INVENTORY");
    int sel = 0;
    int scroll = 0;
#define INV_VISIBLE 14   /* rows available for item list */

    /* Adjust sel to a valid index (skip filtered-out items) */
    bool dirty = true;

    while (1) {
        /* Count items visible under filter */
        int visible_count = 0;
        int vis_idx[INVENTORY_MAX];  /* maps list position → inventory index */
        for (int i = 0; i < player->inv_count; i++) {
            if (filter == ITEM_NONE || player->inventory[i].type == filter)
                vis_idx[visible_count++] = i;
        }

        if (sel >= visible_count) sel = visible_count > 0 ? visible_count - 1 : 0;
        if (scroll > sel) scroll = sel;
        if (scroll + INV_VISIBLE <= sel) scroll = sel - INV_VISIBLE + 1;

        if (dirty) {
            werase(win);
            menu_draw_box(win, "INVENTORY");

            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, 2, 3, "Items: %d/%d   Credits: %d¢",
                      player->inv_count, INVENTORY_MAX, player->credits);
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            /* Column headers */
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, 3, 3, " [TYPE  ] %-16s %-10s %s",
                      "Name", "Rarity", "Value");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));
            mvwhline(win, 4, 1, ACS_HLINE, HUB_W - 2);

            /* Item list */
            int row = 5;
            for (int vi = scroll;
                 vi < visible_count && row < 5 + INV_VISIBLE;
                 vi++, row++) {
                int i = vis_idx[vi];
                Item *it = &player->inventory[i];
                bool is_sel = (vi == sel);

                int rcol = item_rarity_color(it->rarity);
                attr_t base = is_sel ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                                     : COLOR_PAIR(COL_MENU_DIM);

                wattron(win, base);
                mvwprintw(win, row, 3, "%s[%s] %-16s",
                          is_sel ? ">" : " ",
                          item_type_label(it->type),
                          it->name);
                wattroff(win, base);

                wattron(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));
                mvwprintw(win, row, 30, "%-10s", item_rarity_name(it->rarity));
                wattroff(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));

                wattron(win, base);
                mvwprintw(win, row, 40, "%4d¢", it->value);
                wattroff(win, base);
            }

            mvwhline(win, 5 + INV_VISIBLE, 1, ACS_HLINE, HUB_W - 2);

            /* Detail panel for selected item */
            if (visible_count > 0) {
                Item *it = &player->inventory[vis_idx[sel]];
                int rcol = item_rarity_color(it->rarity);
                wattron(win, COLOR_PAIR(rcol) | A_BOLD);
                mvwprintw(win, 5 + INV_VISIBLE + 1, 3,
                          "%s  %+d %s",
                          it->name,
                          it->stat_bonus,
                          stat_label(it->type));
                wattroff(win, COLOR_PAIR(rcol) | A_BOLD);

                if (it->mod_count > 0) {
                    wattron(win, COLOR_PAIR(COL_MENU_DIM));
                    mvwprintw(win, 5 + INV_VISIBLE + 2, 3, "Mods: ");
                    for (int m = 0; m < it->mod_count; m++) {
                        if (m > 0) wprintw(win, "  ");
                        wprintw(win, "%s", it->mods[m].label);
                    }
                    wattroff(win, COLOR_PAIR(COL_MENU_DIM));
                } else {
                    wattron(win, COLOR_PAIR(COL_MENU_DIM));
                    mvwprintw(win, 5 + INV_VISIBLE + 2, 3, "No mods");
                    wattroff(win, COLOR_PAIR(COL_MENU_DIM));
                }
            }

            mvwhline(win, HUB_H - 4, 1, ACS_HLINE, HUB_W - 2);
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, HUB_H - 3, 2,
                      "[j/k] nav  [Enter] equip  [s] sell  [d] drop  [Esc] back");
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
            if (visible_count > 0 && sel < visible_count - 1) sel++;
            dirty = true;
            break;
        case '\n': case KEY_ENTER:
            if (visible_count > 0) {
                entity_equip(player, vis_idx[sel]);
                /* After equip the item moves out of inventory; recount */
                if (sel >= player->inv_count) sel = player->inv_count - 1;
                if (sel < 0) sel = 0;
                dirty = true;
            }
            break;
        case 's': case 'S':   /* sell */
            if (visible_count > 0) {
                int i = vis_idx[sel];
                player->credits += player->inventory[i].value / 2;
                entity_inv_remove(player, i);
                if (sel >= player->inv_count && sel > 0) sel--;
                dirty = true;
            }
            break;
        case 'd': case 'D':   /* drop */
            if (visible_count > 0) {
                entity_inv_remove(player, vis_idx[sel]);
                if (sel >= player->inv_count && sel > 0) sel--;
                dirty = true;
            }
            break;
        case 'q': case 27:    /* Esc */
            delwin(win);
            return;
        }
    }
#undef INV_VISIBLE
}

/* ------------------------------------------------------------------ */
/* In-mission inventory overlay                                         */
/* ------------------------------------------------------------------ */

void mission_inventory(Entity *player)
{
#define MINV_VISIBLE 8   /* inventory rows shown at once */
    static const char *mslot_name[EQUIP_SLOT_COUNT] = {
        "WEAPON", "HEAD  ", "BODY  ", "LEGS  ", "DECK  "
    };

    int w  = 60, h = 26;
    int wy = (MAP_HEIGHT - h) / 2;
    int wx = (MAP_WIDTH  - w) / 2;

    WINDOW *win = newwin(h, w, wy, wx);
    keypad(win, TRUE);

    int sel    = 0;
    int scroll = 0;
    bool dirty = true;

    while (1) {
        /* Build full (unfiltered) visible index each frame */
        int vis_idx[INVENTORY_MAX];
        int vcount = 0;
        for (int i = 0; i < player->inv_count; i++)
            vis_idx[vcount++] = i;

        if (sel >= vcount) sel = vcount > 0 ? vcount - 1 : 0;
        if (scroll > sel)                        scroll = sel;
        if (scroll + MINV_VISIBLE <= sel)        scroll = sel - MINV_VISIBLE + 1;

        if (dirty) {
            werase(win);
            menu_draw_box(win, "FIELD INVENTORY");

            /* ---- Equipped section --------------------------------- */
            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, 2, 3, "Equipped");
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
                Item *it = &player->equipped[s];
                wattron(win, COLOR_PAIR(COL_MENU_DIM));
                mvwprintw(win, 3 + s, 3, "%s  ", mslot_name[s]);
                wattroff(win, COLOR_PAIR(COL_MENU_DIM));

                if (it->type != ITEM_NONE) {
                    int rcol = item_rarity_color(it->rarity);
                    wattron(win, COLOR_PAIR(rcol) | A_BOLD);
                    if (it->type == ITEM_WEAPON && it->range > 0)
                        wprintw(win, "%-18s %+d %-3s  rng %d",
                                it->name, it->stat_bonus,
                                stat_label(it->type), it->range);
                    else
                        wprintw(win, "%-18s %+d %-3s",
                                it->name, it->stat_bonus,
                                stat_label(it->type));
                    wattroff(win, COLOR_PAIR(rcol) | A_BOLD);
                } else {
                    wattron(win, COLOR_PAIR(COL_MENU_DIM));
                    wprintw(win, "--");
                    wattroff(win, COLOR_PAIR(COL_MENU_DIM));
                }
            }
            mvwhline(win, 3 + EQUIP_SLOT_COUNT, 1, ACS_HLINE, w - 2);

            /* ---- Inventory list ----------------------------------- */
            int inv_top = 3 + EQUIP_SLOT_COUNT + 1;  /* row 9 */

            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, inv_top, 3, "Backpack  %d/%d",
                      player->inv_count, INVENTORY_MAX);
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, inv_top + 1, 3, " [TYPE  ] %-16s %-10s  Val",
                      "Name", "Rarity");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            int row = inv_top + 2;
            for (int vi = scroll;
                 vi < vcount && row < inv_top + 2 + MINV_VISIBLE;
                 vi++, row++) {
                int i   = vis_idx[vi];
                Item *it = &player->inventory[i];
                bool is_sel = (vi == sel);

                int   rcol = item_rarity_color(it->rarity);
                attr_t base = is_sel ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                                     : COLOR_PAIR(COL_MENU_DIM);
                wattron(win, base);
                mvwprintw(win, row, 3, "%s[%s] %-16s",
                          is_sel ? ">" : " ",
                          item_type_label(it->type), it->name);
                wattroff(win, base);

                wattron(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));
                wprintw(win, " %-10s", item_rarity_name(it->rarity));
                wattroff(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));

                wattron(win, base);
                wprintw(win, " %4d", it->value);
                wattroff(win, base);
            }

            mvwhline(win, inv_top + 2 + MINV_VISIBLE, 1, ACS_HLINE, w - 2);

            /* ---- Detail panel ------------------------------------- */
            int drow = inv_top + 2 + MINV_VISIBLE + 1;  /* row 20 */
            if (vcount > 0) {
                Item *it = &player->inventory[vis_idx[sel]];
                int rcol  = item_rarity_color(it->rarity);
                wattron(win, COLOR_PAIR(rcol) | A_BOLD);
                if (it->type == ITEM_WEAPON && it->range > 0)
                    mvwprintw(win, drow, 3, "%s  %+d %-3s  range %d",
                              it->name, it->stat_bonus,
                              stat_label(it->type), it->range);
                else
                    mvwprintw(win, drow, 3, "%s  %+d %-3s",
                              it->name, it->stat_bonus, stat_label(it->type));
                wattroff(win, COLOR_PAIR(rcol) | A_BOLD);

                wattron(win, COLOR_PAIR(COL_MENU_DIM));
                if (it->mod_count > 0) {
                    mvwprintw(win, drow + 1, 3, "Mods: ");
                    for (int m = 0; m < it->mod_count; m++) {
                        if (m > 0) wprintw(win, "  ");
                        wprintw(win, "%s", it->mods[m].label);
                    }
                } else {
                    mvwprintw(win, drow + 1, 3, "No mods");
                }
                wattroff(win, COLOR_PAIR(COL_MENU_DIM));
            }

            /* ---- Controls ----------------------------------------- */
            mvwhline(win, h - 4, 1, ACS_HLINE, w - 2);
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, h - 3, 2,
                      "[j/k] navigate  [Enter] equip  [i/Esc] close");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            wrefresh(win);
            dirty = false;
        }

        int ch = wgetch(win);
        switch (ch) {
        case KEY_UP: case 'k':
            if (sel > 0) { sel--; dirty = true; }
            break;
        case KEY_DOWN: case 'j':
            if (vcount > 0 && sel < vcount - 1) { sel++; dirty = true; }
            break;
        case '\n': case KEY_ENTER:
            if (vcount > 0) {
                entity_equip(player, vis_idx[sel]);
                if (sel >= player->inv_count)
                    sel = player->inv_count > 0 ? player->inv_count - 1 : 0;
                dirty = true;
            }
            break;
        case 'i': case 'I': case 27:  /* Esc */
            goto done;
        }
    }
done:
    delwin(win);
    touchwin(stdscr);
    refresh();
#undef MINV_VISIBLE
}

/* ------------------------------------------------------------------ */
/* Equipment screen                                                     */
/* ------------------------------------------------------------------ */

static const char *slot_name[EQUIP_SLOT_COUNT] = {
    "WEAPON", "HEAD  ", "BODY  ", "LEGS  ", "DECK  "
};

static void screen_equipment(Entity *player)
{
    WINDOW *win = make_panel("EQUIPMENT");
    int sel = 0;   /* 0..EQUIP_SLOT_COUNT-1 */
    bool dirty = true;

    while (1) {
        if (dirty) {
            werase(win);
            menu_draw_box(win, "EQUIPMENT");

            /* Stats header */
            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, 2, 3,
                      "LVL:%-2d  HP:%d/%d  ATK:%+d  DEF:%+d  HCK:%+d",
                      player->level, player->hp, player->max_hp,
                      entity_gear_atk(player),
                      entity_gear_def(player),
                      entity_gear_hack(player));
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

            mvwhline(win, 3, 1, ACS_HLINE, HUB_W - 2);

            /* Slot rows */
            for (int s = 0; s < EQUIP_SLOT_COUNT; s++) {
                int row = 4 + s;
                bool is_sel = (s == sel);
                Item *it = &player->equipped[s];

                attr_t base = is_sel ? (COLOR_PAIR(COL_MENU_SEL) | A_BOLD)
                                     : COLOR_PAIR(COL_MENU_DIM);
                wattron(win, base);
                mvwprintw(win, row, 3, "%s[%s] ",
                          is_sel ? ">" : " ", slot_name[s]);
                wattroff(win, base);

                if (item_valid(it)) {
                    int rcol = item_rarity_color(it->rarity);
                    wattron(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));
                    mvwprintw(win, row, 13, "%-18s", it->name);
                    wattroff(win, COLOR_PAIR(rcol) | (is_sel ? A_BOLD : 0));

                    wattron(win, base);
                    mvwprintw(win, row, 32, "%-10s %+d%s",
                              item_rarity_name(it->rarity),
                              it->stat_bonus,
                              stat_label(it->type));
                    wattroff(win, base);
                } else {
                    wattron(win, COLOR_PAIR(COL_MENU_DIM));
                    mvwprintw(win, row, 13, "-- empty --");
                    wattroff(win, COLOR_PAIR(COL_MENU_DIM));
                }
            }

            /* Augmentations section */
            int aug_row = 4 + EQUIP_SLOT_COUNT + 1;
            mvwhline(win, aug_row - 1, 1, ACS_HLINE, HUB_W - 2);
            wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            mvwprintw(win, aug_row, 3, "AUGMENTATIONS");
            wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
            aug_row++;

            int aug_shown = 0;
            for (int i = 0; i < SHOP_COUNT; i++) {
                if (!shop_items[i].sold) continue;
                wattron(win, COLOR_PAIR(COL_RARITY_UNCOMMON));
                mvwprintw(win, aug_row, 5, "%-20s  %s",
                          shop_items[i].name, shop_items[i].desc);
                wattroff(win, COLOR_PAIR(COL_RARITY_UNCOMMON));
                aug_row++;
                aug_shown++;
            }
            if (aug_shown == 0) {
                wattron(win, COLOR_PAIR(COL_MENU_DIM));
                mvwprintw(win, aug_row, 5, "-- none installed --");
                wattroff(win, COLOR_PAIR(COL_MENU_DIM));
            }

            mvwhline(win, HUB_H - 4, 1, ACS_HLINE, HUB_W - 2);
            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, HUB_H - 3, 2,
                      "[j/k] navigate  [Enter] unequip  [i] equip from inv  [Esc] back");
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
            if (sel < EQUIP_SLOT_COUNT - 1) sel++;
            dirty = true;
            break;
        case '\n': case KEY_ENTER:
            entity_unequip(player, (EquipSlot)sel);
            dirty = true;
            break;
        case 'i': case 'I': {
            /* Open inventory filtered to items that go in this slot */
            static const ItemType slot_type[EQUIP_SLOT_COUNT] = {
                ITEM_WEAPON, ITEM_ARMOR_HEAD, ITEM_ARMOR_BODY,
                ITEM_ARMOR_LEGS, ITEM_DECK,
            };
            delwin(win);
            screen_inventory(player, slot_type[sel]);
            win = make_panel("EQUIPMENT");
            keypad(win, TRUE);
            dirty = true;
            break;
        }
        case 'q': case 27:
            delwin(win);
            return;
        }
    }
}

static GameState screen_missions(Entity *player)
{
    missions_generate();

    char labels[MISSION_COUNT][80];
    for (int i = 0; i < MISSION_COUNT; i++) {
        snprintf(labels[i], sizeof(labels[i]), "%-10s %-20s @ %s",
                 missions[i].name, missions[i].target, missions[i].location);
    }

    WINDOW *win = make_panel("MISSION BOARD");
    int sel = 0;
    bool dirty = true;

    while (1) {
        if (dirty) {
            werase(win);
            menu_draw_box(win, "MISSION BOARD");

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, 2, 3, "Select a contract. Difficulty and payout shown.");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            for (int i = 0; i < MISSION_COUNT; i++) {
                int row = 5 + i * 4;
                bool is_sel = (i == sel);

                if (is_sel) wattron(win, COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
                else        wattron(win, COLOR_PAIR(COL_MENU_DIM));

                mvwprintw(win, row, 3, "%s%s", is_sel ? "> " : "  ", labels[i]);
                mvwprintw(win, row + 1, 5, "Difficulty: ");

                if (is_sel) wattroff(win, COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
                else        wattroff(win, COLOR_PAIR(COL_MENU_DIM));

                stars(win, row + 1, 17, missions[i].difficulty);

                wattron(win, is_sel
                    ? COLOR_PAIR(COL_MENU_SEL) | A_BOLD
                    : COLOR_PAIR(COL_MENU_DIM));
                mvwprintw(win, row + 1, 23, "  Reward: %d ¢", missions[i].reward);
                wattroff(win, is_sel
                    ? COLOR_PAIR(COL_MENU_SEL) | A_BOLD
                    : COLOR_PAIR(COL_MENU_DIM));
            }

            wattron(win, COLOR_PAIR(COL_MENU_DIM));
            mvwprintw(win, HUB_H - 3, 3, "[Enter] accept contract   [Esc/q] back");
            wattroff(win, COLOR_PAIR(COL_MENU_DIM));

            wrefresh(win);
            dirty = false;
        }

        int ch = wgetch(win);
        switch (ch) {
        case KEY_UP: case 'k': sel = (sel - 1 + MISSION_COUNT) % MISSION_COUNT; dirty = true; break;
        case KEY_DOWN: case 'j': sel = (sel + 1) % MISSION_COUNT;               dirty = true; break;
        case '\n': case KEY_ENTER:
            player->active_mission_reward = missions[sel].reward;
            delwin(win);
            return GAME_STATE_PLAYING;
        case 'q': case 27:
            delwin(win);
            return GAME_STATE_HUB;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Main hub entry point                                                 */
/* ------------------------------------------------------------------ */

GameState hub_run(Entity *player)
{
    static const char *main_items[] = {
        "Character",
        "Equipment",
        "Inventory",
        "Black Market",
        "Mission Board",
        "Quit",
    };
    enum { MAIN_CHARACTER, MAIN_EQUIPMENT, MAIN_INVENTORY,
           MAIN_SHOP, MAIN_MISSIONS, MAIN_QUIT };
    int nitems = 6;
    int sel = 0;

    while (1) {
        hub_title();

        WINDOW *win = make_panel("SAFE HOUSE");
        menu_draw_box(win, "SAFE HOUSE");

        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, 2, 3, "Welcome back, runner.");
        mvwprintw(win, 3, 3, "Credits: %-6d  LVL: %-2d  SP: %d  Inv: %d/%d",
                  player->credits, player->level, player->skill_points,
                  player->inv_count, INVENTORY_MAX);
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));

        menu_draw_list(win, 6, 4, main_items, nitems, sel);

        wattron(win, COLOR_PAIR(COL_MENU_DIM));
        mvwprintw(win, HUB_H - 3, 3, "[j/k / arrows] navigate   [Enter] select");
        wattroff(win, COLOR_PAIR(COL_MENU_DIM));

        wrefresh(win);

        int ch = wgetch(win);
        delwin(win);

        switch (ch) {
        case KEY_UP: case 'k': sel = (sel - 1 + nitems) % nitems; break;
        case KEY_DOWN: case 'j': sel = (sel + 1) % nitems;         break;
        case '\n': case KEY_ENTER:
            switch (sel) {
            case MAIN_CHARACTER:  screen_character(player);  break;
            case MAIN_EQUIPMENT:  screen_equipment(player);  break;
            case MAIN_INVENTORY:  screen_inventory(player, ITEM_NONE); break;
            case MAIN_SHOP:       screen_shop(player);       break;
            case MAIN_MISSIONS: {
                GameState gs = screen_missions(player);
                if (gs != GAME_STATE_HUB) return gs;
                break;
            }
            case MAIN_QUIT:
                if (confirm_quit()) return GAME_STATE_QUIT;
                break;
            }
            break;
        case 'q':
            if (confirm_quit()) return GAME_STATE_QUIT;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Debrief                                                              */
/* ------------------------------------------------------------------ */

GameState debrief_run(Entity *player)
{
    player->credits += player->active_mission_reward;
    player->level++;
    player->skill_points += 2;
    player->active_mission_reward = 0;

    /* Collect loot into inventory */
    for (int i = 0; i < player->pending_loot_count; i++)
        entity_inv_add(player, player->pending_loot[i]);

    hub_title();

    WINDOW *win = make_panel("MISSION COMPLETE");
    menu_draw_box(win, "MISSION COMPLETE");

    wattron(win, COLOR_PAIR(COL_MENU_SEL) | A_BOLD);
    mvwprintw(win, 4,  3, "  EXTRACTION SUCCESSFUL");
    wattroff(win, COLOR_PAIR(COL_MENU_SEL) | A_BOLD);

    wattron(win, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(win, 7,  3, "Credits earned  : %d¢",   player->credits);
    mvwprintw(win, 8,  3, "New level       : %d",    player->level);
    mvwprintw(win, 9,  3, "Skill points    : +2 (total %d)", player->skill_points);
    wattroff(win, COLOR_PAIR(COL_MENU_DIM));

    /* Show loot items */
    if (player->pending_loot_count > 0) {
        wattron(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);
        mvwprintw(win, 11, 3, "LOOT RECOVERED:");
        wattroff(win, COLOR_PAIR(COL_MENU_TITLE) | A_BOLD);

        for (int i = 0; i < player->pending_loot_count; i++) {
            Item *it = &player->pending_loot[i];
            int rcol = item_rarity_color(it->rarity);
            wattron(win, COLOR_PAIR(rcol) | A_BOLD);
            mvwprintw(win, 12 + i, 5, "  %-20s %-10s %+d%s",
                      it->name,
                      item_rarity_name(it->rarity),
                      it->stat_bonus,
                      stat_label(it->type));
            wattroff(win, COLOR_PAIR(rcol) | A_BOLD);
        }
    }

    wattron(win, COLOR_PAIR(COL_MENU_DIM));
    mvwprintw(win, HUB_H - 4, 3, "Press any key to return to safe house...");
    wattroff(win, COLOR_PAIR(COL_MENU_DIM));

    wrefresh(win);
    wgetch(win);
    delwin(win);

    player->pending_loot_count = 0;
    return GAME_STATE_HUB;
}
