ROOT = ../..
PROJECT_ROOT = .
WAM_SDK_PATH = $(ROOT)/Dependencies/IPlug/WAM_SDK/wamsdk
WDL_PATH = $(ROOT)/WDL
IPLUG_PATH = $(ROOT)/IPlug
IGRAPHICS_PATH = $(ROOT)/IGraphics
CONTROLS_PATH = $(IGRAPHICS_PATH)/Controls
PLATFORMS_PATH = $(IGRAPHICS_PATH)/Platforms
IPLUG_EXTRAS_PATH = $(IPLUG_PATH)/Extras
IPLUG_WEB_PATH = $(IPLUG_PATH)/WEB

IPLUG_SRC = $(IPLUG_PATH)/IPlugAPIBase.cpp \
	$(IPLUG_PATH)/IPlugParameter.cpp \
	$(IPLUG_PATH)/IPlugPluginBase.cpp \
	$(IPLUG_PATH)/IPlugPaths.cpp \
	$(IPLUG_EXTRAS_PATH)/MidiSynth.cpp \
	# $(IPLUG_PATH)/IPlugTimer.cpp

IGRAPHICS_SRC = $(IGRAPHICS_PATH)/IGraphics.cpp \
	$(IGRAPHICS_PATH)/IControl.cpp \
	$(CONTROLS_PATH)/IControls.cpp \
	$(PLATFORMS_PATH)/IGraphicsWeb.cpp

INCLUDE_FLAGS = -I$(PROJECT_ROOT) \
-I$(WAM_SDK_PATH) \
-I$(WDL_PATH) \
-I$(IPLUG_PATH) \
-I$(IPLUG_EXTRAS_PATH) \
-I$(IPLUG_WEB_PATH) \
-I$(IGRAPHICS_PATH) \
-I$(CONTROLS_PATH) \
-I$(PLATFORMS_PATH)

#every cpp file that is needed for the WAM audio processor WASM module running in the audio worklet
WAM_SRC = IPlugEffect.cpp \
$(IPLUG_SRC) \
$(IPLUG_WEB_PATH)/IPlugWAM.cpp \
$(WAM_SDK_PATH)/processor.cpp

WAM_CFLAGS = $(INCLUDE_FLAGS) \
-DWAM_API \
-DNO_IGRAPHICS \
-DSAMPLE_TYPE_FLOAT \
-DNO_PARAMS_MUTEX \
-DIPLUG_DSP=1 \
-std=c++11  \
-Wno-bitwise-op-parentheses

WAM_LDFLAGS = -O2

#every cpp file that is needed for the graphics WASM module
WEB_SRC = IPlugEffect.cpp \
$(IPLUG_SRC) \
$(IGRAPHICS_SRC) \
$(IPLUG_WEB_PATH)/IPlugWeb.cpp \
$(IGRAPHICS_PATH)/IGraphicsEditorDelegate.cpp

WEB_CFLAGS = $(INCLUDE_FLAGS) \
-DWEB_API \
-DSAMPLE_TYPE_FLOAT \
-DIGRAPHICS_WEB \
-DNO_PARAMS_MUTEX \
-DIPLUG_EDITOR=1 \
-std=c++11 \
-Wno-bitwise-op-parentheses

WEB_LDFLAGS = -O2
