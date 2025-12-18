# Lunar Telescope Main Makefile
#
# Builds the complete Lunar Telescope framework including:
# - Core C modules
# - Input prediction (C + Rust)
# - Compositor integration
# - Lens adapters
# - Tests

.PHONY: all clean install uninstall test help core input compositor lenses check-deps check-deps-jsonc check-runtime doctor hooks-install hooks-uninstall preflight preflight-ci preflight-baseline preflight-rust preflight-format coverage coverage-report preflight-strict preflight-sanitize preflight-tsan preflight-tidy pr pr-create pr-open pr-status
.DEFAULT_GOAL := all

# Configuration
CC ?= gcc
RUSTC ?= cargo
FEATURE_MACROS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700

# Strictness toggles
WERROR ?= 0
WERROR_CFLAGS :=
ifeq ($(WERROR),1)
WERROR_CFLAGS += -Werror
endif

SANITIZE ?= 0
SANITIZE_KIND ?= address,undefined
SANITIZE_CFLAGS :=
SANITIZE_LDFLAGS :=
ifeq ($(SANITIZE),1)
SANITIZE_CFLAGS += -O1 -fno-omit-frame-pointer -fsanitize=$(SANITIZE_KIND) -fno-sanitize-recover=all
SANITIZE_LDFLAGS += -fsanitize=$(SANITIZE_KIND)
endif
COVERAGE ?= 0
COVERAGE_CFLAGS :=
COVERAGE_LDFLAGS :=
ifeq ($(COVERAGE),1)
COVERAGE_CFLAGS += -Og --coverage
COVERAGE_LDFLAGS += --coverage
endif

CFLAGS = -Wall -Wextra -g -std=c11 -fPIC $(FEATURE_MACROS) $(COVERAGE_CFLAGS)
CFLAGS += $(WERROR_CFLAGS) $(SANITIZE_CFLAGS)
LDFLAGS = -lm $(COVERAGE_LDFLAGS) $(SANITIZE_LDFLAGS)
INCLUDES = -I. -Icore -Iinput -Icompositor -Ilenses

# Hybrid build options
# - C-only baseline is always supported.
# - Rust and Python are optional accelerators/tooling.
#
# By default we prefer the C-only baseline (widest reach); opt in to Rust with WITH_RUST=1.
WITH_RUST ?= 0
WITH_JSONC ?= 1

HAVE_CARGO := $(shell command -v $(RUSTC) >/dev/null 2>&1 && echo 1 || echo 0)

# Directories
CORE_DIR = core
INPUT_DIR = input
COMPOSITOR_DIR = compositor
LENSES_DIR = lenses
RUST_DIR = rust/input_predictor
TESTS_DIR = tests
BUILD_DIR = build
LIB_DIR = $(BUILD_DIR)/lib
OBJ_DIR = $(BUILD_DIR)/obj

