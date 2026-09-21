# AI Behaviour Models & Scenario Testing — Master Plan

**Status**: M0–M7 complete; follow-up tuning continues as normal maintenance
**Date**: 2026-09-20
**Scope**: AI enemy controller — behaviour models, per-difficulty profiles, on-the-fly recording/verification, HAR move-execution validation, analysis tooling
**Depends on**: `../../REFACTORING-DESIGN.md`, `../../MIGRATION-PLAN.md`, `../../AI_MOVE_CATALOG.md`, `../../AI_DIFFICULTY_CONFIG.md`, `../../plans/2026-08-27-difficulty-moddable-ai.md`

## Progress snapshot (2026-09-20)

This is the working status of the current AI refactor. The M0–M7 milestone chain is now complete and validated; the remaining work is ongoing tuning and maintenance around the AI behaviour data, not foundational implementation.

| Milestone | Status | Notes |
|-----------|--------|-------|
| M0 — Per-module debug logging + AI decision trace | ✅ Complete | Module-scoped logging was added and verified with `--log-modules=ai`; AI decision noise can now be filtered per subsystem. |
| M1 — On-the-fly REC generation with assertions | ✅ Complete | Deterministic REC generation + assertion replay is working; pass/fail assertion behavior was verified. |
| M2 — HAR move-execution verification matrix | ✅ Complete | Real AI tactic execution is validated by preloading a tactic queue and asserting the HAR moves as expected. |
| M3 — "Find a good move when close" | ✅ Complete | Move selector scoring and selection policy now prefer the best valid close-range option instead of first-match logic. |
| M4a — Custom pilot/HAR AI model overrides for bosses & special pilots | ✅ Complete | Dedicated Raven/Kreissack profile separation and Nova HAR precedence are implemented and validated against regression tests. |
| M4 — Behaviour models + difficulty config cascade | ✅ Complete | Core config/difficulty layering and modular model loading are implemented and in place. |
| M5 — Scenario runner (N trials, deterministic) | ✅ Complete | Multi-run behavioural validation is available and deterministic within the harness. |
| M6 — Analysis skill + log/metric tooling | ✅ Complete | AI trace parsing, multi-run summary comparison, and the VS Code analysis workflow are implemented and tested. |
| M7 — Tuning loop + regression gate | ✅ Complete | Baseline drift checking and the tuning loop gate are in place and verified by the analysis regression suite. |

### Current delivery focus

1. Finalise the analysis tooling for AI logs, traces, and aggregated scenario metrics.
2. Lock the tuned difficulty profiles and regression checks behind the scenario runner and REC assertion harness.
3. Continue the cleanup that moves remaining `log_debug()` calls onto the AI logging module/channel path and keeps the debug stream reviewable.
4. Use the verified model stack to tune boss/pilot variants and tournament-specific overrides without disturbing the default fallback path.
5. Close out the M7 tuning loop and final regression gate before moving on to additional AI expansion work.

### Current confirmed status

This roadmap is now effectively in the “analysis and tuning” phase. The core move-selection fix, config-cascade model layer, scenario execution harness, and boss/pilot override work are all in place and verified: HAR execution prefers the highest-scoring valid move instead of taking the first passable move when several sit in the same attack range; the behaviour model and difficulty cascade are active; deterministic multi-trial scenario checks are running; and Raven/Kreissack remain distinct profiles, with the Nova HAR archetype treated as a separate slow, heavy, projectile-focused profile rather than a generic override.

The immediate next milestone is to turn those verified execution paths into a clean, repeatable tuning loop: aggregate the AI trace data, compare difficulty profiles across scenarios, and lock the final regression gate for the M6/M7 analysis and tuning pass.

### HAR move-execution fix: AF move-string convention (2026-09-20)

While validating M2 (HAR move-execution), we found that Shadow never fired its
projectile moves and instead whiffed a standing punch/kick at range. Root cause:
the `.AF` `move_string` matcher requires a motion+button move's button to be
pressed **while the motion's final direction is still held** (a bare button
inserts a neutral `5` into the input buffer and breaks the match). The AI
executor sent the button as a bare input.

