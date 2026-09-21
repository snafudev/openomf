# AI Controller Refactoring - Executive Summary

**Chosen Approach**: Approach C (Full Separation with Plugin Pattern)  
**Scope**: Internal refactoring only (public API unchanged)  
**Timeline**: 5-7 weeks (8 phases, 285 tests)  
**Status**: Design Complete → Ready for Phase 1 Implementation  
**⚠️ IMPORTANT**: Do not automatically commit during refactoring. Follow the commit messages provided in MIGRATION-PLAN.md for each phase exactly. This ensures proper history and clear documentation of changes.

---

## Quick Reference

### Documents
- **REFACTORING-DESIGN.md** - Deep dive: architecture analysis, dependencies, what CAN/CANNOT be extracted
- **MIGRATION-PLAN.md** - Step-by-step phases, test plans, success criteria, rollback strategy

### Key Decisions
- ✅ Break into modular components within `src/controller/`
- ✅ Config-driven personalities, tactics, character moves
- ✅ Config loading extended via modmanager overlays so mods can extend AI config
- ✅ No public API changes (game code unaffected)
- ✅ No breaking changes to existing saves
- ❌ Cannot fully extract: event handler (too complex, context-dependent)
- ❌ Cannot easily extract: hard-coded character move sequences → but CAN extract to config + generic executor

---

## Breakdown: What Gets Extracted

### NEW MODULES (7 files, ~1400 LOC total)

1. **ai_decision_engine.c/h** (~150 LOC)
   - Pure decision functions: `smart_usually()`, `dumb_usually()`, `diff_scale()`, `learning_moment()`, `roll_pref()`, etc.
   - Zero state mutation
   - 100% test coverage

2. **ai_utils.c/h** (~100 LOC)
   - Helpers: `get_enemy_range()`, `har_has_projectiles()`, `char_to_act()`, etc.
   - Pure functions
   - 100% test coverage

3. **ai_state.c/h** (~50 LOC)
   - Initialization: `reset_tactic_state()`, `reset_act_timer()`
   - Simple setters

4. **ai_movement.c/h** (~80 LOC)
   - Movement direction selection logic
   - Parameterized HAR state
   - 95% test coverage

5. **ai_move_selector.c/h** (~150 LOC)
   - Move validation & scoring
   - Separate from execution
   - 95% test coverage

6. **ai_tactic_engine.c/h** (~250 LOC)
   - Tactic decision logic (registry-based instead of switch statement)
   - 11 tactics with conditions
   - 90% test coverage

7. **ai_har_skills.c/h** (~400 LOC)
   - Generic move executors (charge, push, projectile, trip)
   - Loads move data from config
   - 90% test coverage

8. **ai_config/ (NEW FOLDER)**
   - `ai_config_loader.c` - JSON/YAML parsing
   - `ai_tactics_config.c` - Tactic registry loader
   - `ai_skills_config.c` - Character move registry loader

### UPDATED FILE
- **ai_controller.c** (~2875 → ~500 LOC)
  - Reduced from 2875 to ~500 lines (82% reduction)
  - Stays as main orchestrator
  - Calls modular subsystems instead of inline logic
  - Keeps public API unchanged

### REMOVED CODE
- Hard-coded switch statements (move sequences, tactics, personalities)
- All logic moves to either:
  - Config files (moddable data)
  - Module functions (testable logic)

### MOD OVERLAY LAYER
- Base AI config still loads from `resources/ai_config/*`
- Mod overlays are applied through `src/resources/modmanager.c/h` in deterministic load order
- Missing/invalid mod keys fall back safely to base config

---

## Config Files (NEW)

```
resources/ai_config/
├── pilots.json                 # 11 pilot personalities
├── tactics.json                # Tactic metadata + conditions
├── difficulty.yaml             # Difficulty scaling curves
└── characters/
    ├── jaguar.json             # Charge, push, projectile moves
    ├── shadow.json
    ├── katana.json
    ├── flail.json
    ├── thorn.json
    ├── pyros.json
    ├── electra.json
    ├── chronos.json
    ├── shredder.json
    ├── nova.json
    └── gargoyle.json
```

---

## 8-Phase Migration (Sequential)

