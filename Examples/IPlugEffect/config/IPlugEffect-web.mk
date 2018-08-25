include ./../../common-web.mk

SRC += IPlugEffect.cpp

WAM_SRC += $(IPLUG_EXTRAS_PATH)/MidiSynth.cpp

# WAM_CFLAGS +=

WEB_CFLAGS += -DIGRAPHICS_CANVAS

# WAM_LDFLAGS +=

# WEB_LDFLAGS += -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1
