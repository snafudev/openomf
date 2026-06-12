/**
 * AI Core Configuration Loader
 *
 * Loads core AI configuration parameters from ai_config/ai_core.ini
 */

#ifndef AI_CORE_CONFIG_H
#define AI_CORE_CONFIG_H

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
} ai_core_config;

/**
 * \brief Get or create the singleton AI core config instance
 *
 * \return Pointer to the AI core config structure with loaded values
 */
const ai_core_config *ai_core_config_get(void);

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
