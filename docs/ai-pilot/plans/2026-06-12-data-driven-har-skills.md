# Data-Driven HAR Skill Execution — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace hardcoded per-HAR switch cases in `ai_har_skills.c` with a generic executor driven by move definitions parsed at load time from `resources/ai_config/hars/*.json`.

**Architecture:** New types (`ai_move_def`, `ai_move_condition`, `ai_move_range`) are added to `ai_skills_config_loader.h`. A parser in `ai_skills_config_loader.c` populates `ai_char_config` with pre-resolved `ai_move_def` arrays at load time. A generic executor in `ai_har_skills.c` iterates move lists at runtime — selecting the first move whose `range_min`, `range_max`, and `conditions` all pass — then chains the resolved ACT_* inputs.

**Tech Stack:** C99, CUnit tests, existing JSON string helpers in `ai_skills_config_loader.c`.

**Reference:** Design doc at `../../plans/2026-06-12-data-driven-har-skills-design.md`

---

### Task 1: Extend header with new types

**Files:**
- Modify: `src/game/ai/ai_skills_config_loader.h`

**Step 1: Add the new types before the `ai_char_config` struct**

Replace the existing header body with the following (keep the file guards and includes):

```c
#ifndef AI_SKILLS_CONFIG_LOADER_H
#define AI_SKILLS_CONFIG_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#define AI_MOVE_MAX_INPUTS          8
#define AI_MOVE_MAX_FOLLOW_TACTICS  8
#define AI_MOVE_NAME_LEN            32
#define AI_MAX_MOVES_PER_TYPE       8

typedef enum {
    MOVE_COND_NONE              = 0,
    MOVE_COND_DIFF_SCALE        = 1 << 0, // diff_scale(a)
    MOVE_COND_SPECIAL_PREF      = 1 << 1, // roll_pref(ap_special)
    MOVE_COND_LOW_PREFERRED     = 1 << 2, // roll_pref(ap_low)
    MOVE_COND_JUMP_PREFERRED    = 1 << 3, // roll_pref(att_jump)
    MOVE_COND_RANDOM_HALF       = 1 << 4, // roll_chance(2)
    MOVE_COND_RANDOM_THIRD      = 1 << 5, // roll_chance(3)
    MOVE_COND_ENEMY_NOT_STUNNED = 1 << 6, // !enemy_is_stunned_or_stasis
} ai_move_condition;

typedef enum {
    MOVE_RANGE_ANY = 0,
    MOVE_RANGE_CLOSE,
    MOVE_RANGE_MID,
    MOVE_RANGE_FAR,
} ai_move_range;

typedef struct {
    char             name[AI_MOVE_NAME_LEN];
    int              inputs[AI_MOVE_MAX_INPUTS];
    uint8_t          input_count;
    ai_move_range    range_min;
    ai_move_range    range_max; // MOVE_RANGE_FAR = no max constraint
    ai_move_condition conditions;
    int              follow_up_tactics[AI_MOVE_MAX_FOLLOW_TACTICS];
    uint8_t          follow_up_tactic_count;
} ai_move_def;

typedef struct ai_char_config {
    int     har_id;
    bool    loaded_from_file;
    bool    has_charge_moves;
    bool    has_push_moves;
    bool    has_projectile_moves;
    uint8_t charge_move_count;
    uint8_t push_move_count;
    uint8_t projectile_move_count;
    ai_move_def charge_moves[AI_MAX_MOVES_PER_TYPE];
    ai_move_def push_moves[AI_MAX_MOVES_PER_TYPE];
    ai_move_def projectile_moves[AI_MAX_MOVES_PER_TYPE];
} ai_char_config;

const ai_char_config *ai_skills_config_get(int har_id);
bool ai_skills_config_apply_overlay(ai_char_config *cfg, const char *json_buf);
void ai_skills_config_reset_cache(void);

#endif // AI_SKILLS_CONFIG_LOADER_H
```

**Step 2: Build to confirm no regressions**

```bash
cmake --build build -j4 2>&1 | head -40
```
Expected: compiles cleanly (the new arrays in `ai_char_config` are zero-initialised by the existing `{0}` init in the loader).

**Step 3: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 2: Write token parser tests (TDD)

**Files:**
- Create: `testing/ai/ai_move_parser_test.c`
- Modify: `testing/test_main.c`

**Step 1: Create the test file**

