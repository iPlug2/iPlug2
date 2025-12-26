#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AUv3 (Audio Unit v3) shared definitions for iPlug2
# macOS only - defines iPlug2::AUv3 interface and helper functions
# Included by AUv3Framework.cmake and AUv3Appex.cmake

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::AUv3)
  if(NOT APPLE)
    message(WARNING "AUv3 is only supported on macOS")
    return()
  endif()

  add_library(iPlug2::AUv3 INTERFACE IMPORTED)

  # iPlug2 AUv3 wrapper sources
  set(AUV3_IPLUG_SRC
    ${IPLUG_DIR}/AUv3/IPlugAUv3.mm
    ${IPLUG_DIR}/AUv3/IPlugAUAudioUnit.mm
    ${IPLUG_DIR}/AUv3/IPlugAUViewController.mm
  )

  target_sources(iPlug2::AUv3 INTERFACE ${AUV3_IPLUG_SRC})

  # Note: ARC flag must be set by consuming targets, not here
  # (set_source_files_properties doesn't propagate through INTERFACE targets)

  target_include_directories(iPlug2::AUv3 INTERFACE
    ${IPLUG_DIR}/AUv3
  )

  target_compile_definitions(iPlug2::AUv3 INTERFACE
    AUv3_API
    IPLUG_EDITOR=1
    IPLUG_DSP=1
  )

  # AUv3 headers need Cocoa for NSView on macOS (only for ObjC++ files)
  target_compile_options(iPlug2::AUv3 INTERFACE
    "$<$<COMPILE_LANGUAGE:OBJCXX>:SHELL:-include Cocoa/Cocoa.h>"
  )

  target_link_libraries(iPlug2::AUv3 INTERFACE
    "-framework AudioToolbox"
    "-framework AVFoundation"
    "-framework CoreAudioKit"
    "-framework CoreMIDI"
    "-framework Cocoa"
    iPlug2::IPlug
  )
endif()

# ============================================================================
# Get or generate the appex source file
# Returns the path in APPEX_SOURCE_OUT variable
# ============================================================================
function(iplug_get_auv3appex_source project_name APPEX_SOURCE_OUT)
  # Check for existing appex source with either naming convention
  if(EXISTS "${PLUG_RESOURCES_DIR}/${project_name}AUv3Appex.m")
    set(${APPEX_SOURCE_OUT} "${PLUG_RESOURCES_DIR}/${project_name}AUv3Appex.m" PARENT_SCOPE)
  else()
    # Generate a minimal appex source file
    set(GENERATED_APPEX "${CMAKE_CURRENT_BINARY_DIR}/${project_name}AUv3Appex.m")
    file(WRITE ${GENERATED_APPEX}
"// Auto-generated AUv3 appex source
#import <Foundation/Foundation.h>
// Dummy function to ensure linking
void AUv3AppexDummy(void) {}
")
    set(${APPEX_SOURCE_OUT} "${GENERATED_APPEX}" PARENT_SCOPE)
  endif()
endfunction()

# ============================================================================
# Embed AUv3 appex and framework in an existing APP target
# Structure matches Xcode: framework at App/Contents/Frameworks/,
# appex at App/Contents/PlugIns/
# ============================================================================
function(iplug_embed_auv3_in_app app_target project_name)
  set(appex_target ${project_name}AUv3-appex)
  set(framework_target ${project_name}AU-framework)

  add_dependencies(${app_target} ${appex_target})

  # Get deployment path for APP (if deployment is enabled)
  # This must happen AFTER signing, so we include it in this function
  include(${IPLUG2_CMAKE_DIR}/Deploy.cmake)
  iplug_get_default_deploy_path(APP)
  set(deploy_path "${IPLUG_DEPLOY_PATH_APP}")

  # Post-build: Embed framework at app level, then embed appex, then sign, then deploy
  # Use cp -R to preserve symlinks in framework bundle
  # Note: Paths use $<CONFIG> generator expression for Xcode compatibility
  if(XCODE)
    if(IPLUG2_DEVELOPMENT_TEAM)
      add_custom_command(TARGET ${app_target} POST_BUILD
        # Copy framework to App/Contents/Frameworks/ (preserving symlinks)
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/Frameworks"
        COMMAND cp -R
          "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${project_name}AU.framework"
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/Frameworks/"
        # Copy appex to App/Contents/PlugIns/
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${project_name}AUv3.appex"
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns/${project_name}AUv3.appex"
        # Sign embedded components with entitlements, then sign the app
        # Re-sign the embedded appex with entitlements (--deep strips them)
        COMMAND codesign --force --sign "Apple Development" -o runtime --timestamp=none
          --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/projects/${project_name}-macOS.entitlements"
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns/${project_name}AUv3.appex"
        # Sign the app with hardened runtime (must be after embedded content is signed)
        COMMAND codesign --force --sign "Apple Development" -o runtime --timestamp=none
          "$<TARGET_BUNDLE_DIR:${app_target}>"
        COMMENT "Embedding AUv3 and signing ${project_name}.app"
      )
      # Deploy signed app and register with Launch Services (separate command to run after the above)
      if(IPLUG_DEPLOY_PLUGINS AND NOT "${deploy_path}" STREQUAL "")
        add_custom_command(TARGET ${app_target} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E echo "[iPlug2] Deploying signed app: ${deploy_path}/${project_name}.app"
          COMMAND ${CMAKE_COMMAND} -E make_directory "${deploy_path}"
          COMMAND ${CMAKE_COMMAND} -E rm -rf "${deploy_path}/${project_name}.app"
          COMMAND cp -R "$<TARGET_BUNDLE_DIR:${app_target}>" "${deploy_path}/"
          # Register with Launch Services to enable AUv3 discovery
          COMMAND /System/Library/Frameworks/CoreServices.framework/Versions/Current/Frameworks/LaunchServices.framework/Versions/Current/Support/lsregister
            -f -R -trusted "${deploy_path}/${project_name}.app"
          COMMENT "Deploying ${project_name}.app with AUv3"
        )
      endif()
    else()
      add_custom_command(TARGET ${app_target} POST_BUILD
        # Copy framework to App/Contents/Frameworks/ (preserving symlinks)
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/Frameworks"
        COMMAND cp -R
          "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${project_name}AU.framework"
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/Frameworks/"
        # Copy appex to App/Contents/PlugIns/
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_BINARY_DIR}/out/$<CONFIG>/${project_name}AUv3.appex"
          "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns/${project_name}AUv3.appex"
        COMMENT "Embedding AUv3 (framework + appex) in ${project_name}.app"
      )
    endif()
  else()
    add_custom_command(TARGET ${app_target} POST_BUILD
      # Copy framework to App/Contents/Frameworks/ (preserving symlinks)
      COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/Frameworks"
      COMMAND cp -R
        "${CMAKE_BINARY_DIR}/out/${project_name}AU.framework"
        "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/Frameworks/"
      # Copy appex to App/Contents/PlugIns/
      COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/out/${project_name}AUv3.appex"
        "$<TARGET_BUNDLE_DIR:${app_target}>/Contents/PlugIns/${project_name}AUv3.appex"
      COMMENT "Embedding AUv3 (framework + appex) in ${project_name}.app"
    )
  endif()
endfunction()