# Dependencies
HAVE_PKG_CONFIG := $(shell command -v pkg-config >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_JSONC := $(shell pkg-config --exists json-c >/dev/null 2>&1 && echo 1 || echo 0)

ifeq ($(HAVE_JSONC),1)
JSON_C_CFLAGS := $(shell pkg-config --cflags json-c)
JSON_C_LDFLAGS := $(shell pkg-config --libs json-c)
else
JSON_C_CFLAGS :=
JSON_C_LDFLAGS :=
endif

DEFS :=
ifeq ($(WITH_JSONC),1)
DEFS += -DLT_HAVE_JSONC=1
else
DEFS += -DLT_HAVE_JSONC=0
endif

# Core objects
CORE_OBJS = $(OBJ_DIR)/schema.o \
            $(OBJ_DIR)/profiles.o \
            $(OBJ_DIR)/telescope.o \
            $(OBJ_DIR)/metrics.o \
            $(OBJ_DIR)/logging.o \
            $(OBJ_DIR)/utils.o

# Input objects
INPUT_OBJS = $(OBJ_DIR)/input_proxy.o \
             $(OBJ_DIR)/scroll_smoother.o
ifeq ($(WITH_RUST),0)
INPUT_OBJS += $(OBJ_DIR)/rust_predictor_stub.o
endif

# Compositor objects
COMPOSITOR_OBJS = $(OBJ_DIR)/wl_input.o \
                  $(OBJ_DIR)/wl_surface.o \
                  $(OBJ_DIR)/wlroots_glue.o

# Lens objects
LENS_OBJS = $(OBJ_DIR)/lens_waypipe.o \
            $(OBJ_DIR)/lens_sunshine.o \
            $(OBJ_DIR)/lens_moonlight.o

# Rust predictor artifacts (optional)
RUST_TARGET_DIR = $(RUST_DIR)/target/release
RUST_ARTIFACT_A = $(RUST_TARGET_DIR)/libinput_predictor.a
RUST_ARTIFACT_SO = $(RUST_TARGET_DIR)/libinput_predictor.so
RUST_ARTIFACT_DYLIB = $(RUST_TARGET_DIR)/libinput_predictor.dylib
RUST_LIB = $(LIB_DIR)/libinput_predictor.a
RUST_SO = $(LIB_DIR)/libinput_predictor.so
RUST_STAMP = $(LIB_DIR)/input_predictor.stamp

# Output library
OUTPUT_LIB = $(LIB_DIR)/liblunar_telescope.a
OUTPUT_SO = $(LIB_DIR)/liblunar_telescope.so

help:
	@echo "Lunar Telescope Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all components (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install libraries and headers"
	@echo "  uninstall    - Remove installed files"
	@echo "  test         - Run all tests"
	@echo "  doctor       - Check build/runtime dependencies (no network, deterministic)"
	@echo "  check-runtime- Check runtime binaries (waypipe required; sunshine/moonlight optional)"
	@echo "  hooks-install- Install git pre-push hook (runs CI-equivalent preflight before push)"
	@echo "  hooks-uninstall- Remove git pre-push hook"
	@echo "  pr           - Run baseline preflight, push current branch, and open a PR via gh"
	@echo "  pr-open      - Open the current branch PR (if it exists) via gh"
	@echo "  pr-status    - Show PR status for current branch via gh"
	@echo "  preflight    - Run local checks before pushing (best-effort; skips unavailable tools)"
	@echo "  preflight-ci - Mirror CI checks locally (requires: json-c dev, python3; rust optional)"
	@echo "  preflight-baseline - Baseline build+tests (WITH_RUST=0 WITH_JSONC=0)"
	@echo "  preflight-strict - Stricter-than-CI local checks (includes C coverage)"
	@echo "  preflight-sanitize - C sanitizers (ASan+UBSan) build+tests (clang)"
	@echo "  preflight-tsan - C ThreadSanitizer build+tests (clang; slower)"
	@echo "  preflight-tidy - clang-tidy (changed C files vs base ref)"
	@echo "  coverage     - Generate C coverage report (gcovr) and enforce minimum threshold"
	@echo "  core         - Build core C modules"
	@echo "  input        - Build input prediction modules"
	@echo "  compositor   - Build compositor integration"
	@echo "  lenses       - Build lens adapters"
	@echo "  rust         - Build Rust input predictor (WITH_RUST=1)"
	@echo "  help         - Show this help message"

# Create directories
$(BUILD_DIR) $(LIB_DIR) $(OBJ_DIR):
	mkdir -p $@

# Dependency checks
check-deps: check-deps-jsonc check-deps-rust

check-deps-jsonc:
ifeq ($(WITH_JSONC),1)
	@command -v pkg-config >/dev/null 2>&1 || { \
		echo "Error: pkg-config not found. Install pkg-config (or set WITH_JSONC=0 to build without JSON parsing)."; \
		exit 1; \
	}
	@pkg-config --exists json-c >/dev/null 2>&1 || { \
		echo "Error: json-c development files not found."; \
		echo "  - Debian/Ubuntu: sudo apt-get install -y libjson-c-dev"; \
		echo "  - Fedora: sudo dnf install -y json-c-devel"; \
		echo "  - Arch: sudo pacman -S json-c"; \
		echo "Or set WITH_JSONC=0 to build without config parsing (telescope_config_load will return -ENOTSUP)."; \
		exit 1; \
	}
endif

# Rust dependency check (only when enabled)
check-deps-rust:
ifeq ($(WITH_RUST),1)
	@command -v $(RUSTC) >/dev/null 2>&1 || { \
		echo "Error: cargo not found. Install Rust toolchain or build with WITH_RUST=0."; \
		exit 1; \
	}
endif

# Rust build (optional)
rust: check-deps-rust $(RUST_STAMP)

$(RUST_STAMP): check-deps-rust | $(LIB_DIR)
ifeq ($(WITH_RUST),1)
	@echo "Building Rust input predictor..."
	cd $(RUST_DIR) && $(RUSTC) build --release
	@test -f "$(RUST_ARTIFACT_A)" || { echo "Error: expected staticlib not found at $(RUST_ARTIFACT_A)."; exit 1; }
	@if [ -f "$(RUST_ARTIFACT_SO)" ]; then \
		cp "$(RUST_ARTIFACT_SO)" "$(RUST_SO)"; \
	elif [ -f "$(RUST_ARTIFACT_DYLIB)" ]; then \
		cp "$(RUST_ARTIFACT_DYLIB)" "$(RUST_SO)"; \
	else \
		echo "Error: expected cdylib not found at $(RUST_ARTIFACT_SO) (or .dylib)."; \
		exit 1; \
	fi
	@cp "$(RUST_ARTIFACT_A)" "$(RUST_LIB)"
	@touch "$(RUST_STAMP)"
	@echo "Rust predictor built"
else
	@echo "Rust predictor disabled (WITH_RUST=0); using C stub predictor."
	@touch "$(RUST_STAMP)"
endif

$(RUST_LIB): $(RUST_STAMP)

# Runtime checks (upstream transports are system-provided; we do not vendor/fetch them)
check-runtime:
	@missing=0; \
	echo "Runtime dependency check (system-installed upstreams):"; \
	echo "  - Policy: no vendoring, no submodules, no Makefile auto-fetch. See docs/UPSTREAM_DEPENDENCIES.md"; \
	echo "  - Licenses: verbatim upstream texts are recorded in docs/upstream-licenses/ (see docs/DEPENDENCY_ACQUISITION.md)"; \
	echo ""; \
	if command -v waypipe >/dev/null 2>&1; then echo "  ✓ waypipe found"; else echo "  ✗ waypipe missing (required baseline lens)"; missing=1; fi; \
	if command -v sunshine >/dev/null 2>&1; then echo "  ✓ sunshine found"; else echo "  ✗ sunshine missing (optional)"; fi; \
	if command -v moonlight >/dev/null 2>&1; then echo "  ✓ moonlight found"; else echo "  ✗ moonlight missing (optional)"; fi; \
	echo ""; \
	echo "Notes:"; \
	echo "  - Baseline support requires waypipe; Sunshine/Moonlight are optional accelerators."; \
	echo "  - Lens exec handshakes still fail-fast (missing binaries surface as -errno)."; \
	echo "  - Install via your distro packages or Nix (nixpkgs). Licenses are governed by upstream LICENSE files."; \
	if [ $$missing -ne 0 ]; then exit 1; fi

doctor: check-deps check-runtime

# Local preflight (best-effort, deterministic, no network fetch)
preflight: preflight-baseline preflight-ci preflight-format

# Local strict preflight (must be >= CI)
preflight-strict: preflight-baseline preflight-ci preflight-sanitize preflight-tidy coverage preflight-format preflight-rust
	@echo "OK: preflight-strict"

# Sanitizers (C): address+undefined behavior sanitizers.
preflight-sanitize:
	@echo "== preflight-sanitize: ASan+UBSan (C) =="
	@if ! command -v clang >/dev/null 2>&1; then \
		echo "Error: clang not found (required for sanitizers). Use nix develop or install clang."; \
		exit 1; \
	fi
	@$(MAKE) clean >/dev/null
	@$(MAKE) -j$$(nproc) CC=clang WITH_RUST=0 WITH_JSONC=1 SANITIZE=1 WERROR=1 all >/dev/null
	@$(MAKE) -C tests test CC=clang WITH_JSONC=1 WITH_PYTHON=0 SANITIZE=1 WERROR=1 >/dev/null
	@echo "OK: sanitizers"

# ThreadSanitizer (C): slower; best suited for nightly or dedicated CI lanes.
preflight-tsan:
	@echo "== preflight-tsan: TSan (C) =="
	@if ! command -v clang >/dev/null 2>&1; then \
		echo "Error: clang not found (required for TSan). Use nix develop or install clang."; \
		exit 1; \
	fi
	@$(MAKE) clean >/dev/null
	@TSAN_OPTIONS="halt_on_error=1" \
		$(MAKE) -j$$(nproc) CC=clang WITH_RUST=0 WITH_JSONC=1 SANITIZE=1 SANITIZE_KIND=thread WERROR=1 all >/dev/null
	@TSAN_OPTIONS="halt_on_error=1" \
		$(MAKE) -C tests test CC=clang WITH_JSONC=1 WITH_PYTHON=0 SANITIZE=1 SANITIZE_KIND=thread WERROR=1 >/dev/null
	@echo "OK: tsan"

# clang-tidy on changed C sources (relative to base ref).
TIDY_BASE_REF ?= origin/main
TIDY_ALL ?= 0
preflight-tidy:
	@echo "== preflight-tidy: clang-tidy (changed files) =="
	@if ! command -v clang-tidy >/dev/null 2>&1; then \
		echo "Error: clang-tidy not found. Use nix develop or install clang-tools/clang-tidy."; \
		exit 1; \
	fi
	@base_ref="$(TIDY_BASE_REF)"; \
	files=""; \
	if [ "$(TIDY_ALL)" = "1" ]; then \
		files=$$(find core input compositor lenses -name '*.c' -print 2>/dev/null || true); \
	elif git rev-parse --is-inside-work-tree >/dev/null 2>&1 && git rev-parse --verify "$$base_ref" >/dev/null 2>&1; then \
		files=$$(git diff --name-only --diff-filter=ACMR "$$base_ref"...HEAD -- '*.c' || true); \
	elif git rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
		files=$$(git diff --name-only --diff-filter=ACMR -- '*.c' || true); \
	fi; \
	if [ -z "$$files" ]; then \
		echo "SKIP: clang-tidy (no target .c detected; set TIDY_ALL=1 or TIDY_BASE_REF=origin/main)"; \
		exit 0; \
	fi; \
	extra=""; \
	if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists json-c >/dev/null 2>&1; then \
		extra="$$(pkg-config --cflags json-c)"; \
	fi; \
	nix_inc=""; \
	if [ -n "$${LT_NIX_GLIBC_INCLUDE:-}" ]; then nix_inc="$$nix_inc -isystem $${LT_NIX_GLIBC_INCLUDE}"; fi; \
	if [ -n "$${NIX_CFLAGS_COMPILE:-}" ]; then nix_inc="$$nix_inc $${NIX_CFLAGS_COMPILE}"; fi; \
	echo "$$files" | xargs -I{} clang-tidy -quiet -warnings-as-errors='*' {} -- \
		$(FEATURE_MACROS) -std=c11 -DLT_HAVE_JSONC=1 $(INCLUDES) $$extra $$nix_inc

# Baseline contract check (this must always work)
preflight-baseline:
	@echo "== preflight-baseline: C-only baseline build+tests =="
	@$(MAKE) clean >/dev/null
	@$(MAKE) -j$$(nproc) CC=gcc WITH_RUST=0 WITH_JSONC=0 WERROR=1 >/dev/null
	@$(MAKE) CC=gcc WITH_JSONC=0 WERROR=1 test >/dev/null
	@echo "OK: baseline build+tests"

# Mirror CI job logic locally (as close as possible)
preflight-ci:
	@echo "== preflight-ci: mirror GitHub Actions Build and Test =="
	@if ! command -v python3 >/dev/null 2>&1; then \
		echo "Error: python3 is required for preflight-ci (mirrors CI)."; \
		exit 1; \
	fi
	@if ! command -v $(RUSTC) >/dev/null 2>&1; then \
		echo "Error: cargo is required for preflight-ci (mirrors CI Rust build step)."; \
		exit 1; \
	fi
	@$(MAKE) clean >/dev/null
	@$(MAKE) -j$$(nproc) CC=gcc all WITH_RUST=1 WITH_JSONC=1 WERROR=1 >/dev/null
	@$(MAKE) -C tests test CC=gcc WITH_JSONC=1 WITH_PYTHON=1 WERROR=1 >/dev/null
	@echo "OK: CI-equivalent build+tests"

# Optional Rust checks (only if cargo exists)
preflight-rust:
	@echo "== preflight-rust: optional Rust island =="
	@if command -v $(RUSTC) >/dev/null 2>&1; then \
		$(MAKE) WITH_RUST=1 rust >/dev/null; \
		(cd rust/input_predictor && cargo fmt -- --check >/dev/null 2>&1 || true); \
		(cd rust/input_predictor && cargo clippy -- -D warnings >/dev/null 2>&1 || true); \
		echo "OK: Rust preflight (best-effort)"; \
	else \
		echo "SKIP: cargo not found"; \
	fi

# Formatting checks (best-effort)
preflight-format:
	@echo "== preflight-format: best-effort formatting checks =="
	@if command -v clang-format >/dev/null 2>&1; then \
		base_ref="$${FORMAT_BASE_REF:-origin/main}"; \
		files=""; \
		if git rev-parse --is-inside-work-tree >/dev/null 2>&1 && git rev-parse --verify "$$base_ref" >/dev/null 2>&1; then \
			files=$$(git diff --name-only --diff-filter=ACMR "$$base_ref"...HEAD -- '*.c' '*.h' || true); \
		elif git rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
			files=$$(git diff --name-only --diff-filter=ACMR -- '*.c' '*.h' || true); \
		fi; \
		if [ -z "$$files" ]; then \
			echo "SKIP: clang-format (no changed .c/.h detected; set FORMAT_BASE_REF=origin/main to compare against base)"; \
		else \
			echo "$$files" | xargs clang-format --dry-run --Werror; \
			echo "OK: clang-format (changed files)"; \
		fi; \
	else \
		echo "SKIP: clang-format not found"; \
	fi

# Coverage (C): compile+run tests with coverage instrumentation and report via gcovr.
#
# NOTE: This uses the existing build/obj layout so coverage data is stable.
# Requires: gcovr, and a build/test run with COVERAGE=1.
COVERAGE_MIN ?= 25
GCOVR ?= gcovr
COVERAGE_DIR ?= build/coverage
COVERAGE_XML := $(COVERAGE_DIR)/coverage.xml
COVERAGE_HTML := $(COVERAGE_DIR)/index.html

coverage:
	@echo "== coverage: C line coverage via gcovr =="
	@if ! command -v $(GCOVR) >/dev/null 2>&1; then \
		echo "Error: gcovr not found. Use nix develop or install gcovr."; \
		exit 1; \
	fi
	@$(MAKE) clean >/dev/null
	@$(MAKE) -j$$(nproc) CC=gcc WITH_RUST=0 WITH_JSONC=1 COVERAGE=1 WERROR=1 all >/dev/null
	@$(MAKE) -C tests test CC=gcc WITH_JSONC=1 WITH_PYTHON=0 >/dev/null
	@mkdir -p "$(COVERAGE_DIR)"
	@$(GCOVR) -r . \
		--object-directory build/obj \
		--exclude 'tests/.*' \
		--exclude 'rust/.*' \
		--print-summary \
		--fail-under-line $(COVERAGE_MIN)
	@$(GCOVR) -r . \
		--object-directory build/obj \
		--exclude 'tests/.*' \
		--exclude 'rust/.*' \
		--xml-pretty --output "$(COVERAGE_XML)" >/dev/null
	@$(GCOVR) -r . \
		--object-directory build/obj \
		--exclude 'tests/.*' \
		--exclude 'rust/.*' \
		--html-details --output "$(COVERAGE_HTML)" >/dev/null
	@echo "OK: coverage >= $(COVERAGE_MIN)%"
	@echo "  - HTML: $(COVERAGE_HTML)"
	@echo "  - XML:  $(COVERAGE_XML)"

# Git hook installation (repo policy: always run local CI-equivalent checks before push)
# NOTE: Git hooks cannot be enforced purely by repo contents for security reasons.
# We therefore auto-install the hook during normal workflows, unless CI=true.
hooks-install:
	@if [ -n "$$CI" ]; then \
		echo "SKIP: hooks-install in CI"; \
		exit 0; \
	fi
	@if git rev-parse --git-dir >/dev/null 2>&1; then \
		GITDIR=$$(git rev-parse --git-dir); \
		HOOKSDIR="$$GITDIR/hooks"; \
		mkdir -p "$$HOOKSDIR"; \
		cp .githooks/pre-push "$$HOOKSDIR/pre-push"; \
		chmod +x "$$HOOKSDIR/pre-push"; \
		echo "Installed pre-push hook -> $$HOOKSDIR/pre-push"; \
	else \
		echo "SKIP: not a git repo (hooks-install)"; \
	fi

hooks-uninstall:
	@if git rev-parse --git-dir >/dev/null 2>&1; then \
		GITDIR=$$(git rev-parse --git-dir); \
		HOOKSDIR="$$GITDIR/hooks"; \
		rm -f "$$HOOKSDIR/pre-push"; \
		echo "Removed $$HOOKSDIR/pre-push"; \
	else \
		echo "SKIP: not a git repo (hooks-uninstall)"; \
	fi

# GH-based self-PR flow (PR-first trunk)
pr: pr-create

pr-create:
	@if ! command -v gh >/dev/null 2>&1; then \
		echo "Error: gh (GitHub CLI) not found."; \
		exit 1; \
	fi
	@if ! git rev-parse --git-dir >/dev/null 2>&1; then \
		echo "Error: not a git repo."; \
		exit 1; \
	fi
	@branch=$$(git rev-parse --abbrev-ref HEAD); \
	if [ "$$branch" = "main" ]; then \
		echo "Error: you're on main. Create a feature branch first (e.g., git switch -c feature/name)."; \
		exit 1; \
	fi; \
	echo "== baseline preflight =="; \
	$(MAKE) preflight-baseline; \
	echo "== push branch $$branch =="; \
	git push -u origin HEAD; \
	echo "== create PR (base=main) =="; \
	gh pr create --fill --base main || { \
		echo "NOTE: PR may already exist. Try: make pr-open"; \
		exit 1; \
	}

pr-open:
	@if ! command -v gh >/dev/null 2>&1; then echo "Error: gh not found."; exit 1; fi
	@gh pr view --web || { echo "No PR found for current branch."; exit 1; }

pr-status:
	@if ! command -v gh >/dev/null 2>&1; then echo "Error: gh not found."; exit 1; fi
	@gh pr status

# Core modules
core: $(CORE_OBJS)

$(OBJ_DIR)/schema.o: $(CORE_DIR)/schema.c $(CORE_DIR)/telescope.h | check-deps-jsonc $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) $(JSON_C_CFLAGS) -c -o $@ $<

