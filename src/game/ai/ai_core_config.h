/**
 * AI Core Configuration Loader
 *
 * Loads core AI configuration parameters from ai_config/ai_core.ini
 */

#ifndef AI_CORE_CONFIG_H
#define AI_CORE_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief AI core configuration structure
 */
typedef struct {
    int base_act_chance;
    int base_fwd_jump_chance;
    int base_back_jump_chance;
    int base_still_jump_chance;
    int random_attack_chance;
    int base_act_timer;
    // Percentage likelihood (0-100) to block instead of attacking.
    // Higher = block more often (the one "higher = more likely" knob).
    int block_chance;
    // 0 = default tactic gating, 1 = also allow advanced tactics (e.g. COUNTER).
    int aggressive_tactics;
    // Jump frequency multiplier applied as a divisor on the roll_chance operand.
    // 100 = normal, higher = jump more often.
    int jump_frequency_mult;
} ai_core_config;

/**
 * \brief Get or create the singleton AI core config instance
 *
 * \return Pointer to the AI core config structure with loaded values
 */
const ai_core_config *ai_core_config_get(void);

/**
 * \brief Load the AI core configuration for a specific difficulty level.
 *
 * The base ai_core.ini values are always loaded first, then any matching
 * difficulty file under ai_config/ai_difficulty overrides those values.
 */
bool ai_core_config_load_for_difficulty(int difficulty, ai_core_config *config);

/**
 * \brief Fetch the AI core configuration for a specific difficulty level.
 *
 * This returns a cached per-difficulty snapshot created on demand.
 */
const ai_core_config *ai_core_config_get_for_difficulty(int difficulty);

/**
 * \brief Reload AI core configuration from disk
 *
 * \return true if configuration was successfully reloaded, false otherwise
 */
bool ai_core_config_reload(void);

/**
 * \brief Free the AI core config singleton
 */
void ai_core_config_free(void);

#endif // AI_CORE_CONFIG_H
