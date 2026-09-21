/**
 * Unit tests for AI tactic and pilot state helpers
 */

#include "game/ai/ai_state.h"
#include "game/ai/ai_core_config.h"
#include "game/common_defines.h"
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

    const ai_core_config *cfg = ai_core_config_get_for_difficulty(a.difficulty);
    int base_act_timer = cfg->base_act_timer;
    CU_ASSERT(a.act_timer <= (base_act_timer - 2));
    CU_ASSERT(a.act_timer >= (base_act_timer - 4));
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

void test_reset_pilot_personality_for_raven(void) {
    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));
    pilot.pilot_id = PILOT_RAVEN;
    pilot.har_id = HAR_NOVA;

    reset_pilot_personality(&pilot);

    CU_ASSERT_EQUAL(pilot.ap_throw, 100);
    CU_ASSERT_EQUAL(pilot.ap_special, 120);
    CU_ASSERT_EQUAL(pilot.pref_fwd, 35);
    CU_ASSERT_DOUBLE_EQUAL(pilot.learning, 3.0f, 0.001);
}

void test_reset_pilot_personality_for_kreissack(void) {
    sd_pilot pilot;
    memset(&pilot, 0, sizeof(pilot));
    pilot.pilot_id = PILOT_KREISSACK;
    pilot.har_id = HAR_NOVA;

    reset_pilot_personality(&pilot);

    CU_ASSERT_EQUAL(pilot.att_hyper, 90);
    CU_ASSERT_EQUAL(pilot.ap_special, 140);
    CU_ASSERT_EQUAL(pilot.pref_fwd, 40);
    CU_ASSERT_DOUBLE_EQUAL(pilot.learning, 4.0f, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(pilot.forget, 0.18f, 0.001);
}

void test_ai_core_config_loads_difficulty_override(void) {
    ai_core_config cfg;
    memset(&cfg, 0, sizeof(cfg));

    bool ok = ai_core_config_load_for_difficulty(1, &cfg);

    CU_ASSERT_TRUE(ok);
    CU_ASSERT_EQUAL(cfg.base_act_chance, 15);
    CU_ASSERT_EQUAL(cfg.base_fwd_jump_chance, 40);
    CU_ASSERT_EQUAL(cfg.base_back_jump_chance, 45);
    CU_ASSERT_EQUAL(cfg.base_still_jump_chance, 70);
    CU_ASSERT_EQUAL(cfg.random_attack_chance, 35);
    CU_ASSERT_EQUAL(cfg.base_act_timer, 32);
    CU_ASSERT_EQUAL(cfg.block_chance, 40);
    CU_ASSERT_EQUAL(cfg.aggressive_tactics, 0);
    CU_ASSERT_EQUAL(cfg.jump_frequency_mult, 60);
}

void test_ai_core_config_veteran_is_baseline(void) {
    ai_core_config cfg;
    memset(&cfg, 0, sizeof(cfg));

    bool ok = ai_core_config_load_for_difficulty(2, &cfg);

    // Veteran must be a no-op cascade: identical to ai_core.ini.
    CU_ASSERT_TRUE(ok);
    CU_ASSERT_EQUAL(cfg.base_act_chance, 5);
    CU_ASSERT_EQUAL(cfg.base_fwd_jump_chance, 5);
    CU_ASSERT_EQUAL(cfg.base_back_jump_chance, 5);
    CU_ASSERT_EQUAL(cfg.base_still_jump_chance, 40);
    CU_ASSERT_EQUAL(cfg.random_attack_chance, 10);
    CU_ASSERT_EQUAL(cfg.base_act_timer, 28);
    CU_ASSERT_EQUAL(cfg.block_chance, 15);
    CU_ASSERT_EQUAL(cfg.aggressive_tactics, 0);
    CU_ASSERT_EQUAL(cfg.jump_frequency_mult, 100);
}

void test_ai_core_config_progression_matches_difficulty_expectations(void) {
    ai_core_config punching_bag;
    ai_core_config rookie;
    ai_core_config veteran;
    ai_core_config world_class;
    ai_core_config champion;
    ai_core_config deadly;
    ai_core_config ultimate;

    memset(&punching_bag, 0, sizeof(punching_bag));
    memset(&rookie, 0, sizeof(rookie));
    memset(&veteran, 0, sizeof(veteran));
    memset(&world_class, 0, sizeof(world_class));
    memset(&champion, 0, sizeof(champion));
    memset(&deadly, 0, sizeof(deadly));
    memset(&ultimate, 0, sizeof(ultimate));

    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(0, &punching_bag));
    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(1, &rookie));
    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(2, &veteran));
    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(3, &world_class));
    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(4, &champion));
    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(5, &deadly));
    CU_ASSERT_TRUE(ai_core_config_load_for_difficulty(6, &ultimate));

    // Higher difficulty should mean less blocking and more aggression.
    CU_ASSERT_TRUE(punching_bag.block_chance > rookie.block_chance);
    CU_ASSERT_TRUE(rookie.block_chance > veteran.block_chance);
    CU_ASSERT_TRUE(veteran.block_chance > world_class.block_chance);
    CU_ASSERT_TRUE(world_class.block_chance > champion.block_chance);
    CU_ASSERT_TRUE(champion.block_chance > deadly.block_chance);
    CU_ASSERT_TRUE(deadly.block_chance >= ultimate.block_chance);

    CU_ASSERT_EQUAL(punching_bag.aggressive_tactics, 0);
    CU_ASSERT_EQUAL(rookie.aggressive_tactics, 0);
    CU_ASSERT_EQUAL(veteran.aggressive_tactics, 0);
    CU_ASSERT_EQUAL(world_class.aggressive_tactics, 1);
    CU_ASSERT_EQUAL(champion.aggressive_tactics, 1);
    CU_ASSERT_EQUAL(deadly.aggressive_tactics, 1);
    CU_ASSERT_EQUAL(ultimate.aggressive_tactics, 1);

    CU_ASSERT_TRUE(punching_bag.jump_frequency_mult < rookie.jump_frequency_mult);
    CU_ASSERT_TRUE(rookie.jump_frequency_mult < veteran.jump_frequency_mult);
    CU_ASSERT_TRUE(veteran.jump_frequency_mult < world_class.jump_frequency_mult);
    CU_ASSERT_TRUE(world_class.jump_frequency_mult < champion.jump_frequency_mult);
    CU_ASSERT_TRUE(champion.jump_frequency_mult < deadly.jump_frequency_mult);
    CU_ASSERT_TRUE(deadly.jump_frequency_mult <= ultimate.jump_frequency_mult);
}

void ai_state_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "reset tactic state: clears active tactic", test_reset_tactic_state_clears_active_tactic) == NULL) return;
    if(CU_add_test(suite, "reset tactic state: no active tactic", test_reset_tactic_state_with_no_active_tactic) == NULL) return;
    if(CU_add_test(suite, "reset act timer: difficulty one", test_reset_act_timer_for_difficulty_one) == NULL) return;
    if(CU_add_test(suite, "reset pilot personality: crystal", test_reset_pilot_personality_for_crystal) == NULL) return;
    if(CU_add_test(suite, "reset pilot personality: raven", test_reset_pilot_personality_for_raven) == NULL) return;
    if(CU_add_test(suite, "reset pilot personality: kreissack", test_reset_pilot_personality_for_kreissack) == NULL) return;
    if(CU_add_test(suite, "ai core config: difficulty override", test_ai_core_config_loads_difficulty_override) == NULL) return;
    if(CU_add_test(suite, "ai core config: veteran is baseline", test_ai_core_config_veteran_is_baseline) == NULL) return;
    if(CU_add_test(suite, "ai core config: difficulty progression follows expectations", test_ai_core_config_progression_matches_difficulty_expectations) == NULL) return;
}
