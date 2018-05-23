include ./config/IPlugEffect-web.mk

TARGET = ./build-web/scripts/IPlugEffect-WAM.js

SRC = $(WAM_SRC) IPlugEffect.cpp

EXPORTS = "[\
	'_createModule','_wam_init','_wam_terminate','_wam_resize', \
	'_wam_onprocess', '_wam_onmidi', '_wam_onsysex', '_wam_onparam', \
	'_wam_onmessageN', '_wam_onmessageS', '_wam_onmessageA', '_wam_onpatch' \
	]"

CFLAGS = \
-I$(PROJECT_ROOT) \
-I$(WAM_SDK_PATH) \
-I$(WDL_PATH) \
-I$(IPLUG_PATH) \
-I$(IPLUG_EXTRAS_PATH) \
-I$(IPLUG_WEB_PATH) \
-DWAM_API \
-DNO_IGRAPHICS \
-DSAMPLE_TYPE_FLOAT \
-DIPLUG_DSP=1 \
-std=c++11  \
-Wno-bitwise-op-parentheses

LDFLAGS = \
-s EXPORTED_FUNCTIONS=$(EXPORTS) \
-O2

JSFLAGS = \
-s BINARYEN_ASYNC_COMPILATION=0 \
-s ALLOW_MEMORY_GROWTH=1 \
-s WASM=1 \
-s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlug'" \
-s ASSERTIONS=0 \
--bind

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(JSFLAGS) -o $@ $(SRC)
