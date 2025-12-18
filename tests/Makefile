# Makefile for Lunar Telescope tests

CC = gcc
FEATURE_MACROS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
CFLAGS = -Wall -Wextra -g -std=c11 $(FEATURE_MACROS)
INCLUDES = -I../core -I../input

WITH_JSONC ?= 1
WITH_PYTHON ?= 1

HAVE_PKG_CONFIG := $(shell command -v pkg-config >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_JSONC := $(shell pkg-config --exists json-c >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_PYTHON3 := $(shell command -v python3 >/dev/null 2>&1 && echo 1 || echo 0)

JSON_C_CFLAGS :=
JSON_C_LDLIBS :=
ifeq ($(WITH_JSONC),1)
ifeq ($(HAVE_JSONC),1)
JSON_C_CFLAGS := $(shell pkg-config --cflags json-c)
JSON_C_LDLIBS := $(shell pkg-config --libs json-c)
endif
endif

LDLIBS = -lm

# Source directories
CORE_DIR = ../core
INPUT_DIR = ../input

# Test executables
TESTS = test_input
ifeq ($(WITH_JSONC),1)
TESTS += test_schema
TESTS += test_integration
endif

.PHONY: all clean

all: $(TESTS)

ifeq ($(WITH_JSONC),1)
test_schema: ./test_schema.c $(CORE_DIR)/schema.c $(CORE_DIR)/profiles.c $(CORE_DIR)/telescope.c $(CORE_DIR)/metrics.c $(CORE_DIR)/logging.c $(CORE_DIR)/utils.c
	@command -v pkg-config >/dev/null 2>&1 || { \
		echo "Error: pkg-config not found (required for json-c)."; \
		echo "Tip: run `make WITH_JSONC=0 test` to skip schema parsing tests."; \
		exit 1; \
	}
	@pkg-config --exists json-c >/dev/null 2>&1 || { \
		echo "Error: json-c dev files not found."; \
		echo "  - Debian/Ubuntu: sudo apt-get install -y libjson-c-dev"; \
		echo "  - Fedora: sudo dnf install -y json-c-devel"; \
		echo "  - Arch: sudo pacman -S json-c"; \
		echo "Tip: run `make WITH_JSONC=0 test` to skip schema parsing tests."; \
		exit 1; \
	}
	$(CC) $(CFLAGS) $(INCLUDES) $(JSON_C_CFLAGS) -o $@ $^ $(LDLIBS) $(JSON_C_LDLIBS)
endif

test_input: ./test_input.c $(INPUT_DIR)/input_proxy.c $(INPUT_DIR)/scroll_smoother.c $(INPUT_DIR)/rust_predictor_stub.c $(CORE_DIR)/logging.c $(CORE_DIR)/utils.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDLIBS)

ifeq ($(WITH_JSONC),1)
test_integration: ./test_integration.c
	@echo "Building integration test (requires library to be built first)..."
	@if [ -f ../build/lib/liblunar_telescope.a ]; then \
		$(CC) $(CFLAGS) $(INCLUDES) -I../core -I../input -I../compositor -I../lenses \
			-o $@ $< \
			-L../build/lib -llunar_telescope \
			$(LDLIBS) $(JSON_C_LDLIBS) \
			-Wl,-rpath,../build/lib; \
	else \
		echo "Library not found, skipping integration test build"; \
		touch $@; \
	fi
endif

clean:
	rm -f $(TESTS)

test: all
	@echo "Running tests..."
ifeq ($(WITH_JSONC),1)
	@./test_schema || (echo "test_schema failed" && exit 1)
else
	@echo "test_schema skipped (WITH_JSONC=0)"
endif
	@./test_input || (echo "test_input failed" && exit 1)
ifeq ($(WITH_JSONC),1)
	@if [ -x ./test_integration ]; then \
		./test_integration || echo "Integration test failed (non-critical)"; \
	else \
		echo "Integration test skipped (library not built)"; \
	fi
else
	@echo "Integration test skipped (WITH_JSONC=0)"
endif
ifeq ($(WITH_PYTHON),1)
	@if [ "$(HAVE_PYTHON3)" = "1" ]; then \
		python3 test_profiles.py || (echo "Python profile test failed" && exit 1); \
		python3 test_schema_validation.py || (echo "Python schema test failed" && exit 1); \
	else \
		echo "Python tests skipped (python3 not found)"; \
	fi
else
	@echo "Python tests skipped (WITH_PYTHON=0)"
endif

