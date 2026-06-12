/**
 * Unit tests for AI tactic and pilot state helpers
 */

#include "game/ai/ai_state.h"
#include "CUnit/CUnit.h"
#include <stdlib.h>
#include <string.h>

void test_reset_tactic_state_clears_active_tactic(void) {
    tactic_state tactic;
    memset(&tactic, 0, sizeof(tactic));
    tactic.tactic_type = 3;
    tactic.move_type = 2;
    tactic.move_timer = 5;
    tactic.attack_type = 4;
    tactic.attack_id = 17;
    tactic.attack_timer = 2;
    tactic.attack_on = 1;
    tactic.chain_hit_on = 2;
    tactic.chain_hit_tactic = 9;

    ai a;
    memset(&a, 0, sizeof(a));
    a.tactic = &tactic;

    reset_tactic_state(&a);

    CU_ASSERT_EQUAL(tactic.last_tactic, 3);
    CU_ASSERT_EQUAL(tactic.tactic_type, 0);
    CU_ASSERT_EQUAL(tactic.move_type, 0);
    CU_ASSERT_EQUAL(tactic.move_timer, 0);
    CU_ASSERT_EQUAL(tactic.attack_type, 0);
    CU_ASSERT_EQUAL(tactic.attack_id, 0);
    CU_ASSERT_EQUAL(tactic.attack_timer, 0);
    CU_ASSERT_EQUAL(tactic.attack_on, 0);
    CU_ASSERT_EQUAL(tactic.chain_hit_on, 0);
    CU_ASSERT_EQUAL(tactic.chain_hit_tactic, 0);
}

void test_reset_tactic_state_with_no_active_tactic(void) {
    tactic_state tactic;
    memset(&tactic, 0, sizeof(tactic));
    tactic.last_tactic = 7;

    ai a;
    memset(&a, 0, sizeof(a));
    a.tactic = &tactic;

    reset_tactic_state(&a);

    CU_ASSERT_EQUAL(tactic.last_tactic, 0);
}

void test_reset_act_timer_for_difficulty_one(void) {
    srand(12345);

    ai a;
    memset(&a, 0, sizeof(a));
    a.difficulty = 1;

    reset_act_timer(&a);

    CU_ASSERT(a.act_timer <= (AI_BASE_ACT_TIMER - 2));
    CU_ASSERT(a.act_timer >= (AI_BASE_ACT_TIMER - 4));
}

void test_reset_pilot_personality_for_crystal(void) {
    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));
    pilot.pilot_id = 0;

    reset_pilot_personality(&pilot);

    CU_ASSERT_EQUAL(pilot.att_normal, 30);
    CU_ASSERT_EQUAL(pilot.att_hyper, 10);
    CU_ASSERT_EQUAL(pilot.ap_throw, 100);
    CU_ASSERT_EQUAL(pilot.pref_fwd, 30);
    CU_ASSERT_DOUBLE_EQUAL(pilot.learning, 1.5f, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(pilot.forget, 0.25f, 0.001);
}

void test_reset_pilot_personality_for_kreissack(void) {
    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));
    pilot.pilot_id = 10;

    reset_pilot_personality(&pilot);

    CU_ASSERT_EQUAL(pilot.att_hyper, 75);
    CU_ASSERT_EQUAL(pilot.ap_special, 100);
    CU_ASSERT_DOUBLE_EQUAL(pilot.learning, 3.0f, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(pilot.forget, 0.25f, 0.001);
}

void ai_state_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "reset tactic state: clears active tactic", test_reset_tactic_state_clears_active_tactic) == NULL) return;
    if(CU_add_test(suite, "reset tactic state: no active tactic", test_reset_tactic_state_with_no_active_tactic) == NULL) return;
    if(CU_add_test(suite, "reset act timer: difficulty one", test_reset_act_timer_for_difficulty_one) == NULL) return;
    if(CU_add_test(suite, "reset pilot personality: crystal", test_reset_pilot_personality_for_crystal) == NULL) return;
    if(CU_add_test(suite, "reset pilot personality: kreissack", test_reset_pilot_personality_for_kreissack) == NULL) return;
}
