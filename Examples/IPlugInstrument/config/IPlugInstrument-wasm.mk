# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugInstrument/config
IPLUG2_ROOT = ../../..

include ../../../common-wasm.mk

SRC += $(PROJECT_ROOT)/IPlugInstrument.cpp
WASM_DSP_SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp

INCLUDE_PATHS += -I$(IPLUG_SYNTH_PATH)

# DSP module flags
WASM_DSP_CFLAGS += -I$(IPLUG_SYNTH_PATH)

WASM_DSP_LDFLAGS += -O3 -s ASSERTIONS=0

# UI module flags
WASM_UI_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WASM_UI_LDFLAGS += -O3 -s ASSERTIONS=0

WASM_UI_LDFLAGS += $(NANOVG_LDFLAGS)
