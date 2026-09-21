# Difficulty-Moddable AI System Implementation Plan

> Superseded by [2026-09-20-difficulty-config-and-pilot-stat-alignment.md](2026-09-20-difficulty-config-and-pilot-stat-alignment.md).
>
> The current implementation already keeps the active fields as `block_chance`, `aggressive_tactics`, and `jump_frequency_mult`, while removing the stale `movement_distance_pref` / `passive_defense` concepts from the final config model.

**Goal:** Allow AI behavior to be fully configurable per difficulty level, so PUNCHING BAG is passive while ULTIMATE is aggressive, with complete modding flexibility.

**Architecture:** Three-layer config cascade:
1. Load `ai_core.ini` base defaults
2. Load difficulty-specific `ai_difficulty/[difficulty].ini` (overwrites matching keys)
3. HAR JSON supports per-difficulty move overrides (future enhancement, schema preparation)

**Tech Stack:** C config parser, INI file format, JSON schema extension

---

## Task 1: Extend ai_core_config_t struct

**File:** `src/game/ai/ai_core_config.h`

Add new behavior parameters to track all configurable values. Current struct has ~18 fields; add placeholders for block frequency, passive defense, movement preferences.

**What to do:**
- Add fields: `block_chance`, `passive_defense`, `aggressive_tactics`, `movement_distance_pref`, `jump_frequency_mult`
- Document each field with comment explaining its purpose
- No code changes yet, just struct expansion

**Verify:**
- File compiles with `make -j$(nproc)` (will fail until ai_core_config.c initializes these)

---

## Task 2: Update ai_core_config.c defaults

**File:** `src/game/ai/ai_core_config.c`

Initialize new struct fields with safe defaults in `ai_core_config_load()`.

**What to do:**
- Find `ai_core_config_load()` function
- Add default initializations for new fields (e.g., `config->block_chance = 10;`)
- These are overridden by .ini files, so reasonable defaults are fine

**Verify:**
- Builds clean: `make -j$(nproc) 2>&1 | grep -E "error|warning"`

---

## Task 3: Extend ai_core_config.c parser

**File:** `src/game/ai/ai_core_config.c`

Add parsing for new parameters in `ai_core_config_load()`.

**What to do:**
- Find the `parse_int_line()` calls that read from .ini
- Add new `parse_int_line()` calls for each new parameter
- Follow exact pattern: `parse_int_line(trimmed, "block_chance", &config->block_chance);`

**Verify:**
- Builds clean
- Function still loads existing parameters correctly

---

## Task 4: Create difficulty config directory

**Shell:**

```bash
mkdir -p /home/sharnw/dev/openomf/resources/ai_config/ai_difficulty
```

---

## Task 5: Create 7 difficulty-specific config files

**Files:** Create all of these in `resources/ai_config/ai_difficulty/`:
- `punching_bag.ini`
- `rookie.ini`
- `veteran.ini`
- `world_class.ini`
- `champion.ini`
- `deadly.ini`
- `ultimate.ini`

**Design pattern:** Each file only includes parameters that differ from base. For example:

**`punching_bag.ini`:**
```ini
# Punching Bag - Very passive, just defends
base_act_chance = 25
base_fwd_jump_chance = 60
base_back_jump_chance = 60
base_still_jump_chance = 90
random_attack_chance = 60
base_act_timer = 50
block_chance = 70
passive_defense = 1
aggressive_tactics = 0
```

**`rookie.ini`:**
```ini
# Rookie - Below average, defensive
base_act_chance = 15
base_fwd_jump_chance = 40
base_back_jump_chance = 45
base_still_jump_chance = 70
random_attack_chance = 35
base_act_timer = 32
block_chance = 40
passive_defense = 0
aggressive_tactics = 0
```

**`veteran.ini`:**
```ini
# Veteran - Balanced
base_act_chance = 8
base_fwd_jump_chance = 20
base_back_jump_chance = 25
base_still_jump_chance = 50
random_attack_chance = 15
base_act_timer = 28
block_chance = 15
passive_defense = 0
aggressive_tactics = 0
```

**`world_class.ini`:**
```ini
# World Class - Aggressive
base_act_chance = 4
base_fwd_jump_chance = 8
base_back_jump_chance = 12
base_still_jump_chance = 35
random_attack_chance = 8
base_act_timer = 24
block_chance = 5
passive_defense = 0
aggressive_tactics = 1
```

**`champion.ini`:**
```ini
# Champion - Very aggressive
base_act_chance = 2
base_fwd_jump_chance = 3
base_back_jump_chance = 5
base_still_jump_chance = 25
random_attack_chance = 4
base_act_timer = 20
block_chance = 2
passive_defense = 0
aggressive_tactics = 1
```

**`deadly.ini`:**
```ini
# Deadly - Expert tactics, constant pressure
base_act_chance = 1
base_fwd_jump_chance = 2
base_back_jump_chance = 3
base_still_jump_chance = 15
random_attack_chance = 2
base_act_timer = 18
block_chance = 1
passive_defense = 0
aggressive_tactics = 1
```