- Fixed in `src/game/ai/ai_har_skills.c` via `ai_resolve_move_input()`, which
  combines a bare `P`/`K` with the previously held direction; the executor now
  tracks/clears that direction (neutral `5` resets it for charge moves).
- Audited every `resources/ai_config/hars/*.json` sequence against its fighter's
  AF move strings and corrected two phantom moves:
  - Electra `super_rolling_thunder` was `hcf+P` (no such AF move) → corrected to
    `qcf,f+P` (`P63`), the wiki's "Shadow Rolling Thunder".
  - Gargoyle `shadow_talon` was `hcf+P` (only exists as CAT_SCRAP) → removed.
- Full reference, convention, HAR↔FIGHTR mapping, and the audit table are in
  `../analysis/AF_MOVE_STRING_REFERENCE.md`.

This closes the "HAR moves wave weak attacks at range" symptom for all HARs and
is the prerequisite for a reliable M2 move-execution matrix.

### New work item: pilot attribute parity + boss AI model

This requirement is now complete for the initial implementation and is tracked as a completed M4a milestone in the roadmap above:

- Replace the hard-coded pilot stat defaults with values derived from the reference data for normal pilots and boss variants.
- Treat Major Kreissack as a special-case boss profile with boosted stats and a custom AI model that reflects his boss-role behavior.
- Tune the Nova HAR profile to match its role as a slow, heavy, projectile-focused robot, not just a generic mid-tier robot profile.
- Confirm the custom model is selected for this boss and that the AI can express its extra durability, pressure, and projectile behavior without breaking the default difficulty model pipeline.
- Preserve the distinction between the pilot identity (e.g. Raven / Kreissack) and the HAR archetype (Nova) so the boss override stays authoritative while the HAR profile remains modular and additive.

### Design inputs: pilot personalities, stats, HAR identity, and tournament bonuses

The following behavioural model inputs should be treated as authoritative design constraints when selecting or tuning AI personalities:

- Pilot AI personalities are not generic. They have distinct behavioural tendencies:
  - Ibrahim is described as vicious, moderately intelligent, and willing to use heavy attacks.
  - Raven favours special moves and throws, and dislikes defense.
  - Crystal loves special moves while still maintaining solid defense.
  - Other pilots (for example Angel) have unique combat patterns that should show up as model-specific preferences rather than a single uniform AI profile.
- Pilot stats directly affect HAR performance. Strength improves damage output; endurance increases health and stun resistance; agility increases animation speed and jump height. These values also feed passive combat tendencies such as faster punches or improved stun tolerance.
- HAR identity defines the move pool and the robot's built-in role. The pilot changes how the HAR plays, but the HAR itself defines the attack range, movement profile, speed, and special abilities. Examples:
  - Chronos manipulates time.
  - Flail has exceptional reach.
  - Gargoyle leans heavily toward aerial pressure.
- Tournament mode enhancements are a major behavioural modifier layer. Certain HARs receive tournament-only upgrades that change their tactical profile:
  - Shadow gains a Sub-Zero-style freeze effect.
  - Flail can cancel rushpunches into command grabs.
  - Jaguar can become a more combo-heavy, highly agile nightmare.
- Implication for behaviour models: the AI system should be modeled as a layered identity stack: pilot personality + pilot stats + HAR capability + tournament modifier. Behaviour models must therefore be able to express not only difficulty scaling, but also pilot- and HAR-specific tactical bias.

These notes should be folded into the M4 design for model selection and the M7 tuning loop, especially when creating boss-specific or tournament-variant profiles.

---

## 1. Context & Current State (verified against source)

The Approach-C refactor is **largely complete**. `src/controller/ai_controller.c` is now a
~1300-line orchestrator over modular subsystems in `src/game/ai/`:

