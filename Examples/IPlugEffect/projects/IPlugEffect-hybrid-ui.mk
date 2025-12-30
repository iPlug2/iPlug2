include ../config/IPlugEffect-web-hybrid.mk

TARGET = ../build-web-hybrid/scripts/IPlugEffect-ui.js

SRC += $(HYBRID_UI_SRC)
CFLAGS += $(HYBRID_UI_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(HYBRID_UI_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(HYBRID_UI_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
