# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugConvoEngine/config
IPLUG2_ROOT = ../../..

include ../../../common-wasm.mk

SRC += $(PROJECT_ROOT)/IPlugConvoEngine.cpp

# r8brain resampler
SRC += $(PROJECT_ROOT)/r8brain/r8bbase.cpp
SRC += $(PROJECT_ROOT)/r8brain/pffft.cpp

# WDL FFT and Convolution Engine
WASM_DSP_SRC += $(WDL_PATH)/fft.c
WASM_DSP_SRC += $(WDL_PATH)/convoengine.cpp

# DSP module flags
WASM_DSP_CFLAGS += -I$(PROJECT_ROOT)/r8brain -DUSE_R8BRAIN

# Use DEBUG=1 for DWARF debugging: emmake make -f IPlugConvoEngine-hybrid-dsp.mk DEBUG=1
# Note: DSP module runs in AudioWorklet with synchronous WASM loading constraints
# Don't use SAFE_HEAP as it breaks Module._malloc in AudioWorklet scope
ifeq ($(DEBUG),1)
  WASM_DSP_CFLAGS += -g3 -O1
  WASM_DSP_LDFLAGS += -g3 -O1 -s ASSERTIONS=2 -s STACK_OVERFLOW_CHECK=2
else
  WASM_DSP_LDFLAGS += -O3 -s ASSERTIONS=0
endif

# UI module flags (headless - no IGraphics)
# For headless plugins, we still need to build a minimal UI module
# that provides the web component shell
WASM_UI_CFLAGS +=

ifeq ($(DEBUG),1)
  WASM_UI_CFLAGS += -g3 -O1
  WASM_UI_LDFLAGS += -g3 -O1 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2
else
  WASM_UI_LDFLAGS += -O3 -s ASSERTIONS=0
endif
