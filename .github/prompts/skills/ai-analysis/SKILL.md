---
name: ai-analysis
description: Analyse AI decision logs, tactic traces, and scenario metrics for OpenOMF tuning.
---

# AI analysis workflow

Use this skill when you need to inspect the AI decision trace from a headless run, summarise move/tactic usage, or compare scenario-level behaviour across runs.

## Typical commands

1. Build the project and run a short headless scenario with diagnostics enabled:
   `.devcontainer/build.sh build`
   `.devcontainer/build.sh run --log-level=DEBUG --log-modules=ai,ai-tactics --speed=10 -P rectests/6P.REC`

2. Save the output to a log file and run the parser:
   `./build/openomf --log-level=DEBUG --log-modules=ai,ai-tactics --speed=10 -P rectests/6P.REC 2>&1 | tee /tmp/openomf-ai.log`
   `python3 tools/ai_log_analyse.py /tmp/openomf-ai.log`

3. Review the summary for tactic and move distribution, range, and anomalies.

## What the parser reports

- `rows`: total AI decision traces seen
- `tactic_counts`: per tactic usage frequency
- `move_counts`: per move usage frequency
- `attack_counts`: per attack-type usage frequency
- `state_counts`: state changes observed
- `last_move_counts`: last move recorded per tick
- `range`: start/end tick coverage for the run

## Success criteria

- The tool finds real `tick=... tactic=... move=...` traces from the AI log.
- The JSON output is usable as a machine-readable summary for later tuning comparisons.
- Output can be used in a follow-up M7 tuning loop or manual review without code archaeology.
