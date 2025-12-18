# Git hooks (optional)

This repo includes hooks to reduce public CI failures.

## Enable hooks

```bash
make hooks-install
```

## Hooks included

- `pre-push`: runs `make preflight-ci` (CI-equivalent checks, deterministic, no network fetches)

## What preflight-ci requires

To mirror the server-side CI job locally, you need:

- `pkg-config`
- `libjson-c-dev` (or equivalent json-c dev package)
- `python3`
- `cargo` (Rust toolchain)

## No-bypass policy

Git technically allows bypassing hooks. **This project does not.**

Enforce this where it matters:
- Protect `main` in GitHub settings:
  - require PRs (no direct pushes)
  - require status checks to pass
  - restrict who can push


