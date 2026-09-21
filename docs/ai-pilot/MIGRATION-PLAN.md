# AI Controller Migration Plan (Approach C)

**Target**: Modular, testable, config-driven AI system  
**Phases**: 8 sequential phases  
**Estimated timeline**: 5-7 weeks (depends on test coverage goals)  
**Breaking changes**: None (public API stays identical)  
**⚠️ IMPORTANT**: Do not automatically commit during refactoring. Follow the commit messages provided in each phase exactly. This ensures proper history and clear documentation of changes.

---

## Phase 1: Foundation & Decision Engine
**Duration**: 3-4 days  
**Effort**: Easy  
**Risk**: None (new code, no dependencies)

### Goals
- Create test infrastructure
- Extract decision predicates (pure functions)
- Verify all decisions work with old AI

### Deliverables

1. **Create test framework**
   - File: `testing/ai/ai_controller_test.h` (test helpers)
   - Provides mock game state, controller, HAR objects
   - Fixture for running AI in isolated environment

2. **Extract `src/game/ai/ai_decision_engine.c/h`**
   - Functions: `smart_usually()`, `dumb_usually()`, `smart_sometimes()`, `dumb_sometimes()`, `diff_scale()`, `learning_moment()`, `forgetful()`, `roll_chance()`, `roll_pref()`
   - Tests: `testing/ai/ai_decision_engine_test.c` (20 unit tests)
   - Verify: Each difficulty level, all random branches

3. **Update src/controller/ai_controller.c**
   - Replace inline calls to decision functions with `ai_decision_engine` calls
   - No behavioral change (same random seed = same sequence)

### Verification
```bash
# Run AI in standalone mode, verify identical decisions
testing/ai/regression_test --old-vs-new --seed=12345
```

### Commit Message
```
AI refactor - Phase 1: Extract decision engine from ai_controller
* Move pure decision functions to ai_decision_engine.c
* Add 20 unit tests for decision rolls
* Maintain identical behavior with same random seed
```

---

## Phase 2: Utilities & Helpers
**Duration**: 2-3 days  
**Effort**: Easy  
**Risk**: Low (small, well-defined)

### Goals
- Extract utility functions
- Improve testability of range/state checks
- Prepare for config-driven character data

### Deliverables

1. **Extract `src/game/ai/ai_utils.c/h`**
   - Functions:
     - `get_enemy_range()` - distance classification
     - `enemy_is_stunned_or_stasis()` - state check
     - `is_special_move()` - move filtering
     - `har_has_projectiles()`, `har_has_charge()`, `har_has_push()` - HAR capability checks
     - `char_to_act()` - move string to controller action
   - Tests: `testing/ai/ai_utils_test.c` (15 unit tests)

2. **Extract `src/game/ai/ai_state.c/h`**
   - Functions: `reset_tactic_state()`, `reset_act_timer()`, `reset_pilot_personality()`
   - Tests: `testing/ai/ai_state_test.c` (5 unit tests)

3. **Update src/controller/ai_controller.c**
   - Replace inline utility calls with module calls
   - No behavioral change

### Verification
- All tests pass
- AI behavior identical to Phase 1

### Commit Message
```
AI refactor - Phase 2: Extract utility functions from ai_controller
* Move range/state checks to ai_utils.c
* Move initialization logic to ai_state.c
* Add 20 unit tests
```

---

## Phase 3: Movement Module
**Duration**: 3-4 days  
**Effort**: Medium  
**Risk**: Medium (timing sensitive)

### Goals
- Extract movement selection logic
- Parameterize HAR state
- Create testable movement predicates

### Deliverables

1. **Extract `src/game/ai/ai_movement.c/h`**
   - Functions:
     - `ai_movement_decide()` - select forward/back/still based on state
     - `ai_movement_jump_chance()` - calculate jump likelihood
   - Takes parameters: enemy range, pilot prefs, HAR ID, difficulty
   - Returns: movement direction enum
   - Tests: `testing/ai/ai_movement_test.c` (10 unit tests)

2. **Update `handle_movement()` in src/controller/ai_controller.c**
   - Calls `ai_movement_decide()` instead of inline logic
   - Maintains exact same controller commands
   - Same jump probability with same seed

