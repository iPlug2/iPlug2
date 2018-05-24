include ./config/IPlugEffect-web.mk

TARGET = ./build-web/scripts/IPlugEffect.js

EXPORTS = "['_main']"

LDFLAGS = $(WEB_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(EXPORTS) \

JSFLAGS = -s BINARYEN_ASYNC_COMPILATION=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s WASM=1 \
-s ASSERTIONS=0 \
-s FORCE_FILESYSTEM=1 \
--bind

$(TARGET): $(OBJECTS)
	$(CC) $(WEB_CFLAGS) $(LDFLAGS) $(JSFLAGS) -o $@ $(WEB_SRC)
