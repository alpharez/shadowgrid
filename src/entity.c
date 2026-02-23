#include "entity.h"
#include "items.h"

#include <string.h>

const char *SKILL_NAMES[SKILL_COUNT] = {
    "Stealth",
    "Hacking",
    "Combat",
    "Engineering",
    "Social",
    "Athletics",
};

void entity_init(Entity *e)
{
    memset(e, 0, sizeof(*e));
    e->symbol       = '@';
    e->hp           = 10;
    e->max_hp       = 10;
    e->credits      = 500;
    e->level        = 1;
    e->skill_points = 3;
    for (int i = 0; i < SKILL_COUNT; i++)
        e->skills[i] = 1;

    /* tree_points and tree_bought are already zeroed by memset above */

    /* TODO: remove — test loadout */
    Item flechette = {
        .name      = "Flechette Gun",
        .type      = ITEM_WEAPON,
        .rarity    = RARITY_UNCOMMON,
        .value     = 550,
        .stat_bonus = 2,
        .range     = 10,
    };
    entity_inv_add(e, flechette);
}

/* Reposition the entity in the first room of the map without touching stats. */
void entity_place(Entity *e, const Map *map)
{
    if (map->num_rooms > 0) {
        e->x = map->rooms[0].x + map->rooms[0].w / 2;
        e->y = map->rooms[0].y + map->rooms[0].h / 2;
    } else {
        e->x = MAP_WIDTH  / 2;
        e->y = MAP_HEIGHT / 2;
    }
}

bool entity_move(Entity *e, const Map *map, int dx, int dy)
{
    int nx = e->x + dx;
    int ny = e->y + dy;
    if (map_is_blocked(map, nx, ny))
        return false;
    e->x = nx;
    e->y = ny;
    return true;
}

/* ------------------------------------------------------------------ */
/* Inventory management                                                 */
/* ------------------------------------------------------------------ */

bool entity_inv_add(Entity *e, Item item)
{
    if (e->inv_count >= INVENTORY_MAX)
        return false;
    e->inventory[e->inv_count++] = item;
    return true;
}

void entity_inv_remove(Entity *e, int idx)
{
    if (idx < 0 || idx >= e->inv_count)
        return;
    for (int i = idx; i < e->inv_count - 1; i++)
        e->inventory[i] = e->inventory[i + 1];
    e->inv_count--;
}

void entity_equip(Entity *e, int inv_idx)
{
    if (inv_idx < 0 || inv_idx >= e->inv_count)
        return;
    Item *it = &e->inventory[inv_idx];
    if (!item_valid(it))
        return;
    if (it->type == ITEM_CONSUMABLE)
        return;  /* consumables are used, not equipped */

    EquipSlot slot = item_equip_slot(it->type);
    Item old = e->equipped[slot];
    e->equipped[slot] = *it;
    entity_inv_remove(e, inv_idx);

    /* Return the previously equipped item to inventory (if any) */
    if (item_valid(&old))
        entity_inv_add(e, old);
}

void entity_unequip(Entity *e, EquipSlot slot)
{
    if (!item_valid(&e->equipped[slot]))
        return;
    if (entity_inv_add(e, e->equipped[slot]))
        memset(&e->equipped[slot], 0, sizeof(Item));
    /* If inventory is full, item stays equipped */
}

/* ------------------------------------------------------------------ */
/* Gear stat totals                                                      */
/* ------------------------------------------------------------------ */

int entity_gear_atk(const Entity *e)
{
    const Item *w = &e->equipped[EQUIP_WEAPON];
    return item_valid(w) ? w->stat_bonus : 0;
}

int entity_gear_def(const Entity *e)
{
    int total = 0;
    for (int s = EQUIP_HEAD; s <= EQUIP_LEGS; s++) {
        const Item *it = &e->equipped[s];
        if (item_valid(it))
            total += it->stat_bonus;
    }
    return total;
}

int entity_gear_hack(const Entity *e)
{
    const Item *d = &e->equipped[EQUIP_DECK];
    return item_valid(d) ? d->stat_bonus : 0;
}

void entity_recalc_stats(Entity *e)
{
    /* Base values */
    int max_hp = 10;

    /* CHROME tree (2), tier 0, option 1: Tough — maximum HP +20% */
    if (e->tree_bought[2][0] & (1 << 1))
        max_hp = max_hp * 12 / 10;   /* 10 → 12 */

    e->max_hp = max_hp;
    if (e->hp > e->max_hp)
        e->hp = e->max_hp;
}

void entity_use_medkit(Entity *e)
{
    for (int i = 0; i < e->inv_count; i++) {
        if (e->inventory[i].type == ITEM_CONSUMABLE) {
            e->hp += e->inventory[i].stat_bonus;
            if (e->hp > e->max_hp)
                e->hp = e->max_hp;
            entity_inv_remove(e, i);
            return;
        }
    }
}
