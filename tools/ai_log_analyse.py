#!/usr/bin/env python3
"""Summarize AI decision traces from OpenOMF log output.

This tool parses compact AI tick logs such as:
    DEBUG [ai] tick=42 state=3 tactic=8 move=14 attack=2 last_move=7

and produces a compact JSON summary for M6 analysis work.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable

AI_TRACE_RE = re.compile(
    r"tick=(?P<tick>\d+)\s+state=(?P<state>\d+)\s+tactic=(?P<tactic>\d+)\s+"
    r"move=(?P<move>\d+)\s+attack=(?P<attack>\d+)\s+last_move=(?P<last_move>\d+)"
)


def parse_ai_log(source: str | Path) -> list[dict[str, int]]:
    """Return per-tick AI decisions from a log file or log text."""
    text = _read_source(source)
    rows: list[dict[str, int]] = []
    for line in text.splitlines():
        match = AI_TRACE_RE.search(line)
        if not match:
            continue
        row = {key: int(value) for key, value in match.groupdict().items()}
        rows.append(row)
    return rows


def _read_source(source: str | Path) -> str:
    if isinstance(source, (str, Path)):
        path = Path(source)
        if path.exists() and path.is_file():
            return path.read_text(encoding="utf-8", errors="replace")
    return str(source)


def summarize(source: str | Path) -> dict[str, Any]:
    """Return aggregated counts and range metadata for an AI log."""
    rows = parse_ai_log(source)
    if not rows:
        return {
            "rows": 0,
            "tactic_counts": {},
            "move_counts": {},
            "attack_counts": {},
            "state_counts": {},
            "last_move_counts": {},
            "range": {"start_tick": None, "end_tick": None, "ticks": 0},
        }

    tactic_counts = Counter(str(row["tactic"]) for row in rows)
    move_counts = Counter(str(row["move"]) for row in rows)
    attack_counts = Counter(str(row["attack"]) for row in rows)
    state_counts = Counter(str(row["state"]) for row in rows)
    last_move_counts = Counter(str(row["last_move"]) for row in rows)
    ticks = [row["tick"] for row in rows]

    summary: dict[str, Any] = {
        "rows": len(rows),
        "tactic_counts": dict(sorted(tactic_counts.items(), key=lambda item: (int(item[0]), item[0]))),
        "move_counts": dict(sorted(move_counts.items(), key=lambda item: (int(item[0]), item[0]))),
        "attack_counts": dict(sorted(attack_counts.items(), key=lambda item: (int(item[0]), item[0]))),
        "state_counts": dict(sorted(state_counts.items(), key=lambda item: (int(item[0]), item[0]))),
        "last_move_counts": dict(sorted(last_move_counts.items(), key=lambda item: (int(item[0]), item[0]))),
        "range": {
            "start_tick": min(ticks),
            "end_tick": max(ticks),
            "ticks": max(ticks) - min(ticks) + 1,
        },
    }
    return summary


def compare_summaries(paths: Iterable[str | Path]) -> dict[str, Any]:
    """Aggregate multiple log summaries into a single comparison view."""
    path_list = list(paths)
    trial_summaries = [summarize(path) for path in path_list]

    aggregate = {
        "tactic_counts": Counter(),
        "move_counts": Counter(),
        "attack_counts": Counter(),
        "state_counts": Counter(),
        "last_move_counts": Counter(),
    }
    for summary in trial_summaries:
        for key, counter in (
            ("tactic_counts", aggregate["tactic_counts"]),
            ("move_counts", aggregate["move_counts"]),
            ("attack_counts", aggregate["attack_counts"]),
            ("state_counts", aggregate["state_counts"]),
            ("last_move_counts", aggregate["last_move_counts"]),
        ):
            for k, v in summary.get(key, {}).items():
                counter[k] += v

    return {
        "runs": len(path_list),
        "trial_summaries": trial_summaries,
        "aggregate": {
            "rows": sum(summary["rows"] for summary in trial_summaries),
            "tactic_counts": dict(sorted(aggregate["tactic_counts"].items(), key=lambda item: (int(item[0]), item[0]))),
            "move_counts": dict(sorted(aggregate["move_counts"].items(), key=lambda item: (int(item[0]), item[0]))),
            "attack_counts": dict(sorted(aggregate["attack_counts"].items(), key=lambda item: (int(item[0]), item[0]))),
            "state_counts": dict(sorted(aggregate["state_counts"].items(), key=lambda item: (int(item[0]), item[0]))),
            "last_move_counts": dict(sorted(aggregate["last_move_counts"].items(), key=lambda item: (int(item[0]), item[0]))),
        },
    }


def diff_against_baseline(summary: dict[str, Any], baseline: dict[str, Any], tolerances: dict[str, float] | None = None) -> dict[str, Any]:
    """Compare a summary against a baseline and flag bucket-level drift beyond the allowed tolerance."""
    if tolerances is None:
        tolerances = {}

    counts_keys = ["tactic_counts", "move_counts", "attack_counts", "state_counts", "last_move_counts"]
    diff: dict[str, Any] = {"status": "pass", "failed": [], "differences": {}}

    for key in counts_keys:
        actual = summary.get(key, {})
        expected = baseline.get(key, {})
        all_keys = sorted(set(actual) | set(expected), key=lambda v: (int(v), v))
        drift = {}
        for bucket in all_keys:
            actual_count = int(actual.get(bucket, 0))
            expected_count = int(expected.get(bucket, 0))
            delta = actual_count - expected_count
            tolerance = float(tolerances.get(key, tolerances.get("*", 0)))
            if abs(delta) > tolerance:
                diff["failed"].append({
                    "key": key,
                    "bucket": bucket,
                    "baseline": expected_count,
                    "actual": actual_count,
                    "delta": delta,
                    "tolerance": tolerance,
                })
                diff["status"] = "fail"
            drift[bucket] = {
                "baseline": expected_count,
                "actual": actual_count,
                "delta": delta,
                "tolerance": tolerance,
            }
        if drift:
            diff["differences"][key] = drift

    return diff


def _as_json(obj: Any) -> str:
    return json.dumps(obj, indent=2, sort_keys=True)


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Parse OpenOMF AI tick logs into summary metrics.")
    parser.add_argument("logfiles", nargs="*", help="Paths to one or more log files or '-' for stdin")
    parser.add_argument("--stdin", action="store_true", help="Read log input from stdin")
    parser.add_argument("--baseline", help="JSON file with a baseline AI summary to compare against")
    parser.add_argument("--tolerance", type=float, default=0.0, help="Maximum absolute bucket drift allowed")
    args = parser.parse_args(list(argv) if argv is not None else None)

    if args.stdin:
        payload = sys.stdin.read()
        dump = summarize(payload)
    elif len(args.logfiles) == 1:
        dump = summarize(args.logfiles[0])
    elif len(args.logfiles) > 1:
        dump = compare_summaries(args.logfiles)
    else:
        parser.error("supply a log file or use --stdin")

    if args.baseline:
        with open(args.baseline, "r", encoding="utf-8") as fh:
            baseline = json.load(fh)
        comparison = diff_against_baseline(dump, baseline, {"*": args.tolerance})
        print(_as_json(comparison))
        return 0 if comparison["status"] == "pass" else 1

    print(_as_json(dump))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
