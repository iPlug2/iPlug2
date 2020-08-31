#
#==============================================================================
#
# This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
# See LICENSE.txt for  more info.
#
# ==============================================================================
#

###
# Sources for dependecies
# _NEED_XXX should be set to download/build them

_ZLIB_VERSION := 1.2.11
_ZLIB_DIR     := zlib-$(_ZLIB_VERSION)
_ZLIB_SRC     := $(_ZLIB_DIR).tar.gz
_ZLIB_NET_SRC := https://www.zlib.net/$(_ZLIB_SRC)

_LIBPNG_VERSION := 1.6.37
_LIBPNG_NET_SRC :=  https://github.com/glennrp/libpng/archive/v$(_LIBPNG_VERSION).tar.gz

# NOT READY
_HARFBUZZ_VERSION := 2.6.4
_HARFBUZZ_DIR := harfbuzz-$(_HARFBUZZ_VERSION)
_HARFBUZZ_SRC := $(_HARFBUZZ_DIR).tar.xz
_HARFBUZZ_NET_SRC := https://www.freedesktop.org/software/harfbuzz/release/$(_HARFBUZZ_SRC)

_FONTCONFIG_VERSION := 2.13.1
_FONTCONFIG_DIR := fontconfig-$(_FONTCONFIG_VERSION)
_FONTCONFIG_SRC := $(_FONTCONFIG_DIR).tar.gz
_FONTCONFIG_NET_SRC := https://www.freedesktop.org/software/fontconfig/release/$(_FONTCONFIG_SRC)

_FREETYPE_VERSION := 2.10.1
_FREETYPE_DIR     := freetype-$(_FREETYPE_VERSION)
_FREETYPE_SRC     := $(_FREETYPE_DIR).tar.gz
_FREETYPE_NET_SRC := https://download.savannah.gnu.org/releases/freetype/$(_FREETYPE_SRC)

_PIXMAN_VERSION := 0.38.4
_PIXMAN_DIR := pixman-$(_PIXMAN_VERSION)
_PIXMAN_SRC := $(_PIXMAN_DIR).tar.gz
_PIXMAN_NET_SRC := https://www.cairographics.org/releases/$(_PIXMAN_SRC)

_EXPAT_VERSION := 2.2.9
_EXPAT_DIR := expat-$(_EXPAT_VERSION)
_EXPAT_SRC := $(_EXPAT_DIR).tar.gz
_EXPAT_NET_SRC := https://github.com/libexpat/libexpat/releases/download/R_2_2_9/$(_EXPAT_SRC)

_CAIRO_VERSION := 1.16.0
_CAIRO_DIR := cairo-$(_CAIRO_VERSION)
_CAIRO_SRC := $(_CAIRO_DIR).tar.xz
_CAIRO_NET_SRC := https://www.cairographics.org/releases/$(_CAIRO_SRC)

#### IPlug
_IPLUG_PATH := $(IROOT)/IPlug
_WDL_PATH := $(IROOT)/WDL
_IDEPS_PATH := $(IROOT)/Dependencies
_IDEPS_INSTALL_PATH := $(_IDEPS_PATH)/Build/Linux-$(IARCH)
_IGRAPHICS_DEPS_PATH := $(_IDEPS_PATH)/IGraphics
_IPLUG_DEPS_PATH := $(_IDEPS_PATH)/IPlug

IPSRC_DIR += $(_IPLUG_PATH) $(_WDL_PATH)
IPINC_DIR += $(_IPLUG_PATH)/Extras

CXXFLAGS += -Wall -std=c++14 -DNOMINMAX -fPIC -DPIC -pipe -fvisibility=hidden
CFLAGS   += -Wall -fPIC -DPIC -pipe -fvisibility=hidden
LIBS     += -lpthread -ldl

# add debug
ifneq ($(IDEBUG),)
  CFLAGS += -g -D_DEBUG -O0
  CXXFLAGS += -g -D_DEBUG -O0
  LFLAGS += -g
else
  CFLAGS += -O2 -DRELEASE
  CXXFLAGS += -O2 -DRELEASE
