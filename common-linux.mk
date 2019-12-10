ROOT = ../..
PROJECT_ROOT = .
DEPS_PATH = $(ROOT)/Dependencies
BUILT_LIBS_PATH = $(DEPS_PATH)/Build/linux
BUILT_LIBS_INC_PATH = $(BUILT_LIBS_PATH)/include
BUILT_LIBS_LIB_PATH = $(BUILT_LIBS_PATH)/lib
WDL_PATH = $(ROOT)/WDL
IPLUG_PATH = $(ROOT)/IPlug
SWELL_PATH = $(WDL_PATH)/swell
IGRAPHICS_PATH = $(ROOT)/IGraphics
CONTROLS_PATH = $(IGRAPHICS_PATH)/Controls
PLATFORMS_PATHROOT = ../..
PROJECT_ROOT = .
DEPS_PATH = $(ROOT)/Dependencies
WAM_SDK_PATH = $(DEPS_PATH)/IPlug/WAM_SDK/wamsdk
STB_PATH = $(DEPS_PATH)/IGraphics/STB
WDL_PATH = $(ROOT)/WDL
IPLUG_PATH = $(ROOT)/IPlug
APP_PATH = $(IPLUG_PATH)/APP
IVST3_PATH = $(IPLUG_PATH)/VST3
SWELL_PATH = $(WDL_PATH)/swell
IGRAPHICS_PATH = $(ROOT)/IGraphics
CONTROLS_PATH = $(IGRAPHICS_PATH)/Controls
PLATFORMS_PATH = $(IGRAPHICS_PATH)/Platforms
DRAWING_PATH = $(IGRAPHICS_PATH)/Drawing
IPLUG_EXTRAS_PATH = $(IPLUG_PATH)/Extras
LICE_PATH = $(WDL_PATH)/lice
NANOVG_PATH = $(DEPS_PATH)/IGraphics/NanoVG/src
NANOSVG_PATH = $(DEPS_PATH)/IGraphics/NanoSVG/src
RTAUDIO_PATH = $(DEPS_PATH)/IPlug/RTAudio
RTMIDI_PATH = $(DEPS_PATH)/IPlug/RTMidi
VST3_SDK_PATH = $(DEPS_PATH)/IPlug/VST3_SDK
XCBT_PATH = $(DEPS_PATH)/IGraphics/xcbt

ifeq ($(IGRAPHICS_TYPE), IGRAPHICS_GL2)
	GLAD_PATH = $(DEPS_PATH)/IGraphics/glad_GL2
else ifeq ($(IGRAPHICS_TYPE), IGRAPHICS_GL3)
	GLAD_PATH = $(DEPS_PATH)/IGraphics/glad_GL3
endif
GLAD_GLX_PATH = $(DEPS_PATH)/IGraphics/glad_GLX

IPLUG_SRC := $(addprefix $(IPLUG_PATH)/, IPlugAPIBase.cpp IPlugParameter.cpp IPlugPluginBase.cpp IPlugProcessor.cpp IPlugPaths.cpp IPlugTimer.cpp)

APP_SRC := $(addprefix $(APP_PATH)/, IPlugAPP.cpp IPlugAPP_main.cpp IPlugAPP_dialog.cpp IPlugAPP_host.cpp)

IVST3_SRC := $(addprefix $(IVST3_PATH)/, IPlugVST3.cpp IPlugVST3_Controller.cpp IPlugVST3_Processor.cpp IPlugVST3_ProcessorBase.cpp IPlugVST3_RunLoop.cpp)

