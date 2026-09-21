# OpenOMF Copilot instructions

## Project overview
- This repository is OpenOMF, a C/C++ game project with a CMake/Ninja build and a dev-container workflow.
- The main build entrypoint is `.devcontainer/build.sh`.
- Default validation paths are the project build, focused game run, and the deterministic AI test binary.

## Build and run commands
Use the repo wrapper instead of ad hoc commands when possible:
- `.devcontainer/build.sh build`
- `.devcontainer/build.sh gui`
- `.devcontainer/build.sh run`
- `.devcontainer/build.sh shell`
- `.devcontainer/build.sh run --version`

For AI-focused runtime debugging, prefer log filtering:
- `.devcontainer/build.sh run --log-level=DEBUG --log-modules=ai-tactics`
- `.devcontainer/build.sh gui --log-level=DEBUG --log-modules=ai-tactics`

Important runtime notes:
- `--forward` is not a valid OpenOMF CLI option; do not use it.
- The Linux/container environment usually needs the NULL audio backend, which is already handled by the project scripts.
- The project uses the build tree at `build/` for resources and runtime state.

## Validation workflow
For deterministic AI or tactic work, validate with the real test binary:
- `cmake --build build --target openomf_test_main`
- `OPENOMF_RUN_DETERMINISTIC_TESTS=1 ./build/openomf_test_main`

Prefer deterministic tests for AI regressions and tuning changes. Add or update a regression test when behavior changes.

## AI logging conventions
- Keep the AI tick and AI tactic logs separate when possible.
- Use the module tag `ai` for general AI controller/logging.
- Use `ai-tactics` for tactic-selection decisions and strategy logging.
- If a fix touches tactical behavior, add logs only to the relevant channel and avoid polluting other AI channels.

## Editing conventions
- Prefer config/data changes in `resources/ai_config/` before hard-coding logic in C.
- For AI tuning, look first at:
  - `resources/ai_config/tactics.json`
  - `resources/ai_config/pilots.json`
  - `resources/ai_config/hars/*.json`
  - `src/game/ai/`
- For gameplay balance issues, separate between:
  - general difficulty scaling
  - HAR-specific move profiles
  - pilot-specific behavior biases
  - anti-spam / cooldown logic in the AI learning layer

## Documentation and issue-tracking conventions
- Keep project docs under `docs/` and `docs/ai-pilot/`.
- If a change affects AI config, balancing, or deterministic validation, update the relevant documentation and AI notes.
- When the issue is behavior tuning, document what was changed, what was verified, and which config knob controls it.

## Practical rules
- Do not add test-only hooks to production code for convenience.
- Prefer minimal, root-cause fixes over broad refactors.
- Reproduce the bug or behavior before fixing it.
- Verify with the smallest relevant command that checks the actual behavior.
