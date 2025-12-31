# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Tests/IGraphicsStressTest/config
IPLUG2_ROOT = ../../..

include ../../../common-wasm.mk

SRC += $(PROJECT_ROOT)/IGraphicsStressTest.cpp

# DSP module flags
WASM_DSP_CFLAGS +=

WASM_DSP_LDFLAGS += -O3 -s ASSERTIONS=0

# UI module flags
WASM_UI_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WASM_UI_LDFLAGS += -O3 -s ASSERTIONS=0

WASM_UI_LDFLAGS += $(NANOVG_LDFLAGS)
