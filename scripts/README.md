# Lunar Telescope Scripts

## waypipe-connect-smart.sh

Smart connection orchestration script for Waypipe-based remote applications.

### Usage

```bash
./waypipe-connect-smart.sh <config.json> [--profile <name>] [--dry-run] [--verbose]
```

### Options

- `--profile <name>`: Override performance profile (low-latency, balanced, high-quality, bandwidth-constrained)
- `--dry-run`: Validate configuration and show commands without executing
- `--verbose`: Enable verbose logging

### Example

```bash
./waypipe-connect-smart.sh config.json --profile low-latency
```

