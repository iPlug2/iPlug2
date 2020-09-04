cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

cmake_policy(SET CMP0046 NEW)  # Error on non-existent dependency in add_dependencies
cmake_policy(SET CMP0054 NEW)  # Only interpret if() arguments as variables or keywords when unquoted
cmake_policy(SET CMP0063 NEW)  # Honor visibility properties for all target types
cmake_policy(SET CMP0091 NEW)  # MSVC runtime library flags are selected by an abstraction


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
include(IPlug2)


#------------------------------------------------------------------------------
# Project settings
# If we're calling from an external source we let that project be the owner of
# IPlug2 static library instead of our own project. This is needed due to how
# the library is designed and needs config.h from the 'client' when building.

if(NOT IPLUG2_EXTERNAL_PROJECT)
    project( "IPlug2"
            VERSION        ${IPLUG_VERSION}
            DESCRIPTION    "IPlug2"
            HOMEPAGE_URL   "https://iplug2.github.io"
            LANGUAGES      C CXX )

    # Set up defaults internaly
    _iplug_check_initialized()
endif()

# Testing architecture. CMAKE_SIZEOF_VOID_P is only available *after* a project has been declared.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    iplug_syntax_error("32-bit is not supported.")
endif()

# hide some variables in normal cmake gui applications to make it more clear
mark_as_advanced(CMAKE_AR)
mark_as_advanced(CMAKE_INSTALL_PREFIX)
mark_as_advanced(_VCPKG_CL)
mark_as_advanced(_VCPKG_INSTALLED_DIR)
mark_as_advanced(VCPKG_APPLOCAL_DEPS)
mark_as_advanced(VCPKG_TARGET_TRIPLET)


#------------------------------------------------------------------------------

# IPlug2 Interface libraries
add_library(IPlug_SharedDefinitions    INTERFACE)
add_library(IPlug_SharedIncludes       INTERFACE)
add_library(IPlug_SharedCompileOptions INTERFACE)
add_library(IPlug_SharedLinkLibraries  INTERFACE)

# Target Interface libraries
add_library(IPlug_APP   INTERFACE)
add_library(IPlug_AAX   INTERFACE)
add_library(IPlug_AU    INTERFACE)
add_library(IPlug_AUv3  INTERFACE)
add_library(IPlug_VST2  INTERFACE)
add_library(IPlug_VST3  INTERFACE)
add_library(IPlug_VST3C INTERFACE)
add_library(IPlug_VST3P INTERFACE)
add_library(IPlug_WEB   INTERFACE)


#------------------------------------------------------------------------------

add_subdirectory(IPlug)
add_subdirectory(IGraphics)
add_subdirectory(WDL)
add_subdirectory(Dependencies)


#------------------------------------------------------------------------------
# Compile options for all IPlug2 related libraries. This is not used by consumers

target_compile_options(IPlug_SharedCompileOptions
    INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /Zp8    # Packs structures on 8-byte boundaries. Default is 16
        >
)

#------------------------------------------------------------------------------
# Definitions

# NanoVG definitions
set(_NANOVG_DEF "")
if(GFXLIB_NANOVG)
    list(APPEND _NANOVG_DEF IGRAPHICS_NANOVG)

    if(IPLUG2_ENABLE_METAL)
        list(APPEND _NANOVG_DEF IGRAPHICS_METAL)
    else()
        list(APPEND _NANOVG_DEF
            # these are just here for templating, don't know if some platform requires other GL versions
            $<$<BOOL:${PLATFORM_WINDOWS}>:IGRAPHICS_GL3>
            $<$<BOOL:${PLATFORM_MAC}>:IGRAPHICS_GL3>
            $<$<BOOL:${PLATFORM_LINUX}>:IGRAPHICS_GL3>
            $<$<BOOL:${PLATFORM_IOS}>:IGRAPHICS_GL3>
        )
    endif()
endif()

# TODO: Add support for the other graphics libraries


# Shared definitions for all IPlug2 related libraries and consumers
target_compile_definitions(IPlug_SharedDefinitions
    INTERFACE
        ${PLATFORM_ID}=1
        PLATFORM_NAME=${PLATFORM_NAME}
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

        ${_NANOVG_DEF}

        $<$<BOOL:${PLATFORM_MAC}>:
            SWELL_CLEANUP_ON_UNLOAD
        >
        $<$<BOOL:${PLATFORM_WINDOWS}>:
            WIN32
            _WINDOWS
            MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0
        >
)

# Target specific definitions
target_compile_definitions(IPlug_APP  INTERFACE APP_API  IPLUG_EDITOR=1 IPLUG_DSP=1 $<$<BOOL:${PLATFORM_MAC}>:SWELL_COMPILED> )
target_compile_definitions(IPlug_VST3 INTERFACE VST3_API IPLUG_EDITOR=1 IPLUG_DSP=1                                           )


#------------------------------------------------------------------------------
# Shared includes for all IPlug2 related libraries and consumers