```c
/**
 * Tests for the ai_move_def sequence token and condition parser.
 */

#include <CUnit/CUnit.h>
#include <string.h>

#include "game/ai/ai_skills_config_loader.h"

// Forward-declare the internal parser function we will expose for testing.
// It is defined in ai_skills_config_loader.c and declared extern here.
extern bool ai_parse_sequence_token(const char *token, int *out_bits);
extern ai_move_condition ai_parse_condition_string(const char *cond);
extern int ai_parse_tactic_string(const char *tactic);
extern bool ai_parse_move_array(const char *json, const char *key,
                                 ai_move_def *out, uint8_t *out_count,
                                 uint8_t max_count);

/* ---- token tests ---- */

void test_token_F_gives_ACT_RIGHT(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("F", &bits));
    CU_ASSERT_EQUAL(bits, ACT_RIGHT);
}

void test_token_B_gives_ACT_LEFT(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("B", &bits));
    CU_ASSERT_EQUAL(bits, ACT_LEFT);
}

void test_token_D_gives_ACT_DOWN(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("D", &bits));
    CU_ASSERT_EQUAL(bits, ACT_DOWN);
}

void test_token_U_gives_ACT_UP(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("U", &bits));
    CU_ASSERT_EQUAL(bits, ACT_UP);
}

void test_token_DF_gives_DOWN_RIGHT(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("DF", &bits));
    CU_ASSERT_EQUAL(bits, ACT_DOWN | ACT_RIGHT);
}

void test_token_DB_gives_DOWN_LEFT(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("DB", &bits));
    CU_ASSERT_EQUAL(bits, ACT_DOWN | ACT_LEFT);
}

void test_token_5_gives_ACT_STOP(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("5", &bits));
    CU_ASSERT_EQUAL(bits, ACT_STOP);
}

void test_token_P_gives_ACT_PUNCH(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("P", &bits));
    CU_ASSERT_EQUAL(bits, ACT_PUNCH);
}

void test_token_K_gives_ACT_KICK(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("K", &bits));
    CU_ASSERT_EQUAL(bits, ACT_KICK);
}

void test_token_compound_DB_plus_K(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("DB+K", &bits));
    CU_ASSERT_EQUAL(bits, ACT_DOWN | ACT_LEFT | ACT_KICK);
}

void test_token_compound_F_plus_P(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("F+P", &bits));
    CU_ASSERT_EQUAL(bits, ACT_RIGHT | ACT_PUNCH);
}

void test_token_compound_B_plus_K(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("B+K", &bits));
    CU_ASSERT_EQUAL(bits, ACT_LEFT | ACT_KICK);
}

void test_token_compound_D_plus_P(void) {
    int bits = 0;
    CU_ASSERT_TRUE(ai_parse_sequence_token("D+P", &bits));
    CU_ASSERT_EQUAL(bits, ACT_DOWN | ACT_PUNCH);
}

void test_token_unknown_returns_false(void) {
    int bits = 0;
    CU_ASSERT_FALSE(ai_parse_sequence_token("X", &bits));
}

void test_token_empty_returns_false(void) {
    int bits = 0;
    CU_ASSERT_FALSE(ai_parse_sequence_token("", &bits));
}

/* ---- condition string tests ---- */

void test_cond_diff_scale(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("diff_scale"), MOVE_COND_DIFF_SCALE);
}

void test_cond_special_preferred(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("special_preferred"), MOVE_COND_SPECIAL_PREF);
}

void test_cond_low_preferred(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("low_preferred"), MOVE_COND_LOW_PREFERRED);
}

void test_cond_jump_preferred(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("jump_preferred"), MOVE_COND_JUMP_PREFERRED);
}

void test_cond_random_half(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("random_half"), MOVE_COND_RANDOM_HALF);
}

void test_cond_random_third(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("random_third"), MOVE_COND_RANDOM_THIRD);
}

void test_cond_enemy_not_stunned(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("enemy_not_stunned"), MOVE_COND_ENEMY_NOT_STUNNED);
}

void test_cond_unknown_returns_none(void) {
    CU_ASSERT_EQUAL(ai_parse_condition_string("not_a_condition"), MOVE_COND_NONE);
}

/* ---- tactic string tests ---- */

void test_tactic_grab(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("grab"), TACTIC_GRAB);
}

void test_tactic_push(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("push"), TACTIC_PUSH);
}

void test_tactic_shoot(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("shoot"), TACTIC_SHOOT);
}

void test_tactic_spam(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("spam"), TACTIC_SPAM);
}

void test_tactic_trip(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("trip"), TACTIC_TRIP);
}

void test_tactic_unknown_returns_neg1(void) {
    CU_ASSERT_EQUAL(ai_parse_tactic_string("nonexistent"), -1);
}

/* ---- parse_move_array tests ---- */

void test_parse_move_array_empty_array(void) {
    const char *json = "{\"charge_moves\":[]}";
    ai_move_def moves[8];
    uint8_t count = 99;
    bool ok = ai_parse_move_array(json, "charge_moves", moves, &count, 8);
    CU_ASSERT_TRUE(ok);
    CU_ASSERT_EQUAL(count, 0);
}

void test_parse_move_array_single_move_sequence(void) {
    const char *json =
        "{\"charge_moves\":["
        "{\"name\":\"test_move\","
        "\"sequence\":[\"D\",\"DF\",\"F+P\"],"
        "\"range_min\":\"ANY\"}]}";
    ai_move_def moves[8];
    uint8_t count = 0;
    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, 8));
    CU_ASSERT_EQUAL(count, 1);
    CU_ASSERT_EQUAL(moves[0].input_count, 3);
    CU_ASSERT_EQUAL(moves[0].inputs[0], ACT_DOWN);
    CU_ASSERT_EQUAL(moves[0].inputs[1], ACT_DOWN | ACT_RIGHT);
    CU_ASSERT_EQUAL(moves[0].inputs[2], ACT_RIGHT | ACT_PUNCH);
    CU_ASSERT_EQUAL(moves[0].range_min, MOVE_RANGE_ANY);
    CU_ASSERT_EQUAL(moves[0].range_max, MOVE_RANGE_FAR);
    CU_ASSERT_EQUAL(moves[0].conditions, MOVE_COND_NONE);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactic_count, 0);
}

void test_parse_move_array_conditions_bitmask(void) {
    const char *json =
        "{\"charge_moves\":["
        "{\"name\":\"m\",\"sequence\":[\"D\"],"
        "\"range_min\":\"MID\","
        "\"conditions\":[\"special_preferred\",\"diff_scale\"]}]}";
    ai_move_def moves[8];
    uint8_t count = 0;
    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, 8));
    CU_ASSERT_EQUAL(count, 1);
    CU_ASSERT_EQUAL(moves[0].range_min, MOVE_RANGE_MID);
    CU_ASSERT_EQUAL(moves[0].conditions,
                    (ai_move_condition)(MOVE_COND_SPECIAL_PREF | MOVE_COND_DIFF_SCALE));
}

void test_parse_move_array_range_max(void) {
    const char *json =
        "{\"charge_moves\":["
        "{\"name\":\"m\",\"sequence\":[\"D\"],"
        "\"range_min\":\"ANY\",\"range_max\":\"MID\"}]}";
    ai_move_def moves[8];
    uint8_t count = 0;
    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, 8));
    CU_ASSERT_EQUAL(count, 1);
    CU_ASSERT_EQUAL(moves[0].range_min, MOVE_RANGE_ANY);
    CU_ASSERT_EQUAL(moves[0].range_max, MOVE_RANGE_MID);
}

void test_parse_move_array_follow_up_tactics(void) {
    const char *json =
        "{\"charge_moves\":["
        "{\"name\":\"m\",\"sequence\":[\"D\",\"P\"],"
        "\"range_min\":\"MID\","
        "\"conditions\":[\"special_preferred\",\"diff_scale\"],"
        "\"follow_up_tactics\":[\"grab\",\"push\",\"shoot\",\"spam\",\"trip\"]}]}";
    ai_move_def moves[8];
    uint8_t count = 0;
    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, 8));
    CU_ASSERT_EQUAL(count, 1);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactic_count, 5);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[0], TACTIC_GRAB);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[1], TACTIC_PUSH);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[2], TACTIC_SHOOT);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[3], TACTIC_SPAM);
    CU_ASSERT_EQUAL(moves[0].follow_up_tactics[4], TACTIC_TRIP);
}

void test_parse_move_array_missing_key_returns_false(void) {
    const char *json = "{\"charge_moves\":[]}";
    ai_move_def moves[8];
    uint8_t count = 0;
    CU_ASSERT_FALSE(ai_parse_move_array(json, "push_moves", moves, &count, 8));
}

void test_parse_move_array_unknown_condition_skipped(void) {
    const char *json =
        "{\"charge_moves\":["
        "{\"name\":\"m\",\"sequence\":[\"D\"],"
        "\"range_min\":\"ANY\","
        "\"conditions\":[\"diff_scale\",\"not_real\"]}]}";
    ai_move_def moves[8];
    uint8_t count = 0;
    CU_ASSERT_TRUE(ai_parse_move_array(json, "charge_moves", moves, &count, 8));
    CU_ASSERT_EQUAL(count, 1);
    // only diff_scale set; unknown condition silently skipped
    CU_ASSERT_EQUAL(moves[0].conditions, MOVE_COND_DIFF_SCALE);
}

void ai_move_parser_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "token: F -> ACT_RIGHT", test_token_F_gives_ACT_RIGHT) == NULL) return;
    if(CU_add_test(suite, "token: B -> ACT_LEFT", test_token_B_gives_ACT_LEFT) == NULL) return;
    if(CU_add_test(suite, "token: D -> ACT_DOWN", test_token_D_gives_ACT_DOWN) == NULL) return;
    if(CU_add_test(suite, "token: U -> ACT_UP", test_token_U_gives_ACT_UP) == NULL) return;
    if(CU_add_test(suite, "token: DF -> DOWN|RIGHT", test_token_DF_gives_DOWN_RIGHT) == NULL) return;
    if(CU_add_test(suite, "token: DB -> DOWN|LEFT", test_token_DB_gives_DOWN_LEFT) == NULL) return;
    if(CU_add_test(suite, "token: 5 -> ACT_STOP", test_token_5_gives_ACT_STOP) == NULL) return;
    if(CU_add_test(suite, "token: P -> ACT_PUNCH", test_token_P_gives_ACT_PUNCH) == NULL) return;
    if(CU_add_test(suite, "token: K -> ACT_KICK", test_token_K_gives_ACT_KICK) == NULL) return;
    if(CU_add_test(suite, "token: DB+K compound", test_token_compound_DB_plus_K) == NULL) return;
    if(CU_add_test(suite, "token: F+P compound", test_token_compound_F_plus_P) == NULL) return;
    if(CU_add_test(suite, "token: B+K compound", test_token_compound_B_plus_K) == NULL) return;
    if(CU_add_test(suite, "token: D+P compound", test_token_compound_D_plus_P) == NULL) return;
    if(CU_add_test(suite, "token: unknown returns false", test_token_unknown_returns_false) == NULL) return;
    if(CU_add_test(suite, "token: empty returns false", test_token_empty_returns_false) == NULL) return;
    if(CU_add_test(suite, "cond: diff_scale", test_cond_diff_scale) == NULL) return;
    if(CU_add_test(suite, "cond: special_preferred", test_cond_special_preferred) == NULL) return;
    if(CU_add_test(suite, "cond: low_preferred", test_cond_low_preferred) == NULL) return;
    if(CU_add_test(suite, "cond: jump_preferred", test_cond_jump_preferred) == NULL) return;
    if(CU_add_test(suite, "cond: random_half", test_cond_random_half) == NULL) return;
    if(CU_add_test(suite, "cond: random_third", test_cond_random_third) == NULL) return;
    if(CU_add_test(suite, "cond: enemy_not_stunned", test_cond_enemy_not_stunned) == NULL) return;
    if(CU_add_test(suite, "cond: unknown returns NONE", test_cond_unknown_returns_none) == NULL) return;
    if(CU_add_test(suite, "tactic: grab", test_tactic_grab) == NULL) return;
    if(CU_add_test(suite, "tactic: push", test_tactic_push) == NULL) return;
    if(CU_add_test(suite, "tactic: shoot", test_tactic_shoot) == NULL) return;
    if(CU_add_test(suite, "tactic: spam", test_tactic_spam) == NULL) return;
    if(CU_add_test(suite, "tactic: trip", test_tactic_trip) == NULL) return;
    if(CU_add_test(suite, "tactic: unknown returns -1", test_tactic_unknown_returns_neg1) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: empty", test_parse_move_array_empty_array) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: single move", test_parse_move_array_single_move_sequence) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: conditions bitmask", test_parse_move_array_conditions_bitmask) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: range_max", test_parse_move_array_range_max) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: follow_up_tactics", test_parse_move_array_follow_up_tactics) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: missing key", test_parse_move_array_missing_key_returns_false) == NULL) return;
    if(CU_add_test(suite, "parse_move_array: unknown cond skipped", test_parse_move_array_unknown_condition_skipped) == NULL) return;
}
```

