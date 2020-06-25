cmake_minimum_required(VERSION 3.11)

find_file(VST3_32_PATH
  "VST3"
  PATHS "C:/Program Files (x86)/Common Files"
  DOC "Path to install 32-bit VST3 plugins"
)
find_file(VST3_64_PATH
  "VST3"
  PATHS "C:/Program Files/Common Files" "$ENV{HOME}/Library/Audio/Plug-Ins"
  DOC "Path to install 64-bit VST3 plugins"
)

set(IPLUG2_VST_ICON 
  "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK/doc/artwork/VST_Logo_Steinberg.ico"
  CACHE FILEPATH "Path to VST3 plugin icon"
)

##########################
# VST3 Interface Library #
##########################

add_library(iPlug2_VST3 INTERFACE)

set(_inc "")
set(_def "VST3_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1")
set(_src "")
# iPlug2 stuff
set(sdk ${IPLUG2_DIR}/IPlug/VST3)
list(APPEND _inc ${sdk})
target_sources(iPlug2_VST3 INTERFACE 
  "${sdk}/IPlugVST3.h"
  "${sdk}/IPlugVST3.cpp"
  "${sdk}/IPlugVST3_Common.h"
  "${sdk}/IPlugVST3_Controller.h"
  "${sdk}/IPlugVST3_Controller.cpp"
  "${sdk}/IPlugVST3_ControllerBase.h"
  "${sdk}/IPlugVST3_Defs.h"
  "${sdk}/IPlugVST3_Parameter.h"
  "${sdk}/IPlugVST3_Processor.h"
  #"${sdk}/IPlugVST3_Processor.cpp"
  "${sdk}/IPlugVST3_ProcessorBase.h"
  "${sdk}/IPlugVST3_ProcessorBase.cpp"
  "${sdk}/IPlugVST3_View.h"
)
# VST3 SDK files
set(VST3_SDK ${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK)
list(APPEND _inc "${VST3_SDK}")
# Pick up all the VST3 source files, there are a LOT of them. 
file(GLOB_RECURSE tmp ${VST3_SDK}/base/*.cpp)
list(APPEND _src ${tmp})
file(GLOB_RECURSE tmp ${VST3_SDK}/pluginterfaces/*.cpp)
list(APPEND _src ${tmp})
# In the public.sdk dir we only add specific sources.
set(sdk ${VST3_SDK}/public.sdk/source)
list(APPEND _src
  "${sdk}/common/commoniids.cpp"
  "${sdk}/common/memorystream.cpp"
  "${sdk}/common/pluginview.cpp" "${sdk}/vst/vstaudioeffect.cpp"
  "${sdk}/vst/vstbus.cpp" "${sdk}/vst/vstcomponent.cpp"
  "${sdk}/vst/vstcomponentbase.cpp" "${sdk}/vst/vstinitiids.cpp"
  "${sdk}/vst/vstparameters.cpp" "${sdk}/vst/vstsinglecomponenteffect.cpp"
)
list(APPEND _inc
  "${sdk}/vst/vst2wrapper"
)
# Platform-dependent stuff
if (WIN32)
  list(APPEND _src "${sdk}/main/dllmain.cpp" "${sdk}/main/pluginfactory.cpp" "${sdk}/main/winexport.def")
  list(APPEND _src "${sdk}/common/threadchecker_win32.cpp")

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  list(APPEND _def "SWELL_CLEANUP_ON_UNLOAD")
  list(APPEND _src "${sdk}/main/macmain.cpp" "${sdk}/main/pluginfactory.cpp" "${sdk}/main/macexport.exp")
endif()

source_group(TREE ${VST3_SDK} PREFIX "IPlug/VST3" FILES ${_src})
iplug2_add_interface(iPlug2_VST3 INCLUDE ${_inc} SOURCE ${_src} DEFINE ${_def} LINK iPlug2_Core)



function(iplug2_configure_vst3 target)
  if (WIN32)
    # Use .vst3 as the extension instead of .dll
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${IPLUG2_APP_NAME}"
      PREFIX ""
      SUFFIX ".vst3"
    )
    # After building, we run the post-build script
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat" 
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".vst3\""
    )

  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Set the Info.plist file we're using and add resources
    set_target_properties(${target} PROPERTIES 
      BUNDLE TRUE
      MACOS_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/macOS-VST3-Info.plist.in
      BUNDLE_EXTENSION "vst3"
      PREFIX ""
      SUFFIX ""
    )
    target_compile_definitions(${target} PUBLIC "$<IF:$<CONFIG:DEBUG>,DEVELOPMENT,RELEASE>")
  endif()
endfunction()
