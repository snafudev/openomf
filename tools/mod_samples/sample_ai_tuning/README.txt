Sample AI tuning mod for OpenOMF.

What this changes:
- Pilot overlays:
  - Crystal (id 0): increases hyper aggression and special preference.
  - Nova pilot profile (id 9): biases more forward pressure.
- HAR overlays:
  - Jaguar: replaces charge_moves with a more aggressive set.
  - Chronos: replaces projectile_moves with a stasis-focused move.

How to use:
1. Build the zip from this folder content so manifest.ini is at zip root.
2. Place the zip into either:
   - <resource_dir>/mods (system mods), or
   - <state_dir>/mods (user mods)
3. Launch OpenOMF; overlays are applied in manifest load_order.
