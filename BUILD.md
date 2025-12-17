# Lunar Telescope Build Instructions

## Prerequisites

- C compiler (GCC or Clang)
- Rust toolchain (for input_predictor)
- Python 3 (for configuration and profiles)
- System dependencies:
  - `waypipe` (system-installed)
  - `json-c` development headers
  - `libwayland` development headers (for compositor integration)

## Building

### Core C Modules

```bash
cd core
gcc -c -fPIC -o schema.o schema.c -I. $(pkg-config --cflags json-c)
gcc -c -fPIC -o profiles.o profiles.c -I.
gcc -c -fPIC -o telescope.o telescope.c -I.
gcc -c -fPIC -o metrics.o metrics.c -I.
```

### Input Modules

```bash
cd input
gcc -c -fPIC -o input_proxy.o input_proxy.c -I. -I../core
gcc -c -fPIC -o scroll_smoother.o scroll_smoother.c -I. -I../core -lm
gcc -c -fPIC -o reconciliation.o reconciliation.c -I. -I../core
```

### Rust Input Predictor

```bash
cd rust/input_predictor
cargo build --release
```

### Compositor Stubs

```bash
cd compositor
gcc -c -fPIC -o wl_input.o wl_input.c -I. -I../core -I../input
gcc -c -fPIC -o wl_surface.o wl_surface.c -I. -I../core
```

### Running Tests

```bash
cd tests
make test
```

## Integration Notes

- All upstream components (waypipe, sunshine, moonlight) must be system-installed
- No vendored dependencies are used
- Rust module is compiled as a C-compatible shared library