$(OBJ_DIR)/profiles.o: $(CORE_DIR)/profiles.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/telescope.o: $(CORE_DIR)/telescope.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/metrics.o: $(CORE_DIR)/metrics.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/logging.o: $(CORE_DIR)/logging.c $(CORE_DIR)/logging.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/utils.o: $(CORE_DIR)/utils.c $(CORE_DIR)/utils.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

# Input modules
input: $(INPUT_OBJS)

$(OBJ_DIR)/input_proxy.o: $(INPUT_DIR)/input_proxy.c $(INPUT_DIR)/input.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/scroll_smoother.o: $(INPUT_DIR)/scroll_smoother.c $(INPUT_DIR)/input.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/rust_predictor_stub.o: $(INPUT_DIR)/rust_predictor_stub.c $(INPUT_DIR)/rust_predictor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

# Compositor modules
compositor: $(COMPOSITOR_OBJS)

$(OBJ_DIR)/wl_input.o: $(COMPOSITOR_DIR)/wl_input.c $(COMPOSITOR_DIR)/compositor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/wl_surface.o: $(COMPOSITOR_DIR)/wl_surface.c $(COMPOSITOR_DIR)/compositor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/wlroots_glue.o: $(COMPOSITOR_DIR)/wlroots_glue.c $(COMPOSITOR_DIR)/compositor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -DWLR_USE_UNSTABLE -c -o $@ $< 2>/dev/null || \
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

