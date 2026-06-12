/**
 * AI Move Selector Module Implementation
 *
 * Extracted move validation and selection logic from ai_controller.c.
 * Provides testable, pure functions for move evaluation.
 */

#include "game/ai/ai_move_selector.h"
#include "game/ai/ai_utils.h"
#include "game/ai/ai_decision_engine.h"
#include "game/common_defines.h"
#include "utils/random.h"
#include <stddef.h>
#include <string.h>

// ============================================================================
// Internal Helper: Extract HAR state from context
// ============================================================================

/**
 * \brief Extract state and close flags from HAR context.
 *
 * The context.har is a generic pointer that could be:
 * - A real har* from the game (during ai_controller usage)
 * - A test_har_state for testing
 *
 * We extract what we need without making assumptions about the structure.
 * For now, we cast to test_har_state since that's our test pattern.
 */
typedef struct {
    uint8_t state;
    uint8_t close;
    uint8_t id;
} har_state_view;

static har_state_view extract_har_state(const void *har) {
    if(har == NULL) {
        return (har_state_view){0, 0, 0};
    }
    return *(const har_state_view *)har;
}

// ============================================================================
// Function: ai_move_is_valid
// ============================================================================

bool ai_move_is_valid(const af_move *move, const void *har, bool force_allow_projectile) {
    if(move == NULL || har == NULL) {
        return false;
    }

    har_state_view h = extract_har_state(har);

    // If category is any of these, and bot is not close, then
    // do not try to execute any of them. This attempts
    // to make the HARs close up instead of standing in place
    // wawing their hands towards each other. Not a perfect solution.
    switch(move->category) {
        case CAT_CLOSE:
        case CAT_LOW:
        case CAT_MEDIUM:
        case CAT_HIGH:
            // Only allow handwaving if close or jumping
            if(!h.close && h.state != STATE_JUMPING) {
                return false;
            }
    }

    if(move->category == CAT_JUMPING && h.state != STATE_JUMPING) {
        // not jumping but trying to execute a jumping move
        return false;
    }

    if(move->category != CAT_JUMPING && h.state == STATE_JUMPING) {
        // jumping but this move is not a jumping move
        return false;
    }

    if(move->category == CAT_SCRAP && h.state != STATE_VICTORY) {
        return false;
    }

    if(move->category == CAT_DESTRUCTION && h.state != STATE_SCRAP) {
        return false;
    }

    if(move->category == CAT_FIRE_ICE) {
        return false;
    }

    // Validate move_string characters
    int move_str_len = str_size(&move->move_string);
    for(int i = 0; i < move_str_len; i++) {
        char tmp = str_at(&move->move_string, i);
        if(!((tmp >= '1' && tmp <= '9') || tmp == 'K' || tmp == 'P')) {
            if(force_allow_projectile && move->category == CAT_PROJECTILE) {
                return true;  // projectile is always true
            }
            return false;
        }
    }

    // Valid if has damage or is projectile/special
    if((move->damage > 0 || move->category == CAT_PROJECTILE || move->category == CAT_SCRAP ||
        move->category == CAT_DESTRUCTION) && move_str_len > 0) {
        return true;
    }

    return false;
}

// ============================================================================
// Internal Helper: Move preference checks
// ============================================================================

/**
 * \brief Create a temporary AI struct for decision function calls.
 */
static inline ai make_temp_ai(const move_stat_context *ctx) {
    ai result = {0};
    result.difficulty = ctx->difficulty;
    result.pilot = (sd_pilot *)&ctx->pilot;
    return result;
}

static bool dislikes_move_in_context(const af_move *move, const move_stat_context *ctx) {
    const sd_pilot *pilot = &ctx->pilot;

    // check for non-projectile special moves
    if(is_special_move(move)) {
        // pilots with bad special ability dislike special moves
        return !roll_pref(pilot->ap_special);
    }

    switch(move->category) {
        case CAT_BASIC:
            // smart AI dislike basic moves
            {
                ai temp_ai = make_temp_ai(ctx);
                return !roll_pref(pilot->att_normal) && smart_usually(&temp_ai);
            }
        case CAT_LOW:
            // pilots with bad low ability dislike low moves
            return !roll_pref(pilot->att_normal) && !roll_pref(pilot->ap_low);
        case CAT_MEDIUM:
            // pilots with bad middle ability dislike middle moves
            return !roll_pref(pilot->att_normal) && !roll_pref(pilot->ap_middle);
        case CAT_HIGH:
            // pilots with bad high ability dislike high moves
            return !roll_pref(pilot->att_normal) && !roll_pref(pilot->ap_high);
        case CAT_CLOSE:
            // non-hyper pilots with bad throw ability dislike throw moves
            return !roll_pref(pilot->att_hyper) && !roll_pref(pilot->ap_throw);
        case CAT_JUMPING:
            // non-jumper pilots with bad jump ability dislike jump moves
            return !roll_pref(pilot->att_jump) && !roll_pref(pilot->ap_jump);
        case CAT_PROJECTILE:
            // non-sniper pilots with bad special ability dislike projectile moves
            return !roll_pref(pilot->att_sniper) && !roll_pref(pilot->ap_special);
    }

    return false;
}

