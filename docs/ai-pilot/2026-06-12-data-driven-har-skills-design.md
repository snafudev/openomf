# Data-Driven HAR Skill Execution — Design

**Date:** 2026-06-12  
**Status:** Approved

## Overview

Replace the hardcoded per-HAR switch cases in `ai_har_skills.c` with a generic executor
driven by move definitions parsed from the existing `resources/ai_config/hars/<name>.json`
files. Modders can add, remove, or change AI move sequences and conditions entirely in JSON
without touching C code.

---

## 1. Data Structures

### `ai_move_condition` (bitmask enum)

All conditions in a move's bitmask must pass for the move to be selected.

```c
typedef enum {
    MOVE_COND_NONE              = 0,
    MOVE_COND_DIFF_SCALE        = 1 << 0,  // diff_scale(a)
    MOVE_COND_SPECIAL_PREF      = 1 << 1,  // roll_pref(ap_special)
    MOVE_COND_LOW_PREFERRED     = 1 << 2,  // roll_pref(ap_low)
    MOVE_COND_JUMP_PREFERRED    = 1 << 3,  // roll_pref(att_jump)
    MOVE_COND_RANDOM_HALF       = 1 << 4,  // roll_chance(2)
    MOVE_COND_RANDOM_THIRD      = 1 << 5,  // roll_chance(3)
    MOVE_COND_ENEMY_NOT_STUNNED = 1 << 6,  // !enemy_is_stunned_or_stasis
} ai_move_condition;
```

### `ai_move_range` (enum)

`range_min` is interpreted as `enemy_range >= value`. `MOVE_RANGE_ANY` always passes.

```c
typedef enum {
    MOVE_RANGE_ANY = 0,
    MOVE_RANGE_CLOSE,
    MOVE_RANGE_MID,
    MOVE_RANGE_FAR,
} ai_move_range;
```

### `ai_move_def` (struct)

Parsed at load time; stored in `ai_char_config`.

```c
#define AI_MOVE_MAX_INPUTS       8
#define AI_MOVE_MAX_FOLLOW_TACTICS 8
#define AI_MOVE_NAME_LEN         32

typedef struct {
    char     name[AI_MOVE_NAME_LEN];
    int      inputs[AI_MOVE_MAX_INPUTS];       // ACT_* bitmasks, stored right-facing
    uint8_t  input_count;
    ai_move_range    range_min;
    ai_move_condition conditions;              // bitmask — ALL must pass
    int      follow_up_tactics[AI_MOVE_MAX_FOLLOW_TACTICS];
    uint8_t  follow_up_tactic_count;
} ai_move_def;
```

Horizontal direction tokens (`F`, `B`, `DF`, `DB`) are stored in their right-facing form
(`ACT_RIGHT`, `ACT_LEFT`, etc.) and flipped at execute time based on `o->direction` by
inverting `ACT_LEFT`↔`ACT_RIGHT` bits.

### `ai_char_config` additions

```c
#define AI_MAX_MOVES_PER_TYPE 8

// added to ai_char_config:
ai_move_def charge_moves[AI_MAX_MOVES_PER_TYPE];
ai_move_def push_moves[AI_MAX_MOVES_PER_TYPE];
ai_move_def projectile_moves[AI_MAX_MOVES_PER_TYPE];
```

The existing `charge_move_count`, `push_move_count`, `projectile_move_count` uint8_t fields
are reused as the populated entry counts for the above arrays.

---

## 2. JSON Schema

Each HAR JSON file gains fully populated move arrays. Example (`chronos.json`):

```json
{
  "id": 9,
  "name": "chronos",
  "charge_moves": [
    {
      "name": "chronos_charge_special",
      "sequence": ["D", "D+P"],
      "range_min": "MID",
      "conditions": ["special_preferred", "diff_scale"],
      "follow_up_tactics": ["grab", "push", "shoot", "spam", "trip"]
    },
    {
      "name": "chronos_charge_db_kick",
      "sequence": ["DB+K"],
      "range_min": "ANY"
    }
  ]
}
```

### Sequence token reference

| Token   | ACT_* value (right-facing canonical) |
|---------|--------------------------------------|
| `F`     | `ACT_RIGHT`                          |
| `B`     | `ACT_LEFT`                           |
| `D`     | `ACT_DOWN`                           |
| `U`     | `ACT_UP`                             |
| `DF`    | `ACT_DOWN\|ACT_RIGHT`                |
| `DB`    | `ACT_DOWN\|ACT_LEFT`                 |
| `5`     | `ACT_STOP`                           |
| `P`     | `ACT_PUNCH`                          |
| `K`     | `ACT_KICK`                           |

Tokens may be combined with `+` (e.g. `"DB+K"` → `ACT_DOWN|ACT_LEFT|ACT_KICK`).

