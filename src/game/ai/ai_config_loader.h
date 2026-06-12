/**
 * AI config loader
 *
 * Loads AI personality configuration from resource files.
 */

#ifndef AI_CONFIG_LOADER_H
#define AI_CONFIG_LOADER_H

#include "formats/pilot.h"
#include <stdbool.h>

/**
 * Load pilot personality fields from resources/ai_config/pilots.json.
 *
 * Returns true when pilot data was found and applied.
 * Returns false when config is unavailable, malformed, or pilot is missing.
 */
bool ai_config_load_pilot_personality(sd_pilot *pilot);

#endif // AI_CONFIG_LOADER_H
