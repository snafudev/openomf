# Difficulty Config & Pilot-Stat Alignment — Implementation Plan

**Status**: Finalized implementation; live behavior matches the intended model
**Date**: 2026-09-20
**Parent**: `../../plans/2026-09-20-ai-behaviour-models-and-testing.md` (M4)
**Scope**: Record the final difficulty semantics, keep the deterministic AI checks opt-in for
local tuning, and lock down the physical-vs-mental pilot stat split without introducing new
permanent pilot aggression flags.

> Final state: the active config model is `block_chance`, `aggressive_tactics`, and
> `jump_frequency_mult`; stale `movement_distance_pref` / `passive_defense` concepts are not in
> the live code or runtime config. Default CI remains clean; local AI tuning remains explicitly
> enabled with `OPENOMF_RUN_DETERMINISTIC_TESTS=1`.

## Background

The five "extra" difficulty fields (`block_chance`, `passive_defense`, `aggressive_tactics`,
`movement_distance_pref`, `jump_frequency_mult`) exist on disk in
`resources/ai_config/ai_difficulty/*.ini` but are **not parsed** by `ai_core_config.c`
(which only reads the six `base_*` fields). Before wiring them in, a sanity check against
`pilots.json`, `../../HAR_WIKI_INFORMATION.md` and `../../OMF2097_REFERENCE.md` surfaced several
semantic problems. This plan records the decisions and the ordered work to fix them.

## Verified facts (read the code, don't re-derive)

- `roll_chance(x)` in `ai_decision_engine.c`: **lower = more likely** (`x<=1 → always true`,
  else `rand_int(x)==1`). This is the convention behind all six `base_*` fields and
  `ai_movement_jump_chance()`.
- `roll_pref(p)`: `rand_int(200) <= p+100`, i.e. **positive = more likely** (used by all
  `pref_*` / `ap_*` / `att_*` pilot fields).
- `movement` works in discrete buckets only — `RANGE_CRAMPED/CLOSE/MID/FAR` from
  `ai_enemy_range_from_positions()` (`abs(dx)/30`). There is **no continuous distance knob**.
- Physical pilot stats are already applied in-engine (see mapping below); only the *mental*
  stats are the AI's job.

## Locked decisions

1. **`jump_frequency_mult` — keep, fix the backwards semantics.** It must behave as a true
   "higher = jump more often" multiplier under the `roll_chance` convention (i.e. applied as a
   **divisor** on the roll operand). Update comments so it is unambiguous.
2. **`movement_distance_pref` — remove.** No continuous-distance concept exists, and a single
   difficulty-scaled distance is wrong for HAR identity (Nova/Electra want mid-far, Shredder/
   Flail want close). Movement direction is already covered by pilot `pref_fwd`/`pref_back`
   and the HAR face-hug special case. Revisit later via HAR condition tags or per-pilot models.
3. **Pilot stats — audit, don't add blindly.** All physical stats must map to a knob somewhere;
   the verified mapping (below) shows they already do. Mental stats stay in pilot-AI. The
   deliverable is an audit/test that *locks* this, not new formulas.
4. **Baseline — Veteran == `ai_core.ini`.** `ai_core.ini` holds the original hardcoded
   constants (parity reference). Make `veteran.ini` a no-op so "no override" equals the
   balanced/standard difficulty. Rookie stays one notch below baseline.
5. **HAR ID — code is the source of truth.** `src/game/common_defines.h` (resource order:
   `HAR_JAGUAR=0 … HAR_NOVA=10`) is authoritative. `HAR_WIKI_INFORMATION.md` uses its own
   numbering for the old game; ignore it for any code mapping. Add a doc note so this stops
   tripping people up.
6. **`block_chance` — keep.** It is the per-difficulty block tuner. *Open question:* a small
   per-pilot block bias (e.g. high-`endurance` pilots block a bit more) — deferred, likely bad
   for balance. The current per-difficulty values are provisional and will be tuned in the M7
   loop if the blocking feel is off.
7. **`passive_defense` — remove.** Durability already comes from the HAR base stats + pilot
   `endurance`/`power`; a separate "prefer blocking" flag is redundant with `block_chance` and
   pilot `att_def`.

## Pilot stat application map (verified 2026-09-20)

| Stat | Where applied | Notes |
|------|---------------|-------|
| `power` | `calc_damage_and_stun()` (damage + stun), `har_create()` health via `power_multiplier` | single-player & tournament formulas differ |
| `agility` | `har_create()` — fwd/back speed, jump/superjump/fall speed, `stride` | `(agility+20)/30`, `(agility+35)/45` |
| `endurance` | `har_create()` — `health_max`, `endurance_max` (stun cap), `stun_factor` | also in stun cap formulas |
| `stun_resistance` | `har_create()` — `endurance_max`, `stun_factor` (tournament only) | |
| `armor` | `har_take_damage()` — damage mitigation `0.25*(2.5+armor)` (tournament) | |
| `arm_power` / `leg_power` | `calc_damage_and_stun()` — damage multiplier (tournament) | |
| `arm_speed` / `leg_speed` | `har_create()` — animation extra-string selection | |
| `att_*`, `ap_*`, `pref_*`, `learning`, `forget` | AI modules (`ai_decision_engine`, `ai_movement`, `ai_move_selector`, `ai_learning`) | the "mental" stats |

