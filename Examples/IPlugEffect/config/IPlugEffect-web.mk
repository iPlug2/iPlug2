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

IPLUG_SRC = $(IPLUG_PATH)/IPlugBase.cpp \
	$(IPLUG_PATH)/IPlugParameter.cpp \
	$(IPLUG_PATH)/IPlugPluginDelegate.cpp \
	# $(IPLUG_PATH)/IPlugTimer.cpp

IGRAPHICS_SRC = $(IGRAPHICS_PATH)/IGraphics.cpp \
	$(IGRAPHICS_PATH)/IControl.cpp \
	$(CONTROLS_PATH)/IControls.cpp \
	$(PLATFORMS_PATH)/IGraphicsWeb.cpp

#every cpp file that is needed for the WAM audio processor WASM module running in the audio worklet
WAM_SRC = $(IPLUG_SRC) $(WAM_SDK_PATH)/processor.cpp $(IPLUG_WEB_PATH)/IPlugWAM.cpp

#every cpp file that is needed for the graphics WASM module
WEB_SRC = $(IPLUG_SRC) $(IGRAPHICS_SRC) $(IPLUG_WEB_PATH)/IPlugWeb.cpp $(IPLUG_PATH)/IPlugGraphicsDelegate.cpp
