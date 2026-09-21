/**
 * AI Learning Module
 *
 * Encapsulates pilot personality adaptation based on repeated opponent behaviour.
 * Functions are extracted from ai_har_event() in ai_controller.c.
 */

#ifndef AI_LEARNING_H
#define AI_LEARNING_H

#include "game/ai/ai_types.h"
#include <stdbool.h>

/** Times thrown before AI adjusts to avoid being thrown. */
#define MAX_TIMES_THROWN 3
/** Times shot before AI adjusts to avoid repeated projectiles. */
#define MAX_TIMES_SHOT 4

/**
 * \brief Increment the thrown counter and adjust pilot personality if threshold met.
 *
 * Should be called when the AI takes a CAT_CLOSE (throw/grab) hit.
 * Adjusts pilot to favor sniper, jump, and backwards-movement tactics.
 *
 * \param a The AI instance.
 */
void ai_learning_adjust_from_throw(ai *a);

/**
 * \brief Increment the shot counter and adjust pilot personality if threshold met.
 *
 * Should be called when the AI takes a projectile hit (HAR_EVENT_TAKE_HIT_PROJECTILE).
 * Adjusts pilot to favor aggressive forward/jump-in approaches.
 *
 * \param a The AI instance.
 */
void ai_learning_adjust_from_projectile(ai *a);

/**
 * \brief Return the maximum projectile burst size for a given difficulty.
 */
int ai_projectile_max_streak(int difficulty);

/**
 * \brief Tick the projectile cooldown timer and clear burst pressure once it expires.
 */
void ai_projectile_tick(ai *a);

/**
 * \brief Check whether the AI is allowed to fire another projectile now.
 */
bool ai_projectile_is_allowed(const ai *a);

/**
 * \brief Register a projectile use and apply the burst cooldown.
 */
void ai_projectile_use(ai *a);

/**
 * \brief Return the maximum pressure-burst size for a given difficulty.
 */
int ai_pressure_max_streak(int difficulty);

/**
 * \brief Tick the pressure cooldown timer and clear pressure burst once it expires.
 */
void ai_pressure_tick(ai *a);

/**
 * \brief Check whether the AI is allowed to continue a pressure-heavy attack pattern.
 */
bool ai_pressure_is_allowed(const ai *a);

/**
 * \brief Register repeated pressure-heavy usage and apply the burst cooldown.
 */
void ai_pressure_use(ai *a);

/**
 * \brief Randomly forget learned adaptations.
 *
 * Rolls a chance. If successful and the pilot has a high forget value, the pilot
 * personality is reset and all learning counters are cleared.
 *
 * \param a The AI instance.
 * \return true if the AI forgot its learning, false otherwise.
 */
bool ai_learning_maybe_forget(ai *a);

#endif // AI_LEARNING_H
