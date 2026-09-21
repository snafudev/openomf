import os

import pytest


AI_DETERMINISTIC_ENV = "OPENOMF_RUN_DETERMINISTIC_TESTS"


def pytest_collection_modifyitems(config, items):
    if os.environ.get(AI_DETERMINISTIC_ENV, "0") == "1":
        return

    skip = pytest.mark.skip(
        reason=(
            "AI pilot deterministic tests are opt-in and should only run while actively tuning AI pilot behavior. "
            f"Set {AI_DETERMINISTIC_ENV}=1 to enable them."
        )
    )
    for item in items:
        if "deterministic_ai" in item.keywords:
            item.add_marker(skip)
