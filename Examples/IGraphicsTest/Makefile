
ROOT = ../..
WDL_PATH = $(ROOT)/WDL
IPLUG_PATH = $(ROOT)/IPlug
IGRAPHICS_PATH = $(IPLUG_PATH)/IGraphics
TARGET = ./build-web/IGraphicsTest.html

IGRAPHICS_SRC = $(IGRAPHICS_PATH)/IControl.cpp \
	$(IGRAPHICS_PATH)/IControls.cpp \
	$(IGRAPHICS_PATH)/IGraphics.cpp \
	$(IGRAPHICS_PATH)/IGraphicsWeb.cpp

IPLUG_SRC = $(IPLUG_PATH)/IPlugParameter.cpp $(IPLUG_PATH)/IPlugDelegate.cpp $(IPLUG_PATH)/IPlugGraphicsDelegate.cpp

THIS_SRC = main_web.cpp IGraphicsTest.cpp

SRC = $(IGRAPHICS_SRC) $(THIS_SRC) $(IPLUG_SRC)

EXPORTS = "['_main']"

CFLAGS = -I./ -I$(IPLUG_PATH) -I$(WDL_PATH) -I$(IGRAPHICS_PATH) -std=c++11 -DIGRAPHICS_WEB -Wno-bitwise-op-parentheses
LDFLAGS = -s EXPORTED_FUNCTIONS=$(EXPORTS) -O2 --emrun
JSFLAGS = -s BINARYEN_ASYNC_COMPILATION=1 -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 --bind -s ASSERTIONS=0
#JSFLAGS =

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(JSFLAGS) -o $@ $(SRC)
