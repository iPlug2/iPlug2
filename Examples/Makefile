######
#
# Copyright (C) Alexey Zhelezov, 2019 www.azslow.com
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
######
#
# Generic iPlug 2 plug-in makefile for Linux
#  * target can be specified as an argument to make or as compile time setting ITARGET (see later),
#        f.e. make APP [VST2|VST3|LV2|clean]
#  * particular target can be cleaned, without cleaning other targets
#        f.e. make IOP=CLEAN APP
#  * compile time settings can be specified as (exported) environment variables, in make command line or in makefile
#        f.e. make IGRAPHICS=NANOVG VST3
#  * config.h is parsed
#  * some constants are taken from config.h, but compile time setting ICONF_xxx for xxx, note that will not
#    overwrite config.h (what and from where was used is printed during build process, for clarity).

###
# Compile time settings
###

## Directory path for iPlug2 IROOT
#   by default it will be set to ../.. from plug-in directory, it should be set
#   in case plug-in is not iPlug2 examples
#IROOT := 

## Plug-in incarnation ITARGET: 
#   APP - standalone application (default)
#   VST2
#   VST3
#   LV2
#	ITARGET := APP

## Operation with target IOP:
#   BUILD - build specified target (default)
#   CLEAN - clean specified target (only, other targets will not be cleaned)
#	IOP := BUILD

## Graphic engine IGRAPHICS:
#   NANOVG_GL2 - NanoVG library with GL2 backend (default)
#   NANOVG_GL3 - NanoVG library with GL3 backend
#   SKIA_GL2   - SKIA with GL2
#   SKIA_GL3   - SKIA with GL3
#   LICE       - LICE
#   CAIRO      - CAIRO (raster rendering)
#   AGG        - AGG
#	IGRAPHICS := NANOVG_GL2

## Build directory
#     ../Build/Linux-<arch>/<project name> by default
# IBUILD :=

## iPlug2 root directory
#  <project directory>/../.. by default, correct for iPlug2 examples only
# IROOT :=

## Has to be "YES" to produce debug build
# IDEBUG := [NO|YES], NO is default

###################################################################

# Disable all implicit rules
.SUFFIXES:

# I do not know better way to define "empty" strings
_EMPTY := 
_SPACE := $(_EMPTY) $(_EMPTY)
_TAB   := $(_SPACE)$(_SPACE)$(_SPACE)$(_SPACE)


## $(call define_iconf_var,variable[,default])
#
# Set ICONF_<variable> from config.h or given default value_ in case it is not already set
# Produce error if it can not be defined 
#
define get_iconf_var_config_h =
$(shell sed -e '{ /^#define $(1)/!d ; s/#define $(1) \(.*\)/\1/ ; s/[ "]//g}' < config.h)
endef

define set_iconf_var =
ICONF_$(1)_FROM := $(if $(ICONF_$(1)),explicit,$(if $(call get_iconf_var_config_h,$(1)),config.h,$(if $(2),default,$(error $(1) is not defined))))
ICONF_$(1) := $(if $(ICONF_$(1)),$(ICONF_$(1)),$(if $(call get_iconf_var_config_h,$(1)),$(call get_iconf_var_config_h,$(1)),$(2)))
endef

define define_iconf_var =
$(eval $(call set_iconf_var,$(1),$(2)))
endef

define print_iconf_var =
	$(info $(_TAB)ICONF_$(1) = '$(ICONF_$(1))' ($(ICONF_$(1)_FROM)))
endef

## $(call define_var,variable[,default])
#
# Set <variable> in in case it is not already defined to <default>, or produce error (in case <default> is empty)
#
define set_var =
$(1)_FROM := $(if $($(1)),explicit,$(if $(2),default,$(error $(1) is not defined)))
$(1) := $(if $($(1)),$($(1)),$(2))
endef

define define_var =
$(eval $(call set_var,$(1),$(2)))
endef

define print_var =
	$(info $(_TAB)$(1) = '$($(1))' ($($(1)_FROM)))
endef


## $(eval $(call mkdir,<dir>))
#
# Make directory creation rule
define mkdir =
$(1):
		@echo -n "Creating directory '$(1)' ... "
		@mkdir -p $(1)
		@echo Done.
endef

## $(eval $(call target,<target suffix>))
#
# Make target creation rule (target endet with .so is a shared library, executable otherwise)
define target =
$(IPT$(1)): $(IPT_DEPS$(1))
		@echo "Linking $(notdir $(IPT$(1))) ... "
		@$(CXX) $(LFLAGS$(1)) $(LFLAGSE$(1))\
      $(if $(filter %.so,$(IPT$(1))),-shared,)\
      $$(filter %.o,$$^) -o $$@\
      $(LIBS$(1)) $(LIBSE($1)) -Wl,-z,defs
		@echo -----------------------------------------------------
