# Lunar Telescope Design Constraints

## Policy: Glue Layer Only

**Lunar Telescope shall only implement the experience and orchestration layer.**

### Rules

1. **No vendored upstream code**

   * Do not include Waypipe, Sunshine, Moonlight source code.
   * All dependencies must be installed from the system (e.g., nixpkgs).

2. **No submodules for upstream**

   * Submodules create maintenance overhead and discourage upstream collaboration.

3. **Upstream patches**

   * Minimal patches may be created for upstream projects.
   * All patches must be submitted upstream immediately.
   * Patches live in `upstream/` for reference only.

4. **Glue responsibilities only**

   * Policy decisions, session orchestration, input prediction, performance heuristics.
   * Do not implement encoding/decoding or transport protocols.

5. **Separation of concerns**

   * All Waypipe/Sunshine/Moonlight internals are strictly off-limits.
   * The repo must compile and operate without modifications to upstream components.

6. **NixOS Compliance**

   * Build must rely on system-provided packages.
   * Ensure reproducibility and maintainability.

### Enforcement

* Any pull request that vendors upstream code or bypasses system dependencies will be rejected.
* Reviewers must verify that all patches have an upstream submission record.
* Glue layer policy must be referenced in all design and architecture documentation.

**End of Policy**