3. **Verify behavior unchanged**
   - Record AI movements for 100 ticks with seed
   - Compare with Phase 2 version
   - Must be identical

### Verification
```bash
# Record movement sequences
testing/ai/movement_test --record --seed=54321 > movement.baseline

# Run new code, verify identical
testing/ai/movement_test --verify --seed=54321 --compare=movement.baseline
```

### Commit Message
```
AI refactor - Phase 3: Extract movement selection to ai_movement.c
* Create testable movement decision functions
* Parameterize HAR state instead of direct access
* Add 10 unit tests
```

---

## Phase 4: Move Selector Module
**Duration**: 4-5 days  
**Effort**: Medium  
**Risk**: Low (logic is clear, high test coverage possible)

### Goals
- Modularize move evaluation logic
- Separate move validation from selection
- Prepare for config-driven move data

### Deliverables

1. **Extract `src/game/ai/ai_move_selector.c/h`**
   - Functions:
     - `ai_move_is_valid()` - validity checks (replaces `is_valid_move()`)
     - `ai_move_eval_score()` - compute move value (learning, damage, preference)
     - `ai_move_select_best()` - pick best move from list
   - Input: Move array, move_stats array, scoring context
   - Output: Selected move or NULL
   - No mutation of global state (returns results)
   - Tests: `testing/ai/ai_move_selector_test.c` (25 unit tests)

2. **Update `assign_move_by_cat()` and `attempt_attack()` in src/controller/ai_controller.c**
   - Use `ai_move_select_best()` instead of inline loops
   - Pass move_stat context explicitly
   - Maintain exact same move selection

3. **Behavior verification**
   - Same random seed = same move selections
   - Test against "golden" move sequences from Phase 3

### Verification
```bash
# Verify move selections match previous phase
testing/ai/move_selector_test --verify --seed=11111 --compare=baseline
```

### Commit Message
```
AI refactor - Phase 4: Extract move selection logic to ai_move_selector.c
* Separate move validation from scoring
* Make move evaluation testable
* Add 25 unit tests
```

---

## Phase 5: Tactic Engine & Configuration
**Duration**: 5-7 days  
**Effort**: Hard  
**Risk**: High (complex logic, 11 different tactics)

### Current Status (2026-06-12)
- [x] `pilots.json` extraction and config-first personality loading with fallback defaults.
- [x] `ai_tactic_engine.c/h` extraction and integration into AI controller wrappers.
- [x] `tactics.json` enabled/disabled registry with safe partial-config semantics.
- [x] Tactic metadata parsing for `move_type` and `attack_type`.
- [x] Metadata token support checks with warning diagnostics for unsupported tokens.
- [x] Parse optional tactic `conditions` arrays and expose query APIs for condition tokens.
- [x] Validate condition tokens and warn for unsupported condition metadata.
- [x] Apply first-pass condition gating in `ai_tactic_likes_it()` when tactic conditions are present.
- [x] Add deterministic runtime condition-matching API and direct tests for condition-gated behavior.
- [x] Start declarative migration in `ai_tactic_likes_it()` by making `TACTIC_SHOOT` condition-aware (no duplicate gates when equivalent condition tokens are configured).
- [x] Extend declarative migration by making `TACTIC_CLOSE` condition-aware for `has_charge`, `pref_hyper`, and `enemy_not_cramped`.
- [x] Complete condition-aware de-dup across all `ai_tactic_likes_it()` tactic branches with legacy-fallback semantics preserved.
- [x] Add deterministic condition-matcher coverage that spans all 11 tactics (`enemy_not_cramped` gating baseline).
- [x] Add branch-specific deterministic condition tests for `has_push`, `not_shot_too_much`, and `not_thrown_too_much`.
- [x] Expanded `testing/ai/ai_tactic_engine_test.c` coverage for enabled flags, metadata parsing, support checks, and unknown-token fallback safety.
- [x] Continue migrating queue behavior from legacy switch branches to declarative metadata rules (without regressions).
- [x] Broaden tactic-engine test depth toward the original 100+ target in this phase.

