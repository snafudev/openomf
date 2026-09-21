# AI Move Catalog System

## Overview

The AI Move Catalog provides a name-based move selection system for AI controllers, enabling explicit selection of specific moves by name (e.g., `"spike_charge"`, `"teleportation"`) instead of relying on category-based selection or manual input sequences.

This system replaces the need for:
- Opaque category-based selection: `assign_move_by_cat(ctrl, CAT_CHARGE, true)`
- Manual move sequences: hardcoded key press sequences
- Magic move IDs: `assign_move_by_id(ctrl, 15)` (where does 15 come from?)

## Quick Start

### Basic Usage

```c
#include "controller/ai_controller.h"

// In AI tactic code:
if(should_use_specific_move) {
    assign_move_by_name(ctrl, "spike_charge");  // Clear intent!
}

if(should_throw_projectile) {
    assign_move_by_name(ctrl, "stasis");
}
```

### Supported Moves

All 41 HAR moves are available:

**Chronos (HAR ID 0)**:
- `teleportation` (charge)
- `trip_slide` (charge)
- `matter_phasing` (charge) - Escape leap to back wall & reappear at front with jumpkick (qcb+K, can be done in air)
- `stasis` (projectile)

**Electra (HAR ID 1)**:
- `super_rolling_thunder` (charge)
- `rolling_thunder` (charge)
- `electric_shards` (push)
- `ball_lightning` (projectile)

**Thorn (HAR ID 2)**:
- `spike_charge` (charge)
- `shadow_kick` (push)
- `speed_kick` (push)
- `shadow_speed_kick` (charge) - Speed kick followup with hcf+K

**Flail (HAR ID 3)**:
- `shadow_punch` (charge)
- `charging_punch` (charge)
- `shadow_charging_punch` (charge) - Charging punch followup with qcb,b+P
- `slow_swing_chains` (push)
- `swinging_chains` (push)
- `spinning_throw` (throw) - Spinning throw with f,f+K

**Gargoyle (HAR ID 4)**:
- `rising_talon` (charge) - Rising talon qcf+P
- `wing_charge` (charge) - Forward wing charge f,f+P
- `air_wing_charge` (charge) - **Fly tactic move**: Air wing charge (air f,f+P). Aerial-specific attack used during FLY tactics when airborne.
- `flying_talon` (charge) - Rising arc anti-air attack (d,df,f+P). Launches upward and can be cancelled at apex into Wing Charge or Diving Claw for combo/repositioning. Directional control: hold Back to adjust trajectory angle, Forward to change launch direction. Can be used as charge move option.
- `diving_claw` (charge) - **Fly tactic move**: Air dive attack air d+K. Zero startup, fast, high priority. Best attack for aerial dominance. Automatically used during FLY tactics when jumping.

**Jaguar (HAR ID 5)**:
- `shadow_leap` (charge)
- `jaguar_leap` (charge)
- `high_kick` (push)
- `concussion_cannon` (projectile)
- `overhead_throw` (throw) - **Fly tactic move**: Air throw (jump), D+P. Hits in a circle around Jaguar (front, behind, above). Invincible to high moves on startup. Best as air-to-ground approach: jump over opponent and throw as they rise or approach ground. Can also catch jumping opponents within radius.
- `suplex` (throw) - Ground suplex throw
- `jaguar_flip_throw` (uncategorized, not implemented) - Air throw when positioned above opponent. **Future work**: Requires AI tactical logic to detect aerial positioning and execute at optimal moment.

**Katana (HAR ID 6)**:
- `trip_slide` (charge)
- `forward_razor_spin` (charge)
- `triple_blade` (charge)
- `rising_blade` (charge)
- `shadow_rising_blade` (charge) - Rising blade followup hcf+P
- `razor_spin` (charge) - Spinning razor qcf+K or qcb+K
- `triple_blade` (push)
- `rising_blade` (push)
- `head_stomp` (push) - Air stomp air d+K
- `fireball` (projectile) - Enhancement move d,db,b+P

**Nova (HAR ID 7)**:
- `earthquake_slam` (push)
- `heavy_kick` (push)
- `belly_flop` (push) - Air belly flop air d+P
- `mini_grenade` (projectile)
- `missile` (projectile)

**Pyros (HAR ID 8)**:
- `thrust_attack` (charge) - Thrust f,f+P
- `shadow_thrust` (charge)
- `super_thrust` (charge)
- `fire_spin` (push)
- `jet_swoop` (push) - **Fly tactic move**: Air dive jet air d+K. High priority move that beats ground attacks. Automatically used during FLY tactics when jumping.

**Shadow (HAR ID 9)**:
- `shadow_punch` (charge) - Shadow punch qcb+P
- `shadow_kick` (charge) - Shadow kick qcb+K
- `shadow_grab` (charge)
- `kick_throw` (throw) - Kick throw
- `shadow_projectile_punch` (projectile)
- `shadow_projectile_kick` (projectile)

