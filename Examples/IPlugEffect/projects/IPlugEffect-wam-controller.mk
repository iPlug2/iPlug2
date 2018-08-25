include ./config/IPlugEffect-web.mk

TARGET = ./build-web/scripts/IPlugEffect.js

SRC += $(WEB_SRC)
CFLAGS += $(WEB_CFLAGS)
LDFLAGS += $(WEB_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(WEB_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
