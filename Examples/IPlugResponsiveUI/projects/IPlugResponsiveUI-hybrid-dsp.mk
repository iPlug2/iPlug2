include ../config/IPlugResponsiveUI-web-hybrid.mk

TARGET = ../build-web-hybrid/scripts/IPlugResponsiveUI-dsp.js

SRC += $(HYBRID_DSP_SRC)
CFLAGS += $(HYBRID_DSP_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(HYBRID_DSP_LDFLAGS) \
-s EXPORTED_FUNCTIONS=$(HYBRID_DSP_EXPORTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
