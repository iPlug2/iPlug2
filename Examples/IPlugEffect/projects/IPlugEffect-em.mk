# IPlugEffect-em.mk
# Makefile for building IPlugEffect with Emscripten native AudioWorklet support
# This creates a single unified WASM module with both DSP and UI

include ../config/IPlugEffect-web-em.mk

TARGET = ../build-web-em/scripts/IPlugEffect-em.js

SRC += $(EM_SRC)
CFLAGS += $(EM_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EM_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(EM_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