static bool move_too_powerful_for_context(const af_move *move, const move_stat_context *ctx) {
    ai temp_ai = make_temp_ai(ctx);
    return is_special_move(move) && dumb_usually(&temp_ai);
}

// ============================================================================
// Function: ai_move_eval_score
// ============================================================================

int ai_move_eval_score(const af_move *move, const move_stat_context *ctx) {
    if(move == NULL || ctx == NULL) {
        return 0;
    }

    move_stat *ms = &ctx->move_stats[move->id];

    if(ctx->highest_damage) {
        // evaluate the move based purely on damage
        return (int)move->damage * 10;
    } else {
        // evaluate the move based on learning reinforcement
        int value = ms->value + rand_int(10);

        ai temp_ai = make_temp_ai(ctx);

        if(learning_moment(&temp_ai) && ms->min_hit_dist != -1) {
            if(ms->last_dist < ms->max_hit_dist + 5 && ms->last_dist > ms->min_hit_dist + 5) {
                value += 2;
            } else if(ms->last_dist > ms->max_hit_dist + 10) {
                value -= 3;
            }
        }

        // AI is less likely to use exact same move as last attack
        if(ctx->last_move_id > 0 && ctx->last_move_id == move->id) {
            value -= rand_int(10);
        }

        // smart AI will slightly favor high damage moves
        if(smart_usually(&temp_ai)) {
            int divisor = ctx->damage_divisor > 0 ? ctx->damage_divisor : 4;
            value += ((int)move->damage / divisor);
        }

        // AI is less likely to use disliked moves
        if(dislikes_move_in_context(move, ctx)) {
            value -= rand_int(10);
        }

        value -= ms->attempts / 2;
        value -= ms->consecutive * 2;

        // sometimes skip move if it is too powerful for difficulty
        if(move_too_powerful_for_context(move, ctx)) {
            return -999;  // effectively disable this move by giving it very low score
        }

        return value;
    }
}

// ============================================================================
// Function: ai_move_select_best
// ============================================================================

af_move *ai_move_select_best(af_move **moves, int count, const move_stat_context *ctx) {
    if(moves == NULL || count <= 0 || ctx == NULL) {
        return NULL;
    }

    af_move *selected_move = NULL;
    int top_value = 0;
    bool first_valid = true;

    // Create temporary ai for range checks
    ai temp_ai = make_temp_ai(ctx);

    // Iterate through candidate moves
    for(int i = 0; i < count; i++) {
        af_move *move = moves[i];
        if(move == NULL) {
            continue;
        }

        // Check if move is valid
        if(!ai_move_is_valid(move, ctx->har, ctx->force_allow_projectile)) {
            continue;
        }

        // Additional range constraint for attempt_attack context
        // (smart AI will bail out unless close enough to hit)
        if(ctx->enemy_range > 0) {
            bool in_attempt_range = (ctx->enemy_range <= RANGE_CLOSE || 
                                    (ctx->enemy_range == RANGE_MID && dumb_sometimes(&temp_ai)));
            
            if(!in_attempt_range && (move->category == CAT_BASIC || move->category == CAT_LOW ||
                                     move->category == CAT_MEDIUM || move->category == CAT_HIGH)) {
                continue;
            }
        }

        // Score this move
        int value = ai_move_eval_score(move, ctx);

        // Skip if move is too powerful (returned very low score)
        if(value < -900) {
            continue;
        }

        // Select as best move
        if(first_valid) {
            selected_move = move;
            top_value = value;
            first_valid = false;
        } else if(value > top_value) {
            selected_move = move;
            top_value = value;
        }
    }

    return selected_move;
}