**Step 2: Register the suite in `testing/test_main.c`**

Add to the forward declarations block (after the existing AI suite declarations):
```c
void ai_move_parser_test_suite(CU_pSuite suite);
```

Add suite registration (after the `ai_har_skills_suite` block):
```c
    CU_pSuite ai_move_parser_suite = CU_add_suite("AI Move Parser", NULL, NULL);
    if(ai_move_parser_suite == NULL) {
        goto end;
    }
    ai_move_parser_test_suite(ai_move_parser_suite);
```

**Step 3: Build — expect link/compile errors for undefined symbols**

```bash
cmake --build build -j4 2>&1 | grep -E "error:|undefined"
```
Expected: errors about `ai_parse_sequence_token` etc. being undefined. This is correct — tests are written, now implement.

---

### Task 3: Implement the token, condition, and tactic parsers

**Files:**
- Modify: `src/game/ai/ai_skills_config_loader.c`

**Step 1: Add the include for TACTIC_* and ACT_* at the top of the loader**

The file already includes `ai_utils.h`. Also add:
```c
#include "controller/controller.h"
#include "game/ai/ai_tactic_engine.h"
```

**Step 2: Add the three parser functions (non-static, so tests can call them)**

Add after the existing static helpers and before `load_har_config`:

```c
bool ai_parse_sequence_token(const char *token, int *out_bits) {
    if(token == NULL || out_bits == NULL || token[0] == '\0') {
        return false;
    }

    // Handle compound tokens like "DB+K" or "F+P" by splitting on '+'
    const char *plus = strchr(token, '+');
    if(plus != NULL) {
        char left[8] = {0};
        char right[8] = {0};
        size_t left_len = (size_t)(plus - token);
        if(left_len == 0 || left_len >= sizeof(left)) {
            return false;
        }
        memcpy(left, token, left_len);
        strncpy(right, plus + 1, sizeof(right) - 1);

        int left_bits = 0, right_bits = 0;
        if(!ai_parse_sequence_token(left, &left_bits) ||
           !ai_parse_sequence_token(right, &right_bits)) {
            return false;
        }
        *out_bits = left_bits | right_bits;
        return true;
    }

    if(strcmp(token, "F")  == 0) { *out_bits = ACT_RIGHT;              return true; }
    if(strcmp(token, "B")  == 0) { *out_bits = ACT_LEFT;               return true; }
    if(strcmp(token, "D")  == 0) { *out_bits = ACT_DOWN;               return true; }
    if(strcmp(token, "U")  == 0) { *out_bits = ACT_UP;                 return true; }
    if(strcmp(token, "DF") == 0) { *out_bits = ACT_DOWN | ACT_RIGHT;   return true; }
    if(strcmp(token, "DB") == 0) { *out_bits = ACT_DOWN | ACT_LEFT;    return true; }
    if(strcmp(token, "5")  == 0) { *out_bits = ACT_STOP;               return true; }
    if(strcmp(token, "P")  == 0) { *out_bits = ACT_PUNCH;              return true; }
    if(strcmp(token, "K")  == 0) { *out_bits = ACT_KICK;               return true; }

    return false;
}

ai_move_condition ai_parse_condition_string(const char *cond) {
    if(cond == NULL) return MOVE_COND_NONE;
    if(strcmp(cond, "diff_scale")        == 0) return MOVE_COND_DIFF_SCALE;
    if(strcmp(cond, "special_preferred") == 0) return MOVE_COND_SPECIAL_PREF;
    if(strcmp(cond, "low_preferred")     == 0) return MOVE_COND_LOW_PREFERRED;
    if(strcmp(cond, "jump_preferred")    == 0) return MOVE_COND_JUMP_PREFERRED;
    if(strcmp(cond, "random_half")       == 0) return MOVE_COND_RANDOM_HALF;
    if(strcmp(cond, "random_third")      == 0) return MOVE_COND_RANDOM_THIRD;
    if(strcmp(cond, "enemy_not_stunned") == 0) return MOVE_COND_ENEMY_NOT_STUNNED;
    return MOVE_COND_NONE;
}

int ai_parse_tactic_string(const char *tactic) {
    if(tactic == NULL) return -1;
    if(strcmp(tactic, "escape")  == 0) return TACTIC_ESCAPE;
    if(strcmp(tactic, "turtle")  == 0) return TACTIC_TURTLE;
    if(strcmp(tactic, "grab")    == 0) return TACTIC_GRAB;
    if(strcmp(tactic, "spam")    == 0) return TACTIC_SPAM;
    if(strcmp(tactic, "shoot")   == 0) return TACTIC_SHOOT;
    if(strcmp(tactic, "trip")    == 0) return TACTIC_TRIP;
    if(strcmp(tactic, "quick")   == 0) return TACTIC_QUICK;
    if(strcmp(tactic, "close")   == 0) return TACTIC_CLOSE;
    if(strcmp(tactic, "fly")     == 0) return TACTIC_FLY;
    if(strcmp(tactic, "push")    == 0) return TACTIC_PUSH;
    if(strcmp(tactic, "counter") == 0) return TACTIC_COUNTER;
    return -1;
}
```

