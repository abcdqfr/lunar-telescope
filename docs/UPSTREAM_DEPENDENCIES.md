# Upstream Dependencies (SSOT)

Lunar Telescope is a **glue-layer** project. It does not reimplement transports; it **leans on upstream** and uses **constraints to breed determinism**.

## Policy (non-negotiable)

- **No vendoring upstream source** (Waypipe/Sunshine/Moonlight).
- **No git submodules** for upstream.
- **No Makefile auto-fetch of upstream code** (no “download latest” behavior).
- Upstream components are consumed as **system-installed runtime dependencies** (including via Nix/nixpkgs).
- **Licensing**
  - Each upstream’s **own LICENSE file** governs how it may be redistributed/packaged.
  - This repo **does not redistribute upstream source by default**, so we keep licensing boundaries explicit.
  - For transparency, we keep verbatim copies of upstream license texts under `docs/upstream-licenses/` (with `*.SOURCE.txt` pointing at the upstream URL used).

## Upstreams we rely on

- **Waypipe**
  - **Role**: baseline “tunnel” transport (protocol correctness, general app remoting)
  - **How we use it**: execute `waypipe` via the Waypipe lens (`lenses/lens_waypipe.c`)
  - **License**: see Waypipe upstream LICENSE

- **Sunshine**
  - **Role**: high-motion streaming backend (gaming/video oriented)
  - **How we use it**: execute `sunshine` via the Sunshine lens (`lenses/lens_sunshine.c`)
  - **License**: see Sunshine upstream LICENSE

- **Moonlight**
  - **Role**: low-latency decode/client backend (gaming/video oriented)
  - **How we use it**: execute `moonlight` via the Moonlight lens (`lenses/lens_moonlight.c`)
  - **License**: see Moonlight upstream LICENSE

## Deterministic behavior expectations

- **Build determinism**: building Lunar Telescope should never depend on “whatever the network returns today.”
- **Runtime determinism**:
  - Lenses must fail fast if a binary is missing (exec handshake).
  - Supported installations provide **all three upstream binaries** (waypipe/sunshine/moonlight). Missing binaries are considered a misconfigured environment.
  - Session start may still fall back on runtime errors (e.g., exec failures, remote connectivity), but this is **not** a substitute for installing the required upstreams.

## Developer ergonomics

- Use `make check-runtime` to see what’s missing on a machine (strict by default).
- Prefer packaging via a deterministic system (Nix, distro packages) over ad-hoc installs.
- See `docs/DEPENDENCY_ACQUISITION.md` for the explicit “ease vs determinism vs license boundary” rationale.


