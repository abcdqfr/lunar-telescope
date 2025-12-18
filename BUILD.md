# Lunar Telescope Build Instructions

## Prerequisites

- C compiler (GCC or Clang) + `make`
- Build-time system deps (required for default `make`):
  - `pkg-config`
  - `json-c` development headers
- Runtime system deps (required to run Lunar Telescope as intended):
  - `waypipe`
  - `sunshine`
  - `moonlight`

See `docs/DEPENDENCIES.md` for the tiered dependency policy and the explicit “escape hatch” flags (`WITH_JSONC=0`, `ALLOW_INCOMPLETE_RUNTIME=1`).

## Building

### Build everything (default)

```bash
make
```

### Build without json-c (disables JSON config parsing)

```bash
make WITH_JSONC=0
```

**Note:** When built with `WITH_JSONC=0`, `telescope_config_load()` will return `-ENOTSUP`.

### Running Tests

```bash
make test
```

## Integration Notes

- All upstream components (waypipe, sunshine, moonlight) are **system-installed** (no vendoring / no auto-fetch)
- No vendored dependencies are used (see `docs/design-constraints-policy.md` and `docs/UPSTREAM_DEPENDENCIES.md`)
- Use `make check-runtime` to verify runtime dependencies on a machine