**Step 3: Build and run tests**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure -R "AI Move Parser"
```
Expected: all parser token/condition/tactic tests pass. The `parse_move_array` tests will still fail — that comes next.

**Step 4: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 4: Implement `ai_parse_move_array`

**Files:**
- Modify: `src/game/ai/ai_skills_config_loader.c`

**Step 1: Understand the existing JSON helpers available in this file**

The file already has:
- `find_matching_bracket(start)` — finds the closing `]` for an array
- `json_array_entry_count(json, field_name)` — counts `{` at depth 1 within a named array

New helpers needed:
- `find_json_array(json, key, start_out, end_out)` — locates `[...]` for a named key
- `find_next_object(cursor, end, obj_start_out, obj_end_out)` — iterates objects in an array
- `json_extract_string(obj_start, obj_end, key, buf, buf_size)` — extracts a string value
- `json_extract_string_array(obj_start, obj_end, key, cb, userdata)` — iterates a string array

**Step 2: Add helper: `find_json_array`**

```c
static bool find_json_array(const char *json, const char *key,
                             const char **arr_start_out, const char **arr_end_out) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *field = strstr(json, search);
    if(field == NULL) return false;

    const char *colon = strchr(field + strlen(search), ':');
    if(colon == NULL) return false;

    const char *arr = strchr(colon + 1, '[');
    if(arr == NULL) return false;

    const char *end = find_matching_bracket(arr);
    if(end == NULL) return false;

    *arr_start_out = arr;
    *arr_end_out = end;
    return true;
}
```

**Step 3: Add helper: `find_next_object`**

Walks an array looking for the next `{...}` object at depth 1 of the array.

```c
static bool find_next_object(const char *cursor, const char *arr_end,
                              const char **obj_start_out, const char **obj_end_out) {
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    const char *obj_start = NULL;

    for(const char *p = cursor; p < arr_end; p++) {
        char c = *p;
        if(in_string) {
            if(escaped) { escaped = false; continue; }
            if(c == '\\') { escaped = true; continue; }
            if(c == '"') { in_string = false; }
            continue;
        }
        if(c == '"') { in_string = true; continue; }
        if(c == '{') {
            if(depth == 0) { obj_start = p; }
            depth++;
        } else if(c == '}') {
            depth--;
            if(depth == 0 && obj_start != NULL) {
                *obj_start_out = obj_start;
                *obj_end_out = p;
                return true;
            }
        }
    }
    return false;
}
```

**Step 4: Add helper: `json_extract_string`**

```c
static bool json_extract_string(const char *obj_start, const char *obj_end,
                                 const char *key, char *buf, size_t buf_size) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *field = obj_start;
    while(field < obj_end) {
        field = strstr(field, search);
        if(field == NULL || field >= obj_end) return false;
        field += strlen(search);

        // skip whitespace and colon
        while(field < obj_end && (*field == ' ' || *field == '\t' || *field == ':')) field++;
        if(field >= obj_end || *field != '"') {
            continue; // might be a key substring match; keep searching
        }

        field++; // skip opening quote
        size_t i = 0;
        bool esc = false;
        while(field < obj_end && i < buf_size - 1) {
            char c = *field++;
            if(esc) { buf[i++] = c; esc = false; continue; }
            if(c == '\\') { esc = true; continue; }
            if(c == '"') break;
            buf[i++] = c;
        }
        buf[i] = '\0';
        return true;
    }
    return false;
}
```

**Step 5: Add helper: `json_iterate_string_array`**

```c
typedef void (*string_cb_t)(const char *str, void *userdata);

