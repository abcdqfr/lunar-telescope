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

## Bypass

```bash
git push --no-verify
```