### Phase 5 Coverage Snapshot (2026-06-12)
- Phase 5 implementation status: complete.
- Current dedicated tactic-engine tests: 24 (`testing/ai/ai_tactic_engine_test.c`).
- Current emphasis: config parsing/safety semantics and token support guardrails.
- Next emphasis: handoff to Phase 6 (HAR skills module) while preserving Phase 5 regression coverage.

### Goals
- Modularize tactic decision logic
- Create tactic registry/metadata
- Load pilot personalities from config

### Deliverables

1. **Create config directory**
   ```
   resources/ai_config/
   ├── pilots.json
   ├── tactics.json
   ├── difficulty.yaml
   └── characters/
       ├── jaguar.json
       ├── shadow.json
       └── ...
   ```

2. **Extract pilot personalities to `resources/ai_config/pilots.json`**
   - Remove `reset_pilot_personality()` hard-coded switch statement
   - JSON schema:
     ```json
     {
       "pilots": {
         "crystal": {
           "id": 0,
           "att_normal": 30,
           "att_hyper": 10,
           "learning": 1.5,
           "forget": 0.25
         }
       }
     }
     ```
   - Create `ai_config_loader.c` to parse JSON

3. **Extract tactic metadata to `resources/ai_config/tactics.json`**
   - Schema:
     ```json
     {
       "tactics": {
         "GRAB": {
           "id": 1,
           "conditions": ["not_thrown_too_much", "pref_hyper"],
           "move_type": "MOVE_CLOSE",
           "attack_type": "ATTACK_GRAB"
         }
       }
     }
     ```

4. **Extract `ai_tactic_engine.c/h`**
   - Functions:
     - `ai_tactic_likes_it()` - evaluate if AI would use tactic (uses registry)
     - `ai_tactic_queue()` - set up tactic state
     - `ai_tactic_consider_list()` - iterate tactics, pick best
   - Progressive replacement of hard-coded switch logic with config-driven mapping + safe fallback
   - Tests: `testing/ai/ai_tactic_engine_test.c` (100+ unit tests)

5. **Update `ai_har_event()` and `chain_consider_tactics()`**
   - Use `ai_tactic_engine` instead of inline logic
   - Maintain exact same tactic selections

6. **Load config on AI creation**
   - `ai_controller_create()` calls `ai_config_load_pilots()`
   - Graceful fallback to defaults if config missing

### Verification
- Tactic sequences match Phase 4 (same seed)
- Config file validation (missing pilots error handling)
- 100 unit tests covering all tactic paths

### Commit Message
```
AI refactor - Phase 5: Extract tactic engine and load pilot config
* Create ai_tactic_engine.c with registry-based tactics
* Load pilot personalities from resources/ai_config/pilots.json
* Add 100+ unit tests for tactic logic
```

---

## Phase 6: HAR Skills Module
**Duration**: 6-8 days  
**Effort**: Hard  
**Risk**: High (per-HAR testing, 10 HARs × 3 move types each)

### Current Status (2026-06-12)
- [x] Added `resources/ai_config/hars/*.json` for all HARs.
- [x] Added `src/game/ai/ai_har_skills.c/h` with charge, push, projectile, and trip executors.
- [x] Added `src/game/ai/ai_skills_config_loader.c/h` with per-HAR lazy load + cache semantics.
- [x] Updated `attempt_*_attack()` functions in `src/controller/ai_controller.c` to route through config lookup + generic executors.
- [x] Added initial Phase 6 unit tests in `testing/ai/ai_har_skills_test.c` and registered suite in `testing/test_main.c`.
- [x] Expand dedicated per-HAR behavior tests toward the original 50+ target.

### Phase 6 Coverage Snapshot (2026-06-12)
- Phase 6 implementation status: complete for module extraction, config loading, controller integration, and expanded per-HAR parity coverage.
- Current dedicated character-skills tests: 120.
- Next emphasis: handoff to Phase 7 modmanager-backed config extension integration.

### Goals
- Modularize character-specific moves
- Extract move data to config
- Enable adding new characters without code changes

### Deliverables