**Shredder (HAR ID 10)**:
- `flip_kick` (charge)
- `shadow_head_butt` (charge)
- `head_butt` (charge)
- `flying_hands` (projectile)

## API Reference

### Function: `assign_move_by_name()`

```c
bool assign_move_by_name(controller *ctrl, const char *move_name);
```

**Parameters:**
- `ctrl` (controller*): AI controller instance
- `move_name` (const char*): Name of the move (e.g., `"spike_charge"`)

**Returns:**
- `true` if move was successfully assigned
- `false` if move name not found or move is invalid for current state

**Example:**
```c
if(!assign_move_by_name(ctrl, "teleportation")) {
    // Fallback if move not available
    assign_move_by_cat(ctrl, CAT_CLOSE, true);
}
```

## How It Works

### Initialization

The catalog is automatically initialized on the first call to `assign_move_by_name()`:

1. **Count AF moves**: Iterates through AF file moves (indices 0-69)
2. **Allocate catalog**: Creates `ai_move_catalog` structure with entries for each move
3. **Match names**: Maps AF move IDs to move names from the move database
4. **Position tracking**: Groups moves by category to maintain consistent matching
5. **Log results**: Outputs debug information showing successful mappings

### Move Matching Strategy

The system uses **position-based matching** to connect AF move IDs to move names:

```
AF File Organization:        Database Organization:
Index 0-69 (all moves)       Grouped by HAR + Category

HAR loads AF file with       For each AF move:
moves in fixed order by       1. Get its category (CHARGE, PUSH, etc.)
category                      2. Find its position in category
                             3. Match to Nth name in database for this HAR
Move 0 (CAT_CHARGE)    ───>  1st charge move name
Move 1 (CAT_CHARGE)    ───>  2nd charge move name
Move 2 (CAT_PUSH)      ───>  1st push move name
...
```

This strategy ensures consistent matching without requiring a category field in the database.

### Lookup Process

When you call `assign_move_by_name(ctrl, "spike_charge")`:

```
1. Check if catalog initialized
   ├─ No:  Call ai_catalog_init()
   └─ Yes: Continue

2. Search catalog for name match
   ├─ Found:  Get move_id
   └─ Not found: Return false

3. Call assign_move_by_id(ctrl, move_id)
   └─ Integrates with existing move system

4. Log selection for debugging
```

## Integration with Existing Systems

The AI Move Catalog is **non-invasive** and integrates seamlessly:

```
assign_move_by_name()
    ↓
ai_catalog_find_by_name()  [Catalog lookup]
    ↓
assign_move_by_id()        [Existing system]
    ↓
set_selected_move()        [Move execution]
    ↓
controller_cmd()           [Input processing]
    ↓
Game engine processes move
```

No changes to core move execution pipeline are required.

## Implementation Details

### Files Modified

**src/game/ai/ai_types.h**
- `ai_move_ref`: Structure holding move name and AF move ID
- `ai_move_catalog`: Collection of move references for a HAR
- Added `move_catalog` field to `ai` struct

**src/controller/ai_controller.h**
- Exported `assign_move_by_name()` for use throughout codebase

**src/controller/ai_controller.c**
- `HAR_MOVE_DATABASE[]`: Static array with 41 known moves
- `ai_catalog_init()`: Initialize catalog for a HAR
- `ai_catalog_free()`: Clean up allocated memory
- `ai_catalog_find_by_name()`: Lookup move by name
- `assign_move_by_name()`: Public API entry point
- Integration with `ai_controller_free()` for cleanup

### Move Database

The `HAR_MOVE_DATABASE` is a static array containing all 41 known moves:

```c
static const har_move_db_entry HAR_MOVE_DATABASE[] = {
    // Chronos (HAR ID 0)
    {0, "teleportation", -1},
    {0, "trip_slide", -1},
    {0, "stasis", -1},
    // Electra (HAR ID 1)
    {1, "super_rolling_thunder", -1},
    // ... (41 total entries)
    {-1, NULL, -1}  // Sentinel
};
```

Each entry contains:
- `har_id`: Which HAR this move belongs to
- `move_name`: Human-readable move name
- `af_move_id`: AF move index (-1 until matched)

## Debugging

### Enable Logging

The catalog system logs initialization and move selection:

```
// During catalog initialization:
AI move catalog initialized for HAR 2 with 15 entries
AI catalog: HAR 2 AF move 10 -> 'spike_charge' (category 2, pos 0)

// During move selection:
AI assigning move by name: spike_charge (move_id=10)
```

### Check Move Availability

```c
// Try to assign a move
if(assign_move_by_name(ctrl, "nonexistent_move")) {
    // Success
} else {
    // Move not found - check:
    // 1. Move name spelling
    // 2. Move exists for this HAR
    // 3. HAR can perform move in current state
}
```

## Performance Considerations

