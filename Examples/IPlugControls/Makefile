# should be set prior include
DEBUG = 1

# select drawing method
#IGRAPHICS=IGRAPHICS_LICE
#IGRAPHICS_TYPE=

IGRAPHICS=IGRAPHICS_NANOVG
IGRAPHICS_TYPE=IGRAPHICS_GL2
#IGRAPHICS_TYPE=IGRAPHICS_GL3
#IGRAPHICS_TYPE=IGRAPHICS_GLES2

include ./../../common-linux.mk

PROJECT_SRC = $(wildcard *.cpp)

#in case if IGRAPHICS_LICE, LICE_SWELL_SRC should be included, otherwise LICE_SRC
# The problem is IGraphics_Lice source many LICE sources, but not everything required for SWELL on Linux

ALL_SRC  = $(IPLUG_SRC) $(APP_SRC) $(IGRAPHICS_SRC) $(SWELL_SRC) $(PROJECT_SRC)
ifeq ($(IGRAPHICS), IGRAPHICS_LICE)
    ALL_SRC += $(LICE_SWELL_SRC)
else
    ALL_SRC += $(LICE_SRC)
endif

vpath %.cpp $(sort $(dir $(ALL_SRC)))
ABS_OBJS = $(ALL_SRC:.cpp=.o)
OBJS = $(notdir $(ABS_OBJS))


### Compiler/Linker Flags
CFLAGS += $(IPLUG_INC_PATHS) $(IGRAPHICS_INC_PATHS) -I$(RTAUDIO_PATH) -I$(RTMIDI_PATH) -I$(PROJECT_ROOT)/resources -I$(IPLUG_PATH)/APP -I$(STB_PATH)
CFLAGS += -Wno-unused-result -Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-reorder -Wno-shadow -Wno-sign-compare -Wno-unused-variable
# WDL needs that
CFLAGS += -Wno-strict-aliasing -Wno-strict-overflow -Wno-maybe-uninitialized -Wno-sequence-point
# stb_image.h needs that
CFLAGS += -Wno-misleading-indentation
#CFLAGS += -O0 -g -D_DEBUG
#LFLAGS = -lpthread -ldl -lX11 -lXi
### GDK
#CFLAGS += -DSWELL_TARGET_GDK=2 $(shell pkg-config gdk-2.0 --cflags)
#LFLAGS += $(shell pkg-config gdk-2.0 --libs)
### Freetype
#CFLAGS += -DSWELL_LICE_GDI -DSWELL_FREETYPE $(shell freetype-config --cflags)
#LFLAGS += $(shell freetype-config --libs)

CXXFLAGS = $(CFLAGS) -std=c++14 -DNOMINMAX -D$(IGRAPHICS) -D$(IGRAPHICS_TYPE) -DAPP_API -DIPLUG_DSP -DIPLUG_EDITOR

# Link some libriaries static
LINKEXTRA += -lpng

LINKEXTRA += -Wl,-Bstatic
LINKEXTRA += -L$(RTAUDIO_PATH)/.libs/ -lrtaudio
LINKEXTRA += -L$(RTMIDI_PATH)/.libs/ -lrtmidi
LINKEXTRA += -Wl,-Bdynamic -lpulse-simple -lpulse -lasound -ljack

ifeq ($(IGRAPHICS), IGRAPHICS_NANOVG)
    LINKEXTRA += -lGL -lGLEW
endif

LINKEXTRA += -Wl,--as-needed

### Targets
.PHONY: clean run
default: app

$(RESFILES): resources/main.rc
	php $(SWELL_PATH)/mac_resgen.php $^

IPlugAPP_main.o : $(RESFILES)


app: $(RESFILES) $(OBJS)
	$(CXX) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS) $(LINKEXTRA)


run: app
	./$<
clean:
	-rm -f app $(OBJS) $(RESFILES)
