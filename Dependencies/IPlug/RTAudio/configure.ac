# Process this file with autoconf to produce a configure script.
AC_INIT(RtAudio, 5.1.0, gary@music.mcgill.ca, rtaudio)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(RtAudio.cpp)
AC_CONFIG_FILES([rtaudio.pc Makefile tests/Makefile doc/Makefile doc/doxygen/Doxyfile])
AM_INIT_AUTOMAKE([1.14 -Wall -Werror foreign subdir-objects])

# libtool version: current:revision:age
#
# If the library source code has changed at all since the last update, then
# increment revision (`c:r:a' becomes `c:r+1:a').
#
# If any interfaces have been added, removed, or changed since the last update,
# increment current, and set revision to 0.
#
# If any interfaces have been added since the last public release, then
# increment age.
#
# If any interfaces have been removed since the last public release, then set
# age to 0.
m4_define([lt_current], 6)
m4_define([lt_revision], 1)
m4_define([lt_age], 0)

m4_define([lt_version_info], [lt_current:lt_revision:lt_age])
m4_define([lt_current_minus_age], [m4_eval(lt_current - lt_age)])

SO_VERSION=lt_version_info
AC_SUBST(SO_VERSION)
AC_SUBST(api)
AC_SUBST(req)
AC_SUBST(visibility)

api=""
req=""
use_asio=""


# configure flags
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable various debug output])])
AC_ARG_WITH(jack, [AS_HELP_STRING([--with-jack], [choose JACK server support])])
AC_ARG_WITH(alsa, [AS_HELP_STRING([--with-alsa], [choose native ALSA API support (linux only)])])
AC_ARG_WITH(pulse, [AS_HELP_STRING([--with-pulse], [choose PulseAudio API support (unixes)])])
AC_ARG_WITH(oss, [AS_HELP_STRING([--with-oss], [choose OSS API support (unixes)])])
AC_ARG_WITH(core, [AS_HELP_STRING([--with-core], [choose CoreAudio API support (mac only)])])
AC_ARG_WITH(asio, [AS_HELP_STRING([--with-asio], [choose ASIO API support (win32 only)])])
AC_ARG_WITH(ds, [AS_HELP_STRING([--with-ds], [choose DirectSound API support (win32 only)])])
AC_ARG_WITH(wasapi, [AS_HELP_STRING([--with-wasapi], [choose Windows Audio Session API support (win32 only)])])

# Check version number coherency between RtAudio.h and configure.ac
AC_MSG_CHECKING([that version numbers are coherent])
RTAUDIO_VERSION=`sed -n 's/#define RTAUDIO_VERSION "\(.*\)"/\1/p' $srcdir/RtAudio.h`
AS_IF([test "x$RTAUDIO_VERSION" != "x$PACKAGE_VERSION"],[
   AC_MSG_RESULT([no])
   AC_MSG_FAILURE([testing RTAUDIO_VERSION==PACKAGE_VERSION failed, check that RtAudio.h defines RTAUDIO_VERSION as "$PACKAGE_VERSION" or that the first line of configure.ac has been updated.])
   ],[
   AC_MSG_RESULT([yes])
])
# Enable some nice automake features if they are available
m4_ifdef([AM_MAINTAINER_MODE], [AM_MAINTAINER_MODE])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Fill GXX with something before test.
GXX="no"
# if the user did not provide any CXXFLAGS, we can override them
AS_IF([test "x$CXXFLAGS" = "x" ], [override_cxx=yes], [override_cxx=no])
AS_IF([test "x$CFLAGS" = "x" ], [override_c=yes], [override_c=no])

# Checks for programs.
AC_PROG_CXX(g++ CC c++ cxx)
AM_PROG_AR
AC_PATH_PROG(AR, ar, no)
AS_IF([test "x${AR}" = "xno" ], [
    AC_MSG_ERROR([Could not find ar - needed to create a library])
])