| Module | Role |
|--------|------|
| `ai_decision_engine` | pure difficulty/preference rolls (`diff_scale`, `roll_pref`, `smart_usually`…) |
| `ai_movement` | movement direction + jump selection |
| `ai_move_selector` | move validation & scoring (`ai_move_select_best`) |
| `ai_tactic_engine` | 11 tactics, conditions, queue/chain |
| `ai_har_skills` | generic charge/push/trip/projectile executors |
| `ai_skills_config_loader` | parses `resources/ai_config/hars/*.json` → `ai_move_def[]` |
| `ai_config_loader` | `pilots.json` personality loading (+ mod overlay) |
| `ai_core_config` | `ai_core.ini` runtime params |
| `ai_event` / `ai_learning` / `ai_state` / `ai_utils` | event routing, adaptation, reset, helpers |

**Known gaps that this plan addresses:**

1. **`ai_core_config` is minimal.** Only 6 fields; the
   `../../plans/2026-08-27-difficulty-moddable-ai.md` plan (block frequency, passive defense,
   aggression, movement distance pref, jump mult) was **not implemented**. The
   `resources/ai_config/ai_difficulty/*.ini` files exist on disk but the loader does **not**
   cascade-load them (no difficulty parameter).
2. **No "behaviour model" concept.** `ai` (`src/game/ai/ai_types.h`) holds `difficulty`,
   `pilot`, `tactic_state`, `move_stats[70]` — no profile/model reference. Difficulty is
   still partly hardcoded via `diff_scale(a)` mixed with config rolls in `ai_controller_poll()`.
3. **Recording is input-only for generated tests.** `tools/gen_move_test.c` +
   `pytest/test_move_triggers.py` generate REC files with *inputs only*; they do not embed
   **assertions** (even though `src/formats/rec_assertion.{c,h}` and `REC_LOOKUP10_ASSERT_BYTE`
   already exist and are evaluated by `rec_controller.c`).
4. **No per-module debug logging.** `src/utils/log.{h,c}` supports a single global level; there
   is no way to say "listen to `ai` and `tactic` debug messages only".
5. **HAR move execution is partway verified.** `ai_har_execute_move_list` picks the *first*
   move whose `range_min/range_max` and conditions pass; trip attack is still hardcoded. The
   "when close, find a good move" behaviour is not implemented.

---

## 2. Goals

1. **Behaviour models** — loadable, configurable AI profiles (multiple per difficulty, and
   per-pilot profiles for tournament mode) that compose the existing config layers.
2. **Scenario testing** — run a behaviour model N times in a fixed scenario, deterministically,
   and verify/aggregate how it behaves.
3. **On-the-fly recording + verification** — tests create a REC at run time (inputs + embedded
   assertions), replay it headless, and verify the move/behaviour actually executed.
4. **HAR move-execution confidence** — a green matrix for every catalogued move on every HAR.
5. **Iteration loop** — tweak config → run scenarios → diff behaviour metrics, until each
   difficulty is tuned to satisfaction.

---

## 3. Architecture

```
resources/ai_config/
├── ai_core.ini                      # base runtime params (today)
├── ai_difficulty/<level>.ini        # per-difficulty overrides (cascade into core config)
├── pilots.json                      # pilot personalities (exists)
├── tactics.json                     # tactic metadata/conditions (exists)
├── hars/<har>.json                  # move defs per HAR (exists)
└── models/<model>.json              # NEW: named behaviour bundles

Behaviour model (new, models/<name>.json):
{
  "id": "veteran_zoning",
  "difficulty": 2,            # 0..6 (PUNCHING_BAG..ULTIMATE)
  "pilot_overrides": { ... }, # optional personality tweaks over pilots.json
  "tactic_weights": { ... },  # optional per-tactic weight/cooldown overrides
  "movement": { ... },        # optional movement/jump params
  "har_skill_overrides": { ... } # optional per-HAR move list patches
}
```

A model is a **composition** of the existing layers (difficulty INI → core config, pilot
personality, tactic registry, HAR skills). `ai_controller_create()` gains a model id (defaulting
to a model derived from `difficulty` so current behaviour is unchanged).

---

## 4. Milestones

Milestones are ordered by dependency; each is independently shippable and testable.

> Implementation note: M0 and M1 are complete and validated. M2 has become the primary active milestone because it validates the real AI execution path before broader behavioural tuning begins.