| Phase | Name | LOC Added | Tests | Duration | Risk |
|-------|------|-----------|-------|----------|------|
| 1 | Decision Engine | 150+20 | 20 | 3-4 days | None |
| 2 | Utilities | 100+20 | 20 | 2-3 days | Low |
| 3 | Movement | 80+10 | 10 | 3-4 days | Medium |
| 4 | Move Selector | 150+25 | 25 | 4-5 days | Low |
| 5 | Tactics + Config | 250+100 | 100 | 5-7 days | High |
| 6 | HAR Skills | 400+50 | 50 | 6-8 days | High |
| 7 | Modmanager Config Integration | 20 | 20 | 2-3 days | Medium |
| 8 | Events + Integration | 40 | 40 | 4-5 days | Medium |
| **TOTAL** | | **1400** | **285** | **29-39 days** | |

### Key Milestones

- **After Phase 2**: All utilities extracted, AI still monolithic
- **After Phase 4**: Move selection modular, event handler still monolithic
- **After Phase 5**: 🔥 MAJOR: Pilot personalities load from config, tactics registry working
- **After Phase 6**: 🔥 MAJOR: Character moves load from config, 10 HARs tested individually
- **After Phase 8**: ✅ COMPLETE: All 285 tests pass, 85%+ coverage, fully modular

---

## What CAN Be Broken Apart

✅ **Easy** (Pure functions, no state mutation)
- Decision rolls (smart_usually, etc.)
- Movement selection
- Utility helpers
- State initialization

⚠️ **Medium** (Requires logic refactoring, but feasible)
- Move selection/evaluation
- Tactic engine (extract from switch statement)
- Character skills (generic executor + config)

---

## What CANNOT Be Broken Apart

❌ **Event Handler (`ai_har_event()`)**
- Why: 10 event types × 3-5 response chains each = exponential complexity
- Problem: Each event depends on current tactic state, game state, AI history
- Solution: Keep in main controller, simplify by extracting decision helpers
- Mitigation: Extract event response logic into helper functions

❌ **Hard-Coded Character Move Sequences**
- Why: Each HAR has unique move combos, hard-coded button sequences
- Current: `case HAR_JAGUAR: int cmds[] = {BACK, DOWNBACK};`
- Solution: Extract move DATA to config, keep move EXECUTION in code
- Result: Generic executor that reads move sequences from config files

❌ **Pilot Learning Algorithm**
- Why: Cross-cutting concern, affects multiple code paths
- Problem: Learning happens during move selection AND event handling
- Solution: Keep core logic in code, but load pilot baseline from config
- Result: Base personalities in JSON, learning adjustments in code

---

## Testing Strategy

### 285 Total Tests Across 8 Phases

1. **Unit Tests** (by module)
   - Decision engine: 20 tests (all paths for each difficulty)
   - Utils: 20 tests (range classification, state checks)
   - Movement: 10 tests (direction selection + jumping)
   - Move selector: 25 tests (validation, scoring, selection)
   - Tactic engine: 100 tests (all 11 tactics × game state combinations)
   - Character skills: 50 tests (per-HAR move validation)
   - Event handling: 30 tests (all event types, tactic responses)
   - Learning: 10 tests (adaptation on throws/projectiles)

2. **Integration Tests**
   - Scenario tests: 10+ simulated 1v1 matches
   - Regression tests: New code vs old code, same random seed = same decisions
   - Config validation: All JSON/YAML files parse correctly

3. **Coverage Goals**
   - Decision engine: 100%
   - Utilities: 100%
   - Overall: 85%+

---

## Files to Create (Phase by Phase)

### Phase 1
```
src/game/ai/ai_decision_engine.c
src/game/ai/ai_decision_engine.h
testing/ai/ai_decision_engine_test.c
```

### Phase 2
```
src/game/ai/ai_utils.c
src/game/ai/ai_utils.h
src/game/ai/ai_state.c
src/game/ai/ai_state.h
testing/ai/ai_utils_test.c
testing/ai/ai_state_test.c
```

### Phase 3
```
src/game/ai/ai_movement.c
src/game/ai/ai_movement.h
testing/ai/ai_movement_test.c
```

### Phase 4
```
src/game/ai/ai_move_selector.c
src/game/ai/ai_move_selector.h
testing/ai/ai_move_selector_test.c
```