static void json_iterate_string_array(const char *obj_start, const char *obj_end,
                                       const char *key, string_cb_t cb, void *userdata) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *field = strstr(obj_start, search);
    if(field == NULL || field >= obj_end) return;
    field += strlen(search);

    const char *arr = strchr(field, '[');
    if(arr == NULL || arr >= obj_end) return;
    const char *arr_end = find_matching_bracket(arr);
    if(arr_end == NULL || arr_end > obj_end) return;

    bool in_string = false;
    bool escaped = false;
    char token_buf[64];
    size_t tok_i = 0;

    for(const char *p = arr + 1; p < arr_end; p++) {
        char c = *p;
        if(!in_string) {
            if(c == '"') { in_string = true; tok_i = 0; }
            continue;
        }
        if(escaped) { if(tok_i < sizeof(token_buf) - 1) token_buf[tok_i++] = c; escaped = false; continue; }
        if(c == '\\') { escaped = true; continue; }
        if(c == '"') {
            token_buf[tok_i] = '\0';
            cb(token_buf, userdata);
            in_string = false;
            tok_i = 0;
            continue;
        }
        if(tok_i < sizeof(token_buf) - 1) token_buf[tok_i++] = c;
    }
}
```

**Step 6: Add `parse_range_string` helper**

```c
static ai_move_range parse_range_string(const char *s) {
    if(s == NULL || strcmp(s, "ANY") == 0) return MOVE_RANGE_ANY;
    if(strcmp(s, "CLOSE") == 0) return MOVE_RANGE_CLOSE;
    if(strcmp(s, "MID")   == 0) return MOVE_RANGE_MID;
    if(strcmp(s, "FAR")   == 0) return MOVE_RANGE_FAR;
    return MOVE_RANGE_ANY;
}
```

**Step 7: Implement `ai_parse_move_array`**

```c
bool ai_parse_move_array(const char *json, const char *key,
                          ai_move_def *out, uint8_t *out_count, uint8_t max_count) {
    if(json == NULL || key == NULL || out == NULL || out_count == NULL) return false;

    const char *arr_start = NULL, *arr_end = NULL;
    if(!find_json_array(json, key, &arr_start, &arr_end)) {
        return false;
    }

    *out_count = 0;
    const char *cursor = arr_start + 1;

    while(*out_count < max_count) {
        const char *obj_start = NULL, *obj_end = NULL;
        if(!find_next_object(cursor, arr_end, &obj_start, &obj_end)) break;
        cursor = obj_end + 1;

        ai_move_def *def = &out[*out_count];
        memset(def, 0, sizeof(ai_move_def));
        def->range_max = MOVE_RANGE_FAR; // default: no max constraint

        // name
        json_extract_string(obj_start, obj_end, "name", def->name, AI_MOVE_NAME_LEN);

        // range_min
        char range_buf[16] = {0};
        if(json_extract_string(obj_start, obj_end, "range_min", range_buf, sizeof(range_buf))) {
            def->range_min = parse_range_string(range_buf);
        }

        // range_max
        char range_max_buf[16] = {0};
        if(json_extract_string(obj_start, obj_end, "range_max", range_max_buf, sizeof(range_max_buf))) {
            def->range_max = parse_range_string(range_max_buf);
        }

        // conditions
        typedef struct { ai_move_def *def; } cond_ctx;
        cond_ctx cctx = {def};
        json_iterate_string_array(obj_start, obj_end, "conditions",
            [](const char *s, void *ud) { /* C99 workaround: use static function instead */ },
            &cctx);
        // NOTE: C99 doesn't have lambdas; use a named callback:
        // (see actual implementation below using a named static function)

        // follow_up_tactics
        // (same pattern)

        // sequence
        // (same pattern)

        (*out_count)++;
    }

    return true;
}
```

> **Implementation note:** Since C99 has no lambdas, use named static callback functions:

```c
typedef struct {
    ai_move_def *def;
} condition_parse_ctx;