endef

## Auto-dependencies.
# Very good explanation how that works: 
#  http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
# That is GCC specific, note $$ is required since it is used in functions
DFLAGS = -MT $$@ -MMD -MP -MF

## $(eval $(call compile,<build suffix>))
#
# Make compile rules (for c and cpp)
define compile =
$(IPBOD)/$(if $(1),$(1:_%=%)/,)%.o: %.cpp $(IPBOD)/$(if $(1),$(1:_%=%/)).deps/%.d
		@echo "Compiling $(if $(1),$(1:_%=%)/,)$$(notdir $$@) ... "
		@$(CXX) $(CXXFLAGS$(1)) $(CXXFLAGSE$(1)) -c $$< -o $$@ $(DFLAGS) $$(@:%.o=%.d)
		@[ -e $$(dir $$(@))/.deps ] || mkdir $$(dir $$(@))/.deps
		@mv $$(@:%.o=%.d) $$(dir $$(@))/.deps/
		@touch $$@
$(IPBOD)/$(if $(1),$(1:_%=%)/,)%.o: %.c   $(IPBOD)/$(if $(1),$(1:_%=%/)).deps/%.d
		@echo "Compiling $(if $(1),$(1:_%=%)/,)$$(notdir $$@) ... "
		@$(CC) $(CFLAGS$(1)) $(CFLAGSE$(1)) -c $$< -o $$@ $(DFLAGS) $$(@:%.o=%.d)
		@[ -e $$(dir $$(@))/.deps ] || mkdir $$(dir $$(@))/.deps
		@mv $$(@:%.o=%.d) $$(dir $$(@))/.deps/
endef

## $(eval $(call flags,<suffix>))
#
# Exlicitly define all flags when they are not yet set
define flags =
  CFLAGS$(1) := $(if $(CFLAGS$(1)), $(CFLAGS$(1)), $(CFLAGS))
  CFLAGSE$(1) := $(if $(CFLAGSE$(1)), $(CFLAGSE$(1)), $(CFLAGSE))
  CXXFLAGS$(1) := $(if $(CXXFLAGS$(1)), $(CXXFLAGS$(1)), $(CXXFLAGS))
  CXXFLAGSE$(1) := $(if $(CXXFLAGSE$(1)), $(CXXFLAGSE$(1)), $(CXXFLAGSE))
  LFLAGS$(1) := $(if $(LFLAGS$(1)), $(LFLAGS$(1)), $(LFLAGS))
  LFLAGSE$(1) := $(if $(LFLAGSE$(1)), $(LFLAGSE$(1)), $(LFLAGSE))
  LIBS$(1) := $(if $(LIBS$(1)), $(LIBS$(1)), $(LIBS))
  LIBSE$(1) := $(if $(LIBSE$(1)), $(LIBSE$(1)), $(LIBSE))
endef

## $(call allflags,<suffix>
#
# All compiling flags for suffix, use for IPCONFIG
define allflags =
	$(CFLAGS$(1)) $(CFLAGSE$(1)) $(CXXFLAGS$(1)) $(CXXFLAGSE$(1)) $(LFLAGS$(1)) $(LFLAGSE$(1)) $(LIBS$(1)) $(LIBSE$(1))
endef

## $(eval $(call tprefix,<target suffix>))
#
# Prefix IPBD to targets and not object dependencies, IPOD to object dependencies
define tprefix =
  IPT$(1) := $(IPBD)/$(IPT$(1))
  IPT_DEPS$(1) := $(addprefix $(IPBD)/,$(filter-out %.o,$(IPT_DEPS$(1)))) $(addprefix $(IPBOD)/,$(filter %.o,$(IPT_DEPS$(1))))
endef

## $(eval $(all addflagtoall,<flag>,<value>))
#
# Add <value> to all currently defined <flag> (with prefixes) (IPSS should be already defined before the call)
define addflagtoall =
  $(1) += $(2)
  $(foreach _sx,$(IPSS),$(if $($(1)_$(_sx)),$(1)_$(_sx) += $(2)))
endef

## $(eval $(call dep_dcmi,<pkg>,<subsystem>))
#
# Build build for dependency package using download/configure/make/make install sequence
#   pkg - package name
#   subsystem - IPLUG or IGRAPHICS
#
# The following variables must be defined:
#   _<pkg>_TARGET   - target name in the install path
#   _<pkg>_SRC      - source (XXX.YYY) file name
#   _<pkg>_DIR      - the directory name when the source file is extracted
#   _<pkg>_NET_SRC  - network source file URL
#   _<pkg>_CFG      - options (apart from PREFIX) for configure (optional)
#   _<pkg>_CFG_ENV  - environment for configure (optional)
define dep_dcmi =

