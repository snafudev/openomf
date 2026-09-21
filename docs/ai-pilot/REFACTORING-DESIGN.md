# AI Controller Refactoring Design (Approach C)

**Status**: Design Phase  
**Date**: 2026-06-12  
**Target**: Make AI system modular, testable, and config-driven  
**Scope**: Internal refactoring only (public API unchanged)

AI config loading should support a base layer from `resources/ai_config/*` and an extension layer from `src/resources/modmanager.c/h` so mods can extend/override AI config deterministically.

---

## Current Architecture Analysis

### File Size & Structure
- **File**: `src/controller/ai_controller.c` (2875 lines)
- **Header**: `src/controller/ai_controller.h` (3 public functions only)
- **Current organization**: Monolithic with clear but tightly coupled layers

### Code Patterns Identified

#### 1. **Difficulty-Based Decision Rolls** (~100 lines)
```
smart_usually(), dumb_usually(), smart_sometimes(), dumb_sometimes(), diff_scale()
```
- ✅ **Can extract**: Pure functions, no state mutation, simple input→output
- **Dependencies**: Only `ai.difficulty` field
- **Location target**: `ai_decision_engine.c`

#### 2. **Pilot Preference Evaluation** (~80 lines)
```
roll_pref(), learning_moment(), forgetful()
```
- ✅ **Can extract**: Pure functions
- **Dependencies**: `sd_pilot` fields, random number generator
- **Location target**: `ai_decision_engine.c`

#### 3. **Tactical Decision Logic** (~300 lines)
```
likes_tactic(), chain_consider_tactics(), queue_tactic()
```
- ⚠️ **Partially extractable**: Has many conditional branches, but core decision logic is testable
- **Tight coupling**: Directly mutates `tactic_state`, reads `har *h`, `ai *a`
- **Issue**: 11 tactics with different condition chains - needs abstraction
- **Location target**: `ai_tactic_engine.c`
- **Refactoring needed**: Extract tactic predicates into registry

#### 4. **HAR-Specific Move Execution** (~600 lines)
```
attempt_charge_attack(), attempt_push_attack(), attempt_projectile_attack(), attempt_trip_attack()
```
- ⚠️ **Cannot easily extract as-is**: Each HAR has hard-coded move sequences
- **Current pattern**: 10-switch statement per function, huge case blocks
- **Issue**: Tightly coupled to `controller *ctrl` and controller_cmd() calls
- **Problem to solve**: Need HAR skill registry
- **Location target**: `ai_har_skills.c` (generic executor + data registry)
- **Config needed**: Character move data should load from config

#### 5. **Event Handling & Tactic Response** (~400 lines)
```
ai_har_event() - massive switch statement
```
- ❌ **Strongly coupled**: Directly mutates state, nested conditions, many edge cases
- **Deep dependencies**: Reads game state, pilot personality, move stats
- **Issue**: 10 event types × 3-5 response chains each = exponential complexity
- **Cannot fully abstract**: Game events are external, must handle in controller
- **Location target**: Keep in `ai_controller.c` but simplify via event bus
- **Refactoring needed**: Route events to decision engine, return action

#### 6. **Movement Execution** (~100 lines)
```
handle_movement()
```
- ✅ **Can extract**: Pure movement logic
- **Minimal coupling**: Only reads `o->pos`, `h->close`, `h->is_wallhugging`
- **Location target**: `ai_movement.c`

#### 7. **Move Selection & Evaluation** (~150 lines)
```
assign_move_by_cat(), assign_move_by_id(), attempt_attack(), is_valid_move()
```
- ⚠️ **Can extract with state object**: Not pure (reads move stats), but logic is clear
- **Dependencies**: `move_stat[]` array, `af_move` data, HAR capabilities
- **Location target**: `ai_move_selector.c`

#### 8. **State Management** (~50 lines)
```
reset_tactic_state(), reset_pilot_personality(), reset_act_timer()
```
- ✅ **Can extract**: Simple initialization/reset logic
- **Location target**: `ai_state.c`

#### 9. **Utility Helpers** (~50 lines)
```
get_enemy_range(), enemy_is_stunned_or_stasis(), har_has_projectiles(), char_to_act()
```
- ✅ **Can extract**: Pure helpers with clear purpose
- **Location target**: `ai_utils.c`

#### 10. **Polling Loop** (`ai_controller_poll()` - ~80 lines)
```
Main orchestration: block → movement → tactic → random attack
```
- ⚠️ **Heavily interdependent**: Coordinates all subsystems
- **Issue**: Must manage control flow (early returns block further execution)
- **Refactoring**: Abstract into state machine? Or keep as clear orchestrator?
- **Location target**: Stay in `ai_controller.c` but call cleaner subsystem APIs