static void condition_cb(const char *s, void *ud) {
    condition_parse_ctx *ctx = (condition_parse_ctx *)ud;
    ai_move_condition c = ai_parse_condition_string(s);
    ctx->def->conditions = (ai_move_condition)(ctx->def->conditions | c);
}

typedef struct {
    ai_move_def *def;
} tactic_parse_ctx;

static void tactic_cb(const char *s, void *ud) {
    tactic_parse_ctx *ctx = (tactic_parse_ctx *)ud;
    if(ctx->def->follow_up_tactic_count >= AI_MOVE_MAX_FOLLOW_TACTICS) return;
    int t = ai_parse_tactic_string(s);
    if(t >= 0) {
        ctx->def->follow_up_tactics[ctx->def->follow_up_tactic_count++] = t;
    }
}

typedef struct {
    ai_move_def *def;
} sequence_parse_ctx;

static void sequence_cb(const char *s, void *ud) {
    sequence_parse_ctx *ctx = (sequence_parse_ctx *)ud;
    if(ctx->def->input_count >= AI_MOVE_MAX_INPUTS) return;
    int bits = 0;
    if(ai_parse_sequence_token(s, &bits)) {
        ctx->def->inputs[ctx->def->input_count++] = bits;
    }
}
```

Then in `ai_parse_move_array`, replace the lambda placeholders with these callbacks.

**Step 8: Build and run parser tests**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure -R "AI Move Parser"
```
Expected: all 37 parser tests pass.

**Step 9: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 5: Wire `ai_parse_move_array` into the load path

**Files:**
- Modify: `src/game/ai/ai_skills_config_loader.c`

**Step 1: Call `ai_parse_move_array` in `load_har_config`**

After the block that sets `has_*` and `*_move_count` from the JSON (around line 234), add:

```c
    ai_parse_move_array(json, "charge_moves",
                        g_configs[har_id].charge_moves,
                        &g_configs[har_id].charge_move_count,
                        AI_MAX_MOVES_PER_TYPE);
    ai_parse_move_array(json, "push_moves",
                        g_configs[har_id].push_moves,
                        &g_configs[har_id].push_move_count,
                        AI_MAX_MOVES_PER_TYPE);
    ai_parse_move_array(json, "projectile_moves",
                        g_configs[har_id].projectile_moves,
                        &g_configs[har_id].projectile_move_count,
                        AI_MAX_MOVES_PER_TYPE);
    // Counts are now authoritative from the parser; update flags
    g_configs[har_id].has_charge_moves     = g_configs[har_id].charge_move_count > 0;
    g_configs[har_id].has_push_moves       = g_configs[har_id].push_move_count > 0;
    g_configs[har_id].has_projectile_moves = g_configs[har_id].projectile_move_count > 0;
```

