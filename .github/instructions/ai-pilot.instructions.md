---
applyTo: "src/**, testing/ai/**, resources/ai_config/**, docs/ai-pilot/**"
---

# AI pilot instructions

## Scope
This instruction set applies to AI subsystem work, HAR behavior tuning, pilot preference changes, tactic selection, deterministic AI validation, and docs under `docs/ai-pilot/`.

## Primary principles
- Prefer data-driven tuning over hard-coded behavior changes.
- Keep difficulty scaling separate from special pilot or boss-specific overrides.
- Treat anti-spam and cooldown logic as a first-class part of AI behavior.
- Use deterministic tests to validate AI regressions before and after a tuning change.

## Where to look
### Config and data
- `resources/ai_config/tactics.json`
- `resources/ai_config/pilots.json`
- `resources/ai_config/hars/*.json`
- `resources/ai_config/ai_core.ini`
- `resources/ai_config/ai_difficulty/*.ini`

### Runtime logic
- `src/game/ai/ai_learning.c`
- `src/game/ai/ai_tactic_engine.c`
- `src/game/ai/ai_types.h`
- `src/controller/ai_controller.c`

### Deterministic validation
- `testing/ai/ai_learning_test.c`
- `testing/ai/ai_tactic_engine_test.c`
- `testing/ai/ai_har_skills_test.c`
- `testing/ai/ai_state_test.c`

## Tuning guidance
### If the issue is a spam loop or repeat pressure pattern
- inspect the cooldown and burst logic in `ai_learning.c`
- look at projectile and pressure streak logic before changing HAR JSON
- add a regression test that proves the issue and the fix

### If the issue is range or move selection
- check `tactics.json` and the relevant HAR JSON file
- verify the move `range_min` / `range_max` and conditions match the intended behavior
- ensure the tactical gating code matches the move profile

### If the issue is pilot personality
- adjust pilot preference values in `pilots.json`
- avoid conflating pilot preference with base difficulty tuning

### If the issue is special pilot or tournament behavior
- keep special overrides separate from `ai_difficulty` data
- document the override precedence clearly

## Logging and debugging
- Use `--log-level=DEBUG --log-modules=ai-tactics` for tactic-only debugging.
- Keep runtime logs scoped to `ai` and `ai-tactics` rather than mixing controller logs into tactic traces.
- If a fix includes logging changes, ensure the logs remain useful for tuning rather than noisy.

## Validation commands
Use the real deterministic test path:
- `cmake --build build --target openomf_test_main`
- `OPENOMF_RUN_DETERMINISTIC_TESTS=1 ./build/openomf_test_main`

For runtime observation:
- `.devcontainer/build.sh gui --log-level=DEBUG --log-modules=ai-tactics`

## Documentation expectations
When changing AI behavior, document:
- the root cause
- the config/code surfaces changed
- what deterministic test covers the fix
- which tuning knob a modder should adjust in the future

This keeps the AI system easy to tune without reverse-engineering the implementation.