| # | Milestone | Size | Risk |
|---|-----------|------|------|
| M0 | Per-module debug logging + AI decision trace | S | Low |
| M1 | On-the-fly REC generation **with assertions** | M | Med |
| M2 | HAR move-execution verification matrix (+ fixes) | M | Med |
| M3 | "Find a good move when close" move selection | M | High |
| M4a | Custom pilot/HAR AI model overrides for bosses & special pilots | M | Med |
| M4 | Behaviour models + difficulty config cascade | L | Med |
| M5 | Scenario runner (N trials, deterministic) | L | High |
| M6 | Analysis skill + log/metric tooling | M | Low |
| M7 | Tuning loop + regression gate | M | Med |

### M4a — Custom pilot/HAR AI model overrides for bosses and special pilots

**Goal:** allow explicit AI model selection per pilot and HAR without breaking the normal difficulty cascade. This covers the boss-pilot special cases and the “pilot + HAR + difficulty” layered identity model.

**Deliverables**
1. Add a model-selection layer that can override the default difficulty-based model for specific pilots, e.g. Major Kreissack and his Nova HAR profile.
2. Treat Major Kreissack as a boss profile with custom model logic, elevated stats, and a distinctive tactical profile rather than a normal pilot using the default difficulty cascade.
3. Tune the Nova HAR profile as a separate slow, heavy, projectile-focused boss archetype instead of a generic mid-tier HAR profile.
4. Verify custom-model selection still works when the difficulty cascade is active, so the boss override is additive rather than replacing the whole config stack.
5. Keep the design extensible for future tournament-mode and special-pilot overrides such as other boss pilots or signature HAR variants.

**Tests:** repeatable scenario checks for pilot/HAR selection and model selection under difficulty cascade; boss profile stats compared against the OMF2097 reference spread.

**Verification:** a debug or summary trace shows the selected AI model for Major Kreissack/Nova and confirms the model id differs from the normal default difficulty model while the cascade and fallback rules remain intact.

### Follow-up: migrate remaining AI log_debug calls to AI module logging

The logging milestone is already represented by M0, but the cleanup task is still a follow-up work item because the codebase still contains ad hoc `log_debug()` usage outside the AI logging module.

**Deliverables**
1. Convert remaining AI engine `log_debug()` calls to the module-aware `log_debug_m()` / AI channel path.
2. Preserve the existing default behaviour when no module filter is set, while allowing `--log-modules=ai` to isolate AI diagnostics cleanly.
3. Keep the logging output consistent with the REC and tactic trace tooling so AI behaviour can be debugged and compared as a single subsystem.

**Verification:** `--log-modules=ai --log-level=DEBUG` emits only the AI subsystem diagnostics while other subsystems remain silent.

> Logging note: the AI tick/poll loop and the AI tactic-brain narrative should likely be split into distinct channels, e.g. `ai` for controller/state/tick diagnostics and `ai-tactics` for tactical decisions, queueing, and move-selection trace. This keeps low-level polling noise from drowning out the higher-level tactical reasoning when filtering by module.

---


### M0 — Per-module debug logging + AI decision trace

**Goal:** let the developer (and tools) subscribe to debug output from specific modules.

**Deliverables**
1. Extend `src/utils/log.{h,c}` with module-tagged logging:
   - `typedef uint64_t log_modules;` with registered tags (`LOG_MODULE_AI`, `_TACTIC`,
     `_HAR`, `_REC`, `_MOVE`, `_MOVEMENT`, …).
   - `#define log_debug_m(module, ...)` routing through `log_msg_module(module, LOG_DEBUG, ...)`.
   - Runtime filter via CLI `--log-modules=ai,tactic,rec` and/or `log_set_modules()`.
   - Backward compatible: no filter set → all modules (current behaviour).
2. Instrument the AI modules with `log_debug_m` (replace/augment the many commented-out
   `// log_debug(...)` lines in `ai_controller.c`).
3. Add a compact **per-frame decision trace** (tick, tactic, move, range, action bytes) emitted
   to a file under `LOG_MODULE_AI`, for the analysis tooling in M6.

**Tests:** `testing/misc/log_module_test.c` (filter include/exclude, multi-tag, file/stderr
outputs).

