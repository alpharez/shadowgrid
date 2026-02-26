#pragma once

#include "entity.h"
#include "hub.h"   /* GameState */

/* ------------------------------------------------------------------ */
/* Hub NPC types                                                        */
/* ------------------------------------------------------------------ */

typedef enum {
    HUB_NPC_ARMS,       /* black market arms dealer  */
    HUB_NPC_RIPPERDOC,  /* cybernetics / augmentations */
    HUB_NPC_TECH,       /* armor, decks, tech gear   */
    HUB_NPC_FIXER,      /* mission board             */
    HUB_NPC_MEDIC,      /* consumables / street doc  */
    HUB_NPC_FENCE,      /* sell inventory items      */
    HUB_NPC_AMBIENT,    /* flavor NPC — dialog only  */
} HubNpcType;

/* ------------------------------------------------------------------ */
/* Hub NPC struct                                                       */
/* ------------------------------------------------------------------ */

#define HUB_DIALOG_LINES 3
#define HUB_DIALOG_LEN   80

typedef struct {
    int        x, y;
    char       symbol;
    int        color_pair;   /* COL_HUB_* constant from render.h */
    char       name[32];
    char       dialog[HUB_DIALOG_LINES][HUB_DIALOG_LEN];
    int        dialog_count;
    HubNpcType type;
} HubNpc;

#define HUB_NPC_MAX 12

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

/* Run the physical hub map loop. Returns next GameState. */
GameState hub_map_run(Entity *player);
