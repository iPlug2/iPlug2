include ./../../common-linux.mk

ARCH := $(shell uname -m)
UNAME_S := $(shell uname -s)

vpath %.cpp .. $(IPLUG_PATH)
vpath %.cpp .. $(SWELL_PATH)/swell
vpath %.cpp .. $(LICE_PATH)/lice
vpath %.cpp .. $(IPLUG_PATH)/APP
vpath %.cpp .. $(PROJECT_ROOT)

APP_OBJS = IPlugAPP.o IPlugAPP_main.o IPlugAPP_dialog.o IPlugAPP_host.o

PROJECT_OBJS = IPlugMinimal.o

OBJS = $(APP_OBJS) $(PROJECT_OBJS)
RESFILES = resources/main.rc_mac_dlg resources/main.rc_mac_menu

### Compiler/Linker Flags
CFLAGS += $(IPLUG_INC_PATHS) $(IGRAPHICS_INC_PATHS) -I$(RTAUDIO_PATH) -I$(RTMIDI_PATH) -I$(PROJECT_ROOT)/resources -I$(IPLUG_PATH)/APP
CFLAGS += -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wno-unused-function -Wno-multichar -Wno-unused-result -Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-reorder -Wno-shadow -Wno-sign-compare
CFLAGS += -O0 -g -D_DEBUG
LFLAGS = -lpthread -ldl -lX11 -lXi
### GDK
CFLAGS += -DSWELL_TARGET_GDK=2 $(shell pkg-config gdk-2.0 --cflags)
LFLAGS += $(shell pkg-config gdk-2.0 --libs)
### Freetype
CFLAGS += -DSWELL_LICE_GDI -DSWELL_FREETYPE $(shell freetype-config --cflags)
LFLAGS += $(shell freetype-config --libs)

CXXFLAGS = $(CFLAGS) -std=c++14 -DNOMINMAX -DNO_IGRAPHICS -DAPP_API -DIPLUG_DSP -DIPLUG_EDITOR

### Targets
.PHONY: clean run
default: app

$(RESFILES): resources/main.rc
	php $(SWELL_PATH)/mac_resgen.php $^

app: $(RESFILES) $(OBJS)
	$(CXX) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS)


run: app
	./$<
clean:
	-rm app $(OBJS) $(RESFILES)