### Condition string reference

| String              | Condition flag              |
|---------------------|-----------------------------|
| `diff_scale`        | `MOVE_COND_DIFF_SCALE`      |
| `special_preferred` | `MOVE_COND_SPECIAL_PREF`    |
| `low_preferred`     | `MOVE_COND_LOW_PREFERRED`   |
| `jump_preferred`    | `MOVE_COND_JUMP_PREFERRED`  |
| `random_half`       | `MOVE_COND_RANDOM_HALF`     |
| `random_third`      | `MOVE_COND_RANDOM_THIRD`    |
| `enemy_not_stunned` | `MOVE_COND_ENEMY_NOT_STUNNED` |

### Follow-up tactic string reference

Tactic strings map to the existing `TACTIC_*` enum values (e.g. `"grab"` → `TACTIC_GRAB`).

---

## 3. Execution Flow

The three `ai_char_execute_*` functions are replaced by a single generic executor called
with the appropriate move list:

```
ai_char_execute_move_list(ctrl, o, a, moves, count, enemy_range, ev):
    for i in 0..count:
        move = moves[i]
        if enemy_range < move.range_min: continue
        if not eval_conditions(ctrl, a, move.conditions): continue
        emit chain_resolved_inputs(ctrl, o, move, ev)
        if move.follow_up_tactic_count > 0:
            ai_tactic_consider_list(ctrl, move.follow_up_tactics, move.follow_up_tactic_count)
        return true
    return false
```

**Direction resolution** in `chain_resolved_inputs`: for each input, if `o->direction ==
OBJECT_FACE_LEFT`, swap `ACT_LEFT`↔`ACT_RIGHT` bits before passing to `controller_cmd`.

**`ai_char_execute_charge/push/projectile`** become thin wrappers that:
1. Guard/validate (null checks, `can_start_ground_attack`, `has_*` flag)
2. Call `ai_char_execute_move_list` with the relevant array from `char_cfg`

---

## 4. Parser

A new internal function in `ai_skills_config_loader.c`:

```
parse_move_array(json, key, out_moves, out_count)
```

- Extracts the named array from the JSON string
- For each object entry: parses `sequence`, `range_min`, `conditions`, `follow_up_tactics`
- Token parsing splits on `+`, maps each part via a lookup table
- Unknown tokens or conditions are logged as warnings and skipped (not fatal)
- Called from `ai_skills_config_load_har` after the count is established
- Also called from `ai_skills_config_apply_overlay` when arrays are present in an overlay

---

## 5. Testing

### New test files / additions

1. **`testing/ai/ai_move_parser_test.c`** — unit tests for `parse_move_array`:
   - Token-to-ACT mapping for each token including `+` combinations
   - Condition string parsing, all 7 flags, combined bitmask
   - `follow_up_tactics` string-to-enum parsing
   - Unknown token/condition is skipped without crash
   - Empty array produces count=0

2. **`testing/ai/ai_move_executor_test.c`** — executor integration tests:
   - First qualifying move is selected when multiple pass
   - Move is skipped when `range_min` not met
   - Move is skipped when any condition fails
   - `follow_up_tactics` triggers `ai_tactic_consider_list` (via mock/spy)
   - Direction resolution: `F` emits `ACT_RIGHT` facing right, `ACT_LEFT` facing left

3. **`testing/ai/ai_har_skills_test.c`** — existing flag/count tests retained;
   per-HAR switch-case tests removed and replaced by executor tests above.

4. **JSON round-trip tests** — load `jaguar.json`, `chronos.json`, `nova.json` and assert
   parsed `ai_move_def` structs match expected values (sequence inputs, range, conditions,
   follow-up tactics).

---

## 6. Files Changed

| File | Change |
|------|--------|
| `src/game/ai/ai_skills_config_loader.h` | Add `ai_move_condition`, `ai_move_range`, `ai_move_def` types; extend `ai_char_config` |
| `src/game/ai/ai_skills_config_loader.c` | Add `parse_move_array`; populate move arrays in load + overlay paths |
| `src/game/ai/ai_har_skills.c` | Replace switch cases with `ai_char_execute_move_list`; thin wrappers remain |
| `src/game/ai/ai_har_skills.h` | No signature changes expected |
| `resources/ai_config/hars/*.json` | Update `conditions` strings; add `follow_up_tactics` to Chronos charge move; ensure sequences are complete for all HARs |
| `testing/ai/ai_move_parser_test.c` | New |
| `testing/ai/ai_move_executor_test.c` | New |
| `testing/ai/ai_har_skills_test.c` | Remove obsolete switch-case tests |
| `testing/CMakeLists.txt` | Register new test files |
