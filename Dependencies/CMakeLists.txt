cmake_minimum_required(VERSION 3.17)
include_guard(GLOBAL)

set(_LIBFOLDER "Libraries")

set(_DEPENDENCIES_WARNINGS
    # MSVC
    $<$<CXX_COMPILER_ID:MSVC>:
        /wd4430     # missing type specifier - int assumed. Note: C++ does not support default-int
        /wd5105     # macro expansion producing 'defined' has undefined behavior
        /wd4267     # 'var' : conversion from 'size_t' to 'type', possible loss of data
        /wd4244     # 'conversion' conversion from 'type1' to 'type2', possible loss of data
    >
)


#-----------------------------------------------------------------------------
# Glad
#-----------------------------------------------------------------------------

set(_libName "Glad")
set(_libPath "${IPLUG2_ROOT_PATH}/Dependencies/IGraphics/glad")

add_library(${_libName} STATIC)

target_compile_options(${_libName} PRIVATE ${_DEPENDENCIES_WARNINGS})

target_compile_definitions(${_libName}
    PRIVATE
        GLAD_GLAPI_EXPORT
        GLAD_GLAPI_EXPORT_BUILD
)

target_include_directories(${_libName}
    PUBLIC
        # "IGraphics/glad_GL2/include"
        # "IGraphics/glad_GL2/src"
        # "IGraphics/glad_GL3/include"
        # "IGraphics/glad_GL3/src"
        "IGraphics/glad"
)

target_sources(${_libName}
    PRIVATE
        "IGraphics/glad/glad.c"
        "IGraphics/glad/glad.h"
        "IGraphics/glad/khrplatform.h"
)

get_target_property(DEPSOURCE_LIST ${_libName} SOURCES)
source_group(TREE "${_libPath}" FILES ${DEPSOURCE_LIST})
set_target_properties(${_libName} PROPERTIES FOLDER "${_LIBFOLDER}")


# -----------------------------------------------------------------------------
# RTAudio
# -----------------------------------------------------------------------------

set(_libName "RTAudio")
set(_libPath "${IPLUG2_ROOT_PATH}/Dependencies/IPlug/RTAudio")

add_library(${_libName} STATIC)

target_compile_options(${_libName} PRIVATE ${_DEPENDENCIES_WARNINGS})

target_compile_definitions(${_libName}
    PRIVATE
        # RTAudio (https://www.music.mcgill.ca/~gary/rtaudio/compiling.html)
        # __LINUX_ALSA__
        # __LINUX_PULSE__
        # __LINUX_OSS__        (Do not use)
        # __UNIX_JACK__        (Linux or Mac)
        # __MACOSX_CORE__
        # __WINDOWS_DS__       DirectSound
        # __WINDOWS_MM__       MultiMedia Audio (No.. do not use)
        # __WINDOWS_ASIO__     ASIO (Prefered if host, and ASIO4ALL isn't used on client)
        # __WINDOWS_WASAPI__   WASAPI (Prefered on windows 10)
        $<$<BOOL:${PLATFORM_WINDOWS}>: __WINDOWS_ASIO__ __WINDOWS_DS__ __WINDOWS_WASAPI__ >
        $<$<BOOL:${PLATFORM_MAC}>:     __MACOSX_CORE__                                    >

)

target_include_directories(${_libName}
    PUBLIC
        "IPlug/RTAudio/include"
        "IPlug/RTAudio"
)

target_sources(${_libName}
    PRIVATE
        "IPlug/RTAudio/RtAudio.h"
        "IPlug/RTAudio/rtaudio_c.h"
        "IPlug/RTAudio/RtAudio.cpp"
        "IPlug/RTAudio/rtaudio_c.cpp"
        "IPlug/RTAudio/include/asio.cpp"
        "IPlug/RTAudio/include/asio.h"
        "IPlug/RTAudio/include/asiodrivers.cpp"
        "IPlug/RTAudio/include/asiodrivers.h"
        "IPlug/RTAudio/include/asiodrvr.h"
        "IPlug/RTAudio/include/asiolist.cpp"
        "IPlug/RTAudio/include/asiolist.h"
        "IPlug/RTAudio/include/asiosys.h"
        "IPlug/RTAudio/include/dsound.h"
        "IPlug/RTAudio/include/functiondiscoverykeys_devpkey.h"
        "IPlug/RTAudio/include/ginclude.h"
        "IPlug/RTAudio/include/iasiodrv.h"
        "IPlug/RTAudio/include/iasiothiscallresolver.cpp"
        "IPlug/RTAudio/include/iasiothiscallresolver.h"
        "IPlug/RTAudio/include/soundcard.h"
)

