##LINUX ONLY FOR NOW

# APP=IPlugEffect
# 
# WDL_PATH = ../../WDL
# vpath %.cpp .. $(WDL_PATH)/swell
# vpath %.cpp .. $(WDL_PATH)/lice
# vpath %.cpp .. app_wrapper
# 
# ### Objects and resources
# SWELL_OBJS = swell.o swell-ini.o swell-miscdlg-generic.o swell-wnd-generic.o \
#              swell-menu-generic.o swell-kb-generic.o swell-dlg-generic.o \
#              swell-gdi-generic.o swell-misc-generic.o swell-gdi-lice.o \
#              swell-generic-headless.o swell-generic-gdk.o \
#              swell-appstub-generic.o swell-modstub-generic.o
# 
# LICE_OBJS = lice.o lice_arc.o lice_colorspace.o lice_line.o lice_text.o \
#             lice_textnew.o lice_png.o
# 
# OBJS = app_main.o main_dialog.o $(SWELL_OBJS) $(LICE_OBJS)
# RESFILES = resources/main.rc_mac_dlg resources/main.rc_mac_menu
# 
# ### Compiler/Linker Flags
# CFLAGS += -pipe -fvisibility=hidden -fno-math-errno -fPIC -DPIC -Wall -Wshadow -Wno-unused-function -Wno-multichar -Wno-unused-result -Werror -Wno-deprecated-declarations
# CFLAGS += -O0 -g -D_DEBUG
# LFLAGS = -lpthread -ldl -lX11 -lXi
# ### GDK
# CFLAGS += -DSWELL_TARGET_GDK=2 $(shell pkg-config gdk-2.0 --cflags)
# LFLAGS += $(shell pkg-config gdk-2.0 --libs)
# ### Freetype
# CFLAGS += -DSWELL_LICE_GDI -DSWELL_FREETYPE $(shell freetype-config --cflags)
# LFLAGS += $(shell freetype-config --libs)
# 
# CXXFLAGS = $(CFLAGS)
# 
# 
# ### Targets
# .PHONY: clean run
# default: $(APP)
# 
# $(RESFILES): resources/IPlugEffect.rc
# 	php $(WDL_PATH)swell/mac_resgen.php $^
# 
# $(APP): $(RESFILES) $(OBJS)
# 	$(CXX) -o $@ $(CFLAGS) $(OBJS) $(LFLAGS)
# 
# 
# run: $(APP)
# 	./$<
# clean:
# 	-rm $(APP) $(OBJS) $(RESFILES)

ROOT = ../..
TARGET = ./build-web/IPlugEffect-WAM.js
API = $(ROOT)/Dependencies/IPlug/WAM_SDK/wamsdk
WDL_PATH = $(ROOT)/WDL
IPLUG_PATH = $(ROOT)/IPlug

SRC = \
IPlugEffect.cpp \
$(API)/processor.cpp \
$(IPLUG_PATH)/IPlugWAM.cpp \
$(IPLUG_PATH)/IPlugBase.cpp \
$(IPLUG_PATH)/IPlugParameter.cpp

EXPORTS = "[\
	'_createModule','_wam_init','_wam_terminate','_wam_resize', \
	'_wam_onprocess', '_wam_onmidi', '_wam_onsysex', '_wam_onparam', \
	'_wam_onmessageN', '_wam_onmessageS', '_wam_onpatch' \
	]"

CFLAGS = -I./ -I$(API) -I$(WDL_PATH) -I$(IPLUG_PATH) -DWAM_API -std=c++11
LDFLAGS = -O2 -s EXPORTED_FUNCTIONS=$(EXPORTS)
JSFLAGS = -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s EXPORT_NAME="'AudioWorkletGlobalScope.WAM'"

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(JSFLAGS) -o $@ $(SRC)