SUBDIRS = . tests
if MAKE_DOC
SUBDIRS += doc
endif

AM_CXXFLAGS = @visibility@

lib_LTLIBRARIES = %D%/librtaudio.la
%C%_librtaudio_la_LDFLAGS = -no-undefined -export-dynamic -version-info @SO_VERSION@
%C%_librtaudio_la_SOURCES = \
  %D%/RtAudio.cpp \
  %D%/rtaudio_c.cpp

if ASIO
%C%_librtaudio_la_SOURCES += \
	include/asio.cpp \
	include/asiodrivers.cpp \
	include/asiolist.cpp \
	include/iasiothiscallresolver.cpp
endif

rtaudio_incdir = $(includedir)/rtaudio
rtaudio_inc_HEADERS = \
  %D%/RtAudio.h \
  %D%/rtaudio_c.h

pkgconfigdatadir = $(libdir)/pkgconfig
pkgconfigdata_DATA = rtaudio.pc

EXTRA_DIST = autogen.sh readme install.txt contrib include
