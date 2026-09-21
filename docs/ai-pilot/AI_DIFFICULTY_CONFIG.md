# AI Difficulty-Specific Configuration

## Overview

The AI system now supports full difficulty-specific configuration, allowing behavior to be customized per difficulty level. This gives modders complete control over AI aggressiveness, defensive posture, movement patterns, and attack frequency across 7 difficulty levels.

## Difficulty Levels

The system supports 7 difficulty levels, each with customizable behavior:

1. **PUNCHING BAG (0)** - Extremely passive, mostly blocks and takes hits
2. **ROOKIE (1)** - Below average, somewhat defensive
3. **VETERAN (2)** - Balanced difficulty (baseline)
4. **WORLD CLASS (3)** - Aggressive with good tactics
5. **CHAMPION (4)** - Very aggressive, expert tactics
6. **DEADLY (5)** - Maximum expert tactics with relentless pressure
7. **ULTIMATE (6)** - Absolute maximum aggression

## Configuration File Structure

### Base Configuration

**File:** `resources/ai_config/ai_core.ini`

Contains default parameters that apply to all difficulties if not overridden.

**Example:**
```ini
base_act_chance = 5
random_attack_chance = 10
base_act_timer = 28
```

### Difficulty-Specific Overrides

**Location:** `resources/ai_config/ai_difficulty/[difficulty].ini`

Each difficulty level has its own config file that overrides base parameters. Only include parameters you want to change:

**Example: `ai_difficulty/punching_bag.ini`**
```ini
# Change these from base defaults:
base_act_chance = 25        # Less frequent action changes
random_attack_chance = 60   # Rarely attacks
block_chance = 70           # Frequently blocks
```

## Configurable Parameters

### Movement & Action Parameters

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `base_act_chance` | int | 1-100 | Likelihood to change movement action. Lower = more frequent changes. |
| `base_fwd_jump_chance` | int | 1-100 | Likelihood to jump while moving forward. Lower = more frequent jumps. |
| `base_back_jump_chance` | int | 1-100 | Likelihood to jump while moving backward. Lower = more frequent jumps. |
| `base_still_jump_chance` | int | 1-100 | Likelihood to jump while standing still. Lower = more frequent jumps. |
| `base_act_timer` | int | 10-100 | Duration (frames) to maintain current action. |

### Combat & Tactical Parameters

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `random_attack_chance` | int | 1-100 | Likelihood to attempt a move/attack. Lower = more aggressive. |
| `block_chance` | int | 0-100 | Percentage likelihood to block instead of attacking. Higher = block more often. |
| `aggressive_tactics` | int | 0-1 | 0 = default tactic gating, 1 = also allow advanced tactics (e.g. COUNTER). |

### Advanced Parameters

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `jump_frequency_mult` | int | 10-300 | Jump frequency multiplier, applied as a divisor on the roll chance. Higher = jump more often (100 = normal). |

## How It Works

### Loading Cascade

When an AI controller is created:

1. Load base config from `ai_core.ini`
2. Load difficulty-specific config from `ai_difficulty/[difficulty].ini`
3. Difficulty-specific values override base values
4. AI uses merged configuration for all behavior

### Parameter Application

All AI behavior checks use these parameters:

- **Action Changes:** Checked every frame, uses `base_act_chance`
- **Jump Decisions:** Uses jump_chance parameters based on movement state
- **Attack Frequency:** Checked per decision cycle, uses `random_attack_chance`
- **Blocking:** Probabilistic check during combat, uses `block_chance`

## Modding Guide

### Creating a Custom Difficulty Config

1. Create a new file: `resources/ai_config/ai_difficulty/custom_name.ini`
2. Add parameters you want to customize
3. Rebuild the game (no code changes needed, just file addition)

### Recommended Parameter Progressions

**For Passive → Aggressive Progression:**

```
PUNCHING_BAG:  base_act_chance = 25
ROOKIE:        base_act_chance = 15
VETERAN:       base_act_chance = 8
WORLD_CLASS:   base_act_chance = 4
CHAMPION:      base_act_chance = 2
DEADLY:        base_act_chance = 1
ULTIMATE:      base_act_chance = 1
```

**For Attack Frequency:**

```
PUNCHING_BAG:  random_attack_chance = 60
ROOKIE:        random_attack_chance = 35
VETERAN:       random_attack_chance = 15
WORLD_CLASS:   random_attack_chance = 8
CHAMPION:      random_attack_chance = 4
DEADLY:        random_attack_chance = 2
ULTIMATE:      random_attack_chance = 1
```

### Tweaking Individual Difficulties

To make VETERAN more defensive:
```ini
# resources/ai_config/ai_difficulty/veteran.ini
block_chance = 25           # Increased from 15
```

To make ROOKIE more aggressive:
```ini
# resources/ai_config/ai_difficulty/rookie.ini
base_act_chance = 8         # Reduced from 15
random_attack_chance = 20   # Reduced from 35
```

