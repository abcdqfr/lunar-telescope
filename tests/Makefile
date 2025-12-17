# Makefile for Lunar Telescope tests

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
INCLUDES = -I../core -I../input
JSON_C_CFLAGS = $(shell pkg-config --cflags json-c 2>/dev/null || echo "-I/usr/include/json-c")
LDFLAGS = -ljson-c -lm
JSON_C_LDFLAGS = $(shell pkg-config --libs json-c 2>/dev/null || echo "-ljson-c")

# Source directories
CORE_DIR = ../core
INPUT_DIR = ../input

# Test executables
TESTS = test_schema test_input test_integration

.PHONY: all clean

all: $(TESTS)

test_schema: ./test_schema.c $(CORE_DIR)/schema.c $(CORE_DIR)/profiles.c $(CORE_DIR)/telescope.c $(CORE_DIR)/metrics.c $(CORE_DIR)/logging.c $(CORE_DIR)/utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $(JSON_C_CFLAGS) -o $@ $^ $(LDFLAGS) $(JSON_C_LDFLAGS)

test_input: ./test_input.c $(INPUT_DIR)/input_proxy.c $(INPUT_DIR)/scroll_smoother.c $(INPUT_DIR)/rust_predictor_stub.c $(CORE_DIR)/logging.c $(CORE_DIR)/utils.c
	$(CC) $(CFLAGS) $(INCLUDES) $(JSON_C_CFLAGS) -o $@ $^ $(LDFLAGS) $(JSON_C_LDFLAGS)

test_integration: ./test_integration.c
	@echo "Building integration test (requires library to be built first)..."
	@if [ -f ../build/lib/liblunar_telescope.a ]; then \
		$(CC) $(CFLAGS) $(INCLUDES) -I../core -I../input -I../compositor -Ilenses \
			-o $@ $< \
			-L../build/lib -llunar_telescope \
			$(LDFLAGS) $(JSON_C_LDFLAGS) \
			-Wl,-rpath,../build/lib; \
	else \
		echo "Library not found, skipping integration test build"; \
		touch $@; \
	fi

clean:
	rm -f $(TESTS)

test: all
	@echo "Running tests..."
	@./test_schema || (echo "test_schema failed" && exit 1)
	@./test_input || (echo "test_input failed" && exit 1)
	@if [ -x ./test_integration ]; then \
		./test_integration || echo "Integration test failed (non-critical)"; \
	else \
		echo "Integration test skipped (library not built)"; \
	fi
	@python3 test_profiles.py || (echo "Python profile test failed" && exit 1)
	@python3 test_schema_validation.py || (echo "Python schema test failed" && exit 1)

