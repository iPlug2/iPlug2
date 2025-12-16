
noinst_PROGRAMS = audioprobe playsaw playraw record duplex apinames testall teststops

AM_CXXFLAGS = -Wall -I$(top_srcdir)

audioprobe_SOURCES = audioprobe.cpp
audioprobe_LDADD = $(top_builddir)/librtaudio.la

playsaw_SOURCES = playsaw.cpp
playsaw_LDADD = $(top_builddir)/librtaudio.la

playraw_SOURCES = playraw.cpp
playraw_LDADD = $(top_builddir)/librtaudio.la

record_SOURCES = record.cpp
record_LDADD = $(top_builddir)/librtaudio.la

duplex_SOURCES = duplex.cpp
duplex_LDADD = $(top_builddir)/librtaudio.la

apinames_SOURCES = apinames.cpp
apinames_LDADD = $(top_builddir)/librtaudio.la

testall_SOURCES = testall.cpp
testall_LDADD = $(top_builddir)/librtaudio.la

teststops_SOURCES = teststops.cpp
teststops_LDADD = $(top_builddir)/librtaudio.la

EXTRA_DIST = Windows CMakeLists.txt

TESTS = apinames
