#pragma once

#include <stdbool.h>

/* These constants define Entity array sizes — included by entity.h */
#define TREE_COUNT    3
#define TREE_TIERS    5
#define TIER_MAX_OPTS 3

typedef struct {
    const char *name;
    const char *desc;
} TalentOpt;

typedef struct {
    int       opt_count;
    TalentOpt opts[TIER_MAX_OPTS];
} TreeTier;

typedef struct {
    const char *name;
    const char *tagline;
    TreeTier    tiers[TREE_TIERS];
} SkillTree;

extern const SkillTree TREES[TREE_COUNT];

/* Points in the tree required to unlock each tier (0-indexed).
 * Each skill costs 1 SP; tiers 0-3 have up to 3 options each (max 12 pts/tree).
 * Thresholds: T1=0, T2=2, T3=5, T4=8, T5=11 */
extern const int TIER_REQ[TREE_TIERS];

/* True if tier T is unlocked given `points_in_tree` already spent. */
bool skilltree_tier_unlocked(int points_in_tree, int tier);
