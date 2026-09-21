"""M1: on-the-fly REC generation with embedded assertions (pass/fail)."""

import os
import sys
from pathlib import Path

import pytest

sys.path.insert(0, os.path.dirname(__file__))

from ai_rec_harness import generate_rec, get_binaries, run_rec

pytestmark = pytest.mark.deterministic_ai

build_dir, openomf_bin, gen_move_test = get_binaries()

requires_tools = pytest.mark.skipif(
    not os.path.exists(str(gen_move_test)),
    reason="gen_move_test not built (configure with -DUSE_TOOLS=On)",
)


def _gen(tmp_path, name, **kwargs):
    rec = tmp_path / name
    res = generate_rec(gen_move_test, 0, 1, str(rec), **kwargs)
    assert res.returncode == 0, res.stderr
    return rec


@requires_tools
def test_true_assertion_passes(tmp_path):
    rec = _gen(tmp_path, "true.rec", assertions=["har1.health>0@200"])
    run = run_rec(openomf_bin, build_dir, rec)
    assert run.returncode == 0, run.stdout + run.stderr


@requires_tools
def test_false_assertion_fails(tmp_path):
    rec = _gen(tmp_path, "false.rec", assertions=["har1.health==99999@200"])
    run = run_rec(openomf_bin, build_dir, rec)
    assert run.returncode != 0


@requires_tools
def test_set_then_assert_position(tmp_path):
    rec = _gen(tmp_path, "set.rec", assertions=["har1.xpos:=300@150", "har1.xpos==300@150"])
    run = run_rec(openomf_bin, build_dir, rec)
    assert run.returncode == 0, run.stdout + run.stderr


@requires_tools
def test_seed_record_is_accepted(tmp_path):
    rec = _gen(tmp_path, "seeded.rec", seed=12345, assertions=["har1.health>0@200"])
    run = run_rec(openomf_bin, build_dir, rec)
    assert run.returncode == 0, run.stdout + run.stderr