endif

# disable quite some warnings
CXXFLAGS += -Wno-multichar -Wno-unknown-pragmas -Wno-reorder -Wno-sign-compare -Wno-sequence-point
CXXFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-result
# WDL needs that
CXXFLAGS += -Wno-strict-aliasing -Wno-strict-overflow -Wno-maybe-uninitialized


# basis object
_IPSRC := $(notdir $(wildcard $(_IPLUG_PATH)/*.cpp))
IPT_DEPS += $(_IPSRC:%.cpp=%.o)

# extras
IPSRC_DIR += $(_IPLUG_PATH)/Extras $(_IPLUG_PATH)/Extras/Synth


ifeq ($(ITARGET),APP)
  CXXFLAGS += -DAPP_API -DIPLUG_DSP -DIPLUG_EDITOR
	
	# APP source
  IPSRC_DIR += $(_IPLUG_PATH)/APP
  IPT_DEPS += IPlugAPP.o IPlugAPP_dialog.o IPlugAPP_host.o IPlugAPP_main.o

  # APP is using SWELL
  _SWELL_PATH := $(_WDL_PATH)/swell
  _SWELL_LIB_PATH := $(_IDEPS_INSTALL_PATH)/lib/libSwell.so
  
  # resource generation
  _RESDIR  := $(IPBOD)/resources
  IPSRC_DIR += $(_SWELL_PATH)
  IPINC_DIR += $(_IPLUG_DEPS_PATH)/SWELL $(IPBOD)
  IPBED += $(_RESDIR)
  _RESFILE := $(_RESDIR)/main.rc_mac_dlg

  # dynamically loading libSwell
	CXXFLAGS += -DSWELL_PROVIDED_BY_APP
  IPBSS += SWELL
  CXXFLAGSE_SWELL := -DSWELL_LOAD_SWELL_DYLIB
  IPT_DEPS += SWELL/swell-modstub-generic.o libSwell.so

  # Check which audio backends we have in the system
  _RTAUDIO_CONFIG :=
  _RTAUDIO_LIBS :=
  ifneq ($(wildcard /usr/include/alsa),)
    _RTAUDIO_CONFIG += --with-alsa
    _RTAUDIO_LIBS += -lasound
  endif
  ifneq ($(wildcard /usr/include/pulse),)
    _RTAUDIO_CONFIG += --with-pulse
    _RTAUDIO_LIBS += -lpulse-simple -lpulse
  endif
  ifneq ($(wildcard /usr/include/jack),)
    _RTAUDIO_CONFIG += --with-jack
    _RTAUDIO_LIBS += -ljack
  endif
	# RTAudio/RTMidi, includes directly from the source
	IPINC_DIR += $(_IPLUG_DEPS_PATH)/RTAudio
	IPINC_DIR += $(_IPLUG_DEPS_PATH)/RTMidi
	# Libraries
  LIBS +=  $(_IDEPS_INSTALL_PATH)/lib/librtaudio.a  $(_RTAUDIO_LIBS)
  LIBS +=  $(_IDEPS_INSTALL_PATH)/lib/librtmidi.a
  IPB_DEPS += $(_IDEPS_INSTALL_PATH)/lib/librtaudio.a $(_IDEPS_INSTALL_PATH)/lib/librtmidi.a
	
else ifeq ($(ITARGET),VST2)
  # TODO: set correct defines, f.e. for debug
  CXXFLAGS += -DSMTG_OS_LINUX -DVST2_API -DIPLUG_DSP -DIPLUG_EDITOR

  # VST2 source
  IPSRC_DIR += $(_IPLUG_PATH)/VST2
	IPT_DEPS += IPlugVST2.o

	# VST2 SDK, see README in this folder
	IPINC_DIR += $(_IPLUG_DEPS_PATH)/VST2_SDK

else ifeq ($(ITARGET),VST3)
  # TODO: set correct defines, f.e. for debug
  CXXFLAGS += -DSMTG_OS_LINUX -DVST3_API -DIPLUG_DSP -DIPLUG_EDITOR

  # VST3 source
  IPSRC_DIR += $(_IPLUG_PATH)/VST3
	IPT_DEPS += IPlugVST3.o IPlugVST3_Controller.o IPlugVST3_Processor.o IPlugVST3_ProcessorBase.o IPlugVST3_RunLoop.o

	# VST3 SDK  We compile "in project" way.
	_VST3_SDK_PATH := $(_IPLUG_DEPS_PATH)/VST3_SDK

  # we need VST3 SDK before we can construct dependencies, so we can not do that as a rule
  ifeq ($(wildcard $(_VST3_SDK_PATH)/public.sdk/source/vst3stdsdk.cpp),)
    $(info "Downloading VST3 SDK ...")
    $(info $(shell cd $(_IPLUG_DEPS_PATH) && rm -rf VST3_SDK && \
       git clone https://github.com/steinbergmedia/vst3sdk.git VST3_SDK && cd VST3_SDK &&\
       git submodule update --init pluginterfaces && \
       git submodule update --init base && \
       git submodule update --init public.sdk))
  endif

	# Prepend arbitrary file to dependencies (so it is checked before any compilation)
	IPB_DEPS += $(_VST3_SDK_PATH)/public.sdk/source/vst3stdsdk.cpp $(IPB_DEPS)
	IPINC_DIR += $(_VST3_SDK_PATH)
	IPSRC_DIR += $(_VST3_SDK_PATH)/base/source $(_VST3_SDK_PATH)/base/thread/source $(_VST3_SDK_PATH)/pluginterfaces/base
	IPSRC_DIR += $(_VST3_SDK_PATH)/public.sdk/source/common $(_VST3_SDK_PATH)/public.sdk/source/vst $(_VST3_SDK_PATH)/public.sdk/source/vst/hosting
	IPSRC_DIR += $(_VST3_SDK_PATH)/public.sdk/source/vst/hosting $(_VST3_SDK_PATH)/public.sdk/source/main/
	IPT_DEPS += fstring.o fobject.o updatehandler.o fdebug.o baseiids.o flock.o
	_VST3_SRC_BASE := $(notdir $(wildcard $(_VST3_SDK_PATH)/pluginterfaces/base/*.cpp))
	_VST3_SRC_COMMON := $(notdir $(wildcard $(_VST3_SDK_PATH)/public.sdk/source/common/*.cpp))
	IPT_DEPS += $(_VST3_SRC_BASE:%.cpp=%.o) $(_VST3_SRC_COMMON:%.cpp=%.o)
	IPT_DEPS += vstaudioeffect.o vstcomponent.o vstcomponentbase.o vstinitiids.o vstsinglecomponenteffect.o vstparameters.o vstbus.o
	IPT_DEPS += parameterchanges.o pluginfactory.o linuxmain.o
else ifeq ($(ITARGET),LV2)
  CXXFLAGS += -DLV2_API

	CXXFLAGSE     := -DIPLUG_DSP
	CXXFLAGSE_CFG := -DIPLUG_DSP -DLV2_CFG
  CXXFLAGSE_UI  := -DIPLUG_EDITOR

  # IPlug LV2 source
  IPSRC_DIR += $(_IPLUG_PATH)/LV2

  # UI is compiled separately
  IPT_DEPS_UI   := $(addprefix UI/, $(IPT_DEPS)) UI/IPlugLV2.o

  IPT_DEPS_CFG  := $(addprefix CFG/, $(IPT_DEPS)) CFG/IPlugLV2.o CFG/IPlugLV2_cfg.o

  # make ttl deps for DSP (only), "also" ttl is build in parallel with manifest
  IPT_DEPS += IPlugLV2.o manifest.ttl

	# LV2 source
	_LV2_PATH := $(_IPLUG_DEPS_PATH)/LV2
	IPINC_DIR += $(_LV2_PATH)
  
  # we need LV2 source before we can construct dependencies, so we can not do that as a rule
  ifeq ($(wildcard $(_LV2_PATH)/waf),)
    $(info "Downloading LV2 SDK ...")
    $(info $(shell cd $(_IPLUG_DEPS_PATH) && rm -rf LV2 && git clone https://gitlab.com/lv2/lv2.git LV2))
  endif
endif

#### IGraphics is included even in case of IGRAPHICS == NO_IGRAPHICS
_IGRAPHICS_PATH := $(IROOT)/IGraphics

IPSRC_DIR += $(_IGRAPHICS_PATH)

ifeq ($(IGRAPHICS),)
	IGRAPHICS := NANOVG_GL2
endif

ifneq ($(IGRAPHICS),NO_IGRAPHICS)
  # graphics is in UI IPTSS for LV2
  _TSX  :=
  _TSXD :=
  ifeq ($(ITARGET),LV2)
    _TSX  := _UI
    _TSXD := UI/
    CXXFLAGSE += -DNO_IGRAPHICS -DLV2_WITH_UI
    CXXFLAGSE_CFG += -DNO_IGRAPHICS -DLV2_WITH_UI
  endif

  ifeq ($(IGRAPHICS),NANOVG_GL2)
    _GL_VERSION := GL2
    CXXFLAGSE$(_TSX) += -DIGRAPHICS_GL2
    _IGRAPHICS := NANOVG
  else ifeq ($(IGRAPHICS),NANOVG_GL3)
    _GL_VERSION := GL3
    CXXFLAGSE$(_TSX) += -DIGRAPHICS_GL3
    _IGRAPHICS := NANOVG
  else ifeq ($(IGRAPHICS),SKIA_GL2)
    _GL_VERSION := GL2
    CXXFLAGSE$(_TSX) += -DIGRAPHICS_GL2
    _IGRAPHICS := SKIA
  else ifeq ($(IGRAPHICS),SKIA_GL3)
    _GL_VERSION := GL3
    CXXFLAGSE$(_TSX) += -DIGRAPHICS_GL3
    _IGRAPHICS := SKIA
  else
    _IGRAPHICS := $(IGRAPHICS)
  endif

  IPSRC_DIR += $(_IGRAPHICS_PATH)/Platforms $(_IGRAPHICS_PATH)/Drawing $(_IGRAPHICS_PATH)/Controls
  IPT_DEPS$(_TSX)  += $(_TSXD)IGraphicsLinux.o

	_IGSRC := $(notdir $(wildcard $(_IGRAPHICS_PATH)/*.cpp) $(wildcard $(_IGRAPHICS_PATH)/Controls/*.cpp))
	IPT_DEPS$(_TSX) += $(addprefix $(_TSXD), $(_IGSRC:%.cpp=%.o))

	IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/STB

  # XCBT (always with GL support at the moment)
  ifeq ($(_GL_VERSION),GL3)
    _GLAD_PATH := $(_IGRAPHICS_DEPS_PATH)/glad_GL3
  else
	  _GLAD_PATH := $(_IGRAPHICS_DEPS_PATH)/glad_GL2
  endif
	IPSRC_DIR += $(_GLAD_PATH)/src
	IPINC_DIR += $(_GLAD_PATH)/include
	_GLAD_GLX_PATH := $(_IGRAPHICS_DEPS_PATH)/glad_GLX
	IPSRC_DIR += $(_GLAD_GLX_PATH)/src
	IPINC_DIR += $(_GLAD_GLX_PATH)/include
  IPSRC_DIR += $(_IGRAPHICS_DEPS_PATH)/xcbt
  IPT_DEPS$(_TSX)  += $(_TSXD)xcbt.o $(_TSXD)glad.o $(_TSXD)glad_glx.o
  LIBS$(_TSX) += -lxcb -lfontconfig

  ifeq ($(_IGRAPHICS),NANOVG)
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoVG/src
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoSVG/src
		
	  CXXFLAGSE$(_TSX) += -DIGRAPHICS_NANOVG
	  
	  # Not nice, but stb_ produce that and it is included directly
	  CXXFLAGSE$(_TSX) += -Wno-misleading-indentation
	else ifeq ($(_IGRAPHICS),SKIA)
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoSVG/src
    
    _SKIA_PATH = $(_IGRAPHICS_DEPS_PATH)/skia
    
    ifeq ($(wildcard $(_SKIA_PATH)/include/core),)
      $(info "Downloading SKIA ...")
      $(info $(shell cd $(_IGRAPHICS_DEPS_PATH) && rm -rf skia && git clone https://skia.googlesource.com/skia.git))
    endif

    ifeq ($(wildcard $(_SKIA_PATH)/third_party/externals),)
      $(info "Downloading SKIA externals ...")
      $(info $(shell cd $(_SKIA_PATH) && rm -rf .git && python2 tools/git-sync-deps))
    endif
    
    IPINC_DIR += $(_SKIA_PATH) $(_SKIA_PATH)/include/core $(_SKIA_PATH)/include/effects $(_SKIA_PATH)/include/gpu $(_SKIA_PATH)/experimental/svg/model

    CXXFLAGSE$(_TSX) += -DIGRAPHICS_SKIA
		
		LIBS$(_TSX) += $(_IDEPS_INSTALL_PATH)/skia/libskia.a
    IPB_DEPS$(_TSX) += $(_IDEPS_INSTALL_PATH)/skia/libskia.a

    # some extra libraries
    LIBS$(_TSX) += -lfreetype -lGL -lexpat -lz
  else ifeq ($(_IGRAPHICS),LICE)
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoSVG/src

		# LICE is using SWELL
		_SWELL_PATH := $(_WDL_PATH)/swell
    _LICE_PATH := $(_WDL_PATH)/lice
    
    IPINC_DIR += $(_LICE_PATH) $(_SWELL_PATH) $(_IPLUG_DEPS_PATH)/SWELL
    
    CXXFLAGSE$(_TSX) += -DIGRAPHICS_LICE
    
    _NEED_LIBPNG := yes
    LIBS$(_TSX) += -lpng -lz

    ifneq ($(ITARGET),APP)
      # parts of SWELL are needed for LICE
      CXXFLAGSE$(_TSX) += -DSWELL_EXTRA_MINIMAL -DSWELL_LICE_GDI -DSWELL_FREETYPE
      IPSRC_DIR += $(_SWELL_PATH)
      # that is headless swell with freetype font engine
      _SWELL_DEPS := swell-gdi-generic.o swell-wnd-generic.o swell-menu-generic.o swell-generic-headless.o swell-dlg-generic.o swell-gdi-lice.o swell.o swell-ini.o
      _SWELL_DEPS := $(addprefix $(_TSXD), $(_SWELL_DEPS))
      IPT_DEPS$(_TSX) += $(_SWELL_DEPS)
      # freetype
      IPINC_DIR += /usr/include/freetype2
      LIBS$(_TSX) += -lfreetype -ldl -lpthread
    endif
  else ifeq ($(_IGRAPHICS),CAIRO)
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoSVG/src

    _NEED_CAIRO := yes
    IPINC_DIR += $(_IDEPS_INSTALL_PATH)/include

    CXXFLAGSE$(_TSX) += -DIGRAPHICS_CAIRO
    
    LIBS$(_TSX) += -lcairo -lpixman-1 -lfreetype -lpng -lz -lxcb-shm -lxcb-render -lxcb
  else ifeq ($(_IGRAPHICS),AGG)
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoSVG/src

		_NEED_FREETYPE := yes
		_AGG_PATH := $(_IGRAPHICS_DEPS_PATH)/AGG/agg-2.4

		IPINC_DIR += $(_AGG_PATH)/include  $(_AGG_PATH)/include/platform/linux $(_AGG_PATH)/src $(_AGG_PATH)/src/platform/linux $(_AGG_PATH)/font_freetype
		
		CXXFLAGSE$(_TSX) += -DIGRAPHICS_AGG		

    LIBS$(_TSX) += -lfreetype -lpng -lz -lxcb
  else
    $(error FATAL: '$(IGRAPHICS)' graphics flaviour is not currently supported)
  endif
endif

# Downloading and installing dependencies which are not included into iPlug source tree

# CAIRO
ifneq ($(_NEED_CAIRO),)
	_NEED_LIBPNG := yes
	_NEED_FREETYPE := yes

  _CAIRO_TARGET := lib/libcairo.a
  _CAIRO_CFG := --disable-dependency-tracking --disable-shared  --disable-gtk-doc --disable-valgrind --enable-xlib=no --enable-xcb=yes --enable-xcb-shm=yes --enable-qt=no --enable-ft=yes --enable-fc=no
	_CAIRO_CFG_ENV := CFLAGS='-O2 -fPIC'

  $(eval $(call dep_dcmi,CAIRO,IGRAPHICS))
endif

# PIXMAN
ifneq ($(_NEED_PIXMAN),)
  _NEED_LIBPNG := yes

  _PIXMAN_TARGET := lib/libpixman-1.a
  _PIXMAN_CFG := --disable-shared --disable-gtk
	_PIXMAN_CFG_ENV := CFLAGS='-O2 -fPIC'

  $(eval $(call dep_dcmi,PIXMAN,IGRAPHICS))
endif

# FONTCONFIG
# since configuration which make sense is system wide, I do not use local fontconfig
ifneq ($(_NEED_FONTCONFIG),)
  _NEED_EXPAT := yes

  _FONTCONFIG_TARGET := lib/libft2.a
  _FONTCONFIG_CFG := --disable-shared
	_FONTCONFIG_CFG_ENV := CFLAGS='-O2 -fPIC'

  $(eval $(call dep_dcmi,FONTCONFIG,IGRAPHICS))
endif


# FREETYPE
ifneq ($(_NEED_FREETYPE),)
  _NEED_ZLIB := yes
  _NEED_LIBPNG := yes
  IPINC_DIR += $(_IDEPS_INSTALL_PATH)/include/freetype2
  $(info $(IPINC_DIR))
  # _NEED_HARFBUZZ := no  #  at least for now 

  _FREETYPE_TARGET := lib/libfreetype.a
  _FREETYPE_CFG := --disable-shared --with-bzip2=no --with-harfbuzz=no
	_FREETYPE_CFG_ENV := CFLAGS='-O2 -fPIC'

  $(eval $(call dep_dcmi,FREETYPE,IGRAPHICS))
endif


# HARFBUZZ
# Fail to compile and not used at the moment
ifneq ($(_NEED_HARFBUZZ),)

  _HARFBUZZ_TARGET := lib/libharfbuzz.a
  _HARFBUZZ_CFG := --disable-dependency-tracking --disable-shared --disable-gtk-doc
	_HARFBUZZ_CFG_ENV := CFLAGS='-O2 -fPIC'

  $(eval $(call dep_dcmi,HARFBUZZ,IGRAPHICS))
endif


# libpng
ifneq ($(_NEED_LIBPNG),)
  _NEED_ZLIB := yes

  _LIBPNG_TARGET := lib/libpng.a
  _LIBPNG_DIR := libpng-$(_LIBPNG_VERSION)
  _LIBPNG_SRC := $(_LIBPNG_DIR).tar.gz
  _LIBPNG_CFG := --disable-shared 
  _LIBPNG_CFG_ENV := CFLAGS='-I$(_IDEPS_INSTALL_PATH)/include -O2 -fPIC' LDFLAGS=-L$(_IDEPS_INSTALL_PATH)/lib

  $(eval $(call dep_dcmi,LIBPNG,IGRAPHICS))
endif

# zlib
ifneq ($(_NEED_ZLIB),)
  _ZLIB_TARGET := lib/libz.a
  _ZLIB_CFG := --static 
	_ZLIB_CFG_ENV := CFLAGS='-O3 -fPIC'

  $(eval $(call dep_dcmi,ZLIB,IGRAPHICS))
endif


# EXPAT
ifneq ($(_NEED_EXPAT),)
  _EXPAT_TARGET := lib/libexpat.a
  _EXPAT_CFG := --disable-shared --without-xmlwf --without-examples --without-tests --disable-dependency-tracking
	_EXPAT_CFG_ENV := CFLAGS='-O2 -fPIC'

  $(eval $(call dep_dcmi,EXPAT,IGRAPHICS))
endif


# I do not want rules till the end, Makefile indentation bugs are displayed better then
ifneq ($(_SKIA_PATH),)
$(_IDEPS_INSTALL_PATH)/skia/libskia.a:
		@echo "Compiling skia..."
		@cd $(_SKIA_PATH) && ./bin/gn gen $(_IDEPS_INSTALL_PATH)/skia --args='is_official_build = true is_debug = false skia_use_system_libjpeg_turbo = false skia_use_system_libpng = false skia_use_system_zlib = false skia_use_libwebp = false skia_use_xps = false skia_use_dng_sdk = false skia_use_expat = true skia_use_icu = true skia_use_sfntly = false skia_enable_skottie = true skia_enable_pdf = false skia_enable_particles = true skia_enable_gpu = true skia_enable_skparagraph = true skia_enable_sksl_interpreter = true cc = "clang" cxx = "clang++"'
		@cd $(_SKIA_PATH) && ninja -C  $(_IDEPS_INSTALL_PATH)/skia $(SKIA_NINJA_ARGS)
endif

ifeq ($(ITARGET),APP)

# AKA resouce file generation	
$(_RESFILE): $(IP_ROOT)/resources/main.rc
		@echo "Generating resource ..."
		@cp $< $(IPBOD)/resources
		php $(_SWELL_PATH)/mac_resgen.php $(IPBOD)/resources/$(notdir $<)

# and make it explicit dependency
$(IPBOD)/IPlugAPP_main.o: $(_RESFILE)

# generating libSwell
$(_SWELL_LIB_PATH):
		@echo "Compiling SWELL ..."
		@[ -e $(dir $@) ] || mkdir -p $(dir $@)
		@cd $(_SWELL_PATH); make
		@cp $(_SWELL_PATH)/libSwell.so $@
		@cd $(_SWELL_PATH); make clean >/dev/null 2>/dev/null || true

# copy libSwell
$(IPBD)/libSwell.so: $(_SWELL_LIB_PATH)
		@echo "Copy SWELL ..."
		@cp $< $@

# compiling RTAudio
$(_IDEPS_INSTALL_PATH)/lib/librtaudio.a:
		@echo "Compiling RTAudio ..."
		@[ -e $(dir $@) ] || mkdir -p $(dir $@)
		@rm -rf $(_IDEPS_INSTALL_PATH)/tmp
		@mkdir $(_IDEPS_INSTALL_PATH)/tmp
		@cp -a $(_IDEPS_PATH)/IPlug/RTAudio/* $(_IDEPS_INSTALL_PATH)/tmp/
		@cd $(_IDEPS_INSTALL_PATH)/tmp && ./autogen.sh --no-configure && ./configure $(_RTAUDIO_CONFIG) && make
		@cp $(_IDEPS_INSTALL_PATH)/tmp/.libs/librtaudio.a $@
		@rm -rf $(_IDEPS_INSTALL_PATH)/tmp

# compiling RTMidi
$(_IDEPS_INSTALL_PATH)/lib/librtmidi.a:
		@echo "Compiling RTMidi ..."
		@[ -e $(dir $@) ] || mkdir -p $(dir $@)
		@rm -rf $(_IDEPS_INSTALL_PATH)/tmp
		@mkdir $(_IDEPS_INSTALL_PATH)/tmp
		@cp -a $(_IDEPS_PATH)/IPlug/RTMidi/* $(_IDEPS_INSTALL_PATH)/tmp/
		@cd $(_IDEPS_INSTALL_PATH)/tmp && ./autogen.sh && make
		@cp $(_IDEPS_INSTALL_PATH)/tmp/.libs/librtmidi.a $@
		@rm -rf $(_IDEPS_INSTALL_PATH)/tmp

else ifeq ($(ITARGET),VST3)

else ifeq ($(ITARGET),LV2)

$(IPBD)/manifest.ttl: $(IPBOD)/cfg
		@echo "Generating ttl files ..."
		@cd $(IPBD) && .obj/cfg

endif
