/**
 * AI Move Selector Module
 *
 * Modularizes move evaluation and selection logic from ai_controller.c.
 * Provides testable, configuration-ready move scoring and filtering.
 *
 * Key responsibilities:
 * - Validate moves based on HAR state and game constraints
 * - Score moves based on learning, damage, preferences, and difficulty
 * - Select the best move from a list of candidate moves
 *
 * No mutation of global state: functions return results rather than
 * modifying controller or AI objects directly.
 */

#ifndef AI_MOVE_SELECTOR_H
#define AI_MOVE_SELECTOR_H

#include "resources/af_move.h"
#include "formats/pilot.h"
#include "game/ai/ai_types.h"
#include <stdbool.h>

/**
 * \brief Context for move evaluation and selection.
 *
 * Encapsulates all data needed to score and select moves without
 * coupling to controller or ai objects.
 */
typedef struct {
    // Move statistics array (indexed by move ID)
    move_stat *move_stats;
    
    // HAR state for validity checks
    const void *har;  // typedef struct { uint8_t state, close, id; } test_har_state;
    
    // Scoring mode
    bool highest_damage;
    
    // AI difficulty level
    int difficulty;
    
    // Pilot data for preference checks
    sd_pilot pilot;
    
    // Enemy range (for attempt_attack context)
    int enemy_range;
    
    // Last move ID (to avoid repetition)
    int last_move_id;
    
    // Damage bonus divisor for smart AI (typically 3 or 4)
    // Set to 3 for assign_move_by_cat, 4 for attempt_attack
    int damage_divisor;
    
    // Whether to force-allow projectile moves (assign_move_by_cat uses true, attempt_attack uses false)
    bool force_allow_projectile;
} move_stat_context;

/**
 * \brief Validate a move based on HAR state and constraints.
 *
 * Checks if a move can be executed given the current HAR state:
 * - Jump state constraints
 * - Distance constraints (close/mid/high/low)
 * - Category-specific constraints (projectile, scrap, etc.)
 * - Move string validation
 *
 * This is the extracted version of is_valid_move() from ai_controller.c,
 * generalized to take HAR state parameters instead of direct object access.
 *
 * \param move The move to validate
 * \param har Pointer to test_har_state (contains: state, close, id)
 * \param force_allow_projectile If true, projectiles are always valid regardless of move_string
 *
 * \return true if move can be executed in current HAR state
 */
bool ai_move_is_valid(const af_move *move, const void *har, bool force_allow_projectile);

/**
 * \brief Compute move value/score for selection purposes.
 *
 * Scores a single move based on:
 * - In highest_damage mode: damage * 10
 * - In learning mode:
 *   - Base value from move_stats
 *   - Random variance
 *   - Learning reinforcement (distance-based hits)
 *   - Damage bonus for smart AI
 *   - Attempt and consecutive use penalties
 *
 * This is the extracted move scoring logic from assign_move_by_cat()
 * and attempt_attack() combined.
 *
 * \param move The move to score
 * \param ctx Evaluation context (statistics, mode, difficulty, pilot)
 *
 * \return Integer score (higher = better)
 */
int ai_move_eval_score(const af_move *move, const move_stat_context *ctx);

/**
 * \brief Select the best move from a candidate list.
 *
 * Filters candidate moves by validity, scores each, and returns the
 * highest-scoring move. Returns NULL if no valid moves.
 *
 * This replaces the scoring loops in assign_move_by_cat() and attempt_attack().
 * Maintains deterministic behavior with same random seed.
 *
 * \param moves Array of af_move pointers (can be sparse)
 * \param count Number of moves in array
 * \param ctx Evaluation context
 *
 * \return Pointer to selected move, or NULL if no valid moves
 */
af_move *ai_move_select_best(af_move **moves, int count, const move_stat_context *ctx);

#endif // AI_MOVE_SELECTOR_H