---

## Dependencies Map

### What Dependencies Exist

#### External (Cannot change):
```
controller →  game_state, har, object, pilot
game_state → arena, scene, player
har → object, af_data (move definitions)
```

#### Internal (Can refactor):
```
ai_controller_poll() 
  ├→ handle_movement()
  ├→ ai_block_har() / ai_block_projectile()
  ├→ process_selected_move()
  ├→ handle_queued_tactic()
  │   ├→ attempt_charge_attack()
  │   ├→ attempt_push_attack()
  │   ├→ attempt_trip_attack()
  │   ├→ attempt_projectile_attack()
  │   └→ attempt_attack()
  ├→ ai_har_event() [event handler]
  │   ├→ queue_tactic()
  │   └→ chain_consider_tactics()
  └→ Global move_stat[] array
```

### State Mutations During Execution

**Critical issue**: State is mutated in multiple places:
- `tactic_state` (queued during `likes_tactic()`, mutated in `handle_queued_tactic()`)
- `move_stat[]` (updated in `attempt_attack()`, `ai_har_event()`)
- `ai.pilot` personality (modified in learning moments)
- `ai.blocked`, `ai.thrown`, `ai.shot` (counters)
- `ai.selected_move`, `ai.move_str_pos` (execution state)

**Problem**: Makes testing difficult, multiple code paths can fail silently

---

## What CAN Be Broken Into Smaller Parts

### ✅ High-Confidence Extractions

1. **Decision Engine Module** (`ai_decision_engine.c/h`)
   - Pure functions: `smart_usually()`, `dumb_usually()`, `diff_scale()`, etc.
   - No state mutation
   - Easy to unit test
   - ~150 LOC

2. **Movement Module** (`ai_movement.c/h`)
   - Pure movement direction selection
   - No move execution (that stays in executor)
   - ~80 LOC

3. **Utilities Module** (`ai_utils.c/h`)
   - Range calculation, state checks
   - HAR capability lookups
   - ~100 LOC

4. **State Module** (`ai_state.c/h`)
   - Initialization functions
   - Reset/cleanup logic
   - ~50 LOC

### ⚠️ Medium-Confidence Extractions (Require Refactoring)

5. **Move Selector Module** (`ai_move_selector.c/h`)
   - Move validation logic
   - Move evaluation/scoring
   - **Requires**: Passing move_stat context, not mutating global state
   - ~150 LOC
   - **Interface**: `move_selector_eval(moves[], move_stats[], criteria) → best_move`

6. **Tactic Engine Module** (`ai_tactic_engine.c/h`)
   - Tactic predicates (why AI likes each tactic)
   - Tactic queue setup (what moves/movements get assigned)
   - **Requires**: Extracting tactic definitions into registry
   - **Current problem**: 11 tactics with custom logic in `likes_tactic()` switch
   - **Solution**: Create `tactic_predicate_t` function pointers + metadata
   - ~250 LOC

### ❌ Cannot Easily Extract

7. **Event Handler** (`ai_har_event()`)
   - **Why**: 10 event types, each with 3-5 response chains, deeply nested
   - **Problem**: Must handle context-dependent decisions ("did tactic fail?", "is this a learning moment?")
   - **Cannot test in isolation**: Requires full game state
   - **Solution**: Keep in `ai_controller.c`, refactor as calls to decision engine
   - **Mitigation**: Extract decision predicates, call from cleaner event routing

8. **Blocking Logic** (`ai_block_har()`, `ai_block_projectile()`)
   - **Why**: Timing-sensitive, depends on game animation state
   - **Problem**: Must poll projectile positions, predict moves
   - **Solution**: Keep in main controller, simplify via helpers

---

## Config-Driven Behavior

### Moddable Elements

These should be **loadable from config files** (`resources/ai_config/`):

1. **Pilot Personalities** (currently in `reset_pilot_personality()`)
   - JSON: `pilots.json`
   ```json
   {
     "crystal": {
       "att_normal": 30, "att_hyper": 10, "att_jump": 10,
       "learning": 1.5, "forget": 0.25
     }
   }
   ```
   - ✅ Easy to extract
   - ✅ Enables tweaking without recompile
   - Impact: High (pilots define playstyle)

