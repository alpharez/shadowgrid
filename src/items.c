#include "items.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Metadata helpers                                                     */
/* ------------------------------------------------------------------ */

const char *item_type_label(ItemType t)
{
    switch (t) {
    case ITEM_WEAPON:      return "WEAPON";
    case ITEM_ARMOR_HEAD:  return "HEAD  ";
    case ITEM_ARMOR_BODY:  return "BODY  ";
    case ITEM_ARMOR_LEGS:  return "LEGS  ";
    case ITEM_DECK:        return "DECK  ";
    case ITEM_CONSUMABLE:  return "USE   ";
    default:               return "------";
    }
}

const char *item_rarity_name(Rarity r)
{
    switch (r) {
    case RARITY_COMMON:    return "Common";
    case RARITY_UNCOMMON:  return "Uncommon";
    case RARITY_RARE:      return "Rare";
    case RARITY_EPIC:      return "Epic";
    case RARITY_LEGENDARY: return "Legendary";
    default:               return "Unknown";
    }
}

int item_rarity_color(Rarity r)
{
    /* Returns colour-pair indices defined in render.h as COL_RARITY_* (11-15) */
    switch (r) {
    case RARITY_COMMON:    return 11;
    case RARITY_UNCOMMON:  return 12;
    case RARITY_RARE:      return 13;
    case RARITY_EPIC:      return 14;
    case RARITY_LEGENDARY: return 15;
    default:               return 11;
    }
}

EquipSlot item_equip_slot(ItemType t)
{
    switch (t) {
    case ITEM_WEAPON:     return EQUIP_WEAPON;
    case ITEM_ARMOR_HEAD: return EQUIP_HEAD;
    case ITEM_ARMOR_BODY: return EQUIP_BODY;
    case ITEM_ARMOR_LEGS: return EQUIP_LEGS;
    case ITEM_DECK:       return EQUIP_DECK;
    default:              return EQUIP_WEAPON;
    }
}

bool item_valid(const Item *it)
{
    return it && it->type != ITEM_NONE;
}

/* ------------------------------------------------------------------ */
/* Base item tables (used for random loot generation)                   */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    int         stat_bonus;
    int         base_value;
    int         range;      /* 0 = melee; >0 = ranged (tiles) */
} BaseItem;

static const BaseItem BASE_WEAPONS[] = {
    /* --- Melee (range = 0) ------------------------------------------- */
    { "Knuckle Dusters",  1,   150,  0 },  /* bare-fist backup          */
    { "Switchblade",      1,   200,  0 },  /* cheap folding street blade */
    { "Ceramic Knife",    1,   250,  0 },  /* silent, undetectable      */
    { "Baton",            2,   300,  0 },  /* non-lethal option         */
    { "Combat Knife",     2,   300,  0 },  /* reliable field blade      */
    { "Vibroblade",       2,   350,  0 },  /* vibrating mono-edge       */
    { "Shock Glove",      2,   400,  0 },  /* electrified strike        */
    { "Stun Maul",        3,   500,  0 },  /* heavy EM stunner          */
    { "Micro SMG",        3,   600,  0 },  /* compact spray             */
    { "SMG",              3,   700,  0 },  /* full-size submachine gun  */
    { "Carbon Whip",      3,   700,  0 },  /* reinforced cutting whip   */
    { "Neural Lash",      3,   800,  0 },  /* neural-disrupting flail   */
    { "Katana",           4,   900,  0 },  /* mono-edge steel           */
    { "Shotgun",          4,  1000,  0 },  /* pump-action devastation   */
    { "Combat Shotgun",   4,  1050,  0 },  /* military auto-shotgun     */
    { "Monowire",         5,  1300,  0 },  /* monofilament, cuts all    */
    /* --- Ranged (range > 0) ------------------------------------------ */
    { "Needler",          1,   300,  6 },  /* near-silent, short range  */
    { "Flechette Gun",    2,   550, 10 },  /* 3-dart burst, suppressed  */
    { "Rail Pistol",      2,   700,  8 },  /* magnetic accelerator      */
    { "Smart Pistol",     3,   800,  8 },  /* AI-guided targeting       */
    { "Needle Rifle",     3,   950, 14 },  /* long-range, quiet         */
    { "Sniper Rifle",     5,  1600, 18 },  /* extreme range, lethal     */
};
#define BASE_WEAPONS_COUNT 22

static const BaseItem BASE_HEADS[] = {
    { "Street Helmet",    1,  150 },
    { "Urban Balaclava",  1,  200 },
    { "Infiltrator Hood", 1,  320 },
    { "Tactical Mask",    1,  300 },
    { "Combat Helm",      2,  350 },
    { "Ballistic Visor",  2,  450 },
    { "Mil-Spec Helmet",  3,  600 },
    { "Cybernetic Crown", 3,  800 },
};
#define BASE_HEADS_COUNT 8