**`ultimate.ini`:**
```ini
# Ultimate - Maximum aggression, relentless
base_act_chance = 1
base_fwd_jump_chance = 1
base_back_jump_chance = 2
base_still_jump_chance = 10
random_attack_chance = 1
base_act_timer = 16
block_chance = 1
passive_defense = 0
aggressive_tactics = 1
```

---

## Task 6: Extend ai_core_config_load() to cascade-load difficulty files

**File:** `src/game/ai/ai_core_config.c`

Modify `ai_core_config_load()` to load base defaults, then overlay difficulty-specific config.

**What to do:**
- After loading `ai_core.ini`, construct path like `resources/ai_config/ai_difficulty/veteran.ini`
- Pass difficulty level (0-6 enum) to function
- Call same parser again on difficulty file (it will overwrite matching keys only)
- Handle missing difficulty file gracefully (just use base defaults)

**Signature change:**
```c
bool ai_core_config_load(ai_core_config_t *config, int difficulty_level)
```

**Verify:**
- Builds clean
- Existing calls to `ai_core_config_load()` need to pass difficulty level

---

## Task 7: Find and update all ai_core_config_load() call sites

**File:** `src/game/ai/ai_core_config.c` (initialization)

Search for places calling `ai_core_config_load()` and pass correct difficulty level.

**What to do:**
```bash
grep -r "ai_core_config_load" src/ --include="*.c"
```

Update each call to pass the AI's current difficulty level. Typically in:
- AI initialization when controller is created
- Maybe in settings/menu code

**Verify:**
- All calls compile
- Build succeeds: `make -j$(nproc)`

---

## Task 8: Update ai_controller.c to use config params directly

**File:** `src/controller/ai_controller.c`

Replace hardcoded `diff_scale()` rolls with config-driven parameters.

**Current pattern (remove this):**
```c
if(a->act_timer <= 0 && (roll_chance(ai_core_config_get()->base_act_chance) || diff_scale(a))) {
```

**New pattern (replace with):**
```c
if(a->act_timer <= 0 && roll_chance(ai_core_config_get()->base_act_chance)) {
```

The difficulty-specific config already encodes the behavior. `diff_scale()` rolls are redundant.

**What to do:**
- Find all `|| diff_scale(a)` in ai_controller.c
- Remove them (config file already does difficulty scaling)
- Find all `if(diff_scale(a))` and evaluate: some might stay for high-difficulty special tactics

**Verify:**
- Builds clean
- Behavior tests still pass (or improve)

---

## Task 9: Create HAR JSON schema documentation

**File:** Create `../../AI_DIFFICULTY_OVERRIDES.md`

Document the optional difficulty override schema for future use (prepare schema, don't implement yet).

**Contents:**
```markdown
# AI Difficulty Overrides (HAR JSON Schema)

## Optional: Per-Difficulty Move Configuration

Each HAR JSON can include difficulty-specific move overrides:

\`\`\`json
{
  "id": 0,
  "name": "chronos",
  "charge_moves": [...],
  "difficulty_overrides": {
    "rookie": {
      "charge_moves": [
        { "name": "spike_charge", "conditions": ["low_difficulty"] }
      ]
    },
    "champion": {
      "charge_moves": [...all available...]
    }
  }
}
\`\`\`

When difficulty_overrides[difficulty] exists, use those moves instead of base.
```

---

## Task 10: Build and test

**What to do:**
```bash
cd /home/sharnw/dev/openomf/build
make clean && make -j$(nproc)
```

**Verify:**
- Builds successfully
- No compile errors
- No new warnings

---

## Task 11: Smoke test difficulty changes

**Manual test (in-game or headless if possible):**

Create a test HAR match at each difficulty level and observe:
- PUNCHING BAG: Should stand still, block often, rarely jump
- ROOKIE: Should move less, attack less
- VETERAN: Normal behavior (current base)
- CHAMPION: Constant motion, aggressive attacks
- ULTIMATE: Relentless pressure

**If headless testing available:**
```bash
./run_pytest.sh pytest/test_move_triggers.py
```

---

## Task 12: Documentation

**File:** `../../AI_DIFFICULTY_CONFIG.md`

Write user-facing documentation:
- How to modify difficulty behavior (edit .ini files)
- Parameter meanings
- Examples for custom pilots
- How modders can create new difficulty levels

---

## Commits (Suggested Pattern)

1. "feat: extend ai_core_config_t with difficulty-specific parameters"
2. "feat: add cascade loader for difficulty config files"
3. "feat: create difficulty-specific ini config files (punching_bag through ultimate)"
4. "refactor: use config params directly instead of diff_scale() rolls in ai_controller"
5. "docs: add AI difficulty overrides schema and modding guide"

---

## Known Unknowns

- [ ] Are there other places using `diff_scale()` that should be updated?
- [ ] Do HAR per-difficulty move overrides need immediate implementation, or is difficulty config enough?
- [ ] Should block frequency use its own rolls, or integrate into move selection system?

---

## Success Criteria

✅ PUNCHING BAG is noticeably passive (blocks/stands still frequently)
✅ ULTIMATE is noticeably aggressive (moves/attacks constantly)
✅ Each difficulty level between is progressively more aggressive
✅ Build passes with no errors
✅ No regression in existing behavior at VETERAN difficulty