IPB_INST_DEPS := $(_IDEPS_INSTALL_PATH)/$(_$(1)_TARGET) $(IPB_INST_DEPS)

$(_$(2)_DEPS_PATH)/$(_$(1)_SRC):
		@echo "Downloading $(1)... "
		@cd $(_$(2)_DEPS_PATH) && curl -s -S -L $(_$(1)_NET_SRC) -o $(_$(1)_SRC)

$(_IDEPS_INSTALL_PATH)/$(_$(1)_TARGET): $(_$(2)_DEPS_PATH)/$(_$(1)_SRC)
		@echo "Compiling $(1)... "
		@rm -rf $(_IDEPS_INSTALL_PATH)/tmp
		@mkdir $(_IDEPS_INSTALL_PATH)/tmp
		@cd $(_IDEPS_INSTALL_PATH)/tmp && tar $(if $(filter %.tar.gz,$(_$(1)_SRC)),xzf,$(if $(filter %.tar.xz,$(_$(1)_SRC)),xJf,)) $(_$(2)_DEPS_PATH)/$(_$(1)_SRC)
		@cd $(_IDEPS_INSTALL_PATH)/tmp/$(_$(1)_DIR) && PKG_CONFIG_PATH=$(_IDEPS_INSTALL_PATH)/lib/pkgconfig $(_$(1)_CFG_ENV) ./configure --prefix=$(_IDEPS_INSTALL_PATH) $(_$(1)_CFG) && make && make install
		#@rm -rf $(_IDEPS_INSTALL_PATH)/tmp
endef

###################################################################


# Current working directory, shoud be plug-in directory
#override IP_ROOT := .
# Geany can not open files when it is set to .
override IP_ROOT := $(abspath .)

# Path to the config.h
override IP_CONFIG_H := $(IP_ROOT)/config.h

#
# Check we are in correct directory, 
#
ifeq ("$(wildcard $(IP_CONFIG_H))","")
 	$(error FATAL: $(IP_CONFIG_H) is not found. Make should be started from plug-in directory.)
endif

$(call define_iconf_var,PLUG_NAME)

#
# Check we know iPlug 2 root
#
ifeq ($(IROOT),)
  # In case it is not defined explicity
  ifneq ("$(wildcard $(IP_ROOT)/../IPlug)","")
    # we are in iPlug2/Examples/
    $(call define_var,IROOT,$(abspath $(IP_ROOT)/..))
  else
    # we are somewhere else, check Makefile is in iPlug2/Examples
    _MAKEFILE_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
    _MAKEFILE_DIR := $(_MAKEFILE_DIR:%/=%)
    ifneq ("$(wildcard $(_MAKEFILE_DIR)/../IPlug)","")
      $(call define_var,IROOT,$(abspath $(_MAKEFILE_DIR)/..))
    endif
  endif
endif

ifeq ("$(wildcard $(IROOT)/IPlug)","")
 	$(error FATAL: iPlug2 could not be found in '$(IROOT)'. This makefile should be in Examples or IROOT should be defined explicitly)
endif

ifneq ($(IDEBUG),YES)
  override IDEBUG :=
else
	override IDEBUG := -dbg
endif

#
# Build directory
#
$(call define_var,IARCH,$(shell uname -m))
$(call define_var,IBUILD,../Build/Linux-$(IARCH)$(IDEBUG)/$(ICONF_PLUG_NAME))

#
# Clarify target
#
ifneq ($(MAKECMDGOALS),)
  override ITARGET := $(MAKECMDGOALS)
endif
ifeq ($(ITARGET),CLEAN)
  override IOP := CLEAN
  ITARGET_TYPE_STR := build directory
else ifeq ($(ITARGET),APP)
else ifeq ($(ITARGET),)
  override ITARGET := APP
else ifeq ($(ITARGET),VST2)
else ifeq ($(ITARGET),VST3)
else ifeq ($(ITARGET),LV2)
else
  $(error FATAL: not supported target '$(ITARGET)' (supported are APP, VST3 and LV2, case sensitive))
endif

#
# Check operation and construct build
#
.DEFAULT_GOAL := $(ITARGET)
ifeq ($(IOP),)
	IOP := BUILD
endif
ifneq ($(IOP),BUILD)
  ifneq ($(IOP),CLEAN)
    $(error FATAL: unknown operation '$(IOP)' (supported are CLEAN and BUILD, case sensitive))
  endif
