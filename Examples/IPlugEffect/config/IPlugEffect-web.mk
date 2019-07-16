include ./../../common-web.mk

SRC += IPlugEffect.cpp

# WAM_SRC +=

# WAM_CFLAGS +=

WEB_CFLAGS += -DIGRAPHICS_CANVAS# -DIGRAPHICS_GLES2

WAM_LDFLAGS += -O3 -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlugEffect'" -s ASSERTIONS=0

WEB_LDFLAGS += -O3 -s ASSERTIONS=0

#WEB_LDFLAGS += $(NANOVG_LDFLAGS)
