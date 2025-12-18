# Git hooks (optional)

This repo includes **opt-in** hooks to reduce public CI failures.

## Enable hooks

```bash
git config core.hooksPath .githooks
```

## Hooks included

- `pre-push`: runs `make preflight` (deterministic, no network fetches)

## Bypass

```bash
git push --no-verify
```


