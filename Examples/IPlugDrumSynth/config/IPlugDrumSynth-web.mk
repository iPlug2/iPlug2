# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugDrumSynth/config
IPLUG2_ROOT = ../../..
include ../../../common-web.mk

SRC += $(PROJECT_ROOT)/IPlugDrumSynth.cpp

WAM_SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp

WAM_CFLAGS +=  -I$(IPLUG_SYNTH_PATH)

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlugDrumSynth'" -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
