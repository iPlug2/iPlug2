# IPlugConvoEngine-em.mk
# Makefile for building IPlugConvoEngine with Emscripten native AudioWorklet support
# This is a headless build (no IGraphics)

include ../config/IPlugConvoEngine-web-em.mk

TARGET = ../build-web-em/scripts/IPlugConvoEngine-em.js

# Use headless source files (no IGraphics)
SRC += $(EM_SRC_HEADLESS)
CFLAGS += $(EM_CFLAGS_HEADLESS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EM_LDFLAGS_HEADLESS) \
-s EXPORTED_FUNCTIONS=$(EM_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
