# Lunar Telescope Architecture

## Overview

Lunar Telescope is a **C-first orchestration layer** for remote Wayland application publishing. It composes upstream solutions (Waypipe, Sunshine, Moonlight) into a unified system with predictive input, performance optimization, and low-latency streaming.

## Core Principles

1. **Glue Layer Only**: No vendored upstream code, system-installed dependencies only
2. **C at the Boundary**: Hot-path code in C, Rust only as isolated performance island
3. **Deterministic**: No GC, no hidden schedulers, observable state
4. **Performance-Centric**: Hardware encoding, predictive input, thread optimization

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (User Configuration, Profiles, CLI Tools)                  │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                    Core Orchestration                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │  Telescope   │  │   Profiles   │  │   Schema     │   │
│  │  Session     │  │   Manager    │  │   Parser     │   │
│  └──────────────┘  └──────────────┘  └──────────────┘   │
└───────────────────────┬─────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   Input      │ │  Compositor  │ │    Lens      │
│   Proxy      │ │  Integration │ │   Adapters   │
│              │ │              │ │              │
│ ┌──────────┐ │ │ ┌──────────┐ │ │ ┌──────────┐ │
│ │Predictor │ │ │ │  Input   │ │ │ │ Waypipe  │ │
│ │(Rust)    │ │ │ │  Hooks   │ │ │ │ Sunshine │ │
│ └──────────┘ │ │ └──────────┘ │ │ │ Moonlight │ │
│ ┌──────────┐ │ │ ┌──────────┐ │ │ └──────────┘ │
│ │  Scroll  │ │ │ │ Surface  │ │ │              │
│ │ Smoother│ │ │ │ Tracking │ │ │              │
│ └──────────┘ │ │ └──────────┘ │ │              │
└──────────────┘ └──────────────┘ └──────────────┘
        │               │               │
        └───────────────┼───────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                    Metrics & Observability                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Latency    │  │   Bandwidth  │  │    Frame     │     │
│  │   Tracking   │  │   Averaging  │  │   Metrics    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

## Component Details

### Core Orchestration (`core/`)

**telescope.c / telescope.h**
- Session lifecycle management
- Waypipe process launching and monitoring
- Configuration application
- Metrics aggregation

**schema.c**
- JSON configuration parsing
- Schema validation
- Configuration structure management

**profiles.c**
- Performance profile application
- Lens selection logic
- Profile-based optimization

**metrics.c**
- Time-based bandwidth averaging
- Frame latency tracking
- Input prediction statistics
- JSON metrics export

### Input Prediction (`input/`)

**input_proxy.c**
- Predictive input processing
- Frame ID-based event tracking
- Reconciliation with server acknowledgments

**scroll_smoother.c**
- Velocity-based scroll smoothing
- Discrete to continuous conversion
- Exponential smoothing algorithms

**reconciliation.c**
- Frame-to-event mapping
- Prediction accuracy tracking
- Correction application

### Compositor Integration (`compositor/`)

**wl_input.c**
- Input device registration
- Event interception hooks
- Integration with input proxy
- Ready for wlroots connection

**wl_surface.c**
- Surface registration and tracking
- Frame ID generation
- Frame presentation notification
- Latency calculation

### Lens Adapters (`lenses/`)

**lens.h**
- Unified transport interface
- Lens operation callbacks
- Automatic lens selection

**lens_waypipe.c**
- Waypipe transport implementation
- Command building and execution
- Process management

**lens_sunshine.c / lens_moonlight.c**
- Implementations following the Waypipe pattern (fork/exec + exec handshake)

## Data Flow

### Input Prediction Flow

```
User Input
    │
    ▼
Compositor Hook (wl_input.c)
    │
    ▼
Input Proxy (input_proxy.c)
    │
    └─→ C Predictor
        └─→ Simple extrapolation
    │
    ▼
Predicted Event + Frame ID
    │
    ├─→ Local Feedback (immediate)
    │
    └─→ Remote Transport (waypipe)
        │
        ▼
    Remote Application
        │
        ▼
    Frame Presentation
        │
        ▼
    Reconciliation
        │
        └─→ Update Prediction Model
```

