#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# Configuration for IPlug Core

if(NOT TARGET iPlug2::IPlug)
  add_library(iPlug2::IPlug INTERFACE IMPORTED)
  
  set(IPLUG_DIR ${IPLUG2_DIR}/IPlug)
  set(WDL_DIR ${IPLUG2_DIR}/WDL)
  set(DEPS_DIR ${IPLUG2_DIR}/Dependencies)
  set(IPLUG_DEPS_DIR ${DEPS_DIR}/IPlug)

  set(IPLUG_SRC
    ${IPLUG_DIR}/IPlugAPIBase.h
    ${IPLUG_DIR}/IPlugAPIBase.cpp
    ${IPLUG_DIR}/IPlugConstants.h
    ${IPLUG_DIR}/IPlugEditorDelegate.h
    ${IPLUG_DIR}/IPlugLogger.h
    ${IPLUG_DIR}/IPlugMidi.h
    ${IPLUG_DIR}/IPlugParameter.h
    ${IPLUG_DIR}/IPlugParameter.cpp
    ${IPLUG_DIR}/IPlugPaths.h
    ${IPLUG_DIR}/IPlugPaths.cpp
    ${IPLUG_DIR}/IPlugPlatform.h
    ${IPLUG_DIR}/IPlugPluginBase.h
    ${IPLUG_DIR}/IPlugPluginBase.cpp
    ${IPLUG_DIR}/IPlugProcessor.h
    ${IPLUG_DIR}/IPlugProcessor.cpp
    ${IPLUG_DIR}/IPlugQueue.h
    ${IPLUG_DIR}/IPlugStructs.h
    ${IPLUG_DIR}/IPlugTimer.h
    ${IPLUG_DIR}/IPlugTimer.cpp
    ${IPLUG_DIR}/IPlugUtilities.h
  )

  if(APPLE)
    list(APPEND IPLUG_SRC ${IPLUG_DIR}/IPlugPaths.mm)
  endif()
  
  target_sources(iPlug2::IPlug INTERFACE ${IPLUG_SRC})
  
  target_include_directories(iPlug2::IPlug INTERFACE 
    ${IPLUG_DIR}
    ${WDL_DIR}
    ${WDL_DIR}/libpng
    ${WDL_DIR}/zlib
    ${IPLUG_DIR}/Extras
  )
  
  target_compile_definitions(iPlug2::IPlug INTERFACE
    NOMINMAX
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
  )
  
  if(MSVC)
    target_compile_definitions(iPlug2::IPlug INTERFACE
      _CRT_SECURE_NO_WARNINGS
      _CRT_SECURE_NO_DEPRECATE
      _CRT_NONSTDC_NO_DEPRECATE
      _MBCS
    )
    target_compile_options(iPlug2::IPlug INTERFACE 
      /wd4250 /wd4018 /wd4267 /wd4068 
      /MT$<$<CONFIG:Debug>:d>
    )
  endif()
  
  if(WIN32)
    target_link_libraries(iPlug2::IPlug INTERFACE 
      Shlwapi.lib
      comctl32.lib
      wininet.lib
    )
  elseif(APPLE)
    target_link_libraries(iPlug2::IPlug INTERFACE
      "-framework CoreData"
      "-framework CoreFoundation"
      "-framework CoreServices"
      "-framework Foundation"
    )
  elseif(UNIX AND NOT APPLE)
    message("Error - Linux not yet supported")
  endif()
  
  if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
    target_link_options(iPlug2::IPlug INTERFACE -Wl)
  endif()
endif()