include ../config/IPlugSurroundEffect-wasm.mk

TARGET = ../build-web-wasm/scripts/IPlugSurroundEffect-ui.js

SRC += $(WASM_UI_SRC)
CFLAGS += $(WASM_UI_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(WASM_UI_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(WASM_UI_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