static const BaseItem BASE_BODIES[] = {
    { "Runner's Vest",    1,   200 },
    { "Leather Jacket",   1,   200 },
    { "Corp Uniform",     1,   250 },
    { "Combat Jacket",    2,   450 },
    { "Tac Weave",        2,   500 },
    { "Ballistic Vest",   3,   750 },
    { "Exo-Partial",      3,   850 },
    { "Heavy Plate",      4,  1200 },
    { "Mil Exo-Frame",    5,  1800 },
};
#define BASE_BODIES_COUNT 9

static const BaseItem BASE_LEGS[] = {
    { "Cargo Pants",      1,  150 },
    { "Urban Leggings",   1,  200 },
    { "Combat Pants",     2,  350 },
    { "Stealth Runners",  2,  450 },
    { "Grav Boots",       2,  500 },
    { "Tac. Greaves",     3,  700 },
    { "Exo-Legs",         3,  800 },
    { "Sprint Pistons",   3,  900 },
};
#define BASE_LEGS_COUNT 8

static const BaseItem BASE_DECKS[] = {
    { "Junk Rig",         1,   180 },
    { "Budget Deck",      1,   250 },
    { "Street Deck",      2,   450 },
    { "Standard Deck",    2,   500 },
    { "Ghost Rig",        2,   700 },
    { "Assault Rig",      2,   750 },
    { "Pro Deck",         3,   900 },
    { "Phantom Deck",     3,  1100 },
    { "ICE Breaker",      3,  1200 },
    { "Quantum Rig",      4,  1600 },
    { "Neural Interface", 4,  1800 },
};
#define BASE_DECKS_COUNT 11

/* ------------------------------------------------------------------ */
/* Modifier pools                                                        */
/* ------------------------------------------------------------------ */

/* skill indices: 0=Stealth, 1=Hacking, 2=Combat */

static const ItemMod WEAPON_MODS[] = {
    { 0, 1, "+1 STL" },  /* Silenced        — quieter shot        */
    { 2, 1, "+1 CMB" },  /* AP rounds       — armour-piercing     */
    { 1, 1, "+1 HCK" },  /* Smart-linked    — AI target assist    */
    { 0, 2, "+2 STL" },  /* Whisper barrel  — near-silent         */
    { 2, 2, "+2 CMB" },  /* Overcharge      — boosted power cell  */
    { 1, 2, "+2 HCK" },  /* AI-Guided       — full neural assist  */
};
#define WEAPON_MODS_COUNT 6

static const ItemMod ARMOR_MODS[] = {
    { 0, 1, "+1 STL" },  /* Thermal-dampd   — heat signature low  */
    { 1, 1, "+1 HCK" },  /* Networked       — integrated antenna  */
    { 2, 1, "+1 CMB" },  /* Reinf. Plates   — extra steel inserts */
    { 0, 2, "+2 STL" },  /* Chameleon Weave — adaptive camo       */
    { 1, 2, "+2 HCK" },  /* Relay Mesh      — woven signal booster*/
};
#define ARMOR_MODS_COUNT 5

static const ItemMod DECK_MODS[] = {
    { 0, 1, "+1 STL" },  /* Ghost-rigged    — signal masking      */
    { 2, 1, "+1 CMB" },  /* Military-grade  — hardened firmware   */
    { 1, 1, "+1 HCK" },  /* Overclocked     — pushed past limits  */
    { 0, 2, "+2 STL" },  /* Phantom Proto.  — corp black project  */
    { 2, 2, "+2 CMB" },  /* Assault FW      — ICE-crushing code   */
};
#define DECK_MODS_COUNT 5

/* ------------------------------------------------------------------ */
/* Rarity roll                                                          */
/* ------------------------------------------------------------------ */

Rarity item_roll_rarity(int difficulty)
{
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 5) difficulty = 5;

    /* Weights (sum = 100): Common decreases 5/diff, shifted to Rare+ */
    int common   = 60 - difficulty * 5;   /* 60 → 35 */
    int uncommon = 25;
    int rare     = 10 + difficulty * 2;   /* 10 → 20 */
    int epic     =  4 + difficulty * 2;   /*  4 → 14 */
    /* legendary = rest: 1 + difficulty */

    int roll = rand() % 100;
    if (roll < common)                            return RARITY_COMMON;
    if (roll < common + uncommon)                 return RARITY_UNCOMMON;
    if (roll < common + uncommon + rare)          return RARITY_RARE;
    if (roll < common + uncommon + rare + epic)   return RARITY_EPIC;
    return RARITY_LEGENDARY;
}

/* ------------------------------------------------------------------ */
/* Consumable factories                                                  */
/* ------------------------------------------------------------------ */

Item item_make_stimpack(void)
{
    Item it;
    memset(&it, 0, sizeof(it));
    strncpy(it.name, "Stim Pack", sizeof(it.name) - 1);
    it.type      = ITEM_CONSUMABLE;
    it.rarity    = RARITY_COMMON;
    it.stat_bonus = 2;   /* heals 2 HP */
    it.value     = 60;
    return it;
}

