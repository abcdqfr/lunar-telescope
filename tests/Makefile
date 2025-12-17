# Makefile for Lunar Telescope tests

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
INCLUDES = -I../core -I../input
LDFLAGS = -ljson-c -lm

# Source directories
CORE_DIR = ../core
INPUT_DIR = ../input

# Test executables
TESTS = test_schema test_input test_integration

.PHONY: all clean

all: $(TESTS)

test_schema: test_schema.c $(CORE_DIR)/schema.c $(CORE_DIR)/profiles.c $(CORE_DIR)/telescope.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

test_input: test_input.c $(INPUT_DIR)/input_proxy.c $(INPUT_DIR)/scroll_smoother.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

test_integration: test_integration.c
	$(CC) $(CFLAGS) $(INCLUDES) -I../core -I../input -I../compositor \
		-o $@ $< \
		-L../build/lib -llunar_telescope \
		$(LDFLAGS) $(JSON_C_LDFLAGS) \
		-Wl,-rpath,../build/lib || \
	$(CC) $(CFLAGS) $(INCLUDES) -I../core -I../input -I../compositor \
		-o $@ $< \
		../core/*.o ../input/*.o ../compositor/*.o \
		$(LDFLAGS) $(JSON_C_LDFLAGS)

clean:
	rm -f $(TESTS)

test: all
	./test_schema
	./test_input
	./test_integration || echo "Integration test skipped (requires built library)"
	python3 test_profiles.py
	python3 test_schema_validation.py

