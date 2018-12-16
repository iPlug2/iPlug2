##LINUX ONLY FOR NOW

APP=IPlugControls

WDL_PATH = ../../WDL
vpath %.cpp .. $(WDL_PATH)/swell
vpath %.cpp .. $(WDL_PATH)/lice
vpath %.cpp .. app_wrapper

### Objects and resources
SWELL_OBJS = swell.o swell-ini.o swell-miscdlg-generic.o swell-wnd-generic.o \
             swell-menu-generic.o swell-kb-generic.o swell-dlg-generic.o \
             swell-gdi-generic.o swell-misc-generic.o swell-gdi-lice.o \
             swell-generic-headless.o swell-generic-gdk.o \
             swell-appstub-generic.o swell-modstub-generic.o

LICE_OBJS = lice.o lice_arc.o lice_colorspace.o lice_line.o lice_text.o \
            lice_textnew.o lice_png.o

OBJS = app_main.o main_dialog.o $(SWELL_OBJS) $(LICE_OBJS)
RESFILES = resources/IPlugControls.rc_mac_dlg resources/IPlugControls.rc_mac_menu

### Compiler/Linker Flags
CFLAGS += -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wno-unused-function -Wno-multichar -Wno-unused-result -Werror -Wno-deprecated-declarations
CFLAGS += -O0 -g -D_DEBUG
LFLAGS = -lpthread -ldl -lX11 -lXi
### GDK
CFLAGS += -DSWELL_TARGET_GDK=2 $(shell pkg-config gdk-2.0 --cflags)
LFLAGS += $(shell pkg-config gdk-2.0 --libs)
### Freetype
CFLAGS += -DSWELL_LICE_GDI -DSWELL_FREETYPE $(shell freetype-config --cflags)
LFLAGS += $(shell freetype-config --libs)

CXXFLAGS = $(CFLAGS)


### Targets
.PHONY: clean run
default: $(APP)

$(RESFILES): resources/IPlugControls.rc
	php $(WDL_PATH)swell/mac_resgen.php $^

$(APP): $(RESFILES) $(OBJS)
	$(CXX) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS)


run: $(APP)
	./$<
clean:
	-rm $(APP) $(OBJS) $(RESFILES)
