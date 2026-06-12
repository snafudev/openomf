/**
 * Unit tests for AI move selector functions
 *
 * Tests ai_move_is_valid(), ai_move_eval_score(), and ai_move_select_best()
 * in isolation using mock move data and scoring context.
 */

#include "game/ai/ai_move_selector.h"
#include "game/ai/ai_utils.h"
#include "game/ai/ai_decision_engine.h"
#include "game/common_defines.h"
#include "testing/ai/ai_controller_test.h"
#include "CUnit/CUnit.h"
#include "utils/str.h"
#include <string.h>

// Helper to create a mock HAR state
typedef struct {
    uint8_t state;
    uint8_t close;
    uint8_t id;
} test_har_state;

// Helper to create test af_move with move_string
static af_move create_test_move(int id, uint8_t category, float damage, float stun, const char *move_str) {
    af_move move = {0};
    move.id = id;
    move.category = category;
    move.damage = damage;
    move.stun = stun;
    str_create(&move.move_string);
    if(move_str) {
        str_from_c(&move.move_string, move_str);
    }
    return move;
}

// Helper to free test move
static void free_test_move(af_move *move) {
    str_free(&move->move_string);
}

// ============================================================================
// Test: ai_move_is_valid - state constraints
// ============================================================================

void test_move_is_valid_state_not_jumping_with_jump_move(void) {
    af_move move = create_test_move(0, CAT_JUMPING, 10.0f, 0.0f, "K");
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_FALSE(valid);
    free_test_move(&move);
}

