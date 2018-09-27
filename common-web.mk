ROOT = ../..
PROJECT_ROOT = .
DEPS_PATH = $(ROOT)/Dependencies
WAM_SDK_PATH = $(DEPS_PATH)/IPlug/WAM_SDK/wamsdk
WDL_PATH = $(ROOT)/WDL
IPLUG_PATH = $(ROOT)/IPlug
IGRAPHICS_PATH = $(ROOT)/IGraphics
CONTROLS_PATH = $(IGRAPHICS_PATH)/Controls
PLATFORMS_PATH = $(IGRAPHICS_PATH)/Platforms
DRAWING_PATH = $(IGRAPHICS_PATH)/Drawing
IPLUG_EXTRAS_PATH = $(IPLUG_PATH)/Extras
IPLUG_WEB_PATH = $(IPLUG_PATH)/WEB
NANOVG_PATH = $(DEPS_PATH)/IGraphics/NanoVG/src
NANOSVG_PATH = $(DEPS_PATH)/IGraphics/NanoSVG/src

IPLUG_SRC = $(IPLUG_PATH)/IPlugAPIBase.cpp \
	$(IPLUG_PATH)/IPlugParameter.cpp \
	$(IPLUG_PATH)/IPlugPluginBase.cpp \
	$(IPLUG_PATH)/IPlugPaths.cpp
	# $(IPLUG_PATH)/IPlugTimer.cpp

IGRAPHICS_SRC = $(IGRAPHICS_PATH)/IGraphics.cpp \
	$(IGRAPHICS_PATH)/IControl.cpp \
	$(CONTROLS_PATH)/*.cpp \
	$(PLATFORMS_PATH)/IGraphicsWeb.cpp

INCLUDE_PATHS = -I$(PROJECT_ROOT) \
-I$(WAM_SDK_PATH) \
-I$(WDL_PATH) \
-I$(IPLUG_PATH) \
-I$(IPLUG_EXTRAS_PATH) \
-I$(IPLUG_WEB_PATH) \
-I$(IGRAPHICS_PATH) \
-I$(DRAWING_PATH) \
-I$(CONTROLS_PATH) \
-I$(PLATFORMS_PATH) \
-I$(NANOVG_PATH) \
-I$(NANOSVG_PATH)

#every cpp file that is needed for both WASM modules
SRC = $(IPLUG_SRC)

#every cpp file that is needed for the WAM audio processor WASM module running in the audio worklet
WAM_SRC = $(IPLUG_WEB_PATH)/IPlugWAM.cpp \
$(WAM_SDK_PATH)/processor.cpp

#every cpp file that is needed for the "WEB" graphics WASM module
WEB_SRC = $(IGRAPHICS_SRC) \
$(IPLUG_WEB_PATH)/IPlugWeb.cpp \
$(IGRAPHICS_PATH)/IGraphicsEditorDelegate.cpp

CFLAGS = $(INCLUDE_PATHS) \
-std=c++11  \
-Wno-bitwise-op-parentheses \
-DNO_PARAMS_MUTEX

WAM_CFLAGS = -DWAM_API \
-DIPLUG_DSP=1 \
-DNO_IGRAPHICS \
-DSAMPLE_TYPE_FLOAT

WEB_CFLAGS = -DWEB_API \
-DIPLUG_EDITOR=1

WAM_EXPORTS = "[\
  '_createModule','_wam_init','_wam_terminate','_wam_resize', \
  '_wam_onprocess', '_wam_onmidi', '_wam_onsysex', '_wam_onparam', \
  '_wam_onmessageN', '_wam_onmessageS', '_wam_onmessageA', '_wam_onpatch' \
  ]"

WEB_EXPORTS = "['_main']"

LDFLAGS = -O2 \
-s ASSERTIONS=0 \
-s ALLOW_MEMORY_GROWTH=1 \
--bind

WAM_LDFLAGS = -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setValue', 'Pointer_stringify']" \
-s BINARYEN_ASYNC_COMPILATION=0 \
-s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlug'"

WEB_LDFLAGS = -s EXPORTED_FUNCTIONS=$(WEB_EXPORTS) \
-s EXTRA_EXPORTED_RUNTIME_METHODS="['Pointer_stringify']" \
-s BINARYEN_ASYNC_COMPILATION=1 \
-s FORCE_FILESYSTEM=1