1. **Create character config files**
   - `resources/ai_config/hars/jaguar.json`
   ```json
   {
     "id": 0,
     "charge_moves": [
       {
         "name": "Shadow Leap",
         "sequence": ["B", "D", "F", "P"],
         "range_min": "MID",
         "conditions": ["special_prefered", "high_difficulty"]
       },
       {
         "name": "Jaguar Leap",
         "sequence": ["D", "F", "P"],
         "range_min": "ANY"
       }
     ],
     "push_moves": [...],
     "projectile_moves": [...]
   }
   ```
   - All 10 HARs get config files

2. **Extract `ai_har_skills.c/h`**
   - Functions:
     - `ai_har_execute_charge()` - generic charge executor
     - `ai_har_execute_push()` - generic push executor
     - `ai_har_execute_projectile()` - generic projectile executor
     - `ai_har_execute_trip()` - generic trip executor
   - Takes: `har_id`, `char_moves_config`, `context` (range, difficulty, etc.)
   - Returns: Success boolean
   - Tests: `tests/controller/ai_har_skills_test.c` (50+ unit tests)

3. **Load character configs**
   - Create `ai_skills_config_loader.c`
   - Load on first AI creation for each HAR
   - Cache in module-level registry

4. **Update attempt_*_attack() functions**
   - Replace 10-way switch statements with config lookup + executor call
   - Maintain exact same move sequences (same seed)
   - Example:
     ```c
     bool attempt_charge_attack(controller *ctrl, ctrl_event **ev) {
       ai *a = ctrl->data;
       har *h = object_get_userdata(...);
       ai_har_config *char_cfg = ai_skills_config_get(h->id);
       return ai_har_execute_charge(ctrl, char_cfg, ev);
     }
     ```

5. **Per-HAR testing**
   - Test all charge moves for each HAR (10 × ~5 = 50 tests)
   - Test all push moves for each HAR (10 × ~3 = 30 tests)
   - Verify sequences match old code

### Verification
- Record move sequences for all 10 HARs (old code)
- Run new code, verify identical
- All 50+ tests pass

### Commit Message
```
AI refactor - Phase 6: Extract HAR skills to config-driven module
* Create ai_har_skills.c with generic move executors
* Load character move data from resources/ai_config/hars/
* Add 50+ unit tests per-HAR
```

---

## Phase 7: Modmanager Config Extension Integration
**Duration**: 2-3 days  
**Effort**: Medium  
**Risk**: Medium (load-order and fallback semantics)

### Current Status (2026-06-12)
- [x] Added `.json` file loading to `modmanager_init()` (stored as `MOD_BUFFER` lists, same as `.ini`).
- [x] Added `modmanager_apply_json_overlays()` API to `modmanager.c/h` with `modmanager_initialized` guard.
- [x] Added `ai_config_apply_pilot_overlay()` to `ai_config_loader.c/h`; `ai_config_load_pilot_personality()` now calls modmanager overlays after loading base file.
- [x] Added `ai_skills_config_apply_overlay()` to `ai_skills_config_loader.c/h`; `load_har_config()` now calls modmanager overlays after loading base file.
- [x] Added `testing/ai/ai_config_mod_overlay_test.c` with 19 overlay/fallback/load-order tests.
- [x] Fixed pre-existing `CAT_FIRE_ICE` undeclared error in `ai_move_selector.c`.
- [x] Fixed pre-existing `testing/ai/ai_controller_test.h` include path error in test files.
- Phase 7 implementation status: complete.
- Total tests: 504, all passing.
- Next: Phase 8 — event handler refactoring & integration.

### Goals
- Integrate AI config lookup loading with `src/resources/modmanager.c/h`
- Allow mods to extend/override AI config without replacing base files
- Keep deterministic fallback behavior when no mods are present

### Deliverables

1. **Add modmanager-backed config extension path for AI config loading**
   - Wire config lookup in `ai_config_loader` / `ai_skills_config_loader` through modmanager APIs
   - Ensure base `resources/ai_config/*` is always loaded first, then mod overlays are applied in mod load order
   - Keep non-mod behavior identical to current implementation

2. **Define mod config path conventions**
   - Document extension keys/paths for AI config overlays (pilots, tactics, difficulty, HAR skill files)
   - Ensure naming/path rules are explicit and validated

