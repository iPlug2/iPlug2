cmake_minimum_required(VERSION 3.26 FATAL_ERROR) # What CMAKE version am I even *on* on this thing
cmake_policy(SET CMP0091 NEW)

#########


# To setup the build (change CMAKE_BUILD_TYPE as desired):
#   mkdir build-mac
#   cd build-mac
#   cmake .. -DCMAKE_BUILD_TYPE=Debug
# To build the APP version:
#   cmake --build . --target app -j
# To build the VST3 version:
#   cmake --build . --target vst3 -j
# To build the AUv2 version
#   cmake --build . --target auv2 -j

project(IPlugWebUI VERSION 1.0.0 LANGUAGES C CXX)

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2)
set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
if(WIN32) 
  # Courtesy of rovingeye, again
  execute_process(
    # requires NuGet
    COMMAND nuget restore -PackagesDirectory "${CMAKE_BINARY_DIR}/_deps/Nuget"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  cmake_path( 
    SET WEBVIEW2_SOURCE_DIR 
    "${CMAKE_BINARY_DIR}/_deps/Nuget/Microsoft.Web.WebView2.1.0.1587.40"
  )

  cmake_path( 
    SET WIL_SOURCE_DIR 
    "${CMAKE_BINARY_DIR}/_deps/Nuget/Microsoft.Windows.ImplementationLibrary.1.0.230202.1"
  )
  set(EXTRA_INCLUDE_DIRECTORIES 
    ${WEBVIEW2_SOURCE_DIR}/build/native/include 
    ${WIL_SOURCE_DIR}/include
  )
  set(EXTRA_LINK_DIRECTORIES 
    ${WEBVIEW2_SOURCE_DIR}/build/native/x64
  )
  set(EXTRA_LINK_LIBRARIES 
    WebView2LoaderStatic.lib 
  )

  set(IWEBVIEW_SOURCE 
    ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebView.cpp 
    ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebViewEditorDelegate.cpp
  )
elseif(APPLE) 
  # thanks ben vining!!
  # On further inspection, this is done in the top-level cmake file anyway - but maybe doesn't hurt for OOS builds and verbosity
  enable_language(OBJC)
  enable_language(OBJCXX)

  set(EXTRA_DEFINES 
    OBJC_PREFIX=vIPlugWebUI
    SWELL_APP_PREFIX=Swell_vIPlugWebUI
    SWELL_COMPILED
  )

  # Very possible we don't need some of these - not super familiar with mac frameworks
  set(EXTRA_LINK_LIBRARIES 
    "-framework Carbon -framework AppKit -framework Metal -framework MetalKit -framework WebKit -framework CoreFoundation"
  )

  set(IWEBVIEW_SOURCE 
    ${IPLUG2_DIR}/IPlug/IPlugPaths.mm # is this necessary? 
    ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebView.mm 
    ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebViewEditorDelegate.mm
  )
  set_source_files_properties(${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebView.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
  # because the file hierarchy needs preserving, I did this the KISS way - but maybe some of the existing iPlug stuff has a more elegant way to do this
  set(WEB_RESOURCES 
    ${PROJECT_DIR}/resources/web/index.html
    ${PROJECT_DIR}/resources/web/script.js
    ${PROJECT_DIR}/resources/web/webaudio-controls.js
  )

  set_source_files_properties(${PROJECT_DIR}/resources/web/index.html PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")
  set_source_files_properties(${PROJECT_DIR}/resources/web/script.js PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")
  set_source_files_properties(${PROJECT_DIR}/resources/web/webaudio-controls.js PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")

endif()

#find_package(iPlug2 REQUIRED COMPONENTS Skia)
set(SRC_FILES
  "${PROJECT_DIR}/config.h"
  "${PROJECT_DIR}/IPlugWebUI.h"
  "${PROJECT_DIR}/IPlugWebUI.cpp"
)

set(RESOURCES
)

# While not required, creating a base interface for includes and settings seems like a good idea.
add_library(_base INTERFACE)
# iplug_target_add() is a shorthand function for adding sources and include directories,
# linking libraries, adding resources, setting compile options, etc.
iplug_target_add(_base INTERFACE
  SOURCE ${IWEBVIEW_SOURCE} ${WEB_RESOURCES}
  INCLUDE ${PROJECT_DIR} ${IPLUG2_DIR}/Dependencies/Extras/nlohmann ${IPLUG2_DIR}/IPlug/Extras/WebView ${EXTRA_INCLUDE_DIRECTORIES} ${PROJECT_DIR}/resources
  LINK_DIR ${EXTRA_LINK_DIRECTORIES}
  LINK iPlug2_GL2 ${EXTRA_LINK_LIBRARIES}
  FEATURE cxx_std_17
  DEFINE WEBVIEW_EDITOR_DELEGATE NO_IGRAPHICS ${EXTRA_DEFINES}
)


# For typing convenience the TARGET name is put into a variable.
set(TARGET app)
add_executable(${TARGET} WIN32 MACOSX_BUNDLE ${SRC_FILES} )
iplug_target_add(${TARGET} PUBLIC LINK iPlug2_APP _base RESOURCE ${RESOURCES})
iplug_configure_target(${TARGET} app)
# target_link_libraries(${TARGET} PRIVATE ${EXTRA_LINK_LIBRARIES})
# target_compile_options(${TARGET} PRIVATE)

#set(TARGET vst2)
#add_library(${TARGET} MODULE ${SRC_FILES})
#iplug_target_add(${TARGET} PUBLIC LINK iPlug2_VST2 _base RESOURCE ${RESOURCES})
#iplug_configure_target(${TARGET} vst2)

set(TARGET vst3)
add_library(${TARGET} MODULE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC LINK iPlug2_VST3 _base RESOURCE ${RESOURCES})
iplug_configure_target(${TARGET} vst3)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(TARGET auv2)
  add_library(${TARGET} MODULE ${SRC_FILES})
  iplug_target_add(${TARGET} PUBLIC LINK iPlug2_AUv2 _base RESOURCE ${RESOURCES})
  iplug_configure_target(${TARGET} auv2)
endif()

if (WIN32)
  set(CMAKE_RC_FLAGS "/I${PROJECT_DIR}/resources /I${PROJECT_DIR}/resources/fonts /I${PROJECT_DIR}/resources/img ${CMAKE_RC_FLAGS}")
endif()