get_target_property(DEPSOURCE_LIST ${_libName} SOURCES)
source_group(TREE "${_libPath}" FILES ${DEPSOURCE_LIST})
set_target_properties(${_libName} PROPERTIES FOLDER "${_LIBFOLDER}")



# -----------------------------------------------------------------------------
# RTMidi
# -----------------------------------------------------------------------------

set(_libName "RTMidi")
set(_libPath "${IPLUG2_ROOT_PATH}/Dependencies/IPlug/RTMidi")

add_library(${_libName} STATIC)

target_compile_options(${_libName} PRIVATE ${_DEPENDENCIES_WARNINGS})

target_include_directories(${_libName}
    PUBLIC
        "IPlug/RTMidi"
)

target_sources(${_libName}
    PRIVATE
        "IPlug/RTMidi/RtMidi.cpp"
        "IPlug/RTMidi/RtMidi.h"
        "IPlug/RTMidi/rtmidi_c.cpp"
        "IPlug/RTMidi/rtmidi_c.h"
)

get_target_property(DEPSOURCE_LIST ${_libName} SOURCES)
source_group(TREE "${_libPath}" FILES ${DEPSOURCE_LIST})
set_target_properties(${_libName} PROPERTIES FOLDER "${_LIBFOLDER}")


# -----------------------------------------------------------------------------
# Yoga
# -----------------------------------------------------------------------------

set(_libName "Yoga")
set(_libPath "${IPLUG2_ROOT_PATH}/Dependencies/IGraphics/yoga/yoga")

add_library(${_libName} STATIC)

target_compile_options(${_libName} PRIVATE ${_DEPENDENCIES_WARNINGS})

target_include_directories(${_libName}
    PUBLIC
        "IGraphics/yoga"
)

target_sources(${_libName}
    PRIVATE
        "IGraphics/yoga/yoga/Bitfield.h"
        "IGraphics/yoga/yoga/CompactValue.h"
        "IGraphics/yoga/yoga/event/event.cpp"
        "IGraphics/yoga/yoga/event/event.h"
        "IGraphics/yoga/yoga/log.cpp"
        "IGraphics/yoga/yoga/log.h"
        "IGraphics/yoga/yoga/Utils.cpp"
        "IGraphics/yoga/yoga/Utils.h"
        "IGraphics/yoga/yoga/YGConfig.cpp"
        "IGraphics/yoga/yoga/YGConfig.h"
        "IGraphics/yoga/yoga/YGEnums.cpp"
        "IGraphics/yoga/yoga/YGEnums.h"
        "IGraphics/yoga/yoga/YGFloatOptional.h"
        "IGraphics/yoga/yoga/YGLayout.cpp"
        "IGraphics/yoga/yoga/YGLayout.h"
        "IGraphics/yoga/yoga/YGMacros.h"
        "IGraphics/yoga/yoga/YGNode.cpp"
        "IGraphics/yoga/yoga/YGNode.h"
        "IGraphics/yoga/yoga/YGNodePrint.cpp"
        "IGraphics/yoga/yoga/YGNodePrint.h"
        "IGraphics/yoga/yoga/YGStyle.cpp"
        "IGraphics/yoga/yoga/YGStyle.h"
        "IGraphics/yoga/yoga/YGValue.cpp"
        "IGraphics/yoga/yoga/YGValue.h"
        "IGraphics/yoga/yoga/Yoga.cpp"
        "IGraphics/yoga/yoga/Yoga.h"
        "IGraphics/yoga/yoga/Yoga-internal.h"
)