### Phase 5
```
src/game/ai/ai_tactic_engine.c
src/game/ai/ai_tactic_engine.h
src/game/ai/ai_config.c
src/game/ai/ai_config.h
resources/ai_config/pilots.json
resources/ai_config/tactics.json
resources/ai_config/difficulty.yaml
testing/ai/ai_tactic_engine_test.c
testing/ai/ai_config_test.c
```

### Phase 6
```
src/game/ai/ai_har_skills.c
src/game/ai/ai_har_skills.h
src/game/ai/ai_skills_config.c
resources/ai_config/hars/jaguar.json
resources/ai_config/hars/shadow.json
(... all 10 HARs)
testing/ai/ai_har_skills_test.c
```

### Phase 7
```
src/game/ai/ai_config_overlay.c
src/game/ai/ai_config_overlay.h
testing/ai/ai_config_overlay_test.c
```

### Phase 8
```
src/game/ai/ai_learning.c
src/game/ai/ai_learning.h
testing/ai/ai_event_test.c
testing/ai/ai_learning_test.c
testing/ai/ai_integration_test.c
```

---

## How to Verify Success

After each phase, run:

```bash
# Run phase-specific tests
cmake --build build
ctest --verbose -R "ai_<phase_name>"

# Regression test (same random seed = same behavior)
testing/ai/regression_test --seed=12345 --phase=<N> --tolerance=0.001

# Code coverage
lcov --directory=src/game/ai --capture --output-file=coverage.info
genhtml coverage.info --output-directory=html

# Static analysis
clang-analyzer --analyze src/game/ai/ai_*.c
```

---

## Dependencies (Internal Module DAG)

```
ai_utils, ai_decision_engine  [leaf - no dependencies]
  ↑
ai_movement, ai_state         [leaf]
  ↑
ai_move_selector              [depends on utils, decision]
  ↑
ai_tactic_engine              [depends on utils, decision, move_selector]
  ↑
ai_har_skills           [depends on utils]
  ↑
ai_config                      [depends on all above]
   ↑
modmanager overlay layer       [extends base config with mod-provided data]
  ↑
ai_controller.c               [orchestrates all, main entry point]
```

---

## Success Criteria

- ✅ All 285 tests pass
- ✅ 85%+ code coverage
- ✅ AI behavior identical to original (regression tests)
- ✅ Pilot personalities load from JSON
- ✅ Character moves load from config
- ✅ AI config is extensible via modmanager overlays
- ✅ New tactics can be added in config without code changes
- ✅ New characters can be added in config without code changes
- ✅ No public API changes (ai_controller.h unchanged)
- ✅ No game code changes required (src/game/ untouched)
- ✅ No changes to existing saves (backward compatible)

---

## Next Steps

### Immediate (Now → Phase 1)
1. Review REFACTORING-DESIGN.md and MIGRATION-PLAN.md
2. Set up test framework & test fixtures
3. Create CMakeLists.txt entries for new modules
4. Begin Phase 1: Extract decision engine

### After Phase 1
1. Get peer code review
2. Run regression tests
3. Begin Phase 2

### After Phase 8
1. **Final code review** - Have another dev review entire refactoring
2. **Architecture documentation** - Create `../source/architecture/ai.rst` Sphinx documentation
   - Document current AI controller/engine behavior and responsibilities
   - Explain module structure and dependencies
   - Document configuration system (pilots, tactics, character moves)
   - Document how to add new tactics and characters (modding guide)
   - Keep this document up-to-date as the AI system evolves
3. **Module documentation** - Update code comments and add module READMEs in `src/game/ai/`
4. **Update developer documentation** - Add AI refactoring to main development guide
5. **Publish release notes** - Document all changes and new capabilities

---

## References

- **REFACTORING-DESIGN.md** - 400+ lines of deep analysis
  - Current architecture breakdown
  - What CAN/CANNOT be extracted (detailed reasoning)
  - Module boundaries
  - Dependency rules

- **MIGRATION-PLAN.md** - 300+ lines of step-by-step guide
   - All 8 phases with deliverables
  - Test plan for each phase
  - Verification steps
  - Rollback strategy

- **This document** - Quick reference (you are here)

