#
#==============================================================================
#
# This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
# See LICENSE.txt for  more info.
#
# ==============================================================================
#

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
  _SWELL_PATH = $(_WDL_PATH)/swell
  _SWELL_LIB_PATH = $(_IDEPS_INSTALL_PATH)/lib/libSwell.so
  
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
  # for LV2, graphics is in UI IPTSS
  _TSX  :=
  _TSXD :=
  ifeq ($(ITARGET),LV2)
    _TSX  := _UI
    _TSXD := UI/
    CXXFLAGSE += -DNO_IGRAPHICS -DLV2_WITH_UI
    CXXFLAGSE_CFG += -DNO_IGRAPHICS -DLV2_WITH_UI
  endif

  IPSRC_DIR += $(_IGRAPHICS_PATH)/Platforms $(_IGRAPHICS_PATH)/Drawing $(_IGRAPHICS_PATH)/Controls
  IPT_DEPS$(_TSX)  += $(_TSXD)IGraphicsLinux.o

	_IGSRC := $(notdir $(wildcard $(_IGRAPHICS_PATH)/*.cpp) $(wildcard $(_IGRAPHICS_PATH)/Controls/*.cpp))
	IPT_DEPS$(_TSX) += $(addprefix $(_TSXD), $(_IGSRC:%.cpp=%.o))

	IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/STB

  # XCBT (always with GL support at the moment)
	_GLAD_PATH := $(_IGRAPHICS_DEPS_PATH)/glad_GL2
	IPSRC_DIR += $(_GLAD_PATH)/src
	IPINC_DIR += $(_GLAD_PATH)/include
	_GLAD_GLX_PATH := $(_IGRAPHICS_DEPS_PATH)/glad_GLX
	IPSRC_DIR += $(_GLAD_GLX_PATH)/src
	IPINC_DIR += $(_GLAD_GLX_PATH)/include
  IPSRC_DIR += $(_IGRAPHICS_DEPS_PATH)/xcbt
  IPT_DEPS$(_TSX)  += $(_TSXD)xcbt.o $(_TSXD)glad.o $(_TSXD)glad_glx.o
  LIBS$(_TSX) += -lxcb -lfontconfig

  ifeq ($(IGRAPHICS),NANOVG_GL2)
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoVG/src
    IPINC_DIR += $(_IGRAPHICS_DEPS_PATH)/NanoSVG/src
		
	  CXXFLAGSE$(_TSX) += -DIGRAPHICS_NANOVG -DIGRAPHICS_GL2
	  
	  # Not nice, but stb_ produce that and it is included directly
	  CXXFLAGSE$(_TSX) += -Wno-misleading-indentation

  else
    $(error FATAL: '$(IGRAPHICS)' graphics flaviour is not currently supported)
  endif
endif

# I do not want rules till the end, Makefile indentation bugs are displayed better then
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
$(info $(_IDEPS_INSTALL_PATH)/lib/librtaudio.a)
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
$(info $(_IDEPS_INSTALL_PATH)/lib/librtmidi.a)
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
