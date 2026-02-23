#include "skilltree.h"

const SkillTree TREES[TREE_COUNT] = {

    /* ---------------------------------------------------------------- */
    /* SHADOW — Stealth, infiltration, movement                          */
    /* ---------------------------------------------------------------- */
    {
        "SHADOW",
        "The best operatives are the ones nobody knows exist.",
        {
            { 3, {
                { "Light Step",      "Reduced movement noise when traversing the facility." },
                { "Quick Crouch",    "Instant stance switch with no action penalty."        },
                { "Keen Eyes",       "Extended vision range in low-light conditions."       },
            }},
            { 3, {
                { "Shadow Meld",     "Standing still in darkness renders you invisible."    },
                { "Grey Man",        "Suspicious guards lose interest twice as fast."       },
                { "Wall Hug",        "Staying adjacent to a wall reduces detection range by 1." },
            }},
            { 3, {
                { "Silent Takedown", "Melee takedowns produce zero sound."                  },
                { "Predict Patrol",  "See guard patrol routes 3 turns ahead."              },
                { "Vent Rat",        "Move through vent systems at full movement speed."    },
            }},
            { 3, {
                { "Vanish",          "Breaking line of sight triggers instant de-aggro."   },
                { "Disguise Master", "Stolen uniforms last twice as long before suspicion." },
                { "Cat Fall",        "No fall damage; landing is completely silent."        },
            }},
            { 1, {
                { "Ghost Protocol",  "5 turns of complete invisibility. Once per mission." },
            }},
        },
    },

    /* ---------------------------------------------------------------- */
    /* NETRUNNER — Hacking, tech, engineering                            */
    /* ---------------------------------------------------------------- */
    {
        "NETRUNNER",
        "Why walk through a door when you can open it from across the street?",
        {
            { 3, {
                { "Quick Hack",      "Hack execution speed increased by 25%."              },
                { "Scavenger",       "Extract better components from defeated tech."        },
                { "Deep Scan",       "Passively reveal nearby network nodes."               },
            }},
            { 3, {
                { "Ghost Signal",    "Trace buildup rate reduced by 40%."                  },
                { "Trap Master",     "Craft and remotely trigger electronic traps."         },
                { "Overclock",       "Push deck and cyberware beyond rated specs briefly."  },
            }},
            { 3, {
                { "Daemon",          "Deploy an autonomous hack program as a distraction."  },
                { "Drone Buddy",     "Scout drone follows you and highlights hostiles."     },
                { "EMP Specialist",  "EMP grenade and ability radius doubled."              },
            }},
            { 3, {
                { "Backdoor King",   "Compromised systems stay hacked permanently."        },
                { "Spoof",           "Fake security signals; misdirect guards at will."    },
                { "Remote Detonate", "Trigger any electronic device as a weapon."          },
            }},
            { 1, {
                { "GOD MODE",        "Full facility network control for 3 turns. Once/mission." },
            }},
        },
    },

    /* ---------------------------------------------------------------- */
    /* CHROME — Combat, cyberware, physicality                           */
    /* ---------------------------------------------------------------- */
    {
        "CHROME",
        "When the plan falls apart, you ARE the plan.",
        {
            { 3, {
                { "Quick Draw",      "Your first shot each combat costs no action."        },
                { "Tough",           "Maximum HP increased by 20%."                        },
                { "Iron Grip",       "No accuracy penalty while moving and firing."        },
            }},
            { 3, {
                { "Bullet Time",     "Take 2 actions in 1 turn. Once per mission."         },
                { "CQC",             "Melee attacks disarm opponents."                     },
                { "Adrenaline",      "Kills restore a small amount of HP."                 },
            }},
            { 3, {
                { "Suppressive Fire","Pin enemies behind cover; they cannot act."          },
                { "Counterattack",   "Successful dodge triggers a free melee strike."      },
                { "Pain Editor",     "Ignore wound penalties until combat ends."           },
            }},
            { 3, {
                { "Chrome Rush",     "Charge through enemies, knocking them aside."        },
                { "Precise Shot",    "Called shots to limbs or weapons."                   },
                { "Second Wind",     "Auto-revive from lethal damage once per mission."    },
            }},
            { 1, {
                { "REAPER",          "Every kill grants a bonus action. Chainable. Once/mission." },
            }},
        },
    },
};

/* Cumulative points needed in a tree to unlock each tier.
 * Each skill costs 1 SP; tiers 0-3 have up to 3 options = 12 pts max.
 *   T1 always open; T2 needs 2 (buy 2 tier-1 skills);
 *   T3 needs 5 (all T1 + 2 T2); T4 needs 8 (all T1+T2 + 2 T3);
 *   T5 needs 11 (all T1-T3 + 2 T4) */
const int TIER_REQ[TREE_TIERS] = { 0, 2, 5, 8, 11 };

bool skilltree_tier_unlocked(int points_in_tree, int tier)
{
    return points_in_tree >= TIER_REQ[tier];
}
