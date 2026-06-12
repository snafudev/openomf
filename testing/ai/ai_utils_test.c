/**
 * Unit tests for AI utility helpers
 */

#include "game/ai/ai_utils.h"
#include "resources/af_move.h"
#include "CUnit/CUnit.h"
#include <string.h>

static void test_move_init(af_move *move, const char *move_string) {
    memset(move, 0, sizeof(*move));
    str_create(&move->move_string);
    str_from_c(&move->move_string, move_string);
}

static void test_move_free(af_move *move) {
    str_free(&move->move_string);
}

void test_enemy_range_cramped(void) {
    CU_ASSERT_EQUAL(ai_enemy_range_from_positions(100.0f, 120.0f), RANGE_CRAMPED);
}

void test_enemy_range_close(void) {
    CU_ASSERT_EQUAL(ai_enemy_range_from_positions(100.0f, 160.0f), RANGE_CLOSE);
}

void test_enemy_range_mid(void) {
    CU_ASSERT_EQUAL(ai_enemy_range_from_positions(100.0f, 220.0f), RANGE_MID);
}

void test_enemy_range_far(void) {
    CU_ASSERT_EQUAL(ai_enemy_range_from_positions(100.0f, 280.0f), RANGE_FAR);
}

void test_enemy_stunned_or_stasis_when_stunned(void) {
    CU_ASSERT_TRUE(ai_enemy_is_stunned_or_stasis(STATE_STUNNED, 0));
}

void test_enemy_stunned_or_stasis_when_in_stasis(void) {
    CU_ASSERT_TRUE(ai_enemy_is_stunned_or_stasis(STATE_STANDING, 3));
}

void test_enemy_stunned_or_stasis_when_neither(void) {
    CU_ASSERT_FALSE(ai_enemy_is_stunned_or_stasis(STATE_STANDING, 0));
}

void test_is_special_move_returns_false_for_kick(void) {
    af_move move;
    test_move_init(&move, "K");
    CU_ASSERT_FALSE(is_special_move(&move));
    test_move_free(&move);
}

void test_is_special_move_returns_false_for_punch_variant(void) {
    af_move move;
    test_move_init(&move, "P6");
    CU_ASSERT_FALSE(is_special_move(&move));
    test_move_free(&move);
}

void test_is_special_move_returns_true_for_directional_input(void) {
    af_move move;
    test_move_init(&move, "236P");
    CU_ASSERT_TRUE(is_special_move(&move));
    test_move_free(&move);
}

void test_har_has_projectiles_for_jaguar(void) {
    CU_ASSERT_TRUE(har_has_projectiles(HAR_JAGUAR));
}

void test_har_has_projectiles_for_katana(void) {
    CU_ASSERT_FALSE(har_has_projectiles(HAR_KATANA));
}

void test_har_has_charge_for_gargoyle(void) {
    CU_ASSERT_TRUE(har_has_charge(HAR_GARGOYLE));
}

void test_har_has_push_for_nova(void) {
    CU_ASSERT_TRUE(har_has_push(HAR_NOVA));
}

void test_char_to_act_maps_forward_for_right_facing(void) {
    str move;
    str_create(&move);
    str_from_c(&move, "6");

    int pos = 0;
    int action = char_to_act(&move, OBJECT_FACE_RIGHT, &pos);

    CU_ASSERT_EQUAL(action, ACT_RIGHT);
    CU_ASSERT_EQUAL(pos, 0);

    str_free(&move);
}

void ai_utils_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "enemy range: cramped", test_enemy_range_cramped) == NULL) return;
    if(CU_add_test(suite, "enemy range: close", test_enemy_range_close) == NULL) return;
    if(CU_add_test(suite, "enemy range: mid", test_enemy_range_mid) == NULL) return;
    if(CU_add_test(suite, "enemy range: far", test_enemy_range_far) == NULL) return;
    if(CU_add_test(suite, "enemy state: stunned", test_enemy_stunned_or_stasis_when_stunned) == NULL) return;
    if(CU_add_test(suite, "enemy state: stasis", test_enemy_stunned_or_stasis_when_in_stasis) == NULL) return;
    if(CU_add_test(suite, "enemy state: normal", test_enemy_stunned_or_stasis_when_neither) == NULL) return;
    if(CU_add_test(suite, "special move: kick", test_is_special_move_returns_false_for_kick) == NULL) return;
    if(CU_add_test(suite, "special move: punch variant", test_is_special_move_returns_false_for_punch_variant) == NULL) return;
    if(CU_add_test(suite, "special move: directional", test_is_special_move_returns_true_for_directional_input) == NULL) return;
    if(CU_add_test(suite, "projectiles: jaguar", test_har_has_projectiles_for_jaguar) == NULL) return;
    if(CU_add_test(suite, "projectiles: katana", test_har_has_projectiles_for_katana) == NULL) return;
    if(CU_add_test(suite, "charge: gargoyle", test_har_has_charge_for_gargoyle) == NULL) return;
    if(CU_add_test(suite, "push: nova", test_har_has_push_for_nova) == NULL) return;
    if(CU_add_test(suite, "char_to_act: forward", test_char_to_act_maps_forward_for_right_facing) == NULL) return;
}
