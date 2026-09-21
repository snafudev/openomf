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
4. use higher-difficulty thresholds to gate the hazard score
5. back the behavior with deterministic tests

## Proposed AI hooks

Add a hazard-context helper layer, likely near the tactic engine and/or event layer:

- `ai_hazard_context_fire_orb_bonus(ctrl)`
- `ai_hazard_context_wall_pressure_bonus(ctrl)`
- `ai_hazard_context_spike_danger_penalty(ctrl)`
- `ai_hazard_apply_tactic_bias(ctrl, tactic_id, score)`

These functions should return additive score values, not hard-coded tactic assignments.

This keeps the system extensible and makes it easy to tune by arena type and difficulty without writing map-specific logic everywhere.

## Difficulty gating

Hazard exploitation should be gated by difficulty, roughly as follows:

- low / rookie: hazard use is mostly ignored unless the AI is already in a severe threat state
- veteran / world class: occasional orb and wall-trap exploitation
- champion / deadly / ultimate: hazard pressure becomes a meaningful part of the tactical profile

This matches the project’s existing difficulty framework and keeps high-difficulty AI feeling smarter without making low difficulties too chaotic.

## Deterministic testing approach

Hazard behavior needs a deterministic regression suite before it is considered valid.

### Tests to add

1. Fire orb opportunity test
   - setup: difficulty high, enemy within orb assist range, orb in the same lane, AI is in a valid attacking state
   - expect: hazard-opportunity score is positive and close-range tactics are preferred

2. Fire orb low-difficulty suppression test
   - same setup, but low difficulty
   - expect: hazard bonus is absent or low enough that normal close-range play still dominates

3. Spike avoidance test
   - setup: AI is near the spike danger zone
   - expect: escape / jump / dodge preference outranks attack preference

4. Wall-pressure test
   - setup: enemy is near a wall hazard zone and the AI is in a valid pressure state
   - expect: pressure tactic weight increases

5. No-op safety test
   - setup: no hazard context, no wall danger, no spike danger
   - expect: base AI tactics are unchanged

These are pure logic tests and should not depend on visual gameplay playback.

### Files to update

- [testing/ai/ai_event_test.c](testing/ai/ai_event_test.c)
- optionally a dedicated hazard-focused test file if the suite becomes large enough

## Intended implementation order

1. Add the hazard-context scoring helpers and hook them into the tactic ranking pipeline.
2. Add the failing deterministic tests for fire orb, wall pressure, and spike avoidance.
3. Implement the fire-orb bonus and low-difficulty gating. (done in the initial pass; keep the scoring subtle and difficulty-gated)
4. Implement wall-pressure and spike-danger weighting.
5. Tune the numbers against a few real matches and deterministic scenarios.
6. Re-run the AI deterministic suite plus a runtime sanity check with the AI log filter enabled.

## Acceptance criteria

The feature is considered complete when all of these are true:

- high-difficulty AI uses fire orb / wall / spike contexts opportunistically
- low-difficulty AI does not become erratic or “hazard-bot” behavior
- map-specific hazard logic is scored rather than hard-coded to one arena
- hazard behavior is covered by deterministic AI tests
- tuning is possible through config or scoring weights without needing a bespoke hard-coded patch for each map

## Risks and cautions

- Do not hard-code every arena individually unless absolutely necessary. Use hazard categories and arena context instead.
- Do not treat hazard use as always “good”; some hazards are dangerous to the AI itself.
- Do not rely only on visual review. The hazard logic needs deterministic tests because this is exactly the kind of behavior that can look reasonable in one match and be broken in the next.
- Keep the scoring subtle at lower difficulty. The goal is smarter AI, not random hazard abuse.

## Conclusion

This is a natural extension of the current AI system: the engine already knows when hazards hit and where they are spawned. The next step is to use that map-aware context as a tactical decision input, especially at higher difficulty levels, while preserving low-difficulty stability and a deterministic validation path.
yes