ARCH := $(shell uname -m)
UNAME_S := $(shell uname -s)

PLUGIN_CFLAGS = -fPIC -DWDL_FFT_REALSIZE=8 -DSWELL_PROVIDED_BY_APP -DDYNAMIC_LICE -DFORCE_WIN32_CURSES
ifeq ($(UNAME_S), Darwin)
  TARGET=jsfx.dylib
  SWELL_MODSTUB=swell-modstub.o
  LFLAGS=-ldl -framework Cocoa -shared
else
  TARGET=jsfx.so
  SWELL_MODSTUB=swell-modstub-generic.o
  LFLAGS=-ldl -shared
endif

ifdef DEBUG
  CFLAGS=-O0 -g -D_DEBUG -DWDL_CHECK_FOR_NON_UTF8_FOPEN
else
  CFLAGS=-O2 -DNDEBUG
  ifneq ($(UNAME_S), Darwin)
    CFLAGS+= -s
  endif
endif

CFLAGS += -fvisibility=hidden -fstrict-aliasing -fno-math-errno -Wall -Wtype-limits -Wno-unused-parameter -Wno-sign-compare

ifneq ($(UNAME_S), Darwin)
  CFLAGS += -Wno-format-truncation -D_FILE_OFFSET_BITS=64
else
  CFLAGS += -Wno-deprecated
endif

CXXFLAGS := $(CFLAGS) -std=c++03

override CFLAGS += $(PLUGIN_CFLAGS)
override CXXFLAGS += $(PLUGIN_CFLAGS)

default: $(TARGET)

vpath %.cpp ../WDL/swell ../WDL/wingui ../WDL/win32_curses
vpath %.c ../WDL/eel2

EEL_OBJS = nseel-lextab.o nseel-cfunc.o nseel-caltab.o nseel-compiler.o nseel-eval.o nseel-ram.o nseel-yylex.o

SWELL_OBJS = $(SWELL_MODSTUB)

ifeq ($(UNAME_S), Darwin)
  vpath %.mm ../WDL/swell

%.o : %.mm
	$(CXX) -ObjC++ $(CXXFLAGS) -c -o $@ $<

  ASM_FMT = macho64
  NASM_OPT = --prefix _

../WDL/eel2/asm-nseel-x64-sse.o: ../WDL/eel2/asm-nseel-x64-sse.asm
	nasm -D AMD64ABI -f $(ASM_FMT) $(NASM_OPT) -o $@ $^

else
  ASM_FMT = elf64
  NASM_OPT =

../WDL/eel2/asm-nseel-x64-sse.o: ../WDL/eel2/asm-nseel-x64-sse.asm
	nasm -D AMD64ABI -f elf64 -o $@ $^

endif

ifeq ($(ARCH),x86_64)
  EEL_OBJS += ../WDL/eel2/asm-nseel-x64-sse.o
endif

JSFX_OBJS = effectload.o effectproc.o jsfx_api.o miscfunc.o sfxui.o wndsize.o
JSFX_OBJS += sfx_edit.o curses_win32.o eel_edit.o curses_editor.o

res.rc_mac_dlg:
	../WDL/swell/swell_resgen.pl --quiet res.rc

sfxui.o: ../WDL/eel2/eel_lice.h
jsfx_api.o: res.rc res.rc_mac_dlg
effectproc.o: ../WDL/eel2/eel_lice.h

ifneq ($(TARGET),)

$(TARGET): $(SWELL_OBJS) $(JSFX_OBJS) $(EEL_OBJS) $(OBJS_STATIC)
	$(CXX) -o $@ $(CXXFLAGS) $(LFLAGS) $^

endif


clean:
	-rm -f -- $(TARGET) $(JSFX_OBJS) $(SWELL_OBJS) $(EEL_OBJS) res.rc_mac_menu res.rc_mac_dlg

.PHONY: clean default