## Tasks

### T1 — Verify `ai_core_config` struct + parser

**Files:** `src/game/ai/ai_core_config.h`, `src/game/ai/ai_core_config.c`

- Confirm the active fields are `block_chance`, `aggressive_tactics`, `jump_frequency_mult`.
- Confirm `movement_distance_pref` and `passive_defense` are intentionally absent.
- Verify the cascade order (`ai_core.ini` → `ai_difficulty/<level>.ini`) and that a missing
  difficulty file remains a no-op.

**Status:** already implemented; keep as validation rather than a new feature.

**Verify:** a focused test/assertion confirms the loaded values match the intended cascade and
that the removed fields stay absent.

### T2 — Verify `jump_frequency_mult` application

**Files:** `src/controller/ai_controller.c` and the movement decision path

- Confirm the multiplier is applied as a divisor on the final jump roll operand, so higher
  multiplier means more frequent jumping under the existing `roll_chance` convention.
- Keep the semantics documented in comments so it is unambiguous.

**Status:** already implemented; confirm with a targeted arithmetic/unit test.

### T3 — Verify `block_chance` and `aggressive_tactics`

**Files:** `src/controller/ai_controller.c`, `src/game/ai/ai_tactic_engine.{h,c}`

- Confirm `block_chance` is the difficulty knob for defensive behavior and is in the block path.
- Confirm `aggressive_tactics` remains a graduated gate rather than a hard binary toggle.
- Ensure Veteran/no-override behaviour still matches the old baseline with a fixed seed.

**Status:** already implemented; keep this as the main behavioral proof pass.

**Verify:** a parity test with fixed seed compares no override vs veteran over the same setup.

### T4 — Baseline alignment

**Files:** `resources/ai_config/ai_difficulty/veteran.ini`, `resources/ai_config/ai_core.ini`

- Confirm `Veteran = ai_core.ini` is the balanced baseline.
- Ensure the docs state the baseline clearly and the rookie file remains one notch below it.

**Status:** already aligned in intent; confirm the working files and comments match it.

### T5 — Remove stale keys / stale docs

**Files:** all 7 `resources/ai_config/ai_difficulty/*.ini`,
`../../AI_DIFFICULTY_CONFIG.md`, the parent plan, and any leftover references.

- Remove any lingering `movement_distance_pref` or `passive_defense` references.
- Remove them from docs and config examples if they remain.

**Status:** cleanup pass, not a redesign.

**Verify:** grep confirms no remaining references in `resources/ai_config/` or `src/`.

### T6 — HAR ID source-of-truth note

**Files:** `../../HAR_WIKI_INFORMATION.md` and/or `../../OMF2097_REFERENCE.md`

- Add one line clarifying the in-code HAR enum is sourced from `src/game/common_defines.h` and
  the wiki numbering is not authoritative.

**Status:** documentation cleanup.

### T7 — Pilot-stat audit test

**Files:** `testing/ai/ai_pilot_stats_test.c` (new), registered in `testing/test_main.c`

- Lock the physical-stat mapping from the table above with targeted assertions in the unit-test
  path where the functions are testable.
- Assert the mental stats remain in the AI modules rather than being duplicated into HAR/game
  state math.

**Status:** still required; this is the proof layer for the split.

### T8 — Parity + smoke test

**Files:** `pytest/test_difficulty_config.py` (new), `run_pytest.sh` unchanged.

- Determinism/parity: fixed seed, no difficulty override == Veteran behaviour.
- Smoke: run the demo headless at PUNCHING_BAG vs ULTIMATE with `--demo-difficulty` and assert
  the decision trace (`LOG_MODULE_AI`) shows the expected passive/aggressive skew.
- Regression: existing `pytest/test_move_triggers.py` + `test_rec_assertions.py` still pass.

**Status:** final proof pass; required before calling this closed.

## Suggested order

T3 → T7 → T8 → T5 → T6 → T1/T2/T4 (as validation checks)

The sequence is deliberately behavioral-first: prove the live result, then clean up stale references
and lock the semantics in tests.

## Out of scope / deferred

- Per-pilot `block_chance` bias (still likely bad for balance).
- `movement_distance_pref` replacement via HAR condition tags or per-pilot models (future M4 work).
- Behaviour model loader + per-pilot model overrides (separate M4 deliverables).