## Testing Changes

1. Edit a difficulty config file in `resources/ai_config/ai_difficulty/`
2. Start a match at that difficulty level
3. Observe AI behavior (movement frequency, block behavior, attacks)
4. Adjust parameters and repeat

### Local AI pilot deterministic runs

Deterministic AI pilot tests are intentionally opt-in and are not part of the default CI suite. While actively tuning AI behavior, enable the local-only test gate with:

```bash
OPENOMF_RUN_DETERMINISTIC_TESTS=1 ./build/openomf_test_main
```

This keeps normal CI runs stable while still allowing focused AI pilot validation during active development.

## Deterministic AI verification matrix

The deterministic test suite is the primary guardrail for AI balance work. When a tuning change affects aggression, projectile usage, range decisions, or tactical pressure, it should be backed by one of the tests below:

### Core suites

- `testing/ai/ai_learning_test.c`
  - Verifies learning counters, projectile burst cooldowns, pressure-burst limits, and difficulty-sensitive adaptation.
  - These tests protect against regressions like repeated projectile spam or repeated charge pressure loops.

- `testing/ai/ai_tactic_engine_test.c`
  - Verifies tactic enable/disable logic, range gating, condition matching, and special-case shoot logic such as Shadow projectile preference and Shredder medium-range-only attack behavior.
  - This is the best place to tune whether a HAR should shoot, close, trip, fly, or charge in a given range/state.

- `testing/ai/ai_har_skills_test.c`
  - Verifies each HAR has the correct charge, push, projectile, and trip moves configured, and that the JSON-loaded move arrays match expectations for the relevant character.
  - Use this to verify that a balance change did not break a HAR’s move profile.

- `testing/ai/ai_state_test.c` and related AI tests
  - Verifies base state reset, tactic cleanup, and difficulty scaling assumptions.

### How to run the suite

```bash
cd /work
cmake --build build --target openomf_test_main
OPENOMF_RUN_DETERMINISTIC_TESTS=1 ./build/openomf_test_main
```

### What a good balance fix should include

Before merging a tuning change, check which category the issue falls into:

- Range logic: modify `resources/ai_config/hars/*.json` and/or `ai_tactic_should_use_shoot()` in `src/game/ai/ai_tactic_engine.c`
- Pressure loop / spam: modify `ai_projectile_*` and `ai_pressure_*` logic in `src/game/ai/ai_learning.c`
- Personality / preference bias: modify pilot stats in `resources/ai_config/pilots.json` or the pilot adaptation logic in `src/game/ai/ai_learning.c`
- Tactic enablement / decision gating: modify `resources/ai_config/tactics.json` or the gate logic in `src/game/ai/ai_tactic_engine.c`

## AI tuning knobs for modders and developers

The AI is designed so that balance changes should usually happen in configuration or data files before hard-coding behavior in C.

### 1. Tactic selection and default behavior

File: `resources/ai_config/tactics.json`

Use this to:

- enable or disable a tactic globally
- change the implied default move/attack type for that tactic
- adjust which tactics are considered by default in a match

This is the first place to look when a HAR is choosing the wrong family of attacks.

### 2. HAR-specific move profile

Files: `resources/ai_config/hars/*.json`

Use this to:

- set move sequence strings
- set `range_min` / `range_max`
- change move conditions such as `high_difficulty`, `special_preferred`, `jump_preferred`, `enemy_stunned`
- add or remove charge / push / projectile / trip moves for a specific HAR

This is the most important place for character balance work. For example, Shredder’s projectile is intentionally limited to a medium-range band, so the JSON `range_max` and the move-specific shoot logic should be tuned together.

### 3. Pilot personality tuning

File: `resources/ai_config/pilots.json`

Use this to adjust:

- `att_sniper`, `att_hyper`, `att_def`, `att_jump`
- `pref_fwd`, `pref_back`, `pref_jump`
- `ap_special`, `ap_low`, and other pilot preference values

This is the right knob for “this pilot wants to zone / pressure / counter more often” without changing the underlying HAR move list.

### 4. Burst and cooldown tuning

Files:

- `src/game/ai/ai_learning.c`
- `src/game/ai/ai_learning.h`

These contain the repeated-pressure limits that prevent spam loops. The key functions are:

- `ai_projectile_max_streak()`
- `ai_projectile_is_allowed()`
- `ai_projectile_use()`
- `ai_pressure_max_streak()`
- `ai_pressure_is_allowed()`
- `ai_pressure_use()`

If the issue is “this HAR keeps repeating the same attack forever,” this is the place to fix it.

### 5. Tactical decision gating

File: `src/game/ai/ai_tactic_engine.c`

This is the runtime gate for questions like:

- should this HAR shoot right now?
- is the enemy too cramped for ranged attacks?
- does this HAR need a special-case projectile rule?
- should a charge or push be blocked by burst pressure?

When a balance problem is not obviously a config issue, this is the next place to inspect.

### Recommended workflow for tuning a balance issue

