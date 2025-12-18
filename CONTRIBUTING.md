# Contributing (PR-first, deterministic)

This repo runs a **PR-only** workflow on `main`.

## Rules

- **No direct pushes to `main`** (use PRs).
- **No vendoring upstream code** (Waypipe/Sunshine/Moonlight).
- **Determinism first**: no “download latest in Makefile”.

## Local checks (do these before pushing)

- **Baseline contract** (must always pass):
  - `make preflight-baseline`
- **CI-equivalent preflight** (recommended; mirrors GitHub Actions):
  - `make preflight-ci`

The pre-push hook runs baseline always and runs CI-preflight when the toolchain is present.

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