2. **Tactic Weights** (implicit in `likes_tactic()` logic)
   - JSON: `tactics.json`
   ```json
   {
     "TACTIC_GRAB": {
       "cooldown": 5,
       "difficulty_min": 1,
       "range": "CRAMPED",
       "conditions": ["not_thrown_recently", "enemy_close"]
     }
   }
   ```
   - ⚠️ Moderate extraction (encoded in conditionals)
   - ✅ Enables balance tweaking
   - Impact: Very High (gameplay feel)

3. **Difficulty Scales**
   - YAML: `difficulty.yaml`
   ```yaml
   difficulty:
     1: { act_chance: 5, jump_chance: 40, smartness: 0 }
     6: { act_chance: 7, jump_chance: 15, smartness: 0.9 }
   ```
   - ✅ Easy to extract
   - Impact: High (AI aggression/skill)

4. **Character Move Data** (currently hard-coded in `attempt_charge_attack()`)
   - JSON: `characters/jaguar.json`
   ```json
   {
     "charge_moves": [
       { "name": "Leap", "sequence": ["D", "F", "P"], "range": "MID" }
     ],
     "push_moves": [
       { "name": "High Kick", "sequence": ["B", "K"] }
     ]
   }
   ```
   - ❌ Hard extraction (move sequences are tightly coupled to controller_cmd calls)
   - ✅ Worth extracting (enables modding new characters)
   - Impact: Very High (character feel)

### Non-Moddable (Hardcoded Business Logic)

These should stay in code:
- Event response priorities (what to do when hit)
- Learning algorithms (how AI adapts)
- Range/state validation (safety checks)

### Modmanager Overlay Model

AI config loading should follow this order:
1. Load base files from `resources/ai_config/*`.
2. Apply mod overlays discovered by modmanager in deterministic mod load order.

Constraints:
- No-mod behavior must be identical to base-only behavior.
- Missing keys in mod overlays must fall back to base values.
- Conflicting keys across mods must follow documented precedence.
- Invalid mod overlay payloads should warn and continue with safe fallback.

---

## Module Boundary Definitions

### New File Structure

```
src/controller/
└── ai_controller.c          # [2875 → 500] Main orchestrator, event handler (STAYS HERE)
└── ai_controller.h          # PUBLIC: unchanged (STAYS HERE)

src/game/ai/
├── ai_decision_engine.c/h   # [~150] Pure decision functions
├── ai_movement.c/h          # [~80] Movement selection
├── ai_move_selector.c/h     # [~150] Move evaluation & selection
├── ai_tactic_engine.c/h     # [~250] Tactic logic (refactored)
├── ai_har_skills.c/h  # [~400] Character-specific moves (from attempt_*_attack)
├── ai_state.c/h             # [~50] State initialization
├── ai_utils.c/h             # [~100] Helper functions
│
└── ai_config/               # [NEW] Configuration loading
    ├── ai_config.c/h        # Config manager
    ├── ai_tactics_config.c  # Tactic registry
    └── ai_skills_config.c   # Character skills registry

src/resources/
└── modmanager.c/h           # Mod overlay source and load-order authority

resources/
└── ai_config/               # [NEW] Data files
    ├── pilots.json          # Pilot personalities
    ├── tactics.json         # Tactic metadata
    ├── difficulty.yaml      # Difficulty scales
    └── characters/
        ├── jaguar.json      # Jaguar moves
        ├── shadow.json      # Shadow moves
        └── ...
```

---

## Dependency Rules (To Enforce)

### Module Dependencies (Acyclic)

```
ai_utils, ai_decision_engine      [leaf - no dependencies]
  ↑
ai_movement, ai_state             [leaf]
  ↑
ai_move_selector                   [depends on utils, decision]
  ↑
ai_tactic_engine                   [depends on utils, decision, move_selector]
  ↑
ai_har_skills                [depends on utils]
  ↑
ai_config                          [depends on all above, loads data]
   ↑
modmanager overlay lookup          [extends base config with mod-provided overlays]
  ↑
ai_controller.c                    [orchestrates all, main entry point]
```

### External Dependencies (Allowed)

All modules can depend on:
- `game/game_state.h`, `game/objects/har.h`, `game/objects/object.h`
- `formats/pilot.h`
- `resources/af_loader.h`
- `utils/log.h`, `utils/random.h`, etc.

### External Dependencies (NOT Allowed)

No module should:
- Call `controller_cmd()` except `ai_controller.c` (orchestrator only)
- Mutate `controller *ctrl` state
- Create/free HAR objects
- Access game physics directly

---

## Key Dependencies & Constraints

### Hard to Break Apart

1. **Move Execution Loop** (`process_selected_move()`)
   - Must call `controller_cmd()` in sequence
   - Tightly coupled to move string parsing
   - **Keep**: In `ai_controller.c`, small focused function

