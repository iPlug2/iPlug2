# Set minimum CMake required version for this project.
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Define a C++ project.
project(RtAudio LANGUAGES CXX)

# standards version
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check for Jack (any OS)
find_library(JACK_LIB jack)
find_package(PkgConfig)
pkg_check_modules(jack jack)
if(JACK_LIB OR jack_FOUND)
  set(HAVE_JACK TRUE)
endif()

# Check for Pulse (any OS)
pkg_check_modules(pulse libpulse-simple)

# Check for known non-Linux unix-likes
if (CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
  message(STATUS "NetBSD detected, using OSS")
  set(xBSD ON)
elseif(UNIX AND NOT APPLE)
  set(LINUX ON)
endif()

# Necessary for Windows
if(MINGW)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(NEED_PTHREAD ON)
endif()

# Standard CMake options
option(BUILD_SHARED_LIBS "Build as shared library" ON)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")
endif()
if(WIN32)
  set(CMAKE_DEBUG_POSTFIX d CACHE STRING "Postfix for debug version of library")
endif()

# Build Options
option(RTAUDIO_BUILD_PYTHON "Build PyRtAudio python bindings" OFF)
set(RTAUDIO_TARGETNAME_UNINSTALL "uninstall" CACHE STRING "Name of 'uninstall' build target")

# API Options
option(RTAUDIO_API_DS "Build DirectSound API" OFF)
option(RTAUDIO_API_ASIO "Build ASIO API" OFF)
option(RTAUDIO_API_WASAPI "Build WASAPI API" ${WIN32})
option(RTAUDIO_API_OSS "Build OSS4 API" ${xBSD})
option(RTAUDIO_API_ALSA "Build ALSA API" ${LINUX})
option(RTAUDIO_API_PULSE "Build PulseAudio API" ${pulse_FOUND})
option(RTAUDIO_API_JACK "Build JACK audio server API" ${HAVE_JACK})
option(RTAUDIO_API_CORE "Build CoreAudio API" ${APPLE})

# Check for functions
include(CheckFunctionExists)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
if (HAVE_GETTIMEOFDAY)
    add_definitions(-DHAVE_GETTIMEOFDAY)
endif ()

# Add -Wall if possible
if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif (CMAKE_COMPILER_IS_GNUCXX)

# Add debug flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-D__RTAUDIO_DEBUG__)
  if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif (CMAKE_COMPILER_IS_GNUCXX)
endif ()

# Read libtool version info from configure.ac
set(R "m4_define\\(\\[lt_([a-z]+)\\], ([0-9]+)\\)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/configure.ac" CONFIGAC
  REGEX ${R})
foreach(_S ${CONFIGAC})
  string(REGEX REPLACE ${R} "\\1" k ${_S})
  string(REGEX REPLACE ${R} "\\2" v ${_S})
  set(SO_${k} ${v})
endforeach()
math(EXPR SO_current_minus_age "${SO_current} - ${SO_age}")
set(SO_VER "${SO_current_minus_age}")
set(FULL_VER "${SO_current_minus_age}.${SO_age}.${SO_revision}")

# Read package version info from configure.ac
set(R "AC_INIT\\(RtAudio, ([0-9\\.]+),.*\\)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/configure.ac" CONFIGAC
  REGEX "${R}")
string(REGEX REPLACE "${R}" "\\1" PACKAGE_VERSION "${CONFIGAC}")

# Init variables
set(rtaudio_SOURCES RtAudio.cpp RtAudio.h rtaudio_c.cpp rtaudio_c.h)
set(LINKLIBS)
set(PKGCONFIG_REQUIRES)
set(LIBS_REQUIRES)
set(API_DEFS)
set(API_LIST)

# Tweak API-specific configuration.

# Jack
if (RTAUDIO_API_JACK AND jack_FOUND)
  set(NEED_PTHREAD ON)
  list(APPEND PKGCONFIG_REQUIRES "jack")
  list(APPEND API_DEFS "-D__UNIX_JACK__")
  list(APPEND API_LIST "jack")
  if(jack_FOUND)
    list(APPEND LINKLIBS ${jack_LIBRARIES})
    list(APPEND INCDIRS ${jack_INCLUDEDIR})
  else()
    list(APPEND LINKLIBS ${JACK_LIB})
  endif()
endif()

# ALSA
if (RTAUDIO_API_ALSA)
  set(NEED_PTHREAD ON)
  find_package(ALSA)
  if (NOT ALSA_FOUND)
    message(FATAL_ERROR "ALSA API requested but no ALSA dev libraries found")
  endif()
  list(APPEND INCDIRS ${ALSA_INCLUDE_DIR})
  list(APPEND LINKLIBS ${ALSA_LIBRARIES})
  list(APPEND PKGCONFIG_REQUIRES "alsa")
  list(APPEND API_DEFS "-D__LINUX_ALSA__")
  list(APPEND API_LIST "alsa")
endif()

# OSS
if (RTAUDIO_API_OSS)
  set(NEED_PTHREAD ON)
  find_library(OSSAUDIO_LIB ossaudio)
  if (OSSAUDIO_LIB)
    list(APPEND LINKLIBS ossaudio)
    # Note: not an error on some systems
  endif()
  list(APPEND API_DEFS "-D__LINUX_OSS__")
  list(APPEND API_LIST "oss")
endif()

# Pulse
if (RTAUDIO_API_PULSE)
  set(NEED_PTHREAD ON)
  find_library(PULSE_LIB pulse)
  find_library(PULSESIMPLE_LIB pulse-simple)
  list(APPEND LINKLIBS ${PULSE_LIB} ${PULSESIMPLE_LIB})
  list(APPEND PKGCONFIG_REQUIRES "libpulse-simple")
  list(APPEND API_DEFS "-D__LINUX_PULSE__")
  list(APPEND API_LIST "pulse")
endif()

# CoreAudio
if (RTAUDIO_API_CORE)
  find_library(COREAUDIO_LIB CoreAudio)
  find_library(COREFOUNDATION_LIB CoreFoundation)
  list(APPEND LINKLIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
  list(APPEND LIBS_REQUIRES "-framework CoreAudio -framework CoreFoundation")
  list(APPEND API_DEFS "-D__MACOSX_CORE__")
  list(APPEND API_LIST "core")
endif()

# ASIO
if (RTAUDIO_API_ASIO)
  set(NEED_WIN32LIBS ON)
  include_directories(include)
  list(APPEND rtaudio_SOURCES
    include/asio.cpp
    include/asiodrivers.cpp
    include/asiolist.cpp
    include/iasiothiscallresolver.cpp)
  list(APPEND API_DEFS "-D__WINDOWS_ASIO__")
  list(APPEND API_LIST "asio")
endif()

# DSound
if (RTAUDIO_API_DS)
  set(NEED_WIN32LIBS ON)
  list(APPEND LINKLIBS dsound)
  list(APPEND API_DEFS "-D__WINDOWS_DS__")
  list(APPEND API_LIST "ds")
endif()

# WASAPI
if (RTAUDIO_API_WASAPI)
  include_directories(include)
  set(NEED_WIN32LIBS ON)
  list(APPEND LINKLIBS ksuser mfplat mfuuid wmcodecdspuuid)
  list(APPEND API_DEFS "-D__WINDOWS_WASAPI__")
  list(APPEND API_LIST "wasapi")
endif()

# Windows libs
if (NEED_WIN32LIBS)
  list(APPEND LINKLIBS winmm ole32)
endif()

# pthread
if (NEED_PTHREAD)
  find_package(Threads REQUIRED
    CMAKE_THREAD_PREFER_PTHREAD
    THREADS_PREFER_PTHREAD_FLAG)
  list(APPEND LINKLIBS Threads::Threads)
endif()

# Create library targets.
set(LIB_TARGETS)

# Use RTAUDIO_BUILD_SHARED_LIBS / RTAUDIO_BUILD_STATIC_LIBS if they
# are defined, otherwise default to standard BUILD_SHARED_LIBS.
if (DEFINED RTAUDIO_BUILD_SHARED_LIBS)
  if (RTAUDIO_BUILD_SHARED_LIBS)
    add_library(rtaudio SHARED ${rtaudio_SOURCES})
  else()
    add_library(rtaudio STATIC ${rtaudio_SOURCES})
    set(RTAUDIO_IS_STATIC TRUE)
  endif()
elseif (DEFINED RTAUDIO_BUILD_STATIC_LIBS)
  if (RTAUDIO_BUILD_STATIC_LIBS)
    add_library(rtaudio STATIC ${rtaudio_SOURCES})
    set(RTAUDIO_IS_STATIC TRUE)
  else()
    add_library(rtaudio SHARED ${rtaudio_SOURCES})
  endif()
else()
  add_library(rtaudio ${rtaudio_SOURCES})
  if(NOT BUILD_SHARED_LIBS)
    set(RTAUDIO_IS_STATIC TRUE)
  endif()
endif()
list(APPEND LIB_TARGETS rtaudio)

# Windows: If RTAUDIO_STATIC_MSVCRT is not set, it defaults to ON when building as a
# static library and OFF when building as a DLL.  If you want to have more control, you
# can explicitly override RTAUDIO_STATIC_MSVCRT to turn it off/on.  It controls the flags
# related to MSVC runtime linkage in the next clause, below.
if (NOT DEFINED RTAUDIO_STATIC_MSVCRT)
  set(RTAUDIO_STATIC_MSVCRT ${RTAUDIO_IS_STATIC})
endif()

# In MSVC, set MD/MT appropriately for a static library
# (From https://github.com/protocolbuffers/protobuf/blob/master/cmake/CMakeLists.txt)
if(MSVC AND RTAUDIO_STATIC_MSVCRT)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach(flag_var)
endif()

set_target_properties(rtaudio PROPERTIES
  SOVERSION ${SO_VER}
  VERSION ${FULL_VER})

# Set standard installation directories.
include(GNUInstallDirs)

# Set include paths, populate target interface.
target_include_directories(rtaudio
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${INCDIRS}
)

# Set compile-time definitions
target_compile_definitions(rtaudio PRIVATE ${API_DEFS})
target_compile_definitions(rtaudio PRIVATE RTAUDIO_EXPORT)
target_link_libraries(rtaudio ${LINKLIBS})

# Subdirs
include(CTest)

if (NOT DEFINED RTAUDIO_BUILD_TESTING OR RTAUDIO_BUILD_TESTING STREQUAL "")
  set(RTAUDIO_BUILD_TESTING ${BUILD_TESTING})
endif()
if (RTAUDIO_BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Message
string(REPLACE ";" " " apilist "${API_LIST}")
message(STATUS "Compiling with support for: ${apilist}")

# PkgConfig file
string(REPLACE ";" " " req "${PKGCONFIG_REQUIRES}")
string(REPLACE ";" " " req_libs "${LIBS_REQUIRES}")
string(REPLACE ";" " " api "${API_DEFS}")
set(prefix ${CMAKE_INSTALL_PREFIX})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/rtaudio.pc.in" "rtaudio.pc" @ONLY)

# Add install rule.
install(TARGETS ${LIB_TARGETS}
        EXPORT RtAudioTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rtaudio)

# Install public header files
install(FILES RtAudio.h rtaudio_c.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rtaudio)

# Store the package in the user registry.
export(PACKAGE RtAudio)

# Set installation path for CMake files.
set(RTAUDIO_CMAKE_DESTINATION share/rtaudio)

# Create CMake configuration export file.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfig.cmake.in "@PACKAGE_INIT@\n")

if(NEED_PTHREAD)
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfig.cmake.in "find_package(Threads REQUIRED)\n")
endif()

file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfig.cmake.in "include(\${CMAKE_CURRENT_LIST_DIR}/RtAudioTargets.cmake)")

# Install CMake configuration export file.
include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfig.cmake
  INSTALL_DESTINATION ${RTAUDIO_CMAKE_DESTINATION}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfig-version.cmake
    VERSION ${FULL_VER}
    COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/RtAudioConfig.cmake
    ${CMAKE_BINARY_DIR}/RtAudioConfig-version.cmake
  DESTINATION
    ${RTAUDIO_CMAKE_DESTINATION}
)

# Export library target (build-tree).
export(EXPORT RtAudioTargets
       NAMESPACE RtAudio::)

# Export library target (install-tree).
install(EXPORT RtAudioTargets
        DESTINATION ${RTAUDIO_CMAKE_DESTINATION}
        NAMESPACE RtAudio::)

# Configure uninstall target.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RtAudioConfigUninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfigUninstall.cmake" @ONLY)

# Create uninstall target.
add_custom_target(${RTAUDIO_TARGETNAME_UNINSTALL}
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/RtAudioConfigUninstall.cmake)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/rtaudio.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
