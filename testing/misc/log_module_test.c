/**
 * Tests for per-module log filtering (M0).
 *
 * Exercises the pure parsing logic `log_modules_from_string()` and the module
 * bitmask constants. Actual sink filtering is exercised via the CLI flag in
 * manual/integration verification.
 */

#include "utils/log.h"
#include <CUnit/CUnit.h>

void test_log_modules_single(void) {
    CU_ASSERT_EQUAL(log_modules_from_string("ai"), LOG_MODULE_AI);
    CU_ASSERT_EQUAL(log_modules_from_string("tactic"), LOG_MODULE_TACTIC);
    CU_ASSERT_EQUAL(log_modules_from_string("ai-tactics"), LOG_MODULE_TACTIC);
    CU_ASSERT_EQUAL(log_modules_from_string("ai_tactics"), LOG_MODULE_TACTIC);
    CU_ASSERT_EQUAL(log_modules_from_string("rec"), LOG_MODULE_REC);
}

void test_log_modules_multiple(void) {
    CU_ASSERT_EQUAL(log_modules_from_string("ai,tactic"), LOG_MODULE_AI | LOG_MODULE_TACTIC);
    CU_ASSERT_EQUAL(log_modules_from_string("rec,har,move"), LOG_MODULE_REC | LOG_MODULE_HAR | LOG_MODULE_MOVE);
}

void test_log_modules_whitespace(void) {
    CU_ASSERT_EQUAL(log_modules_from_string("ai, tactic"), LOG_MODULE_AI | LOG_MODULE_TACTIC);
    CU_ASSERT_EQUAL(log_modules_from_string(" ai "), LOG_MODULE_AI);
}

void test_log_modules_all(void) {
    CU_ASSERT_EQUAL(log_modules_from_string("all"), LOG_MODULE_ALL);
    CU_ASSERT_EQUAL(log_modules_from_string("ai,all"), LOG_MODULE_ALL);
}

void test_log_modules_empty_falls_back_to_all(void) {
    CU_ASSERT_EQUAL(log_modules_from_string(""), LOG_MODULE_ALL);
    CU_ASSERT_EQUAL(log_modules_from_string(NULL), LOG_MODULE_ALL);
}

void test_log_modules_unknown_tokens_ignored(void) {
    // Unknown tokens are ignored; a list with at least one known token yields
    // only the known bits.
    CU_ASSERT_EQUAL(log_modules_from_string("typo,ai"), LOG_MODULE_AI);

    // A list with no recognized tokens falls back to "all" to avoid silently
    // silencing every module.
    CU_ASSERT_EQUAL(log_modules_from_string("typo"), LOG_MODULE_ALL);
}

void test_log_module_bits_are_distinct(void) {
    CU_ASSERT_TRUE(LOG_MODULE_AI != LOG_MODULE_TACTIC);
    CU_ASSERT_TRUE(LOG_MODULE_TACTIC != LOG_MODULE_HAR);
    CU_ASSERT_TRUE(LOG_MODULE_HAR != LOG_MODULE_REC);
    CU_ASSERT_EQUAL(LOG_MODULE_NONE, 0);
}

void log_module_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "log modules: single tag", test_log_modules_single) == NULL) return;
    if(CU_add_test(suite, "log modules: multiple tags", test_log_modules_multiple) == NULL) return;
    if(CU_add_test(suite, "log modules: whitespace", test_log_modules_whitespace) == NULL) return;
    if(CU_add_test(suite, "log modules: all", test_log_modules_all) == NULL) return;
    if(CU_add_test(suite, "log modules: empty falls back to all", test_log_modules_empty_falls_back_to_all) == NULL)
        return;
    if(CU_add_test(suite, "log modules: unknown tokens ignored", test_log_modules_unknown_tokens_ignored) == NULL)
        return;
    if(CU_add_test(suite, "log modules: bits distinct", test_log_module_bits_are_distinct) == NULL) return;
}
