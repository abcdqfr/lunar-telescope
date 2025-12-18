# Dependencies (deterministic, tiered)

This repo treats **constraints as product**: dependencies are explicit, tiered, and checked with **fail-fast** “doctor” targets. We do **not** vendor or auto-fetch upstream projects.

## Runtime (system/userland) — REQUIRED

These are the upstream transports (“lenses”) Lunar Telescope orchestrates.

- **`waypipe`**: protocol-correct baseline (**required**)
- **`sunshine`**: high-motion streaming backend (**optional accelerator**)
- **`moonlight`**: low-latency client/decode stack (**optional accelerator**)

Check:

- `make check-runtime` (fails if `waypipe` is missing; reports Sunshine/Moonlight status without failing)

## Build (system) — REQUIRED for default `make`

- **C toolchain**: `gcc` or `clang`, plus `make`
- **`pkg-config`** (used for deterministic detection of system libraries)
- **`json-c` development headers** (default build enables JSON config parsing)

Escape hatches (explicit degraded builds):

- `make WITH_JSONC=0` (JSON config parsing becomes unsupported; loader returns `-ENOTSUP`)

## Optional accelerators / tooling

These must never block the C-only baseline, but they improve performance/UX when present:

- **Rust toolchain** (`cargo`): enable with `make WITH_RUST=1`
- **Python 3** (`python3`): enables scripts and Python tests (tests auto-skip if unavailable)


