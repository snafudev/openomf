/**
 * Unit tests for AI learning module
 *
 * Tests: ai_learning_adjust_from_throw, ai_learning_adjust_from_projectile,
 *        ai_learning_maybe_forget
 */

#include "game/ai/ai_learning.h"
#include "game/ai/ai_state.h"
#include "CUnit/CUnit.h"
#include <string.h>

/* Helpers --------------------------------------------------------------- */

static void make_ai_with_difficulty(ai *a, sd_pilot *p, tactic_state *t, int difficulty) {
    memset(a, 0, sizeof(ai));
    memset(p, 0, sizeof(sd_pilot));
    memset(t, 0, sizeof(tactic_state));
    p->pilot_id = 0;
    p->learning = 8.0f;   // rand_int(8) gives 0..7, all <= 8 → learning_moment always true
    p->forget = 0.0f;     // never forgets by default
    a->pilot = p;
    a->difficulty = difficulty;
    a->tactic = t;
}

/* -----------------------------------------------------------------------
 * ai_learning_adjust_from_throw
 * -------------------------------------------------------------------- */

void test_learning_throw_increments_counter(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);

    CU_ASSERT_EQUAL(a.thrown, 0);
    ai_learning_adjust_from_throw(&a);
    CU_ASSERT_EQUAL(a.thrown, 1);
    ai_learning_adjust_from_throw(&a);
    CU_ASSERT_EQUAL(a.thrown, 2);
}

void test_learning_throw_no_adjustment_below_threshold(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    p.att_def = 95;
    p.att_sniper = 20;

    // Call once — below MAX_TIMES_THROWN (3)
    ai_learning_adjust_from_throw(&a);
    CU_ASSERT_EQUAL(a.thrown, 1);
    // att_def should not have been reduced yet
    CU_ASSERT_EQUAL(p.att_def, 95);
}

void test_learning_throw_adjusts_at_threshold(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    p.att_def = 95;
    p.att_sniper = 20;
    p.att_jump = 20;
    p.pref_jump = 20;
    p.pref_back = 20;
    p.pref_fwd = 95;

    // Reach threshold
    a.thrown = MAX_TIMES_THROWN - 1;
    ai_learning_adjust_from_throw(&a);

    CU_ASSERT_EQUAL(a.thrown, MAX_TIMES_THROWN);
    // att_def capped down
    CU_ASSERT_EQUAL(p.att_def, 10);
    // sniper, jump increased
    CU_ASSERT(p.att_sniper > 20);
    CU_ASSERT(p.att_jump > 20);
    // movement preferences adjusted
    CU_ASSERT(p.pref_jump > 20);
    CU_ASSERT(p.pref_back > 20);
    CU_ASSERT(p.pref_fwd < 95);
}

void test_learning_throw_defence_not_capped_when_low(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    p.att_def = 50; // below 90 threshold — should NOT be modified

    a.thrown = MAX_TIMES_THROWN - 1;
    ai_learning_adjust_from_throw(&a);

    CU_ASSERT_EQUAL(p.att_def, 50);
}

void test_learning_throw_no_adjustment_low_difficulty(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 1); // low difficulty — learning_moment usually false
    p.learning = 0.0f; // guarantees learning_moment returns false
    p.att_def = 95;

    a.thrown = MAX_TIMES_THROWN - 1;
    ai_learning_adjust_from_throw(&a);

    // att_def unchanged because learning_moment is false
    CU_ASSERT_EQUAL(p.att_def, 95);
}

/* -----------------------------------------------------------------------
 * ai_learning_adjust_from_projectile
 * -------------------------------------------------------------------- */

void test_learning_projectile_increments_counter(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);

    CU_ASSERT_EQUAL(a.shot, 0);
    ai_learning_adjust_from_projectile(&a);
    CU_ASSERT_EQUAL(a.shot, 1);
}

void test_learning_projectile_no_adjustment_below_threshold(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    p.att_def = 95;

    // One call — below MAX_TIMES_SHOT (4)
    ai_learning_adjust_from_projectile(&a);
    CU_ASSERT_EQUAL(a.shot, 1);
    CU_ASSERT_EQUAL(p.att_def, 95);
}

