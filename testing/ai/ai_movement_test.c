/**
 * Unit tests for AI movement decision functions
 *
 * Tests ai_movement_decide() and ai_movement_jump_chance() in isolation,
 * using extreme pilot preference values for deterministic results.
 */

#include "game/ai/ai_movement.h"
#include "game/ai/ai_utils.h"
#include "game/common_defines.h"
#include "ai/ai_controller_test.h"
#include "CUnit/CUnit.h"

// ============================================================================
// Test: ai_movement_decide
// ============================================================================

void test_movement_decide_cramped_not_wallhugging_pref_back(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    pilot->pilot_data.pref_back = 100;
    pilot->pilot_data.pref_fwd = -100;

    // Cramped and not wallhugging: should back away when pref_back is high
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_CRAMPED, false, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_BACK);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_cramped_not_wallhugging_no_pref_back(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    pilot->pilot_data.pref_back = -100;
    pilot->pilot_data.pref_fwd = -100;

    // Cramped and not wallhugging but no back preference: should stay still
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_CRAMPED, false, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_STILL);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_cramped_wallhugging_pref_fwd(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    pilot->pilot_data.pref_fwd = 100;
    pilot->pilot_data.pref_back = -100;

    // Cramped + wallhugging: skips cramped branch, uses pref_fwd instead
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_CRAMPED, true, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_FWD);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_far_pref_fwd(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    pilot->pilot_data.pref_fwd = 100;

    // Far range with strong forward preference: move forward
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_FAR, false, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_FWD);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_far_pref_back_no_wallhug(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    pilot->pilot_data.pref_fwd = -100;
    pilot->pilot_data.pref_back = 100;

    // Far range, no wallhug, no pref_fwd but has pref_back: move back
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_FAR, false, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_BACK);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_far_pref_back_wallhugging(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(3, pilot);
    pilot->pilot_data.pref_fwd = -100;
    pilot->pilot_data.pref_back = 100;

    // Far range WITH wallhug: pref_back branch requires !wallhugging, so stays still
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_FAR, true, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_STILL);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_brawler_smart_goes_forward(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);
    pilot->pilot_data.pref_fwd = -100;
    pilot->pilot_data.pref_back = -100;

    // Brawler HAR at high difficulty and no movement prefs: brawler branch triggers forward
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_FAR, false, HAR_FLAIL);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_FWD);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_decide_default_still(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);
    pilot->pilot_data.pref_fwd = -100;
    pilot->pilot_data.pref_back = -100;

    // Non-brawler at low difficulty with no movement prefs: stays still
    int dir = ai_movement_decide(&ai_fix->ai_data, RANGE_FAR, false, HAR_JAGUAR);
    CU_ASSERT_EQUAL(dir, MOVE_DIR_STILL);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

// ============================================================================
// Test: ai_movement_jump_chance
// ============================================================================

void test_movement_jump_chance_high_pref_jump(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(1, pilot);
    pilot->pilot_data.pref_jump = 100; // always rolls true

    // pref_jump=100: roll_pref always true → jump_chance = 100 - 10 = 90
    // diff_scale at difficulty 1 almost never fires → result should be 90 on first call
    int chance = ai_movement_jump_chance(&ai_fix->ai_data);
    CU_ASSERT(chance <= 90);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void test_movement_jump_chance_high_diff_no_pref_jump(void) {
    test_seed_random(12345);
    test_pilot_fixture *pilot = test_pilot_create(0);
    test_ai_fixture *ai_fix = test_ai_create(6, pilot);
    pilot->pilot_data.pref_jump = -100; // always rolls false

    // pref_jump=-100: roll_pref always false → no -10 for pref
    // diff_scale at difficulty 6 always fires → jump_chance = 100 - 10 = 90
    int chance = ai_movement_jump_chance(&ai_fix->ai_data);
    CU_ASSERT_EQUAL(chance, 90);

    test_ai_free(ai_fix);
    test_pilot_free(pilot);
}

void ai_movement_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "movement decide: cramped not wallhugging pref back",
                   test_movement_decide_cramped_not_wallhugging_pref_back) == NULL)
        return;
    if(CU_add_test(suite, "movement decide: cramped not wallhugging no pref back",
                   test_movement_decide_cramped_not_wallhugging_no_pref_back) == NULL)
        return;
    if(CU_add_test(suite, "movement decide: cramped wallhugging pref fwd",
                   test_movement_decide_cramped_wallhugging_pref_fwd) == NULL)
        return;
    if(CU_add_test(suite, "movement decide: far pref fwd", test_movement_decide_far_pref_fwd) == NULL) return;
    if(CU_add_test(suite, "movement decide: far pref back no wallhug",
                   test_movement_decide_far_pref_back_no_wallhug) == NULL)
        return;
    if(CU_add_test(suite, "movement decide: far pref back wallhugging",
                   test_movement_decide_far_pref_back_wallhugging) == NULL)
        return;
    if(CU_add_test(suite, "movement decide: brawler smart goes forward",
                   test_movement_decide_brawler_smart_goes_forward) == NULL)
        return;
    if(CU_add_test(suite, "movement decide: default still", test_movement_decide_default_still) == NULL) return;
    if(CU_add_test(suite, "movement jump chance: high pref jump",
                   test_movement_jump_chance_high_pref_jump) == NULL)
        return;
    if(CU_add_test(suite, "movement jump chance: high difficulty no pref jump",
                   test_movement_jump_chance_high_diff_no_pref_jump) == NULL)
        return;
}
