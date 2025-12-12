#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# iOS Standalone App target configuration for iPlug2
# Creates a standalone iOS app that hosts the AUv3 plugin for testing

include(${CMAKE_CURRENT_LIST_DIR}/AUv3iOS.cmake)

# Define iOS App helper sources
set(IPLUG_IOS_APP_SOURCES
  ${IPLUG_DIR}/AUv3/iOSApp/main.m
  ${IPLUG_DIR}/AUv3/iOSApp/AppDelegate.m
  ${IPLUG_DIR}/AUv3/iOSApp/AppDelegate.h
  ${IPLUG_DIR}/AUv3/iOSApp/AppViewController.mm
  ${IPLUG_DIR}/AUv3/iOSApp/AppViewController.h
  ${IPLUG_DIR}/AUv3/iOSApp/IPlugAUPlayer.mm
  ${IPLUG_DIR}/AUv3/iOSApp/IPlugAUPlayer.h
)

function(iplug_configure_iosapp target project_name)
  # Determine iOS app icon name - examples use "ProjectiOSAppIcon", others may use "Project-iOS"
  set(IOS_APPICON_NAME "${project_name}iOSAppIcon")
  if(EXISTS "${PLUG_RESOURCES_DIR}/Images.xcassets/${project_name}-iOS.appiconset")
    set(IOS_APPICON_NAME "${project_name}-iOS")
  endif()

  # Link AUv3iOS interface for the app (same headers/frameworks as the plugin)
  target_link_libraries(${target} PUBLIC iPlug2::AUv3iOS)

  # Define OBJC_PREFIX for ObjC class name mangling (must match storyboard customClass)
  # Applied to all languages since Xcode doesn't properly handle OBJC/OBJCXX generator expressions
  target_compile_definitions(${target} PRIVATE OBJC_PREFIX=v${project_name})

  # Add iOS App helper sources
  target_sources(${target} PRIVATE ${IPLUG_IOS_APP_SOURCES})

  # Set ARC for ObjC/ObjC++ sources
  set(ARC_SOURCES
    ${IPLUG_DIR}/AUv3/iOSApp/main.m
    ${IPLUG_DIR}/AUv3/iOSApp/AppDelegate.m
    ${IPLUG_DIR}/AUv3/iOSApp/AppViewController.mm
    ${IPLUG_DIR}/AUv3/iOSApp/IPlugAUPlayer.mm
  )
  set_source_files_properties(${ARC_SOURCES}
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
  )

  # AUv3 wrapper sources also need ARC
  set(AUV3_SOURCES
    ${IPLUG_DIR}/AUv3/IPlugAUv3.mm
    ${IPLUG_DIR}/AUv3/IPlugAUAudioUnit.mm
    ${IPLUG_DIR}/AUv3/IPlugAUViewController.mm
  )
  set_source_files_properties(${AUV3_SOURCES}
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
  )

  # IGraphics sources that require ARC
  # Note: IGraphicsIOS.mm does NOT use ARC (uses manual release), only the view does
  set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)
  set(IGRAPHICS_ARC_SOURCES
    ${IGRAPHICS_DIR}/Drawing/IGraphicsNanoVG_src.m
    ${IGRAPHICS_DIR}/Platforms/IGraphicsIOS_view.mm
  )
  set_source_files_properties(${IGRAPHICS_ARC_SOURCES}
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
  )

  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-iOS-Info.plist
    OUTPUT_NAME "${project_name}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    # iOS App Xcode attributes
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.${project_name}"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "${IOS_APPICON_NAME}"
  )

  # Create PkgInfo for app bundle
  set(PKGINFO_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/write_pkginfo_${target}.cmake")
  file(WRITE ${PKGINFO_SCRIPT} "file(WRITE \"\${PKGINFO_PATH}\" \"APPL????\")")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="$<TARGET_BUNDLE_DIR:${target}>/PkgInfo"
      -P "${PKGINFO_SCRIPT}"
    COMMENT "Creating PkgInfo for ${project_name}.app (iOS)"
  )

  # Compile storyboards
  # Main storyboard
  set(MAIN_STORYBOARD ${PLUG_RESOURCES_DIR}/${project_name}-iOS.storyboard)
  if(EXISTS ${MAIN_STORYBOARD})
    if(XCODE)
      # Xcode handles storyboard compilation automatically
      target_sources(${target} PRIVATE ${MAIN_STORYBOARD})
      set_source_files_properties(${MAIN_STORYBOARD} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    else()
      # For other generators, compile storyboard manually
      set(MAIN_STORYBOARDC ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-iOS.storyboardc)
      add_custom_command(
        OUTPUT ${MAIN_STORYBOARDC}
        COMMAND ibtool --compile ${MAIN_STORYBOARDC} ${MAIN_STORYBOARD}
          --target-device iphone --target-device ipad
        DEPENDS ${MAIN_STORYBOARD}
        COMMENT "Compiling ${project_name}-iOS.storyboard"
      )
      target_sources(${target} PRIVATE ${MAIN_STORYBOARDC})
      set_source_files_properties(${MAIN_STORYBOARDC} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()
  endif()

  # Launch screen storyboard
  set(LAUNCH_STORYBOARD ${PLUG_RESOURCES_DIR}/${project_name}-iOS-LaunchScreen.storyboard)
  if(EXISTS ${LAUNCH_STORYBOARD})
    if(XCODE)
      target_sources(${target} PRIVATE ${LAUNCH_STORYBOARD})
      set_source_files_properties(${LAUNCH_STORYBOARD} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    else()
      set(LAUNCH_STORYBOARDC ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-iOS-LaunchScreen.storyboardc)
      add_custom_command(
        OUTPUT ${LAUNCH_STORYBOARDC}
        COMMAND ibtool --compile ${LAUNCH_STORYBOARDC} ${LAUNCH_STORYBOARD}
          --target-device iphone --target-device ipad
        DEPENDS ${LAUNCH_STORYBOARD}
        COMMENT "Compiling ${project_name}-iOS-LaunchScreen.storyboard"
      )
      target_sources(${target} PRIVATE ${LAUNCH_STORYBOARDC})
      set_source_files_properties(${LAUNCH_STORYBOARDC} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()
  endif()

  # AUv3 MainInterface storyboard - needed in app bundle for AppViewController to load
  # (AppViewController loads it with bundle:nil which searches main bundle)
  set(MAININTERFACE_STORYBOARD ${PLUG_RESOURCES_DIR}/${project_name}-iOS-MainInterface.storyboard)
  if(EXISTS ${MAININTERFACE_STORYBOARD})
    if(XCODE)
      target_sources(${target} PRIVATE ${MAININTERFACE_STORYBOARD})
      set_source_files_properties(${MAININTERFACE_STORYBOARD} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    else()
      set(MAININTERFACE_STORYBOARDC ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-iOS-MainInterface-app.storyboardc)
      add_custom_command(
        OUTPUT ${MAININTERFACE_STORYBOARDC}
        COMMAND ibtool --compile ${MAININTERFACE_STORYBOARDC} ${MAININTERFACE_STORYBOARD}
          --target-device iphone --target-device ipad
        DEPENDS ${MAININTERFACE_STORYBOARD}
        COMMENT "Compiling ${project_name}-iOS-MainInterface.storyboard for app"
      )
      target_sources(${target} PRIVATE ${MAININTERFACE_STORYBOARDC})
      set_source_files_properties(${MAININTERFACE_STORYBOARDC} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()
  endif()

  # Add asset catalog for app icons
  set(ASSET_CATALOG ${PLUG_RESOURCES_DIR}/Images.xcassets)
  if(EXISTS ${ASSET_CATALOG})
    if(XCODE)
      target_sources(${target} PRIVATE ${ASSET_CATALOG})
      set_source_files_properties(${ASSET_CATALOG} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()
    # For non-Xcode builds, asset catalogs need actool compilation (complex, skip for now)
  endif()
endfunction()
