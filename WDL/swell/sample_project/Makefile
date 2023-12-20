APPNAME=myapp

WDL_PATH = ../WDL
vpath swell%.cpp $(WDL_PATH)/swell
vpath lice%.cpp $(WDL_PATH)/lice
vpath %.c $(WDL_PATH)
vpath %.cpp $(WDL_PATH) $(WDL_PATH)/wingui

###### Objects and resources (probably too many)
SWELL_OBJS = swell.o swell-ini.o swell-miscdlg-generic.o swell-wnd-generic.o \
             swell-menu-generic.o swell-kb-generic.o swell-dlg-generic.o \
             swell-gdi-generic.o swell-misc-generic.o swell-gdi-lice.o \
             swell-generic-gdk.o swell-appstub-generic.o swell-modstub-generic.o

LICE_OBJS = lice_image.o lice_arc.o lice_line.o lice_text.o \
            lice_textnew.o lice.o lice_colorspace.o

OTHER_OBJS = wndsize.o

RESFILES = res.rc_mac_dlg res.rc_mac_menu

OBJS += myapp.o $(SWELL_OBJS) $(OTHER_OBJS)


###### Compiler/Linker flags
CFLAGS += -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wtype-limits \
          -Wno-unused-function -Wno-multichar -Wno-unused-result

CFLAGS += -D_FILE_OFFSET_BITS=64

ARCH := $(shell uname -m)
PKG_CONFIG = pkg-config

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
  CFLAGS += -O0 -g -D_DEBUG -DWDL_CHECK_FOR_NON_UTF8_FOPEN
else
  CFLAGS += -O2 -DNDEBUG
endif

LINKEXTRA =  -lpthread -ldl

ifndef NOGDK
  ifdef GDK2
    CFLAGS += -DSWELL_TARGET_GDK=2 $(shell $(PKG_CONFIG) --cflags gdk-2.0)
    LINKEXTRA += $(shell $(PKG_CONFIG) --libs gdk-2.0)
    LINKEXTRA += -lX11 -lXi
  else
    ifdef SWELL_SUPPORT_GTK
      CFLAGS += -DSWELL_TARGET_GDK=3 $(shell $(PKG_CONFIG) --cflags gtk+-3.0) -DSWELL_SUPPORT_GTK
    else
      CFLAGS += -DSWELL_TARGET_GDK=3 $(shell $(PKG_CONFIG) --cflags gdk-3.0)
    endif
    LINKEXTRA += -lX11 -lXi -lGL
    ifdef SWELL_SUPPORT_GTK
      LINKEXTRA += $(shell $(PKG_CONFIG) --libs gtk+-3.0)
    else
      LINKEXTRA += $(shell $(PKG_CONFIG) --libs gdk-3.0)
    endif
  endif
  CFLAGS += -DSWELL_LICE_GDI
  OBJS += $(LICE_OBJS)

  ifndef NOFREETYPE
    CFLAGS += -DSWELL_FREETYPE $(shell $(PKG_CONFIG) --cflags freetype2)
    LINKEXTRA += $(shell $(PKG_CONFIG) --libs freetype2)
    ifndef NOFONTCONFIG
      CFLAGS += -DSWELL_FONTCONFIG
      LINKEXTRA += -lfontconfig
    endif
  endif
endif


CXXFLAGS = $(CFLAGS)


default: $(APPNAME)

.PHONY: clean run

$(RESFILES): res.rc
	$(WDL_PATH)/swell/swell_resgen.pl $^

$(APPNAME): $(RESFILES) $(OBJS)
	$(CXX) -o $@ $(CFLAGS) $(OBJS) $(LINKEXTRA)

run: $(APPNAME)
	./$^

clean:
	-rm $(OBJS) $(APPNAME) $(RESFILES)
