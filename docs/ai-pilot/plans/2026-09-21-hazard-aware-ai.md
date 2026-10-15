# Hazard-Aware AI Plan

**Status**: in progress
**Date**: 2026-09-21
**Scope**: map hazards, hazard-aware tactic scoring, difficulty-gated hazard exploitation, and deterministic regression testing

**Implemented checkpoint**: a first deterministic fire-orb opportunity check is in place in [src/game/ai/ai_tactic_engine.c](src/game/ai/ai_tactic_engine.c), with the AI preferring `CLOSE`, `QUICK`, and `FLY` when a hazard orb is lined up between the HARs. This deliberately does not treat `GRAB` as the default orb tactic.
**Depends on**: `../AI_DIFFICULTY_CONFIG.md`, `2026-09-20-ai-behaviour-models-and-testing.md`, `2026-09-20-difficulty-config-and-pilot-stat-alignment.md`

## Goal

Make the AI use arena hazards as tactical tools at higher difficulties without making low-difficulty AI look broken or random. The target is not simply “AI triggers hazard collisions,” but “AI understands when a hazard is a good pressure or escape tool and chooses tactics accordingly.”

This plan covers:

- fire-orb exploitation in the fire pit arena
- wall-hazard pressure when the enemy is near a wall or danger zone
- spike avoidance when the AI itself is near a dangerous spike area
- deterministic validation so hazard behavior is checkable in tests rather than only by eyeballing a match

## Why this is needed

The engine already contains hooks for hazard collisions and hazard-hit events, but those are reactive rather than tactical. The current code path is basically:

- a HAR is hit by a hazard
- the AI receives `HAR_EVENT_HAZARD_HIT` / `HAR_EVENT_ENEMY_HAZARD_HIT`
- the controller chooses a follow-up tactic afterward

This is useful as a “capitalize on a bad event” response, but it does not model hazard opportunity in advance. That means the AI cannot reliably reason about a fire orb, wall hazard, or spike before stepping into danger or while placing the opponent in harm’s way.

We want the next layer to be proactive and difficulty-aware.

## Current engine signals we can build on

The following source points are the right starting point:

