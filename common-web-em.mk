# common-web-em.mk
# Build configuration for iPlug2 using Emscripten's native AudioWorklet support
# This is a simplified alternative to the WAM SDK-based build (common-web.mk)
#
# Requirements:
# - Emscripten SDK with AudioWorklet support
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

# Core IPlug source files
IPLUG_SRC = $(IPLUG_PATH)/IPlugAPIBase.cpp \
	$(IPLUG_PATH)/IPlugParameter.cpp \
	$(IPLUG_PATH)/IPlugPluginBase.cpp \
	$(IPLUG_PATH)/IPlugPaths.cpp \
	$(IPLUG_PATH)/IPlugTimer.cpp

# IGraphics source files
IGRAPHICS_SRC = $(IGRAPHICS_PATH)/IGraphics.cpp \
	$(IGRAPHICS_PATH)/IControl.cpp \
	$(CONTROLS_PATH)/*.cpp \
	$(PLATFORMS_PATH)/IGraphicsWeb.cpp

# Include paths
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

# Base source files for all builds
SRC = $(IPLUG_SRC)

# Source files for Emscripten AudioWorklet build (unified - both DSP and UI in one module)
# Note: IPlugWeb.cpp is NOT included as it's for the separate WEB_API build
EM_SRC = $(IPLUG_WEB_PATH)/IPlugEmAudioWorklet.cpp \
	$(IPLUG_PATH)/IPlugProcessor.cpp \
	$(IGRAPHICS_SRC) \
	$(IGRAPHICS_PATH)/IGraphicsEditorDelegate.cpp

NANOVG_LDFLAGS = -s USE_WEBGL2=0 -s FULL_ES3=1

# Common CFLAGS for all builds
CFLAGS = $(INCLUDE_PATHS) \
-Wno-bitwise-op-parentheses \
-Wno-deprecated-declarations \
-DWDL_NO_DEFINE_MINMAX \
-DNDEBUG=1

# Emscripten AudioWorklet specific flags
EM_CFLAGS = -DEM_AUDIOWORKLET_API \
-DIPLUG_DSP=1 \
-DIPLUG_EDITOR=1 \
-DSAMPLE_TYPE_FLOAT

# Functions exported to JavaScript
EM_EXPORTS = "['_malloc', '_free', '_main', '_iplug_fsready', '_iplug_syncfs', '_initAudioWorklet']"

# Common linker flags
LDFLAGS = -s ALLOW_MEMORY_GROWTH=1 --bind

# Emscripten AudioWorklet specific linker flags
# Note: AUDIO_WORKLET requires WASM_WORKERS and SharedArrayBuffer support
EM_LDFLAGS = -s EXPORTED_FUNCTIONS=$(EM_EXPORTS) \
-s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setValue', 'UTF8ToString']" \
-s AUDIO_WORKLET=1 \
-s WASM_WORKERS=1 \
-s BINARYEN_ASYNC_COMPILATION=1 \
-s FORCE_FILESYSTEM=1 \
-s ENVIRONMENT=web,worker \
-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE="['\$$Browser']" \
-lidbfs.js \
-pthread \
-s PTHREAD_POOL_SIZE=1
