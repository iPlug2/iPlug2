cmake_minimum_required(VERSION 3.11)

set(VST3_SDK_DIR "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK" CACHE PATH "VST3 SDK directory.")

set(_src "")
set(_def "")
set(_inf "")

set(sdk "${VST3_SDK_DIR}/base/source")
list(APPEND _src
  "${sdk}/baseiids.cpp"
  "${sdk}/classfactoryhelpers.h"
  "${sdk}/fbuffer.cpp"
  "${sdk}/fbuffer.h"
  "${sdk}/fcleanup.h"
  "${sdk}/fcommandline.h"
  "${sdk}/fdebug.cpp"
  "${sdk}/fdebug.h"
  "${sdk}/fdynlib.cpp"
  "${sdk}/fdynlib.h"
  "${sdk}/fobject.cpp"
  "${sdk}/fobject.h"
  "${sdk}/fstdmethods.h"
  "${sdk}/fstreamer.cpp"
  "${sdk}/fstreamer.h"
  "${sdk}/fstring.cpp"
  "${sdk}/fstring.h"
  "${sdk}/hexbinary.h"
  "${sdk}/timer.cpp"
  "${sdk}/timer.h"
  "${sdk}/updatehandler.cpp"
  "${sdk}/updatehandler.h"
)
set(sdk "${VST3_SDK_DIR}/base/thread")
list(APPEND _src
  "${sdk}/include/fcondition.h"
  "${sdk}/include/flock.h"
  "${sdk}/source/fcondition.cpp"
  "${sdk}/source/flock.cpp"
)
set(sdk "${VST3_SDK_DIR}/pluginterfaces/base")
list(APPEND _src
  "${sdk}/conststringtable.cpp"
  "${sdk}/coreiids.cpp"
  "${sdk}/funknown.cpp"
  "${sdk}/ustring.cpp"
)
# In the public.sdk dir we only add specific sources.
set(sdk ${VST3_SDK_DIR}/public.sdk/source)
list(APPEND _src
  "${sdk}/common/commoniids.cpp"
  "${sdk}/common/memorystream.cpp"
  "${sdk}/common/pluginview.cpp" 
  "${sdk}/vst/vstaudioeffect.cpp"
  "${sdk}/vst/vstbus.cpp" 
  "${sdk}/vst/vstcomponent.cpp"
  "${sdk}/vst/vstcomponentbase.cpp" 
  "${sdk}/vst/vstinitiids.cpp"
  "${sdk}/vst/vstparameters.cpp"
  "${sdk}/vst/vstsinglecomponenteffect.cpp"
)

# Platform-dependent stuff
if (WIN32)
  list(APPEND _src "${sdk}/main/dllmain.cpp" "${sdk}/main/pluginfactory.cpp" "${sdk}/common/threadchecker_win32.cpp")
  list(APPEND _inf "${sdk}/main/winexport.def")

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  list(APPEND _def "SWELL_CLEANUP_ON_UNLOAD")
  list(APPEND _src "${sdk}/main/macmain.cpp" "${sdk}/main/pluginfactory.cpp")
  list(APPEND _inf "${sdk}/main/macexport.exp")
endif()

add_library(VST3_SDK STATIC ${_src})
target_include_directories(VST3_SDK PUBLIC "${VST3_SDK_DIR}")
target_compile_definitions(VST3_SDK PUBLIC "$<IF:$<CONFIG:Debug>,DEVELOPMENT,RELEASE>")
source_group(TREE ${VST3_SDK_DIR} PREFIX "IPlug/VST3" FILES ${_src})
iplug2_target_add(VST3_SDK INTERFACE SOURCE ${_inf} DEFINE ${_def})

