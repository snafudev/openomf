/**
 * AI tactic engine
 *
 * Encapsulates tactic preference checks and queue setup logic.
 */

#ifndef AI_TACTIC_ENGINE_H
#define AI_TACTIC_ENGINE_H

#include "controller/controller.h"
#include "formats/pilot.h"
#include <stdbool.h>
#include <stddef.h>

enum
{
    TACTIC_ESCAPE = 1, // escape from enemy
    TACTIC_TURTLE,     // block attacks
    TACTIC_GRAB,       // charge and grab enemy
    TACTIC_SPAM,       // spam the same attack
    TACTIC_SHOOT,      // shoot a projectile
    TACTIC_TRIP,       // trip enemy
    TACTIC_QUICK,      // quick attack
    TACTIC_CLOSE,      // close with the enemy
    TACTIC_FLY,        // fly towards the enemy
    TACTIC_PUSH,       // spam power moves to push them back
    TACTIC_COUNTER     // block then attack
};

enum
{
    MOVE_CLOSE = 1, // close distance
    MOVE_AVOID,     // gain distance
    MOVE_JUMP,      // jump towards
    MOVE_HIGH_JUMP, // high-jump towards
    MOVE_BLOCK      // hold block
};

enum
{
    ATTACK_ID = 1, // attack by id
    ATTACK_TRIP,   // trip attack
    ATTACK_GRAB,   // grab/throw attack
    ATTACK_LIGHT,  // light/quick attack
    ATTACK_HEAVY,  // heavy/power attack
    ATTACK_JUMP,   // jumping attack
    ATTACK_RANGED, // ranged attack
    ATTACK_CHARGE, // charge attack
    ATTACK_PUSH,   // push attack
    ATTACK_RANDOM, // random attack
};

bool ai_tactic_likes_it(const controller *ctrl, int tactic_type);
void ai_tactic_queue(controller *ctrl, int tactic_type);
void ai_tactic_consider_list(controller *ctrl, int tactics[], size_t n_tactics);

/**
 * Return the first enabled tactic id from a tactic candidate list.
 * Returns 0 when none of the provided tactics are enabled.
 */
int ai_tactic_first_enabled(const int *tactics, size_t n_tactics);

/**
 * Return configured move_type metadata string for a tactic, or empty string.
 */
const char *ai_tactic_config_move_type(int tactic_type);

/**
 * Return whether configured move_type token is recognized by queue mapping.
 */
bool ai_tactic_config_move_type_supported(int tactic_type);

/**
 * Return configured attack_type metadata string for a tactic, or empty string.
 */
const char *ai_tactic_config_attack_type(int tactic_type);

/**
 * Return whether configured attack_type token is recognized by queue mapping.
 */
bool ai_tactic_config_attack_type_supported(int tactic_type);

/**
 * Return whether a tactic config contains the given condition token.
 */
bool ai_tactic_config_has_condition(int tactic_type, const char *condition);

/**
 * Return number of parsed condition tokens for a tactic config entry.
 */
int ai_tactic_config_condition_count(int tactic_type);

/**
 * Return whether all parsed condition tokens for a tactic are recognized.
 */
bool ai_tactic_config_conditions_supported(int tactic_type);

/**
 * Return whether the specific condition token is recognized by tactic logic.
 */
bool ai_tactic_config_condition_token_supported(const char *condition);

/**
 * Evaluate parsed tactic conditions against runtime context values.
 * Unknown condition tokens are treated as matched for behavioral safety.
 */
bool ai_tactic_conditions_match_context(int tactic_type, int har_id, int enemy_range, int thrown, int shot,
                                        const sd_pilot *pilot);

/**
 * Return whether a tactic is currently enabled by config.
 * If config is unavailable, valid tactic ids are treated as enabled.
 */
bool ai_tactic_is_enabled(int tactic_type);

/**
 * Reset cached tactic config state so subsequent queries re-read tactics.json.
 */
void ai_tactic_reset_config_cache(void);

#endif // AI_TACTIC_ENGINE_H
