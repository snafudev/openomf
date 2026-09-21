# OpenOMF AI Release Summary

This release is compared against the repository's mainline branch, `master`.

## At a glance

This branch introduces the new `ai-pilot-v2` system alongside the related logging and test-recording improvements. The main themes are:

- a new config-driven AI pilot system
- hazard-aware tactical scoring
- anti-spam cooldown logic and difficulty gating
- richer logging and AI test-record analysis
- deterministic regression coverage for AI and recording behavior

The delta includes 124 changed files, with 23,981 insertions and 1,760 deletions.

## AI changes: ai-pilot-v2

This is the main category for the release and covers the new gameplay-facing AI system.

The new `ai-pilot-v2` stack replaces the older controller-coupled behavior with a modular, config-driven AI framework built around HAR personality, pilot tuning, and difficulty-aware tactic selection.

### AI architecture and configuration

- Introduced the new `ai-pilot-v2` architecture by splitting the AI logic out of the controller into dedicated modules such as:
  - `ai_decision_engine`
  - `ai_move_selector`
  - `ai_movement`
  - `ai_state`
  - `ai_learning`
  - `ai_tactic_engine`
  - `ai_har_skills`
- Converted HAR and tactic behavior to configuration-driven data loaded from `resources/ai_config/`.
- Made the AI system explicitly moddable through config files, including:
  - `resources/ai_config/pilots.json`
  - `resources/ai_config/tactics.json`
  - `resources/ai_config/hars/*.json`
  - `resources/ai_config/ai_core.ini`
  - `resources/ai_config/ai_difficulty/*.ini`
- Added support for pilot personalities, difficulty profiles, and HAR-specific skill data.
- Added mod-friendly config overlays and load-order handling so tuning can be layered and overridden by content mods.

### Tactic and difficulty tuning

- Added a registry-based tactic engine with configurable move selection and validation.
- Introduced pilot and difficulty scaling to make AI behavior more readable and tunable.
- Made the tactical system moddable via data files rather than hardcoded logic, including tactic weights, pilot profile values, difficulty thresholds, and HAR-specific move behavior.
- Added hazard-aware scoring for tactical situations, including:
  - fire orb opportunities
  - wall-pressure windows
  - spike danger avoidance
- Added gating so higher-difficulty AI can take more aggressive hazard-driven decisions without destabilizing lower difficulties.
- Added cooldown logic to prevent projectile and pressure spam.
- Added ranged and charge tactic gating based on active cooldown conditions.

### Defensive and behavior improvements

- Reduced bad repeated attacks and pressure loops via anti-spam AI learning adjustments.
- Improved logic for when to pressure, retreat, jump, or charge based on range and state.
- Made HAR-specific move execution and conditions configurable per character setup via the config-driven HAR definitions.
- Kept the system testable through deterministic AI evaluation and regression tests.

### Documentation and planning

- Added the AI architecture docs and migration/design writeups for the refactor.
- Added tuning docs covering difficulty config, pilot alignment, and HAR behavior modeling.
- Added a dedicated hazard-aware AI plan describing safer, map-aware tactical opportunities.

## Logging / test recording changes

This category covers the operational tooling and verification infrastructure around AI behavior and match recordings.

### Logging and analysis

- Added structured AI logging support and module filtering for tactical debug output.
- Added AI log analysis tooling to process and inspect recorded sessions.
- Expanded logging to make AI decision paths easier to debug and tune.
- Added tests for log-module behavior and logging-related features.

### Test recording and deterministic validation

- Added recording-focused pytest support and harnesses for AI regression analysis.
- Added test utilities to assert and validate recorded move triggers and AI behavior.
- Added deterministic harnessing for AI replay/record validation.
- Added regression tests covering:
  - AI tactic baselines
  - move triggers
  - rec assertions
  - config overlay behavior
  - AI learning and event logic
- Included automated test support for AI tuning workflows and demo/autoload validation.

## Supporting work

This release also includes supporting documentation, build/test wiring, and config additions that make the AI work easier to maintain:

- project instructions and AI-pilot guidance files
- CMake/test configuration updates
- example AI tuning mod content and sample mod resources
- build verification and general documentation cleanup

## Release summary

Overall, this release introduces the new `ai-pilot-v2` system and brings the AI substantially more in line with a data-driven, tunable, and testable gameplay framework. The key point is that this is no longer just a refactor: it is a configurable AI platform. Pilot personalities, tactic weights, HAR skill behavior, difficulty thresholds, hazard handling, and cooldown gating are all exposed as moddable data surfaces that can be tuned without rewriting core C logic.
