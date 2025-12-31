include ../config/IPlugVisualizer-wasm.mk

TARGET = ../build-web-wasm/scripts/IPlugVisualizer-dsp.js

SRC += $(WASM_DSP_SRC)
CFLAGS += $(WASM_DSP_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(WASM_DSP_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(WASM_DSP_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
