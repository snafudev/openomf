/**
 * Pilot stat audit tests (T7 of
 * docs/plans/2026-09-20-difficulty-config-and-pilot-stat-alignment.md)
 *
 * Locks the physical-vs-mental pilot stat split:
 *   - Physical stats (power, agility, endurance, stun_resistance, armor,
 *     arm/leg power, arm/leg speed) drive in-engine combat in
 *     src/game/objects/har.c (har_create, calc_damage_and_stun, har_take_damage).
 *   - Mental stats (att_*, ap_*, pref_*, learning, forget) drive the AI layer
 *     under src/game/ai/.
 *
 * These tests assert that the physical source of truth (resources/pilots.c)
 * matches the OMF 2097 reference, and that the AI personality path never
 * mutates physical stats.
 */

#include "game/ai/ai_state.h"
#include "game/common_defines.h"
#include "resources/pilots.h"
#include "CUnit/CUnit.h"
#include <string.h>

static void assert_pilot_stats(int id, int power, int agility, int endurance) {
    pilot p;
    memset(&p, 0, sizeof(p));
    pilot_get_info(&p, id);
    CU_ASSERT_EQUAL(p.power, power);
    CU_ASSERT_EQUAL(p.agility, agility);
    CU_ASSERT_EQUAL(p.endurance, endurance);
}

void test_pilot_stats_match_reference(void) {
    // Expected values from docs/OMF2097_REFERENCE.md "Pilot Stats Reference".
    assert_pilot_stats(PILOT_CRYSTAL, 5, 16, 9);
    assert_pilot_stats(PILOT_STEFFAN, 13, 9, 8);
    assert_pilot_stats(PILOT_MILANO, 7, 20, 4);
    assert_pilot_stats(PILOT_CHRISTIAN, 9, 7, 15);
    assert_pilot_stats(PILOT_SHIRRO, 20, 1, 8);
    assert_pilot_stats(PILOT_JEANPAUL, 9, 10, 11);
    assert_pilot_stats(PILOT_IBRAHIM, 10, 1, 20);
    assert_pilot_stats(PILOT_ANGEL, 7, 10, 13);
    assert_pilot_stats(PILOT_COSSETTE, 14, 8, 8);
    assert_pilot_stats(PILOT_RAVEN, 14, 4, 12);
    assert_pilot_stats(PILOT_KREISSACK, 16, 15, 16);
}

void test_personality_reset_preserves_physical_stats(void) {
    sd_pilot p;
    memset(&p, 0, sizeof(p));
    p.pilot_id = PILOT_CRYSTAL;
    p.power = 5;
    p.agility = 16;
    p.endurance = 9;
    p.stun_resistance = 4;
    p.armor = 6;
    p.arm_power = 3;
    p.leg_power = 5;
    p.arm_speed = 2;
    p.leg_speed = 7;

    reset_pilot_personality(&p);

    // The AI personality layer must never mutate physical combat stats.
    CU_ASSERT_EQUAL(p.power, 5);
    CU_ASSERT_EQUAL(p.agility, 16);
    CU_ASSERT_EQUAL(p.endurance, 9);
    CU_ASSERT_EQUAL(p.stun_resistance, 4);
    CU_ASSERT_EQUAL(p.armor, 6);
    CU_ASSERT_EQUAL(p.arm_power, 3);
    CU_ASSERT_EQUAL(p.leg_power, 5);
    CU_ASSERT_EQUAL(p.arm_speed, 2);
    CU_ASSERT_EQUAL(p.leg_speed, 7);
}

void ai_pilot_stats_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "pilot stats: reference values", test_pilot_stats_match_reference) == NULL) {
        return;
    }
    if(CU_add_test(suite, "pilot stats: personality reset preserves physical stats",
                   test_personality_reset_preserves_physical_stats) == NULL) {
        return;
    }
}
