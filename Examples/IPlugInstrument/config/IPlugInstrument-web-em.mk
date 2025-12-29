# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugInstrument/config
IPLUG2_ROOT = ../../..

include ../../../common-web-em.mk

SRC += $(PROJECT_ROOT)/IPlugInstrument.cpp

# Synth extras
SRC += $(IPLUG_EXTRAS_PATH)/Synth/*.cpp

EM_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2
EM_CFLAGS += -I$(IPLUG_SYNTH_PATH)

EM_LDFLAGS += -O3 -s ASSERTIONS=0

EM_LDFLAGS += $(NANOVG_LDFLAGS)
