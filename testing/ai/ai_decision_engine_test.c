/**
 * Unit tests for AI decision engine
 *
 * Tests pure decision functions: roll_chance, roll_pref, smart_usually,
 * dumb_usually, smart_sometimes, dumb_sometimes, diff_scale, learning_moment, forgetful
 */

#include "game/ai/ai_decision_engine.h"
#include "testing/ai/ai_controller_test.h"
#include "CUnit/CUnit.h"
#include <stdint.h>

// ============================================================================
// Test: roll_chance
// ============================================================================

void test_roll_chance_threshold_1_always_true(void) {
    test_seed_random(12345);
    CU_ASSERT_TRUE(roll_chance(1));
    CU_ASSERT_TRUE(roll_chance(0));
}

void test_roll_chance_high_threshold_mostly_false(void) {
    test_seed_random(12345);
    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(roll_chance(100)) count++;
    }
    // roll_chance(100) = 1/100 chance, so should be rarely true
    CU_ASSERT(count <= 10);
}

void test_roll_chance_low_threshold_mostly_false(void) {
    test_seed_random(12345);
    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(roll_chance(2)) count++;
    }
    CU_ASSERT(count >= 30 && count <= 70);
}

// ============================================================================
// Test: roll_pref
// ============================================================================

void test_roll_pref_neutral_preference(void) {
    test_seed_random(12345);
    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(roll_pref(0)) count++;
    }
    CU_ASSERT(count >= 30 && count <= 70);
}

void test_roll_pref_high_preference(void) {
    test_seed_random(12345);
    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(roll_pref(100)) count++;
    }
    CU_ASSERT(count >= 80);
}

void test_roll_pref_low_preference(void) {
    test_seed_random(12345);
    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(roll_pref(-100)) count++;
    }
    CU_ASSERT(count <= 20);
}

// ============================================================================
// Test: smart_usually
// ============================================================================