2. **Event Cascading** (`ai_har_event()`)
   - One event can queue multiple tactics
   - Tactics can chain other tactics on hit
   - **Mitigation**: Extract tactic consideration into engine, but keep routing in controller

3. **Timing State** (input lag, act timer, move timers)
   - All interrelated
   - **Mitigation**: Create `ai_timing.c` helper if extracted

4. **Move Statistics Learning**
   - Updated during attack selection AND event handling
   - **Issue**: Move stats are global per HAR, hard to test in isolation
   - **Solution**: Pass `move_stat_context` through function calls, not globals

### Can Be Cleanly Separated

1. **Decision predicates** (smart_usually → should I do X?)
   - Pure functions, easy to unit test
   - No side effects

2. **Move validation** (is_valid_move)
   - State-independent checks
   - Easy to mock game state

3. **Configuration loading**
   - Separate from runtime logic
   - Can validate schemas independently

---

## Breaking Points (Where to Cut)

### Phase 1: Extract Decision Engine (EASY)
- **What**: `smart_usually()`, `dumb_usually()`, `roll_pref()`, etc.
- **How**: New `ai_decision_engine.c`, pure functions
- **Risk**: None (no existing code calls these)
- **Tests**: 20+ unit tests (all paths for each difficulty)

### Phase 2: Extract Utilities (EASY)
- **What**: `get_enemy_range()`, `har_has_projectiles()`, `char_to_act()`, etc.
- **How**: New `ai_utils.c`, small pure helpers
- **Risk**: Low (well-defined contracts)
- **Tests**: 15+ unit tests

### Phase 3: Refactor Movement (MEDIUM)
- **What**: `handle_movement()` logic extraction
- **How**: New `ai_movement.c`, parameter-ize HAR state
- **Risk**: Medium (timing sensitive, must maintain exact behavior)
- **Tests**: 10+ tests with mocked game state

### Phase 4: Extract HAR Skills (HARD)
- **What**: `attempt_charge_attack()`, `attempt_push_attack()`, etc.
- **How**: New `ai_har_skills.c` + config loader
- **Risk**: High (complex move sequences, hard-coded per-character)
- **Requires**: Designing skill registry, testing all 10 HARs
- **Tests**: 50+ tests (per-HAR move validation)

### Phase 5: Refactor Tactic Engine (HARD)
- **What**: `likes_tactic()`, `queue_tactic()` logic
- **How**: New `ai_tactic_engine.c` with tactic predicates
- **Risk**: High (complex conditional logic, 11 tactics × game state combinations)
- **Tests**: 100+ tests (all tactic paths)

### Phase 6: HAR Skills Extraction (HIGH)
- **What**: Complete generic per-HAR skill execution with config-driven data
- **How**: `ai_har_skills.c/h` + `ai_skills_config_loader.c/h`
- **Risk**: High (10 HAR behavior parity)
- **Tests**: 50+ tests (per-HAR behavior)

### Phase 7: Modmanager Config Integration (MEDIUM)
- **What**: Integrate AI config lookup loading with modmanager overlays
- **How**: Extend `ai_config` loaders to merge base config with mod overlays
- **Risk**: Medium (load-order/fallback semantics)
- **Tests**: 20+ tests (overlay order, fallback, conflict handling)

### Phase 8: Refactor Event Handler (MEDIUM)
- **What**: `ai_har_event()` simplification
- **How**: Route to decision engine, extract logic into helper functions
- **Risk**: Medium (many edge cases, must verify behavior unchanged)
- **Tests**: 30+ tests (all event types, all tactic states)

---

## Testing Strategy

### Unit Tests (by module)

1. **ai_decision_engine_test.c** (20 tests)
   - Each decision predicate (smart_usually, dumb_usually, etc.)
   - All difficulty levels
   - All pilot preference scenarios

2. **ai_movement_test.c** (10 tests)
   - Direction selection based on enemy position
   - Pilot preferences affecting movement
   - Difficulty scaling

3. **ai_move_selector_test.c** (20 tests)
   - Move validation
   - Move scoring (damage, learning, prefer/dislike)
   - Category filtering

4. **ai_tactic_engine_test.c** (100+ tests)
   - Each tactic's conditions (all 11 tactics)
   - Range/state combinations
   - Tactic chaining

5. **ai_har_skills_test.c** (50+ tests)
   - Each HAR's charge moves
   - Each HAR's push moves
   - Each HAR's projectile moves
   - All move sequences execute without error