(Remove the now-redundant `json_array_entry_count` calls for the three arrays since the parser's `out_count` supersedes them.)

**Step 2: Also call in `ai_skills_config_apply_overlay`**

Replace the three `json_array_entry_count` blocks in `ai_skills_config_apply_overlay` with:

```c
    if(strstr(json_buf, "\"charge_moves\"") != NULL) {
        ai_parse_move_array(json_buf, "charge_moves",
                            cfg->charge_moves, &cfg->charge_move_count,
                            AI_MAX_MOVES_PER_TYPE);
        cfg->has_charge_moves = cfg->charge_move_count > 0;
        changed = true;
    }
    // ... same for push_moves and projectile_moves
```

**Step 3: Build and run all tests**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```
Expected: all tests pass (no regressions).

**Step 4: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 6: Update all 11 HAR JSON files

**Files:**
- Modify: `resources/ai_config/hars/chronos.json`
- Modify: `resources/ai_config/hars/electra.json`
- Modify: `resources/ai_config/hars/flail.json`
- Modify: `resources/ai_config/hars/gargoyle.json`
- Modify: `resources/ai_config/hars/jaguar.json`
- Modify: `resources/ai_config/hars/katana.json`
- Modify: `resources/ai_config/hars/nova.json`
- Modify: `resources/ai_config/hars/pyros.json`
- Modify: `resources/ai_config/hars/shadow.json`
- Modify: `resources/ai_config/hars/shredder.json`
- Modify: `resources/ai_config/hars/thorn.json`

**Summary of changes** (apply each file):

`chronos.json` — teleportation: `"high_difficulty"` → `"diff_scale"`, add `"follow_up_tactics": ["grab", "push", "shoot", "spam", "trip"]`; stasis: rename condition `"enemy_not_stunned_or_stasis"` → `"enemy_not_stunned"`.

`electra.json` — super_rolling_thunder: `"high_difficulty"` → `"diff_scale"`.

`flail.json` — shadow_punch: `"high_difficulty"` → `"diff_scale"`; slow_swing_chains: add `"conditions": ["random_third"]`.

`gargoyle.json` — wing_charge: `"high_difficulty"` → `"diff_scale"`; shadow_talon: `"high_difficulty"` → `"diff_scale"`.

`jaguar.json` — shadow_leap: `"high_difficulty"` → `"diff_scale"`.

`katana.json` — trip_slide: `["low_preferred"]` → `["random_half", "low_preferred"]`; forward_razor_spin: add `"conditions": ["random_half"]`; both triple_blade entries: `"high_difficulty"` → `"diff_scale"`.

`nova.json` — earthquake_slam: `["high_difficulty"]` → `["diff_scale"]`; mini_grenade: prepend `"D"` to sequence, add `"conditions": ["random_third"]`; missile: prepend `"D"` to sequence.

`pyros.json` — shadow_thrust: `"high_difficulty"` → `"diff_scale"`.

`shadow.json` — split shadow_projectile `"P|K"` into two entries:
```json
{
  "name": "shadow_projectile_punch",
  "sequence": ["D", "DB", "B", "P"],
  "range_min": "ANY",
  "conditions": ["random_half"]
},
{
  "name": "shadow_projectile_kick",
  "sequence": ["D", "DB", "B", "K"],
  "range_min": "ANY"
}
```

`shredder.json` — flip_kick: `"high_difficulty"` → `"diff_scale"`; shadow_head_butt: `"high_difficulty"` → `"diff_scale"`. (flying_hands already has `"range_max": "MID"` — no change needed.)

`thorn.json` — shadow_kick: `"high_difficulty"` → `"diff_scale"`.

**Step 1: Apply all changes**

Edit each file as described above.

**Step 2: Run tests to confirm parse round-trip**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure -R "AI HAR Skills"
```
Expected: all existing `ai_har_skills` tests still pass (count/flag tests use the new parser path now).

**Step 3: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 7: Write executor tests (TDD)

**Files:**
- Create: `testing/ai/ai_move_executor_test.c`
- Modify: `testing/test_main.c`

The executor tests need lightweight stubs for `controller`, `object`, `har`, and `ai`. Use the patterns already established in `ai_har_skills_test.c` (which stubs these for null-controller guard tests).

```c
/**
 * Tests for the data-driven ai_char_execute_move_list executor.
 */

#include <CUnit/CUnit.h>
#include <string.h>

#include "controller/controller.h"
#include "game/ai/ai_skills_config_loader.h"
#include "game/ai/ai_har_skills.h"

// Expose internal executor function for testing
extern bool ai_char_execute_move_list(controller *ctrl, const object *o,
                                       const ai *a,
                                       const ai_move_def *moves, uint8_t count,
                                       int enemy_range, ctrl_event **ev);

// Expose direction-resolution helper
extern int ai_resolve_input(int input, object_direction dir);

/* ---- direction resolution tests ---- */

void test_resolve_F_facing_right(void) {
    // F (ACT_RIGHT) stays ACT_RIGHT when facing right
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_RIGHT, OBJECT_FACE_RIGHT), ACT_RIGHT);
}

void test_resolve_F_facing_left(void) {
    // F (ACT_RIGHT canonical) flips to ACT_LEFT when facing left
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_RIGHT, OBJECT_FACE_LEFT), ACT_LEFT);
}

void test_resolve_B_facing_right(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_LEFT, OBJECT_FACE_RIGHT), ACT_LEFT);
}

void test_resolve_B_facing_left(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_LEFT, OBJECT_FACE_LEFT), ACT_RIGHT);
}

void test_resolve_DB_facing_left(void) {
    // DB = ACT_DOWN|ACT_LEFT: flips to ACT_DOWN|ACT_RIGHT
    int resolved = ai_resolve_input(ACT_DOWN | ACT_LEFT, OBJECT_FACE_LEFT);
    CU_ASSERT_EQUAL(resolved, ACT_DOWN | ACT_RIGHT);
}

void test_resolve_down_no_flip(void) {
    // ACT_DOWN has no horizontal component — unchanged
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_DOWN, OBJECT_FACE_LEFT), ACT_DOWN);
}

void test_resolve_punch_no_flip(void) {
    CU_ASSERT_EQUAL(ai_resolve_input(ACT_PUNCH, OBJECT_FACE_LEFT), ACT_PUNCH);
}

void ai_move_executor_test_suite(CU_pSuite suite) {
    if(CU_add_test(suite, "resolve: F right-facing", test_resolve_F_facing_right) == NULL) return;
    if(CU_add_test(suite, "resolve: F left-facing flips", test_resolve_F_facing_left) == NULL) return;
    if(CU_add_test(suite, "resolve: B right-facing", test_resolve_B_facing_right) == NULL) return;
    if(CU_add_test(suite, "resolve: B left-facing flips", test_resolve_B_facing_left) == NULL) return;
    if(CU_add_test(suite, "resolve: DB left-facing flips", test_resolve_DB_facing_left) == NULL) return;
    if(CU_add_test(suite, "resolve: down unchanged", test_resolve_down_no_flip) == NULL) return;
    if(CU_add_test(suite, "resolve: punch unchanged", test_resolve_punch_no_flip) == NULL) return;
}
```

> **Note:** The full `ai_char_execute_move_list` integration tests (condition evaluation, selection logic, follow_up_tactics) require a game-state harness that is complex to mock. These are covered indirectly by the existing record-file regression tests in `rectests/`. Add only the direction-resolution unit tests here. Condition evaluation is exercised via the JSON round-trip and integration tests.

Register in `test_main.c` (same pattern as Task 2 Step 2, suite name `"AI Move Executor"`).

**Build and run:**
```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure -R "AI Move Executor"
```
Expected: direction-resolution tests fail (symbol undefined). Implement next.

---

### Task 8: Implement `ai_resolve_input` and `ai_char_execute_move_list`

**Files:**
- Modify: `src/game/ai/ai_har_skills.c`

**Step 1: Add the direction-resolution helper (non-static for tests)**

```c
int ai_resolve_input(int input, object_direction dir) {
    if(dir == OBJECT_FACE_LEFT) {
        int has_left  = input & ACT_LEFT;
        int has_right = input & ACT_RIGHT;
        input &= ~(ACT_LEFT | ACT_RIGHT);
        if(has_left)  input |= ACT_RIGHT;
        if(has_right) input |= ACT_LEFT;
    }
    return input;
}
```

**Step 2: Add condition evaluator (static)**

```c
static bool eval_conditions(controller *ctrl, const ai *a,
                             ai_move_condition cond, int enemy_range) {
    if(cond == MOVE_COND_NONE) return true;

    if((cond & MOVE_COND_DIFF_SCALE)        && !diff_scale(a))                  return false;
    if((cond & MOVE_COND_SPECIAL_PREF)      && !roll_pref(a->pilot->ap_special)) return false;
    if((cond & MOVE_COND_LOW_PREFERRED)     && !roll_pref(a->pilot->ap_low))     return false;
    if((cond & MOVE_COND_JUMP_PREFERRED)    && !roll_pref(a->pilot->att_jump))   return false;
    if((cond & MOVE_COND_RANDOM_HALF)       && !roll_chance(2))                  return false;
    if((cond & MOVE_COND_RANDOM_THIRD)      && !roll_chance(3))                  return false;
    if((cond & MOVE_COND_ENEMY_NOT_STUNNED) && enemy_is_stunned_or_stasis(ctrl)) return false;

    return true;
}
```

**Step 3: Implement `ai_char_execute_move_list`**

```c
bool ai_char_execute_move_list(controller *ctrl, const object *o,
                                const ai *a,
                                const ai_move_def *moves, uint8_t count,
                                int enemy_range, ctrl_event **ev) {
    for(uint8_t i = 0; i < count; i++) {
        const ai_move_def *m = &moves[i];

        if(enemy_range < (int)m->range_min) continue;
        if(enemy_range > (int)m->range_max) continue;
        if(!eval_conditions(ctrl, a, m->conditions, enemy_range)) continue;

        for(uint8_t j = 0; j < m->input_count; j++) {
            int resolved = ai_resolve_input(m->inputs[j], o->direction);
            controller_cmd(ctrl, resolved, ev);
        }

        if(m->follow_up_tactic_count > 0) {
            ai_tactic_consider_list(ctrl,
                                    (int *)m->follow_up_tactics,
                                    m->follow_up_tactic_count);
        }

        return true;
    }
    return false;
}
```

**Step 4: Build and run executor tests**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure -R "AI Move Executor"
```
Expected: all direction-resolution tests pass.

**Step 5: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 9: Replace switch cases in `ai_char_execute_charge`

**Files:**
- Modify: `src/game/ai/ai_har_skills.c`

**Step 1: Replace the entire switch block**

Remove everything from `int enemy_range = get_enemy_range(ctrl);` down to the closing `}` of `ai_char_execute_charge`, and replace with:

```c
    int enemy_range = get_enemy_range(ctrl);

    if(ai_char_execute_move_list(ctrl, o, a,
                                  char_cfg->charge_moves,
                                  char_cfg->charge_move_count,
                                  enemy_range, ev)) {
        return true;
    }

    return false;
