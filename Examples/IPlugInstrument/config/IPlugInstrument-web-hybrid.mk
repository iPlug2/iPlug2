# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugInstrument/config
IPLUG2_ROOT = ../../..

include ../../../common-web-hybrid.mk

SRC += $(PROJECT_ROOT)/IPlugInstrument.cpp

INCLUDE_PATHS += -I$(IPLUG_SYNTH_PATH)

# DSP module flags
HYBRID_DSP_CFLAGS += -I$(IPLUG_SYNTH_PATH)

HYBRID_DSP_LDFLAGS += -O3 -s ASSERTIONS=0

# UI module flags
HYBRID_UI_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

HYBRID_UI_LDFLAGS += -O3 -s ASSERTIONS=0

HYBRID_UI_LDFLAGS += $(NANOVG_LDFLAGS)