# Lens modules
lenses: $(LENS_OBJS)

$(OBJ_DIR)/lens_waypipe.o: $(LENSES_DIR)/lens_waypipe.c $(LENSES_DIR)/lens.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/lens_sunshine.o: $(LENSES_DIR)/lens_sunshine.c $(LENSES_DIR)/lens.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/lens_moonlight.o: $(LENSES_DIR)/lens_moonlight.c $(LENSES_DIR)/lens.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c -o $@ $<

# Static library
$(OUTPUT_LIB): $(CORE_OBJS) $(INPUT_OBJS) $(COMPOSITOR_OBJS) $(LENS_OBJS) | $(LIB_DIR)
	@echo "Creating static library..."
	ar rcs $@ $^
	@echo "Static library created: $@"

# Shared library
OUTPUT_SO_RUST_DEPS :=
ifeq ($(WITH_RUST),1)
OUTPUT_SO_RUST_DEPS += $(RUST_LIB) $(RUST_STAMP)
endif

$(OUTPUT_SO): $(CORE_OBJS) $(INPUT_OBJS) $(COMPOSITOR_OBJS) $(LENS_OBJS) $(OUTPUT_SO_RUST_DEPS) | $(LIB_DIR)
	@echo "Creating shared library..."
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(JSON_C_LDFLAGS) -Wl,-rpath,$(abspath $(LIB_DIR))
	@echo "Shared library created: $@"

