# IPLUG2_ROOT should point to the top level IPLUG2 folder from the project folder
# By default, that is three directories up from /Examples/IPlugEffect/config
IPLUG2_ROOT = ../../..

include ../../../common-web-em.mk

SRC += $(PROJECT_ROOT)/IPlugEffect.cpp

EM_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

# Use DEBUG=1 for DWARF debugging: emmake make -f IPlugEffect-em.mk DEBUG=1
ifeq ($(DEBUG),1)
  EM_CFLAGS += -g3 -O1
  EM_LDFLAGS += -g3 -O1 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2
else
  EM_LDFLAGS += -O3 -s ASSERTIONS=0
endif

EM_LDFLAGS += $(NANOVG_LDFLAGS)
