# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is two directories up from /Examples/IPlugEffect
IPLUG2_ROOT = ./../../
include ./../../common-web.mk

SRC += IPlugFaustDSP.cpp

# WAM_SRC +=

WAM_CFLAGS += -DFAUST_COMPILED -I$(ROOT)/Dependencies/Build/mac/include

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2 -DFAUST_COMPILED

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlugFaustDSP'" -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
