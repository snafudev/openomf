import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.ai_log_analyse import compare_summaries, diff_against_baseline, parse_ai_log, summarize


def test_parse_ai_log_extracts_tick_rows():
    log = """
2026-09-21 10:00:00 DEBUG [ai] tick=42 state=3 tactic=8 move=14 attack=2 last_move=7
2026-09-21 10:00:01 DEBUG [ai] tick=43 state=3 tactic=8 move=14 attack=2 last_move=7
2026-09-21 10:00:02 DEBUG [ai] tick=44 state=5 tactic=1 move=9 attack=0 last_move=9
"""

    rows = parse_ai_log(log)

    assert len(rows) == 3
    assert rows[0]["tick"] == 42
    assert rows[0]["tactic"] == 8
    assert rows[0]["move"] == 14
    assert rows[2]["state"] == 5


def test_summarize_counts_tactics_and_moves():
    log = """
2026-09-21 10:00:00 DEBUG [ai] tick=10 state=3 tactic=8 move=14 attack=2 last_move=7
2026-09-21 10:00:01 DEBUG [ai] tick=11 state=3 tactic=8 move=14 attack=2 last_move=7
2026-09-21 10:00:02 DEBUG [ai] tick=12 state=5 tactic=1 move=9 attack=0 last_move=9
2026-09-21 10:00:03 DEBUG [ai] tick=13 state=5 tactic=1 move=9 attack=0 last_move=9
"""

    summary = summarize(log)

    assert summary["rows"] == 4
    assert summary["tactic_counts"]["8"] == 2
    assert summary["tactic_counts"]["1"] == 2
    assert summary["move_counts"]["14"] == 2
    assert summary["attack_counts"]["0"] == 2
    assert summary["state_counts"]["5"] == 2
    assert summary["range"]["start_tick"] == 10
    assert summary["range"]["end_tick"] == 13


def test_summarize_json_is_valid_dict():
    log = """
2026-09-21 10:00:00 DEBUG [ai] tick=7 state=3 tactic=2 move=5 attack=1 last_move=5
"""

    summary = summarize(log)
    json.dumps(summary)
    assert summary["rows"] == 1


def test_compare_summaries_across_runs(tmp_path):
    log_a = tmp_path / "run_a.log"
    log_b = tmp_path / "run_b.log"
    log_a.write_text(
        "2026-09-21 10:00:00 DEBUG [ai] tick=10 state=3 tactic=8 move=14 attack=2 last_move=7\n"
        "2026-09-21 10:00:01 DEBUG [ai] tick=11 state=3 tactic=8 move=14 attack=2 last_move=7\n",
        encoding="utf-8",
    )
    log_b.write_text(
        "2026-09-21 10:00:00 DEBUG [ai] tick=10 state=3 tactic=8 move=14 attack=2 last_move=7\n"
        "2026-09-21 10:00:01 DEBUG [ai] tick=11 state=5 tactic=1 move=9 attack=0 last_move=9\n",
        encoding="utf-8",
    )

    comparison = compare_summaries([log_a, log_b])

    assert comparison["runs"] == 2
    assert comparison["trial_summaries"][0]["rows"] == 2
    assert comparison["trial_summaries"][1]["rows"] == 2
    assert comparison["aggregate"]["tactic_counts"]["8"] == 3
    assert comparison["aggregate"]["state_counts"]["5"] == 1


def test_diff_against_baseline_flags_tactic_drift():
    baseline = {
        "rows": 2,
        "tactic_counts": {"8": 2, "1": 0},
        "move_counts": {"14": 2},
        "attack_counts": {"2": 2},
        "state_counts": {"3": 2},
        "last_move_counts": {"7": 2},
    }
    actual = {
        "rows": 2,
        "tactic_counts": {"8": 1, "1": 1},
        "move_counts": {"14": 1, "9": 1},
        "attack_counts": {"2": 1, "0": 1},
        "state_counts": {"3": 1, "5": 1},
        "last_move_counts": {"7": 1, "9": 1},
    }

    check = diff_against_baseline(actual, baseline, {"*": 0})

    assert check["status"] == "fail"
    assert any(item["key"] == "tactic_counts" and item["bucket"] == "1" for item in check["failed"])
    assert check["differences"]["tactic_counts"]["1"]["delta"] == 1