Item item_make_medkit(void)
{
    Item it;
    memset(&it, 0, sizeof(it));
    strncpy(it.name, "Med Kit", sizeof(it.name) - 1);
    it.type      = ITEM_CONSUMABLE;
    it.rarity    = RARITY_COMMON;
    it.stat_bonus = 5;   /* heals 5 HP */
    it.value     = 150;
    return it;
}

Item item_make_trauma_kit(void)
{
    Item it;
    memset(&it, 0, sizeof(it));
    strncpy(it.name, "Trauma Kit", sizeof(it.name) - 1);
    it.type      = ITEM_CONSUMABLE;
    it.rarity    = RARITY_UNCOMMON;
    it.stat_bonus = 8;   /* heals 8 HP */
    it.value     = 280;
    return it;
}

/* ------------------------------------------------------------------ */
/* Random item generation                                               */
/* ------------------------------------------------------------------ */

Item item_generate_random(Rarity rarity)
{
    Item it;
    memset(&it, 0, sizeof(it));

    /* Only pick equippable types — never consumables */
    static const ItemType equip_types[] = {
        ITEM_WEAPON, ITEM_ARMOR_HEAD, ITEM_ARMOR_BODY, ITEM_ARMOR_LEGS, ITEM_DECK,
    };
    ItemType type = equip_types[rand() % 5];

    const BaseItem *base = NULL;
    int             base_count = 0;
    const ItemMod  *mod_pool = NULL;
    int             mod_pool_count = 0;

    switch (type) {
    case ITEM_WEAPON:
        base = BASE_WEAPONS; base_count = BASE_WEAPONS_COUNT;
        mod_pool = WEAPON_MODS; mod_pool_count = WEAPON_MODS_COUNT;
        break;
    case ITEM_ARMOR_HEAD:
        base = BASE_HEADS; base_count = BASE_HEADS_COUNT;
        mod_pool = ARMOR_MODS; mod_pool_count = ARMOR_MODS_COUNT;
        break;
    case ITEM_ARMOR_BODY:
        base = BASE_BODIES; base_count = BASE_BODIES_COUNT;
        mod_pool = ARMOR_MODS; mod_pool_count = ARMOR_MODS_COUNT;
        break;
    case ITEM_ARMOR_LEGS:
        base = BASE_LEGS; base_count = BASE_LEGS_COUNT;
        mod_pool = ARMOR_MODS; mod_pool_count = ARMOR_MODS_COUNT;
        break;
    case ITEM_DECK:
        base = BASE_DECKS; base_count = BASE_DECKS_COUNT;
        mod_pool = DECK_MODS; mod_pool_count = DECK_MODS_COUNT;
        break;
    default:
        break;
    }

    const BaseItem *b = &base[rand() % base_count];
    strncpy(it.name, b->name, sizeof(it.name) - 1);
    it.type       = type;
    it.rarity     = rarity;
    it.stat_bonus = b->stat_bonus;
    it.range      = b->range;

    /* Number of bonus mods granted by rarity: Common=0, Uncommon=1, ..., Legendary=4 */
    int extra_mods = (int)rarity;
    if (extra_mods > ITEM_MOD_MAX)      extra_mods = ITEM_MOD_MAX;
    if (extra_mods > mod_pool_count)    extra_mods = mod_pool_count;

    /* Add unique mods from the pool (no duplicates).
     * FIX: used[] must be large enough for the whole pool (was hardcoded [4]). */
    bool used[16] = {0};
    for (int m = 0; m < extra_mods; m++) {
        int tries = 0;
        int idx;
        do {
            idx = rand() % mod_pool_count;
            tries++;
        } while (used[idx] && tries < 20);
        used[idx] = true;
        it.mods[it.mod_count++] = mod_pool[idx];
    }

    /* Value scales with rarity: Common×1, Uncommon×1.5, Rare×2.5, Epic×4, Legendary×7 */
    static const int rarity_mult[RARITY_COUNT] = { 100, 150, 250, 400, 700 };
    it.value = b->base_value * rarity_mult[rarity] / 100;

    return it;
}

/* ------------------------------------------------------------------ */
/* Mission loot                                                          */
/* ------------------------------------------------------------------ */

void item_loot_mission(int difficulty, Item *out, int *count)
{
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 5) difficulty = 5;

    /* 1-3 items; higher difficulty more likely to give 2-3 */
    int n = 1 + rand() % 3;
    if (n > 3) n = 3;
    *count = n;

    for (int i = 0; i < n; i++) {
        /* 15% chance of a consumable instead of equipment */
        if (rand() % 100 < 15) {
            int roll = rand() % 3;
            if      (roll == 0) out[i] = item_make_stimpack();
            else if (roll == 1) out[i] = item_make_medkit();
            else                out[i] = item_make_trauma_kit();
        } else {
            Rarity r = item_roll_rarity(difficulty);
            out[i] = item_generate_random(r);
        }
    }
}
