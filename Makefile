# Lunar Telescope Main Makefile
#
# Builds the complete Lunar Telescope framework including:
# - Core C modules
# - Input prediction (C + Rust)
# - Compositor integration
# - Lens adapters
# - Tests

.PHONY: all clean install uninstall test help
.DEFAULT_GOAL := all

# Configuration
CC = gcc
RUSTC = cargo
CFLAGS = -Wall -Wextra -g -std=c11 -fPIC
LDFLAGS = -lm
INCLUDES = -I. -Icore -Iinput -Icompositor -Ilenses

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
JSON_C_CFLAGS = $(shell pkg-config --cflags json-c 2>/dev/null || echo "")
JSON_C_LDFLAGS = $(shell pkg-config --libs json-c 2>/dev/null || echo "-ljson-c")

# Core objects
CORE_OBJS = $(OBJ_DIR)/schema.o \
            $(OBJ_DIR)/profiles.o \
            $(OBJ_DIR)/telescope.o \
            $(OBJ_DIR)/metrics.o \
            $(OBJ_DIR)/logging.o \
            $(OBJ_DIR)/utils.o

# Input objects
INPUT_OBJS = $(OBJ_DIR)/input_proxy.o \
             $(OBJ_DIR)/scroll_smoother.o \
             $(OBJ_DIR)/rust_predictor_stub.o

# Compositor objects
COMPOSITOR_OBJS = $(OBJ_DIR)/wl_input.o \
                  $(OBJ_DIR)/wl_surface.o \
                  $(OBJ_DIR)/wlroots_glue.o

# Lens objects
LENS_OBJS = $(OBJ_DIR)/lens_waypipe.o \
            $(OBJ_DIR)/lens_sunshine.o \
            $(OBJ_DIR)/lens_moonlight.o

# Rust library
RUST_LIB = $(LIB_DIR)/libinput_predictor.a
RUST_SO = $(LIB_DIR)/libinput_predictor.so

# Output library
OUTPUT_LIB = $(LIB_DIR)/liblunar_telescope.a
OUTPUT_SO = $(LIB_DIR)/liblunar_telescope.so

# Test executables
TEST_EXECS = $(BUILD_DIR)/test_schema \
             $(BUILD_DIR)/test_input

help:
	@echo "Lunar Telescope Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all components (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install libraries and headers"
	@echo "  uninstall    - Remove installed files"
	@echo "  test         - Run all tests"
	@echo "  rust         - Build Rust input predictor"
	@echo "  core         - Build core C modules"
	@echo "  input        - Build input prediction modules"
	@echo "  compositor   - Build compositor integration"
	@echo "  lenses       - Build lens adapters"
	@echo "  help         - Show this help message"

# Create directories
$(BUILD_DIR) $(LIB_DIR) $(OBJ_DIR):
	mkdir -p $@

# Rust predictor library
rust: $(RUST_LIB) $(RUST_SO)

$(RUST_LIB) $(RUST_SO): | $(LIB_DIR)
	@echo "Building Rust input predictor..."
	cd $(RUST_DIR) && $(RUSTC) build --release
	@mkdir -p $(LIB_DIR)
	@cp $(RUST_DIR)/target/release/libinput_predictor.a $(RUST_LIB) 2>/dev/null || true
	@cp $(RUST_DIR)/target/release/libinput_predictor.so $(RUST_SO) 2>/dev/null || true
	@echo "Rust predictor built"

# Core modules
core: $(CORE_OBJS)

$(OBJ_DIR)/schema.o: $(CORE_DIR)/schema.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(JSON_C_CFLAGS) -c -o $@ $<

$(OBJ_DIR)/profiles.o: $(CORE_DIR)/profiles.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/telescope.o: $(CORE_DIR)/telescope.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/metrics.o: $(CORE_DIR)/metrics.c $(CORE_DIR)/telescope.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/logging.o: $(CORE_DIR)/logging.c $(CORE_DIR)/logging.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/utils.o: $(CORE_DIR)/utils.c $(CORE_DIR)/utils.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Input modules
input: $(INPUT_OBJS)

$(OBJ_DIR)/input_proxy.o: $(INPUT_DIR)/input_proxy.c $(INPUT_DIR)/input.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/scroll_smoother.o: $(INPUT_DIR)/scroll_smoother.c $(INPUT_DIR)/input.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/rust_predictor_stub.o: $(INPUT_DIR)/rust_predictor_stub.c $(INPUT_DIR)/rust_predictor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compositor modules
compositor: $(COMPOSITOR_OBJS)

$(OBJ_DIR)/wl_input.o: $(COMPOSITOR_DIR)/wl_input.c $(COMPOSITOR_DIR)/compositor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/wl_surface.o: $(COMPOSITOR_DIR)/wl_surface.c $(COMPOSITOR_DIR)/compositor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/wlroots_glue.o: $(COMPOSITOR_DIR)/wlroots_glue.c $(COMPOSITOR_DIR)/compositor.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -DWLR_USE_UNSTABLE -c -o $@ $< 2>/dev/null || \
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Lens modules
lenses: $(LENS_OBJS)

$(OBJ_DIR)/lens_waypipe.o: $(LENSES_DIR)/lens_waypipe.c $(LENSES_DIR)/lens.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/lens_sunshine.o: $(LENSES_DIR)/lens_sunshine.c $(LENSES_DIR)/lens.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_DIR)/lens_moonlight.o: $(LENSES_DIR)/lens_moonlight.c $(LENSES_DIR)/lens.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Static library
$(OUTPUT_LIB): $(CORE_OBJS) $(INPUT_OBJS) $(COMPOSITOR_OBJS) $(LENS_OBJS) | $(LIB_DIR)
	@echo "Creating static library..."
	ar rcs $@ $^
	@echo "Static library created: $@"

# Shared library
$(OUTPUT_SO): $(CORE_OBJS) $(INPUT_OBJS) $(COMPOSITOR_OBJS) $(LENS_OBJS) $(RUST_SO) | $(LIB_DIR)
	@echo "Creating shared library..."
	$(CC) -shared -o $@ $^ $(LDFLAGS) $(JSON_C_LDFLAGS) -Wl,-rpath,$(abspath $(LIB_DIR))
	@echo "Shared library created: $@"

# Build all
all: | $(BUILD_DIR)
all: rust core input compositor lenses $(OUTPUT_LIB) $(OUTPUT_SO)
	@echo ""
	@echo "Build complete!"
	@echo "  Static library: $(OUTPUT_LIB)"
	@echo "  Shared library: $(OUTPUT_SO)"
	@echo "  Rust predictor: $(RUST_SO)"

# Tests
test: $(TEST_EXECS)
	@echo "Running tests..."
	@cd $(TESTS_DIR) && $(MAKE) test

$(BUILD_DIR)/test_schema: $(TESTS_DIR)/test_schema.c $(CORE_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(CORE_OBJS) $(LDFLAGS) $(JSON_C_LDFLAGS)

$(BUILD_DIR)/test_input: $(TESTS_DIR)/test_input.c $(INPUT_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(INPUT_OBJS) $(LDFLAGS)

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