void test_smart_usually_difficulty_1_always_false(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    bool result = smart_usually(&ai_fix->ai_data);
    CU_ASSERT_FALSE(result);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_smart_usually_difficulty_6_mostly_true(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(smart_usually(&ai_fix->ai_data)) {
            count++;
        }
    }
    CU_ASSERT(count >= 85);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_smart_usually_difficulty_3_moderate(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(smart_usually(&ai_fix->ai_data)) {
            count++;
        }
    }
    // difficulty 3 uses roll_chance(4) = 25% chance; 3-sigma range [12, 38]
    CU_ASSERT(count >= 12 && count <= 38);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: dumb_usually
// ============================================================================

void test_dumb_usually_difficulty_1_mostly_true(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(dumb_usually(&ai_fix->ai_data)) {
            count++;
        }
    }
    CU_ASSERT(count >= 85);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_dumb_usually_difficulty_6_always_false(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    bool result = dumb_usually(&ai_fix->ai_data);
    CU_ASSERT_FALSE(result);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: smart_sometimes
// ============================================================================

void test_smart_sometimes_difficulty_1_always_false(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    bool result = smart_sometimes(&ai_fix->ai_data);
    CU_ASSERT_FALSE(result);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_smart_sometimes_difficulty_6_sometimes_true(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(smart_sometimes(&ai_fix->ai_data)) {
            count++;
        }
    }
    // difficulty 6 uses roll_chance(4) = 25% chance; 3-sigma range [12, 38]
    CU_ASSERT(count >= 12 && count <= 38);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: dumb_sometimes
// ============================================================================

void test_dumb_sometimes_difficulty_1_sometimes_true(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(dumb_sometimes(&ai_fix->ai_data)) {
            count++;
        }
    }
    // difficulty 1 uses roll_chance(3) = 33% chance; 3-sigma range [19, 47]
    CU_ASSERT(count >= 19 && count <= 47);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_dumb_sometimes_difficulty_6_always_false(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    bool result = dumb_sometimes(&ai_fix->ai_data);
    CU_ASSERT_FALSE(result);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: diff_scale
// ============================================================================

void test_diff_scale_difficulty_1_low_chance(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    int count = 0;
    for(int i = 0; i < 1000; i++) {
        if(diff_scale(&ai_fix->ai_data)) {
            count++;
        }
    }
    // 1/36 chance = ~2.8%, allow 0-50 out of 1000
    CU_ASSERT(count <= 50);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_diff_scale_difficulty_6_high_chance(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(diff_scale(&ai_fix->ai_data)) {
            count++;
        }
    }
    // 36/36 = 100% chance
    CU_ASSERT_EQUAL(count, 100);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: learning_moment
// ============================================================================

void test_learning_moment_with_high_learning_rate(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_pilot_set_learning(pilot, 10.0f);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(learning_moment(&ai_fix->ai_data)) {
            count++;
        }
    }
    CU_ASSERT(count >= 50);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_learning_moment_with_low_learning_rate(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_pilot_set_learning(pilot, 0.1f);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(learning_moment(&ai_fix->ai_data)) {
            count++;
        }
    }
    CU_ASSERT(count <= 20);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: forgetful
// ============================================================================

void test_forgetful_with_high_forget_rate(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_pilot_set_forget(pilot, 10.0f);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(forgetful(&ai_fix->ai_data)) {
            count++;
        }
    }
    CU_ASSERT(count >= 50);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_forgetful_with_low_forget_rate(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_pilot_set_forget(pilot, 0.1f);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);

    int count = 0;
    for(int i = 0; i < 100; i++) {
        if(forgetful(&ai_fix->ai_data)) {
            count++;
        }
    }
    CU_ASSERT(count <= 20);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// CUnit test suite registration
// ============================================================================

void ai_decision_engine_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "roll_chance: threshold 1 always true", test_roll_chance_threshold_1_always_true) == NULL) return;
    if(CU_add_test(suite, "roll_chance: high threshold mostly false", test_roll_chance_high_threshold_mostly_false) == NULL) return;
    if(CU_add_test(suite, "roll_chance: low threshold mostly false", test_roll_chance_low_threshold_mostly_false) == NULL) return;
    if(CU_add_test(suite, "roll_pref: neutral preference", test_roll_pref_neutral_preference) == NULL) return;
    if(CU_add_test(suite, "roll_pref: high preference", test_roll_pref_high_preference) == NULL) return;
    if(CU_add_test(suite, "roll_pref: low preference", test_roll_pref_low_preference) == NULL) return;
    if(CU_add_test(suite, "smart_usually: difficulty 1 always false", test_smart_usually_difficulty_1_always_false) == NULL) return;
    if(CU_add_test(suite, "smart_usually: difficulty 6 mostly true", test_smart_usually_difficulty_6_mostly_true) == NULL) return;
    if(CU_add_test(suite, "smart_usually: difficulty 3 moderate", test_smart_usually_difficulty_3_moderate) == NULL) return;
    if(CU_add_test(suite, "dumb_usually: difficulty 1 mostly true", test_dumb_usually_difficulty_1_mostly_true) == NULL) return;
    if(CU_add_test(suite, "dumb_usually: difficulty 6 always false", test_dumb_usually_difficulty_6_always_false) == NULL) return;
    if(CU_add_test(suite, "smart_sometimes: difficulty 1 always false", test_smart_sometimes_difficulty_1_always_false) == NULL) return;
    if(CU_add_test(suite, "smart_sometimes: difficulty 6 sometimes true", test_smart_sometimes_difficulty_6_sometimes_true) == NULL) return;
    if(CU_add_test(suite, "dumb_sometimes: difficulty 1 sometimes true", test_dumb_sometimes_difficulty_1_sometimes_true) == NULL) return;
    if(CU_add_test(suite, "dumb_sometimes: difficulty 6 always false", test_dumb_sometimes_difficulty_6_always_false) == NULL) return;
    if(CU_add_test(suite, "diff_scale: difficulty 1 low chance", test_diff_scale_difficulty_1_low_chance) == NULL) return;
    if(CU_add_test(suite, "diff_scale: difficulty 6 high chance", test_diff_scale_difficulty_6_high_chance) == NULL) return;
    if(CU_add_test(suite, "learning_moment: with high learning rate", test_learning_moment_with_high_learning_rate) == NULL) return;
    if(CU_add_test(suite, "learning_moment: with low learning rate", test_learning_moment_with_low_learning_rate) == NULL) return;
    if(CU_add_test(suite, "forgetful: with high forget rate", test_forgetful_with_high_forget_rate) == NULL) return;
    if(CU_add_test(suite, "forgetful: with low forget rate", test_forgetful_with_low_forget_rate) == NULL) return;
}
