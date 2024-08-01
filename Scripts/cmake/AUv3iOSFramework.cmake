#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AUv3 Framework target configuration for iPlug2 iOS
# The framework contains the actual plugin code and is loaded in-process

include(${CMAKE_CURRENT_LIST_DIR}/AUv3iOS.cmake)

function(iplug_configure_auv3iosframework target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::AUv3iOS)

  # Define OBJC_PREFIX for ObjC class name mangling (must match storyboard customClass)
  # Applied to all languages since Xcode doesn't properly handle OBJC/OBJCXX generator expressions
  target_compile_definitions(${target} PRIVATE OBJC_PREFIX=v${project_name})

  # AUv3 requires ARC for its Objective-C++ sources
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

  # Generate Info.plist with correct executable name
  set(FRAMEWORK_PLIST "${CMAKE_CURRENT_BINARY_DIR}/${project_name}-iOS-AUv3Framework-Info.plist")
  file(WRITE ${FRAMEWORK_PLIST}
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleDisplayName</key>
	<string>${project_name}AU</string>
	<key>CFBundleExecutable</key>
	<string>${project_name}AU</string>
	<key>CFBundleIdentifier</key>
	<string>com.AcmeInc.${project_name}.AUv3Framework</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>${project_name}AU</string>
	<key>CFBundlePackageType</key>
	<string>FMWK</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleVersion</key>
	<string>1.0.0</string>
</dict>
</plist>
")

  set_target_properties(${target} PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_INFO_PLIST ${FRAMEWORK_PLIST}
    OUTPUT_NAME "${project_name}AU"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    # iOS framework settings
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
    XCODE_ATTRIBUTE_DEFINES_MODULE "YES"
    XCODE_ATTRIBUTE_DYLIB_INSTALL_NAME_BASE "@rpath"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.${project_name}.AUv3Framework"
  )

  # Create PkgInfo file for framework bundle (FMWK = framework)
  set(PKGINFO_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/write_pkginfo_${target}.cmake")
  file(WRITE ${PKGINFO_SCRIPT} "file(WRITE \"\${PKGINFO_PATH}\" \"FMWK????\")")

  if(XCODE)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="$<TARGET_BUNDLE_DIR:${target}>/PkgInfo"
        -P "${PKGINFO_SCRIPT}"
      COMMENT "Creating PkgInfo for ${project_name}AU.framework (iOS)"
    )
  else()
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="$<TARGET_BUNDLE_DIR:${target}>/PkgInfo"
        -P "${PKGINFO_SCRIPT}"
      COMMENT "Creating PkgInfo for ${project_name}AU.framework (iOS)"
    )
  endif()

  # Add headers to framework (umbrella + public headers)
  # Check for umbrella header - support both naming conventions
  if(EXISTS "${PLUG_RESOURCES_DIR}/${project_name}AU.h")
    set(UMBRELLA_HEADER "${PLUG_RESOURCES_DIR}/${project_name}AU.h")
  elseif(EXISTS "${PLUG_RESOURCES_DIR}/AUv3Framework.h")
    set(UMBRELLA_HEADER "${PLUG_RESOURCES_DIR}/AUv3Framework.h")
  else()
    message(WARNING "No AUv3 umbrella header found for ${project_name}")
    set(UMBRELLA_HEADER "")
  endif()

  set(FRAMEWORK_HEADERS
    ${UMBRELLA_HEADER}
    ${IPLUG_DIR}/AUv3/IPlugAUViewController.h
    ${IPLUG_DIR}/AUv3/IPlugAUAudioUnit.h
  )
  target_sources(${target} PRIVATE ${FRAMEWORK_HEADERS})
  set_source_files_properties(${FRAMEWORK_HEADERS}
    PROPERTIES MACOSX_PACKAGE_LOCATION Headers
  )

  # Compile storyboard to storyboardc and add to framework Resources
  # iOS uses storyboards instead of XIBs
  set(AU_VIEW_STORYBOARD ${PLUG_RESOURCES_DIR}/${project_name}-iOS-MainInterface.storyboard)
  if(EXISTS ${AU_VIEW_STORYBOARD})
    set(AU_VIEW_STORYBOARDC ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-iOS-MainInterface.storyboardc)
    add_custom_command(
      OUTPUT ${AU_VIEW_STORYBOARDC}
      COMMAND ibtool --compile ${AU_VIEW_STORYBOARDC} ${AU_VIEW_STORYBOARD}
        --target-device iphone --target-device ipad
      DEPENDS ${AU_VIEW_STORYBOARD}
      COMMENT "Compiling ${project_name}-iOS-MainInterface.storyboard"
    )
    # For Xcode, set the storyboard as a resource that gets compiled automatically
    if(XCODE)
      target_sources(${target} PRIVATE ${AU_VIEW_STORYBOARD})
      set_source_files_properties(${AU_VIEW_STORYBOARD} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    else()
      # For other generators, use the compiled storyboardc
      target_sources(${target} PRIVATE ${AU_VIEW_STORYBOARDC})
      set_source_files_properties(${AU_VIEW_STORYBOARDC} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()
  endif()
endfunction()