**Verification:** `./build/openomf --log-modules=ai --log-level=DEBUG -P rectests/6P.REC` shows
only AI-tagged debug lines.

---

### M1 — On-the-fly REC generation with assertions

**Goal:** tests construct a REC programmatically at run time, including **assertions**, and
replay it to verify behaviour.

**Deliverables**
1. Generalise `tools/gen_move_test.c` → `tools/gen_rec_test.c` (or extend it) with an
   assertion DSL on the command line, e.g.
   `gen_rec_test <har> <opp> <out.rec> --seed 1234 40 01 40 44 --assert "t=64 p0.animation == 12"`.
2. Reuse `rec_assertion.{h,c}` (`encode_assertion`, operand model: literal vs
   `(har_id, attr)`) to emit `lookup_id 10` records with `REC_LOOKUP10_ASSERT_BYTE`.
3. Add seed control (`REC_LOOKUP10_SETRANDOM_BYTE`) so generated tests are deterministic.
4. Add a pytest helper module (`pytest/ai_rec_harness.py`) that: builds a REC via the tool,
   runs `openomf -P` headless, and treats a non-zero exit (assertion failure) as a test
   failure.
5. Keep the tool's input model generic enough to later drive the AI-level tactic pre-load mode
   (M2): a REC must be able to express "p2 is AI, pre-load tactic T, tick N frames, assert"
   once playback supports an AI controller.

**Tests:** `pytest/test_rec_assertions.py` (assertion encode→replay pass/fail, seed
determinism, literal + HAR-attr operands).

**Verification:** a generated REC with a deliberately false assertion exits non-zero
(the existing `SHOULDFAIL.REC` pattern, now driven by our tool).

---

### M2 — HAR move-execution verification matrix (+ fixes)

**Goal:** prove every catalogued move actually triggers for every HAR; fix the executor/config
issues found along the way. This is the direct successor to the "part way through verifying and
still had issues" work.

**Verification model (two complementary layers):**
1. **Engine-level (REC)** — feed a move's input sequence and assert the HAR enters the expected
   animation/state. Proves the *input sequence* triggers the move.
2. **AI-level (tactic pre-load)** — the primary mechanism: run a real AI controller in a real
   arena, **pre-load the tactic queue** (`queue_tactic` / set `ai.tactic`), tick the game, and
   assert the AI's *own executor* (`ai_har_execute_*` → `ai_har_execute_move_list`) chose and
   fired the move (animation/state changed to the expected move). Proves the **AI actually
   executes the tactic**, not just that a hand-written input string happens to work.

**Deliverables**
1. **AI tactic harness** (new): a headless/in-process driver that creates a real `game_state`
   with an AI controller on player 2, pre-loads a tactic, ticks N frames with a fixed seed, and
   records + asserts via the M1 assertion machinery. This is the foundation for M5.
2. **Engine-level REC generation** (from M1): per move, a REC that (a) sets the HAR/opponent
   state deterministically, (b) feeds the move's input sequence, and (c) asserts the HAR enters
   the expected animation/state (using `ATTR_ANIMATION_ID` / `ATTR_STATE_ID`).
3. A pytest suite (`pytest/test_ai_move_execution.py`) that iterates all 11 HAR configs and all
   move types (charge/push/projectile/trip) and runs both layers headless.
4. Fix issues surfaced, at minimum:
   - trip attack currently hardcoded (`act_down_back | ACT_KICK` in
     `ai_har_execute_trip`) → make it config-driven like the others.
   - audit `range_min`/`range_max` and `conditions` semantics in `ai_har_execute_move_list`
     (first-match vs best-match — see M3).
   - verify `follow_up_tactics` chaining doesn't misfire.

**Note:** the AI-level layer needs playback support for "p2 = AI" (currently `-P` playback
forces both players to REC controllers). Either add an AI-controller mode to playback, or drive
the AI in-process via the engine's `game_state` (preferred, reuses the M1 assertion evaluator).

**Verification:** a move-execution matrix (markdown table in the project docs tree, auto-generated)
is all green for the base (non-modded) configs. `pytest/test_move_triggers.py` and
`pytest/test_ai_move_execution.py` both pass.

