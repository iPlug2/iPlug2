# IPlugSvelteUI - Emscripten AudioWorklet build makefile
PROJECT_NAME = IPlugSvelteUI

# Build output directory
BUILD_DIR = ../build-web-em

# Target output
TARGET = $(BUILD_DIR)/scripts/$(PROJECT_NAME)-em.js

# Include the web-em configuration
include ../config/$(PROJECT_NAME)-web-em.mk

# Combine all source files
ALL_SRC = $(SRC) $(EM_SRC_HEADLESS)

# Combine all flags
ALL_CFLAGS = $(CFLAGS) $(EM_CFLAGS)
ALL_LDFLAGS = $(LDFLAGS) $(EM_LDFLAGS)

# Default target
all: $(TARGET)

$(TARGET): $(ALL_SRC)
	@mkdir -p $(BUILD_DIR)/scripts
	@mkdir -p $(BUILD_DIR)/styles
	emcc $(ALL_SRC) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
