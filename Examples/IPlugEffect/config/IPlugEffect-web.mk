include ./../../common-web.mk

SRC += IPlugEffect.cpp

# WAM_SRC +=

# WAM_CFLAGS +=

WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2

WAM_LDFLAGS += -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlugEffect'" -O2 -s ASSERTIONS=0

WEB_LDFLAGS += -O2 -s ASSERTIONS=0

WEB_LDFLAGS += $(NANOVG_LDFLAGS)