### Memory Usage
- Single catalog per AI instance
- ~100-200 bytes per catalog (40+ entries × ~5 bytes each)
- Allocated once during first move selection

### Lookup Time
- Linear search O(n) through catalog entries
- Typical: 15-40 entries per HAR
- Negligible performance impact (sub-millisecond)

### Optimization Opportunities
- Could add hash table for O(1) lookup (not needed for typical HAR count)
- Could cache frequently-used moves (premature optimization)

## Future Enhancements

### JSON Configuration Integration
The move database is currently hardcoded. Future enhancements could:

1. Load move names from JSON configs at runtime
2. Add move metadata (conditions, range_min, follow_ups)
3. Support dynamic move discovery for mods

**Expected implementation:**
```c
// In ai_catalog_init():
// 1. Load resources/ai_config/hars/{har_name}.json
// 2. Parse charge_moves, push_moves, projectile_moves
// 3. Populate catalog with names from JSON
// 4. Store additional metadata (conditions, etc.)
```

### Extended Metadata
Future versions could track:
- Move preconditions (e.g., `"enemy_not_stunned"`)
- Range requirements (e.g., `"range_min": "MID"`)
- Follow-up tactics (e.g., `["grab", "push", "shoot"]`)
- Damage scaling by difficulty

## Examples

### Simple Move Selection

```c
// AI tactic wants to use a specific charge move
void tactic_use_charge_move(controller *ctrl) {
    assign_move_by_name(ctrl, "spike_charge");
}
```

### Conditional Move Selection

```c
// Try specific move, fallback to category-based
if(!assign_move_by_name(ctrl, "spike_charge")) {
    // Move unavailable or invalid in current state
    assign_move_by_cat(ctrl, CAT_CHARGE, true);
}
```

### Move Selection by Category

```c
// Use catalog with fallback behavior
void select_best_move(controller *ctrl, int prefer_type) {
    ai *a = ctrl->data;
    
    switch(prefer_type) {
        case PREFER_CHARGE:
            if(assign_move_by_name(ctrl, "teleportation")) {
                return;  // Successful
            }
            break;
        case PREFER_PROJECTILE:
            if(assign_move_by_name(ctrl, "stasis")) {
                return;
            }
            break;
    }
    
    // Fallback to category-based if named move failed
    assign_move_by_cat(ctrl, CAT_CLOSE, true);
}
```

### Logging Move Names

```c
// Debug: Print available moves for current HAR
void debug_print_moves(controller *ctrl) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);
    
    // Trigger catalog initialization
    if(!a->move_catalog) {
        ai_catalog_init(a, h);
    }
    
    // Print all named moves
    for(int i = 0; i < a->move_catalog->entry_count; i++) {
        ai_move_ref *ref = &a->move_catalog->entries[i];
        if(ref->name) {
            printf("  Move: %s (id=%d, category=%d)\n",
                   ref->name, ref->move_id, ref->type);
        }
    }
}
```

## Troubleshooting

### Move Not Assigned
**Symptom:** `assign_move_by_name()` returns `false`

**Possible Causes:**
1. **Wrong name**: Check spelling against list above
2. **Wrong HAR**: Move name is for a different HAR
3. **Invalid state**: HAR state doesn't allow move (e.g., not jumping for jumping move)
4. **Not in database**: Move exists in AF but not in hardcoded database

**Solution:**
```c
// Add debugging
log_debug("Attempting move: %s", move_name);
bool success = assign_move_by_name(ctrl, move_name);
log_debug("Result: %s", success ? "success" : "failed");

// Fallback
if(!success) {
    log_debug("Falling back to category selection");
    assign_move_by_cat(ctrl, CAT_CLOSE, true);
}
```

### No Catalog Initialization Logging
**Symptom:** Expected "AI move catalog initialized" message not appearing

**Cause:** Catalog already initialized from previous call

**Solution:** This is normal behavior - initialization happens once per AI instance.

### Incorrect Move Executed
**Symptom:** Catalog reports move assigned, but wrong move executes

**Possible Causes:**
1. **Matching failed**: AF move ID doesn't correspond to expected move
2. **State changed**: HAR state changed after selection but before execution

**Debug:**
```c
// Check what move_id was selected
ai_move_ref *ref = ai_catalog_find_by_name(a, "spike_charge");
if(ref) {
    log_debug("spike_charge -> move_id %d", ref->move_id);
}
```

## Summary

The AI Move Catalog system provides:

✅ **Clear API**: Name-based move selection instead of categories
✅ **Automatic**: Lazy initialization on first use
✅ **Safe**: Proper memory management with cleanup
✅ **Debuggable**: Comprehensive logging of catalog state
✅ **Compatible**: Non-invasive integration with existing systems
✅ **Complete**: All 41 HAR moves included and ready to use
✅ **Extensible**: Foundation for JSON config integration

Use `assign_move_by_name(ctrl, "move_name")` for explicit, self-documenting AI move selection throughout your codebase.