3. **Add focused tests for extension semantics**
   - Overlay merge behavior (base + one mod + multiple mods)
   - Missing-key fallback behavior
   - Deterministic load-order behavior with competing overrides

4. **Controller integration verification**
   - Verify `attempt_*_attack()` config lookups continue to work with modded character data
   - Verify tactic and pilot personality loading can be extended via mod data

### Verification
- Baseline parity with no mods enabled
- Overlay tests pass with one and multiple mods
- Existing AI regression tests still pass

### Commit Message
```
AI refactor - Phase 7: Integrate AI config lookups with modmanager overlays
* Route AI config loading through modmanager extension lookups
* Preserve base config fallback semantics
* Add overlay/load-order tests for modded AI config
```

---

## Phase 8: Event Handler Refactoring & Integration
**Duration**: 4-5 days  
**Effort**: Medium  
**Risk**: Medium (complex logic, many edge cases)

### Goals
- Simplify `ai_har_event()` routing
- Extract learning logic
- Full integration & final testing

### Deliverables

1. **Extract event response helpers**
   - Create functions for each event type:
     - `ai_event_on_land_hit()`, `ai_event_on_block()`, `ai_event_on_hit()`, etc.
     - Each returns: "what tactic to queue" or "no tactic"
   - Tests: `testing/ai/ai_event_test.c` (30 unit tests)

2. **Extract learning logic**
   - Create `src/game/ai/ai_learning.c`:
     - `ai_learning_adjust_from_throw()` - adapt to repeated throws
     - `ai_learning_adjust_from_projectile()` - adapt to repeated projectiles
     - `ai_learning_forget()` - random personality reset
   - Tests: `testing/ai/ai_learning_test.c` (10 unit tests)

3. **Refactor `ai_har_event()` in src/controller/ai_controller.c**
   - Replace massive switch → cleaner routing:
     ```c
     switch(event.type) {
       case HAR_EVENT_LAND_HIT:
         update_move_stats(event.move);
         tactic = ai_event_on_land_hit(ctrl);
         if(tactic) queue_tactic(ctrl, tactic);
         break;
       // ... other events
     }
     ```
   - Much shorter, easier to understand
   - Tests verify same tactic decisions

4. **Integration testing**
   - Create `testing/ai/ai_integration_test.c`
   - Simulate 10-round match, verify AI doesn't crash
   - Test all event combinations
   - Compare AI statistics (moves used, damage dealt) vs Phase 6

5. **Regression testing**
   - Run full match: old AI vs new AI
   - Same random seed = same decisions
   - Document any unavoidable differences (e.g., floating point rounding)

6. **Final cleanup**
   - Remove duplicate code
   - Ensure all modules follow same style
   - Update comments/documentation
   - Verify all 200+ tests pass

### Verification
```bash
# Regression test against Phase 7
testing/ai/regression_test --match-length=10 --seed=99999 \
   --old=phase7 --new=phase8 --tolerance=0.001

# Coverage report
lcov --directory=. --capture --output-file=coverage.info
lcov --remove coverage.info '/usr/*' --output-file=coverage.info
genhtml coverage.info --output-directory=coverage_html
```

### Commit Message
```
AI refactor - Phase 8: Simplify event handler and complete refactoring
* Extract event response logic to helper functions
* Extract learning adjustments to ai_learning.c
* Refactor ai_har_event() for readability
* Add 40+ unit tests for events and learning
* All 200+ tests pass, 85%+ code coverage
```

---

## Testing Summary by Phase

| Phase | New Tests | Total Tests | Coverage Target |
|-------|-----------|-------------|-----------------|
| 1 | 20 | 20 | Decision engine 100% |
| 2 | 20 | 40 | Utils 100% |
| 3 | 10 | 50 | Movement 95% |
| 4 | 25 | 75 | Move selection 95% |
| 5 | 100 | 175 | Tactic engine 90% |
| 6 | 50 | 225 | HAR skills 90% |
| 7 | 20 | 245 | Modmanager overlay integration 90% |
| 8 | 40 | 285 | Event handling 85%, overall 85% |

---

## Rollback Strategy

If a phase introduces regression (AI behaves differently):

