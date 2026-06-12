# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugP5js/config
IPLUG2_ROOT = ../../..

include ../../../common-wasm.mk

SRC += $(PROJECT_ROOT)/IPlugP5js.cpp

# DSP module flags
WASM_DSP_CFLAGS +=

WASM_DSP_LDFLAGS += -O3 -s ASSERTIONS=0

# WebView UI module flags
# Redirect the generic UI vars to the WebView variants so the shared
# -wasm-ui.mk rules build the WebView module (no IGraphics).
WASM_UI_SRC = $(WASM_WEBVIEW_UI_SRC)
WASM_UI_EXPORTS = $(WASM_WEBVIEW_UI_EXPORTS)

WASM_UI_CFLAGS += $(WEBVIEW_CFLAGS)

WASM_UI_LDFLAGS += -O3 -s ASSERTIONS=0
