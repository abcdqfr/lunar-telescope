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

## PR expectations

- CI must be green (required checks).
- Keep changes small and explain the constraint/determinism impact.


