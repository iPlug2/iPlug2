# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Tests/IGraphicsTest/config
IPLUG2_ROOT = ../../..

include ../../../common-wasm.mk

SRC += $(PROJECT_ROOT)/IGraphicsTest.cpp

# DSP module flags
WASM_DSP_CFLAGS +=

WASM_DSP_LDFLAGS += -O3 -s ASSERTIONS=0

# UI module flags - add FlexBox (unity build includes yoga sources, which need C++20)
WASM_UI_SRC += $(IPLUG2_ROOT)/IGraphics/Extras/IGraphicsFlexBox.cpp
WASM_UI_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2 -std=c++20
WASM_UI_CFLAGS += -I$(IPLUG2_ROOT)/IGraphics/Extras

WASM_UI_LDFLAGS += -O3 -s ASSERTIONS=0

WASM_UI_LDFLAGS += $(NANOVG_LDFLAGS)
