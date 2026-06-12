/**
 * Test framework and fixtures for AI controller refactoring
 *
 * Provides mock pilot and AI objects for isolated testing
 * of decision engine and AI logic.
 */

#ifndef AI_CONTROLLER_TEST_H
#define AI_CONTROLLER_TEST_H

#include "game/ai/ai_decision_engine.h"
#include "formats/pilot.h"
#include "utils/allocator.h"
#include "utils/random.h"
#include <string.h>
#include <stdint.h>

typedef struct {
    sd_pilot pilot_data;
} test_pilot_fixture;

typedef struct {
    ai ai_data;
} test_ai_fixture;

static test_pilot_fixture *test_pilot_create(int id) {
    test_pilot_fixture *fix = omf_calloc(1, sizeof(test_pilot_fixture));
    memset(&fix->pilot_data, 0, sizeof(sd_pilot));
    fix->pilot_data.pilot_id = id;
    fix->pilot_data.learning = 1.5f;
    fix->pilot_data.forget = 0.25f;
    fix->pilot_data.att_normal = 50;
    fix->pilot_data.att_hyper = 50;
    fix->pilot_data.att_jump = 50;
    fix->pilot_data.att_sniper = 50;
    return fix;
}

static void test_pilot_free(test_pilot_fixture *fix) {
    omf_free(fix);
}

static void __attribute__((unused)) test_pilot_set_learning(test_pilot_fixture *fix, float learning) {
    fix->pilot_data.learning = learning;
}

static void __attribute__((unused)) test_pilot_set_forget(test_pilot_fixture *fix, float forget) {
    fix->pilot_data.forget = forget;
}

static test_ai_fixture *test_ai_create(int difficulty, test_pilot_fixture *pilot_fix) {
    test_ai_fixture *fix = omf_calloc(1, sizeof(test_ai_fixture));
    fix->ai_data.difficulty = difficulty;
    fix->ai_data.pilot = &pilot_fix->pilot_data;
    return fix;
}

static void test_ai_free(test_ai_fixture *fix) {
    omf_free(fix);
}

static void test_seed_random(uint32_t seed) {
    srand(seed);
}

#endif // AI_CONTROLLER_TEST_H
