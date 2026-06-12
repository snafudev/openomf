/**
 * AI skills config loader
 *
 * Loads and caches per-HAR character skill config metadata.
 */

#ifndef AI_SKILLS_CONFIG_LOADER_H
#define AI_SKILLS_CONFIG_LOADER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct ai_char_config {
    int har_id;
    bool loaded_from_file;
    bool has_charge_moves;
    bool has_push_moves;
    bool has_projectile_moves;
    uint8_t charge_move_count;
    uint8_t push_move_count;
    uint8_t projectile_move_count;
} ai_char_config;

/**
 * Return cached character config for the given HAR id.
 * Falls back to built-in defaults when config is unavailable.
 */
const ai_char_config *ai_skills_config_get(int har_id);

/**
 * Reset cached character skill config state.
 */
void ai_skills_config_reset_cache(void);

#endif // AI_SKILLS_CONFIG_LOADER_H
