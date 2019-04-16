// IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
// By default, that is two directories up from /Examples/IPlugEffect
IPLUG2_ROOT = ./../../
include ./../../common-web.mk

SRC += IPlugControls.cpp

# WAM_SRC +=

# WAM_CFLAGS +=

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlugControls'" -O2 -s ASSERTIONS=0

WEB_LDFLAGS += -O2 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