# Initialize libtool
LT_INIT([win32-dll])
AC_CONFIG_MACRO_DIR([m4])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/ioctl.h unistd.h)

# Check compiler and use -Wall if gnu
AS_IF([test "x${GXX}" = "xyes" ], [
  CXXFLAGS="-Wall -Wextra ${CXXFLAGS}"
  AS_IF([ test "x${enable_debug}" = "xyes" ], [
    # Add -Werror in debug mode (except unused-function)
    CXXFLAGS="-Werror -Wno-error=unused-function ${CXXFLAGS}"
  ], [
    # hide private symbols in non-debug mode
    visibility="-fvisibility=hidden"
  ])
])

# Check for debug
AC_MSG_CHECKING([whether to compile debug version])
debugflags=""
AS_CASE([${enable_debug}],
  [ yes ], [
    AC_MSG_RESULT([yes])
    AC_DEFINE([__RTAUDIO_DEBUG__])
    debugflags="${debugflags} -g -O0"
    object_path=Debug
  ],
  [ no ], [
    AC_MSG_RESULT([no!])
    debugflags="${debugflags} -O3"
  ], [
    AC_MSG_RESULT([no])
  ])

# For debugging and optimization ... overwrite default because it has both -g and -O2
AS_IF([test "x$debugflags" != x],
  AS_IF([test "x$override_cxx" = "xyes" ], CXXFLAGS="$CXXFLAGS $debugflags", CXXFLAGS="$debugflags $CXXFLAGS")
  AS_IF([test "x$override_c" = "xyes" ], CFLAGS="$CFLAGS $debugflags", CFLAGS="$debugflags $CFLAGS")
  )


# Checks for functions
AC_CHECK_FUNC(gettimeofday, [cppflag="$cppflag -DHAVE_GETTIMEOFDAY"], )

# Checks for doxygen
AC_CHECK_PROG( DOXYGEN, [doxygen], [doxygen] )
AM_CONDITIONAL( MAKE_DOC, [test "x${DOXYGEN}" != x ] )

# Copy doc files to build dir if necessary
AC_CONFIG_LINKS( [doc/release.txt:doc/release.txt] )
AC_CONFIG_LINKS( [doc/doxygen/footer.html:doc/doxygen/footer.html] )
AC_CONFIG_LINKS( [doc/doxygen/error.txt:doc/doxygen/error.txt] )
AC_CONFIG_LINKS( [doc/doxygen/tutorial.txt:doc/doxygen/tutorial.txt] )
AC_CONFIG_LINKS( [doc/doxygen/compiling.txt:doc/doxygen/compiling.txt] )
AC_CONFIG_LINKS( [doc/doxygen/acknowledge.txt:doc/doxygen/acknowledge.txt] )
AC_CONFIG_LINKS( [doc/doxygen/license.txt:doc/doxygen/license.txt] )
AC_CONFIG_LINKS( [doc/doxygen/header.html:doc/doxygen/header.html] )
AC_CONFIG_LINKS( [doc/doxygen/duplex.txt:doc/doxygen/duplex.txt] )
AC_CONFIG_LINKS( [doc/doxygen/settings.txt:doc/doxygen/settings.txt] )
AC_CONFIG_LINKS( [doc/doxygen/probe.txt:doc/doxygen/probe.txt] )
AC_CONFIG_LINKS( [doc/doxygen/playback.txt:doc/doxygen/playback.txt] )
AC_CONFIG_LINKS( [doc/doxygen/multi.txt:doc/doxygen/multi.txt] )
AC_CONFIG_LINKS( [doc/doxygen/recording.txt:doc/doxygen/recording.txt] )
AC_CONFIG_LINKS( [doc/doxygen/apinotes.txt:doc/doxygen/apinotes.txt] )
AC_CONFIG_LINKS( [doc/images/mcgill.gif:doc/images/mcgill.gif] )
AC_CONFIG_LINKS( [doc/images/ccrma.gif:doc/images/ccrma.gif] )