void test_learning_projectile_adjusts_at_threshold(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    p.att_def = 95;
    p.att_sniper = 5;
    p.att_hyper = 5;
    p.att_jump = 5;
    p.pref_jump = 20;
    p.pref_fwd = 20;
    p.pref_back = 95;

    a.shot = MAX_TIMES_SHOT - 1;
    ai_learning_adjust_from_projectile(&a);

    CU_ASSERT_EQUAL(a.shot, MAX_TIMES_SHOT);
    CU_ASSERT_EQUAL(p.att_def, 10);
    CU_ASSERT(p.att_jump > 5);
    CU_ASSERT(p.pref_jump > 20);
    CU_ASSERT(p.pref_fwd > 20);
    CU_ASSERT(p.pref_back < 95);
}

void test_projectile_burst_cooldown_blocks_repeat_fire(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 4);

    a.projectile_streak = ai_projectile_max_streak(a.difficulty) - 1;
    CU_ASSERT_TRUE(ai_projectile_is_allowed(&a));

    ai_projectile_use(&a);
    CU_ASSERT_EQUAL(a.projectile_streak, ai_projectile_max_streak(a.difficulty));
    CU_ASSERT_FALSE(ai_projectile_is_allowed(&a));
    CU_ASSERT_TRUE(a.projectile_cooldown > 0);
}

void test_projectile_cooldown_expires_and_resets_burst(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 4);

    a.projectile_cooldown = 1;
    a.projectile_streak = ai_projectile_max_streak(a.difficulty);

    ai_projectile_tick(&a);

    CU_ASSERT_EQUAL(a.projectile_cooldown, 0);
    CU_ASSERT_EQUAL(a.projectile_streak, 0);
    CU_ASSERT_TRUE(ai_projectile_is_allowed(&a));
}

void test_learning_projectile_is_difficulty_sensitive(void) {
    ai high_difficulty_ai;
    ai low_difficulty_ai;
    sd_pilot high_pilot;
    sd_pilot low_pilot;
    tactic_state high_t;
    tactic_state low_t;

    make_ai_with_difficulty(&high_difficulty_ai, &high_pilot, &high_t, 6);
    high_pilot.att_def = 95;
    high_pilot.att_sniper = 5;
    high_pilot.att_hyper = 5;
    high_pilot.att_jump = 5;
    high_pilot.pref_jump = 20;
    high_pilot.pref_fwd = 20;
    high_pilot.pref_back = 95;

    make_ai_with_difficulty(&low_difficulty_ai, &low_pilot, &low_t, 1);
    low_pilot.learning = 0.0f; // learning_moment must stay false
    low_pilot.att_def = 95;
    low_pilot.att_sniper = 5;
    low_pilot.att_hyper = 5;
    low_pilot.att_jump = 5;
    low_pilot.pref_jump = 20;
    low_pilot.pref_fwd = 20;
    low_pilot.pref_back = 95;

    high_difficulty_ai.shot = MAX_TIMES_SHOT - 1;
    low_difficulty_ai.shot = MAX_TIMES_SHOT - 1;

    ai_learning_adjust_from_projectile(&high_difficulty_ai);
    ai_learning_adjust_from_projectile(&low_difficulty_ai);

    CU_ASSERT_EQUAL(high_difficulty_ai.shot, MAX_TIMES_SHOT);
    CU_ASSERT_EQUAL(low_difficulty_ai.shot, MAX_TIMES_SHOT);
    CU_ASSERT_EQUAL(high_pilot.att_def, 10);
    CU_ASSERT_EQUAL(low_pilot.att_def, 95);
    CU_ASSERT(high_pilot.pref_fwd > 20);
    CU_ASSERT(low_pilot.pref_fwd == 20);
}

void test_pressure_burst_cooldown_blocks_repeat_charge(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 4);

    a.pressure_streak = ai_pressure_max_streak(a.difficulty) - 1;
    CU_ASSERT_TRUE(ai_pressure_is_allowed(&a));

    ai_pressure_use(&a);
    CU_ASSERT_EQUAL(a.pressure_streak, ai_pressure_max_streak(a.difficulty));
    CU_ASSERT_FALSE(ai_pressure_is_allowed(&a));
    CU_ASSERT_TRUE(a.pressure_cooldown > 0);
}

