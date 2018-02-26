cmake_minimum_required(VERSION 2.8.10)
project(RtAudio)

include(CTest)
include(CheckFunctionExists)

option(BUILD_PYTHON "Build PyRtAudio python bindings" OFF)
option(AUDIO_WINDOWS_DS "Build Windows DirectSound API" OFF)
option(AUDIO_WINDOWS_ASIO "Build Windows ASIO API" OFF)
option(AUDIO_WINDOWS_WASAPI "Build Windows WASAPI API" OFF)
option(AUDIO_LINUX_OSS "Build Linux OSS API" OFF)
option(AUDIO_LINUX_ALSA "Build Linux ALSA API" OFF)
option(AUDIO_LINUX_PULSE "Build Linux PulseAudio API" OFF)
option(AUDIO_UNIX_JACK "Build Unix JACK audio server API" OFF)
option(AUDIO_OSX_CORE "Build Mac OSX CoreAudio API" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D__RTAUDIO_DEBUG__)
endif ()

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)

if (HAVE_GETTIMEOFDAY)
    add_definitions(-DHAVE_GETTIMEOFDAY)
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif (CMAKE_COMPILER_IS_GNUCXX)

set(rtaudio_SOURCES RtAudio.cpp rtaudio_c.cpp)

set(LINKLIBS)
if (CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
    message(STATUS "NetBSD detected, using OSS")
    find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
    list(APPEND LINKLIBS ossaudio ${CMAKE_THREAD_LIBS_INIT})
    set(AUDIO_LINUX_OSS ON)
elseif (UNIX AND NOT APPLE)
    if (NOT AUDIO_LINUX_PULSE AND NOT AUDIO_LINUX_ALSA AND NOT AUDIO_LINUX_OSS AND NOT AUDIO_UNIX_JACK)
        set(AUDIO_LINUX_ALSA ON)
    endif()

    if (AUDIO_LINUX_PULSE)
        find_library(PULSE_LIB pulse)
        find_library(PULSESIMPLE_LIB pulse-simple)
        find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
        list(APPEND LINKLIBS ${PULSE_LIB} ${PULSESIMPLE_LIB} ${CMAKE_THREAD_LIBS_INIT})
        add_definitions(-D__LINUX_PULSE__)
        message(STATUS "Using Linux PulseAudio")
    endif (AUDIO_LINUX_PULSE)
    if (AUDIO_LINUX_ALSA)
        find_package(ALSA)
        find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
        if (NOT ALSA_FOUND)
            message(FATAL_ERROR "ALSA API requested but no ALSA dev libraries found")
        endif()
        include_directories(${ALSA_INCLUDE_DIR})
        list(APPEND LINKLIBS ${ALSA_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
        add_definitions(-D__LINUX_ALSA__)
        message(STATUS "Using Linux ALSA")
    endif (AUDIO_LINUX_ALSA)
endif ()

if (APPLE)
    if (NOT AUDIO_OSX_CORE AND NOT AUDIO_UNIX_JACK)
        set(AUDIO_OSX_CORE ON)
    endif()

    if (AUDIO_OSX_CORE)
        find_library(COREAUDIO_LIB CoreAudio)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        list(APPEND LINKLIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
        add_definitions(-D__MACOSX_CORE__)
        message(STATUS "Using OSX CoreAudio")
    endif (AUDIO_OSX_CORE)
endif (APPLE)

# JACK supported on many Unices
if (UNIX)
    if (AUDIO_UNIX_JACK)
        find_library(JACK_LIB jack)
        list(APPEND LINKLIBS ${JACK_LIB})
        add_definitions(-D__UNIX_JACK__)
        message(STATUS "Using JACK")
    endif (AUDIO_UNIX_JACK)
endif (UNIX)

if (WIN32)
    if (NOT AUDIO_WINDOWS_DS AND NOT AUDIO_WINDOWS_ASIO AND NOT AUDIO_WINDOWS_WASAPI)
        set(AUDIO_WINDOWS_WASAPI ON)
    endif()

    include_directories(include)
    list(APPEND LINKLIBS winmm ole32)

    if (AUDIO_WINDOWS_DS)
        add_definitions(-D__WINDOWS_DS__)
        message(STATUS "Using Windows DirectSound")
        list(APPEND LINKLIBS dsound)
    endif (AUDIO_WINDOWS_DS)
    if (AUDIO_WINDOWS_WASAPI)
        add_definitions(-D__WINDOWS_WASAPI__)
        message(STATUS "Using Windows WASAPI")
        list(APPEND LINKLIBS uuid ksuser)
    endif (AUDIO_WINDOWS_WASAPI)
    if (AUDIO_WINDOWS_ASIO)
        list(APPEND rtaudio_SOURCES
            include/asio.cpp
            include/asiodrivers.cpp
            include/asiolist.cpp
            include/iasiothiscallresolver.cpp)
        add_definitions(-D__WINDOWS_ASIO__)
        message(STATUS "Using Windows ASIO")
    endif (AUDIO_WINDOWS_ASIO)
endif (WIN32)

cmake_policy(SET CMP0042 OLD)
add_library(rtaudio SHARED ${rtaudio_SOURCES})
add_library(rtaudio_static STATIC ${rtaudio_SOURCES})

target_link_libraries(rtaudio ${LINKLIBS})

if (BUILD_TESTING)
    add_subdirectory(tests)
endif (BUILD_TESTING)

configure_file("rtaudio.pc.in" "rtaudio.pc" @ONLY)

install(TARGETS rtaudio
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      RUNTIME DESTINATION bin)

install(
    FILES RtAudio.h
    DESTINATION include)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/rtaudio.pc
    DESTINATION lib/pkgconfig)