1. Reproduce with the deterministic AI suite or a focused demo.
2. Decide whether the issue is range, pressure, or personality.
3. Start with the smallest relevant config/jargon knob:
   - range: HAR JSON
   - tactic family: `tactics.json`
   - behavior bias: pilot JSON
   - spam loop: `ai_learning.c`
4. Add or update a deterministic regression test before and after the fix.
5. Re-run the AI suite and confirm the change is stable.

### Observation Checklist

- ✓ How often does AI change actions?
- ✓ Does it jump while moving/standing?
- ✓ How frequently does it attack?
- ✓ Does it block or counter-attack?
- ✓ Does it keep distance or rush in?

## Future Enhancements

### Per-HAR Difficulty Overrides

HAR JSON files can include difficulty-specific move overrides:

```json
{
  "id": 0,
  "name": "chronos",
  "charge_moves": [...],
  "difficulty_overrides": {
    "rookie": {
      "charge_moves": [
        {"name": "spike_charge", "conditions": ["low_difficulty"]}
      ]
    }
  }
}
```

### Per-Pilot Customization

Custom pilots could have their own behavior tweaks (not yet implemented).

## Special Pilot and Tournament AI Models

This document covers the difficulty-layer tuning knobs. It is separate from the special-pilot or tournament-model layer.

A custom AI model is not a difficulty file: it is a named behavioural profile that selects the tactical personality for a particular pilot, HAR, or pilot+HAR combination. The intended precedence is:

1. pilot-specific model override (for bosses or tournament specials)
2. HAR-specific model bias
3. difficulty-based model selection
4. generic fallback profile

This means a boss like Major Kreissack can keep a custom aggression/blocking profile even while the base difficulty cascade still controls the normal difficulty scaling for everyone else.

### Intended file layout

Use a distinct model folder for named profiles rather than mixing them into the difficulty INI files:

```text
resources/ai_config/
  ai_core.ini
  ai_difficulty/
    punching_bag.ini
    rookie.ini
    veteran.ini
    ...
  models/
    veteran_balanced.json
    boss_kreissack.json
    boss_nova.json
```

### Example model file

A model file defines a named AI personality and can override the base difficulty model for a specific pilot or HAR.

```json
{
  "id": "boss_kreissack",
  "name": "major_kreissack_nova",
  "inherits": "veteran_balanced",
  "pilot_overrides": ["kreissack"],
  "har_overrides": ["nova"],
  "difficulty_overrides": {
    "veteran": {
      "block_chance": 18,
      "aggressive_tactics": 1,
      "jump_frequency_mult": 120
    },
    "ultimate": {
      "block_chance": 22,
      "aggressive_tactics": 1,
      "jump_frequency_mult": 140
    }
  },
  "tactic_biases": {
    "pressure": 1.3,
    "zoning": 1.1,
    "counter": 1.2
  }
}
```

### How this differs from difficulty config

- `ai_difficulty/*.ini` answers: "How should this model behave at each difficulty level?"
- `models/*.json` answers: "Which tactical personality should this pilot / HAR use?"

A special pilot or tournament boss should therefore use a model file to select a custom profile while still inheriting the normal difficulty cascade for values that are not overridden.

### Example: Major Kreissack / Nova

The intended setup for a boss identity is:

- pilot: `kreissack`
- HAR: `nova`
- model: `boss_kreissack`
- base difficulty profile still comes from the active difficulty file
- override layer adds boss pressure, more durable blocking, and projectile-heavy timing

This is the correct place to describe a special end-game pilot profile and a distinct Nova boss archetype without confusing it with general difficulty tuning.

### Tournament-pilot override pattern

For tournament or scripted battles, the custom model should be selected by pilot identity before falling back to the default difficulty model. In practice, the selection stack is:

```text
selected_model = pilot_override || har_override || difficulty_model || default_model
```

This keeps the usual difficulty-based path intact for normal pilots while allowing special boss pilots to opt into a unique tactical profile.

## Technical Notes

- Config files use INI format (simple `key = value` lines)
- Comments start with `#`
- Empty lines are ignored
- Only changed parameters need to be in difficulty files
- Files are loaded at AI controller creation time
- All AIs in a match share the same difficulty configuration
- Special pilot models are a separate model-selection layer from difficulty tuning and should not be mixed into the INI difficulty files

## Troubleshooting

**AI not responding to config changes:**
- Ensure config files are in `resources/ai_config/ai_difficulty/`
- Check file names match difficulty level (e.g., `veteran.ini`)
- Rebuild after adding new difficulty files

**AI behavior unchanged:**
- Verify the parameter exists (check this document)
- Check config file syntax (one parameter per line, format: `key = value`)
- Ensure the value is within valid range

**All difficulties feel the same:**
- Increase the spread between difficulty levels
- Check that ULTIMATE has much lower `base_act_chance` than PUNCHING_BAG
- Verify aggressive_tactics is enabled for harder difficulties