void test_pressure_cooldown_expires_and_resets_burst(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 4);

    a.pressure_cooldown = 1;
    a.pressure_streak = ai_pressure_max_streak(a.difficulty);

    ai_pressure_tick(&a);

    CU_ASSERT_EQUAL(a.pressure_cooldown, 0);
    CU_ASSERT_EQUAL(a.pressure_streak, 0);
    CU_ASSERT_TRUE(ai_pressure_is_allowed(&a));
}

/* -----------------------------------------------------------------------
 * ai_learning_maybe_forget
 * -------------------------------------------------------------------- */

void test_learning_forget_resets_counters_when_triggered(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    p.pilot_id = 0;      // valid pilot id so reset_pilot_personality works
    p.forget = 1.0f;     // very high forget value → forgetful() highly likely

    a.thrown = 5;
    a.shot = 4;
    a.blocked = 2;

    // Seed so roll_chance(2) succeeds (50% each call — run enough times)
    int forgot = 0;
    for(int i = 0; i < 100; i++) {
        a.thrown = 5;
        a.shot = 4;
        a.blocked = 2;
        if(ai_learning_maybe_forget(&a)) {
            forgot = 1;
            break;
        }
    }

    CU_ASSERT_TRUE(forgot);
    // When forgotten, counters must be zeroed
    CU_ASSERT_EQUAL(a.thrown, 0);
    CU_ASSERT_EQUAL(a.shot, 0);
    CU_ASSERT_EQUAL(a.blocked, 0);
}

void test_learning_forget_does_not_trigger_when_forget_zero(void) {
    ai a;
    sd_pilot p;
    tactic_state t;
    make_ai_with_difficulty(&a, &p, &t, 6);
    // forgetful() rolls rand_int(3)=0..2 and compares <= pilot->forget.
    // Setting forget below zero guarantees every roll fails.
    p.forget = -1.0f;

    a.thrown = 3;
    a.shot = 2;

    for(int i = 0; i < 50; i++) {
        bool result = ai_learning_maybe_forget(&a);
        CU_ASSERT_FALSE(result);
    }

    // Counters unchanged
    CU_ASSERT_EQUAL(a.thrown, 3);
    CU_ASSERT_EQUAL(a.shot, 2);
}

/* -----------------------------------------------------------------------
 * Test suite registration
 * -------------------------------------------------------------------- */

void ai_learning_test_suite(CU_pSuite suite) {
    CU_add_test(suite, "throw counter is incremented on each call",
                test_learning_throw_increments_counter);
    CU_add_test(suite, "throw: no personality adjustment below threshold",
                test_learning_throw_no_adjustment_below_threshold);
    CU_add_test(suite, "throw: pilot personality adjusted at threshold",
                test_learning_throw_adjusts_at_threshold);
    CU_add_test(suite, "throw: defence not capped when already low",
                test_learning_throw_defence_not_capped_when_low);
    CU_add_test(suite, "throw: no adjustment at low difficulty (learning=0)",
                test_learning_throw_no_adjustment_low_difficulty);
    CU_add_test(suite, "projectile counter is incremented on each call",
                test_learning_projectile_increments_counter);
    CU_add_test(suite, "projectile: no personality adjustment below threshold",
                test_learning_projectile_no_adjustment_below_threshold);
    CU_add_test(suite, "projectile: pilot personality adjusted at threshold",
                test_learning_projectile_adjusts_at_threshold);
    CU_add_test(suite, "projectile burst: cooldown blocks repeat fire",
                test_projectile_burst_cooldown_blocks_repeat_fire);
    CU_add_test(suite, "projectile burst: cooldown expires and resets burst",
                test_projectile_cooldown_expires_and_resets_burst);
    CU_add_test(suite, "projectile: learning is difficulty sensitive",
                test_learning_projectile_is_difficulty_sensitive);
    CU_add_test(suite, "pressure burst: cooldown blocks repeat charge",
                test_pressure_burst_cooldown_blocks_repeat_charge);
    CU_add_test(suite, "pressure burst: cooldown expires and resets burst",
                test_pressure_cooldown_expires_and_resets_burst);
    CU_add_test(suite, "forget: resets counters when triggered",
                test_learning_forget_resets_counters_when_triggered);
    CU_add_test(suite, "forget: never triggers when forget value is zero",
                test_learning_forget_does_not_trigger_when_forget_zero);
}
