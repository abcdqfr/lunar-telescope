# Git hooks (optional)

This repo includes hooks to reduce public CI failures.

## Enable hooks

```bash
make hooks-install
```

## Hooks included

- `pre-push`: runs `make preflight-ci` (CI-equivalent checks, deterministic, no network fetches)

## Bypass

```bash
git push --no-verify
```


