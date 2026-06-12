/**
 * Tests for data-driven move executor helpers.
 */

#include <CUnit/CUnit.h>

#include "controller/controller.h"
#include "game/protos/object.h"

extern int ai_resolve_input(int input, int direction);

void test_resolve_forward_facing_right(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_RIGHT, OBJECT_FACE_RIGHT), ACT_RIGHT);
}

void test_resolve_forward_facing_left(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_RIGHT, OBJECT_FACE_LEFT), ACT_LEFT);
}

void test_resolve_back_facing_right(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_LEFT, OBJECT_FACE_RIGHT), ACT_LEFT);
}

void test_resolve_back_facing_left(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_LEFT, OBJECT_FACE_LEFT), ACT_RIGHT);
}

void test_resolve_down_back_facing_left(void) {
    int expected = ACT_DOWN | ACT_RIGHT;
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_DOWN | ACT_LEFT, OBJECT_FACE_LEFT), expected);
}

void test_resolve_non_direction_bits_unchanged(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_PUNCH, OBJECT_FACE_LEFT), ACT_PUNCH);
}

void ai_move_executor_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "exec: resolve F right", test_resolve_forward_facing_right) == NULL) return;
    if(CU_add_test(suite, "exec: resolve F left", test_resolve_forward_facing_left) == NULL) return;
    if(CU_add_test(suite, "exec: resolve B right", test_resolve_back_facing_right) == NULL) return;
    if(CU_add_test(suite, "exec: resolve B left", test_resolve_back_facing_left) == NULL) return;
    if(CU_add_test(suite, "exec: resolve DB left", test_resolve_down_back_facing_left) == NULL) return;
    if(CU_add_test(suite, "exec: non-dir bits unchanged", test_resolve_non_direction_bits_unchanged) == NULL) return;
}
