# Lunar Telescope SSOT (Single Source of Truth)

This document exists to keep the **portability mindset** and the current **engineering punchlist** explicit and current.

## Architecture (mental model)

- **Tunnel / body**: `core/` orchestration + config + metrics
- **Lenses / backends**: `lenses/` transport adapters that launch/manage the actual client/server process
  - Waypipe lens: protocol-correct, “works everywhere” baseline
  - Sunshine/Moonlight lenses: gaming/video optimized alternatives

The tunnel selects a lens and delegates start/stop to it.

## Determinism (constraints are the product)

- We intentionally **constrain** ourselves (no vendored upstream code, no submodules, no “fetch latest in Makefile”) to keep the system **predictable**.
- This project depends on upstream transports as **external runtime components**.
- The authoritative upstream policy is `docs/UPSTREAM_DEPENDENCIES.md` (and the enforcement policy is `docs/design-constraints-policy.md`).
- For acquisition vs licensing rationale, see `docs/DEPENDENCY_ACQUISITION.md`.

## Hybrid mainline contract (single-branch strategy)

We converged back to **one mainline** with an explicit hybrid contract:

- **C-only baseline (must always work)**:
  - The core build and runtime experience must work without Rust or Python.
  - This is the “widest reach” and “determinism first” contract.

- **Optional accelerators (must never block baseline)**:
  - **Rust**: performance island for input prediction (enable with `WITH_RUST=1`).
  - **Python**: tooling and UX (profiles/scripts/tests); never required to build the C core.

This is how we avoid long-lived divergent branches while still serving constrained environments.

## Portability policy (Linux-first, widest reach)

- **Feature-test macros are global** (never per-file):
  - We standardize on POSIX.1-2008 + X/Open 7:
    - `_POSIX_C_SOURCE=200809L`
    - `_XOPEN_SOURCE=700`
  - These are defined in `Makefile` and `tests/Makefile` via `FEATURE_MACROS`.
  - Rationale: consistent header visibility across translation units; fewer “works on glibc but not musl” surprises.

- **Avoid `_GNU_SOURCE` by default**
  - Only enable GNU extensions if a specific GNU-only API is needed and documented.

## Runtime dependency policy (degrade gracefully)

- **Exec handshake for all lenses**
  - Lens `start()` only reports success if `execvp()` succeeds.
  - Missing binaries (`ENOENT`), permission issues (`EACCES`), etc. fail fast and surface as negative errno.

- **Fallback selection**
  - `telescope_session_start()` tries:
    1. Primary lens (selected by config/heuristics)
    2. `config.lens.fallback[]` in order
    3. `waypipe` last (widest availability)

- **Supported installation expectation**
  - The intended, supported setup includes **all three** upstream binaries: `waypipe`, `sunshine`, `moonlight`.
  - Fallback is for deterministic runtime behavior, not as a substitute for installing required upstreams.

## Build policy (degrade gracefully)

- `WITH_JSONC=0` builds without json-c (JSON config parsing returns `-ENOTSUP`).

## Current punchlist (keep this current)

- **Upstream dependency story**
  - Keep upstream usage system-installed and license-aware (see `docs/UPSTREAM_DEPENDENCIES.md`).
  - Maintain `make check-runtime` as the canonical “doctor” for missing runtime components.

- **Preflight before pushing**
  - Policy: run CI-equivalent checks locally before pushing to avoid public CI churn.
  - We enforce **strict local-first** checks (>= CI) via Nix.
  - The pre-push hook runs:
    - `make preflight-baseline` (always, gcc, -Werror)
    - `make preflight-strict` (via `nix develop -c …`):
      - CI-equivalent build+tests (gcc, -Werror)
      - C sanitizers (clang ASan+UBSan)
      - clang-tidy analyzer gate
      - C coverage gate
  - Hooks are auto-installed on the first `make` run (`make hooks-install`), unless `CI=true`.
  - **No-bypass policy**: bypassing verification is considered a violation of project policy. Enforce with GitHub branch protection (PRs + required checks).

- **Coverage ratchet (C is the meat)**
  - Current CI gate: **25% line coverage** (measured via `gcovr`).
  - Policy: ratchet upward by **+2% per week** (or per meaningful test PR) until we’re in a healthy range.
  - Priority order for new tests:
    1. `core/` + `input/` (high-signal logic, easiest to unit test)
    2. `compositor/` (needs harness/mocks)
    3. `lenses/` (needs fake lens exec/harness; real binaries are external)

- **PR-only trunk**
  - Protect `main` with GitHub branch protection:
    - require PRs (no direct pushes)
    - require status checks (Build and Test, Lint, Docs, Gitleaks)

- **Lens metrics**
  - Decide whether Sunshine/Moonlight metrics should be:
    - implemented (how/where to query), or
    - explicitly unsupported (and remove/adjust TODOs accordingly).


