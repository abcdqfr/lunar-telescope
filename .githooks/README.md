# Git hooks (optional)

This repo includes hooks to reduce public CI failures.

## Enable hooks

```bash
make hooks-install
```

## Hooks included

- `pre-push`: runs `make preflight-baseline` and then enforces `make preflight-strict`
  (CI-equivalent checks + C coverage gate)

## What preflight-strict requires

Recommended (deterministic): use Nix

```bash
nix develop -c make preflight-strict
```

Or install the toolchain equivalents:

- `pkg-config`
- `libjson-c-dev` (or equivalent json-c dev package)
- `python3`
- `cargo` (Rust toolchain)
- `gcovr` (C coverage reporting)

## No-bypass policy

Git technically allows bypassing hooks. **This project does not.**

Enforce this where it matters:
- Protect `main` in GitHub settings:
  - require PRs (no direct pushes)
  - require status checks to pass
  - restrict who can push


