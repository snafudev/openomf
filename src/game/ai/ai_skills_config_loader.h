/**
 * AI skills config loader
 *
 * Loads and caches per-HAR character skill config metadata.
 */

#ifndef AI_SKILLS_CONFIG_LOADER_H
#define AI_SKILLS_CONFIG_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#define AI_MOVE_MAX_INPUTS          8
#define AI_MOVE_MAX_FOLLOW_TACTICS  8
#define AI_MOVE_NAME_LEN            32
#define AI_MAX_MOVES_PER_TYPE       8

typedef enum {
    MOVE_COND_NONE              = 0,
    MOVE_COND_HIGH_DIFFICULTY        = 1 << 0, // diff_scale(a)
    MOVE_COND_SPECIAL_PREF      = 1 << 1, // roll_pref(ap_special)
    MOVE_COND_LOW_PREFERRED     = 1 << 2, // roll_pref(ap_low)
    MOVE_COND_JUMP_PREFERRED    = 1 << 3, // roll_pref(att_jump)
    MOVE_COND_ROLL_D2           = 1 << 4, // roll_chance(2)
    MOVE_COND_ROLL_D3           = 1 << 5, // roll_chance(3)
    MOVE_COND_ENEMY_NOT_STUNNED = 1 << 6, // !enemy_is_stunned_or_stasis
    MOVE_COND_ROLL_D4           = 1 << 7, // roll_chance(4)
    MOVE_COND_ROLL_D10          = 1 << 8, // roll_chance(10)
    MOVE_COND_ROLL_D20          = 1 << 9, // roll_chance(20)
} ai_move_condition;

typedef enum {
    MOVE_RANGE_ANY = 0,
    MOVE_RANGE_CLOSE,
    MOVE_RANGE_MID,
    MOVE_RANGE_FAR,
} ai_move_range;

typedef struct {
    char              name[AI_MOVE_NAME_LEN];
    int               inputs[AI_MOVE_MAX_INPUTS];
    uint8_t           input_count;
    ai_move_range     range_min;
    ai_move_range     range_max; // MOVE_RANGE_FAR = no max constraint
    ai_move_condition conditions;
    int               follow_up_tactics[AI_MOVE_MAX_FOLLOW_TACTICS];
    uint8_t           follow_up_tactic_count;
} ai_move_def;

typedef struct ai_har_config {
    int     har_id;
    bool    loaded_from_file;
    bool    has_charge_moves;
    bool    has_push_moves;
    bool    has_projectile_moves;
    uint8_t charge_move_count;
    uint8_t push_move_count;
    uint8_t projectile_move_count;
    ai_move_def charge_moves[AI_MAX_MOVES_PER_TYPE];
    ai_move_def push_moves[AI_MAX_MOVES_PER_TYPE];
    ai_move_def projectile_moves[AI_MAX_MOVES_PER_TYPE];
} ai_har_config;

/**
 * Return cached character config for the given HAR id.
 * Falls back to built-in defaults when config is unavailable.
 */
const ai_har_config *ai_skills_config_get(int har_id);

/**
 * Apply a character config overlay from a raw JSON buffer.
 * Only move arrays present in json_buf are updated; missing arrays are unchanged.
 *
 * Returns true if at least one array field was applied.
 */
bool ai_skills_config_apply_overlay(ai_har_config *cfg, const char *json_buf);

/**
 * Reset cached character skill config state.
 */
void ai_skills_config_reset_cache(void);

#endif // AI_SKILLS_CONFIG_LOADER_H
