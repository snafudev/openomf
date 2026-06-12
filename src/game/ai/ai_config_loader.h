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
 * Load pilot personality fields from resources/ai_config/pilots.json,
 * then apply any mod overlays from the mod system.
 *
 * Returns true when pilot data was found and applied.
 * Returns false when config is unavailable, malformed, or pilot is missing.
 */
bool ai_config_load_pilot_personality(sd_pilot *pilot);

/**
 * Apply pilot personality fields from a raw pilots.json buffer (partial overlay).
 * Only fields present in json_buf are updated; missing fields are left unchanged.
 *
 * Returns true if at least one field was applied.
 */
bool ai_config_apply_pilot_overlay(sd_pilot *pilot, const char *json_buf);

#endif // AI_CONFIG_LOADER_H
