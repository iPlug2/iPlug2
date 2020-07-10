# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/Testing/config
IPLUG2_ROOT = ../../..
include ../../../common-web.mk

SRC += $(PROJECT_ROOT)/Testing.cpp

WAM_SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp

WAM_CFLAGS +=  -I$(IPLUG_SYNTH_PATH)

#WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2
WEB_CFLAGS += -DIGRAPHICS_CANVAS

WAM_LDFLAGS += -g4 -O0 -DDEBUG -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.Testing'" -s ASSERTIONS=0

WEB_LDFLAGS += -g4 -O0 -DDEBUG -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
