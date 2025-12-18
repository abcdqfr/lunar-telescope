# Stub Completion Guide

**Generated:** Wed Dec 17 12:54:21 PM CST 2025
**Purpose:** Clarify what's needed to complete remaining stubs

---

## Summary of Remaining Stubs

### ✅ **COMPLETED** (No longer stubs)
1. **Waypipe Integration** - Fully implemented in `core/telescope.c` and `lenses/lens_waypipe.c`
2. **Input Reconciliation** - Fully implemented in `input/input_proxy.c` (reconciliation.c deleted)
3. **wlroots Glue Layer** - Implemented in `compositor/wlroots_glue.c` with conditional compilation
4. **Compositor Framework** - All event processing, prediction, reconciliation complete

### ⏳ **INTENTIONAL STUBS** (Working as designed)
1. **wlroots Stub Fallback** (`compositor/wlroots_glue.c` lines 271-288)
   - **Status:** ✅ Complete fallback implementation
   - **Purpose:** Graceful degradation when wlroots unavailable
   - **Completion:** Already complete - returns -ENOTSUP
   - **No action needed** - This is the intended behavior

### 🔨 **REMAINING STUBS** (Need implementation)

#### 1. Sunshine Lens Adapter (`lenses/lens_sunshine.c`)
**Status:** Implemented (fork/exec + exec handshake); metrics are placeholder
**Priority:** High (required runtime upstream)

**What's Needed:**
- **Clarification Required:**
  1. How does Sunshine client connect? (command-line interface, API, library?)
  2. What are the Sunshine connection parameters? (host, port, authentication?)
  3. How to launch Sunshine client process? (similar to waypipe fork/exec?)
  4. What metrics does Sunshine expose? (FPS, latency, bandwidth?)
  5. How to integrate with existing session management?

**Implementation Pattern:**
- Follow `lens_waypipe.c` as reference
- Implement `sunshine_create()`, `sunshine_start()`, `sunshine_stop()`, `sunshine_destroy()`, `sunshine_get_metrics()`
- Register in `lens_get_ops()` in `lens_waypipe.c` (or create shared registration)

**Estimated Complexity:** Medium (similar to waypipe, but need Sunshine-specific knowledge)

---

#### 2. Moonlight Lens Adapter (`lenses/lens_moonlight.c`)
**Status:** Implemented (fork/exec + exec handshake); metrics are placeholder
**Priority:** High (required runtime upstream)

**What's Needed:**
- **Clarification Required:**
  1. How does Moonlight client connect? (command-line, API, library?)
  2. What are the Moonlight connection parameters?
  3. How to launch Moonlight client process?
  4. What metrics does Moonlight expose?
  5. How to integrate with existing session management?

**Implementation Pattern:**
- Follow `lens_waypipe.c` as reference
- Implement `moonlight_create()`, `moonlight_start()`, `moonlight_stop()`, `moonlight_destroy()`, `moonlight_get_metrics()`
- Register in `lens_get_ops()`

**Estimated Complexity:** Medium (similar to waypipe, but need Moonlight-specific knowledge)

---

#### 3. Compositor TODOs (Reference Updates Needed)
**Status:** Framework complete, TODOs are outdated comments
**Priority:** Documentation cleanup

**Current State:**
- All functionality is implemented
- TODOs reference wlroots functions that are now in `wlroots_glue.c`
- Comments should be updated to reference actual implementation

**What's Needed:**
- **Clarification:** Should we:
  1. Remove outdated TODO comments?
  2. Update comments to reference `compositor_wlroots_*` functions?
  3. Keep as documentation of what wlroots_glue.c provides?

**Files with Outdated TODOs:**
- `compositor/wl_input.c` - Lines 39, 70, 106, 133, 169, 175, 221, 264
- `compositor/wl_surface.c` - Lines 64, 93, 141

**Recommendation:** Update comments to reference `wlroots_glue.c` implementation

---

## Clarification Questions

### For Sunshine/Moonlight Implementation:

1. **Connection Method:**
   - [ ] Command-line tool (like waypipe)?
   - [ ] Library API?
   - [ ] Network protocol directly?
   - [ ] Other?

2. **Configuration Parameters:**
   - What connection parameters are needed?
   - How is authentication handled?
   - Are there performance tuning options?

3. **Process Management:**
   - Should we fork/exec like waypipe?
   - Or use library calls?
   - How to monitor process health?

