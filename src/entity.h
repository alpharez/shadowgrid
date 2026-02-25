#pragma once

#include "map.h"
#include "skilltree.h"
#include "items.h"

#define SKILL_COUNT 6

typedef enum {
    SKILL_STEALTH,
    SKILL_HACKING,
    SKILL_COMBAT,
    SKILL_ENGINEERING,
    SKILL_SOCIAL,
    SKILL_ATHLETICS,
} SkillType;

extern const char *SKILL_NAMES[SKILL_COUNT];

typedef struct {
    int  x, y;
    char symbol;

    /* Stats */
    int hp, max_hp;
    int credits;
    int level;
    int skill_points;
    int skills[SKILL_COUNT];

    /* Movement / stance */
    bool crouched;

    /* Experience and leveling */
    int xp;       /* accumulated XP this level */
    int xp_next;  /* XP threshold for next level */

    /* Set by mission selection, paid out on debrief */
    int active_mission_reward;
    int active_mission_difficulty; /* 1-5, stored directly to avoid reward math errors */

    /* Skill trees (SHADOW=0, NETRUNNER=1, CHROME=2) */
    int tree_points[TREE_COUNT];                 /* SP spent per tree (= skills bought) */
    int tree_bought[TREE_COUNT][TREE_TIERS];     /* bitmask: bit N = option N purchased */

    /* Inventory and equipment */
    Item inventory[INVENTORY_MAX];
    int  inv_count;
    Item equipped[EQUIP_SLOT_COUNT];  /* empty slot: type = ITEM_NONE */

    /* Loot awaiting collection at debrief */
    Item pending_loot[3];
    int  pending_loot_count;
} Entity;

/* Initialise all entity fields (call once at game start) */
void entity_init(Entity *e);

/* Reposition entity in the center of rooms[0]; does not touch stats */
void entity_place(Entity *e, const Map *map);

/* Attempt to move by (dx, dy); returns false if blocked */
bool entity_move(Entity *e, const Map *map, int dx, int dy);

/* Inventory management */
bool entity_inv_add(Entity *e, Item item);        /* false if full (INVENTORY_MAX) */
void entity_inv_remove(Entity *e, int idx);       /* shifts remaining items left */

/* Equipment: equip item from inventory slot; unequipped item returns to inventory */
void entity_equip(Entity *e, int inv_idx);
void entity_unequip(Entity *e, EquipSlot slot);   /* returns item to inventory if room */

/* Gear stat totals from equipped items */
int entity_gear_atk(const Entity *e);   /* sum of weapon stat_bonus */
int entity_gear_def(const Entity *e);   /* sum of armor stat_bonus */
int entity_gear_hack(const Entity *e);  /* deck stat_bonus */
int entity_gear_mod_bonus(const Entity *e, int skill); /* sum of mods[] bonuses for a skill */

/* Use the first Med Kit in inventory; heals stat_bonus HP up to max_hp.
 * Does nothing if no consumable is present. */
void entity_use_medkit(Entity *e);

/* Recompute derived stats (max_hp) from base values + purchased talents.
 * Call after buying any talent or after respec. */
void entity_recalc_stats(Entity *e);
