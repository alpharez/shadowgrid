#pragma once

#include <stdbool.h>

#define INVENTORY_MAX    20
#define EQUIP_SLOT_COUNT  5
#define ITEM_MOD_MAX      4

typedef enum {
    ITEM_NONE = 0,
    ITEM_WEAPON,
    ITEM_ARMOR_HEAD,
    ITEM_ARMOR_BODY,
    ITEM_ARMOR_LEGS,
    ITEM_DECK,
    ITEM_CONSUMABLE,   /* stat_bonus = HP restored on use */
    ITEM_TYPE_COUNT,
} ItemType;

typedef enum {
    RARITY_COMMON = 0,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY,
    RARITY_COUNT,
} Rarity;

typedef enum {
    EQUIP_WEAPON = 0,
    EQUIP_HEAD,
    EQUIP_BODY,
    EQUIP_LEGS,
    EQUIP_DECK,
} EquipSlot;

typedef struct {
    int         skill;  /* SKILL_* index this mod buffs (0=Stealth,1=Hacking,2=Combat) */
    int         bonus;
    const char *label;  /* display string, e.g. "+1 STL" */
} ItemMod;

typedef struct {
    char     name[40];
    ItemType type;
    Rarity   rarity;
    int      value;       /* credit value; sell for value/2 */
    int      stat_bonus;  /* ATK for weapons, DEF for armor, HACK for decks */
    int      range;       /* 0 = melee/non-weapon; >0 = ranged range in tiles */
    int      mod_count;
    ItemMod  mods[ITEM_MOD_MAX];
} Item;

/* Metadata helpers */
const char *item_type_label(ItemType t);    /* "WEAPON", "HEAD", "BODY", "LEGS", "DECK" */
const char *item_rarity_name(Rarity r);     /* "Common", "Uncommon", etc. */
int         item_rarity_color(Rarity r);    /* returns COL_RARITY_* pair index (11-15) */
EquipSlot   item_equip_slot(ItemType t);    /* maps type to its equip slot */
bool        item_valid(const Item *it);     /* type != ITEM_NONE */

/* Consumable factory */
Item item_make_medkit(void);   /* Common, heals 5 HP, value 150¢ */

/* Loot generation */
Rarity item_roll_rarity(int difficulty);              /* weighted random; difficulty 0-5 */
Item   item_generate_random(Rarity rarity);           /* random equipment type + mods */
void   item_loot_mission(int difficulty, Item *out, int *count); /* fills out[0..2], sets count */