target_include_directories(IPlug_SharedIncludes
    INTERFACE
        # Are these needed? Shouldn't these already be defined in xcode or env variables?
        $<$<BOOL:${PLATFORM_MAC}>:
            /System/Library/Frameworks
            /Library/Frameworks
        >

        IPlug
        IGraphics

        IGraphics/Controls
        IGraphics/Drawing
        IGraphics/Platforms

        Dependencies/IGraphics/STB

        $<$<BOOL:${PLATFORM_MAC}>:${IPLUG2_ROOT_PATH}/IPlug/Extras/Cocoa>
        $<$<BOOL:${PLATFORM_IOS}>:${IPLUG2_ROOT_PATH}/Dependencies/IGraphics/MSColorPicker>
        $<$<BOOL:${PLATFORM_WEB}>:${IPLUG2_ROOT_PATH}/IPlug/Extras/WebView>
        $<$<BOOL:${PLATFORM_WEB}>:${IPLUG2_ROOT_PATH}/Dependencies/Extras/nlohmann> # Only used by WebView


        $<$<BOOL:${HAVESDK_WAM}>:${IPLUG2_ROOT_PATH}/Dependencies/IPlug/WAM_AWP>
        $<$<BOOL:${HAVESDK_WAM}>:${IPLUG2_ROOT_PATH}/Dependencies/IPlug/WAM_SDK>
        $<$<BOOL:${PLUGINAPI_REAPEREXT}>:${IPLUG2_ROOT_PATH}/IPlug/ReaperExt>
        $<$<BOOL:${PLUGINAPI_REAPEREXT}>:${IPLUG2_ROOT_PATH}/Dependencies/IPlug/Reaper>

        $<$<BOOL:${GFXLIB_NANOVG}>:${IPLUG2_ROOT_PATH}/Dependencies/IGraphics/NanoSVG/src>

        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/include/core>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/include/effects>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/include/config>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/include/utils>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/include/gpu>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/experimental/svg/model>
        $<$<BOOL:${GFXLIB_SKIA}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/skia/third_party/externals/icu/source/common>

        $<$<BOOL:${ENABLE_FREETYPE}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/src/freetype/include>

        $<$<BOOL:${EXTRAS_FAUST}>:${IPLUG2_ROOT_PATH}/IPlug/Extras/Faust
            $<$<BOOL:${PLATFORM_WINDOWS}>:${IPLUG2_ROOT_PATH}/Dependencies/Build/win/Faust/include>
        >
)


#------------------------------------------------------------------------------
# Shared linker settings for all IPlug2 related libraries and consumers

set(_deplibs "")

# MacOS & iOS
if(PLATFORM_APPLE)
    # no clue how this works on apple platforms. writing blindly :)
    find_library(FRAMEWORK_ACCELERATE Accelerate)
    list(APPEND _deplibs
        "${FRAMEWORK_ACCELERATE}"
        "-framework CoreAudioKit"
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework QuartzCore"
        "-framework CoreMIDI"
    )
endif()

if(PLATFORM_MAC)
    find_library(FRAMEWORK_COREFOUNDATION CoreFoundation)
    find_library(FRAMEWORK_COCOA Cocoa)
    find_library(FRAMEWORK_OPENGL OpenGL)
    find_library(FRAMEWORK_QUARTZCORE QuartzCore)
    find_library(FRAMEWORK_CARBON Carbon)
    list(APPEND _deplibs
        "${FRAMEWORK_COREFOUNDATION}"
        "${FRAMEWORK_COCOA}"
        "${FRAMEWORK_OPENGL}"
        "${FRAMEWORK_QUARTZCORE}"
        "${FRAMEWORK_CARBON}"
        "-framework AppKit"
    )
endif()

if(PLATFORM_IOS)
        list(APPEND _deplibs
        "-framework UIKit"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework ImageIO"
        "-framework MobileCoreServices"
        "-framework MediaPlayer"
    )
endif()

if(PLATFORM_WINDOWS)
    list(APPEND _deplibs
        "opengl32.lib"
        "dsound.lib"
        "wininet.lib"
        "comctl32.lib"
        "Shlwapi.lib"
    )
endif()

# TODO: Add support for the other graphics libraries
if(GFXLIB_NANOVG)
    list(APPEND _deplibs
        "Glad"
        "NanoVG"
    )
endif()

list(APPEND _deplibs
    "WDL"
    "Yoga"
)

target_link_libraries(IPlug_SharedLinkLibraries INTERFACE ${_deplibs})

# Target specific linker settings
target_link_libraries(IPlug_AAX   INTERFACE AAXLibrary_x64$<$<CONFIG:Debug>:_D>.lib )  # Only 64bit (32bit needs to finally go away)
target_link_libraries(IPlug_APP   INTERFACE RTAudio RTMidi                          )
target_link_libraries(IPlug_AU    INTERFACE                                         )
target_link_libraries(IPlug_AUv3  INTERFACE                                         )
target_link_libraries(IPlug_VST2  INTERFACE                                         )
target_link_libraries(IPlug_VST3  INTERFACE VST3SDK                                 )
target_link_libraries(IPlug_VST3C INTERFACE IPlug_VST3                              )
target_link_libraries(IPlug_VST3P INTERFACE IPlug_VST3                              )
target_link_libraries(IPlug_WEB   INTERFACE                                         )


#------------------------------------------------------------------------------
# Examples

if(NOT IPLUG2_EXTERNAL_PROJECT)
    foreach(_example_project IN ITEMS #[[IPlugChunks]] IPlugControls)
        add_subdirectory("Examples/${_example_project}")
    endforeach()
endif()
