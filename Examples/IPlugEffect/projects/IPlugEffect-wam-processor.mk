include ./config/IPlugEffect-web.mk

TARGET = ./build-web/scripts/IPlugEffect-WAM.js

EXPORTS = "[\
  '_createModule','_wam_init','_wam_terminate','_wam_resize', \
  '_wam_onprocess', '_wam_onmidi', '_wam_onsysex', '_wam_onparam', \
  '_wam_onmessageN', '_wam_onmessageS', '_wam_onmessageA', '_wam_onpatch' \
  ]"

LDFLAGS = $(WAM_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(EXPORTS) \
-s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'setValue', 'Pointer_stringify']"

JSFLAGS = \
-s BINARYEN_ASYNC_COMPILATION=0 \
-s ALLOW_MEMORY_GROWTH=1 \
-s EXPORT_NAME="'AudioWorkletGlobalScope.WAM.IPlug'" \
-s ASSERTIONS=0 \
--bind

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(WAM_CFLAGS) $(LDFLAGS) $(JSFLAGS) -o $@ $(WAM_SRC)