---

### M3 — "Find a good move when close"

**Goal:** when the AI is in `RANGE_CRAMPED`/`RANGE_CLOSE`, it should *search for a good move*
for the current situation rather than roll a random category.

**Deliverables**
1. Enrich `ai_move_selector.c` scoring context with situational inputs already available to the
   controller: enemy state (stunned / airborne / blocking / knocked-down), wall proximity,
   distance, own `move_stats` learning (damage, hit distance, success), and pilot prefs.
2. Change `ai_har_execute_move_list` selection from **first-match** to **best-match** (score all
   passing moves, pick highest) — gated so parity tests can lock current behaviour first.
3. In `ai_controller_poll()`/`attempt_attack()`, when close, route through the enriched
   selector so the AI prefers a move that is actually good for the current state (e.g. anti-air
   vs jumping enemy, low poke vs blocking enemy, throw vs stunned enemy).

**Tests:** extend `testing/ai/ai_move_selector_test.c` with situational cases (stunned, airborne,
blocking, cornered) and a parity suite proving non-close behaviour is unchanged under the same
seed.

**Verification:** scenario metrics (M5) show higher hit/connect rate when close, with no
regression in existing unit tests.

---

### M4 — Behaviour models + difficulty config cascade

**Goal:** loadable, modular AI profiles; multiple per difficulty; per-pilot profiles in
tournament mode. Finishes the unimplemented part of the 2026-08-27 plan.

**Deliverables**
1. **Complete the difficulty cascade:** give `ai_core_config` the missing params
   (`block_chance`, `aggressive_tactics`, `jump_frequency_mult`), add `difficulty` to the load
   path, and load `ai_difficulty/<level>.ini` over `ai_core.ini`. Keep `ai_core_config_get()`
   semantics for existing callers; add `ai_core_config_get_for(difficulty)`.
   > Field semantics (drop `passive_defense` & `movement_distance_pref`, fix `jump_frequency_mult`,
   > align Veteran to `ai_core.ini`, pilot-stat split) are decided in
   > `../../plans/2026-09-20-difficulty-config-and-pilot-stat-alignment.md`.
2. **Behaviour model loader** `src/game/ai/ai_behaviour.{h,c}`:
   - parses `resources/ai_config/models/<name>.json`;
   - resolves a model → (difficulty, core config, pilot personality, tactic weights, movement,
     HAR-skill overrides);
   - applies overlays through the existing `modmanager` extension path (same rules as pilots
     and HAR skills today).
3. **Wire into controller creation:** `ai_controller_create(ctrl, difficulty, pilot, pilot_id)`
   gains a model parameter (or a parallel `ai_controller_create_with_model(...)`); tournament
   mode selects the model from the pilot's configured profile. Default = legacy behaviour.
4. Ship a starter set of models (e.g. `rookie_balanced`, `veteran_balanced`,
   `veteran_zoning`, `champion_rushdown`, `ultimate_pressure`).

**Tests:** `testing/ai/ai_behaviour_test.c` (model resolution, overlay precedence, fallback to
legacy), extend `ai_core_config` tests for cascade semantics.

**Verification:** no-mod / no-model behaviour identical to today under a fixed seed (parity
suite); each shipped model loads and produces distinguishable behaviour.

---

### M5 — Scenario runner (deterministic, N trials)

**Goal:** run a behaviour model N times in a fixed scenario and aggregate behaviour.

**Deliverables**
1. `tools/ai_scenario_runner.c` (or a pytest orchestration layer) that, given a scenario spec
   (arena, HARs, model, duration, seed series), runs the engine headless `--speed=10`
   `-R`/`-P` N times, each with a distinct seed.
2. **Scenario spec** (`pytest/scenarios/*.json` or YAML): fixed setup + invariant assertions +
   metrics to collect (damage dealt, moves used, distance over time, connect rate, win rate).
3. **Metrics output** (JSON): per-trial + aggregate (mean/variance) for each metric.
4. Determinism guarantees: same seed → identical REC output (regression canary for future
   refactors).

**Tests:** a small scenario with a known outcome; determinism test (two runs, identical REC).

