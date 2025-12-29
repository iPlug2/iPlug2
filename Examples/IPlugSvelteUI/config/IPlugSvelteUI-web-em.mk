# IPlugSvelteUI - Web Emscripten AudioWorklet configuration
# This is a HEADLESS build (no IGraphics) - the UI is handled by Svelte
#
# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugSvelteUI/config
IPLUG2_ROOT = ../../..

include ../../../common-web-em.mk

SRC += $(PROJECT_ROOT)/IPlugSvelteUI.cpp

# Use headless (no IGraphics) source and flags
# The Svelte UI communicates via the JavaScript controller
EM_CFLAGS += $(EM_CFLAGS_HEADLESS)

# Include synth/DSP extras
INCLUDE_PATHS += -I$(IPLUG_SYNTH_PATH)
CFLAGS += $(INCLUDE_PATHS)

# Optimization and assertions
# Use DEBUG=1 for DWARF debugging: emmake make -f IPlugSvelteUI-em.mk DEBUG=1
ifeq ($(DEBUG),1)
  # Debug build: DWARF symbols, low optimization, max assertions
  EM_CFLAGS += -g3 -O1
  EM_LDFLAGS += -g3 -O1 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2
else
  # Release build
  EM_LDFLAGS += -O3 -s ASSERTIONS=0
endif

# Use headless linker flags (no WebGL)
EM_LDFLAGS += $(EM_LDFLAGS_HEADLESS)