1. **Run regression test**
   ```bash
   testing/ai/regression_test --phase=<N> --tolerance=0.001
   ```

2. **Identify problem**
   - Compare old vs new decisions with same seed
   - Check if random number sequence changed
   - Verify game state mocking is correct

3. **Fix and re-verify**
   - Adjust implementation
   - Re-run phase-specific tests
   - Run full integration test

4. **If unfixable**
   - Revert to previous phase
   - Document issue in design doc
   - Plan alternative approach

---

## Success Criteria

- ✅ All 285 tests pass
- ✅ 85%+ code coverage
- ✅ AI behavior identical to original (same seed = same decisions)
- ✅ All HAR skills load from config
- ✅ Pilot personalities load from JSON
- ✅ AI config loading supports modmanager-based overlays/extensions
- ✅ New developers can add tactic in <1 hour
- ✅ No changes to public API (`ai_controller.h`)
- ✅ No changes to game code (`game/`, `scenes/`, etc.)
- ✅ Config files validate on load

---

## Next Steps After Refactoring

### Immediate (1-2 weeks after Phase 8)
1. **Code review** - Have another dev review entire refactoring
2. **Architecture documentation** - Create `../source/architecture/ai.rst` Sphinx documentation
   - Document current AI controller/engine behavior and responsibilities
   - Explain module structure and dependencies (include DAG diagram)
   - Document configuration system (pilots, tactics, character moves)
   - Document how to add new tactics, customise har move preferences, and modding guide
   - This doc must be kept up-to-date as the AI system evolves
3. **Module documentation** - Update code comments, add module READMEs in `src/game/ai/`
4. **Performance testing** - Ensure no fps regression
5. **Config validation** - Add JSON schema validation

### Short term (1-2 months)
1. **Modding guide** - Write documentation for adding new tactics/characters
2. **Balance patches** - Use config to tune difficulty/pilot personalities
3. **New tactic experiment** - Add 1 new tactic via config (no code changes)
4. **New character experiment** - Add 1 new character via config

### Long term (3+ months)
1. **In-game AI tuning** - Allow difficulty selection to affect config loading
2. **AI statistics dashboard** - Track which tactics are used most
3. **Player-vs-AI ratings** - Difficulty balancing based on win rates
4. **Community mods** - Support external config files

---

## Risks & Mitigation

### Risk 1: Randomness Changes During Refactoring
**Problem**: Different code path = different rand_int() call order = different sequence  
**Mitigation**: Regression test with explicit seed; compare decision sequences, not just outcomes

### Risk 2: HAR Behavior Subtly Different
**Problem**: Edge cases in move validation, state checks  
**Mitigation**: Per-HAR testing; record baseline for all 10 HARs before starting Phase 6

### Risk 3: Config Files Missing at Runtime
**Problem**: New AI creation fails if pilots.json not found  
**Mitigation**: Load defaults from code if config missing; log warning; document in README

### Risk 4: Performance Regression
**Problem**: More function calls, config lookups slow things down  
**Mitigation**: Benchmark Phase 0 vs Phase 7; cache config in module; profile hot paths

### Risk 6: Mod Overlay Conflict Semantics
**Problem**: Multiple mods overriding the same config keys can produce unexpected behavior  
**Mitigation**: Enforce deterministic load-order, document precedence rules, add explicit conflict tests

### Risk 5: Circular Dependencies During Refactoring
**Problem**: Module A needs module B which needs module A  
**Mitigation**: Enforce acyclic dependency rules; use include guards; peer review on each commit

---

## Estimated Timeline

- **Phase 1**: 3-4 days (foundation)
- **Phase 2**: 2-3 days (utilities)
- **Phase 3**: 3-4 days (movement)
- **Phase 4**: 4-5 days (move selector)
- **Phase 5**: 5-7 days (tactics + config)
- **Phase 6**: 6-8 days (HAR skills) ⬅️ **Longest phase**
- **Phase 7**: 2-3 days (modmanager config extension integration)
- **Phase 8**: 4-5 days (events + integration)

**Total: 29-39 days (5-6 weeks)**

**With code review, testing, documentation: 6-9 weeks**