# Checks for package options and external software
AC_CANONICAL_HOST

# Aggregate options into a single string.
AS_IF([test "x$with_jack"   = "xyes"], [systems="$systems jack"])
AS_IF([test "x$with_alsa"   = "xyes"], [systems="$systems alsa"])
AS_IF([test "x$with_pulse"  = "xyes"], [systems="$systems pulse"])
AS_IF([test "x$with_oss"    = "xyes"], [systems="$systems oss"])
AS_IF([test "x$with_core"   = "xyes"], [systems="$systems core"])
AS_IF([test "x$with_asio"   = "xyes"], [systems="$systems asio"])
AS_IF([test "x$with_dsound" = "xyes"], [systems="$systems dsound"])
AS_IF([test "x$with_wasapi" = "xyes"], [systems="$systems wasapi"])
required=" $systems "

# If none, assign defaults if any are known for this OS.
# User must specified with-* options for any unknown OS.
AS_IF([test "x$systems" = "x"],
  AS_CASE([$host],
    [*-*-netbsd*],   [systems="oss"],
    [*-*-freebsd*],  [systems="oss"],
    [*-*-linux*],    [systems="alsa pulse jack oss"],
    [*-apple*],      [systems="core jack"],
    [*-mingw32*],    [systems="asio dsound wasapi jack"],
    [*-mingw64*],    [systems="asio dsound wasapi jack"],
    [*-msys*],       [systems="asio dsound wasapi jack"],
  ))

