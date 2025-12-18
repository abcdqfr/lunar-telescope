# 🌕 Lunar Telescope

**Composable Remote Application Publishing for Wayland**

> Waypipe is the telescope body.
> Sunshine and Moonlight are lenses.
> Lunar Telescope decides which optics to use — and when to lie about latency.

---

## What This Is

**Lunar Telescope** is a **C-first orchestration layer** for remote Wayland applications.

It does **not** replace Waypipe, Sunshine, or Moonlight.
It **composes** them into a single, deterministic system that delivers:

* Per-application remote publishing
* Native-feeling input (scroll, pointer)
* Transport selection based on app behavior
* Predictive input *without sacrificing correctness*

The goal is to match — and eventually rival — the user-perceived responsiveness of
**VMware Horizon / Citrix ICA**, using open, upstream-friendly components.

---

## What This Is *Not*

* ❌ Not a remote desktop
* ❌ Not a screen streamer
* ❌ Not a monolithic protocol
* ❌ Not a Python-driven runtime

Lunar Telescope is **policy**, not mechanism.

---

## Core Design Principles

### 1. C at the Boundary

All hot-path code is written in **C**:

* Wayland
* libinput
* wlroots
* VAAPI / DRM / PipeWire

This aligns directly with:

* Waypipe
* Compositors
* Video pipelines

Python is not used in the runtime.

---

### 2. Composability Over Reinvention

We lean on **battle-tested transports**:

| Use Case                  | Transport |
| ------------------------- | --------- |
| Correctness, low overhead | Waypipe   |
| High-motion video         | Sunshine  |
| Low-latency decode        | Moonlight |

Lunar Telescope chooses the lens per app.

---

### 3. Predictive Input (Scoped, Honest)

We apply prediction **only where humans perceive latency**:

* ✅ Pointer motion
* ✅ Scroll / touchpad
* ❌ Text input
* ❌ Clicks / activation

Prediction is:

* Local
* Reversible
* Reconciled on frame acknowledgment

This mirrors how VMware Blast and Citrix ICA work internally — without owning the compositor.

---

### 4. Determinism Beats Cleverness

Constraints are deliberate:

* No GC
* No hidden schedulers
* No implicit buffering
* Observable state at all times

This project optimizes **perceived latency**, not theoretical purity.

---

## Architecture Overview

```
┌───────────────┐
│ Local Input   │
└───────┬───────┘
        │
        ▼
┌────────────────────┐
│ Input Proxy        │  (C / optional Rust island)
│ - Predict          │
│ - Coalesce         │
│ - Reconcile        │
└───────┬────────────┘
        │
        ├──► Immediate Local Feedback
        │
        ▼
┌────────────────────┐
│ Transport Lens     │
│ Waypipe / Sunshine │
└───────┬────────────┘
        │
        ▼
┌────────────────────┐
│ Remote App         │
│ (Unmodified)       │
└────────────────────┘
```

---

## Repository Layout (v2)

```
lunar-telescope/
├── core/                 # C core (policy + orchestration)
│   ├── telescope.c
│   ├── profiles.c
│   ├── schema.c
│   └── telescope.h
│
├── input/                # Predictive input layer
│   ├── input_proxy.c
│   ├── scroll_smoother.c
│   ├── reconciliation.c
│   └── input.h
│
├── lenses/               # Transport adapters
│   ├── lens_waypipe.c
│   ├── lens_sunshine.c
│   └── lens.h
│
├── compositor/           # Wayland / wlroots integration
│   ├── wl_input.c
│   ├── wl_surface.c
│   └── compositor.h
│

├── include/              # Public headers (upstreamable)
│
├── tests/                # Unit + latency tests
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── INPUT-PREDICTION.md
│   ├── WAYPIPE-UPSTREAMING.md
│   └── DESIGN-CONSTRAINTS.md
│
├── nix/
│   ├── module.nix
│   └── flake.nix
│
└── README.md
```

---

## Relationship to Upstream Projects

### Waypipe

* Remains protocol-correct
* No prediction added upstream
* Lunar Telescope lives *around* it

### Sunshine / Moonlight

* Used when frame synthesis beats protocol fidelity
* No forks required

### Wayland Compositors

* Minimal hooks
* No compositor takeover
* wlroots-first targeting

---

## Why This Exists

Waypipe tells the truth.
Sunshine feels good.
Citrix lied convincingly.

**Lunar Telescope makes the lie optional, scoped, and reversible.**

That's the difference.

---

## Current Status

* ✅ Architecture locked
* ✅ C-first direction chosen
* ⏳ Input interception (wlroots)
* ⏳ Predictive scroll prototype
* ⏳ Lens arbitration logic

---

## Non-Goals

* Replacing Waypipe
* Competing with Sunshine
* Becoming a compositor
* Supporting X11

---

## License

MIT (tentative) — compatible with Waypipe and wlroots.
                                                                                                        