get_target_property(DEPSOURCE_LIST ${_libName} SOURCES)
source_group(TREE "${_libPath}" FILES ${DEPSOURCE_LIST})
set_target_properties(${_libName} PROPERTIES FOLDER "${_LIBFOLDER}")

# -----------------------------------------------------------------------------
# NanoVG
#
# -----------------------------------------------------------------------------

set(_libName "NanoVG")
set(_libPath "${IPLUG2_ROOT_PATH}/Dependencies/IGraphics")

add_library(${_libName} STATIC)

target_compile_options(${_libName} PRIVATE ${_DEPENDENCIES_WARNINGS})

target_include_directories(${_libName}
    PUBLIC
        "IGraphics/NanoVG/src"
        $<$<BOOL:${IPLUG2_ENABLE_METALNANOVG}>:${IPLUG2_ROOT_PATH}/Dependencies/IGraphics/MetalNanoVG/src>
)

# FONS_USE_FREETYPE

target_sources(${_libName}
    PRIVATE
        "IGraphics/NanoVG/src/fontstash.h"
        "IGraphics/NanoVG/src/nanovg.c"
        "IGraphics/NanoVG/src/nanovg.h"
        "IGraphics/NanoVG/src/nanovg_gl.h"
        "IGraphics/NanoVG/src/nanovg_gl_utils.h"
        "IGraphics/NanoVG/src/stb_image.h"
        "IGraphics/NanoVG/src/stb_truetype.h"
)

if(IPLUG2_ENABLE_METALNANOVG)
    target_sources(${_libName}
        PRIVATE
            "IGraphics/MetalNanoVG/src/nanovg_mtl.m"
    )
endif()

target_link_libraries(${_libName}
    PUBLIC
        $<$<BOOL:${IPLUG2_ENABLE_METALNANOVG}>:
            -framework Metal
            -framework QuartzCore
        >
)

get_target_property(DEPSOURCE_LIST ${_libName} SOURCES)
source_group(TREE "${_libPath}" FILES ${DEPSOURCE_LIST})
set_target_properties(${_libName} PROPERTIES FOLDER "${_LIBFOLDER}")


