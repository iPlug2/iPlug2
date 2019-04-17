# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is two directories up from /Examples/IPlugEffect
IPLUG2_ROOT = ./../../
include ./../../common-web.mk

SRC += IPlugInstrument.cpp

WAM_SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp

# WAM_CFLAGS +=

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlugInstrument'" -O2 -s ASSERTIONS=0

WEB_LDFLAGS += -O2 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
