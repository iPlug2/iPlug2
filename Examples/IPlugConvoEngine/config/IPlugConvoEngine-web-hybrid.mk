# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugConvoEngine/config
IPLUG2_ROOT = ../../..

include ../../../common-web-hybrid.mk

SRC += $(PROJECT_ROOT)/IPlugConvoEngine.cpp

# r8brain resampler
SRC += $(PROJECT_ROOT)/r8brain/r8bbase.cpp
SRC += $(PROJECT_ROOT)/r8brain/pffft.cpp

# WDL FFT and Convolution Engine
HYBRID_DSP_SRC += $(WDL_PATH)/fft.c
HYBRID_DSP_SRC += $(WDL_PATH)/convoengine.cpp

# DSP module flags
HYBRID_DSP_CFLAGS += -I$(PROJECT_ROOT)/r8brain -DUSE_R8BRAIN

HYBRID_DSP_LDFLAGS += -O3 -s ASSERTIONS=0

# UI module flags (headless - no IGraphics)
# For headless plugins, we still need to build a minimal UI module
# that provides the web component shell
HYBRID_UI_CFLAGS +=

HYBRID_UI_LDFLAGS += -O3 -s ASSERTIONS=0