# Build all
ALL_COMPONENTS = core input compositor lenses $(OUTPUT_LIB) $(OUTPUT_SO)
ifeq ($(WITH_RUST),1)
ALL_COMPONENTS := rust $(ALL_COMPONENTS)
endif

all: hooks-install check-deps | $(BUILD_DIR)
all: $(ALL_COMPONENTS)
	@echo ""
	@echo "Build complete!"
	@echo "  Static library: $(OUTPUT_LIB)"
	@echo "  Shared library: $(OUTPUT_SO)"
ifeq ($(WITH_RUST),1)
	@echo "  Rust predictor: enabled (WITH_RUST=1)"
else
	@echo "  Rust predictor: disabled (WITH_RUST=0)"
endif
ifeq ($(WITH_JSONC),1)
	@echo "  JSON parsing: enabled (json-c)"
else
	@echo "  JSON parsing: disabled (WITH_JSONC=0)"
endif

# Tests
test: check-deps-jsonc
	@$(MAKE) -C $(TESTS_DIR) test WITH_JSONC=$(WITH_JSONC)

# Install
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include/lunar-telescope

install: all
	@echo "Installing Lunar Telescope..."
	install -d $(LIBDIR)
	install -d $(INCDIR)
	install -d $(INCDIR)/core
	install -d $(INCDIR)/input
	install -d $(INCDIR)/compositor
	install -d $(INCDIR)/lenses
	install -m 644 $(OUTPUT_LIB) $(LIBDIR)/
	install -m 755 $(OUTPUT_SO) $(LIBDIR)/
	install -m 644 $(CORE_DIR)/telescope.h $(INCDIR)/core/
	install -m 644 $(INPUT_DIR)/input.h $(INCDIR)/input/
	install -m 644 $(INPUT_DIR)/rust_predictor.h $(INCDIR)/input/
	install -m 644 $(COMPOSITOR_DIR)/compositor.h $(INCDIR)/compositor/
	install -m 644 $(LENSES_DIR)/lens.h $(INCDIR)/lenses/
	@echo "Installation complete"

uninstall:
	@echo "Uninstalling Lunar Telescope..."
	rm -f $(LIBDIR)/liblunar_telescope.a
	rm -f $(LIBDIR)/liblunar_telescope.so
	rm -rf $(INCDIR)
	@echo "Uninstallation complete"

# Clean
clean:
	rm -rf $(BUILD_DIR)
	cd $(RUST_DIR) && $(RUSTC) clean 2>/dev/null || true
	cd $(TESTS_DIR) && $(MAKE) clean 2>/dev/null || true
	@echo "Clean complete"

