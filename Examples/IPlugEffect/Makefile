include ./../../common-linux.mk

ARCH := $(shell uname -m)
UNAME_S := $(shell uname -s)

APP=IPlugEffect

vpath %.cpp .. $(SWELL_PATH)/swell
vpath %.cpp .. $(LICE_PATH)/lice
vpath %.cpp .. $(IPLUG_PATH)/APP
vpath %.cpp .. $(PROJECT_ROOT)

### Objects and resources
SWELL_OBJS = swell.o swell-ini.o swell-miscdlg-generic.o swell-wnd-generic.o \
             swell-menu-generic.o swell-kb-generic.o swell-dlg-generic.o \
             swell-gdi-generic.o swell-misc-generic.o swell-gdi-lice.o \
             swell-generic-headless.o swell-generic-gdk.o \
             swell-appstub-generic.o swell-modstub-generic.o

LICE_OBJS = lice.o lice_arc.o lice_colorspace.o lice_line.o lice_text.o \
            lice_textnew.o lice_png.o

APP_OBJS = IPlugAPP.o IPlugAPP_main.o IPlugAPP_dialog.o IPlugAPP_host.o

PROJECT_OBJS = IPlugEffect.o

OBJS = $(APP_OBJS) $(SWELL_OBJS) $(LICE_OBJS) $(PROJECT_OBJS)
RESFILES = resources/main.rc_mac_dlg resources/main.rc_mac_menu

### Compiler/Linker Flags
CFLAGS += $(INCLUDE_PATHS) -I$(RTAUDIO_PATH) -I$(RTMIDI_PATH) -I$(IGRAPHICS_PATH) -I$(PROJECT_ROOT)/resources
CFLAGS += -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wno-unused-function -Wno-multichar -Wno-unused-result -Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-reorder -Wno-shadow -Wno-sign-compare
CFLAGS += -O0 -g -D_DEBUG
LFLAGS = -lpthread -ldl -lX11 -lXi
### GDK
CFLAGS += -DSWELL_TARGET_GDK=2 $(shell pkg-config gdk-2.0 --cflags)
LFLAGS += $(shell pkg-config gdk-2.0 --libs)
### Freetype
CFLAGS += -DSWELL_LICE_GDI -DSWELL_FREETYPE $(shell freetype-config --cflags)
LFLAGS += $(shell freetype-config --libs)

CXXFLAGS = $(CFLAGS) -std=c++11 -DNOMINMAX

### Targets
.PHONY: clean run
default: $(APP)

$(RESFILES): resources/main.rc
	php $(SWELL_PATH)/mac_resgen.php $^

$(APP): $(RESFILES) $(OBJS)
	$(CXX) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS)


run: $(APP)
	./$<
clean:
	-rm $(APP) $(OBJS) $(RESFILES)