- [src/game/objects/har.c](src/game/objects/har.c#L1620-L1685) already contains a special-case for the fire pit orb: the AI can treat it as a punchable hazard object.
- [src/game/scenes/arena.c](src/game/scenes/arena.c#L1030-L1078) shows how hazards are spawned and how arena-specific hazard objects are created.
- [src/game/ai/ai_event.c](src/game/ai/ai_event.c#L316-L344) already reacts when the enemy is hit by a hazard, but only after the fact.
- [src/game/ai/ai_tactic_engine.c](src/game/ai/ai_tactic_engine.c) is the correct location to add hazard-aware tactic weighting.

This means the design should be hazard-scoring + tactic weighting, not just “fire a new event when touching object X.”

## Hazard categories to support

### 1. Fire orb / fire pit

The fire pit orb is the clearest case for deliberate exploitation:

- the orb floats and can be kicked/punched
- if hit correctly, it can damage the enemy
- the AI should prefer hazard use when the reward is high and the risk is acceptable
- this should be a higher-difficulty behaviour, not a beginner default

Desired behaviour:

- if the AI can do a close-range and low-risk attack while the orb is in the same lane as the enemy, raise a hazard-use bonus
- prefer safer pressure tactics such as `CLOSE` and `QUICK` when the orb is lined up; super-jump remains a broader AI consideration for positioning and escape, but it is not the default orb exploit because it can leave the HAR exposed and easy to punish
- use jump or aerial pressure only as a narrow follow-up when the lane is clean and the orb is already lined up, not as a generic opening move
- avoid this tactic when the orb is too far away or when the AI is already pressured
- emit `HAR_EVENT_HAZARD_PUNCHED` when the HAR's attack actually connects with the orb, then clear or re-evaluate the current orb tactic; this confirms a punch, not necessarily that the orb despawned. The event carries `hazard_punch_mode` (`GROUND` or `OVERHEAD`, based on whether the HAR was airborne at contact)
- a pre-contact "punchable" cue (step forward and punch / jump and punch) is deferred: `har_collide_with_hazard()` runs every tick for every HAR/hazard pair but only learns about contact once the attack hitpoints already overlap the orb, so a cue from there would fire on the same tick as `HAR_EVENT_HAZARD_PUNCHED`. It needs a separate reach check during HAR updates
- make orb exploitation a moddable per-difficulty behavior, controlled by a dedicated `fire_orb_exploitation_enabled` setting in `ai_core.ini` with per-difficulty overrides in `ai_difficulty/*.ini`; do not couple it to `aggressive_tactics` or a hard-coded minimum difficulty

This is now the implemented baseline: the orb-lining check is simple, map-aware enough to be deterministic, and intentionally avoids the earlier “grab when lined up” mistake.

### 2. Wall hazards

Some maps have hazard geometry or wall-adjacent danger that makes the enemy near a wall more vulnerable.

Desired behaviour:

- when the enemy is close to a wall and the hazard encourages a boundary trap, increase pressure and corner tactics
- when the AI itself is trapped near the wall under threat, prefer a reset / escape / keep-distance response rather than a reckless charge

The rule here is not “attack near walls” in general; it is “understand which wall states are advantageous and which are dangerous.”

### 3. Spike danger zones

For spike-heavy arenas, the AI should avoid standing in or walking into the spike actively.

Desired behaviour:

- if the spike is close to the AI’s feet or crossing the safe zone, prefer jump, dodge, retreat, or a delayed attack to keep distance
- if the enemy is being pressured into the spike, increase attack pressure if the combination is safe

This is not just hazard awareness; it is self-preservation and map control.

## Design principle

Hazard use is a tactical weighting problem, not a collision-only event problem.

The intended flow is:

1. read hazard context from the arena / match state
2. compute a hazard-opportunity score for the current AI controller
3. apply that score to tactic ranking
4. use moddable per-difficulty behavior toggles to gate hazard scoring and event-driven actions
5. use HAR events for precise opportunities and confirmed hazard interactions
6. back the behavior with deterministic tests

## Proposed AI hooks

Add a hazard-context helper layer, likely near the tactic engine and/or event layer:

- `ai_hazard_context_fire_orb_bonus(ctrl)`
- `ai_hazard_context_wall_pressure_bonus(ctrl)`
- `ai_hazard_context_spike_danger_penalty(ctrl)`
- `ai_hazard_apply_tactic_bias(ctrl, tactic_id, score)`

These functions should return additive score values, not hard-coded tactic assignments.

This keeps the system extensible and makes it easy to tune by arena type and difficulty without writing map-specific logic everywhere.

## Difficulty configuration

Hazard behavior must be independently moddable for each difficulty through `resources/ai_config/ai_core.ini` and `resources/ai_config/ai_difficulty/[difficulty].ini`. Add a dedicated `fire_orb_exploitation_enabled` setting with a shared default and difficulty-specific overrides. The setting gates both orb opportunity scoring and responses to orb events, so a difficulty with the behavior disabled neither seeks nor reacts to these opportunities.

Choose default values that preserve the intended difficulty progression, but do not make difficulty-number checks the source of truth. Modders must be able to enable or disable orb exploitation on any individual difficulty without changing code or unrelated hazard behaviors. Keep tuning weights separate from the boolean enable toggle.

Apply the same config-driven pattern to other hazard behaviors when they are introduced, using separate settings so orb use, wall pressure, and spike avoidance can be toggled independently.

## Deterministic testing approach

Hazard behavior needs a deterministic regression suite before it is considered valid.

### Tests to add

1. Fire orb opportunity test
   - setup: orb exploitation enabled for the selected difficulty, enemy within orb assist range, orb in the same lane, AI is in a valid attacking state
   - expect: hazard-opportunity score is positive and close-range tactics are preferred

2. Fire orb difficulty-toggle test
   - run the same opportunity setup with `fire_orb_exploitation_enabled` on and off for a difficulty
   - expect: enabled allows orb scoring and event responses; disabled suppresses both, regardless of the difficulty number

3. Fire orb punched event test (`testing/ai/ai_fire_orb_test.c`)
   - setup: synthetic orb and HAR attack frame driven through the real `har_collide_with_hazard()`, grounded and airborne
   - expect: exactly one `HAR_EVENT_HAZARD_PUNCHED` with the correct mode, the orb chains to its punched animation, out-of-reach attacks fire nothing, and an already-punched orb cannot be re-punched
   - expect: the AI clears its orb tactic when the difficulty toggle is on and keeps it when off, without assuming the orb object was destroyed

4. Spike avoidance test
   - setup: AI is near the spike danger zone
   - expect: escape / jump / dodge preference outranks attack preference

5. Wall-pressure test
   - setup: enemy is near a wall hazard zone and the AI is in a valid pressure state
   - expect: pressure tactic weight increases

6. No-op safety test
   - setup: no hazard context, no wall danger, no spike danger
   - expect: base AI tactics are unchanged

These are pure logic tests and should not depend on visual gameplay playback.

### Files to update

- [src/game/ai/ai_core_config.c](src/game/ai/ai_core_config.c) and [src/game/ai/ai_core_config.h](src/game/ai/ai_core_config.h) for parsing and exposing the per-difficulty enable setting
- [resources/ai_config/ai_core.ini](resources/ai_config/ai_core.ini) and [resources/ai_config/ai_difficulty/](resources/ai_config/ai_difficulty/) for the shared default and difficulty overrides
- [testing/ai/ai_tactic_engine_test.c](testing/ai/ai_tactic_engine_test.c) for config-gated orb scoring and opportunity behavior
- [testing/ai/ai_event_test.c](testing/ai/ai_event_test.c)
- [testing/ai/ai_fire_orb_test.c](testing/ai/ai_fire_orb_test.c) for the collision-to-event-to-AI orb punch path
- HAR event definitions and dispatch in [src/game/objects/har.h](src/game/objects/har.h), [src/game/objects/har.c](src/game/objects/har.c), and [src/controller/ai_controller.c](src/controller/ai_controller.c)

## Intended implementation order

1. Add the hazard-context scoring helpers and hook them into the tactic ranking pipeline.
2. Add the failing deterministic tests for fire orb, wall pressure, and spike avoidance.
3. Add the moddable per-difficulty orb enable setting and verify its defaults/overrides.
4. Implement the confirmed-punch event and handle it in the AI controller to clear orb tactics. (done; a pre-contact punchable cue is deferred until there is a reach check in the HAR update path)
5. Implement the fire-orb bonus, gated by the config setting. (the initial scoring pass is done; keep the score subtle)
6. Implement wall-pressure and spike-danger weighting with their own independent config settings.
7. Tune the numbers against a few real matches and deterministic scenarios.
8. Re-run the AI deterministic suite plus a runtime sanity check with the AI log filter enabled.

## Acceptance criteria

The feature is considered complete when all of these are true:

- high-difficulty AI uses fire orb / wall / spike contexts opportunistically
- low-difficulty AI does not become erratic or “hazard-bot” behavior
- each hazard behavior can be independently enabled or disabled per difficulty through moddable config files
- a confirmed orb-punch event distinguishes grounded punches from jump-punches and invalidates stale orb tactics
- map-specific hazard logic is scored rather than hard-coded to one arena
- hazard behavior is covered by deterministic AI tests
- tuning and enablement are possible through config or scoring weights without needing a bespoke hard-coded patch for each map

## Risks and cautions

- Do not hard-code every arena individually unless absolutely necessary. Use hazard categories and arena context instead.
- Do not treat hazard use as always “good”; some hazards are dangerous to the AI itself.
- Do not rely only on visual review. The hazard logic needs deterministic tests because this is exactly the kind of behavior that can look reasonable in one match and be broken in the next.
- Keep the scoring subtle at lower difficulty. The goal is smarter AI, not random hazard abuse.

## Conclusion

This is a natural extension of the current AI system: the engine already knows when hazards hit and where they are spawned. The next step is to use that map-aware context as a tactical decision input, especially at higher difficulty levels, while preserving low-difficulty stability and a deterministic validation path.
yes