6. **ai_controller_test.c** (30+ tests)
   - Poll loop flow (block → tactic → attack)
   - Event handling (HAR events → tactic response)
   - State transitions (tactic start → finish)

### Integration Tests

- **scenario_test.c**: Simulated 1v1 matches, verify AI doesn't crash
- **config_test.c**: Load all config files, verify no parse errors
- **regression_test.c**: Record AI decisions from old code, verify new code matches (within randomness)

### Coverage Goals

- **Decision engine**: 100% coverage (pure functions)
- **Utilities**: 100% coverage (small, well-defined)
- **Move selector**: 95%+ coverage (some edge cases hard to hit)
- **Tactic engine**: 90%+ coverage (11 tactics, many combinations)
- **Overall**: 85%+ coverage

---

## What Cannot Be Easily Extracted

### 1. Event Handler Complexity
- **Function**: `ai_har_event()` (400 lines)
- **Reason**: 10 event types × 3-5 response chains each
- **Problem**: Each event can change game state, affect other events
- **Example**: `HAR_EVENT_LAND` must check if tactic's attack should trigger NOW, or queue for later
- **Solution**: Keep in main controller, extract decision helpers, clean up routing
- **Cannot fully modularize** because:
  - Event priorities are game-design decisions
  - Responses depend on current tactic state
  - Learning logic is cross-cutting concern

### 2. Hard-Coded Character Moves
- **Functions**: `attempt_charge_attack()`, `attempt_push_attack()` (600 lines)
- **Reason**: Each HAR has unique move sequences
- **Problem**: Moves are hard-coded switch statements with manual `controller_cmd()` calls
- **Example**:
  ```c
  case HAR_JAGUAR:
    int cmds[] = {BACK, DOWNBACK};
    chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
  ```
- **Why hard to extract**:
  - Move sequences are not data (they're control flow)
  - Conditional logic for move selection (when to use Leap vs other moves)
  - Difficulty/preference scaling affects which variant to use
- **Solution**: Extract move data to config, but keep selection logic in code
- **Can modularize**: Move registry (what moves exist), but not move execution (how moves trigger)

### 3. Move Statistics Learning
- **Data**: `move_stat move_stats[70]`
- **Problem**: Updated in 3 places:
  - `set_selected_move()` (track distance)
  - `attempt_attack()` (evaluation scoring)
  - `ai_har_event()` (on hit/block feedback)
- **Issue**: Statistics are global per HAR, passed as references
- **Why hard to extract**:
  - Learning is cross-cutting (happens during move selection AND event handling)
  - Must maintain 70 stats per HAR
  - Cannot easily mock learning for tests
- **Solution**: Create `move_stats_context` object, pass through function calls
- **Can partially modularize**: Statistics update logic, but not initialization

### 4. Controller Command Buffering
- **Function**: `process_selected_move()`, `controller_cmd()` calls
- **Problem**: Must maintain sequence of button presses
- **Why hard to extract**:
  - Timing is critical (input lag, move string position)
  - Each tick must send exactly one command
  - Commands are queued into event system
- **Solution**: Keep in main controller, abstract into `move_executor` that main loop calls

### 5. Pilot Personality Adaptation
- **Function**: `reset_pilot_personality()` + learning in `ai_har_event()`
- **Problem**: Hard-coded for 11 pilots, each with custom values
- **Why hard to extract completely**:
  - Needs to load from config AND reset in code
  - Learning algorithm is baked in event handler
  - Each pilot has 15+ personality traits
- **Solution**: Load base personalities from config, keep learning algorithm in code

---

## Summary: What CAN vs CANNOT Be Broken Apart

### ✅ EASY TO EXTRACT (Pure, No Coupling)
- Decision rolls (smart_usually, dumb_usually) → `ai_decision_engine.c`
- Movement selection → `ai_movement.c`
- Utility helpers → `ai_utils.c`
- State initialization → `ai_state.c`

### ⚠️ EXTRACT WITH REFACTORING (Logic + Data Coupling)
- Move selection/evaluation → `ai_move_selector.c` (need move_stats context)
- Tactic logic → `ai_tactic_engine.c` (need tactic registry)
- Character skills → `ai_har_skills.c` + config (keep selection logic)
- Event routing → Simplify in `ai_controller.c` (call helpers)

### ❌ KEEP IN MAIN CONTROLLER (Event, Timing, Control Flow)
- `ai_controller_poll()` orchestration
- `ai_har_event()` routing (simplify, not extract)
- `process_selected_move()` (timing critical)
- Pilot learning (cross-cutting concern)
- Event cascading (dependencies)

