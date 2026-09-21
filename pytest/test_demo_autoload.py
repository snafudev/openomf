import os
import subprocess


def test_demo_autoload_flags_in_help():
    openomf_bin = os.environ["OPENOMF_BIN"]
    result = subprocess.run(
        [openomf_bin, "--help"],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    help_text = (result.stdout or "") + (result.stderr or "")
    assert "--demo-pilot" in help_text
    assert "--demo-har" in help_text
    assert "--demo-difficulty" in help_text