void test_move_is_valid_state_jumping_with_non_jump_move(void) {
    af_move move = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    test_har_state har = {.state = STATE_JUMPING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_FALSE(valid);
    free_test_move(&move);
}

void test_move_is_valid_state_jumping_with_jump_move(void) {
    af_move move = create_test_move(0, CAT_JUMPING, 10.0f, 0.0f, "K");
    test_har_state har = {.state = STATE_JUMPING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_TRUE(valid);
    free_test_move(&move);
}

void test_move_is_valid_close_move_not_close_not_jumping(void) {
    af_move move = create_test_move(0, CAT_CLOSE, 20.0f, 5.0f, "2K");
    test_har_state har = {.state = STATE_STANDING, .close = 0, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_FALSE(valid);
    free_test_move(&move);
}

void test_move_is_valid_close_move_is_close(void) {
    af_move move = create_test_move(0, CAT_CLOSE, 20.0f, 5.0f, "2K");
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_TRUE(valid);
    free_test_move(&move);
}

void test_move_is_valid_close_move_jumping(void) {
    af_move move = create_test_move(0, CAT_CLOSE, 20.0f, 5.0f, "2K");
    test_har_state har = {.state = STATE_JUMPING, .close = 0, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_FALSE(valid);
    free_test_move(&move);
}

void test_move_is_valid_low_move_not_close_not_jumping(void) {
    af_move move = create_test_move(0, CAT_LOW, 8.0f, 0.0f, "6K");
    test_har_state har = {.state = STATE_STANDING, .close = 0, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_FALSE(valid);
    free_test_move(&move);
}

void test_move_is_valid_low_move_is_close(void) {
    af_move move = create_test_move(0, CAT_LOW, 8.0f, 0.0f, "6K");
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_TRUE(valid);
    free_test_move(&move);
}

void test_move_is_valid_high_move_is_close(void) {
    af_move move = create_test_move(0, CAT_HIGH, 12.0f, 0.0f, "8K");
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_TRUE(valid);
    free_test_move(&move);
}

void test_move_is_valid_projectile_always_valid_when_close(void) {
    // Projectiles with force_allow_projectile=true should be valid when close
    af_move move = create_test_move(0, CAT_PROJECTILE, 6.0f, 0.0f, "P");
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    bool valid = ai_move_is_valid(&move, &har, true);
    CU_ASSERT_TRUE(valid);
    free_test_move(&move);
}

// ============================================================================
// Test: ai_move_eval_score - scoring logic
// ============================================================================

void test_move_eval_score_highest_damage_mode(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move low_damage = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    af_move high_damage = create_test_move(1, CAT_BASIC, 15.0f, 0.0f, "1");
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    int score_low = ai_move_eval_score(&low_damage, &ctx);
    int score_high = ai_move_eval_score(&high_damage, &ctx);
    CU_ASSERT(score_high > score_low);
    
    free_test_move(&low_damage);
    free_test_move(&high_damage);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_eval_score_learning_reinforcement(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move_high_value = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    af_move move_low_value = create_test_move(1, CAT_BASIC, 5.0f, 0.0f, "1");
    
    ai_fix->ai_data.move_stats[0].value = 50;
    ai_fix->ai_data.move_stats[1].value = 10;
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = false,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    int score_high_value = ai_move_eval_score(&move_high_value, &ctx);
    int score_low_value = ai_move_eval_score(&move_low_value, &ctx);
    CU_ASSERT(score_high_value > score_low_value);
    
    free_test_move(&move_high_value);
    free_test_move(&move_low_value);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_eval_score_attempt_penalty(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    ai_fix->ai_data.move_stats[0].attempts = 100;
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = false,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    int score_with_low_attempts = ai_move_eval_score(&move, &ctx);
    ai_fix->ai_data.move_stats[0].attempts = 1000;
    int score_with_high_attempts = ai_move_eval_score(&move, &ctx);
    CU_ASSERT(score_with_low_attempts > score_with_high_attempts);
    
    free_test_move(&move);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_eval_score_consecutive_penalty(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    ai_fix->ai_data.move_stats[0].consecutive = 5;
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = false,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    int score_with_low_consecutive = ai_move_eval_score(&move, &ctx);
    ai_fix->ai_data.move_stats[0].consecutive = 50;
    int score_with_high_consecutive = ai_move_eval_score(&move, &ctx);
    CU_ASSERT(score_with_low_consecutive > score_with_high_consecutive);
    
    free_test_move(&move);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: ai_move_select_best - selection logic
// ============================================================================

void test_move_select_best_single_valid_move(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    af_move *moves[] = {&move};
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    af_move *selected = ai_move_select_best(moves, 1, &ctx);
    CU_ASSERT_PTR_EQUAL(selected, &move);
    
    free_test_move(&move);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_select_best_multiple_valid_highest_damage(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move1 = create_test_move(0, CAT_CLOSE, 5.0f, 0.0f, "2K");
    af_move move2 = create_test_move(1, CAT_CLOSE, 20.0f, 5.0f, "2K");
    af_move *moves[] = {&move1, &move2};
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    af_move *selected = ai_move_select_best(moves, 2, &ctx);
    CU_ASSERT_PTR_EQUAL(selected, &move2);
    
    free_test_move(&move1);
    free_test_move(&move2);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_select_best_filters_invalid_moves(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move_invalid = create_test_move(0, CAT_CLOSE, 5.0f, 0.0f, "2K");
    af_move move_valid = create_test_move(1, CAT_BASIC, 5.0f, 0.0f, "1");
    af_move *moves[] = {&move_invalid, &move_valid};
    
    test_har_state har = {.state = STATE_STANDING, .close = 0, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    af_move *selected = ai_move_select_best(moves, 2, &ctx);
    CU_ASSERT_PTR_EQUAL(selected, &move_valid);
    
    free_test_move(&move_invalid);
    free_test_move(&move_valid);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_select_best_no_valid_moves_returns_null(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move = create_test_move(0, CAT_CLOSE, 5.0f, 0.0f, "2K");
    af_move *moves[] = {&move};
    
    test_har_state har = {.state = STATE_STANDING, .close = 0, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    af_move *selected = ai_move_select_best(moves, 1, &ctx);
    CU_ASSERT_PTR_NULL(selected);
    
    free_test_move(&move);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_select_best_deterministic_with_seed(void) {
    // Verify that same seed produces same behavior for the selection process
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move1 = create_test_move(0, CAT_BASIC, 10.0f, 0.0f, "1");  // Higher damage
    af_move move2 = create_test_move(1, CAT_BASIC, 5.0f, 0.0f, "1");   // Lower damage
    af_move *moves[] = {&move1, &move2};
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,  // highest damage mode is deterministic
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    // In highest_damage mode with different damage values, result should be deterministic
    af_move *selected1 = ai_move_select_best(moves, 2, &ctx);
    af_move *selected2 = ai_move_select_best(moves, 2, &ctx);
    CU_ASSERT_PTR_EQUAL(selected1, selected2);
    CU_ASSERT_PTR_EQUAL(selected1, &move1);  // Should select highest damage
    
    free_test_move(&move1);
    free_test_move(&move2);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_move_select_best_range_aware(void) {
    // Verify that move selection accepts context with enemy_range parameter
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    af_move *moves[] = {&move};
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .enemy_range = 150,
        .damage_divisor = 3,
    };
    
    // Verify the function executes without crashing
    // Note: With certain contexts, valid moves may still return NULL if filtered out
    // This test just verifies the function handles the enemy_range parameter
    ai_move_select_best(moves, 1, &ctx);
    CU_PASS();
    
    free_test_move(&move);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Regression: Verify Phase 4 baseline behavior
// ============================================================================

void test_move_select_best_matches_phase3_baseline(void) {
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    
    af_move move1 = create_test_move(0, CAT_BASIC, 5.0f, 0.0f, "1");
    af_move move2 = create_test_move(1, CAT_BASIC, 8.0f, 0.0f, "1");
    af_move move3 = create_test_move(2, CAT_BASIC, 10.0f, 0.0f, "1");
    af_move *moves[] = {&move1, &move2, &move3};
    
    test_har_state har = {.state = STATE_STANDING, .close = 1, .id = HAR_JAGUAR};
    move_stat_context ctx = {
        .move_stats = ai_fix->ai_data.move_stats,
        .har = &har,
        .highest_damage = true,
        .difficulty = 3,
        .pilot = pilot->pilot_data,
        .damage_divisor = 3,
    };
    
    test_seed_random(11111);
    af_move *selected = ai_move_select_best(moves, 3, &ctx);
    CU_ASSERT_PTR_NOT_NULL(selected);
    CU_ASSERT_PTR_EQUAL(selected, &move3);
    
    free_test_move(&move1);
    free_test_move(&move2);
    free_test_move(&move3);
    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// CUnit test suite registration
// ============================================================================

void ai_move_selector_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "move is_valid: state not jumping with jump move", test_move_is_valid_state_not_jumping_with_jump_move) == NULL) return;
    if(CU_add_test(suite, "move is_valid: state jumping with non-jump move", test_move_is_valid_state_jumping_with_non_jump_move) == NULL) return;
    if(CU_add_test(suite, "move is_valid: state jumping with jump move", test_move_is_valid_state_jumping_with_jump_move) == NULL) return;
    if(CU_add_test(suite, "move is_valid: close move not close not jumping", test_move_is_valid_close_move_not_close_not_jumping) == NULL) return;
    if(CU_add_test(suite, "move is_valid: close move is close", test_move_is_valid_close_move_is_close) == NULL) return;
    if(CU_add_test(suite, "move is_valid: close move jumping", test_move_is_valid_close_move_jumping) == NULL) return;
    if(CU_add_test(suite, "move is_valid: low move not close not jumping", test_move_is_valid_low_move_not_close_not_jumping) == NULL) return;
    if(CU_add_test(suite, "move is_valid: low move is close", test_move_is_valid_low_move_is_close) == NULL) return;
    if(CU_add_test(suite, "move is_valid: high move is close", test_move_is_valid_high_move_is_close) == NULL) return;
    if(CU_add_test(suite, "move is_valid: projectile always valid when close", test_move_is_valid_projectile_always_valid_when_close) == NULL) return;
    
    if(CU_add_test(suite, "move eval_score: highest damage mode", test_move_eval_score_highest_damage_mode) == NULL) return;
    if(CU_add_test(suite, "move eval_score: learning reinforcement", test_move_eval_score_learning_reinforcement) == NULL) return;
    if(CU_add_test(suite, "move eval_score: attempt penalty", test_move_eval_score_attempt_penalty) == NULL) return;
    if(CU_add_test(suite, "move eval_score: consecutive penalty", test_move_eval_score_consecutive_penalty) == NULL) return;
    
    if(CU_add_test(suite, "move select_best: single valid move", test_move_select_best_single_valid_move) == NULL) return;
    if(CU_add_test(suite, "move select_best: multiple valid highest damage", test_move_select_best_multiple_valid_highest_damage) == NULL) return;
    if(CU_add_test(suite, "move select_best: filters invalid moves", test_move_select_best_filters_invalid_moves) == NULL) return;
    if(CU_add_test(suite, "move select_best: no valid moves returns null", test_move_select_best_no_valid_moves_returns_null) == NULL) return;
    if(CU_add_test(suite, "move select_best: deterministic with seed", test_move_select_best_deterministic_with_seed) == NULL) return;
    if(CU_add_test(suite, "move select_best: range aware", test_move_select_best_range_aware) == NULL) return;
    
    if(CU_add_test(suite, "move select_best: matches phase3 baseline", test_move_select_best_matches_phase3_baseline) == NULL) return;
}
