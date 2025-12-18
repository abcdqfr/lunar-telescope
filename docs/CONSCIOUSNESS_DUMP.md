# Lunar Telescope — Consciousness Dump / Handoff (for next convo)

This document is a **state dump** of the repo and design intent as of **2025-12-18**.
Open `lunar-telescope.code-workspace` and use it as the primary dev entrypoint.

## What this project is (in one paragraph)

Lunar Telescope is a **deterministic glue-layer orchestration framework** for first-class Wayland remoting, designed to “rival VMware” in the sense of being a *serious* alternative workflow for remote desktop/app publishing, without changing what Wayland is. It relies on upstream transports/backends (Waypipe, Sunshine, Moonlight) rather than reimplementing them, and leans hard on constraints to keep behavior predictable.

## Mental model (the naming story)

- **Tunnel / body (telescope)**: orchestration + config + metrics + policy (`core/`)
- **Lenses / backends**: transport adapters that start/stop external programs (`lenses/`)
  - Waypipe lens = baseline “works everywhere” fallback
  - Sunshine/Moonlight lenses = gaming/video-oriented alternatives

Names:
- `Lunar Telescope` is the better platform name; “MoonPipe” is better as a component name (e.g., Moonlight lens), not the whole system.

## Constraints → determinism (this is the product)

This repo intentionally constrains itself to preserve determinism:

- **Glue layer only**: we orchestrate; we do not implement transports/codecs/protocols.
- **No vendoring upstream code**
- **No submodules**
- **No Makefile “download latest” behavior**
- **System packaging is the distribution boundary**

Authoritative policies:
- `docs/design-constraints-policy.md` (enforcement rules)
- `docs/SSOT.md` (single source of truth for mindset + punchlist)
- `docs/UPSTREAM_DEPENDENCIES.md` (upstream relationship)
- `docs/DEPENDENCY_ACQUISITION.md` (ease vs determinism vs licensing logic)

## Upstream deps + licenses (transparent)

We store verbatim license texts for transparency (not upstream code):

- `docs/upstream-licenses/sunshine.LICENSE.txt` (GPLv3 text)
- `docs/upstream-licenses/moonlight-embedded.LICENSE.txt` (GPLv3 text)
- `docs/upstream-licenses/waypipe.LICENSE.GPLv3.txt` (GPLv3 text)
- `docs/upstream-licenses/waypipe.LICENSE.MIT.txt` (MIT text)

Each has a corresponding `*.SOURCE.txt` with the URL used to fetch.

Why no auto-acquisition by default:
- **Determinism first**: build-time network fetch is non-deterministic and a supply-chain footgun unless fully pinned+hashed+toolchain-locked.
- **License boundary clarity**: auto-downloading/installing third-party software (especially GPL programs) risks making us a redistributor/installer with compliance pitfalls.
- Best path: **distro packages / nixpkgs**.

## Developer UX (workspace is the entrypoint)

Open: `lunar-telescope.code-workspace`

It includes:
- clangd-first C tooling (cpptools disabled)
- shell settings
- tasks for build/test/check-runtime
- gdb launch configs for tests

## Build & runtime behavior (important)

### Feature-test macros (portability policy)

To maximize Linux reach and avoid per-file inconsistency:
- `_POSIX_C_SOURCE=200809L` and `_XOPEN_SOURCE=700` are defined **globally**
  - Root `Makefile`: `FEATURE_MACROS`
  - `tests/Makefile`: `FEATURE_MACROS`
- No per-file `#define _POSIX_C_SOURCE` / `_XOPEN_SOURCE` remain.

### Optional build toggles (degrade gracefully)

- `WITH_JSONC=0`: build without json-c; `telescope_config_load()` returns `-ENOTSUP`

### Runtime checks (deterministic; no fetching)

- `make check-runtime` checks for system-installed `waypipe`, `sunshine`, `moonlight`.

### Lens start correctness (exec handshake)

All lenses now use an **exec handshake**:
- lens `start()` only returns success if `execvp()` succeeds.
- missing binaries return negative errno (e.g., `-ENOENT`), not a “running” lie.

### Fallback behavior (works everywhere bias)

`telescope_session_start()` now tries:
1) primary lens (config/heuristics)
2) `config.lens.fallback[]` (in order)
3) `waypipe` last as the baseline

This encodes the “widest reach” bias while allowing preference for Sunshine/Moonlight.

## Repo state changes made in this session (high-signal)

### Build determinism + clarity
- added explicit dependency checks (json-c) and clear failure messages
- added `make check-runtime`
- updated docs to emphasize no auto-fetch, system packages, and licensing boundaries

### Portability consistency
- standardized `_POSIX_C_SOURCE` / `_XOPEN_SOURCE` via build flags only

### Core ↔ lens architecture alignment
- session start/stop delegates to lens adapters (not hardcoded waypipe exec)
- added reliable fallback across lenses

### Workspace UX
- added `lunar-telescope.code-workspace` with tasks/launch/ext recommendations

### Stash cleanup
- the old stash (`posix macro removals + README whitespace`) was dropped.

## Commands (copy/paste)

- **Runtime doctor**
  - `make check-runtime`
- **Portable build (no json-c)**
  - `make WITH_JSONC=0 -j$(nproc)`
- **Portable tests**
  - `make WITH_JSONC=0 test`

## Current punchlist (what to do next)

1) **Lens metrics policy**
   - Decide if Sunshine/Moonlight metrics are supported or explicitly unsupported.
   - If unsupported, remove/adjust TODOs + document.
   - If supported, define the mechanism (CLI output parse? local API?).

2) **Full-deps verification on a real machine**
   - With `pkg-config`, `json-c` dev, run full build/tests and fix *real* failures.

3) **Packaging pathway**
   - The repo’s constraint-aligned “ease” path is packaging (Nix/nixpkgs + distro packages).
   - If an opt-in fetch mode is desired, it must be pinned+hashed and never default.

## Where to look first (map)

- `docs/SSOT.md`
- `docs/design-constraints-policy.md`
- `docs/UPSTREAM_DEPENDENCIES.md`
- `docs/DEPENDENCY_ACQUISITION.md`
- `Makefile` (build + checks + runtime doctor)
- `core/telescope.c` (session start/fallback)
- `lenses/*.c` (exec handshake + process lifecycle)
- `lunar-telescope.code-workspace` (tasks/launch)


