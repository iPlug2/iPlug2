##LINUX ONLY FOR NOW

APP=IGraphicsTest

IPLUG_PATH = ../../IPlug
WDL_PATH = ../../WDL
SWELL_PATH = $(WDL_PATH)/swell
IGRAPHICS_PATH = $(IPLUG_PATH)/IGraphics
SWELL_PATH = $(WDL_PATH)/swell
LICE_PATH = $(WDL_PATH)/lice
NANOSVG_PATH = $(IGRAPHICS_PATH)/NanoSVG

IPLUG_INC_PATHS = -I$(IPLUG_PATH) -I$(WDL_PATH) -I$(SWELL_PATH)
IGRAPHICS_INC_PATHS = -I$(IGRAPHICS_PATH) -I$(NANOSVG_PATH)

ARCH := $(shell uname -m)
UNAME_S := $(shell uname -s)

PKG_CONFIG = pkg-config

CFLAGS = -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wno-unused-function -Wno-multichar -Wno-unused-result

ifeq ($(UNAME_S),Darwin)
  CFLAGS += -Wno-unused-private-field -DSWELL_FORCE_GENERIC
  DLL_EXT=.dylib
else
  DLL_EXT=.so
endif

ifndef ALLOW_WARNINGS
  CFLAGS += -Werror
endif
ifndef DEPRECATED_WARNINGS
  CFLAGS +=  -Wno-deprecated-declarations
endif

ifneq ($(filter arm%,$(ARCH)),)
  CFLAGS += -fsigned-char -marm
endif


ifdef DEBUG
CFLAGS += -O0 -g -D_DEBUG
else
CFLAGS += -O2 -DNDEBUG
  ifneq ($(UNAME_S),Darwin)
    CFLAGS += -s
  endif
endif

LINKEXTRA =  -lpthread -ldl 

vpath %.cpp .. $(LICE_PATH)
vpath %.cpp .. $(SWELL_PATH)


### Objects and resources
SWELL_OBJS = swell.o swell-ini.o swell-miscdlg-generic.o swell-wnd-generic.o \
             swell-menu-generic.o swell-kb-generic.o swell-dlg-generic.o \
             swell-misc-generic.o \
             swell-appstub-generic.o \
             swell-generic-headless.o \
             swell-generic-gdk.o \
             swell-gdi-generic.o 

ifndef NOLICE
SWELL_OBJS += swell-gdi-lice.o 
LICE_OBJS = lice.o lice_arc.o lice_colorspace.o lice_line.o lice_text.o \
            lice_textnew.o lice_png.o
endif

OBJS = main.o IGraphicsTest.o $(SWELL_OBJS)
RESFILES = resources/IGraphicsTest.rc_mac_dlg resources/IGraphicsTest.rc_mac_menu

### Compiler/Linker Flags

CFLAGS += $(IPLUG_INC_PATHS)
CFLAGS += $(IGRAPHICS_INC_PATHS)
CFLAGS += -DNOMINMAX
CFLAGS += -DNO_IGRAPHICS 
CFLAGS += -std=c++11

ifneq ($(filter arm%,$(ARCH)),)
  CFLAGS += -fsigned-char -marm
endif

ifndef NOGDK
  ifdef GDK2
    CFLAGS += -DSWELL_TARGET_GDK=2 $(shell $(PKG_CONFIG) --cflags gdk-2.0)
    ifndef PRELOAD_GDK
      LINKEXTRA += $(shell $(PKG_CONFIG) --libs gdk-2.0)
    else
      LINKEXTRA += -lX11 -lXi
      CFLAGS += -DSWELL_PRELOAD="libgdk-x11-2.0.so.0"
    endif
  else
    ifdef SWELL_SUPPORT_GTK
      CFLAGS += -DSWELL_TARGET_GDK=3 $(shell $(PKG_CONFIG) --cflags gtk+-3.0) -DSWELL_SUPPORT_GTK
    else
      CFLAGS += -DSWELL_TARGET_GDK=3 $(shell $(PKG_CONFIG) --cflags gdk-3.0)
    endif
    ifndef PRELOAD_GDK
      LINKEXTRA += -lX11 -lXi
      ifdef SWELL_SUPPORT_GTK
        LINKEXTRA += $(shell $(PKG_CONFIG) --libs gtk+-3.0)
      else
        LINKEXTRA += $(shell $(PKG_CONFIG) --libs gdk-3.0)
      endif
    else
      LINKEXTRA += -lX11 -lXi
      ifdef SWELL_SUPPORT_GTK
        CFLAGS += -DSWELL_PRELOAD="libgtk+-3.so.0"
      else
        CFLAGS += -DSWELL_PRELOAD="libgdk-3.so.0"
      endif
    endif
  endif
  ifndef NOLICE
    CFLAGS += -DSWELL_LICE_GDI
    OBJS += $(LICE_OBJS)

    ifndef NOFREETYPE
        CFLAGS += -DSWELL_FREETYPE $(shell freetype-config --cflags)
        ifndef PRELOAD_GDK
        LINKEXTRA += $(shell freetype-config --libs)
        endif
    endif
  endif
endif

CXXFLAGS = $(CFLAGS)


### Targets
default: $(APP)

.PHONY: clean run

$(RESFILES): resources/IGraphicsTest.rc
	php $(SWELL_PATH)/mac_resgen.php $^

$(APP): $(OBJS) 
	$(CXX) -o $(APP) $(CFLAGS) $(LFLAGS) $^ $(LINKEXTRA)

run: $(APP)
	./$<
clean:
	-rm $(APP) $(OBJS) \
    #$(RESFILES)

