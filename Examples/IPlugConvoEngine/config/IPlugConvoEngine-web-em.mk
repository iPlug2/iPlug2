# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugConvoEngine/config
IPLUG2_ROOT = ../../..

include ../../../common-web-em.mk

SRC += $(PROJECT_ROOT)/IPlugConvoEngine.cpp

# WDL convolution engine and FFT sources
SRC += $(WDL_PATH)/convoengine.cpp
SRC += $(WDL_PATH)/fft.c

# Headless build - no IGraphics
EM_CFLAGS_HEADLESS += -I$(PROJECT_ROOT)/r8brain

# Use DEBUG=1 for DWARF debugging: emmake make -f IPlugConvoEngine-em.mk DEBUG=1
ifeq ($(DEBUG),1)
  EM_CFLAGS_HEADLESS += -g3 -O1
  EM_LDFLAGS_HEADLESS += -g3 -O1 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2
else
  EM_LDFLAGS_HEADLESS += -O3 -s ASSERTIONS=0
endif
