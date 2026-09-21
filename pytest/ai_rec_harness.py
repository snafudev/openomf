"""Helpers for generating REC files and running them headless (M1).

These wrap `tools/gen_move_test` (REC generation with optional seed/assertions)
and `openomf -P` (headless replay that evaluates embedded assertions).
"""

import os
import subprocess
from pathlib import Path


def get_binaries():
    """Resolve the build dir, openomf binary and gen_move_test tool.

    Prefers the environment variables exported by `run_pytest.sh`
    (BUILD_DIR, OPENOMF_BIN), falling back to a local `build/` tree.
    """
    build_dir = Path(os.environ.get("BUILD_DIR", "build"))
    openomf_bin = os.environ.get("OPENOMF_BIN", str(build_dir / "openomf"))
    gen_move_test = build_dir / "gen_move_test"
    return build_dir, openomf_bin, gen_move_test


def generate_rec(gen, har_id, opponent_id, output_file, actions=(), seed=None, assertions=(), ticks=None):
    """Generate a REC file via gen_move_test.

    actions:   sequence of hex ACT_* byte strings (e.g. "40", "44").
    seed:      optional integer seed inserted at tick 0.
    assertions: optional list of assertion specs (e.g. "har1.anim==12@64").
    ticks:     optional total recording length.
    """
    cmd = [str(gen), str(har_id), str(opponent_id), str(output_file)]
    if seed is not None:
        cmd += ["--seed", str(seed)]
    if ticks is not None:
        cmd += ["--ticks", str(ticks)]
    for assertion in assertions:
        cmd += ["--assert", assertion]
    cmd += [str(a) for a in actions]
    return subprocess.run(cmd, capture_output=True, text=True, timeout=20)


def run_rec(openomf_bin, build_dir, rec_file, timeout=90):
    """Replay a REC headless. A non-zero exit code means an assertion failed."""
    env = dict(os.environ)
    env["OPENOMF_RESOURCE_PATH"] = str(build_dir)
    return subprocess.run(
        [
            str(openomf_bin),
            "--force-audio-backend=NULL",
            "--force-renderer=NULL",
            "--speed=10",
            "-P",
            str(rec_file),
        ],
        capture_output=True,
        text=True,
        timeout=timeout,
        cwd=str(build_dir),
        env=env,
    )