else
  $(info Building '$(ICONF_PLUG_NAME)' $(ITARGET)/$(IARCH)$(IDEBUG) in: $(shell pwd) ...)

  # Project Build Directory
	IPBD := $(IBUILD)/$(ITARGET)

  # Project build objects directory
  IPBOD := $(IPBD)/.obj
  
  # Project build resources directory (relative to IPBD)
  IPBRD := resources

  # We can have additionall Buid Suffixes for different sets of object files (excluding target files).
  # We define a set of suffixes with IPBSS (project build suffixes), which can be empty (default)
  # A separate set of rules can be defined for these extra sets, object files will be generated
  # in a separate for the set directory. A target dependency can be constructed from different sets.
  # Relevant options (f.e CFLAGSE_<suffix>) can be defined, but assumed to be corresponding general options when not.
  # Build and Target suffixes share name space and so options.
  IPBSS :=

  # Project target (must be defined, relative to IPBD)
  IPT :=
  # Project target dependencies (must be defined, object files should be relative to IPBOD, other files with complete path)
  IPT_DEPS :=

	# We also can have several Targets, they are specified with Target Suffixes. 
  # For all of them  IPT_<suffix> and IPT_DEPS_<suffix> must be defined.
  # Build and Target suffixes share name space and so options.
  IPTSS :=
  
  # The list of extra directories, which can not be "deducted" automatically
  # from build, target and resources
  # Project build extra directories
  IPBED :=
  
  # The list of extra pre-requisites, like dependency packages
  IPB_DEPS :=
  
  # The list of dependecy packages installation rules
  IPB_INST_DEPS :=

  ### Used during building, see later which of them are automatically defined
  #   from other when not set explicitly.
  #   xxE(xtra) flags are to make simple (base + something) options, especially
  #              in case of build and/or target suffixes
  #   yy_<suffix> are used for target/build suffixes
  #   CFLAGS, CXXFLAGS, LFLAGS, LIBS
  
  ### Object are added to deps, but packages should define frome where they come
  IPSRC_DIR := $(IP_ROOT)
  # IPSRC_DIR will be auto-included into IPINC_DIR
  IPINC_DIR := $(IP_ROOT)/resources
  

  # Target specific
  ifeq ($(ITARGET),APP)
    IPT := $(ICONF_PLUG_NAME)    

    # we assume all cpp files are relevant
    _SRC := $(wildcard *.cpp)
    IPT_DEPS += $(notdir $(_SRC:%.cpp=%.o))

  else ifeq ($(ITARGET),VST3)
    # since VST 3.6.10 there is specific structure for VST3 "bundle"
    IPB_VST3 := $(ICONF_PLUG_NAME).vst3/Contents
    IPBRD := $(IPB_VST3)/Resources
    IPT := $(IPB_VST3)/$(IARCH)-linux/$(ICONF_PLUG_NAME).so

    # we assume all cpp files are relevant
    _SRC := $(wildcard *.cpp)
    IPT_DEPS += $(notdir $(_SRC:%.cpp=%.o))

  else ifeq ($(ITARGET),VST2)
    IPT := $(ICONF_PLUG_NAME).so

    # we assume all cpp files are relevant
    _SRC := $(wildcard *.cpp)
    IPT_DEPS += $(notdir $(_SRC:%.cpp=%.o))
  else ifeq ($(ITARGET),LV2)
    # UI and CFG are separate targets
    IPTSS += CFG
    IPBSS += CFG
    ifneq ($(IGRAPHICS),NO_IGRAPHICS)
			IPTSS += UI
			IPBSS += UI
		endif

    # DSP (default)
    IPT := $(ICONF_PLUG_NAME).so

	  # Configuration
	  IPT_CFG := .obj/cfg

    # UI
    IPT_UI := $(ICONF_PLUG_NAME)_ui.so

    # we assume all cpp files are relevant, but may have different compilation flags
    _SRC := $(wildcard *.cpp)
    IPT_DEPS += $(notdir $(_SRC:%.cpp=%.o))
    
  endif
endif


########################################################################

.PHONY = $(ITARGET)

# No need and error prone to define rules in case of cleaning
ifneq ($(IOP),CLEAN)

# fill all requried for used packages options
include $(IROOT)/common-linux.mk

# Constuct the list of all suffixes (Target and Build, without duplicates)
IPSS := $(sort $(IPTSS) $(IPBSS))

# vpath for all source directories
vpath %.cpp $(IPSRC_DIR)
vpath %.c   $(IPSRC_DIR)

IPINC_DIR += $(IPSRC_DIR)
IPINC_DIR += $(_IDEPS_INSTALL_PATH)/include

