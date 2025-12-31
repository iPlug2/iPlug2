# common-web-wasm.mk
# Build configuration for iPlug2 wasm split DSP/UI builds
#
# This approach combines:
# - Two separate WASM modules (like WAM SDK approach)
# - Web component UI (like EmAudioWorklet approach)
# - Simple postMessage communication (no WAM SDK dependency)
#
# DSP module: Runs in AudioWorklet, embedded as BASE64 with SINGLE_FILE=1
# UI module: Runs on main thread with full IGraphics and web component support
#
# Requirements:
# - Server must send COOP/COEP headers for SharedArrayBuffer:
#   Cross-Origin-Opener-Policy: same-origin
#   Cross-Origin-Embedder-Policy: require-corp

PROJECT_ROOT = $(PWD)/..
DEPS_PATH = $(IPLUG2_ROOT)/Dependencies
WDL_PATH = $(IPLUG2_ROOT)/WDL
IPLUG_PATH = $(IPLUG2_ROOT)/IPlug
SWELL_PATH = $(WDL_PATH)/swell
IGRAPHICS_PATH = $(IPLUG2_ROOT)/IGraphics
CONTROLS_PATH = $(IGRAPHICS_PATH)/Controls
PLATFORMS_PATH = $(IGRAPHICS_PATH)/Platforms
DRAWING_PATH = $(IGRAPHICS_PATH)/Drawing
IGRAPHICS_EXTRAS_PATH = $(IGRAPHICS_PATH)/Extras
IPLUG_EXTRAS_PATH = $(IPLUG_PATH)/Extras
IPLUG_SYNTH_PATH = $(IPLUG_EXTRAS_PATH)/Synth
IPLUG_WEB_PATH = $(IPLUG_PATH)/WEB
NANOVG_PATH = $(DEPS_PATH)/IGraphics/NanoVG/src
NANOSVG_PATH = $(DEPS_PATH)/IGraphics/NanoSVG/src
YOGA_PATH = $(DEPS_PATH)/IGraphics/yoga
STB_PATH = $(DEPS_PATH)/IGraphics/STB

# Core IPlug source files (shared by both modules)
IPLUG_SRC = $(IPLUG_PATH)/IPlugAPIBase.cpp \
	$(IPLUG_PATH)/IPlugParameter.cpp \
	$(IPLUG_PATH)/IPlugPluginBase.cpp \
	$(IPLUG_PATH)/IPlugPaths.cpp \
	$(IPLUG_PATH)/IPlugTimer.cpp

# IGraphics source files (UI module only)
IGRAPHICS_SRC = $(IGRAPHICS_PATH)/IGraphics.cpp \
	$(IGRAPHICS_PATH)/IControl.cpp \
	$(CONTROLS_PATH)/*.cpp \
	$(PLATFORMS_PATH)/IGraphicsWeb.cpp

# Include paths (no WAM SDK needed)
INCLUDE_PATHS = -I$(PROJECT_ROOT) \
-I$(WDL_PATH) \
-I$(SWELL_PATH) \
-I$(IPLUG_PATH) \
-I$(IPLUG_EXTRAS_PATH) \
-I$(IPLUG_WEB_PATH) \
-I$(IGRAPHICS_PATH) \
-I$(DRAWING_PATH) \
-I$(CONTROLS_PATH) \
-I$(PLATFORMS_PATH) \
-I$(IGRAPHICS_EXTRAS_PATH) \
-I$(NANOVG_PATH) \
-I$(NANOSVG_PATH) \
-I$(STB_PATH) \
-I$(YOGA_PATH) \
-I$(YOGA_PATH)/yoga

# Base source files for both modules
SRC = $(IPLUG_SRC)

# DSP module source files (runs in AudioWorklet)
WASM_DSP_SRC = $(IPLUG_WEB_PATH)/IPlugWasmDSP.cpp \
	$(IPLUG_PATH)/IPlugProcessor.cpp

# UI module source files (runs on main thread with IGraphics)
WASM_UI_SRC = $(IPLUG_WEB_PATH)/IPlugWasmUI.cpp \
	$(IGRAPHICS_SRC) \
	$(IGRAPHICS_PATH)/IGraphicsEditorDelegate.cpp

NANOVG_LDFLAGS = -s USE_WEBGL2=0 -s FULL_ES3=1

# Common CFLAGS for all builds
CFLAGS = $(INCLUDE_PATHS) \
-Wno-bitwise-op-parentheses \
-Wno-deprecated-declarations \
-DWDL_NO_DEFINE_MINMAX \
-DNDEBUG=1

# DSP module CFLAGS - AudioWorklet processor, no graphics
WASM_DSP_CFLAGS = -DWASM_DSP_API \
-DIPLUG_DSP=1 \
-DNO_IGRAPHICS \
-DSAMPLE_TYPE_FLOAT

# UI module CFLAGS - IGraphics and editor
WASM_UI_CFLAGS = -DWASM_UI_API \
-DIPLUG_EDITOR=1

# DSP module exports (minimal - JS calls via emscripten bindings)
WASM_DSP_EXPORTS = "['_malloc', '_free']"

# UI module exports
WASM_UI_EXPORTS = "['_malloc', '_free', '_main', '_iplug_fsready', '_iplug_syncfs']"

# Common linker flags
LDFLAGS = -s ALLOW_MEMORY_GROWTH=1 --bind

# DSP module linker flags
# CRITICAL: SINGLE_FILE=1 embeds WASM as BASE64 for synchronous loading in AudioWorklet
# CRITICAL: BINARYEN_ASYNC_COMPILATION=0 for synchronous compilation in AudioWorklet scope
# NOTE: ENVIRONMENT=web,worker needed for AudioWorklet (debug mode is strict about environment checks)
WASM_DSP_LDFLAGS = -s EXPORTED_FUNCTIONS=$(WASM_DSP_EXPORTS) \
-s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setValue', 'UTF8ToString', 'HEAPF32', 'HEAPU8']" \
-s BINARYEN_ASYNC_COMPILATION=0 \
-s SINGLE_FILE=1 \
-s ENVIRONMENT=web,worker,shell \
-msimd128 \
--pre-js=$(IPLUG2_ROOT)/IPlug/WEB/Template/scripts/atob-polyfill.js

# UI module linker flags (standard async loading)
WASM_UI_LDFLAGS = -s EXPORTED_FUNCTIONS=$(WASM_UI_EXPORTS) \
-s EXPORTED_RUNTIME_METHODS="['ccall', 'UTF8ToString', 'HEAPU8']" \
-s BINARYEN_ASYNC_COMPILATION=1 \
-s FORCE_FILESYSTEM=1 \
-s ENVIRONMENT=web \
-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE="['\$$Browser']" \
-lidbfs.js