**Verification:** `pytest/test_ai_scenarios.py` runs each model across difficulties and
emits a summary table that the tuning loop (M7) consumes.

---

### M6 — Analysis skill + log/metric tooling

**Goal:** a repeatable "run demo & analyse" workflow.

**Deliverables**
1. A VS Code **agent skill** (`.github/prompts/skills/ai-analysis/SKILL.md` or the repo skill
   folder) that: builds via `.devcontainer/build.sh`, runs the demo or a scenario headless with
   `--log-modules=ai,tactic,rec --log-level=DEBUG`, collects the log + metrics JSON, and
   summarises behaviour (moves used, tactics chosen, anomalies).
2. A small parser (`tools/ai_log_analyse.py`) for the M0 decision trace → per-tick tactic/move
   timeline and aggregate stats.
3. Document the workflow in the project docs tree (commands, output format).

**Verification:** the skill produces a summary from a real run; a second contributor can follow
the documented steps without code archaeology.

---

### M7 — Tuning loop + regression gate

**Goal:** incrementally tweak behaviour per difficulty with confidence.

**Deliverables**
1. A **baseline snapshot** of metrics per model (committed as JSON in `testing/ai/baselines/`).
2. `run_pytest.sh` / CI integration: run the scenario suite; fail on assertion violations or on
   metric drift beyond a configured tolerance.
3. A tuning workflow: edit `models/*.json` or `ai_difficulty/*.ini` → re-run scenarios →
   diff against baseline → accept (update baseline) or revert.

**Verification:** a deliberate config change moves a metric in the expected direction; a
regression (e.g. AI stops attacking) fails the gate.

---

## 5. Mapping of the local `todo.xt` items

| todo.xt item | Milestone |
|--------------|-----------|
| debug level per module | **M0** |
| tests create a recording then verify against it | **M1**, **M2**, **M5** |
| make HARs look for a good move in each situation (when close) | **M3** |
| create an AI skill to run demo and analyse logs | **M6** |
| improve movement/behaviour, moddable models + difficulties | **M3**, **M4**, **M7** |
| testing system: model × N trials in a scenario | **M5**, **M7** |
| configurable modular loading, per-pilot tournament profiles | **M4** |
| verify new HAR move-execution system | **M2** |

---

## 6. Sequencing & Dependencies

```
M0 (logging) ────────────────────────────────┐
M1 (rec+assertions) ──► M2 (move matrix) ────┤
                                             ├─► M5 (scenario runner) ──► M7 (tuning gate)
M3 (good move when close) ─► M4 (models) ────┤        ▲
                                             └─► M6 (analysis skill) ──┘
```

- M0 and M1 are independent and can proceed in parallel.
- M3 should land before/with M4 so models have a meaningful selector to configure.
- M5 depends on M4 (models) and M1 (assertions).
- M6 depends on M0 (trace) and M5 (metrics).

**Suggested first step:** M0 + M1 together — both are small, low-risk, and immediately unblock
everything else.

---

## 7. Risks & mitigations

| Risk | Mitigation |
|------|-----------|
| Behaviour parity drift during refactor | Fixed-seed parity suites at every milestone; M5 determinism canary |
| `ai_har_event` / move execution regressions | M2 matrix as a permanent gate |
| Model config complexity grows unwieldy | Keep models as *compositions* of existing layers, not a new parallel system |
| Non-determinism from floating point / RNG | Central seed control via `REC_LOOKUP10_SETRANDOM_BYTE`; single RNG entry point |
| Verifying "what actually executed" is fuzzy | Use `ATTR_ANIMATION_ID`/`ATTR_STATE_ID` assertions (already supported) |

---

## 8. Open questions

- Should a behaviour model be selected per **pilot** (tournament) only, or also per difficulty
  in arcade mode? (Proposal: difficulty selects a *default* model, pilots may override.)
- Do we want metric **tolerance** thresholds in the regression gate, or exact equality for
  deterministic scenarios? (Proposal: exact for determinism canary; tolerance for behavioural
  metrics.)
- Should `--log-modules` be a CLI flag, an env var, or both? (Proposal: CLI flag + env fallback.)