# If any were specifically requested disabled, do it.
AS_IF([test "x$with_jack"   = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v jack`])
AS_IF([test "x$with_alsa"   = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v alsa`])
AS_IF([test "x$with_pulse"  = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v pulse`])
AS_IF([test "x$with_oss"    = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v oss`])
AS_IF([test "x$with_core"   = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v core`])
AS_IF([test "x$with_asio"   = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v asio`])
AS_IF([test "x$with_dsound" = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v dsound`])
AS_IF([test "x$with_wasapi" = "xno"], [systems=`echo $systems|tr ' ' \\\\n|grep -v wasapi`])
systems=" `echo $systems|tr \\\\n ' '` "

# For each audio system, check if it is selected and found.
# Note: Order specified above is not necessarily respected. However,
# *actual* priority is set at run-time, see RtAudio::openRtApi.
# One AS_CASE per system, since they are not mutually-exclusive.

AS_CASE(["$systems"], [*" alsa "*], [
  AC_CHECK_LIB(asound, snd_pcm_open,
    [api="$api -D__LINUX_ALSA__"
     req="$req alsa"
     need_pthread=yes
     found="$found ALSA"
     LIBS="-lasound $LIBS"],
    AS_CASE(["$required"], [*" alsa "*],
      AC_MSG_ERROR([ALSA support requires the asound library!])))
])

AS_CASE(["$systems"], [*" pulse "*], [
  AC_CHECK_LIB(pulse-simple, pa_simple_flush,
    [api="$api -D__LINUX_PULSE__"
     req="$req libpulse-simple"
     need_pthread=yes
     found="$found PulseAudio"
     LIBS="-lpulse-simple $LIBS"],
    AS_CASE(["$required"], [*" pulse "*],
      AC_MSG_ERROR([PulseAudio support requires the pulse-simple library!])))
])

AS_CASE(["$systems"], [*" oss "*], [
  # libossaudio not required on some platforms (e.g. linux) so we
  # don't break things if it's not found, but issue a warning when we
  # are not sure (i.e. not on linux)
  AS_CASE([$host], [*-*-linux*], [], [*], [need_ossaudio=yes])
  AC_CHECK_LIB(ossaudio, main, [have_ossaudio=true],
    AS_CASE(["$required"], [*" oss "*],
      AS_IF([test "x$need_ossaudio" = xyes],
        AC_MSG_WARN([RtAudio may require the ossaudio library]))))

  # linux systems may have soundcard.h but *not* have OSS4 installed,
  # we have to actually check if it exports OSS4 symbols
  AC_CHECK_DECL(SNDCTL_SYSINFO,
    [api="$api -D__LINUX_OSS__"
     need_pthread=yes
     found="$found OSS"],
     AS_CASE(["$required"], [*" oss "*],
       AC_MSG_ERROR([sys/soundcard.h not found]))
    [],
    [#include <sys/soundcard.h>])
])

AS_CASE(["$systems"], [*" jack "*], [
  AC_CHECK_LIB(jack, jack_client_open,
    [api="$api -D__UNIX_JACK__"
     req="$req jack"
     need_pthread=yes
     found="$found JACK"
     LIBS="-ljack $LIBS"],
    AS_CASE(["$required"], [*" jack "*],
      AC_MSG_ERROR([JACK support requires the jack library!])))
])

AS_CASE(["$systems"], [*" core "*], [
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h,
    [api="$api -D__MACOSX_CORE__"
     need_pthread=yes
     found="$found CoreAudio",
     LIBS="$LIBS -framework CoreAudio -framework CoreFoundation"],
    AS_CASE(["$required"], [*" core "*],
      AC_MSG_ERROR([CoreAudio header files not found!])))
])

AS_CASE(["$systems"], [*" asio "*], [
  api="$api -D__WINDOWS_ASIO__"
  use_asio=yes
  CPPFLAGS="-I$srcdir/include $CPPFLAGS"
  need_ole32=yes
  found="$found ASIO"
])

AS_CASE(["$systems"], [*" dsound "*], [
  AC_CHECK_HEADERS(windows.h)
  AC_CHECK_HEADERS(mmsystem.h mmreg.h dsound.h, [], [],
[#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif])
  AS_IF([test "x$ac_cv_header_windows_h" = xyes \
      && test "x$ac_cv_header_mmsystem_h" = xyes \
      && test "x$ac_cv_header_mmreg_h" = xyes \
      && test "x$ac_cv_header_dsound_h" = xyes],
    [api="$api -D__WINDOWS_DS__"
     need_ole32=yes
     found="$found DirectSound"
     LIBS="-ldsound -lwinmm $LIBS"])
])

AS_CASE(["$systems"], [*" wasapi "*], [
  AC_CHECK_HEADERS(windows.h)
  AC_CHECK_HEADERS(audioclient.h avrt.h mmdeviceapi.h, [], [],
[#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif])
  AS_IF([test "x$ac_cv_header_windows_h" = xyes \
      && test "x$ac_cv_header_audioclient_h" = xyes \
      && test "x$ac_cv_header_avrt_h" = xyes \
      && test "x$ac_cv_header_mmdeviceapi_h" = xyes],
    [api="$api -D__WINDOWS_WASAPI__"
     CPPFLAGS="-I$srcdir/include $CPPFLAGS"
     need_ole32=yes
     found="$found WASAPI"
     LIBS="-lwinmm -lksuser -lmfplat -lmfuuid -lwmcodecdspuuid $LIBS"])
])

AS_IF([test -n "$need_ole32"], [LIBS="-lole32 $LIBS"])

AS_IF([test -n "$need_pthread"],[
  AC_MSG_CHECKING([for pthread])
  AC_CHECK_LIB(pthread, pthread_create, ,
    AC_MSG_ERROR([RtAudio requires the pthread library!]))])

AC_MSG_CHECKING([for audio API])

# Error case: no known realtime systems found.
AS_IF([test x"$api" = "x"], [
  AC_MSG_RESULT([none])
  AC_MSG_ERROR([No known system type found for realtime support!])
], [
  AC_MSG_RESULT([$found])
])

AM_CONDITIONAL( ASIO, [test "x${use_asio}" = "xyes" ])

CPPFLAGS="$CPPFLAGS $api"

AC_OUTPUT
