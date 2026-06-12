/**
 * Tests for ai move parser helpers and JSON move array parsing.
 */

#include <CUnit/CUnit.h>

#include "controller/controller.h"
#include "game/ai/ai_skills_config_loader.h"
#include "game/ai/ai_tactic_engine.h"

extern bool ai_parse_sequence_token(const char *token, int *out_bits);
extern ai_move_condition ai_parse_condition_string(const char *cond);
extern int ai_parse_tactic_string(const char *tactic);
extern bool ai_parse_move_array(const char *json, const char *key, ai_move_def *out, uint8_t *out_count,
                                uint8_t max_count);

void test_token_forward(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("F", &bits));
    CU_ASSERT_EQUAL(bits, ACT_RIGHT);
}

void test_token_down_back_kick(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("DB+K", &bits));
    CU_ASSERT_EQUAL(bits, ACT_DOWN | ACT_LEFT | ACT_KICK);
}

void test_token_unknown(void) {
    int bits = 0;
    CU_ASSERT_FALSE(ai_parse_sequence_token("X", &bits));
}

void test_condition_diff_scale(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("high_difficulty"), MOVE_COND_HIGH_DIFFICULTY);
}

void test_condition_special_pref(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("special_preferred"), MOVE_COND_SPECIAL_PREF);
}

void test_condition_enemy_stunned(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("enemy_stunned"), MOVE_COND_ENEMY_STUNNED);
}

void test_condition_unknown(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("not_real"), MOVE_COND_NONE);
}

void test_tactic_grab(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("grab"), TACTIC_GRAB);
}

void test_tactic_unknown(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("not_real"), -1);
}

void test_parse_move_array_empty(void) {
    const char *json = "{\"charge_moves\":[]}";
    ai_move_def moves[AI_MAX_MOVES_PER_TYPE] = {0};
    uint8_t count = 99;

    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, AI_MAX_MOVES_PER_TYPE));
    CU_ASSERT_EQUAL(count, 0);
}

void test_parse_move_array_single_move(void) {
    const char *json =
        "{\"charge_moves\":[{"
        "\"name\":\"test\","
        "\"sequence\":[\"D\",\"DF\",\"F+P\"],"
        "\"range_min\":\"MID\","
        "\"conditions\":[\"special_preferred\",\"high_difficulty\"],"
        "\"follow_up_tactics\":[\"grab\",\"push\"]"
        "}]}";

    ai_move_def moves[AI_MAX_MOVES_PER_TYPE] = {0};
    uint8_t count = 0;

    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, AI_MAX_MOVES_PER_TYPE));
    CU_ASSERT_EQUAL(count, 1);
    CU_ASSERT_STRING_EQUAL(moves[0].name, "test");
    CU_ASSERT_EQUAL(moves[0].input_count, 3);
    CU_ASSERT_EQUAL(moves[0].inputs[0], ACT_DOWN);
    CU_ASSERT_EQUAL(moves[0].inputs[1], ACT_DOWN | ACT_RIGHT);
    CU_ASSERT_EQUAL(moves[0].inputs[2], ACT_RIGHT | ACT_PUNCH);
    CU_ASSERT_EQUAL(moves[0].range_min, MOVE_RANGE_MID);
    CU_ASSERT_EQUAL(moves[0].conditions, (ai_move_condition)(MOVE_COND_SPECIAL_PREF | MOVE_COND_HIGH_DIFFICULTY));
    CU_ASSERT_EQUAL(moves[0].follow_up_tactic_count, 2);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[0], TACTIC_GRAB);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[1], TACTIC_PUSH);
}

void ai_move_parser_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "parser: token F", test_token_forward) == NULL) return;
    if(CU_add_test(suite, "parser: token DB+K", test_token_down_back_kick) == NULL) return;
    if(CU_add_test(suite, "parser: token unknown", test_token_unknown) == NULL) return;
    if(CU_add_test(suite, "parser: condition diff_scale", test_condition_diff_scale) == NULL) return;
    if(CU_add_test(suite, "parser: condition special_preferred", test_condition_special_pref) == NULL) return;
    if(CU_add_test(suite, "parser: condition enemy_stunned", test_condition_enemy_stunned) == NULL) return;
    if(CU_add_test(suite, "parser: condition unknown", test_condition_unknown) == NULL) return;
    if(CU_add_test(suite, "parser: tactic grab", test_tactic_grab) == NULL) return;
    if(CU_add_test(suite, "parser: tactic unknown", test_tactic_unknown) == NULL) return;
    if(CU_add_test(suite, "parser: empty move array", test_parse_move_array_empty) == NULL) return;
    if(CU_add_test(suite, "parser: single move", test_parse_move_array_single_move) == NULL) return;
}