```

**Step 2: Build and run full test suite + regression tests**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```
Expected: all tests pass. If regression tests exist for charge moves, verify them.

**Step 3: User commit checkpoint**

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 10: Replace switch cases in `ai_char_execute_push`

**Files:**
- Modify: `src/game/ai/ai_har_skills.c`

Same pattern as Task 9. Remove the `int enemy_range` line and the switch, replace with:

```c
    int enemy_range = get_enemy_range(ctrl);

    if(ai_char_execute_move_list(ctrl, o, a,
                                  char_cfg->push_moves,
                                  char_cfg->push_move_count,
                                  enemy_range, ev)) {
        return true;
    }

    return false;
```

Build and test:
```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```

---

### Task 11: Replace switch cases in `ai_char_execute_projectile`

**Files:**
- Modify: `src/game/ai/ai_har_skills.c`

Remove the `int enemy_range` line, the `if(h->state == ...)` stop-guard, and the switch. Replace with:

```c
    int enemy_range = get_enemy_range(ctrl);

    if(h->state == STATE_WALKTO || h->state == STATE_WALKFROM || h->state == STATE_CROUCHBLOCK) {
        controller_cmd(ctrl, ACT_STOP, ev);
    }

    if(ai_char_execute_move_list(ctrl, o, a,
                                  char_cfg->projectile_moves,
                                  char_cfg->projectile_move_count,
                                  enemy_range, ev)) {
        return true;
    }

    return false;
```

Build and test:
```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```

---

### Task 12: Expose `ai_char_execute_move_list` and `ai_resolve_input` in the header

**Files:**
- Modify: `src/game/ai/ai_har_skills.h`

Add declarations:
```c
bool ai_char_execute_move_list(controller *ctrl, const object *o,
                                const ai *a,
                                const ai_move_def *moves, uint8_t count,
                                int enemy_range, ctrl_event **ev);

int ai_resolve_input(int input, object_direction dir);
```

Also add the required includes for `object`, `ai`, and `ai_move_def` if not already present.

Build and test:
```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```

---

### Task 13: Remove obsolete executor tests from `ai_har_skills_test.c`

**Files:**
- Modify: `testing/ai/ai_har_skills_test.c`

Remove the per-HAR switch-case executor tests. Keep:
- All `ai_skills_config_*` flag/count tests (they still validate the load path)
- The null-controller guard tests for `execute_charge/push/trip/projectile`

The per-HAR charge/push/projectile execution tests (if any) can be removed since the executor is now validated by the record-file regression tests in `rectests/`.

Build and run full suite:
```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```
Expected: all tests pass, test count is same or reduced.

User commit checkpoint:

User handles commits manually. No `git add`/`git commit` commands are included in this plan.

---

### Task 14: Final validation

**Step 1: Full build + test run**

```bash
cmake --build build -j4 && ctest --test-dir build --output-on-failure
```
Expected: all tests green.

**Step 2: Run record-file regression tests**

```bash
./run_rectests.sh
```
Expected: all `.REC` files in `rectests/` replay correctly.

**Step 3: Optional local history check**

```bash
git log --oneline -10
```

---

## Appendix: `ai_move_range` to `enemy_range` mapping

| `ai_move_range` value | `range_min` check | `range_max` check |
|-----------------------|-------------------|-------------------|
| `MOVE_RANGE_ANY` (0)  | always passes     | `range_max=FAR` → always passes |
| `MOVE_RANGE_CLOSE`    | `enemy_range >= 1`| `enemy_range <= 1` |
| `MOVE_RANGE_MID`      | `enemy_range >= 2`| `enemy_range <= 2` |
| `MOVE_RANGE_FAR`      | `enemy_range >= 3`| `enemy_range <= 3` |

The `RANGE_*` enum in `ai_utils.h`: `CRAMPED=0, CLOSE=1, MID=2, FAR=3`. Cast `ai_move_range` to `int` for the comparison.
