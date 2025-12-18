# Contributing (PR-first, deterministic)

This repo runs a **PR-only** workflow on `main`.

## Rules

- **No direct pushes to `main`** (use PRs).
- **No vendoring upstream code** (Waypipe/Sunshine/Moonlight).
- **Determinism first**: no “download latest in Makefile”.

## Local checks (do these before pushing)

- **Baseline contract** (must always pass):
  - `make preflight-baseline`
- **CI-equivalent preflight** (mirrors GitHub Actions build+tests):
  - `make preflight-ci`
- **Stricter-than-CI preflight** (required for pushes; includes C coverage gate):
  - `make preflight-strict`
- **Coverage only** (C-heavy signal; generates HTML + Cobertura XML):
  - `make coverage`

The pre-push hook runs baseline always and enforces `preflight-strict` (recommended via Nix).

## Nix devshell (recommended)

To get a deterministic local toolchain matching CI, use:

```bash
nix develop
make preflight-strict
```

## Self-PR flow (GitHub CLI)

This is the default workflow for maintainers too:

```bash
make pr
```

It will:
- run `make preflight-baseline`
- push your current branch
- open a PR into `main` using `gh pr create --fill --base main`

## PR expectations

- CI must be green (required checks).
- Keep changes small and explain the constraint/determinism impact.