# add includes
_INCS := $(addprefix -I,$(IPINC_DIR))
$(eval $(call addflagtoall,CFLAGS,$(_INCS)))
$(eval $(call addflagtoall,CXXFLAGS,$(_INCS)))
# library dirs
_LIBS := $(addprefix -L,$(_IDEPS_INSTALL_PATH)/lib)
$(eval $(call addflagtoall,LFLAGS,$(_LIBS)))

#
# Check IPCONFIG is not changed, clean build directory otherwise
#

# IPCONFIG is a set of all compiling options
IPCONFIG := $(call allflags) $(foreach _sx,$(IPSS),$(_sx) $(call allflags,_$(_sx)))

# The last
IPCONFIG_OLD = $(shell cat $(IPBD)/config.build 2>/dev/null)
ifneq ($(IPCONFIG_OLD),$(IPCONFIG))
  $(info Configuration is changed, compiling from scratch)
  #$(info '->$(IPCONFIG)')
  #$(info '<-$(IPCONFIG_OLD)')
  $(shell rm -rf $(IPBD) >/dev/null)
endif

# Explicitly set all options implicits
CXXFLAGS := $(if $(CXXFLAGS), $(CXXFLAGS), $(CFLAGS))
LFLAGS := $(if $(LFLAGS), $(LFLAGS), $(CXXFLAGS))
$(foreach _sx,$(IPSS),$(eval $(call flags,_$(_sx))))

# Add IPBD prefix to targets and resources, IPBOD to target dependencies
#$(info $(call tprefix))
$(eval $(call tprefix))
$(foreach _tsx,$(IPTSS),$(eval $(call tprefix,_$(_tsx))))
IPBRD := $(IPBD)/$(IPBRD)

# construct targets list
IPT_LIST := $(IPT) $(foreach _tsx,$(IPTSS),$(IPT_$(_tsx)))
#$(info $(IPT_LIST))

# Find all build directories we may need, including
#  Build suffixes
IPBED += $(IPBOD) $(addprefix $(IPBOD)/, $(IPBSS))
#  Resources
IPBED += $(dir $(IPBRD))
#  Target(s)
IPBED += $(dir $(IPT))  $(foreach _tsx,$(IPTSS),$(dir $(IPT_$(_tsx))))
# There can be duplicates and parent directories, duplicates we should remove (to avoid double rules)
IPBED := $(sort $(IPBED))

#### ITARGET rule
$(ITARGET): $(IPB_INST_DEPS) $(IPBED) $(IPBD)/config.build $(IPBRD) $(IPB_DEPS) $(IPT_LIST)

### IPCONFIG rule
$(IPBD)/config.build:
		@echo -n "Saving build configuration... "
		@echo -n "$(IPCONFIG)" > $@
		@echo Done.

### Directory creation rules
$(foreach _dir,$(IPBED),$(eval $(call mkdir, $(_dir))))

### Resource linking rule
$(IPBRD):	$(abspath $(IP_ROOT)/resources)
		@echo -n "Symlinking resources to build directory... "
		@ln -s $< $@
		@echo Done.

### Target rules
$(eval $(call target))
#$(info $(call target))
$(foreach _tsx,$(IPTSS),$(eval $(call target,_$(_tsx))))
#$(foreach _tsx,$(IPTSS),$(info $(call target,_$(_tsx))))

### Compile rules
$(eval $(call compile))
#$(info $(call compile))
$(foreach _bsx,$(IPBSS),$(eval $(call compile,_$(_bsx))))
#$(foreach _bsx,$(IPBSS),$(info $(call compile,_$(_bsx))))


### Dependencies

# compose the list of all object files
IPOBJS := $(sort $(filter %.o,$(IPT_DEPS)) $(foreach _tsx, $(IPTSS),$(filter %.o,$(IPT_DEPS_$(_tsx)))))
# $(info $(IPOBJS))

# all resulting deps files
IPODEPS := $(foreach _f,$(IPOBJS),$(dir $(_f)).deps/$(notdir $(_f:%.o=%.d)))
#$(info $(IPODEPS))

# pattern rules will be happy if it does not exist yet
$(IPODEPS):

# include dependencies, when exist
include $(wildcard $(IPODEPS))

##########################################################
# cleaning
else 
$(ITARGET):
ifeq ($(ITARGET),clean)
		@echo -n "Cleaning $(IBUILD)... "
		@rm -rf $(IBUILD)
		@echo Done.
else
		@echo -n "Cleaning $(IBUILD)/$(ITARGET)... "
		@rm -rf $(IBUILD)/$(ITARGET)
		@echo Done.
endif
endif
