# Lunar Telescope Main Makefile
#
# Builds the complete Lunar Telescope framework including:
# - Core C modules
# - Input prediction (C + Rust)
# - Compositor integration
# - Lens adapters
# - Tests

.PHONY: all clean install uninstall test help core input compositor lenses check-deps check-deps-jsonc check-runtime doctor
.DEFAULT_GOAL := all

# Configuration
CC = gcc
FEATURE_MACROS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
CFLAGS = -Wall -Wextra -g -std=c11 -fPIC $(FEATURE_MACROS)
LDFLAGS = -lm
INCLUDES = -I. -Icore -Iinput -Icompositor -Ilenses

# Build options (can be overridden: `make WITH_JSONC=0`)
WITH_JSONC ?= 1

# Runtime dependency policy
# - Strict-by-default: a "real" Lunar Telescope install includes all upstream lenses.
# - Escape hatch for constrained environments: `make ALLOW_INCOMPLETE_RUNTIME=1 check-runtime`
ALLOW_INCOMPLETE_RUNTIME ?= 0

# Directories
CORE_DIR = core
INPUT_DIR = input
COMPOSITOR_DIR = compositor
LENSES_DIR = lenses
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

# Compositor objects
COMPOSITOR_OBJS = $(OBJ_DIR)/wl_input.o \
                  $(OBJ_DIR)/wl_surface.o \
                  $(OBJ_DIR)/wlroots_glue.o

# Lens objects
LENS_OBJS = $(OBJ_DIR)/lens_waypipe.o \
            $(OBJ_DIR)/lens_sunshine.o \
            $(OBJ_DIR)/lens_moonlight.o

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
	@echo "  doctor       - Check build/test/runtime dependencies (strict by default)"
	@echo "  check-runtime- Check runtime binaries (waypipe/sunshine/moonlight) and print guidance"
	@echo "  core         - Build core C modules"
	@echo "  input        - Build input prediction modules"
	@echo "  compositor   - Build compositor integration"
	@echo "  lenses       - Build lens adapters"
	@echo "  help         - Show this help message"

# Create directories
$(BUILD_DIR) $(LIB_DIR) $(OBJ_DIR):
	mkdir -p $@

# Dependency checks
check-deps: check-deps-jsonc

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

# Runtime checks (upstream transports are system-provided; we do not vendor/fetch them)
check-runtime:
	@missing=0; \
	echo "Runtime dependency check (system-installed upstreams):"; \
	echo "  - Policy: no vendoring, no submodules, no Makefile auto-fetch. See docs/UPSTREAM_DEPENDENCIES.md"; \
	echo "  - Licenses: verbatim upstream texts are recorded in docs/upstream-licenses/ (see docs/DEPENDENCY_ACQUISITION.md)"; \
	echo ""; \
	if command -v waypipe >/dev/null 2>&1; then echo "  ✓ waypipe found"; else echo "  ✗ waypipe missing (REQUIRED)"; missing=1; fi; \
	if command -v sunshine >/dev/null 2>&1; then echo "  ✓ sunshine found"; else echo "  ✗ sunshine missing (REQUIRED)"; missing=1; fi; \
	if command -v moonlight >/dev/null 2>&1; then echo "  ✓ moonlight found"; else echo "  ✗ moonlight missing (REQUIRED)"; missing=1; fi; \
	echo ""; \
	echo "Notes:"; \
	echo "  - Supported installations provide all three upstream lenses: waypipe, sunshine, moonlight."; \
	echo "  - Lens exec handshakes still fail-fast (missing binaries surface as -errno)."; \
	echo "  - Install via your distro packages or Nix (nixpkgs). Licenses are governed by upstream LICENSE files."; \
	if [ $$missing -ne 0 ] && [ "$(ALLOW_INCOMPLETE_RUNTIME)" != "1" ]; then \
		echo ""; \
		echo "ERROR: missing runtime dependencies (strict mode)."; \
		echo "  - To ignore for constrained environments: make ALLOW_INCOMPLETE_RUNTIME=1 check-runtime"; \
		exit 1; \
	fi

doctor: check-deps check-runtime

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
$(OUTPUT_SO): $(CORE_OBJS) $(INPUT_OBJS) $(COMPOSITOR_OBJS) $(LENS_OBJS) | $(LIB_DIR)
	@echo "Creating shared library..."
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(JSON_C_LDFLAGS) -Wl,-rpath,$(abspath $(LIB_DIR))
	@echo "Shared library created: $@"

# Build all
ALL_COMPONENTS = core input compositor lenses $(OUTPUT_LIB) $(OUTPUT_SO)

all: check-deps | $(BUILD_DIR)
all: $(ALL_COMPONENTS)
	@echo ""
	@echo "Build complete!"
	@echo "  Static library: $(OUTPUT_LIB)"
	@echo "  Shared library: $(OUTPUT_SO)"
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
	cd $(TESTS_DIR) && $(MAKE) clean 2>/dev/null || true
	@echo "Clean complete"

