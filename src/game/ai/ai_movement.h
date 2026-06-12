/**
 * AI Movement Module
 *
 * Pure functions for deciding movement direction and baseline jump probability
 * based on AI state, enemy range, and pilot preferences.
 * These functions have no side effects and are easily testable.
 */

#ifndef AI_MOVEMENT_H
#define AI_MOVEMENT_H

#include "game/ai/ai_types.h"
#include <stdbool.h>

enum
{
    MOVE_DIR_STILL = 0,
    MOVE_DIR_FWD,
    MOVE_DIR_BACK
};

/**
 * \brief Decide movement direction based on game state parameters.
 *
 * Selects between MOVE_DIR_STILL, MOVE_DIR_FWD, and MOVE_DIR_BACK using
 * pilot preferences, enemy range, wall proximity, and HAR character identity.
 *
 * \param a The AI instance (provides difficulty and pilot preferences).
 * \param enemy_range Distance classification (RANGE_CRAMPED..RANGE_FAR).
 * \param is_wallhugging Whether the AI's HAR is pressed against the wall.
 * \param har_id The HAR character ID (used for brawler special-cases).
 *
 * \return MOVE_DIR_STILL, MOVE_DIR_FWD, or MOVE_DIR_BACK.
 */
int ai_movement_decide(const ai *a, int enemy_range, bool is_wallhugging, int har_id);

/**
 * \brief Calculate the baseline mid-action jump chance.
 *
 * Computes the initial jump probability threshold before any movement
 * direction decision is applied. Higher return value means less likely to jump
 * (threshold used with roll_chance).
 *
 * \param a The AI instance (provides difficulty and pilot preferences).
 *
 * \return Jump chance threshold (0-100; 100 = almost never, lower = more likely).
 */
int ai_movement_jump_chance(const ai *a);

#endif // AI_MOVEMENT_H