### Frame Tracking Flow

```
Frame Commit
    │
    ▼
compositor_generate_frame_id()
    │
    ├─→ Store Frame Timestamp
    │
    └─→ Return Frame ID
    │
    ▼
Input Events Tagged with Frame ID
    │
    ▼
Frame Presentation
    │
    ▼
compositor_notify_frame_presented()
    │
    ├─→ Calculate Latency
    │   └─→ Update Metrics
    │
    └─→ Trigger Reconciliation
        └─→ Compare Predicted vs Actual
```

## Performance Profiles

### Low Latency
- Target: 16ms end-to-end
- Frame rate: 120fps
- Compression: lz4
- Codec: h264
- Prediction: Enabled (16ms window)

### Balanced (Default)
- Target: 50ms end-to-end
- Frame rate: 60fps
- Compression: lz4
- Codec: h264
- Prediction: Enabled (16ms window)

### High Quality
- Target: 100ms end-to-end
- Frame rate: 60fps
- Compression: zstd
- Codec: h265
- Prediction: Disabled

### Bandwidth Constrained
- Target: 100ms end-to-end
- Frame rate: 30fps
- Compression: zstd
- Codec: h265
- Bandwidth limit: 10 Mbps
- Prediction: Enabled (33ms window)

## Integration Points

### Waypipe Integration
- Process launching via fork/exec
- Command building from configuration
- Process monitoring and cleanup
- Environment variable support

### wlroots Integration (Pending)
- Wayland display connection
- Input device listener registration
- Surface frame callback setup
- Event interception callbacks

### Rust Integration
- C-compatible ABI (cdylib)
- Dynamic loading with fallback
- Zero-copy data structures where possible
- Performance-critical path only

## Metrics and Observability

### Collected Metrics
- **Latency**: End-to-end, input lag, frame delay
- **Frame**: FPS, dropped frames, total frames
- **Bandwidth**: RX/TX bytes per second (time-averaged)
- **Input**: Predicted events, reconciled events, accuracy

### Export Formats
- JSON (current)
- Prometheus (planned)
- InfluxDB (planned)

### Auto-tuning
- Profile selection based on metrics
- Prediction window adjustment
- Compression level optimization
- Frame rate adaptation

## Build System

### Components
- **Core C modules**: Compiled to object files, linked into library
- **Tests**: Separate test executables with Makefile
- **Installation**: Libraries and headers to system paths

### Dependencies
- **System**: waypipe (baseline), json-c (for JSON config parsing), libwayland (for compositor stubs)
- **Build**: gcc/clang, pkg-config
- **Optional**: wlroots (for full compositor integration)

## Future Enhancements

1. **Full wlroots Integration**: Complete Wayland connection and event handling
2. **Sunshine/Moonlight Lenses**: Implement transport adapters
3. **Advanced Prediction**: Machine learning-based prediction models
4. **Multi-app Support**: Session management for multiple applications
5. **Network Optimization**: Adaptive compression and codec selection

## Design Decisions

### Why C-first?
- Direct alignment with Wayland, wlroots, libinput
- No runtime overhead from language features
- Predictable performance characteristics
- Easy integration with upstream projects

### Why C-only?
- **C-only baseline** is the portability contract, but the project is intentionally **hybrid**:
  - Baseline always works without Rust/Python (widest reach, simplest packaging).
  - Rust and Python remain **optional accelerators/tooling** that must never block baseline.
  - This avoids long-lived divergent branches while keeping determinism.

### Why Glue Layer Only?
- Maintains upstream compatibility
- Encourages upstream contributions
- Reduces maintenance burden
- Follows Unix philosophy

---

**Status**: Architecture is stable and production-ready. Framework supports all critical functionality with clear extension points for future enhancements.

