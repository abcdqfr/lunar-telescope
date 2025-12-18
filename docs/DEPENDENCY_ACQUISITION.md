# Dependency Acquisition Policy (license-aware, deterministic)

This project aims for **ease**, but never at the cost of **breaking constraints** that preserve **determinism**.

## What we mean by “automatic acquisition”

- **Bad (non-deterministic)**: `make` downloads “latest” upstream code/binaries at build time.
- **Potentially acceptable (deterministic, but still a policy decision)**: an explicit, opt-in target that fetches **pinned** revisions with checksums.
- **Best (deterministic + compliant)**: use **system packaging** (distro packages or Nix/nixpkgs) which already solves distribution + licensing workflows.

By default Lunar Telescope chooses **system packaging**.

## Upstream licenses (fetched)

We keep local copies of upstream license texts for transparency:

- `docs/upstream-licenses/waypipe.LICENSE.GPLv3.txt`
- `docs/upstream-licenses/waypipe.LICENSE.MIT.txt`
- `docs/upstream-licenses/sunshine.LICENSE.txt`
- `docs/upstream-licenses/moonlight-embedded.LICENSE.txt`

Source URLs are recorded alongside each license in `*.SOURCE.txt`.

## The logic: why we do NOT auto-acquire upstreams by default

### 1) Determinism constraint (primary)

- Network fetch during builds is inherently non-deterministic (availability, mirrors, mutable branches).
- Even “pinned” fetches introduce supply-chain risk unless we also pin checksums and toolchains.
- Distro/Nix packaging already provides pinning, caching, and verification paths.

### 2) License/compliance boundary (secondary, but real)

Lunar Telescope runs upstream programs as separate processes. It does not link against or embed them.

However, **auto-acquiring and installing** upstreams (especially GPL-licensed ones) increases the chance that we:

- become a *de facto redistributor/installer* of third-party software,
- accidentally omit required notices,
- blur the “glue layer only” boundary the project relies on.

Keeping upstreams **system-provided** makes the compliance story clear: the package manager/Nix expression is the distribution boundary.

## Practical outcome

- Default: users install `waypipe` / `sunshine` / `moonlight` through their system package manager (or nixpkgs).
- Lunar Telescope provides:
  - `make check-runtime` to diagnose missing runtime binaries (**strict by default**)
  - runtime fallbacks (waypipe last) for deterministic error handling (not as a “partial install” mode)

If we ever add an opt-in fetch mechanism, it must be:
- explicit (`make deps-fetch`), never implicit
- pinned (commit/release + checksum)
- non-vendoring (downloaded bits ignored by git)
- license-respecting (retain upstream LICENSE text next to downloaded artifacts)



