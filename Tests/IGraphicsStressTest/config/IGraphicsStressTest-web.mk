include ./../../common-web.mk

SRC += IGraphicsStressTest.cpp

# WAM_SRC +=

# WAM_CFLAGS +=

#WEB_CFLAGS += -DIGRAPHICS_CANVAS
WEB_CFLAGS += -DIGRAPHICS_NANOVG -DIGRAPHICS_GLES2 

WAM_LDFLAGS += -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlug'" -O2 -s ASSERTIONS=0 

WEB_LDFLAGS += -O2 -s ASSERTIONS=0 

#if you want to use NANOVG/WebGL, you must link with -s USE_GLFW=3 -s USE_WEBGL2=0 -s FULL_ES3=1

WEB_LDFLAGS += -s USE_GLFW=3 -s USE_WEBGL2=0 -s FULL_ES3=1 