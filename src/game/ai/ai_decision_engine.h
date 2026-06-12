/**
 * AI Decision Engine
 *
 * Pure decision functions for AI difficulty scaling, pilot preferences, and learning.
 * These functions have no side effects and are easily testable.
 */

#ifndef AI_DECISION_ENGINE_H
#define AI_DECISION_ENGINE_H

#include "game/ai/ai_types.h"
#include <stdbool.h>

/**
 * \brief Roll a chance with given threshold (1 to roll_x).
 *
 * Lower roll_x means higher chance of success.
 * - roll_x <= 1: always true (100%)
 * - roll_x = 2: 50% chance
 * - roll_x = 100: 1% chance
 *
 * \param roll_x The denominator for the chance (1/roll_x chance of success)
 * \return A boolean indicating if the roll was successful.
 */
bool roll_chance(int roll_x);

/**
 * \brief Roll chance for pilot preference.
 *
 * Maps pilot preference (-100 to 100) to a probability threshold.
 * Preference is added to 100 to get threshold (0 to 200).
 * Random roll of 0-199, if <= threshold, return true.
 *
 * \param pref_val The value of the pilot preference (-100 to 100)
 * \return A boolean indicating whether the preference is confirmed.
 */
bool roll_pref(int pref_val);

/**
 * \brief Determine whether the AI is smart enough to usually go ahead with an action.
 *
 * At difficulty 6+, very likely (92%). Medium difficulties moderate. Low difficulties no.
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI is smart enough.
 */
bool smart_usually(const ai *a);

/**
 * \brief Determine whether the AI is dumb enough to usually go ahead with an action.
 *
 * At difficulty 1-2, very likely (92%). Higher difficulties no.
 * Inverse of smart_usually for difficulty scaling.
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI is dumb enough.
 */
bool dumb_usually(const ai *a);

/**
 * \brief Determine whether the AI is smart enough to sometimes go ahead with an action.
 *
 * Requires difficulty 2+, then rolls with decreasing chance as difficulty increases.
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI is smart enough.
 */
bool smart_sometimes(const ai *a);

/**
 * \brief Determine whether the AI is dumb enough to sometimes go ahead with an action.
 *
 * Requires difficulty <= 2, then rolls with increasing chance as difficulty increases.
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI is dumb enough.
 */
bool dumb_sometimes(const ai *a);

/**
 * \brief Determine whether AI will proceed with an action using exponentially scaling roll.
 *
 * Probability = (difficulty^2) / 36
 * - Difficulty 1: 1/36 = 2.8%
 * - Difficulty 3: 9/36 = 25%
 * - Difficulty 6: 36/36 = 100%
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI should proceed with an action.
 */
bool diff_scale(const ai *a);

/**
 * \brief Determine whether AI will learn from this moment.
 *
 * Uses pilot's learning personality value.
 * Higher difficulty rolls give more frequent learning opportunities.
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI should learn.
 */
bool learning_moment(const ai *a);

/**
 * \brief Determine whether AI will forget something.
 *
 * Uses pilot's forget personality value.
 * Higher difficulty rolls give more frequent forget opportunities.
 *
 * \param a The AI instance.
 * \return A boolean indicating whether the AI should forget.
 */
bool forgetful(const ai *a);

#endif // AI_DECISION_ENGINE_H
