# Dependencies (deterministic, tiered)

This repo treats **constraints as product**: dependencies are explicit, tiered, and checked with **fail-fast** “doctor” targets. We do **not** vendor or auto-fetch upstream projects.

## Runtime (system/userland) — REQUIRED

These are the upstream transports (“lenses”) Lunar Telescope orchestrates. If you only install `waypipe`, you’re not exercising the core promise of this repo (lens arbitration across transports).

- **`waypipe`**: protocol-correct baseline
- **`sunshine`**: high-motion streaming backend
- **`moonlight`**: low-latency client/decode stack

Check:

- `make check-runtime` (strict; exits non-zero if any are missing)
- Escape hatch (explicit degraded mode): `make ALLOW_INCOMPLETE_RUNTIME=1 check-runtime`

## Build (system) — REQUIRED for default `make`

- **C toolchain**: `gcc` or `clang`, plus `make`
- **`pkg-config`** (used for deterministic detection of system libraries)
- **`json-c` development headers** (default build enables JSON config parsing)

Escape hatches (explicit degraded builds):

- `make WITH_JSONC=0` (JSON config parsing becomes unsupported; loader returns `-ENOTSUP`)