# -----------------------------------------------------------------------------
# VST3_SDK
# -----------------------------------------------------------------------------
if(HAVESDK_VST3)
    set(_libName "VST3SDK")
    set(_libPath "${VST3_SDK_PATH}")


    # if(EXISTS "${_libPath}/VST3_SDK/pluginterfaces/base/fstrdefs.h")
    #     file(RENAME "${_libPath}/VST3_SDK/pluginterfaces/base/fstrdefs.h" "${_libPath}/VST3_SDK/pluginterfaces/base/_fstrdefs.h")
    # endif()

    # if(EXISTS "${_libPath}/VST3_SDK/pluginterfaces/base/ftypes.h")
    #     file(RENAME "${_libPath}/VST3_SDK/pluginterfaces/base/ftypes.h" "${_libPath}/VST3_SDK/pluginterfaces/base/_ftypes.h")
    # endif()


    add_library(${_libName} STATIC)

    target_compile_options(${_libName} PRIVATE ${_DEPENDENCIES_WARNINGS})

    target_include_directories(${_libName}
        PUBLIC
            # "IPlug/VST3_SDK_MOD"
            "${VST3_SDK_PATH}"
    )

    target_compile_definitions(${_libName}
        PUBLIC
            # UNICODE_OFF
            $<$<CONFIG:Debug>:DEVELOPMENT=1>
            $<$<CONFIG:Release>:RELEASE=1>
            $<$<CONFIG:Distributed>:RELEASE=1>
    )

    target_sources(${_libName}
        PRIVATE
            # Modified
            # "IPlug/VST3_SDK_MOD/pluginterfaces/base/ftypes.h"
            # "IPlug/VST3_SDK_MOD/pluginterfaces/base/fstrdefs.h"

            # Steinberg VST3 SDK Source
            "${VST3_SDK_PATH}/base/source/baseiids.cpp"
            "${VST3_SDK_PATH}/base/source/classfactoryhelpers.h"
            "${VST3_SDK_PATH}/base/source/fbuffer.cpp"
            "${VST3_SDK_PATH}/base/source/fbuffer.h"
            "${VST3_SDK_PATH}/base/source/fcleanup.h"
            "${VST3_SDK_PATH}/base/source/fcommandline.h"
            "${VST3_SDK_PATH}/base/source/fdebug.cpp"
            "${VST3_SDK_PATH}/base/source/fdebug.h"
            "${VST3_SDK_PATH}/base/source/fdynlib.cpp"
            "${VST3_SDK_PATH}/base/source/fdynlib.h"
            "${VST3_SDK_PATH}/base/source/fobject.cpp"
            "${VST3_SDK_PATH}/base/source/fobject.h"
            "${VST3_SDK_PATH}/base/source/fstdmethods.h"
            "${VST3_SDK_PATH}/base/source/fstreamer.cpp"
            "${VST3_SDK_PATH}/base/source/fstreamer.h"
            "${VST3_SDK_PATH}/base/source/fstring.cpp"
            "${VST3_SDK_PATH}/base/source/fstring.h"
            "${VST3_SDK_PATH}/base/source/hexbinary.h"
            "${VST3_SDK_PATH}/base/source/timer.cpp"
            "${VST3_SDK_PATH}/base/source/timer.h"
            "${VST3_SDK_PATH}/base/source/updatehandler.cpp"
            "${VST3_SDK_PATH}/base/source/updatehandler.h"

            "${VST3_SDK_PATH}/base/thread/include/fcondition.h"
            "${VST3_SDK_PATH}/base/thread/include/flock.h"
            "${VST3_SDK_PATH}/base/thread/source/fcondition.cpp"
            "${VST3_SDK_PATH}/base/thread/source/flock.cpp"

            "${VST3_SDK_PATH}/pluginterfaces/base/conststringtable.cpp"
            "${VST3_SDK_PATH}/pluginterfaces/base/conststringtable.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/coreiids.cpp"
            "${VST3_SDK_PATH}/pluginterfaces/base/doc.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/falignpop.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/falignpush.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/fplatform.h"

            "${VST3_SDK_PATH}/pluginterfaces/base/fstrdefs.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ftypes.h"

            "${VST3_SDK_PATH}/pluginterfaces/base/funknown.cpp"
            "${VST3_SDK_PATH}/pluginterfaces/base/funknown.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/futils.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/fvariant.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/geoconstants.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ibstream.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/icloneable.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ierrorcontext.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ipersistent.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ipluginbase.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/istringresult.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/iupdatehandler.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/keycodes.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/pluginbasefwd.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/smartpointer.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/typesizecheck.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ucolorspec.h"
            "${VST3_SDK_PATH}/pluginterfaces/base/ustring.cpp"
            "${VST3_SDK_PATH}/pluginterfaces/base/ustring.h"
            "${VST3_SDK_PATH}/pluginterfaces/gui/iplugview.h"
            "${VST3_SDK_PATH}/pluginterfaces/gui/iplugviewcontentscalesupport.h"
            "${VST3_SDK_PATH}/pluginterfaces/README.md"
            "${VST3_SDK_PATH}/pluginterfaces/test/itest.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstattributes.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstaudioprocessor.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstautomationstate.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstchannelcontextinfo.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstcomponent.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstcontextmenu.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivsteditcontroller.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstevents.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivsthostapplication.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstinterappaudio.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstmessage.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstmidicontrollers.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstmidilearn.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstnoteexpression.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstparameterchanges.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstparameterfunctionname.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstphysicalui.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstpluginterfacesupport.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstplugview.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstprefetchablesupport.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstprocesscontext.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstrepresentation.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivsttestplugprovider.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/ivstunits.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/vstpresetkeys.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/vstpshpack4.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/vstspeaker.h"
            "${VST3_SDK_PATH}/pluginterfaces/vst/vsttypes.h"

            "${VST3_SDK_PATH}/public.sdk/source/common/commoniids.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/common/openurl.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/common/openurl.h"
            "${VST3_SDK_PATH}/public.sdk/source/common/pluginview.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/common/pluginview.h"
            # "${VST3_SDK_PATH}/public.sdk/source/common/systemclipboard.h"
            # "${VST3_SDK_PATH}/public.sdk/source/common/systemclipboard_mac.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/common/systemclipboard_win32.cpp"

            # VST
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstaudioeffect.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstaudioeffect.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstaudioprocessoralgo.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstbus.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstbus.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstbypassprocessor.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstcomponent.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstcomponent.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstcomponentbase.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstcomponentbase.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vsteditcontroller.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vsteditcontroller.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vsteventshelper.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstguieditor.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstguieditor.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vsthelpers.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstinitiids.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstnoteexpressiontypes.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstnoteexpressiontypes.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstparameters.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstparameters.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstpresetfile.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstpresetfile.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstrepresentation.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstrepresentation.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstsinglecomponenteffect.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/vstsinglecomponenteffect.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/vstspeakerarray.h"

            # Hosting
            "${VST3_SDK_PATH}/public.sdk/source/common/memorystream.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/common/memorystream.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/basewrapper/basewrapper.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/basewrapper/basewrapper.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/connectionproxy.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/connectionproxy.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/eventlist.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/eventlist.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/hostclasses.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/hostclasses.h"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/parameterchanges.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/parameterchanges.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/pluginterfacesupport.h"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/processdata.cpp"
            # "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/processdata.h"


            "${VST3_SDK_PATH}/public.sdk/source/main/pluginfactory.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/main/pluginfactory.h"
            "${VST3_SDK_PATH}/public.sdk/source/common/threadchecker.h"

    )

    if(PLATFORM_WINDOWS)
        target_sources(${_libName}
            PRIVATE
                # "${VST3_SDK_PATH}/public.sdk/source/vst/vstgui_win32_bundle_support.cpp"
                # "${VST3_SDK_PATH}/public.sdk/source/vst/vstgui_win32_bundle_support.h"
                "${VST3_SDK_PATH}/public.sdk/source/common/threadchecker_win32.cpp"
                "${VST3_SDK_PATH}/public.sdk/source/main/dllmain.cpp"
        )
    elseif(PLATFORM_MAC)
        target_sources(${_libName}
            PRIVATE
                "${VST3_SDK_PATH}/public.sdk/source/common/threadchecker_mac.mm"
                "${VST3_SDK_PATH}/public.sdk/source/main/macmain.cpp"
        )
    elseif(PLATFORM_LINUX)
        target_sources(${_libName}
            PRIVATE
                "${VST3_SDK_PATH}/public.sdk/source/common/threadchecker_linux.cpp"
                "${VST3_SDK_PATH}/public.sdk/source/main/linuxmain.cpp"
        )
    endif()
    get_target_property(DEPSOURCE_LIST ${_libName} SOURCES)
    source_group(TREE "${_libPath}" FILES ${DEPSOURCE_LIST})
    set_target_properties(${_libName} PROPERTIES FOLDER "${_LIBFOLDER}")

    # VST3 Interface lib for export
    set(_libName "IVST3SDK")
    add_library(${_libName} INTERFACE)

    if(PLATFORM_WINDOWS)
        target_sources(${_libName} INTERFACE "${VST3_SDK_PATH}/public.sdk/source/main/winexport.def")
    elseif(PLATFORM_MAC)
        target_sources(${_libName} INTERFACE "${VST3_SDK_PATH}/public.sdk/source/main/macexport.exp")
    endif()
endif()

# -----------------------------------------------------------------------------

# "IGraphics/AGG/agg-2.4/include"
# "IGraphics/AGG/agg-2.4/font_freetype"
# "IGraphics/AGG/agg-2.4/include/util"
# "IGraphics/AGG/agg-2.4/include/platform/win32"
# "IGraphics/STB"