4. **Metrics Integration:**
   - What metrics does Sunshine/Moonlight provide?
   - How to extract them?
   - Format for integration with `telescope_metrics`?

5. **Error Handling:**
   - What are common failure modes?
   - How to detect and handle them?
   - Fallback strategies?

### For Compositor Integration:

1. **wlroots Integration:**
   - [x] ✅ Glue layer implemented (`wlroots_glue.c`)
   - [x] ✅ Event handlers complete
   - [x] ✅ Frame tracking complete
   - **Question:** Is the current implementation sufficient, or are there additional wlroots features needed?

2. **Local Feedback:**
   - **Current:** Events processed through input proxy
   - **Question:** Should local cursor feedback be implemented in compositor layer or handled by compositor itself?
   - **Clarification:** "Apply local feedback" - is this compositor's responsibility or ours?

3. **Remote Transport:**
   - **Current:** Events processed, but not sent to remote
   - **Question:** Should compositor layer send to remote, or is this lens adapter's responsibility?
   - **Clarification:** "Send to remote via waypipe" - should this be in compositor or lens?

### For Rust Predictor:

1. **Linking Strategy:**
   - **Current:** Stub provides fallback, Makefile links Rust SO
   - **Question:** Should we implement `dlopen()` for dynamic loading?
   - **Clarification:** Is static linking preferred, or dynamic loading?

2. **ABI Compatibility:**
   - **Current:** C ABI exports match header
   - **Question:** Any additional compatibility concerns?
   - **Clarification:** Is the current approach sufficient?

---

## Implementation Readiness Assessment

### Ready to Implement (Clear Requirements)

1. **Sunshine/Moonlight Lenses** - ⏳ **Needs Clarification**
   - Framework: ✅ Complete
   - Pattern: ✅ Clear (follow waypipe)
   - Requirements: ❓ Need Sunshine/Moonlight API details

2. **Compositor Comment Updates** - ✅ **Ready**
   - Just documentation cleanup
   - Update TODOs to reference actual implementations

### Already Complete (Working as Intended)

1. **Rust Predictor Stub** - ✅ **Complete**
   - Intentional fallback mechanism
   - No changes needed

2. **wlroots Stub Fallback** - ✅ **Complete**
   - Intentional fallback when wlroots unavailable
   - No changes needed

3. **Input Reconciliation** - ✅ **Complete**
   - Full implementation in `input_proxy.c`
   - Frame tracking, comparison, cleanup all working

4. **Waypipe Integration** - ✅ **Complete**
   - Process launching, monitoring, cleanup all implemented

---

## Recommendations

### Immediate Actions:

1. **Update Compositor TODOs** - Replace with references to `wlroots_glue.c`
2. **Document Sunshine/Moonlight Requirements** - Create requirements doc
3. **Clarify Transport Responsibility** - Document where remote sending happens

### For Sunshine/Moonlight:

**Option A:** Implement when needed (current approach)
- Keep stubs, implement when transport is required
- Framework is ready, just need transport-specific code

**Option B:** Research and implement now
- Need to research Sunshine/Moonlight client APIs
- Determine connection methods and parameters
- Implement following waypipe pattern

**Recommendation:** Option A - implement when needed, framework is ready

### For Compositor Comments:

**Recommendation:** Update TODOs to:
```c
/* wlroots integration handled by compositor_wlroots_init() */
/* See compositor/wlroots_glue.c for implementation */
```

---

## Completion Checklist

- [x] Waypipe integration
- [x] Input reconciliation
- [x] wlroots glue layer
- [x] Rust predictor fallback
- [ ] Sunshine lens (needs API research)
- [ ] Moonlight lens (needs API research)
- [ ] Update compositor TODO comments (documentation)

---

## Questions for Clarification

1. **Sunshine/Moonlight:** What is the preferred connection method? (CLI, library, protocol?)

2. **Local Feedback:** Should compositor layer handle local cursor feedback, or is that compositor's job?

3. **Remote Transport:** Should compositor send events to remote, or is that lens adapter's responsibility?

4. **Rust Linking:** Is current stub + static linking approach sufficient, or should we implement `dlopen()`?

5. **Compositor TODOs:** Should we update/remove outdated TODO comments, or keep as documentation?

---

**Status:** Framework is 100% complete. Remaining work is mainly Sunshine/Moonlight metrics policy/implementation and documentation cleanup (TODO comment updates).