VST3_SDK_SRC := $(addprefix $(VST3_SDK_PATH)/base/source/, fstring.cpp fobject.cpp updatehandler.cpp fdebug.cpp baseiids.cpp) 
VST3_SDK_SRC += $(addprefix $(VST3_SDK_PATH)/base/thread/source/, flock.cpp)
VST3_SDK_SRC += $(wildcard $(VST3_SDK_PATH)/pluginterfaces/base/*.cpp)
VST3_SDK_SRC += $(wildcard $(VST3_SDK_PATH)/public.sdk/source/common/*.cpp)
VST3_SDK_SRC += $(addprefix $(VST3_SDK_PATH)/public.sdk/source/vst/, vstaudioeffect.cpp vstcomponent.cpp vstcomponentbase.cpp vstinitiids.cpp vstsinglecomponenteffect.cpp vstparameters.cpp vstbus.cpp)
VST3_SDK_SRC += $(addprefix $(VST3_SDK_PATH)/public.sdk/source/vst/hosting/, parameterchanges.cpp)
VST3_SDK_SRC += $(addprefix $(VST3_SDK_PATH)/public.sdk/source/main/, pluginfactory.cpp linuxmain.cpp)

CONTROLS_SRC := $(wildcard $(CONTROLS_PATH)/*.cpp)
IGRAPHICS_SRC := $(addprefix $(IGRAPHICS_PATH)/, IGraphics.cpp IControl.cpp IGraphicsEditorDelegate.o) $(CONTROLS_SRC) $(PLATFORMS_PATH)/IGraphicsLinux.cpp

RESFILES = resources/main.rc_mac_dlg resources/main.rc_mac_menu

IPLUG_INC_PATHS = -I$(PROJECT_ROOT) \
-I$(WDL_PATH) \
-I$(SWELL_PATH) \
-I$(IPLUG_PATH) \
-I$(IPLUG_EXTRAS_PATH) \
-I$(IPLUG_WEB_PATH)

IGRAPHICS_INC_PATHS = $(IGRAPHICS_PATH) \
-I$(DRAWING_PATH) \
-I$(CONTROLS_PATH) \
-I$(PLATFORMS_PATH) \
-I$(NANOVG_PATH) \
-I$(NANOSVG_PATH) \
-I$(LICE_PATH) \
-I$(DEPS_PATH)/IPlug/SWELL \
-I$(GLAD_PATH)/include \
-I$(GLAD_PATH)/src \
-I$(GLAD_GLX_PATH)/include \
-I$(GLAD_GLX_PATH)/src \
-I$(XCBT_PATH)

# AZ: CP from WDL/swell/Makefile
#
#   make SWELL_SUPPORT_GTK=1
#     or make NOGDK=1
#     or make DEBUG=1
#   etc
SWELL_SUPPORT_GTK = 1
COMPILER=GCC
ALLOW_WARNINGS = 1

ARCH := $(shell uname -m)
UNAME_S := $(shell uname -s)

PKG_CONFIG = pkg-config

CFLAGS = -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wno-unused-function -Wno-multichar
DLL_EXT=.so

ifeq ($(COMPILER),CLANG)
  CC = clang
  CXX = clang++
endif

ifeq ($(COMPILER),ICC)
  CC = icc
  CXX = icpc
  CFLAGS += -D__PURE_SYS_C99_HEADERS__
else
  CFLAGS +=  -Wno-unused-result
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
ifeq ($(ARCH),aarch64)
  CFLAGS += -fsigned-char
endif


ifdef DEBUG
CFLAGS += -O0 -g -D_DEBUG
else
CFLAGS += -O2 -DNDEBUG
  ifdef DEBUG_INFO
    CFLAGS += -g
  else
    ifneq ($(COMPILER),CLANG)
      CFLAGS += -s
    endif
  endif
endif

SWELL_SRC = $(addprefix $(SWELL_PATH)/, swell.cpp swell-ini.cpp swell-miscdlg-generic.cpp swell-wnd-generic.cpp \
             swell-menu-generic.cpp swell-kb-generic.cpp swell-dlg-generic.cpp \
             swell-gdi-generic.cpp swell-misc-generic.cpp swell-gdi-lice.cpp \
             swell-generic-headless.cpp swell-generic-gdk.cpp \
             swell-appstub-generic.cpp swell-modstub-generic.cpp)

LICE_SRC = $(addprefix $(LICE_PATH)/, lice.cpp  lice_arc.cpp lice_colorspace.cpp lice_line.cpp lice_text.cpp \
            lice_textnew.cpp lice_ico.cpp lice_bmp.cpp)
LICE_SWELL_SRC = $(addprefix $(LICE_PATH)/, lice_colorspace.cpp)

LINKEXTRA =

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
      ifdef SWELL_SUPPORT_GTK
        LINKEXTRA += $(shell $(PKG_CONFIG) --libs gtk+-3.0)
      else
        LINKEXTRA += $(shell $(PKG_CONFIG) --libs gdk-3.0)
      endif
    else
      ifdef SWELL_SUPPORT_GTK
        CFLAGS += -DSWELL_PRELOAD="libgtk+-3.so.0"
      else
        CFLAGS += -DSWELL_PRELOAD="libgdk-3.so.0"
      endif
    endif
  endif
  LINKEXTRA += -lX11 -lXi
  CFLAGS += -DSWELL_LICE_GDI

  ifndef NOFREETYPE
    CFLAGS += -DSWELL_FREETYPE $(shell $(PKG_CONFIG) --cflags freetype2)
    ifndef PRELOAD_GDK
      LINKEXTRA += $(shell $(PKG_CONFIG) --libs freetype2)
    endif
    ifndef NOFONTCONFIG
      CFLAGS += -DSWELL_FONTCONFIG
      LINKEXTRA += -lfontconfig
    endif
  endif
endif

LINKEXTRA +=  -lpthread -ldl
