# AF Move String Reference & Known Discrepancies

This document records how the game's `.AF` move command strings work, how the AI
HAR-skill executor must drive them, and the discrepancies we found between the
AF data, the HAR wiki, and the AI config files while debugging why Shadow never
fired its projectile moves.

---

## 1. Move string convention

Each move's input command is stored in `af_move.move_string` (see
`src/formats/move.h`, parsed in `src/game/objects/har.c:match_move()`).

- The string is `[P|K][motion]`.
- The motion part uses numpad notation: `2`=down, `8`=up, `4`=back, `6`=forward,
  `1`=down-back, `3`=down-forward, `5`=neutral, etc.
- The motion is written in **input-buffer order (most recent first)**, which is
  the **reverse of execution order**.

Examples:

| Move        | Execution order | move_string |
|-------------|-----------------|-------------|
| qcb+P       | 2,1,4 (d,db,b)  | `P41`       |
| qcf+P       | 2,3,6 (d,df,f)  | `P63`       |
| f,f+P       | 6,5,6           | `P656`      |
| d,d+P       | 2,5,2           | `P252`      |
| qcb+K       | 2,1,4           | `K41`       |
| hcf+P       | 6,3,2,1,4       | `P6321` (or `P63214`) |

## 2. Input matching (`har_act` / `match_move`)

`har_act()` pushes each directional input onto a newest-first buffer
(`har.inputs`), deduplicating repeats. A **bare** punch/kick press pushes a
neutral `5` onto the buffer (to separate it from previous inputs).

`match_move()` then compares `move_string[1..]` against the buffer prefix.

Consequence: a motion+button move only matches if the button is pressed **while
the motion's final direction is still held**, so no neutral `5` is inserted.
This is why players hold the stick at the end of the motion while pressing the
button.

## 3. The Shadow projectile bug

Shadow is `FIGHTR1.AF` (`fighter_id` 1). Its relevant moves:

| Move        | AF move id | move_string | category |
|-------------|-----------|-------------|----------|
| Shadow Punch (qcb+P) | 24 | `P41` | LOW (4) |
| Shadow Kick (qcb+K)  | 21 | `K41` | LOW (4) |
| Shadow Grab (d,d+P)  | 16 | `P252` | MEDIUM (5) |
| projectile spawns    | 35, 53 | `!` | PROJECTILE (8) |

The AI config (`resources/ai_config/hars/shadow.json`) sends the projectile
sequences as discrete events ending in a **bare** `P`/`K`:

```
D, DB, B, P   ->   qcb+P
```

The bare `P` inserted a neutral `5` into the input buffer, so the motion never
matched `P41`/`K41`. The AI ended up waving a standing punch/kick at range
instead of firing the projectile.

The actual projectile objects (moves 35/53) are `CAT_PROJECTILE` moves with a
`!` move string — they are spawned by the attack animation, not by raw input.

## 4. Fix

Added `ai_resolve_move_input()` in `src/game/ai/ai_har_skills.c`:

- When an input is a bare `P`/`K` and a direction was previously held, the
  button is combined with that direction (e.g. `P` becomes `B+P`). This removes
  the spurious neutral and lets the move match.
- The move executor `ai_har_execute_move_list()` now tracks the last held
  direction and applies this to every input; a neutral (`5`) resets the held
  direction so charge moves still work.

Regression tests were added in `testing/ai/ai_move_executor_test.c`.

## 5. HAR id ↔ AF file mapping (0-indexed)

The `HAR_*` enum in `src/game/common_defines.h` is 0-indexed and maps directly
to the `FIGHTR<n>.AF` files:

| HAR            | enum value | AF file     |
|----------------|-----------|-------------|
| Jaguar         | 0         | FIGHTR0.AF  |
| Shadow         | 1         | FIGHTR1.AF  |
| Thorn          | 2         | FIGHTR2.AF  |
| Pyros          | 3         | FIGHTR3.AF  |
| Electra        | 4         | FIGHTR4.AF  |
| Katana         | 5         | FIGHTR5.AF  |
| Shredder       | 6         | FIGHTR6.AF  |
| Flail          | 7         | FIGHTR7.AF  |
| Gargoyle       | 8         | FIGHTR8.AF  |
| Chronos        | 9         | FIGHTR9.AF  |
| Nova           | 10        | FIGHTR10.AF |

Note: `../../HAR_WIKI_INFORMATION.md` and `../../AI_MOVE_CATALOG.md` use a
1-based "HAR ID" (e.g. "Shadow (HAR ID 9)"). Do not confuse the two numbering
schemes.

To dump a fighter's move strings for cross-checking:

```sh
./build/aftool -f build/resources/FIGHTR1.AF -m <id> -k move_string
./build/aftool -f build/resources/FIGHTR1.AF -m <id> -k category
```

## 6. Audit results & discrepancies fixed

Cross-referenced every `resources/ai_config/hars/*.json` sequence against its
fighter's AF move strings.

| HAR | config move | sequence | AF match | status |
|-----|-------------|----------|----------|--------|
| Shadow | shadow_projectile_punch | D,DB,B,P | `P41` | fixed by executor |
| Shadow | shadow_projectile_kick | D,DB,B,K | `K41` | fixed by executor |
| Electra | super_rolling_thunder | B,DB,D,DF,F,F+P | none | corrected to D,DF,F,F+P (`P63`) |
| Gargoyle | shadow_talon | B,DB,D,DF,F,P | none (only `P63214` as CAT_SCRAP) | removed |

Everything else in the configs matched an AF move string.

### Discrepancies that remain in the data (documented, not "fixed")

- **Electra "Electric Shards"**: the HAR wiki says qcf+K, but `FIGHTR4.AF` has
  no `K63` (qcf+K) move. The config maps it to qcf+P (`P63`), which does exist.
  This is a wiki-vs-AF divergence; the config was left data-driven against the AF.

- **Projectile spawns**: the actual flying projectile is a separate
  `CAT_PROJECTILE` move with `!` (no input command); it is spawned by the
  attack move's animation. Do not expect a projectile's input string to appear
  as a qcb/qcf command.

## 7. Config move-count expectations

The unit tests in `testing/ai/ai_har_skills_test.c` assert per-file move counts
via `DEFINE_HAR_CONFIG_TESTS(...)` and the `"sequence"` occurrence count table.
When adding or removing a move from a HAR JSON file, update both.
